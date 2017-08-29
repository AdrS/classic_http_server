#ifndef FILES_H
#define FILES_H

#include <time.h>

//open path return file descriptor and file length
//if path is directory, see if it contains "index.html" try opening that instead
int web_open(const char *path, int *length, time_t *mtime);

#endif
