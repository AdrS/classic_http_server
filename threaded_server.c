#include "threaded_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define BACKLOG 10

typedef struct sockaddr sa_t;

static void fatal_error(const char *msg) {
	perror(msg);
	exit(1);
}

static void set_address(struct sockaddr_in *addr, int port) {
	bzero(addr, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons(port);
	addr->sin_addr.s_addr = INADDR_ANY; //TODO: make this configurable
}

//an abstraction to make writing connection handlers nicer
typedef struct {
	connection_handler_t *handler;
	int client_fd;
} start_handler_args;

static void *start_handler(void *arg) {
	start_handler_args *args = (start_handler_args*)arg;
	args->handler(args->client_fd);
	return NULL;
}

static int listen_on(int port) {
	struct sockaddr_in server_addr;
	int listen_fd, optval = 1;

	//create socket
	listen_fd = socket(AF_INET, SOCK_STREAM, 0);
	if(listen_fd == -1) {
		fatal_error("socket");
	}

	//bind to *:<port>
	set_address(&server_addr, port);

	//TODO: set SO_LINGER
	//TODO: REUSEPORT vs REUSEADDR
	setsockopt(listen_fd, SOL_SOCKET, SO_REUSEPORT, &optval, sizeof optval);

	if(bind(listen_fd, (sa_t*)&server_addr, sizeof server_addr)) {
		fatal_error("could not bind to address");
	}

	//listen
	if(listen(listen_fd, BACKLOG)) {
		fatal_error("listen");
	}
	return listen_fd;
}

void print_connection_info(struct sockaddr_in *client_addr) {
	//TODO: add support for IPv6
	char addr_buf[INET_ADDRSTRLEN];
	time_t t;
	struct tm local_time;
	char time_str_buf[32];

	t = time(NULL);
	localtime_r(&t, &local_time);
	strftime(time_str_buf, sizeof(time_str_buf), "%c", &local_time);

	printf("[%s] connection from %s:%d\n",
		time_str_buf,
		inet_ntop(client_addr->sin_family, &(client_addr->sin_addr),
				addr_buf, INET_ADDRSTRLEN),
		ntohs(client_addr->sin_port));
}

//listens forever on specified port, creates thread to handle
//each connection
void serve_forever(int port, connection_handler_t handler) {
	int listen_fd, client_fd;
	struct sockaddr_in client_addr;
	socklen_t addrlen;

	start_handler_args handler_args;
	pthread_t handler_thread;
	pthread_attr_t handler_attr;

	handler_args.handler = handler;

	//TODO: look at: pthread_attr_setstacksize for memory tuning

	//detach thread, so main server loop does not have to wait for it
	if(pthread_attr_init(&handler_attr)) {
		fatal_error("could not initialize thread attributes");
		//no need to call pthread_attr_destroy because this function
		//only exit when program exits
	}
	pthread_attr_setdetachstate(&handler_attr, PTHREAD_CREATE_DETACHED);

	listen_fd = listen_on(port);
	printf("listening on port %d\n", port);

	for(;;) {
		addrlen = sizeof client_addr;
		client_fd = accept(listen_fd, (sa_t*)&client_addr, &addrlen);
		if(client_fd == -1) {
			fatal_error("accept");
		}

		print_connection_info(&client_addr);
		
		//create thread to handle each connection
		handler_args.client_fd = client_fd;

		if(pthread_create(&handler_thread, &handler_attr,
				start_handler, &handler_args)) {
			fprintf(stderr, "could not create thread for connection");
			close(client_fd);
		}
	}
}
