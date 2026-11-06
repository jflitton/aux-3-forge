/*
 * stdlib functions whose code A/UX keeps in /lib/libc_s.a, not the shared library, and
 * ones its libc lacks.
 */
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "libaux.h"

void
abort(void)
{
	static int aborting;
	sigset_t set = sigmask(SIGABRT);

	if (!aborting++)
		_cleanup();
	sigprocmask(SIG_UNBLOCK, &set, NULL);
	signal(SIGABRT, SIG_DFL);
	kill(getpid(), SIGABRT);
	_exit(127);
}

void
__aux_assert(const char *expr, const char *file, int line, const char *func)
{
	fprintf(stderr, "%s:%d: %s: assertion `%s' failed\n", file, line, func, expr);
	abort();
}

long
labs(long n)
{
	return n < 0 ? -n : n;
}

/* strtoull's work: the magnitude, and whether it overflowed; *neg says if '-' came first */
static unsigned long long
parse(const char *s, char **end, int base, int *neg, int *over)
{
	const char *p = s, *digits;
	unsigned long long n = 0;

	*neg = *over = 0;
	while (isspace((unsigned char)*p))
		p++;
	if (*p == '-' || *p == '+')
		*neg = *p++ == '-';
	if ((base == 0 || base == 16) && p[0] == '0' && (p[1] == 'x' || p[1] == 'X') &&
	    isxdigit((unsigned char)p[2])) {
		p += 2;
		base = 16;
	} else if (base == 0)
		base = *p == '0' ? 8 : 10;
	if (base < 2 || base > 36) {
		errno = EINVAL;
		if (end)
			*end = (char *)s;
		return 0;
	}
	for (digits = p;; p++) {
		int c = (unsigned char)*p, d;

		if (isdigit(c))
			d = c - '0';
		else if (isalpha(c))
			d = tolower(c) - 'a' + 10;
		else
			break;
		if (d >= base)
			break;
		if (n > (ULLONG_MAX - d) / base)
			*over = 1;
		n = n * base + d;
	}
	if (end)
		*end = (char *)(p == digits ? s : p);
	return n;
}

unsigned long long
strtoull(const char *s, char **end, int base)
{
	int neg, over;
	unsigned long long n = parse(s, end, base, &neg, &over);

	if (over) {
		errno = ERANGE;
		return ULLONG_MAX;
	}
	return neg ? -n : n;
}

long long
strtoll(const char *s, char **end, int base)
{
	int neg, over;
	unsigned long long n = parse(s, end, base, &neg, &over);

	if (over || n > (unsigned long long)LLONG_MAX + neg) {
		errno = ERANGE;
		return neg ? LLONG_MIN : LLONG_MAX;
	}
	return neg ? -(long long)n : (long long)n;
}

unsigned long
strtoul(const char *s, char **end, int base)
{
	int neg, over;
	unsigned long long n = parse(s, end, base, &neg, &over);

	if (over || n > ULONG_MAX) {
		errno = ERANGE;
		return ULONG_MAX;
	}
	return neg ? -(unsigned long)n : (unsigned long)n;
}

long
strtol(const char *s, char **end, int base)
{
	int neg, over;
	unsigned long long n = parse(s, end, base, &neg, &over);

	if (over || n > (unsigned long long)LONG_MAX + neg) {
		errno = ERANGE;
		return neg ? LONG_MIN : LONG_MAX;
	}
	return neg ? -(long)n : (long)n;
}

void *
bsearch(const void *key, const void *base, size_t n, size_t size,
	int (*cmp)(const void *, const void *))
{
	const char *lo = base;

	while (n) {
		const char *mid = lo + (n / 2) * size;
		int c = cmp(key, mid);

		if (!c)
			return (void *)mid;
		if (c > 0) {
			lo = mid + size;
			n -= n / 2 + 1;
		} else
			n /= 2;
	}
	return NULL;
}

int
mkstemp(char *template)
{
	static const char chars[] =
		"abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
	static unsigned long seed;
	size_t len = strlen(template);
	char *x = template + len - 6;
	int tries, i, fd;

	if (len < 6 || strcmp(x, "XXXXXX")) {
		errno = EINVAL;
		return -1;
	}
	if (!seed)
		seed = (unsigned long)getpid() * 2654435761UL ^ (unsigned long)time(NULL);
	for (tries = 0; tries < 1000; tries++) {
		unsigned long r;

		seed = seed * 1103515245 + 12345;
		r = seed;
		for (i = 0; i < 6; i++, r /= 62)
			x[i] = chars[r % 62];
		if ((fd = open(template, O_RDWR | O_CREAT | O_EXCL, 0600)) >= 0 ||
		    errno != EEXIST)
			return fd;
	}
	return -1;
}

/* The program's name, from argv[0] (set by __aux_init), as BSD's getprogname gives it. */
const char *__progname = "";

const char *
getprogname(void)
{
	return __progname;
}
