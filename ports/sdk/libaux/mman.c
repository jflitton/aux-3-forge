/* mmap and munmap for private mappings (sys/mman.h): a copy of the file in memory. */
#include <errno.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

void *
mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off)
{
	char *p;
	off_t was;
	ssize_t n = 0;
	size_t got = 0;

	if (!len) {
		errno = EINVAL;
		return MAP_FAILED;
	}
	if (!(flags & MAP_PRIVATE) || (flags & (MAP_SHARED | MAP_FIXED))) {
		errno = ENODEV;
		return MAP_FAILED;
	}
	if (!(p = calloc(1, len))) {
		errno = ENOMEM;
		return MAP_FAILED;
	}
	if (flags & MAP_ANON)
		return p;
	if ((was = lseek(fd, 0L, SEEK_CUR)) < 0 || lseek(fd, off, SEEK_SET) < 0) {
		free(p);
		return MAP_FAILED;
	}
	while (got < len && (n = read(fd, p + got, len - got)) > 0)
		got += n;
	lseek(fd, was, SEEK_SET);
	if (n < 0) {
		free(p);
		return MAP_FAILED;
	}
	return p;	/* past the end of the file: zeros */
}

int
munmap(void *addr, size_t len)
{
	free(addr);
	return 0;
}
