#include "util.h"

#include <assert.h>
#include <ctype.h>

//returns index of first occurance of c or -1 or c is not in string
int find_first(const char *line, char c) {
	int i;
	for(i = 0; line[i]; ++i) {
		if(line[i] == c) {
			return i;
		}
	}
	return -1;
}

//replaces trailing newline (CRLF or LF) with null terminator
void remove_endline(char *line, size_t len) {
	//there should actually be a endline
	assert(line[len] == '\0' && line[len - 1] == '\n');
	if(len > 1 && line[len - 2] == '\r') {
		//CRLF
		line[len - 2] = '\0';
	} else {
		//LF
		line[len - 1] = '\0';
	}
}

int is_trailing_space(char c) {
	return c == '\n' || c == '\r' || c == ' ' || c == '\t' || c == '\0';
}

//remove trailing whitespace (ie: ' ' \t \r \n)
void remove_trailing_whitespace(char *line, size_t len) {
	int i = ((int)len) - 1;
	while(i >= 0 && is_trailing_space(line[i])) {
		line[i--] = '\0';
	}
}

char *strip(char *line) {
	assert(line);
	char *pos;

	//skip leading whitespace
	while(isspace(*line)) ++line;

	pos = line;
	//find end of string
	while(*pos) ++pos;

	//strip trailing whitespace
	while(pos > line && (isspace(*pos) || *pos == '\0')) {
		*pos-- = '\0';
	}

	return line;
}

//parses unsigned decimal number, returns -1 on invalid digit string
int parse_uint(const char *num_str) {
	int i = 0;
	//empty string is invalid
	do {
		//non-digit found
		if(!isdigit(*num_str)) {
			return -1;
		}
		i = 10*i + (*num_str - '0');
		++num_str;
	} while(*num_str);
	return i;
}

int parse_list(char *list, int (*handle_elt)(char *elt, void *args), void *args) {
	assert(list);
	assert(handle_elt);
	int num_elts = 0;
	int len;
	char *cur = list, *next = NULL;

	//for each element of list
	do {
		//find end of element
		len = find_first(cur, ',');

		//null terminate element and save location of next
		if(len > 0) {
			cur[len] = '\0';
			next = cur + len + 1;
		}

		//remove leading and trailing whitespace
		cur = strip(cur);
		
		//empty element
		if(!cur[0]) {
			//if whole list is empty, then this is ok
			if(num_elts == 0 && !next) return 0;

			//otherwise there is empty element or trailing ','
			//ex: "abc, , efg" or "a, b, "
			return -1;
		}

		if(handle_elt(cur, args) == -1) return -1;

		cur = next;
		++num_elts;
	} while(len != -1);
	return num_elts;
}
