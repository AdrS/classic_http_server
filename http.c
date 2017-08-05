#include "http.h"
#include "constants.h"
#include <assert.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>

#define BUF_SIZE 1024
#define MAX_URL 1024

#define HTTP1_1     1
#define KEEP_ALIVE  2
#define ACCEPT_GZIP 4

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

//TODO: write structs for each "important" request header

typedef struct {
	http_method_t method;
	char request_line[BUF_SIZE];
	char *url;
	int status;
	int options;
} http_request_t;

static void init_connection(http_connection_t *con, int fd) {
	con->rsize = 0;
	con->rpos = con->recv_buf;
	con->fd = fd;
}

static void init_request(http_request_t *request) {
	request->method = UNKNOWN;
	request->url = NULL;
	request->status = 0;
	request->options = 0;
}

//check that http_connection_t is not null and properly formed
static void assert_connection_invariants_hold(const http_connection_t *con) {
	assert(con);
	assert(con->rpos);
	assert(con->rpos >= con->recv_buf);
	assert(con->rpos < con->recv_buf + BUF_SIZE);
	assert(con->rsize <= con->recv_buf + BUF_SIZE - con->rpos);
	assert(con->fd >= 0);
}

static void print_request(const http_request_t *request) {
	const char *methods[] = {"GET", "POST", "HEAD"};

	printf("Method: %s\n", methods[(int)request->method]);
	printf("Url: %s\n", request->url);
	printf("Version: HTTP/1.%c\n", request->options & HTTP1_1 ? '1' : '0');
	printf("Keep Alive: %s\n", request->options & KEEP_ALIVE ? "true" : "false");
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
		len -= r;
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
	return len;
}

//replaces trailing newline (CRLF or LF) with null terminator
static void remove_endline(char *line, size_t len) {
	//there should actually be a endline
	assert(line[len] == '\0' && line[len - 1] == '\n');
	if(len > 1 && line[len - 2] == '\r') {
		//CRLF
		line[len - 2] = '\0';
	} else {
		//LF
		line[len - 1] = '\0';
	}
}

static int is_trailing_space(char c) {
	return c == '\n' || c == '\r' || c == ' ' || c == '\t' || c == '\0';
}

//remove trailing whitespace (ie: ' ' \t \r \n)
static void remove_trailing_whitespace(char *line, size_t len) {
	int i = ((int)len) - 1;
	while(i >= 0 && is_trailing_space(line[i])) {
		line[i--] = '\0';
	}
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
	r = snprintf(buf, BUF_SIZE - 1, "HTTP/1.1 %d %s\r\n\r\n",
		status_code, reason_phrase(status_code));
	if(r > 0) {
		send_all(con->fd, buf, r);
	}
}

//determine method + start of url
static void parse_method(http_request_t *request) {
	char *request_line = request->request_line;
	switch(request_line[0]) {
		case 'G':
			if(strncmp("ET ", request_line + 1, 3) == 0) {
				request->url = request_line + 4;
				request->method = GET;
				return;
			}
		break;
		case 'P':
			if(strncmp("OST ", request_line + 1, 4) == 0) {
				request->url = request_line + 5;
				request->method = POST;
				return;
			}
		break;
		case 'H':
			if(strncmp("EAD ", request_line + 1, 4) == 0) {
				request->url = request_line + 5;
				request->method = HEAD;
				return;
			}
		break;
	}
	request->method = UNKNOWN;
}

//returns index of first occurance of c or -1 or c is not in string
static int find_first(const char *line, char c) {
	int i;
	for(i = 0; line[i]; ++i) {
		if(line[i] == c) {
			return i;
		}
	}
	return -1;
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
	remove_endline(request->request_line, r);
	printf("%s\n", request->request_line);

	//determine method + start of url
	parse_method(request);
	if(request->method == UNKNOWN) {
		//not implemented
		fprintf(stderr, "not implemented\n");
		request->status = 400;
		return -2;
	}

	//parse url
	r = find_first(request->url, ' ');
	if(r <= 0) {
		//empty url or no space endig url
		fprintf(stderr, "bad url\n");
		request->status = 400;
		return -2;
	}
	request->url[r] = '\0';

	//percent decode url
	//TODO:
	
	//determine version
	version_str = request->url + r + 1;
	if(strncmp("HTTP/1.", version_str, 7)) {
		fprintf(stderr, "bad version\n");
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
			fprintf(stderr, "unsupported version");
			request->status = 505;
			return -2;
	}
	//check that there is nothing after http version
	if(version_str[8] != '\0') {
		fprintf(stderr, "unexpected trailing characters\n");
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
	//Range - part of document client wants
	//Transfer-Encoding
	//
	//TODO: normcase headers
	//check for Connection and Keep-Alive
	//connection: close  -> close
	//connection: keep-alive  -> keep-alive
	//HTTP/1.1 no connection header -> keep alive
	//HTTP/1.0 no connection header -> close

	//determine header type
	switch(name[0]) {
		case 'C':
		case 'c':
			//handler Connection
			if(!strcasecmp(name + 1, "onnection")) {
				if(!strcasecmp(value, "keep-alive")) {
					request->options |= KEEP_ALIVE;
				} else {
					request->options &= (~KEEP_ALIVE);
				}
			} else if(!strcasecmp(name + 10, "ontent-length")) {
			} else if(!strcasecmp(name + 10, "ontent-encoding")) {
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
		remove_trailing_whitespace(buf, r);

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

		value = name + r + 1;
		//skip leading whitespace (trailing whitespace already removed)
		while(*value == ' ' || *value == '\t') ++value;

		fprintf(stderr, "Name: \"%s\", Value: \"%s\"\n", name, value);

		//TODO: check for errors
		handle_header(request, name, value);
	}

	return 0;
}

void handle_http_connection(int fd) {
	http_connection_t con;
	http_request_t request;
	init_connection(&con, fd);
	init_request(&request);
	int r;

	do {
		//read_request_line
		r = read_request_line(&con, &request);

		//io error
		if(r == -1) break;

		//client error or unsupported method
		if(r < 0) {
			reply_with_error(&con, request.status);
			break;
		}

		//read headers
		if(read_headers(&con, &request) < 0) {
			reply_with_error(&con, request.status);
			break;
		}

		print_request(&request);

		//read body (if any)
	} while(request.options & KEEP_ALIVE);
}
