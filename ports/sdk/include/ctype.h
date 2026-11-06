/*
 * A/UX 3's character classes: _ctype, in libaux, says which each byte is in. libaux
 * also has each as a function, and tolower and toupper.
 */
#ifndef _CTYPE_H
#define _CTYPE_H

#define _U	0001	/* upper case */
#define _L	0002	/* lower case */
#define _N	0004	/* digit */
#define _S	0010	/* space */
#define _P	0020	/* punctuation */
#define _C	0040	/* control */
#define _B	0100	/* blank */
#define _X	0200	/* hexadecimal digit */

extern unsigned char _ctype[];

int isalnum(int c);
int isalpha(int c);
int isblank(int c);
int iscntrl(int c);
int isdigit(int c);
int isgraph(int c);
int islower(int c);
int isprint(int c);
int ispunct(int c);
int isspace(int c);
int isupper(int c);
int isxdigit(int c);
int tolower(int c);
int toupper(int c);

#define isalnum(c)	((_ctype + 1)[c] & (_U | _L | _N))
#define isalpha(c)	((_ctype + 1)[c] & (_U | _L))
#define iscntrl(c)	((_ctype + 1)[c] & _C)
#define isdigit(c)	((_ctype + 1)[c] & _N)
#define isgraph(c)	((_ctype + 1)[c] & (_P | _U | _L | _N))
#define islower(c)	((_ctype + 1)[c] & _L)
#define isprint(c)	((_ctype + 1)[c] & (_P | _U | _L | _N | _B))
#define ispunct(c)	((_ctype + 1)[c] & _P)
#define isspace(c)	((_ctype + 1)[c] & _S)
#define isupper(c)	((_ctype + 1)[c] & _U)
#define isxdigit(c)	((_ctype + 1)[c] & _X)
#define isblank(c)	((c) == ' ' || (c) == '\t')

#endif
