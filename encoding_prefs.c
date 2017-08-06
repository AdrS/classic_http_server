#include "encoding_prefs.h"
#include <assert.h>
#include <ctype.h>
#include <string.h>

void init_encoding_prefs(encoding_prefs_t *ep) {
	assert(ep);
	ep->gzip = -1;
	ep->identity = -1;
	ep->catch_all = -1;
}

//reads the decimal portion of a qvalue
//Decimal portion is optional.
//If present it has form '.' followed by 1-3 digits
//returns 1000*decimal value
static int read_decimal_part(const char *str) {
	int i, decimal, digit_power;
	//no decimal part => 0  for decimal value
	if(str[0] == '\0') {
		return 0;
	}
	//no digit for decimal value => invalid
	if(str[0] != '.') {
		return -1;
	}

	decimal = 0;
	digit_power = 1000;
	str += 1;

	//read up to 3 decimal digits
	for(i = 0; i < 3; ++i) {
		//end of digits
		if(!str[i]) {
			//check that there actually were digits after decimal
			if(i == 0) {
				return -1;
			}
			return decimal * digit_power;
		}
		//not actually a digit string
		if(!isdigit(str[i])) {
			return -1;
		}
		decimal = 10*decimal + (int)(str[i] - '0');
		digit_power /= 10;
	}


	//no characters allowed after first 3 digits
	if(str[i] != '\0') {
		return -1;
	}
	return decimal;
}

//qvalue = ( "0" [ "." *3DIGIT ] ) / ( "1" [ "." *3"0" ] )
//parses qvalue and returns 1000*qvalue (so value will be integer)
//returns -1 on invalid value
int parse_qvalue(const char *qstr) {
	assert(qstr);
	switch(qstr[0]) {
		case '1':
			if(read_decimal_part(qstr + 1) != 0) {
				return -1;
			} else {
				return 1000;
			}
		break;
		case '0':
			return read_decimal_part(qstr + 1);
		break;
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
