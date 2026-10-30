/*
 * Changing the environment. The shared library's getenv reads environ through the
 * pointer __aux_init gives it, so it sees these changes. A new environ array is
 * allocated the first time one is added to, and grown from then on.
 */
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static char **ours;	/* environ, once it's an array we allocated */

/* The index of name's entry in environ, or of the terminating NULL if there's none */
static int
find(const char *name, size_t len)
{
	int i;

	for (i = 0; environ[i]; i++)
		if (!strncmp(environ[i], name, len) && environ[i][len] == '=')
			break;
	return i;
}

static int
set(char *entry, size_t len, int replace)
{
	int i = find(entry, len), n;
	char **grown;

	if (environ[i]) {
		if (replace)
			environ[i] = entry;
		return 0;
	}
	for (n = i; environ[n]; n++)
		;
	if (!(grown = realloc(environ == ours ? ours : NULL, (n + 2) * sizeof *grown)))
		return -1;
	if (environ != ours)
		memcpy(grown, environ, n * sizeof *grown);
	grown[n] = entry;
	grown[n + 1] = NULL;
	environ = ours = grown;
	return 0;
}

int
putenv(char *s)
{
	char *eq = strchr(s, '=');

	if (!eq || eq == s) {
		errno = EINVAL;
		return -1;
	}
	return set(s, eq - s, 1);
}

int
setenv(const char *name, const char *value, int replace)
{
	size_t len = strlen(name), vlen = strlen(value);
	char *entry;

	if (!len || strchr(name, '=')) {
		errno = EINVAL;
		return -1;
	}
	if (environ[find(name, len)] && !replace)
		return 0;
	if (!(entry = malloc(len + vlen + 2)))
		return -1;
	memcpy(entry, name, len);
	entry[len] = '=';
	memcpy(entry + len + 1, value, vlen + 1);
	return set(entry, len, 1);
}

int
unsetenv(const char *name)
{
	size_t len = strlen(name);
	int i;

	if (!len || strchr(name, '=')) {
		errno = EINVAL;
		return -1;
	}
	while (environ[i = find(name, len)])
		for (; environ[i]; i++)
			environ[i] = environ[i + 1];
	return 0;
}
