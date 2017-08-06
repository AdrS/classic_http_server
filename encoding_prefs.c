#include "encoding_prefs.h"
#include "util.h"
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

//takes pointer to the ";" part of a weight string and returns
//1000*qvalue or -1 if string is invalid
int parse_weight(const char *wstr) {
	assert(wstr);
	assert(wstr[0] == ';');
	++wstr;
	//skip leading whitespace
	while(*wstr == ' ' || *wstr == '\t') ++wstr;

	//check for "q="
	if(wstr[0] != 'q' || wstr[1] != '=') return -1;

	//get qvalue
	return parse_qvalue(wstr + 2);
}

static int handle_coding_entry(char *entry, void *arg) {
	assert(entry);
	assert(arg);
	int i, weight;
	encoding_prefs_t *prefs = (encoding_prefs_t*)arg;

	//check for weight
	i = find_first(entry, ';');

	//determine encoding weigth
	if(i != -1) {
		weight = parse_weight(entry + i);

		//invalid weigth
		if(weight < 0) return -1;

		//remove whitespace between coding and weight
		entry[i] = '\0';
		remove_trailing_whitespace(entry, (size_t)i);
	} else {
		//default is 1
		weight = 1000;
	}

	//check for known codings
	if(!strcmp(entry, "gzip")) {
		prefs->gzip = weight;
	} else if(!strcmp(entry, "identity")) {
		prefs->identity = weight;
	} else if(!strcmp(entry, "*")) {
		prefs->catch_all = weight;
	}
	return 0;
}

//weight = OWS ";" OWS "q=" qvalue (default weigth is 1)
//Accept-Encoding  = #( codings [ weight ] )
//Accept-Encoding = [ ( "," / ( codings [ weight ] ) ) *( OWS "," [ OWS
//    ( codings [ weight ] ) ] ) ]
//codings = content-coding / "identity" / "*"
//content-coding = token
int parse_accepted_encodings(encoding_prefs_t *result, char *header_str) {
	assert(result);
	assert(header_str);

	//initialize result
	init_encoding_prefs(result);

	//parse each entry in comman seperated list
	return parse_list(header_str, handle_coding_entry, (void*)result);
}
