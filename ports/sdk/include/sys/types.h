/*
 * A/UX 3's types, as its headers give them with _POSIX_SOURCE: 32-bit inode numbers,
 * user and group IDs. size_t and ptrdiff_t are gcc's, which are the same size as A/UX's.
 */
#ifndef _SYS_TYPES_H
#define _SYS_TYPES_H

#include <stddef.h>

typedef char *caddr_t;
typedef long daddr_t;
typedef unsigned short dev_t;
typedef int gid_t;
typedef unsigned long ino_t;
typedef long key_t;
typedef unsigned short mode_t;
typedef short nlink_t;
typedef long off_t;
typedef int pid_t;
typedef long ssize_t;
typedef long time_t;
typedef long clock_t;
typedef int uid_t;

typedef unsigned char u_char;
typedef unsigned short u_short;
typedef unsigned int u_int;
typedef unsigned long u_long;

#define major(dev)	(((dev) >> 8) & 0xff)
#define minor(dev)	((dev) & 0xff)
#define makedev(ma, mi)	((dev_t)(((ma) << 8) | (mi)))

/* select's descriptor sets, which A/UX keeps here */
#define FD_SETSIZE	128
#define NBBY		8
typedef long fd_mask;
#define NFDBITS		(sizeof(fd_mask) * NBBY)
typedef struct fd_set {
	fd_mask fds_bits[(FD_SETSIZE + NFDBITS - 1) / NFDBITS];
} fd_set;
#define FD_SET(n, p)	((p)->fds_bits[(n) / NFDBITS] |= (1L << ((n) % NFDBITS)))
#define FD_CLR(n, p)	((p)->fds_bits[(n) / NFDBITS] &= ~(1L << ((n) % NFDBITS)))
#define FD_ISSET(n, p)	(((p)->fds_bits[(n) / NFDBITS] & (1L << ((n) % NFDBITS))) != 0)
#define FD_ZERO(p)	__builtin_memset((p), 0, sizeof(*(p)))

#endif
