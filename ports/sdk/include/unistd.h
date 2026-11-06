/*
 * A/UX 3's unistd.h: what the shared library has, and libaux's additions (dup2, getcwd,
 * sleep, tcgetpgrp, tcsetpgrp).
 */
#ifndef _UNISTD_H
#define _UNISTD_H

#include <sys/types.h>

#define STDIN_FILENO	0
#define STDOUT_FILENO	1
#define STDERR_FILENO	2

#define F_OK		0
#define X_OK		1
#define W_OK		2
#define R_OK		4

#ifndef SEEK_SET
#define SEEK_SET	0
#define SEEK_CUR	1
#define SEEK_END	2
#endif

#define _POSIX_VERSION		199009L
#define _POSIX_JOB_CONTROL	1
#define _POSIX_SAVED_IDS	1
#define _POSIX_CHOWN_RESTRICTED	1
#define _POSIX_NO_TRUNC		1
#define _POSIX_VDISABLE		0377

extern char **environ;
extern char *optarg;
extern int optind, opterr, optopt;

int access(const char *path, int mode);
unsigned alarm(unsigned seconds);
int brk(void *addr);
int chdir(const char *path);
int chown(const char *path, uid_t uid, gid_t gid);
int chroot(const char *path);
int close(int fd);
int dup(int fd);
int dup2(int fd, int fd2);
int execve(const char *path, char *const argv[], char *const envp[]);
void _exit(int status) __attribute__((noreturn));
int fchown(int fd, uid_t uid, gid_t gid);
pid_t fork(void);
int fsync(int fd);
int ftruncate(int fd, off_t length);
char *getcwd(char *buf, size_t size);
int getdtablesize(void);
gid_t getegid(void);
uid_t geteuid(void);
gid_t getgid(void);
int getgroups(int n, gid_t groups[]);
int getopt(int argc, char *const argv[], const char *options);
pid_t getpgrp(void);
pid_t getpid(void);
pid_t getppid(void);
uid_t getuid(void);
int isatty(int fd);
int link(const char *path, const char *newpath);
int nice(int incr);
off_t lseek(int fd, off_t offset, int whence);
int pause(void);
int pipe(int fds[2]);
ssize_t read(int fd, void *buf, size_t n);
ssize_t readlink(const char *path, char *buf, size_t n);
int rmdir(const char *path);
void *sbrk(int n);
int setegid(gid_t gid);
int seteuid(uid_t uid);
int setgid(gid_t gid);
int setpgid(pid_t pid, pid_t pgid);
int setregid(gid_t rgid, gid_t egid);
int setreuid(uid_t ruid, uid_t euid);
pid_t setsid(void);
int setuid(uid_t uid);
unsigned sleep(unsigned seconds);
int symlink(const char *path, const char *newpath);
void sync(void);
pid_t tcgetpgrp(int fd);
int tcsetpgrp(int fd, pid_t pgrp);
int truncate(const char *path, off_t length);
int unlink(const char *path);
ssize_t write(int fd, const void *buf, size_t n);

#endif
