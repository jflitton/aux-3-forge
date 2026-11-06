/* String functions A/UX's shared library lacks. gcc may also call memmove itself. */
#include <ctype.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

void *
memmove(void *dst, const void *src, size_t n)
{
	unsigned char *d = dst;
	const unsigned char *s = src;

	if (d < s)
		while (n--)
			*d++ = *s++;
	else
		while (n--)
			d[n] = s[n];
	return dst;
}

size_t
strspn(const char *s, const char *accept)
{
	const char *p = s;

	while (*p && strchr(accept, *p))
		p++;
	return p - s;
}

size_t
strcspn(const char *s, const char *reject)
{
	const char *p = s;

	while (*p && !strchr(reject, *p))
		p++;
	return p - s;
}

char *
strpbrk(const char *s, const char *accept)
{
	s += strcspn(s, accept);
	return *s ? (char *)s : NULL;
}

char *
strstr(const char *s, const char *find)
{
	size_t n = strlen(find);

	for (; *s; s++)
		if (!strncmp(s, find, n))
			return (char *)s;
	return n ? NULL : (char *)s;
}

char *
strtok(char *s, const char *sep)
{
	static char *next;
	char *tok;

	if (!s && !(s = next))
		return NULL;
	s += strspn(s, sep);
	if (!*s)
		return next = NULL;
	tok = s;
	s += strcspn(s, sep);
	if (*s)
		*s++ = '\0';
	next = s;
	return tok;
}

size_t
strnlen(const char *s, size_t max)
{
	const char *end = memchr(s, '\0', max);

	return end ? (size_t)(end - s) : max;
}

char *
strdup(const char *s)
{
	size_t n = strlen(s) + 1;
	char *p = malloc(n);

	return p ? memcpy(p, s, n) : NULL;
}

char *
strndup(const char *s, size_t max)
{
	size_t n = strnlen(s, max);
	char *p = malloc(n + 1);

	if (!p)
		return NULL;
	memcpy(p, s, n);
	p[n] = '\0';
	return p;
}

int
strcoll(const char *a, const char *b)
{
	return strcmp(a, b);
}

int
strcasecmp(const char *a, const char *b)
{
	for (;; a++, b++) {
		int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);

		if (ca != cb || !ca)
			return ca - cb;
	}
}

int
strncasecmp(const char *a, const char *b, size_t n)
{
	for (; n; a++, b++, n--) {
		int ca = tolower((unsigned char)*a), cb = tolower((unsigned char)*b);

		if (ca != cb)
			return ca - cb;
		if (!ca)
			break;
	}
	return 0;
}

/* Messages for A/UX's error numbers. */
static const char *const messages[] = {
	[0] = "Success",
	[EPERM] = "Not owner",
	[ENOENT] = "No such file or directory",
	[ESRCH] = "No such process",
	[EINTR] = "Interrupted system call",
	[EIO] = "I/O error",
	[ENXIO] = "No such device or address",
	[E2BIG] = "Argument list too long",
	[ENOEXEC] = "Exec format error",
	[EBADF] = "Bad file number",
	[ECHILD] = "No child processes",
	[EAGAIN] = "No more processes",
	[ENOMEM] = "Not enough space",
	[EACCES] = "Permission denied",
	[EFAULT] = "Bad address",
	[ENOTBLK] = "Block device required",
	[EBUSY] = "Device busy",
	[EEXIST] = "File exists",
	[EXDEV] = "Cross-device link",
	[ENODEV] = "No such device",
	[ENOTDIR] = "Not a directory",
	[EISDIR] = "Is a directory",
	[EINVAL] = "Invalid argument",
	[ENFILE] = "File table overflow",
	[EMFILE] = "Too many open files",
	[ENOTTY] = "Not a typewriter",
	[ETXTBSY] = "Text file busy",
	[EFBIG] = "File too large",
	[ENOSPC] = "No space left on device",
	[ESPIPE] = "Illegal seek",
	[EROFS] = "Read-only file system",
	[EMLINK] = "Too many links",
	[EPIPE] = "Broken pipe",
	[EDOM] = "Argument out of domain",
	[ERANGE] = "Result too large",
	[ENOMSG] = "No message of desired type",
	[EIDRM] = "Identifier removed",
	[EDEADLK] = "Deadlock condition",
	[EWOULDBLOCK] = "Operation would block",
	[EINPROGRESS] = "Operation now in progress",
	[EALREADY] = "Operation already in progress",
	[ENOTSOCK] = "Socket operation on non-socket",
	[EDESTADDRREQ] = "Destination address required",
	[EMSGSIZE] = "Message too long",
	[EPROTOTYPE] = "Protocol wrong type for socket",
	[ENOPROTOOPT] = "Protocol not available",
	[EPROTONOSUPPORT] = "Protocol not supported",
	[ESOCKTNOSUPPORT] = "Socket type not supported",
	[EOPNOTSUPP] = "Operation not supported on socket",
	[EPFNOSUPPORT] = "Protocol family not supported",
	[EAFNOSUPPORT] = "Address family not supported by protocol family",
	[EADDRINUSE] = "Address already in use",
	[EADDRNOTAVAIL] = "Can't assign requested address",
	[ENETDOWN] = "Network is down",
	[ENETUNREACH] = "Network is unreachable",
	[ENETRESET] = "Network dropped connection on reset",
	[ECONNABORTED] = "Software caused connection abort",
	[ECONNRESET] = "Connection reset by peer",
	[ENOBUFS] = "No buffer space available",
	[EISCONN] = "Socket is already connected",
	[ENOTCONN] = "Socket is not connected",
	[ESHUTDOWN] = "Can't send after socket shutdown",
	[ETOOMANYREFS] = "Too many references: can't splice",
	[ETIMEDOUT] = "Connection timed out",
	[ECONNREFUSED] = "Connection refused",
	[ELOOP] = "Too many levels of symbolic links",
	[ENAMETOOLONG] = "File name too long",
	[EHOSTDOWN] = "Host is down",
	[EHOSTUNREACH] = "No route to host",
	[ENOTEMPTY] = "Directory not empty",
	[ESTALE] = "Stale NFS file handle",
	[EREMOTE] = "Too many levels of remote in path",
	[EUSERS] = "Too many users",
	[EDQUOT] = "Disc quota exceeded",
	[ENOLCK] = "No locks available",
	[ENOSYS] = "Function not implemented",
};

char *
strerror(int err)
{
	static char unknown[] = "Unknown error 999999";

	if (err >= 0 && err < (int)(sizeof messages / sizeof *messages) && messages[err])
		return (char *)messages[err];
	if (err >= 0 && err < 1000000) {
		char *p = unknown + sizeof unknown - 1;

		*p = '\0';
		do
			*--p = '0' + err % 10;
		while ((err /= 10));
		memmove(unknown + 14, p, strlen(p) + 1);
	}
	return unknown;
}
