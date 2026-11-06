/*
 * The character classes ctype.h's macros look up, for the C locale: entry 0 is EOF,
 * then one for each byte. The shared library uses it too.
 */
#include <ctype.h>

unsigned char _ctype[1 + 256] = {
	[1 + 0x00 ... 1 + 0x08] = _C,
	[1 + 0x09 ... 1 + 0x0d] = _S | _C,
	[1 + 0x0e ... 1 + 0x1f] = _C,
	[1 + ' '] = _S | _B,
	[1 + '!' ... 1 + '/'] = _P,
	[1 + '0' ... 1 + '9'] = _N | _X,
	[1 + ':' ... 1 + '@'] = _P,
	[1 + 'A' ... 1 + 'F'] = _U | _X,
	[1 + 'G' ... 1 + 'Z'] = _U,
	[1 + '[' ... 1 + '`'] = _P,
	[1 + 'a' ... 1 + 'f'] = _L | _X,
	[1 + 'g' ... 1 + 'z'] = _L,
	[1 + '{' ... 1 + '~'] = _P,
	[1 + 0x7f] = _C,
};

/* Each class as a function too, for code that takes their addresses or #undefs them. */
#define FUNCTION(name, test)	int (name)(int c) { return (test) != 0; }
FUNCTION(isalnum, isalnum(c))
FUNCTION(isalpha, isalpha(c))
FUNCTION(isblank, isblank(c))
FUNCTION(iscntrl, iscntrl(c))
FUNCTION(isdigit, isdigit(c))
FUNCTION(isgraph, isgraph(c))
FUNCTION(islower, islower(c))
FUNCTION(isprint, isprint(c))
FUNCTION(ispunct, ispunct(c))
FUNCTION(isspace, isspace(c))
FUNCTION(isupper, isupper(c))
FUNCTION(isxdigit, isxdigit(c))

int
tolower(int c)
{
	return isupper(c) ? c - 'A' + 'a' : c;
}

int
toupper(int c)
{
	return islower(c) ? c - 'a' + 'A' : c;
}
