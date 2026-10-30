/*
 * A/UX 3's struct stat with _POSIX_SOURCE. The kernel fills the System V fields (16-bit
 * inode, user and group) and the POSIX ones side by side; the spares hold the former.
 */
#ifndef _SYS_STAT_H
#define _SYS_STAT_H

#include <sys/types.h>

struct stat {
	dev_t st_dev;
	short st_spare0;
	mode_t st_mode;
	nlink_t st_nlink;
	int st_spare1;
	dev_t st_rdev;
	off_t st_size;
	time_t st_atime;
	ino_t st_ino;
	time_t st_mtime;
	int st_spare2;
	time_t st_ctime;
	int st_spare3;
	long st_blksize;
	long st_blocks;
	uid_t st_uid;
	gid_t st_gid;
};

#define S_ISUID		04000
#define S_ISGID		02000
#define S_ISVTX		01000
#define S_IREAD		00400
#define S_IWRITE	00200
#define S_IEXEC		00100

#define S_IFMT		0170000
#define S_IFIFO		0010000
#define S_IFCHR		0020000
#define S_IFDIR		0040000
#define S_IFBLK		0060000
#define S_IFREG		0100000
#define S_IFLNK		0120000
#define S_IFSOCK	0140000
#define S_ISFIFO(m)	(((m) & S_IFMT) == S_IFIFO)
#define S_ISCHR(m)	(((m) & S_IFMT) == S_IFCHR)
#define S_ISDIR(m)	(((m) & S_IFMT) == S_IFDIR)
#define S_ISBLK(m)	(((m) & S_IFMT) == S_IFBLK)
#define S_ISREG(m)	(((m) & S_IFMT) == S_IFREG)
#define S_ISLNK(m)	(((m) & S_IFMT) == S_IFLNK)
#define S_ISSOCK(m)	(((m) & S_IFMT) == S_IFSOCK)

#define S_IRWXU		00700
#define S_IRUSR		00400
#define S_IWUSR		00200
#define S_IXUSR		00100
#define S_IRWXG		00070
#define S_IRGRP		00040
#define S_IWGRP		00020
#define S_IXGRP		00010
#define S_IRWXO		00007
#define S_IROTH		00004
#define S_IWOTH		00002
#define S_IXOTH		00001

int chmod(const char *path, mode_t mode);
int fchmod(int fd, mode_t mode);
int fstat(int fd, struct stat *st);
int lstat(const char *path, struct stat *st);
int mkdir(const char *path, mode_t mode);
int stat(const char *path, struct stat *st);
mode_t umask(mode_t mask);

#endif
