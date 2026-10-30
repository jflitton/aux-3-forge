/* A/UX 3's time.h. Only time is in the shared library; libaux has the rest. */
#ifndef _TIME_H
#define _TIME_H

#include <sys/types.h>

#define CLOCKS_PER_SEC	100

struct tm {
	int tm_sec;
	int tm_min;
	int tm_hour;
	int tm_mday;
	int tm_mon;
	int tm_year;
	int tm_wday;
	int tm_yday;
	int tm_isdst;
};

time_t time(time_t *t);
struct tm *gmtime(const time_t *t);
struct tm *localtime(const time_t *t);
time_t mktime(struct tm *tm);
void tzset(void);

extern char *tzname[2];
extern long timezone;
extern int daylight;

#endif
