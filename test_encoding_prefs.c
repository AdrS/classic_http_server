#include "encoding_prefs.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>

void test_parse_qstring() {
	//qvalue = ( "0" [ "." *3DIGIT ] ) / ( "1" [ "." *3"0" ] )

	//test valid
	assert(parse_qvalue("0") == 0);
	assert(parse_qvalue("0.000") == 0);

	assert(parse_qvalue("1") == 1000);
	assert(parse_qvalue("1.000") == 1000);

	assert(parse_qvalue("0.8") == 800);
	assert(parse_qvalue("0.888") == 888);

	//decimal without following digits
	assert(parse_qvalue("1.") == -1);
	assert(parse_qvalue("0.") == -1);

	//too many digits
	assert(parse_qvalue("0.0000") == -1);
	assert(parse_qvalue("1.0000") == -1);

	//not in range 0-1
	assert(parse_qvalue("1.05") == -1);
	assert(parse_qvalue("10") == -1);

	//not digits
	assert(parse_qvalue(" 0.8") == -1);
	assert(parse_qvalue("=0.8") == -1);
	assert(parse_qvalue("q0.8") == -1);
	assert(parse_qvalue("0.8 ") == -1);
}

void test_parse_weight() {
	assert(parse_weight(";q=0") == 0);
	assert(parse_weight(";  q=0") == 0);
	assert(parse_weight(";  q=1.0") == 1000);
	assert(parse_weight(";  q=1.000") == 1000);
	assert(parse_weight("; \t q=0.8") == 800);
	assert(parse_weight(";q=0.001") == 1);

	//invalid
	assert(parse_weight(";  Q=0") == -1);
	assert(parse_weight(";  q =0") == -1);
	assert(parse_weight(";q=0.001f") == -1);
}

void test_parse_accepted_encodings() {
	//Accept-Encoding: compress;q=0.5, gzip;q=1.0
	//Accept-Encoding: gzip;q=1.0, identity; q=0.5, *;q=0
	//
	//test default weigth is 1 rfc7231#section-5.3.1
	encoding_prefs_t prefs;
	char buf[128];

	//test empty list Accept-Encoding:
	init_encoding_prefs(&prefs);
	strncpy(buf, "", 128);
	assert(parse_accepted_encodings(&prefs, buf) != -1);
	assert(prefs.gzip == -1);
	assert(prefs.identity == -1);
	assert(prefs.catch_all == -1);


	//Accept-Encoding: *
	init_encoding_prefs(&prefs);
	strncpy(buf, "*", 128);
	assert(parse_accepted_encodings(&prefs, buf) != -1);
	assert(prefs.gzip == -1);
	assert(prefs.identity == -1);
	assert(prefs.catch_all == 1000);

	//Accept-Encoding: compress;q=0.5, gzip;q=1.0
	init_encoding_prefs(&prefs);
	strncpy(buf, "compress;q=0.5, gzip;q=1.0", 128);
	assert(parse_accepted_encodings(&prefs, buf) != -1);
	assert(prefs.gzip == 1000);
	assert(prefs.identity == -1);
	assert(prefs.catch_all == -1);

	init_encoding_prefs(&prefs);
	strncpy(buf, "compress\t;\tq=0.5 , gzip ;q=0.8, *", 128);
	assert(parse_accepted_encodings(&prefs, buf) != -1);
	assert(prefs.gzip == 800);
	assert(prefs.identity == -1);
	assert(prefs.catch_all == 1000);

	init_encoding_prefs(&prefs);
	strncpy(buf, "identity, *;q=0", 128);
	assert(parse_accepted_encodings(&prefs, buf) != -1);
	assert(prefs.gzip == -1);
	assert(prefs.identity == 1000);
	assert(prefs.catch_all == 0);

	//test empty element
	init_encoding_prefs(&prefs);
	strncpy(buf, "identity,,  *;q=0", 128);
	assert(parse_accepted_encodings(&prefs, buf) == -1);

	//test bad weight
	init_encoding_prefs(&prefs);
	strncpy(buf, "identity, *;q=0.8a", 128);
	assert(parse_accepted_encodings(&prefs, buf) == -1);

	//test missing weigth
	init_encoding_prefs(&prefs);
	strncpy(buf, "identity;, gzip;q=0.8", 128);
	assert(parse_accepted_encodings(&prefs, buf) == -1);
}

int main() {
	test_parse_qstring();
	test_parse_weight();
	test_parse_accepted_encodings();
	return 0;
}
