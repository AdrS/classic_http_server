#include "encoding_prefs.h"
#include <assert.h>
#include <stdio.h>

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

int main() {
	test_parse_qstring();
	return 0;
}
