/*
 * A/UX 3's signals. The shared library has kill and the BSD calls (sigvec, sigblock,
 * sigsetmask, sigpause, sigpending); libaux has signal and the POSIX ones, built on them.
 */
#ifndef _SIGNAL_H
#define _SIGNAL_H

#include <sys/types.h>

#define SIGHUP		1
#define SIGINT		2
#define SIGQUIT		3
#define SIGILL		4
#define SIGTRAP		5
#define SIGIOT		6
#define SIGABRT		6
#define SIGEMT		7
#define SIGFPE		8
#define SIGKILL		9
#define SIGBUS		10
#define SIGSEGV		11
#define SIGSYS		12
#define SIGPIPE		13
#define SIGALRM		14
#define SIGTERM		15
#define SIGUSR1		16
#define SIGUSR2		17
#define SIGCLD		18
#define SIGCHLD		18
#define SIGPWR		19
#define SIGTSTP		20
#define SIGTTIN		21
#define SIGTTOU		22
#define SIGSTOP		23
#define SIGXCPU		24
#define SIGXFSZ		25
#define SIGVTALRM	26
#define SIGPROF		27
#define SIGWINCH	28
#define SIGCONT		29
#define SIGURG		30
#define SIGIO		31
#define NSIG		32

typedef int sig_atomic_t;
typedef int sigset_t;
typedef void (*sighandler_t)(int);

#define SIG_DFL		((sighandler_t)0)
#define SIG_ERR		((sighandler_t)-1)
#define SIG_IGN		((sighandler_t)1)

struct sigaction {
	sighandler_t sa_handler;
	sigset_t sa_mask;
	int sa_flags;
};

#define SA_ONSTACK	0x0001
#define SA_INTERRUPT	0x0002
#define SA_NOCLDSTOP	0x0010
#define SA_RESTART	0x0100	/* libaux's: A/UX's restart unless SA_INTERRUPT */
#define SIG_BLOCK	1
#define SIG_UNBLOCK	2
#define SIG_SETMASK	4

struct sigvec {
	sighandler_t sv_handler;
	int sv_mask;
	int sv_flags;
};

#define SV_ONSTACK	0x0001
#define SV_INTERRUPT	0x0002
#define sigmask(m)	(1 << ((m) - 1))

int kill(pid_t pid, int sig);
int raise(int sig);
sighandler_t signal(int sig, sighandler_t handler);

int sigaction(int sig, const struct sigaction *act, struct sigaction *old);
int sigaddset(sigset_t *set, int sig);
int sigdelset(sigset_t *set, int sig);
int sigemptyset(sigset_t *set);
int sigfillset(sigset_t *set);
int sigismember(const sigset_t *set, int sig);
int sigpending(sigset_t *set);
int sigprocmask(int how, const sigset_t *set, sigset_t *old);
int sigsuspend(const sigset_t *set);

int sigvec(int sig, struct sigvec *vec, struct sigvec *old);
int sigblock(int mask);
int sigsetmask(int mask);
int sigpause(int mask);

#endif
