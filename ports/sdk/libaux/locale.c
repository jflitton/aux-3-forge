/* The C locale, the only one: A/UX's libc has no setlocale. */
#include <locale.h>
#include <stdlib.h>
#include <string.h>

char *
setlocale(int category, const char *name)
{
	if (!name || !*name || !strcmp(name, "C") || !strcmp(name, "POSIX"))
		return "C";
	return NULL;
}

struct lconv *
localeconv(void)
{
	static struct lconv c = { ".", "", "", "", "", "", "", "", "", "",
				  127, 127, 127, 127, 127, 127, 127, 127 };

	return &c;
}

/* Multibyte characters in the C locale: one byte each. */
int
mblen(const char *s, size_t n)
{
	return s && n ? *s != '\0' : 0;
}

int
mbtowc(wchar_t *wc, const char *s, size_t n)
{
	if (!s)
		return 0;
	if (!n)
		return -1;
	if (wc)
		*wc = (unsigned char)*s;
	return *s != '\0';
}

int
wctomb(char *s, wchar_t wc)
{
	if (!s)
		return 0;
	if ((unsigned long)wc > 0xff)
		return -1;
	*s = (char)wc;
	return 1;
}

size_t
mbstowcs(wchar_t *wcs, const char *s, size_t n)
{
	size_t i;

	for (i = 0; (!wcs || i < n) && s[i]; i++)
		if (wcs)
			wcs[i] = (unsigned char)s[i];
	if (wcs && i < n)
		wcs[i] = 0;
	return i;
}

size_t
wcstombs(char *s, const wchar_t *wcs, size_t n)
{
	size_t i;

	for (i = 0; (!s || i < n) && wcs[i]; i++) {
		if ((unsigned long)wcs[i] > 0xff)
			return (size_t)-1;
		if (s)
			s[i] = (char)wcs[i];
	}
	if (s && i < n)
		s[i] = '\0';
	return i;
}
