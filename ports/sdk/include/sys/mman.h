/*
 * mmap, libaux's, as A/UX has none: private mappings only, made by reading the file into
 * memory (or zeroed memory for MAP_ANON), so changes to the file after mmap aren't seen.
 * MAP_SHARED and MAP_FIXED fail with ENODEV.
 */
#ifndef _SYS_MMAN_H
#define _SYS_MMAN_H
#include <sys/types.h>

#define PROT_NONE	0
#define PROT_READ	1
#define PROT_WRITE	2
#define PROT_EXEC	4

#define MAP_SHARED	0x0001
#define MAP_PRIVATE	0x0002
#define MAP_FIXED	0x0010
#define MAP_FILE	0
#define MAP_ANON	0x1000
#define MAP_ANONYMOUS	MAP_ANON
#define MAP_FAILED	((void *)-1)

void *mmap(void *addr, size_t len, int prot, int flags, int fd, off_t off);
int munmap(void *addr, size_t len);

#endif
