#ifndef THREADED_SERVER_H
#define THREADED_SERVER_H

#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>

//connection handlers are passed the file descriptor of the client connection
//WARNING: handler should not close connection itself
typedef void (connection_handler_t)(int client_fd);

//TODO: add more handlers for server
////called when the server starts listening
//typedef void (on_listen_t)(int port);
//
////called whenever the server accepts a new connection
//typedef void (on_accept_t)(struct sockaddr_in *client_addr, socklen_t addrlen);
//
//
//typedef struct {
//	connection_handler_t *connection_handler;
//	on_listen_t *on_listen;
//	on_accept_t *on_accept;
//	//on_error
//} server_handlers_t;

void serve_forever(int port, connection_handler_t handler);

#endif
