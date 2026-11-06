/*
 * Checks of libaux and the headers against A/UX: each line is "ok" or "FAIL" and what
 * was checked, and the exit status is the number of failures.
 */
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <pwd.h>
#include <setjmp.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <time.h>
#include <unistd.h>

static int failures;
static volatile int caught;

static void
check(int ok, const char *what)
{
	printf("%s %s\n", ok ? "ok  " : "FAIL", what);
	if (!ok) {
		printf("     errno %d: %s\n", errno, strerror(errno));
		failures++;
	}
}

static void
handler(int sig)
{
	caught = sig;
}

static jmp_buf jb;
static sigjmp_buf sjb;

/* longjmp from here leaves fp2 as this left it, unless longjmp restores it */
static void
clobber_and_jump(void)
{
	__asm__ volatile ("fmove.l #9, %%fp2" : : : "fp2");
	longjmp(jb, 7);
}

static int
fp2_kept(void)
{
	double before = 2.5, after = 0;

	__asm__ volatile ("fmove.x %0, %%fp2" : : "f"(before) : "fp2");
	if (setjmp(jb) == 0)
		clobber_and_jump();
	__asm__ volatile ("fmove.x %%fp2, %0" : "=f"(after));
	return after == 2.5;
}

int
main(void)
{
	char cwd[PATH_MAX], name[] = "/tmp/sdkXXXXXX", *end;
	struct stat st, st2;
	struct dirent *de;
	struct timeval tv;
	struct sigaction sa;
	struct tm *tm, copy;
	sigset_t set, pending;
	time_t t, back;
	DIR *d;
	FILE *f;
	int fd, found;

	setbuf(stdout, NULL);
	check(getcwd(cwd, sizeof cwd) && cwd[0] == '/', "getcwd");
	printf("     cwd %s\n", cwd);
	check(getcwd(cwd, 2) == NULL && errno == ERANGE, "getcwd too small: ERANGE");

	check(stat("/etc", &st) == 0 && S_ISDIR(st.st_mode), "stat /etc is a directory");
	fd = open("/etc", O_RDONLY);
	check(fd >= 0 && fstat(fd, &st2) == 0 && S_ISDIR(st2.st_mode), "fstat of open /etc");
	check(st.st_ino == st2.st_ino && st.st_dev == st2.st_dev, "  same inode as stat");
	printf("     /etc: dev %d ino %lu mode %o nlink %d uid %d mtime %ld size %ld\n",
	       st.st_dev, st.st_ino, st.st_mode, st.st_nlink, st.st_uid, (long)st.st_mtime,
	       (long)st.st_size);
	close(fd);

	found = 0;
	check((d = opendir("/etc")) != NULL, "opendir /etc");
	while (d && (de = readdir(d)))
		if (!strcmp(de->d_name, "inittab")) {
			check(stat("/etc/inittab", &st) == 0 && st.st_ino == de->d_ino,
			      "readdir's d_ino matches stat's st_ino");
			found = 1;
		}
	check(found, "readdir found inittab");
	if (d)
		check(closedir(d) == 0, "closedir");

	check(gettimeofday(&tv, NULL) == 0 && tv.tv_sec > 600000000, "gettimeofday");
	t = time(NULL);
	check(t >= tv.tv_sec && t - tv.tv_sec < 5, "time agrees with gettimeofday");
	tm = gmtime(&(time_t){ 946684800 });
	check(tm->tm_year == 100 && tm->tm_mon == 0 && tm->tm_mday == 1 && tm->tm_wday == 6 &&
	      tm->tm_yday == 0, "gmtime 2000-01-01 was a Saturday");
	putenv("TZ=PST8PDT");
	tm = localtime(&(time_t){ 646790400 });	/* 1990-07-01 00:00 UTC */
	printf("     localtime: %d-%02d-%02d %02d:%02d dst %d %s/%s\n", tm->tm_year + 1900,
	       tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_isdst, tzname[0],
	       tzname[1]);
	check(tm->tm_hour == 17 && tm->tm_isdst == 1, "localtime: PDT in July 1990");
	copy = *tm;
	back = mktime(&copy);
	check(back == 646790400, "mktime inverts localtime");

	check(strtol("  -0x1F", &end, 0) == -31 && *end == '\0', "strtol hex");
	check(strtoul("4294967295", NULL, 10) == 4294967295UL, "strtoul max");
	errno = 0;
	check(strtol("99999999999", NULL, 10) == LONG_MAX && errno == ERANGE, "strtol overflow");

	memset(&sa, 0, sizeof sa);
	sa.sa_handler = handler;
	check(sigaction(SIGUSR1, &sa, NULL) == 0, "sigaction");
	kill(getpid(), SIGUSR1);
	check(caught == SIGUSR1, "  handler ran");
	caught = 0;
	kill(getpid(), SIGUSR1);
	check(caught == SIGUSR1, "  and stays installed");
	sigemptyset(&set);
	sigaddset(&set, SIGUSR1);
	caught = 0;
	sigprocmask(SIG_BLOCK, &set, NULL);
	kill(getpid(), SIGUSR1);
	check(caught == 0 && sigpending(&pending) == 0 && sigismember(&pending, SIGUSR1),
	      "sigprocmask blocks, sigpending sees it");
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	check(caught == SIGUSR1, "  delivered when unblocked");

	fd = mkstemp(name);
	check(fd >= 0 && strncmp(name, "/tmp/sdk", 8) == 0, "mkstemp");
	check(dup2(fd, 9) == 9, "dup2");
	check((f = fdopen(9, "w")) != NULL && fprintf(f, "%lld\n", 1LL << 40) == 14 &&
	      fclose(f) == 0, "fdopen, fprintf, fclose");
	check(stat(name, &st) == 0 && st.st_size == 14, "  wrote 14 bytes");
	check(remove(name) == 0, "remove");

	check(strcmp(getprogname(), "sdk") == 0, "getprogname");

	{
		volatile int n = 0;
		int r = setjmp(jb);

		if (r == 0) {
			n = 1;
			longjmp(jb, 0);
		}
		check(r == 1 && n == 1, "setjmp, longjmp(env, 0) returns 1");
	}
	check(fp2_kept(), "  longjmp restores fp2");
	sigemptyset(&set);
	sigprocmask(SIG_SETMASK, &set, NULL);
	if (sigsetjmp(sjb, 1) == 0) {
		sigaddset(&set, SIGUSR1);
		sigprocmask(SIG_BLOCK, &set, NULL);
		siglongjmp(sjb, 1);
	}
	sigprocmask(SIG_BLOCK, NULL, &set);
	check(!sigismember(&set, SIGUSR1), "siglongjmp restores the signal mask");

	{
		struct passwd *pw = getpwnam("root");

		check(pw && pw->pw_uid == 0 && !strcmp(pw->pw_dir, "/"), "getpwnam root");
		pw = getpwuid(0);
		check(pw && !strcmp(pw->pw_name, "root"), "getpwuid 0");
		check(getpwnam("no such user") == NULL, "  and no one else");
	}
	check(!strcmp(strsignal(SIGTERM), "Terminated") &&
	      !strcmp(sys_siglist[SIGTSTP], "Stopped"), "strsignal, sys_siglist");

	{
		char *m;

		fd = open("/etc/passwd", O_RDONLY);
		m = mmap(NULL, 4, PROT_READ, MAP_PRIVATE, fd, 0);
		check(m != MAP_FAILED && !strncmp(m, "root", 4), "mmap a file privately");
		check(lseek(fd, 0L, SEEK_CUR) == 0, "  file offset left alone");
		check(munmap(m, 4) == 0, "munmap");
		check(mmap(NULL, 4, PROT_READ, MAP_SHARED, fd, 0) == MAP_FAILED && errno == ENODEV,
		      "  MAP_SHARED: ENODEV");
		close(fd);
	}
	fd = open("/dev/null", O_RDONLY);
	check(tcgetpgrp(fd) == -1, "tcgetpgrp of a file that isn't a terminal");
	close(fd);
	return failures;
}
