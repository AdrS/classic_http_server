#include "encoding_prefs.h"
#include <assert.h>
#include <string.h>

void init_encoding_prefs(encoding_prefs_t *ep) {
	assert(ep);
	ep->gzip = -1;
	ep->identity = -1;
	ep->catch_all = -1;
}

//qvalue = ( "0" [ "." *3DIGIT ] ) / ( "1" [ "." *3"0" ] )
//parses qvalue and returns 1000*qvalue (so value will be integer)
//returns -1 on invalid value
int parse_qvalue(const char *qstr) {
	assert(qstr);
	if(qstr[0] == '1') {
		if(qstr[1] == '.') {
		}
		return 1000;
	}
	if(qstr[0] == '0') {
		if(qstr[1] == '\0') {
			return 0;
		}
	}
	return -1;
}

//weight = OWS ";" OWS "q=" qvalue
//Accept-Encoding = [ ( "," / ( codings [ weight ] ) ) *( OWS "," [ OWS
//    ( codings [ weight ] ) ] ) ]
int parse_accepted_encodings(encoding_prefs_t *result, char *header_str) {
	assert(result);
	assert(header_str);
	if(!strcmp(header_str, "gzip")) {
		result->gzip = 1;
	}
	return -1;
	//TODO: write makefile
	//write parse qvalue tests, write parse qvalue, write parse_accepted_encodings tests
	//write parse_accepted_encodings
	//
	//write parse percent encoding + tests

	//write transfer encoding header parsing + tests
	//write chunked encoding parsing
	//write read body
	//int read_body(http_connection_t *con, http_request_t *request,
	//			int (*data_handler)(char *data, size_t len))
	
	//write is path safe + tests
	//write file reading
	//write send headers
	//write send body
	//write 
	//int send_chunked(http_connection_t *con,
	//			int (*next_chunk)(http_request_t *request))
	//write handle get
	//write handle post
	//write gzip
	//write range requests
	//caching
}
