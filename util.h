#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

//returns index of first occurance of c or -1 or c is not in string
int find_first(const char *line, char c);

//replaces trailing newline (CRLF or LF) with null terminator
//assumes that line[len] is the null terminator and that string
//actually ends with newline (ie: line[len - 1] == '\n')
void remove_endline(char *line, size_t len);

//checks if c is one of ' ' \r \t \t \0
int is_trailing_space(char c);

//remove trailing whitespace (ie: ' ' \t \r \n)
void remove_trailing_whitespace(char *line, size_t len);

//removes leading and trailing whitespace and returns pointer to
//beginning of trimmed line
char *strip(char *line);

//parses unsigned decimal number, returns -1 on invalid digit string
int parse_uint(const char *num_str);

//takes comma seperated list of elements calls handle_elt for each element
//-leading and trailing whitespace is removed from each element
//-the args parameter for handler allows handler to use extra state
//handle_elt should return -1 on error
//returns -1 if handle_elt returns an error for any list element
//returns number of elements otherwise
//TODO: make element delimeter a parameter
int parse_list(char *list, int (*handle_elt)(char *elt, void *args), void *args);

#endif
