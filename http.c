#include "http.h"
#include "constants.h"
#include "encoding_prefs.h"
#include "files.h"
#include "timestamp.h"
#include "url.h"
#include "util.h"
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>
#include <time.h>

#include <sys/sendfile.h>
#include <sys/stat.h>
#include <fcntl.h>

#define BUF_SIZE 8192

#define HTTP1_1     1
#define KEEP_ALIVE  2

typedef struct {
	//read and write buffers
	char recv_buf[BUF_SIZE];
	ssize_t rsize;
	char *rpos;
	//char send_buf[BUF_SIZE];
	//size_t sleft;
	//char *spos;
	int fd;

} http_connection_t;

typedef struct {
	http_method_t method;
	char request_line[BUF_SIZE];
	char *path;
	//length needed for when URL has non trailing null bytes
	int path_length;
	char *query;
	int query_length;

	//status, content_length are -1 if not set
	int status;
	int content_length;
	int options;

	//stores timestamp in If-Modified-Since header or 0 if header not present
	time_t if_modified_since;

	//client encoding preferences
	encoding_prefs_t encoding_prefs;
} http_request_t;

static void init_connection(http_connection_t *con, int fd) {
	con->rsize = 0;
	con->rpos = con->recv_buf;
	con->fd = fd;
}

static void init_request(http_request_t *request) {
	request->method = UNKNOWN;
	request->path = NULL;
	request->path_length = -1;
	request->query = NULL;
	request->query_length = -1;
	request->status = -1;
	request->options = 0;
	request->content_length = -1;
	request->if_modified_since = (time_t)0;
	//Note: google does not remember accepted encodings across requests
	//probably safe to do so
	init_encoding_prefs(&request->encoding_prefs);
}

//check that http_connection_t is not null and properly formed
static void assert_connection_invariants_hold(const http_connection_t *con) {
	assert(con);
	assert(con->rpos);
	assert(con->rpos >= con->recv_buf);
	assert(con->rpos < con->recv_buf + BUF_SIZE);
	assert(con->rsize <= con->recv_buf + BUF_SIZE - con->rpos);
	assert(con->fd >= 0);
	//get around unused parameter warning
	if(!con) {
		assert(0);
	}
}

static void print_request(const http_request_t *request) {
	const char *methods[] = {"GET", "POST", "HEAD"};

	printf("Method: %s\n", methods[(int)request->method]);
	printf("path: %s\n", request->path);
	printf("path length: %d\n", request->path_length);
	printf("query: %s\n", request->query);
	printf("query length: %d\n", request->query_length);
	printf("Version: HTTP/1.%c\n", request->options & HTTP1_1 ? '1' : '0');
	printf("Keep Alive: %s\n", request->options & KEEP_ALIVE ? "true" : "false");
	printf("Content-Length: %d\n", request->content_length);
	printf("Accept-Encoding: gzip: %d, identity: %d, *: %d\n",
		request->encoding_prefs.gzip,
		request->encoding_prefs.identity,
		request->encoding_prefs.catch_all);
	char buf[32];
	time2str(&(request->if_modified_since), buf, 32);
	printf("If-Modified-Since: %s\n", buf);
}
//sends full contents of buffer returns -1 on failure 0 on success
static int send_all(int fd, const char *buf, size_t len) {
	assert(buf);
	ssize_t r;
	while(len) {
		while((r = send(fd, buf, len, 0)) == -1 && errno == EINTR);
		if(r == -1) {
			return -1;
		}
		buf += r;
		len -= (size_t)r;
	}
	return 0;
}

//reads a character
//returns 1 on success, 0 on eof, -1 on failure
static int recv_byte(http_connection_t *con, char *byte) {
	assert_connection_invariants_hold(con);
	assert(byte);

	//if recv buffer is empty get more data
	if(con->rsize < 1) {
		con->rpos = con->recv_buf;

		while((con->rsize = recv(con->fd, con->recv_buf, BUF_SIZE, 0)) == -1 &&
				errno == EINTR);

		//read failure
		if(con->rsize == -1) {
			con->rsize = 0;
			return -1;
		} else if(con->rsize == 0) {
			return 0;
		}
	}
	con->rsize -= 1;
	*byte = *con->rpos++;
	return 1;
}

