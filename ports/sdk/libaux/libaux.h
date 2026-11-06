/* What libaux's files share, and A/UX's names for what isn't in the public headers. */
#ifndef LIBAUX_H
#define LIBAUX_H

#include <stddef.h>
#include <stdio.h>

extern unsigned char _smbuf[_NFILE + 1][_SBFSIZ];
extern FILE *_lastbuf;

void _cleanup(void);
void __aux_init(int argc, char **argv);
void *__aux_malloc(size_t n);
void *__aux_realloc(void *p, size_t n);

#endif
