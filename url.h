#ifndef URL_H
#define URL_H

//see rfc 3986

//percent decode all characters and return new size
//Note: because %00 could be in the string, null termination cannot be used
//length of decoded string is saved in len
//returns -1 on error, 0 if %00 does not occur and str can be treated as
//null terminated, 1 if %00 occurs, and string cannot be treated as zstr
int percent_decode(char *str, int *len);

#endif
