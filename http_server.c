#include "threaded_server.h"
#include "http.h"
#include <stdio.h>
#include <stdlib.h>

int main(int argc, const char **argv) {
	if(argc != 2) {
		fprintf(stderr, "usage: %s <port>\n", argv[0]);
		exit(1);
	}

	serve_forever(atoi(argv[1]), handle_http_connection);
	return 0;
}
