/*
 * The first program built with the SDK: exercises the shared library and each piece of
 * libaux that it calls back into.
 */
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void
bye(void)
{
	printf("atexit ran\n");
}

int
main(int argc, char **argv)
{
	char line[200], *p;
	FILE *f;

	printf("hello from /shlib/libc1_s: argc %d, argv[1] %s\n", argc, argv[1]);

	/* the shared calloc calls our malloc through libc_malloc, and reads a0 */
	p = calloc(10, 10);
	strcpy(p, "calloc");
	p = realloc(p, 5000);
	printf("%s and realloc: %s\n", p, strcmp(p, "calloc") ? "wrong" : "ok");
	free(p);

	/* fopen's buffer comes from our malloc too */
	if (!(f = fopen("/etc/inittab", "r")) || !fgets(line, sizeof line, f))
		printf("fopen /etc/inittab failed\n");
	else
		printf("inittab: %s", line);

	printf("ctype: isdigit('7') %d, isalpha('7') %d, isupper('Q') %d\n",
	       isdigit('7') != 0, isalpha('7') != 0, isupper('Q') != 0);
	printf("getenv: PATH=%s\n", getenv("PATH"));
	fprintf(stderr, "stderr ok\n");
	atexit(bye);
	return 3;
}
