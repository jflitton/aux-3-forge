/*
 * The shared library reaches the program's own malloc, stdio data and so on through
 * pointers in its data at fixed addresses (sdk/lib/libc1_s.ld), which each program
 * fills in as it starts. A/UX's ld does that with .init sections from /lib/libc_s.a;
 * crt0 calls this instead, with main's arguments. It also notes the program's name, and
 * puts the process in A/UX's POSIX mode, as /lib/libposix.a's startup does: reliable
 * signals (without which sigvec fails), job control, BSD groups and chown, and an
 * error rather than silent truncation for over-long names. Not COMPAT_EXEC, which
 * would pass the mode on to A/UX's own System V programs a port runs.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "libaux.h"

char **environ;

extern void *libc_ctype, *libc_malloc, *libc_free, *libc_realloc, *libc_smbuf,
	*libc_iob, *libc_lastbuf, *libc_bufendtab, *libc_environ, *libc_end,
	*libc_cleanup;
extern char end[];
extern const char *__progname;
int setcompat(int flags);

#define COMPAT_BSDGROUPS	0x0001
#define COMPAT_BSDSIGNALS	0x0004
#define COMPAT_BSDTTY		0x0008
#define COMPAT_BSDCHOWN		0x0800
#define COMPAT_BSDNOTRUNC	0x4000
#define COMPAT_POSIXFUS		0x8000

void
__aux_init(int argc, char **argv)
{
	const char *slash;

	if (argc > 0 && argv[0]) {
		slash = strrchr(argv[0], '/');
		__progname = slash ? slash + 1 : argv[0];
	}
	libc_ctype = _ctype;
	libc_malloc = (void *)malloc;
	libc_free = (void *)free;
	libc_realloc = (void *)realloc;
	libc_smbuf = _smbuf;
	libc_iob = _iob;
	libc_lastbuf = &_lastbuf;
	libc_bufendtab = _bufendtab;
	libc_environ = &environ;
	libc_end = end;
	libc_cleanup = (void *)_cleanup;
	setcompat(COMPAT_BSDGROUPS | COMPAT_BSDSIGNALS | COMPAT_BSDTTY | COMPAT_BSDCHOWN |
		  COMPAT_BSDNOTRUNC | COMPAT_POSIXFUS);
}
