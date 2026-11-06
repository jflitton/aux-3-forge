/*
 * Run a program on a pseudo-terminal and type at it, to test interactive programs in
 * A/UX:
 *   ptyrun KEYS... -- PROGRAM [ARG...]
 * PROGRAM (a full path) runs in a session of its own, with the terminal as its
 * controlling terminal. Each KEYS is typed once the program has been quiet for half a
 * second; \e \r \n \t \\ and ^X (control-X) stand for what they say. Everything the
 * program writes is printed, with control characters shown as ^X and escape as \e.
 * Exits with the program's status, after closing the terminal.
 */
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <unistd.h>

static int master;

static void
show(const char *buf, int n)
{
	int i;
	unsigned char c;

	for (i = 0; i < n; i++) {
		c = buf[i];
		if (c == '\n')
			putchar('\n');
		else if (c == 033)
			fputs("\\e", stdout);
		else if (c < ' ')
			printf("^%c", c + '@');
		else if (c == 0177)
			fputs("^?", stdout);
		else
			putchar(c);
	}
	fflush(stdout);
}

/* print output until it has been quiet for ms milliseconds; 0 at end of file */
static int
drain(int ms)
{
	char buf[512];
	fd_set r;
	struct timeval tv;
	int n;

	for (;;) {
		FD_ZERO(&r);
		FD_SET(master, &r);
		tv.tv_sec = ms / 1000;
		tv.tv_usec = ms % 1000 * 1000;
		n = select(master + 1, &r, NULL, NULL, &tv);
		if (n < 0 && errno == EINTR)
			continue;
		if (n <= 0)
			return 1;
		if ((n = read(master, buf, sizeof buf)) <= 0)
			return 0;
		show(buf, n);
	}
}

static int
keys(const char *s, char *out)
{
	int n = 0;

	for (; *s; s++) {
		if (*s == '\\' && s[1]) {
			s++;
			out[n++] = *s == 'e' ? 033 : *s == 'r' ? '\r' : *s == 'n' ? '\n' :
			    *s == 't' ? '\t' : *s;
		} else if (*s == '^' && s[1]) {
			s++;
			out[n++] = *s == '?' ? 0177 : *s & 037;
		} else
			out[n++] = *s;
	}
	return n;
}

int
main(int argc, char **argv)
{
	static const char hex[] = "0123456789abcdef";
	char mname[] = "/dev/ptyp0", sname[] = "/dev/ttyp0", buf[256];
	int i, sep, slave, status, n;
	pid_t pid;

	for (sep = 1; sep < argc && strcmp(argv[sep], "--"); sep++)
		;
	if (sep + 1 >= argc) {
		fputs("usage: ptyrun KEYS... -- PROGRAM [ARG...]\n", stderr);
		return 2;
	}
	for (i = 0; i < 16; i++) {
		mname[9] = sname[9] = hex[i];
		if ((master = open(mname, O_RDWR)) >= 0)
			break;
	}
	if (master < 0) {
		perror("ptyrun: no free pseudo-terminal");
		return 2;
	}
	if ((pid = fork()) < 0) {
		perror("ptyrun: fork");
		return 2;
	}
	if (pid == 0) {
		setsid();
		close(master);
		if ((slave = open(sname, O_RDWR)) < 0) {
			perror(sname);
			_exit(127);
		}
		dup2(slave, 0);
		dup2(slave, 1);
		dup2(slave, 2);
		if (slave > 2)
			close(slave);
		execve(argv[sep + 1], argv + sep + 1, environ);
		perror(argv[sep + 1]);
		_exit(127);
	}
	printf("ptyrun: %s on %s\n", argv[sep + 1], sname);
	drain(1500);
	for (i = 1; i < sep; i++) {
		n = keys(argv[i], buf);
		printf("\n<<< %s >>>\n", argv[i]);
		write(master, buf, n);
		if (!drain(700))
			break;
	}
	drain(1000);
	close(master);
	waitpid(pid, &status, 0);
	printf("\nptyrun: status %d\n", status);
	return WIFEXITED(status) ? WEXITSTATUS(status) : 128 + WTERMSIG(status);
}