//reads until reaching LF, EOF, or buf is full (including null terminator)
//returns length of line, -1 on io error, -2 if line exceeds max_len
static ssize_t recv_line(http_connection_t *con, char *buf, size_t max_len) {
	size_t len = 0;
	int r;
	while(len < max_len) {
		r = recv_byte(con, buf + len);
		//io errors
		if(r == -1) {
			return -1;
		}
		//check for end of line
		if(r == 0 || buf[len++] == '\n') {
			break;
		}
	}
	//line too long (length includes null terminator)
	if(len >= max_len) {
		return -2;
	}
	buf[len] = '\0';
	return (ssize_t)len;
}

static const char *reason_phrase(int status) {
	size_t es = sizeof(INFO[0]);
	if(status < 100) return NULL;

	size_t s = (size_t)status; //to suppress signed unsigned comparison warnings

	//if in lookup table => return it
	//otherwise default to x00 reason phrase

	//1xx: Informational
	if(s < 100 + sizeof(INFO)/es) return INFO[s - 100];
	if(s < 200) return INFO[0];
	//2xx: Success
	if(s < 200 + sizeof(SUCCESS)/es) return SUCCESS[s - 200];
	if(s < 300) return SUCCESS[0];
	//3xx: Redirection
	if(s < 300 + sizeof(REDIRECTION)/es) return REDIRECTION[s - 300];
	if(s < 400) return REDIRECTION[0];
	//4xx: Client Error
	if(s < 400 + sizeof(CLIENT_ERROR)/es) return CLIENT_ERROR[s - 400];
	if(s < 500) return CLIENT_ERROR[0];
	//5xx: Server Error
	if(s < 500 + sizeof(SERVER_ERROR)/es) return SERVER_ERROR[s - 500];
	if(s < 600) return SERVER_ERROR[0];

	return NULL;
}

//if reason phrase is null, a generic one is used instead
//replies with error
static void reply_with_error(http_connection_t *con, int status_code) {
	char *buf = con->recv_buf;
	int r;
	assert(status_code >= 100 && status_code < 600);
	r = snprintf(buf, BUF_SIZE - 1,
		"HTTP/1.1 %d %s\r\nContent-Length: %d\r\n\r\n<html>error: %d</html>",
		status_code, reason_phrase(status_code), 23, status_code);
	if(r > 0) {
		send_all(con->fd, buf, (size_t)r);
	}
}

//determine method + start of URL
static void parse_method(http_request_t *request) {
	char *request_line = request->request_line;
	switch(request_line[0]) {
		case 'G':
			if(strncmp("ET ", request_line + 1, 3) == 0) {
				request->path = request_line + 4;
				request->method = GET;
				return;
			}
		break;
		case 'P':
			if(strncmp("OST ", request_line + 1, 4) == 0) {
				request->path = request_line + 5;
				request->method = POST;
				return;
			}
		break;
		case 'H':
			if(strncmp("EAD ", request_line + 1, 4) == 0) {
				request->path = request_line + 5;
				request->method = HEAD;
				return;
			}
		break;
	}
	request->method = UNKNOWN;
}


