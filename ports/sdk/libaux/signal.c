/*
 * signal and the POSIX signal calls, on the shared library's BSD ones. A signal set is
 * a mask with bit sig - 1, as sigblock's. signal keeps the handler installed and
 * restarts interrupted calls, as BSD's and glibc's do; sigaction interrupts them unless
 * given SA_RESTART.
 */
#include <errno.h>
#include <signal.h>
#include <unistd.h>

sighandler_t
signal(int sig, sighandler_t handler)
{
	struct sigvec vec = { handler, 0, 0 }, old;

	if (sigvec(sig, &vec, &old) < 0)
		return SIG_ERR;
	return old.sv_handler;
}

int
sigaction(int sig, const struct sigaction *act, struct sigaction *old)
{
	struct sigvec vec, ovec;

	if (act) {
		vec.sv_handler = act->sa_handler;
		vec.sv_mask = act->sa_mask;
		vec.sv_flags = act->sa_flags & (SA_ONSTACK | SA_NOCLDSTOP);
		if (!(act->sa_flags & SA_RESTART))
			vec.sv_flags |= SV_INTERRUPT;
	}
	if (sigvec(sig, act ? &vec : NULL, &ovec) < 0)
		return -1;
	if (old) {
		old->sa_handler = ovec.sv_handler;
		old->sa_mask = ovec.sv_mask;
		old->sa_flags = ovec.sv_flags & (SA_ONSTACK | SA_NOCLDSTOP);
		if (!(ovec.sv_flags & SV_INTERRUPT))
			old->sa_flags |= SA_RESTART;
	}
	return 0;
}

int
sigprocmask(int how, const sigset_t *set, sigset_t *old)
{
	int was;

	if (!set)
		was = sigblock(0);
	else if (how == SIG_BLOCK)
		was = sigblock(*set);
	else if (how == SIG_UNBLOCK) {
		was = sigblock(0);
		sigsetmask(was & ~*set);
	} else if (how == SIG_SETMASK)
		was = sigsetmask(*set);
	else {
		errno = EINVAL;
		return -1;
	}
	if (old)
		*old = was;
	return 0;
}

int
sigsuspend(const sigset_t *set)
{
	return sigpause(*set);
}

static int
valid(int sig)
{
	if (sig > 0 && sig < NSIG)
		return 1;
	errno = EINVAL;
	return 0;
}

int
sigemptyset(sigset_t *set)
{
	*set = 0;
	return 0;
}

int
sigfillset(sigset_t *set)
{
	*set = ~0;
	return 0;
}

int
sigaddset(sigset_t *set, int sig)
{
	if (!valid(sig))
		return -1;
	*set |= sigmask(sig);
	return 0;
}

int
sigdelset(sigset_t *set, int sig)
{
	if (!valid(sig))
		return -1;
	*set &= ~sigmask(sig);
	return 0;
}

int
sigismember(const sigset_t *set, int sig)
{
	if (!valid(sig))
		return -1;
	return (*set & sigmask(sig)) != 0;
}

int
raise(int sig)
{
	return kill(getpid(), sig);
}
