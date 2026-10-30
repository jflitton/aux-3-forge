/*
 * The printf family. A/UX's _doprnt, in the shared library, predates C99: it has no
 * long long, intmax_t or size_t conversions, and no snprintf. So conversions are done
 * here, except floating point, which _doprnt does well: each %e, %f or %g goes to it
 * on its own. Not supported: %a (printed as %e), and argument positions (%1$d).
 */
#include <errno.h>
#include <limits.h>
#include <math.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int _doprnt(const char *fmt, va_list ap, FILE *f);

/* Where output goes: a string of size bytes, or a stream through a buffer. */
struct out {
	char *s;
	size_t size;
	FILE *f;
	int fd;
	char buf[128];
	size_t nbuf;
	size_t len;	/* all output, as if nothing were cut short */
	int error;
};

static void
flush(struct out *o)
{
	if (o->nbuf) {
		if (o->f) {
			if (fwrite(o->buf, 1, o->nbuf, o->f) != o->nbuf)
				o->error = 1;
		} else if (write(o->fd, o->buf, o->nbuf) != (ssize_t)o->nbuf)
			o->error = 1;
	}
	o->nbuf = 0;
}

static void
emit(struct out *o, const char *p, size_t n)
{
	if (o->s) {
		if (o->len < o->size) {
			size_t room = o->size - o->len;

			memcpy(o->s + o->len, p, n < room ? n : room);
		}
		o->len += n;
		return;
	}
	o->len += n;
	while (n > 0) {
		size_t k = sizeof o->buf - o->nbuf;

		if (k > n)
			k = n;
		memcpy(o->buf + o->nbuf, p, k);
		o->nbuf += k;
		p += k;
		n -= k;
		if (o->nbuf == sizeof o->buf)
			flush(o);
	}
}

static void
pad(struct out *o, char c, int n)
{
	char chunk[16];

	memset(chunk, c, sizeof chunk);
	for (; n > 0; n -= sizeof chunk)
		emit(o, chunk, n < (int)sizeof chunk ? (size_t)n : sizeof chunk);
}

/* _doprnt into a string that never fills, as A/UX's vsprintf */
static int
doprnt(char *s, const char *fmt, ...)
{
	FILE f;
	va_list ap;
	int n;

	f._cnt = INT_MAX;
	f._base = f._ptr = (unsigned char *)s;
	f._flag = _IOWRT;
	f._file = _NFILE;
	va_start(ap, fmt);
	n = _doprnt(fmt, ap, &f);
	va_end(ap);
	*f._ptr = '\0';
	return n;
}

#define F_MINUS	1
#define F_PLUS	2
#define F_SPACE	4
#define F_HASH	8
#define F_ZERO	16

static void
field(struct out *o, const char *prefix, const char *body, size_t n, int zeros, int width,
      int flags)
{
	int fill = width - (int)(strlen(prefix) + n) - zeros;

	if (!(flags & (F_MINUS | F_ZERO)))
		pad(o, ' ', fill);
	emit(o, prefix, strlen(prefix));
	if (flags & F_ZERO && !(flags & F_MINUS))
		pad(o, '0', fill);
	pad(o, '0', zeros);
	emit(o, body, n);
	if (flags & F_MINUS)
		pad(o, ' ', fill);
}

static void
integer(struct out *o, unsigned long long v, int neg, int conv, int flags, int width,
	int prec)
{
	char digits[24], *p = digits + sizeof digits;
	const char *xdigits = conv == 'X' ? "0123456789ABCDEF" : "0123456789abcdef";
	unsigned base = conv == 'o' ? 8 : conv == 'x' || conv == 'X' || conv == 'p' ? 16 : 10;
	char prefix[3] = "";
	int n, zeros;

	while (v) {
		*--p = xdigits[v % base];
		v /= base;
	}
	n = digits + sizeof digits - p;
	if (prec < 0)
		prec = 1;
	else
		flags &= ~F_ZERO;
	zeros = prec > n ? prec - n : 0;
	if (conv == 'd' || conv == 'i')
		strcpy(prefix, neg ? "-" : flags & F_PLUS ? "+" : flags & F_SPACE ? " " : "");
	else if (conv == 'o' && flags & F_HASH && !zeros && (n == 0 || *p != '0'))
		zeros = 1;
	else if ((conv == 'x' || conv == 'X') && flags & F_HASH && n)
		strcpy(prefix, conv == 'x' ? "0x" : "0X");
	else if (conv == 'p')
		strcpy(prefix, "0x");
	field(o, prefix, p, n, zeros, width, flags);
}

