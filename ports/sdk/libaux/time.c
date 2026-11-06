/*
 * gmtime, localtime and mktime, whose code A/UX keeps in /lib/libc_s.a. Local time
 * comes from the same place as A/UX's: the compiled zone file /etc/zoneinfo/$TZ
 * (/etc/zoneinfo/localtime if TZ isn't set), in the tzfile layout of A/UX's day, which is
 * today's without the "TZif" magic. If there's no such file, TZ is read as a System V
 * zone ("PST8PDT": name, hours west of Greenwich), without daylight saving.
 */
#include <ctype.h>
#include <fcntl.h>
#include <limits.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define ZONEDIR		"/etc/zoneinfo/"
#define MAXTIMES	370
#define MAXTYPES	20

struct type {
	long offset;	/* seconds east of Greenwich */
	int isdst;
	int abbr;	/* into zone.chars */
};

static struct {
	char name[64];	/* the TZ it was loaded for */
	int loaded;
	int ntimes, ntypes;
	long times[MAXTIMES];
	unsigned char index[MAXTIMES];
	struct type types[MAXTYPES];
	char chars[64];
} zone;

char *tzname[2] = { "GMT", "GMT" };
long timezone;
int daylight;

static long
get32(const unsigned char *p)
{
	return (long)((unsigned long)p[0] << 24 | (unsigned long)p[1] << 16 |
		      (unsigned long)p[2] << 8 | p[3]);
}

static int
load(const char *tz)
{
	char path[sizeof ZONEDIR + 64];
	unsigned char buf[44 + MAXTIMES * 5 + MAXTYPES * 6 + 64], *p;
	long timecnt, typecnt, charcnt;
	int fd, n, i;

	if (*tz == ':')
		tz++;
	if (strlen(tz) >= 64 || strstr(tz, ".."))
		return 0;
	strcpy(path, *tz == '/' ? "" : ZONEDIR);
	strcat(path, tz);
	if ((fd = open(path, O_RDONLY)) < 0)
		return 0;
	n = read(fd, buf, sizeof buf);
	close(fd);
	if (n < 44)
		return 0;
	timecnt = get32(buf + 32);
	typecnt = get32(buf + 36);
	charcnt = get32(buf + 40);
	if (timecnt < 0 || timecnt > MAXTIMES || typecnt < 1 || typecnt > MAXTYPES ||
	    charcnt < 0 || charcnt >= (long)sizeof zone.chars ||
	    n < 44 + timecnt * 5 + typecnt * 6 + charcnt)
		return 0;
	p = buf + 44;
	for (i = 0; i < timecnt; i++, p += 4)
		zone.times[i] = get32(p);
	for (i = 0; i < timecnt; i++, p++) {
		if (*p >= typecnt)
			return 0;
		zone.index[i] = *p;
	}
	for (i = 0; i < typecnt; i++, p += 6) {
		zone.types[i].offset = get32(p);
		zone.types[i].isdst = p[4];
		zone.types[i].abbr = p[5] < charcnt ? p[5] : 0;
	}
	memcpy(zone.chars, p, charcnt);
	zone.chars[charcnt] = '\0';
	zone.ntimes = timecnt;
	zone.ntypes = typecnt;
	return 1;
}

/* "PST8PDT", "EST5", "GMT0": a fixed offset, named by the letters before it */
static void
parse(const char *tz)
{
	int n = 0, sign = 1;
	long hours = 0, minutes = 0;

	while (isalpha((unsigned char)tz[n]) && n < 10)
		n++;
	memcpy(zone.chars, tz, n);
	zone.chars[n] = '\0';
	tz += n;
	if (*tz == '-' || *tz == '+')
		sign = *tz++ == '-' ? -1 : 1;
	while (isdigit((unsigned char)*tz))
		hours = hours * 10 + *tz++ - '0';
	if (*tz == ':')
		for (tz++; isdigit((unsigned char)*tz);)
			minutes = minutes * 10 + *tz++ - '0';
	zone.ntimes = 0;
	zone.ntypes = 1;
	zone.types[0].offset = -sign * (hours * 3600 + minutes * 60);
	zone.types[0].isdst = 0;
	zone.types[0].abbr = 0;
}

