/*
 * What netsetup and dhcpc share: A/UX's network structures and ioctls (from its
 * sys/socket.h, netinet/in.h, net/if.h, net/route.h and sys/ioctl.h, which the SDK
 * doesn't have yet), and the files that hold the settings.
 *
 * Addresses are kept in host order, which on the 68k is network order too.
 */
#ifndef NET_H
#define NET_H

#include <sys/types.h>

#define AF_INET		2
#define SOCK_DGRAM	2
#define SOL_SOCKET	0xffff
#define SO_REUSEADDR	0x0004
#define SO_BROADCAST	0x0020

struct sockaddr {
	unsigned short sa_family;
	char	sa_data[14];
};
struct sockaddr_in {
	short	sin_family;
	unsigned short sin_port;
	unsigned long sin_addr;
	char	sin_zero[8];
};
struct ifreq {
	char	ifr_name[16];
	union {
		struct sockaddr_in addr;
		short	flags;
	} u;
};
struct ifconf {
	int	ifc_len;
	struct ifreq *ifc_req;
};
struct rtentry {
	unsigned long rt_hash;
	struct sockaddr_in rt_dst;
	struct sockaddr_in rt_gateway;
	short	rt_flags;
	short	rt_refcnt;
	unsigned long rt_use;
	void	*rt_ifp;
};
#define IFF_UP		0x1
#define IFF_BROADCAST	0x2
#define IFF_LOOPBACK	0x8
#define RTF_UP		0x1
#define RTF_GATEWAY	0x2

/* A/UX's numbers, which aren't 4.3BSD's: _IOW('i', 12, struct ifreq) and so on */
#define SIOCSIFADDR	0x8020690c
#define SIOCGIFADDR	0xc020690d
#define SIOCSIFFLAGS	0x80206910
#define SIOCGIFFLAGS	0xc0206911
#define SIOCGIFCONF	0xc0086914
#define SIOCGIFNETMASK	0xc0206917
#define SIOCSIFNETMASK	0x80206918
#define SIOCGIFBRDADDR	0xc020691b
#define SIOCSIFBRDADDR	0x8020691c
#define SIOCADDRT	0x8030720a
#define SIOCDELRT	0x8030720b

int socket(int domain, int type, int protocol);
int bind(int s, const void *addr, int len);
int setsockopt(int s, int level, int name, const void *val, int len);
int sendto(int s, const void *buf, int len, int flags, const void *to, int tolen);
int recvfrom(int s, void *buf, int len, int flags, void *from, int *fromlen);

/* the files */
#define NETADDRS	"/etc/NETADDRS"		/* A/UX's: IF ADDRESS BROADCAST NETMASK [GATEWAY] */
#define DHCPCONF	"/etc/dhcpc.conf"	/* interfaces that use DHCP, one a line */
#define RESOLV		"/etc/resolv.conf"
#define STATUS		"/etc/dhcpc.%.15s"		/* dhcpc's pid and lease, for an interface */
#define DHCPLOG		"/usr/adm/dhcpc.log"
#define DHCPC		"/usr/local/bin/dhcpc"

#define MAXIF		16

/* an interface as it is now */
struct iface {
	char	name[16];
	int	flags;
	unsigned long addr, mask, bcast;
};

/* a NETADDRS line */
struct manual {
	unsigned long addr, bcast, mask, gw;
};

/* net.c */
extern const char *prog;
void	die(const char *fmt, ...) __attribute__((noreturn));
void	warn(const char *fmt, ...);
char	*ip(unsigned long a);			/* "a.b.c.d", from 8 buffers in turn */
int	parseip(const char *s, unsigned long *a);
int	getifaces(struct iface *v, int max);	/* every interface, loopback too */
int	getiface(const char *name, struct iface *f);	/* -1 if there's no such */
int	setiface(const char *name, unsigned long addr, unsigned long mask,
		 unsigned long bcast);		/* mask or bcast 0: as the kernel works out */
int	setflags(const char *name, int on, int off);
int	getmac(const char *name, unsigned char mac[6]);	/* from /dev/kmem; root */
int	defaults(unsigned long *gw, char (*ifname)[16], int max);	/* default routes */
int	route(int add, unsigned long gw, int flags);	/* the default route through gw */
void	nodefaults(const char *ifname);		/* delete the default routes (through ifname) */
int	readmanual(const char *ifname, struct manual *m);	/* 0 if it has no line */
int	writemanual(const char *ifname, const struct manual *m);	/* NULL m: no line */
int	isdhcp(const char *ifname);
int	setdhcp(const char *ifname, int on);
int	getdns(unsigned long *dns, int max, char *domain, int dlen);
int	setdns(const unsigned long *dns, int n, const char *domain);	/* domain NULL: keep */
char	*readfile(const char *path);
int	writefile(const char *path, const char *text, int mode);	/* via a temporary */
char	*nextline(char **p);
int	fields(char *line, char **f, int max);	/* split at spaces and tabs, in place */
int	run(const char *path, char *const argv[]);	/* exit status, or -1 */

/* dhcp.c: DHCP's messages, over an ordinary UDP socket on port 68 */
struct lease {
	int	type;			/* the message's: OFFER, ACK or NAK */
	unsigned long addr, server, mask, bcast, router, dns[3];
	unsigned long time, t1, t2;	/* seconds: the lease, renewing, rebinding */
	int	ndns;
	char	domain[64];
};
#define OFFER	2
#define ACK	5
#define NAK	6
extern int dhcp_verbose;
void	dhcp_client(const unsigned char mac[6], const char *hostname);
int	dhcp_open(void);		/* -1 if it can't */
/* DISCOVER, OFFER, REQUEST, ACK: 1 with a lease, 0 with none in time, -1 on a NAK */
int	dhcp_get(int s, int seconds, struct lease *l);
/* the lease on addr renewed by server (broadcast if 0): 1, 0 or -1 likewise */
int	dhcp_renew(int s, unsigned long addr, unsigned long server, int seconds,
		   struct lease *l);
void	dhcp_release(int s, unsigned long addr, unsigned long server);

/* net.c: what dhcpc writes in /etc/dhcpc.IF for netsetup and for itself */
struct status {
	long	pid;			/* dhcpc's */
	int	bound;			/* it has a lease, l */
	long	since;			/* when the server last gave or renewed it */
	struct lease l;
};
int	readstatus(const char *ifname, struct status *st);	/* -1 if there's none */
int	writestatus(const char *ifname, const struct status *st);

#endif
