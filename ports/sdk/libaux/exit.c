/*
 * exit, as A/UX's: the shared library's _cleanup flushes stdio, then _exit. A/UX has no
 * atexit; this one runs its functions first, last registered first.
 */
#include <stdlib.h>
#include <unistd.h>
#include "libaux.h"

#define ATEXIT_MAX	32
static void (*handlers[ATEXIT_MAX])(void);
static int nhandlers;

int
atexit(void (*f)(void))
{
	if (nhandlers == ATEXIT_MAX)
		return -1;
	handlers[nhandlers++] = f;
	return 0;
}

void
exit(int status)
{
	while (nhandlers)
		handlers[--nhandlers]();
	_cleanup();
	_exit(status);
}
