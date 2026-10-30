/*
 * stdio's data, which A/UX keeps in the program, not the shared library: the streams,
 * the 8-byte buffers of unbuffered streams, and where each stream's buffer ends. stdin
 * and stdout get buffers from malloc when first used; stderr is unbuffered.
 */
#include <stdio.h>
#include "libaux.h"

_Static_assert(sizeof(FILE) == 14, "FILE must be laid out as A/UX's");

unsigned char _smbuf[_NFILE + 1][_SBFSIZ];

FILE _iob[_NFILE] = {
	{ 0, NULL, NULL, _IOREAD, 0 },
	{ 0, NULL, NULL, _IOWRT, 1 },
	{ 0, _smbuf[2], _smbuf[2], _IOWRT | _IONBF, 2 },
};

FILE *_lastbuf = &_iob[_NFILE];

unsigned char *_bufendtab[_NFILE + 1] = { NULL, NULL, _smbuf[2] + _SBFSIZ };
