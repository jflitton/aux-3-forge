/* A/UX 3's open flags and fcntl. */
#ifndef _FCNTL_H
#define _FCNTL_H

#include <sys/types.h>

#define O_RDONLY	0
#define O_WRONLY	1
#define O_RDWR		2
#define O_ACCMODE	3
#define O_NDELAY	0x00000004
#define O_APPEND	0x00000008
#define O_SYNC		0x00000080
#define O_CREAT		0x00000100
#define O_TRUNC		0x00000200
#define O_EXCL		0x00000400
#define O_NONBLOCK	0x00004000
#define O_NOCTTY	0x00008000

#define F_DUPFD		0
#define F_GETFD		1
#define F_SETFD		2
#define F_GETFL		3
#define F_SETFL		4
#define F_GETLK		5
#define F_SETLK		6
#define F_SETLKW	7
#define F_GETOWN	8
#define F_SETOWN	9
#define FD_CLOEXEC	1

#define F_RDLCK		1
#define F_WRLCK		2
#define F_UNLCK		3

struct flock {
	short l_type;
	short l_whence;
	off_t l_start;
	off_t l_len;
	pid_t l_pid;
};

int creat(const char *path, mode_t mode);
int fcntl(int fd, int cmd, ...);
int open(const char *path, int flags, ...);

#endif