//reads http request line
//request line = method SP request-target SP HTTP-version CRLF
//return -1 or io error -2 on errors that warrent a responce
static int read_request_line(http_connection_t *con, http_request_t *request) {
	ssize_t r;
	char *version_str;

	//read line
	r = recv_line(con, request->request_line, BUF_SIZE);
	//io error, eof, does not end with newline
	if(r == -1 || r == 0 || request->request_line[r - 1] != '\n') return -1;

	//request line too long
	if(r == -2) {
		//request line too long (technically 416 is URL too long)
		fprintf(stderr, "request line too long\n");
		request->status = 413; //TODO: #define PAYLOAD_TO_LARGE 413
		return -2;
	}
	remove_endline(request->request_line, (size_t)r);
	printf("%s\n", request->request_line);

	//determine method + start of URL
	parse_method(request);
	if(request->method == UNKNOWN) {
		//not implemented
		request->status = 501;
		return -2;
	}

	//find end of URL
	r = find_first(request->path, ' ');
	if(r <= 0) {
		//empty URL or no space ending URL
		puts("empty URL");
		request->status = 400;
		return -2;
	}
	request->path[r] = '\0';
	version_str = request->path + r + 1;

	//TODO: move URL parsing into a separate function
	//must separate path and query parts before decoding
	r = find_first(request->path, '?');
	if(r >= 0) {
		request->path[r] = '\0';
		request->query = request->path + r + 1;
	}

	//percent decode path
	if(percent_decode(request->path, &(request->path_length)) == -1) {
		//invalid percent encoding (what error code is this???)
		request->status = 404;
		return -2;
	}
	//TODO: cannot percent decode query until after parameters have been parsed
	
	//determine version
	if(strncmp("HTTP/1.", version_str, 7)) {
		request->status = 400;
		return -2;
	}
	switch(version_str[7]) {
		case '0':
			request->options = 0;
		break;
		case '1':
			//HTTP/1.1 => keep alive by default
			request->options = KEEP_ALIVE | HTTP1_1;
		break;
		default:
			//HTTP version not supported
			request->status = 505;
			return -2;
	}
	//check that there is nothing after http version
	if(version_str[8] != '\0') {
		//unexpected trailing characters
		request->status = 400;
		return -2;
	}
	return 0;
}

static int handle_header(http_request_t *request, const char *name, char *value) {
	//Accept-Encoding - what compression algorithms client supports
	//Connection
	//Content-Encoding - what algorithm the message body is compressed with
	//Content-Length
	//If-Modified-Since
	//Range - part of document client wants
	//Transfer-Encoding
	//
	//TODO: content-length not allowed if transfer encoding is set

	//determine header type
	switch(name[0]) {
		case 'A':
		case 'a':
			if(!strcasecmp(name + 1, "ccept-encoding")) {
				//handle Accept-Encoding
				if(parse_accepted_encodings(&(request->encoding_prefs),
						value) == -1) {
					request->status = 400;
					return -1;
				}
			}
		break;
		case 'C':
		case 'c':
			if(!strcasecmp(name + 1, "onnection")) {
				//handle Connection:
				if(!strcasecmp(value, "keep-alive")) {
					request->options |= KEEP_ALIVE;
				} else {
					request->options &= (~KEEP_ALIVE);
					//TODO: should I return error if value is not close?
				}
			} else if(!strcasecmp(name + 1, "ontent-length")) {
				puts("content length");
				//handle Content-Length:
				request->content_length = parse_uint(value);
				if(request->content_length < 0) {
					request->status = 400;
					return -1;
				}
			} else if(!strcasecmp(name + 1, "ontent-encoding")) {
				//TODO: check for gzip
			}
		break;
		case 'I':
		case 'i':
			if(!strcasecmp(name + 1, "f-modified-since")) {
				//handle If-Modified-Since:
				if(str2time(value, &(request->if_modified_since)) == -1) {
					request->if_modified_since = (time_t)0;
				}
			}
		case 'R':
		case 'r':
			if(!strcasecmp(name + 1, "ange")) {
				//handle Range:
			}
		break;
		case 'T':
		case 't':
			if(!strcasecmp(name + 1, "ransfer-encoding")) {
				//unrecognized transfer encoding => 501 (not implemented)
				//parse list of encodings
				//TODO: handle chuncked
				//
				//chuncked, gzip, identity
			}
		break;
	}
	return 0;
}

