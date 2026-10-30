/*
 * Limits: C's, for gcc's types, and A/UX 3's. NAME_MAX is the UFS file system's (A/UX's
 * limits.h gives 14, the System V file system's).
 */
#ifndef _LIMITS_H
#define _LIMITS_H

#define CHAR_BIT	8
#define SCHAR_MIN	(-128)
#define SCHAR_MAX	127
#define UCHAR_MAX	255
#define CHAR_MIN	SCHAR_MIN
#define CHAR_MAX	SCHAR_MAX
#define MB_LEN_MAX	1
#define SHRT_MIN	(-32768)
#define SHRT_MAX	32767
#define USHRT_MAX	65535
#define INT_MIN		(-INT_MAX - 1)
#define INT_MAX		2147483647
#define UINT_MAX	4294967295U
#define LONG_MIN	(-LONG_MAX - 1L)
#define LONG_MAX	2147483647L
#define ULONG_MAX	4294967295UL
#define LLONG_MIN	(-LLONG_MAX - 1LL)
#define LLONG_MAX	9223372036854775807LL
#define ULLONG_MAX	18446744073709551615ULL
#define SSIZE_MAX	LONG_MAX
#define LONG_BIT	32
#define WORD_BIT	32

#define ARG_MAX		20480
#define CHILD_MAX	50
#define LINK_MAX	32767
#define MAX_CANON	256
#define MAX_INPUT	256
#define NAME_MAX	255
#define NGROUPS_MAX	8
#define OPEN_MAX	128
#define PATH_MAX	1024
#define PIPE_BUF	8192
#define STREAM_MAX	128
#define TZNAME_MAX	50

#define _POSIX_ARG_MAX		4096
#define _POSIX_CHILD_MAX	6
#define _POSIX_LINK_MAX		8
#define _POSIX_MAX_CANON	255
#define _POSIX_MAX_INPUT	255
#define _POSIX_NAME_MAX		14
#define _POSIX_NGROUPS_MAX	0
#define _POSIX_OPEN_MAX		16
#define _POSIX_PATH_MAX		255
#define _POSIX_PIPE_BUF		512
#define _POSIX_SSIZE_MAX	32767
#define _POSIX_STREAM_MAX	8
#define _POSIX_TZNAME_MAX	3

#endif