static void
floating(struct out *o, double v, int conv, int flags, int width, int prec)
{
	char spec[32], small[64], *buf = small, *q = spec;
	int upper = conv == 'E' || conv == 'F' || conv == 'G' || conv == 'A';
	size_t need;
	int e;

	if (isnan(v) || isinf(v)) {
		const char *text = isnan(v) ? (upper ? "NAN" : "nan") : (upper ? "INF" : "inf");

		field(o, signbit(v) ? "-" : flags & F_PLUS ? "+" : flags & F_SPACE ? " " : "",
		      text, 3, 0, width, flags & ~F_ZERO);
		return;
	}
	if (conv == 'a' || conv == 'A')
		conv = conv == 'a' ? 'e' : 'E';
	if (conv == 'F')
		conv = 'f';
	*q++ = '%';
	if (flags & F_MINUS)
		*q++ = '-';
	if (flags & F_PLUS)
		*q++ = '+';
	if (flags & F_SPACE)
		*q++ = ' ';
	if (flags & F_HASH)
		*q++ = '#';
	if (flags & F_ZERO)
		*q++ = '0';
	*q++ = '*';
	if (prec >= 0) {
		*q++ = '.';
		*q++ = '*';
	}
	*q++ = conv;
	*q = '\0';
	frexp(v, &e);
	need = (e > 0 ? e * 3 / 10 + 2 : 2) + (prec >= 0 ? prec : 6) + width + 16;
	if (need > sizeof small && !(buf = malloc(need))) {
		o->error = 1;
		return;
	}
	if (prec >= 0)
		doprnt(buf, spec, width, prec, v);
	else
		doprnt(buf, spec, width, v);
	emit(o, buf, strlen(buf));
	if (buf != small)
		free(buf);
}

static int
format(struct out *o, const char *fmt, va_list ap)
{
	while (*fmt) {
		const char *start = fmt;
		int flags = 0, width = 0, prec = -1, size = 0;
		unsigned long long v;
		int neg;

		const char *pct;

		while (*fmt && *fmt != '%')
			fmt++;
		emit(o, start, fmt - start);
		if (!*fmt)
			break;
		pct = fmt;
		for (fmt++;; fmt++) {
			if (*fmt == '-')
				flags |= F_MINUS;
			else if (*fmt == '+')
				flags |= F_PLUS;
			else if (*fmt == ' ')
				flags |= F_SPACE;
			else if (*fmt == '#')
				flags |= F_HASH;
			else if (*fmt == '0')
				flags |= F_ZERO;
			else if (*fmt != '\'')
				break;
		}
		if (*fmt == '*') {
			width = va_arg(ap, int);
			if (width < 0) {
				flags |= F_MINUS;
				width = -width;
			}
			fmt++;
		} else
			while (*fmt >= '0' && *fmt <= '9')
				width = width * 10 + *fmt++ - '0';
		if (*fmt == '.') {
			fmt++;
			prec = 0;
			if (*fmt == '*') {
				prec = va_arg(ap, int);
				fmt++;
			} else
				while (*fmt >= '0' && *fmt <= '9')
					prec = prec * 10 + *fmt++ - '0';
		}
		/* size: 'H' for hh, 'h', 'l', 'q' for ll and j, 'z' for z and t, 'L' */
		for (;; fmt++) {
			if (*fmt == 'h')
				size = size == 'h' ? 'H' : 'h';
			else if (*fmt == 'l')
				size = size == 'l' ? 'q' : 'l';
			else if (*fmt == 'q' || *fmt == 'j')
				size = 'q';
			else if (*fmt == 'z' || *fmt == 't')
				size = 'z';
			else if (*fmt == 'L')
				size = 'L';
			else
				break;
		}
		switch (*fmt) {
		case 'd':
		case 'i':
			if (size == 'q') {
				long long n = va_arg(ap, long long);

				neg = n < 0;
				v = neg ? -(unsigned long long)n : (unsigned long long)n;
			} else {
				long n = size == 'l' || size == 'z' ? va_arg(ap, long) :
					va_arg(ap, int);

				if (size == 'h')
					n = (short)n;
				else if (size == 'H')
					n = (signed char)n;
				neg = n < 0;
				v = neg ? -(unsigned long)n : (unsigned long)n;
			}
			integer(o, v, neg, *fmt, flags, width, prec);
			break;
		case 'u':
		case 'o':
		case 'x':
		case 'X':
			if (size == 'q')
				v = va_arg(ap, unsigned long long);
			else if (size == 'l' || size == 'z')
				v = va_arg(ap, unsigned long);
			else {
				v = va_arg(ap, unsigned);
				if (size == 'h')
					v = (unsigned short)v;
				else if (size == 'H')
					v = (unsigned char)v;
			}
			integer(o, v, 0, *fmt, flags, width, prec);
			break;
		case 'p':
			integer(o, (uintptr_t)va_arg(ap, void *), 0, 'p', flags, width, prec);
			break;
		case 'c': {
			char c = (char)va_arg(ap, int);

			field(o, "", &c, 1, 0, width, flags & ~F_ZERO);
			break;
		}
		case 's':
			if (size == 'l') {
				const wchar_t *ws = va_arg(ap, const wchar_t *);
				char c;
				int n = 0, i;

				if (!ws)
					ws = L"(null)";
				while (ws[n] && (prec < 0 || n < prec))
					n++;
				if (!(flags & F_MINUS))
					pad(o, ' ', width - n);
				for (i = 0; i < n; i++) {
					c = (char)ws[i];
					emit(o, &c, 1);
				}
				if (flags & F_MINUS)
					pad(o, ' ', width - n);
			} else {
				const char *s = va_arg(ap, const char *);

				if (!s)
					s = "(null)";
				field(o, "", s, prec < 0 ? strlen(s) : strnlen(s, prec), 0, width,
				      flags & ~F_ZERO);
			}
			break;
		case 'e':
		case 'E':
		case 'f':
		case 'F':
		case 'g':
		case 'G':
		case 'a':
		case 'A':
			floating(o, size == 'L' ? (double)va_arg(ap, long double) :
				 va_arg(ap, double), *fmt, flags, width, prec);
			break;
		case 'n':
			if (size == 'q')
				*va_arg(ap, long long *) = o->len;
			else if (size == 'l' || size == 'z')
				*va_arg(ap, long *) = o->len;
			else if (size == 'h')
				*va_arg(ap, short *) = o->len;
			else if (size == 'H')
				*va_arg(ap, signed char *) = o->len;
			else
				*va_arg(ap, int *) = o->len;
			break;
		case '%':
			emit(o, "%", 1);
			break;
		default:
			/* not a conversion: print it as it was */
			emit(o, pct, (*fmt ? fmt + 1 : fmt) - pct);
			if (!*fmt)
				continue;
			break;
		}
		fmt++;
	}
	if (!o->s)
		flush(o);
	if (o->error || o->len > INT_MAX) {
		if (!o->error)
			errno = ERANGE;
		return -1;
	}
	return (int)o->len;
}

