#ifndef TIMESTAMP_H
#define TIMESTAMP_H

#define _XOPEN_SOURCE

#ifndef _BSD_SOURCE
#define _BSD_SOURCE
#endif

#include <time.h>

//converts timestamp to string of form: Wed, 21 Oct 2015 07:28:00 GMT
//returns -1 or error
int time2str(const time_t *timestamp, char *buf, size_t size);

//inverse of time2str, tries multiple timestatmp formats
int str2time(const char *buf, time_t *timestamp);

#endif
