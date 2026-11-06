/* What A/UX 3's shared library has of stdlib, and libaux's additions. */
#ifndef _STDLIB_H
#define _STDLIB_H

#include <stddef.h>

#define EXIT_SUCCESS	0
#define EXIT_FAILURE	1
#define MB_CUR_MAX	1

void *malloc(size_t n);
void *calloc(size_t n, size_t size);
void *realloc(void *p, size_t n);
void free(void *p);

int atexit(void (*f)(void));
void exit(int status) __attribute__((noreturn));

int abs(int n);
int atoi(const char *s);
long atol(const char *s);
double atof(const char *s);
char *getenv(const char *name);
void qsort(void *base, size_t n, size_t size, int (*cmp)(const void *, const void *));

/* libaux */
void abort(void) __attribute__((noreturn));
void *bsearch(const void *key, const void *base, size_t n, size_t size,
	      int (*cmp)(const void *, const void *));
const char *getprogname(void);
long labs(long n);
int mblen(const char *s, size_t n);
size_t mbstowcs(wchar_t *wcs, const char *s, size_t n);
int mbtowc(wchar_t *wc, const char *s, size_t n);
int mkstemp(char *template);
int putenv(char *s);
int setenv(const char *name, const char *value, int replace);
long strtol(const char *s, char **end, int base);
long long strtoll(const char *s, char **end, int base);
unsigned long strtoul(const char *s, char **end, int base);
unsigned long long strtoull(const char *s, char **end, int base);
int unsetenv(const char *name);
size_t wcstombs(char *s, const wchar_t *wcs, size_t n);
int wctomb(char *s, wchar_t wc);

#endif
