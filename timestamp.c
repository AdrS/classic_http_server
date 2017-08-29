#include "timestamp.h"
#include <assert.h>
#include <string.h>

//see: https://tools.ietf.org/html/rfc2616#section-3.3.1
//example: Sun, 06 Nov 1994 08:49:37 GMT
static const char *RFC_1123_DATE = "%a, %d %b %Y %T GMT";

int time2str(const time_t *timestamp, char *buf, size_t size) {
	assert(timestamp);
	assert(buf);
	assert(size > 31);
	struct tm gmt_timestamp;
	if(!gmtime_r(timestamp, &gmt_timestamp)) {
		return -1;
	}
	if(!strftime(buf, size - 1, RFC_1123_DATE, &gmt_timestamp)) {
		return -1;
	}
	return 0;
}

//OBSOLETE formats for backward compatibility
//example: Sunday, 06-Nov-94 08:49:37 GMT
static const char *RFC_850_DATE = "%A, %d-%b-%y %T GMT";
//example: Sun Nov  6 08:49:37 1994
static const char *ANSI_C_DATE = "%a %d %e %T %Y";

int str2time(const char *buf, time_t *timestamp) {
	assert(buf);
	assert(timestamp);

	struct tm tm;
	char *r;
	const char **fmt;
	const char *DATE_FORMATS[] = {
		RFC_1123_DATE, RFC_850_DATE, ANSI_C_DATE, NULL
	};

	//try each format
	for(fmt = DATE_FORMATS; *fmt; ++fmt) {
		memset(&tm, 0, sizeof(struct tm));
		r = strptime(buf, *fmt, &tm);
		if(r && *r == '\0') {
			//timegm is nonstandard
			*timestamp = timegm(&tm);
			if(*timestamp != -1) {
				return 0;
			}
		}
	}
	return -1;
}


