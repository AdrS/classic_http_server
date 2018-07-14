#include "files.h"
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <fcntl.h>

int web_open(const char *path, int *length, time_t *mtime) {
	assert(path);
	assert(length);
	assert(mtime);

	const char *default_file = "index.html";
	if(!path[0]) {
		path = default_file;
	}

	struct stat sb;
	int fd = open(path, O_RDONLY);
	int dirfd;
	if(fd == -1) {
		//TODO: differentiate between errors
		//ex: EACCES vs ENOMEM
		return -1;
	}

	//check if path is directory or file
	if(fstat(fd, &sb)) {
		close(fd);
		return -1;
	}

	//if path is directory, see if it contains an index.html file
	if(S_ISDIR(sb.st_mode)) {
		dirfd = fd;
		fd = openat(dirfd, default_file, O_RDONLY);
		close(dirfd);

		if(fd == -1) return -1;
		if(fstat(fd, &sb)) {
			close(fd);
			return -1;
		}
	}

	//must get a regular file (otherwise sendfile does not work)
	if(!S_ISREG(sb.st_mode)) {
		close(fd);
		return -1;
	}

	*length = (int)sb.st_size;
	*mtime = sb.st_mtime;
	return fd;
}
