#ifndef ENCODING_PREFS_H
#define ENCODING_PREFS_H

//stores client encoding preferences
//default weight is 1 (RFC 7231 5.3.1)
//-1 if unspecified
typedef struct {
	int gzip;
	int identity;
	int catch_all; //weight of all other encodings not on list
} encoding_prefs_t;

//initialize
void init_encoding_prefs(encoding_prefs_t *ep);

//parses the Accept-Encoding header string
//returns -1 if malformed
int parse_accepted_encodings(encoding_prefs_t *result, char *header_str);

int parse_qvalue(const char *qstr);

int parse_weight(const char *wstr);

#endif
