#include "util.h"
#include <assert.h>
#include <string.h>

void test_find_first() {
	assert(find_first("", 'a') == -1);
	assert(find_first(" mdfj sklfj", 'a') == -1);
	assert(find_first("abc", 'a') == 0);

	//if multiple occurances, returns first
	assert(find_first("bcaa", 'a') == 2);
}

void test_remove_endline() {
	char buf[10];
	//test on LF
	strncpy(buf, "hello\n", 10);
	remove_endline(buf, 6);
	assert(!strcmp(buf, "hello"));

	//test on CRLF
	strncpy(buf, "hello\r\n", 10);
	remove_endline(buf, 7);
	assert(!strcmp(buf, "hello"));
}

void test_strip() {
	char buf[20];
	char *r;

	//test empty string
	strncpy(buf, "", 20);
	r = strip(buf);
	assert(r);
	assert(r >= buf && r < buf + 20);
	assert(!strcmp(r, ""));

	//test all whitespace
	strncpy(buf, " \t ", 20);
	r = strip(buf);
	assert(r);
	assert(r >= buf && r < buf + 20);
	assert(!strcmp(r, ""));

	//test no whitespace
	strncpy(buf, " \thello\n", 20);
	r = strip(buf);
	assert(r);
	assert(r >= buf && r < buf + 20);
	assert(!strcmp(r, "hello"));

	//test no leading or lagging whitespace
	strncpy(buf, "hel lo", 20);
	r = strip(buf);
	assert(r);
	assert(r >= buf && r < buf + 20);
	assert(!strcmp(r, "hel lo"));

	strncpy(buf, "\t hel lo\t\t   ", 20);
	r = strip(buf);
	assert(r);
	assert(r >= buf && r < buf + 20);
	assert(!strcmp(r, "hel lo"));
}

typedef struct {
	const char *elts[10];
	int num_elts;
} record_elts_t;

void init_record_elts(record_elts_t *re) {
	re->num_elts = 0;
}

int empty_handler(char *elt, void *arg) {
	assert(elt);
	record_elts_t *memo = (record_elts_t*)arg;
	if(memo->num_elts > 10) {
		return -1;
	}
	memo->elts[memo->num_elts++] = elt;
	return 0;
}

void test_parse_list() {
	//"compress, gzip"
	//""
	//"*"
	//"compress;q=0.5, gzip;q=1.0"
	//"gzip;q=1.0, identity; q=0.5, *;q=0"
	//
	record_elts_t rec;
	char buf[20];

	//test empty list
	init_record_elts(&rec);
	strncpy(buf, "", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == 0);
	assert(rec.num_elts == 0);

	//test one element
	init_record_elts(&rec);
	strncpy(buf, " gzip ", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == 1);
	assert(rec.num_elts == 1);
	assert(!strcmp(rec.elts[0], "gzip"));

	//test multiple elemnts "compress, gzip"
	init_record_elts(&rec);
	strncpy(buf, "compress, gzip\t", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == 2);
	assert(rec.num_elts == 2);
	assert(!strcmp(rec.elts[0], "compress"));
	assert(!strcmp(rec.elts[1], "gzip"));

	//test empty element (just whitespace) "a, , c"
	init_record_elts(&rec);
	strncpy(buf, "a, , c", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == -1);

	//test empty element "a,, c"
	init_record_elts(&rec);
	strncpy(buf, "a,, c", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == -1);

	init_record_elts(&rec);
	strncpy(buf, ",", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == -1);

	init_record_elts(&rec);
	strncpy(buf, " , ", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == -1);

	//test trailing "a, b, "
	init_record_elts(&rec);
	strncpy(buf, "a, b ,", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == -1);

	//test handler returns error
	init_record_elts(&rec);
	strncpy(buf, "0,1,2,3,4,5,6,7,8,9,f", 20);
	assert(parse_list(buf, empty_handler, (void*)&rec) == -1);

	//TODO: repeated elements???
}

int main() {
	test_find_first();
	test_remove_endline();
	test_strip();
	test_parse_list();
	return 0;
}