void
tzset(void)
{
	const char *tz = getenv("TZ");
	int i;

	if (!tz || !*tz)
		tz = "localtime";
	if (zone.loaded && !strcmp(zone.name, tz))
		return;
	if (!load(tz)) {
		if (!strcmp(tz, "localtime"))
			tz = "GMT0";
		parse(tz);
	}
	strncpy(zone.name, tz, sizeof zone.name - 1);
	zone.loaded = 1;

	tzname[0] = tzname[1] = zone.chars + zone.types[0].abbr;
	timezone = -zone.types[0].offset;
	daylight = 0;
	for (i = 0; i < zone.ntypes; i++) {
		if (zone.types[i].isdst) {
			daylight = 1;
			tzname[1] = zone.chars + zone.types[i].abbr;
		} else {
			tzname[0] = zone.chars + zone.types[i].abbr;
			timezone = -zone.types[i].offset;
		}
	}
}

/* The type in effect at t: the last transition at or before it, or else the first type
   that isn't daylight saving. */
static struct type *
type_at(time_t t)
{
	int i;

	for (i = zone.ntimes; i > 0; i--)
		if (zone.times[i - 1] <= t)
			return &zone.types[zone.index[i - 1]];
	for (i = 0; i < zone.ntypes; i++)
		if (!zone.types[i].isdst)
			return &zone.types[i];
	return &zone.types[0];
}

/* Days since 1970-01-01 of a date in the proleptic Gregorian calendar, and back */
static long
days_from_civil(long y, int m, int d)
{
	long era, yoe, doy;

	y -= m <= 2;
	era = (y >= 0 ? y : y - 399) / 400;
	yoe = y - era * 400;
	doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1;
	return era * 146097 + yoe * 365 + yoe / 4 - yoe / 100 + doy - 719468;
}

static struct tm *
breakdown(long long t, struct tm *tm)
{
	long long days = t / 86400;
	long secs = (long)(t % 86400), z, era, doe, yoe, doy, mp, y;

	if (secs < 0) {
		secs += 86400;
		days--;
	}
	tm->tm_hour = secs / 3600;
	tm->tm_min = secs / 60 % 60;
	tm->tm_sec = secs % 60;
	tm->tm_wday = (int)((days % 7 + 11) % 7);	/* 1970-01-01 was a Thursday */
	z = (long)days + 719468;
	era = (z >= 0 ? z : z - 146096) / 146097;
	doe = z - era * 146097;
	yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365;
	y = yoe + era * 400;
	doy = doe - (365 * yoe + yoe / 4 - yoe / 100);
	mp = (5 * doy + 2) / 153;
	tm->tm_mday = doy - (153 * mp + 2) / 5 + 1;
	tm->tm_mon = mp < 10 ? mp + 2 : mp - 10;
	y += tm->tm_mon <= 1;
	tm->tm_year = y - 1900;
	tm->tm_yday = days - days_from_civil(y, 1, 1);
	tm->tm_isdst = 0;
	return tm;
}

struct tm *
gmtime(const time_t *t)
{
	static struct tm tm;

	return breakdown(*t, &tm);
}

struct tm *
localtime(const time_t *t)
{
	static struct tm tm;
	struct type *type;

	tzset();
	type = type_at(*t);
	breakdown((long long)*t + type->offset, &tm);
	tm.tm_isdst = type->isdst;
	return &tm;
}

time_t
mktime(struct tm *tm)
{
	long long mon = tm->tm_mon, year = tm->tm_year + 1900LL + mon / 12, local, t;
	struct type *type;
	int i;

	mon %= 12;
	if (mon < 0) {
		mon += 12;
		year--;
	}
	local = days_from_civil(year, mon + 1, 1) * 86400LL + (tm->tm_mday - 1) * 86400LL +
		tm->tm_hour * 3600LL + tm->tm_min * 60LL + tm->tm_sec;
	tzset();
	/* the offset depends on the time it's for: settle it in a couple of rounds */
	type = type_at(local - zone.types[0].offset);
	for (i = 0; i < 3; i++) {
		t = local - type->offset;
		if (t < LONG_MIN || t > LONG_MAX)
			return (time_t)-1;
		if (type == type_at(t))
			break;
		type = type_at(t);
	}
	*tm = *localtime(&(time_t){ (time_t)t });
	return (time_t)t;
}
