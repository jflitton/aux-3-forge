/*
 * A/UX 3's ioctl requests for files and terminals. Their numbers are BSD's kind, holding
 * the size of what they read or write. struct termio is in termio.h.
 */
#ifndef _SYS_IOCTL_H
#define _SYS_IOCTL_H

#define IOCPARM_MASK	0x7f
#define IOC_VOID	0x20000000
#define IOC_OUT		0x40000000
#define IOC_IN		0x80000000
#define IOC_INOUT	(IOC_IN | IOC_OUT)
#define _IO(x, y)	(IOC_VOID | ((x) << 8) | (y))
#define _IOR(x, y, t)	(IOC_OUT | ((sizeof(t) & IOCPARM_MASK) << 16) | ((x) << 8) | (y))
#define _IOW(x, y, t)	(IOC_IN | ((sizeof(t) & IOCPARM_MASK) << 16) | ((x) << 8) | (y))
#define _IOWR(x, y, t)	(IOC_INOUT | ((sizeof(t) & IOCPARM_MASK) << 16) | ((x) << 8) | (y))

struct winsize {
	unsigned short ws_row;
	unsigned short ws_col;
	unsigned short ws_xpixel;
	unsigned short ws_ypixel;
};

#define FIOCLEX		_IO('f', 1)
#define FIONCLEX	_IO('f', 2)
#define FIONBIO		_IOW('f', 126, int)
#define FIONREAD	_IOR('f', 127, int)

#define TCGETA		_IOR('T', 1, struct termio)
#define TCSETA		_IOW('T', 2, struct termio)
#define TCSETAW		_IOW('T', 3, struct termio)
#define TCSETAF		_IOW('T', 4, struct termio)
#define TCSBRK		_IO('T', 5)
#define TCXONC		_IO('T', 6)
#define TCFLSH		_IO('T', 7)

/* the process group of a terminal: TC_PX_ are POSIX's, which tcgetpgrp uses */
#define TC_PX_SETPGRP	_IOW('t', 101, int)
#define TC_PX_GETPGRP	_IOR('t', 102, int)
#define TIOCSWINSZ	_IOW('t', 103, struct winsize)
#define TIOCGWINSZ	_IOR('t', 104, struct winsize)
#define TIOCNOTTY	_IO('t', 113)
#define TIOCSPGRP	_IOW('t', 118, int)
#define TIOCGPGRP	_IOR('t', 119, int)

int ioctl(int fd, int request, ...);

#endif
