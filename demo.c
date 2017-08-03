#include "threaded_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

//start: 1:50 pm 2 August 2017
//
//TODO: write read line (with buffering???)
//	-write parse request line
//	-write unquote (percent encoding)
//	-write parse header line
//	-write find file length

#define BUF_SIZE 1024

//handler echos back whatever the client sends
void handle_connection(int con) {
	char buf[BUF_SIZE];
	ssize_t r;
	ssize_t left;
	do {
		r = recv(con, buf, BUF_SIZE, 0);
		if(r <= 0) break;

		left = r;
		while(left > 0) {
			r = send(con, buf, left, 0);
			if(r < 0) break;
			left -= r;
		}
	} while(r > 0);
	close(con);
}

//checks that path does not contain ".." and does not begin with "/"
int is_safe_path(const char *path) {
	if(*path == '/') {
		return 0;
	}
	while(*path) {
		if(*path == '.' && path[1] == '.') {
			return 0;
		}
		++path;
	}
	return 1;
}

int recv_all(int con, char *buf, size_t len) {
	size_t left = len;
	ssize_t r;
	while(left > 0) {
		r = recv(con, buf, left, 0);
		if(r < 0) {
			return r;
		}
		if(r == 0) break;
		buf += r;
		left -= r;
	}
	return len - left;
}

int send_all(int con, const char *buf, size_t len) {
	ssize_t r;
	while(len > 0) {
		r = send(con, buf, len, 0);
		if(r < 0) {
			return r;
		}
		buf += r;
		len -= r;
	}
	return len;
}

//send error message and close connection
void send_error(int con, const char *msg) {
	send_all(con, msg, strlen(msg));
	close(con);
}

void file_server_handle_connection(int con) {
	char buf[BUF_SIZE];
	ssize_t len;
	int fd;

	//read file name (eof terminated)
	len = recv_all(con, buf, BUF_SIZE - 1);
	if(len < 0) {
		close(con);
		return;
	}

	//ensure null termination
	buf[len] = '\0';

	//remove trailing newline
	if(len > 0 && buf[len - 1] == '\n') {
		buf[len - 1] = '\0';
	}

	printf("request for \"%s\"\n", buf);

	//block directory traversal
	if(!is_safe_path(buf)) {
		send_error(con, "error: illegal filepath");
		return;
	}

	//send requested file
	//	should I block following of symbolic links O_NOFOLLOW?
	fd = open(buf, O_RDONLY);
	if(fd < 0) {
		send_error(con, "error: could not open requested file");
		return;
	}

	//send file
	do {
		len = read(fd, buf, BUF_SIZE);
		if(len < 0) {
			send_error(con, "error: could not read whole file");
			return;
		} else if(len > 0 && send_all(con, buf, len) < 0) {
			break;
		}
	} while(len > 0);
	close(con);
}

int main(int argc, const char **argv) {
	if(argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	//serve_forever(atoi(argv[1]), handle_connection);
	serve_forever(atoi(argv[1]), file_server_handle_connection);
	return 0;
}
