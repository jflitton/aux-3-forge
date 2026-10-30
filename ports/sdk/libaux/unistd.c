/* unistd and related functions whose code A/UX keeps outside the shared library. */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <unistd.h>

char *getwd(char *buf);
int __aux_gettimeofday(struct timeval *tv);

int
dup2(int fd, int fd2)
{
	if (fd == fd2)
		return fcntl(fd, F_GETFL) < 0 ? -1 : fd2;
	close(fd2);
	return fcntl(fd, F_DUPFD, fd2);
}

/* With the shared library's getwd, which needs PATH_MAX bytes. */
char *
getcwd(char *buf, size_t size)
{
	char path[PATH_MAX];
	size_t n;

	if (!getwd(path)) {
		errno = EACCES;
		return NULL;
	}
	n = strlen(path) + 1;
	if (!buf) {
		if (size < n)
			size = n;
		if (!(buf = malloc(size)))
			return NULL;
	} else if (size < n) {
		errno = size ? ERANGE : EINVAL;
		return NULL;
	}
	return memcpy(buf, path, n);
}

unsigned
sleep(unsigned seconds)
{
	struct timeval tv = { seconds, 0 };

	select(0, NULL, NULL, NULL, &tv);
	return 0;
}

void
rewinddir(DIR *d)
{
	seekdir(d, 0);
}

int
gettimeofday(struct timeval *tv, void *tz)
{
	if (tz)
		memset(tz, 0, sizeof(struct timezone));
	return __aux_gettimeofday(tv);
}
