/*
 * A/UX 3's System V terminal interface: struct termio, read and set with ioctl's TCGETA
 * and TCSETA (sys/ioctl.h). libaux has no termios yet.
 */
#ifndef _TERMIO_H
#define _TERMIO_H
#include <sys/ioctl.h>

#define NCC	8
struct termio {
	unsigned short c_iflag;
	unsigned short c_oflag;
	unsigned short c_cflag;
	unsigned short c_lflag;
	char c_line;
	unsigned char c_cc[NCC];
};

/* c_cc: VMIN and VTIME replace VEOF and VEOL without ICANON */
#define VINTR	0
#define VQUIT	1
#define VERASE	2
#define VKILL	3
#define VEOF	4
#define VEOL	5
#define VEOL2	6
#define VSWTCH	7
#define VMIN	4
#define VTIME	5

/* c_iflag */
#define IGNBRK	0000001
#define BRKINT	0000002
#define IGNPAR	0000004
#define PARMRK	0000010
#define INPCK	0000020
#define ISTRIP	0000040
#define INLCR	0000100
#define IGNCR	0000200
#define ICRNL	0000400
#define IUCLC	0001000
#define IXON	0002000
#define IXANY	0004000
#define IXOFF	0010000

/* c_oflag */
#define OPOST	0000001
#define OLCUC	0000002
#define ONLCR	0000004
#define OCRNL	0000010
#define ONOCR	0000020
#define ONLRET	0000040
#define OFILL	0000100
#define OFDEL	0000200
#define NLDLY	0000400
#define CRDLY	0003000
#define TABDLY	0014000
#define TAB3	0014000
#define BSDLY	0020000
#define VTDLY	0040000
#define FFDLY	0100000

/* c_cflag */
#define CBAUD	0000017
#define B0	0
#define B50	0000001
#define B75	0000002
#define B110	0000003
#define B134	0000004
#define B150	0000005
#define B200	0000006
#define B300	0000007
#define B600	0000010
#define B1200	0000011
#define B1800	0000012
#define B2400	0000013
#define B4800	0000014
#define B9600	0000015
#define B19200	0000016
#define B38400	0000017
#define CSIZE	0000060
#define CS5	0
#define CS6	0000020
#define CS7	0000040
#define CS8	0000060
#define CSTOPB	0000100
#define CREAD	0000200
#define PARENB	0000400
#define PARODD	0001000
#define HUPCL	0002000
#define CLOCAL	0004000

/* c_lflag */
#define ISIG	0000001
#define ICANON	0000002
#define XCASE	0000004
#define ECHO	0000010
#define ECHOE	0000020
#define ECHOK	0000040
#define ECHONL	0000100
#define NOFLSH	0000200
#define IEXTEN	0001000
#define TOSTOP	0100000

#endif
