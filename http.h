#ifndef HTTP_H
#define HTTP_H

//parse request line
//parse header
//percent decode
//read chunked encoding
//see zlib.h for adding gzip + deflate support
//Accept-Encoding: ...
//Content-Encoding: ...

//support for GET and HEAD is required by standard
typedef enum { GET, POST, HEAD, UNKNOWN } http_method_t;

//typedef struct {
//	const char *key;
//	const char *value;
//} http_header_t;

void handle_http_connection(int con);

#endif