int
vsnprintf(char *s, size_t size, const char *fmt, va_list ap)
{
	struct out o = { 0 };
	char dummy;
	int n;

	o.s = size ? s : &dummy;
	o.size = size ? size - 1 : 0;
	n = format(&o, fmt, ap);
	if (size)
		s[o.len < o.size ? o.len : o.size] = '\0';
	return n;
}

int
vsprintf(char *s, const char *fmt, va_list ap)
{
	return vsnprintf(s, INT_MAX, fmt, ap);
}

int
vfprintf(FILE *f, const char *fmt, va_list ap)
{
	struct out o = { 0 };

	if (!(f->_flag & _IOWRT)) {
		if (!(f->_flag & _IORW)) {
			errno = EBADF;
			return -1;
		}
		f->_flag |= _IOWRT;
	}
	o.f = f;
	return format(&o, fmt, ap);
}

int
vprintf(const char *fmt, va_list ap)
{
	return vfprintf(stdout, fmt, ap);
}

int
vdprintf(int fd, const char *fmt, va_list ap)
{
	struct out o = { 0 };

	o.fd = fd;
	return format(&o, fmt, ap);
}

int
vasprintf(char **sp, const char *fmt, va_list ap)
{
	va_list ap2;
	int n;

	va_copy(ap2, ap);
	n = vsnprintf(NULL, 0, fmt, ap2);
	va_end(ap2);
	if (n < 0 || !(*sp = malloc(n + 1)))
		return -1;
	return vsnprintf(*sp, n + 1, fmt, ap);
}

#define VARIADIC(call)	{ va_list ap; int n; va_start(ap, fmt); n = call; va_end(ap); \
			  return n; }

int printf(const char *fmt, ...) VARIADIC(vfprintf(stdout, fmt, ap))
int fprintf(FILE *f, const char *fmt, ...) VARIADIC(vfprintf(f, fmt, ap))
int sprintf(char *s, const char *fmt, ...) VARIADIC(vsnprintf(s, INT_MAX, fmt, ap))
int snprintf(char *s, size_t size, const char *fmt, ...) VARIADIC(vsnprintf(s, size, fmt, ap))
int dprintf(int fd, const char *fmt, ...) VARIADIC(vdprintf(fd, fmt, ap))
int asprintf(char **sp, const char *fmt, ...) VARIADIC(vasprintf(sp, fmt, ap))
