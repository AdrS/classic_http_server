#ifndef FILES_H
#define FILES_H

//open path return file descriptor and file length
//if path is directory, see if it contains "index.html" try opening that instead
int web_open(const char *path, int *length);

#endif
