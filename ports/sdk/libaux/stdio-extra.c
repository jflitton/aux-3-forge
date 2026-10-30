/* stdio functions whose code A/UX keeps outside the shared library. */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

FILE *_findiop(void);

/* A stream on an open descriptor, from the shared library's free streams. */
FILE *
fdopen(int fd, const char *mode)
{
	FILE *f;
	int flag;

	switch (mode[0]) {
	case 'r':
		flag = _IOREAD;
		break;
	case 'w':
	case 'a':
		flag = _IOWRT;
		break;
	default:
		errno = EINVAL;
		return NULL;
	}
	if (strchr(mode, '+'))
		flag = _IORW;
	if (fd < 0 || fd >= _NFILE) {
		errno = EBADF;
		return NULL;
	}
	if (!(f = _findiop())) {
		errno = EMFILE;
		return NULL;
	}
	if (mode[0] == 'a')
		lseek(fd, 0, SEEK_END);
	f->_cnt = 0;
	f->_base = f->_ptr = NULL;
	f->_flag = flag;
	f->_file = fd;
	return f;
}

void
perror(const char *s)
{
	int err = errno;

	if (s && *s) {
		fputs(s, stderr);
		fputs(": ", stderr);
	}
	fputs(strerror(err), stderr);
	fputc('\n', stderr);
}

int
remove(const char *path)
{
	if (unlink(path) == 0)
		return 0;
	if (errno == EISDIR || errno == EPERM)
		return rmdir(path);
	return -1;
}
