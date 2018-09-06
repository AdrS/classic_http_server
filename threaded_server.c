#include "threaded_server.h"
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <pthread.h>

#define BACKLOG 10
#define NUM_WORKERS 100
#define QUEUE_SIZE 500

typedef struct sockaddr sa_t;

static void fatal_error(const char *msg) {
	perror(msg);
	exit(1);
}

static void set_address(struct sockaddr_in *addr, int port) {
	bzero(addr, sizeof(*addr));
	addr->sin_family = AF_INET;
	addr->sin_port = htons((unsigned short)port);
	addr->sin_addr.s_addr = INADDR_ANY; //TODO: make this configurable
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

// Queue for newly connected clients
typedef struct {
	pthread_mutex_t lock;
	pthread_cond_t not_empty;
	pthread_cond_t not_full;
	int *queue; // queue of client connection file descriptors
	int size;
	int start;
	int used;
} request_queue_t;

static request_queue_t *make_queue(int size) {
	request_queue_t *queue = malloc(sizeof(request_queue_t));
	if(pthread_mutex_init(&queue->lock, NULL)) {
		fatal_error("could not create mutex for request queue");
	}
	if(pthread_cond_init(&queue->not_empty, NULL) ||
			pthread_cond_init(&queue->not_full, NULL)) {
		fatal_error("could not create condition variable for request queue");
	}
	queue->queue = malloc(sizeof(int)*(unsigned)size);
	queue->size = size;
	queue->start = queue->used = 0;
	return queue;
}

static void enqueue(request_queue_t *queue, int client_fd) {
	pthread_mutex_lock(&queue->lock);
	// Wait for queue to have space
	while(queue->used == queue->size) {
		pthread_cond_wait(&queue->not_full, &queue->lock);
	}
	queue->queue[(queue->start + queue->used) % queue->size] = client_fd;
	queue->used++;
	if(queue->used == 1) {
		// signal to workers that queue is not longer empty
		pthread_cond_signal(&queue->not_empty);
	}
	pthread_mutex_unlock(&queue->lock);
}

static int dequeue(request_queue_t *queue) {
	pthread_mutex_lock(&queue->lock);
	// Wait for queue to have elements
	while(!queue->used) {
		pthread_cond_wait(&queue->not_empty, &queue->lock);
	}
	int client_fd = queue->queue[queue->start];
	queue->start = (queue->start + 1) % queue->size;
	queue->used--;
	// If queue becomes non full, tell main thread
	if(queue->used == queue->size - 1) {
		pthread_cond_signal(&queue->not_full);
	}
	pthread_mutex_unlock(&queue->lock);
	return client_fd;
}

typedef struct {
	connection_handler_t *handler;
	request_queue_t *queue;
} worker_args_t;

static void *worker(void *arg) {
	worker_args_t *args = (worker_args_t*)arg;
	for(;;) {
		int client_fd = dequeue(args->queue);

		//print_connection_info(&client_addr);
		args->handler(client_fd);
		// don't make main server loop close connections
		close(client_fd);
	}
	return NULL;
}

//listens forever on specified port, creates thread to handle
//each connection
void serve_forever(int port, connection_handler_t handler) {
	int listen_fd, client_fd;
	struct sockaddr_in client_addr;
	socklen_t addrlen;

	// pass connection handler and request queue to worker thread
	worker_args_t worker_args;
	worker_args.handler = handler;
	worker_args.queue = make_queue(QUEUE_SIZE);

	// create worker threads
	pthread_t *workers = malloc(sizeof(pthread_t)*NUM_WORKERS);
	for(int i = 0; i < NUM_WORKERS; ++i) {
		if(pthread_create(workers + i, NULL, worker, &worker_args)) {
			fatal_error("could not create worker threads");
		}
	}

	listen_fd = listen_on(port);
	printf("listening on port %d\n", port);

	for(;;) {
		addrlen = sizeof client_addr;
		client_fd = accept(listen_fd, (sa_t*)&client_addr, &addrlen);
		if(client_fd == -1) {
			fatal_error("accept");
		}
		enqueue(worker_args.queue, client_fd); 
	}
}