//reads headers and records important values
//return -1 on read error, -2 on other errors
static int read_headers(http_connection_t *con, http_request_t *request) {
	//while True
	//	read line
	//	eof or empty line -> end of headers
	//	parse header
	//	if it's something we care about take note of value
	ssize_t r;
	char buf[BUF_SIZE];
	char *name;
	char *value;
	for(;;) {
		//read line
		r = recv_line(con, buf, BUF_SIZE);

		//io error
		if(r == -1) return -1;

		//line too long
		if(r == -2) {
			request->status = 413;
			return -2;
		}
		//TODO: check for eof
		remove_trailing_whitespace(buf, (size_t)r);

		//empty line => end of headers
		if(strcmp(buf, "") == 0) {
			break;
		}

		//parse header
		//header-field   = field-name ":" OWS field-value OWS
		name = buf;

		//find end of key
		r = find_first(name, ':');

		//malformed header
		if(r < 1) {
			request->status = 400;
			return -2;
		}

		name[r] = '\0';

		//skip leading whitespace (trailing whitespace already removed)
		value = name + r + 1;
		while(*value == ' ' || *value == '\t') ++value;

		fprintf(stderr, "Name: \"%s\", Value: \"%s\"\n", name, value);

		if(handle_header(request, name, value)  < 0) {
			return -1;
		}
	}

	return 0;
}

void handle_http_connection(int fd) {
	http_connection_t con;
	http_request_t request;
	init_connection(&con, fd);
	int r;
	int length;
	time_t mtime;

	do {
		init_request(&request);

		//read_request_line
		r = read_request_line(&con, &request);

		//io error
		if(r == -1) break;

		//client error or unsupported method
		if(r < 0) {
			reply_with_error(&con, request.status);
			break;
		}

		if(read_headers(&con, &request) < 0) {
			reply_with_error(&con, request.status);
			break;
		}

		print_request(&request);

		//read body (if any)
		//TODO: should I block bodies for GET requests???
		//TODO: which settings (ex: Accept-Encodings) are reset between requests??

		//TODO: put this elsewhere
		if(request.path[0] == '/') {
			++request.path;
		}
		fprintf(stderr, "GET path: %s\n", request.path);

		if(!safe_path(request.path)) {
			request.status = 404;
			reply_with_error(&con, request.status);
			goto request_end;
		}

		//find file, content length, mtime
		fd = web_open(request.path, &length, &mtime);
		if(fd == -1) {
			request.status = 404;
			reply_with_error(&con, request.status);
			goto request_end;
		}

		//format Last-Modified timestamp
		char time_stamp_buf[32];
		time2str(&mtime, time_stamp_buf, 32);

		//check if file has been modified
		printf("lm: %lu ifs: %lu\n", (unsigned long)mtime, (unsigned long)request.if_modified_since);
		if(mtime <= request.if_modified_since) {
			request.status = 304;
		} else {
			request.status = 200;
		}

		//send headers
		char *buf = con.recv_buf;

		//TODO: set TCP_CORK option before sending header (to minimize number of packets send)
		r = snprintf(buf, BUF_SIZE - 1,
			"HTTP/1.1 %d %s\r\nContent-Length: %lu\r\nLast-Modified: %s\r\n\r\n",
			request.status, reason_phrase(request.status),
			(unsigned long)length, time_stamp_buf);
		assert(r > 0);

		if(send_all(con.fd, buf, (size_t)r) < 0) {
			//could not send header :(
			break;
		}

		//don't send body for HEAD requests of for unmodified files
		if(request.method == HEAD || request.status == 304) {
			goto request_end;
		}

		//TODO: see limitations in man page (only transfers up to certain amount)
		//TODO: fid must point to regular file (not symlink???)
		//also look at splice(2)
		if(sendfile(con.fd, fd, NULL, (size_t)length) != length) {
			//did not send whole file :(
			break;
		}
request_end:;
		
	} while(request.options & KEEP_ALIVE);
}
