/* A/UX 3's wait status. */
#ifndef _SYS_WAIT_H
#define _SYS_WAIT_H

#include <sys/types.h>

#define WNOHANG		1
#define WUNTRACED	2

#define WIFEXITED(s)	(((s) & 0377) == 0)
#define WEXITSTATUS(s)	(((s) >> 8) & 0377)
#define WIFSIGNALED(s)	(((s) & 0377) != 0 && ((s) & 0377) != 0177)
#define WTERMSIG(s)	((s) & 0177)
#define WIFSTOPPED(s)	(((s) & 0377) == 0177)
#define WSTOPSIG(s)	(((s) >> 8) & 0377)

pid_t wait(int *status);
pid_t waitpid(pid_t pid, int *status, int options);

#endif
