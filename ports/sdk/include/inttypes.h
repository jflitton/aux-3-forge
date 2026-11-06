/*
 * printf and scanf formats for stdint.h's types, as gcc defines them for m68k: 32-bit
 * types are long. A/UX's printf can't print 64-bit integers.
 */
#ifndef _INTTYPES_H
#define _INTTYPES_H

#include <stdint.h>

#define PRId8	"d"
#define PRId16	"d"
#define PRId32	"ld"
#define PRId64	"lld"
#define PRIi8	"i"
#define PRIi16	"i"
#define PRIi32	"li"
#define PRIi64	"lli"
#define PRIo8	"o"
#define PRIo16	"o"
#define PRIo32	"lo"
#define PRIo64	"llo"
#define PRIu8	"u"
#define PRIu16	"u"
#define PRIu32	"lu"
#define PRIu64	"llu"
#define PRIx8	"x"
#define PRIx16	"x"
#define PRIx32	"lx"
#define PRIx64	"llx"
#define PRIX8	"X"
#define PRIX16	"X"
#define PRIX32	"lX"
#define PRIX64	"llX"
#define PRIdMAX	"lld"
#define PRIuMAX	"llu"
#define PRIxMAX	"llx"
#define PRIdPTR	"ld"
#define PRIuPTR	"lu"
#define PRIxPTR	"lx"

typedef struct {
	intmax_t quot;
	intmax_t rem;
} imaxdiv_t;

#endif
