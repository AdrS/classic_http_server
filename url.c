#include "url.h"
#include <assert.h>
#include <ctype.h>

//takes character 0-9 | a-f | A-F and returns corresponding hex value
static int decode_hex_char(char c) {
	if('0' <= c && c <= '9') return c - '0';
	if('A' <= c && c <= 'F') return c - 'A' + 10;
	if('A' <= c && c <= 'f') return c - 'a' + 10;
	return -1;
}

//percent decodes string in place, returns -1 on invalid encoding
int percent_decode(char *str, int *len) {
	assert(str);
	assert(len);
	char *spos = str, *dpos = str;
	int has_null = 0;
	while(*spos) {
		//found beginning of percent encoded character
		if(*spos == '%') {
			//check that encoding is valid
			if(!(isxdigit(spos[1]) && isxdigit(spos[2]))) {
				return -1;
			}

			*dpos = (char)((decode_hex_char(spos[1]) << 4) + decode_hex_char(spos[2]));

			//check for percent encoded null bytes
			if(!*dpos) {
				has_null = 1;
			}
			++dpos;
			spos += 3;
		} else {
			*dpos++ = *spos++;
		}
	}
	*dpos = '\0';
	*len = dpos - str;
	return has_null;
}
