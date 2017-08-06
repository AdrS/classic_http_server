#ifndef ENCODING_PREFS_H
#define ENCODING_PREFS_H

//stores client encoding preferences
//-1 if preference for encoding has not been specified
typedef struct {
	int gzip;
	int identity;
	int catch_all; //means all other encodings not on list
} encoding_prefs_t;

//inittialize
void init_encoding_prefs(encoding_prefs_t *ep);

//parses the Accepted-Encodings header string
//returns -1 if malformed
int parse_accepted_encodings(encoding_prefs_t *result, char *header_str);

int parse_qvalue(const char *qstr);

#endif
