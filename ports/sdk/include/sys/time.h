/* A/UX 3's time of day and interval timers. */
#ifndef _SYS_TIME_H
#define _SYS_TIME_H

#include <sys/types.h>

struct timeval {
	long tv_sec;
	long tv_usec;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

#define ITIMER_REAL	0
#define ITIMER_VIRTUAL	1
#define ITIMER_PROF	2

struct itimerval {
	struct timeval it_interval;
	struct timeval it_value;
};

#define timerisset(tv)	((tv)->tv_sec || (tv)->tv_usec)
#define timerclear(tv)	((tv)->tv_sec = (tv)->tv_usec = 0)

int getitimer(int which, struct itimerval *value);
int setitimer(int which, const struct itimerval *value, struct itimerval *old);
int gettimeofday(struct timeval *tv, void *tz);
int select(int n, fd_set *r, fd_set *w, fd_set *e, struct timeval *timeout);
int utimes(const char *path, const struct timeval times[2]);

#endif
