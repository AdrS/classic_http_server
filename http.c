#include "http.h"
#include <assert.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

#include <stdio.h>

#define BUF_SIZE 1024
#define MAX_LINE 1024

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

static void init_connection(http_connection_t *con, int fd) {
	con->rsize = 0;
	con->rpos = con->recv_buf;
	con->fd = fd;
}

//check that http_connection_t is not null and properly formed
static void assert_connection_invariants_hold(http_connection_t *con) {
	assert(con);
	assert(con->rpos);
	assert(con->rpos >= con->recv_buf);
	assert(con->rpos < con->recv_buf + BUF_SIZE);
	assert(con->rsize <= con->recv_buf + BUF_SIZE - con->rpos);
	assert(con->fd >= 0);
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

//read line terminated with CRLF or LF
//void read_line(int con);

//void parse_request_line(char *buf, size_t len);

//if reason phrase is null, a generic one is used instead
/*
static void send_reply_line(http_connection_t *con, int status_code,
				const char *reason_phrase) {
}

static void send_headers(http_connection_t *con) {
}

static void send_body(http_connection_t *con) {
}
static int recv_request_line(http_connection_t *con) {
	return -1;
}
*/

void handle_http_connection(int fd) {
	http_connection_t con;
	char line[MAX_LINE];
	ssize_t r;
	init_connection(&con, fd);

	//TODO: put into "read request line" function
	r = recv_line(&con, line, MAX_LINE);
	if(r == -1) return;

	//line too long
	if(r == -2) {
		send_all(fd, "HTTP/1.1 400 Request Line Too Long\r\nConnection: close\r\n\r\n", 57);
		return;
	}
	printf("Request line: %s", line);

	send_all(fd, "HTTP/1.1 501 Not Implemented\r\nConnection: close\r\n\r\n", 51);

	//read request line
	//read headers
}
