/*
 * A/UX 3's directories. opendir, readdir and the rest are the shared library's, which
 * returns entries laid out as here.
 */
#ifndef _DIRENT_H
#define _DIRENT_H

#include <sys/types.h>

#define MAXNAMLEN	255

struct dirent {
	unsigned long d_ino;
	unsigned short d_reclen;
	unsigned short d_namlen;
	char d_name[MAXNAMLEN + 1];
};

typedef struct __dirdesc {
	int dd_fd;
	long dd_loc;
	long dd_size;
	long dd_bbase;
	long dd_entno;
	long dd_bsize;
	char *dd_buf;
} DIR;

#define dirfd(d)	((d)->dd_fd)

DIR *opendir(const char *path);
struct dirent *readdir(DIR *d);
int closedir(DIR *d);
long telldir(DIR *d);
void seekdir(DIR *d, long loc);
void rewinddir(DIR *d);

#endif
