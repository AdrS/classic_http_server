#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

//returns index of first occurance of c or -1 or c is not in string
int find_first(const char *line, char c);

//replaces trailing newline (CRLF or LF) with null terminator
void remove_endline(char *line, size_t len);

int is_trailing_space(char c);

//remove trailing whitespace (ie: ' ' \t \r \n)
void remove_trailing_whitespace(char *line, size_t len);

//parses unsigned decimal number, returns -1 on invalid digit string
int parse_uint(const char *num_str);

#endif
