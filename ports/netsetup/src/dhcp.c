/*
 * DHCP's messages (RFC 2131, options from RFC 2132), from a client's side. The client
 * sends from an ordinary UDP socket on port 68: before it has an address, its interface
 * is at 0.0.0.0 with broadcast address 255.255.255.255 (dhcpc.c), and it asks the server
 * to broadcast its answers, which A/UX takes in although they aren't to its address.
 */
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>
#include "net.h"

#define DISCOVER	1
#define REQUEST		3
#define RELEASE		7
#define COOKIE		0x63825363UL

static const char *mtname[] = {"?", "DISCOVER", "OFFER", "REQUEST", "DECLINE", "ACK",
			       "NAK", "RELEASE", "INFORM"};

int dhcp_verbose;
static unsigned char mac[6];
static char host[64];
static unsigned long xid;
static struct timeval t0;

static double
now(void)
{
	struct timeval t;

	gettimeofday(&t, 0);
	return (t.tv_sec - t0.tv_sec) + (t.tv_usec - t0.tv_usec) / 1e6;
}

static unsigned long
get32(const unsigned char *p)
{
	return (unsigned long)p[0] << 24 | (unsigned long)p[1] << 16 | p[2] << 8 | p[3];
}

static void
put32(unsigned char *p, unsigned long v)
{
	p[0] = v >> 24;
	p[1] = v >> 16;
	p[2] = v >> 8;
	p[3] = v;
}

void
dhcp_client(const unsigned char m[6], const char *hostname)
{
	memcpy(mac, m, 6);
	strncpy(host, hostname ? hostname : "", sizeof host - 1);
	gettimeofday(&t0, 0);
	xid = (unsigned long)t0.tv_sec ^ (unsigned long)getpid() << 16 ^ t0.tv_usec;
}

int
dhcp_open(void)
{
	struct sockaddr_in me;
	int s, on = 1;

	if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		return -1;
	setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &on, sizeof on);
	setsockopt(s, SOL_SOCKET, SO_BROADCAST, &on, sizeof on);
	memset(&me, 0, sizeof me);
	me.sin_family = AF_INET;
	me.sin_port = 68;
	if (bind(s, &me, sizeof me) < 0) {
		close(s);
		return -1;
	}
	return s;
}

static int
build(unsigned char *b, int type, unsigned long ciaddr, unsigned long req,
      unsigned long server, int bcastflag)
{
	unsigned char *o;

	memset(b, 0, 548);
	b[0] = 1;		/* BOOTREQUEST */
	b[1] = 1;		/* Ethernet */
	b[2] = 6;
	put32(b + 4, xid);
	if (bcastflag)
		b[10] = 0x80;
	put32(b + 12, ciaddr);
	memcpy(b + 28, mac, 6);
	put32(b + 236, COOKIE);
	o = b + 240;
	*o++ = 53; *o++ = 1; *o++ = type;
	*o++ = 61; *o++ = 7; *o++ = 1;		/* client identifier: Ethernet, the MAC */
	memcpy(o, mac, 6);
	o += 6;
	if (req) {
		*o++ = 50; *o++ = 4; put32(o, req); o += 4;
	}
	if (server) {
		*o++ = 54; *o++ = 4; put32(o, server); o += 4;
	}
	if (type != RELEASE) {
		*o++ = 57; *o++ = 2; *o++ = 576 >> 8; *o++ = 576 & 255;
		*o++ = 55; *o++ = 9;		/* what it wants */
		*o++ = 1; *o++ = 3; *o++ = 6; *o++ = 15; *o++ = 28; *o++ = 51; *o++ = 54;
		*o++ = 58; *o++ = 59;
		if (host[0]) {
			int n = strlen(host);

			*o++ = 12; *o++ = n;
			memcpy(o, host, n);
			o += n;
		}
	}
	*o++ = 255;
	return o - b < 300 ? 300 : o - b;	/* BOOTP's minimum */
}

static int
parse(const unsigned char *b, int n, struct lease *l)
{
	const unsigned char *o = b + 240, *end = b + n;

	memset(l, 0, sizeof *l);
	if (n < 240 || get32(b + 236) != COOKIE)
		return 0;
	l->addr = get32(b + 16);
	while (o < end && *o != 255) {
		int c = *o++, len, k;

		if (c == 0)
			continue;
		if (o >= end || o + 1 + *o > end)
			break;
		len = *o++;
		if (len >= 4)
			switch (c) {
			case 54: l->server = get32(o); break;
			case 1:  l->mask = get32(o); break;
			case 28: l->bcast = get32(o); break;
			case 3:  l->router = get32(o); break;
			case 51: l->time = get32(o); break;
			case 58: l->t1 = get32(o); break;
			case 59: l->t2 = get32(o); break;
			case 6:
				for (l->ndns = 0; l->ndns < 3 && l->ndns * 4 + 4 <= len; l->ndns++)
					l->dns[l->ndns] = get32(o + l->ndns * 4);
				break;
			}
		if (c == 53 && len >= 1)
			l->type = o[0];
		if (c == 15) {
			k = len < (int)sizeof l->domain ? len : (int)sizeof l->domain - 1;
			memcpy(l->domain, o, k);
			l->domain[k] = 0;
		}
		o += len;
	}
	return l->type != 0;
}

static int
sendmsg_to(int s, unsigned char *b, int len, unsigned long dst)
{
	struct sockaddr_in to;
	int secs = (int)now();

	b[8] = secs >> 8;
	b[9] = secs;
	memset(&to, 0, sizeof to);
	to.sin_family = AF_INET;
	to.sin_port = 67;
	to.sin_addr = dst;
	if (dhcp_verbose)
		printf("%5.1fs %s to %s\n", now(), mtname[b[242]], ip(dst));
	if (sendto(s, b, len, 0, &to, sizeof to) != len) {
		warn("can't send a DHCP %s to %s: %s", mtname[b[242]], ip(dst), strerror(errno));
		return -1;
	}
	return 0;
}

/* a reply to xid of a type in want (a bit mask), until deadline */
static int
await(int s, double deadline, int want, struct lease *l)
{
	unsigned char b[1500];
	struct sockaddr_in from;
	struct timeval tv;
	fd_set r;
	double left;
	int n, fl;

	while ((left = deadline - now()) > 0) {
		FD_ZERO(&r);
		FD_SET(s, &r);
		tv.tv_sec = (long)left;
		tv.tv_usec = (long)((left - tv.tv_sec) * 1e6);
		if ((n = select(s + 1, &r, 0, 0, &tv)) < 0 && errno != EINTR)
			return 0;
		if (n <= 0)
			continue;
		fl = sizeof from;
		if ((n = recvfrom(s, b, sizeof b, 0, &from, &fl)) < 240 || b[0] != 2 ||
		    get32(b + 4) != xid || memcmp(b + 28, mac, 6) || !parse(b, n, l))
			continue;
		if (dhcp_verbose)
			printf("%5.1fs %s from %s: %s\n", now(),
			       l->type <= 8 ? mtname[l->type] : "?", ip(from.sin_addr), ip(l->addr));
		if (l->type <= 8 && want & 1 << l->type)
			return 1;
	}
	return 0;
}

/* b to dst every 4 seconds until a wanted reply, or until deadline */
static int
exchange(int s, unsigned char *b, int len, unsigned long dst, double deadline, int want,
	 struct lease *l)
{
	double next;

	while (now() < deadline) {
		if (sendmsg_to(s, b, len, dst) < 0)
			return 0;
		next = now() + 4;
		if (await(s, next < deadline ? next : deadline, want, l))
			return 1;
	}
	return 0;
}

/* times the server left out: renew at half the lease, rebind at seven eighths */
static void
fill(struct lease *l, const struct lease *offer)
{
	if (!l->server)
		l->server = offer ? offer->server : 0;
	if (!l->t1 || l->t1 > l->time)
		l->t1 = l->time / 2;
	if (!l->t2 || l->t2 > l->time || l->t2 < l->t1)
		l->t2 = l->time / 8 * 7;
}

int
dhcp_get(int s, int seconds, struct lease *l)
{
	unsigned char b[548];
	struct lease offer;
	double deadline = now() + seconds;
	int len;

	xid++;
	len = build(b, DISCOVER, 0, 0, 0, 1);
	if (!exchange(s, b, len, 0xffffffffUL, deadline, 1 << OFFER, &offer))
		return 0;
	len = build(b, REQUEST, 0, offer.addr, offer.server, 1);
	if (!exchange(s, b, len, 0xffffffffUL, deadline > now() + 8 ? deadline : now() + 8,
		      1 << ACK | 1 << NAK, l))
		return 0;
	if (l->type == NAK)
		return -1;
	fill(l, &offer);
	return 1;
}

int
dhcp_renew(int s, unsigned long addr, unsigned long server, int seconds, struct lease *l)
{
	unsigned char b[548];
	int len;

	xid++;
	len = build(b, REQUEST, addr, 0, 0, 0);
	if (!exchange(s, b, len, server ? server : 0xffffffffUL, now() + seconds,
		      1 << ACK | 1 << NAK, l))
		return 0;
	if (l->type == NAK)
		return -1;
	fill(l, NULL);
	if (!l->server)
		l->server = server;
	return 1;
}

void
dhcp_release(int s, unsigned long addr, unsigned long server)
{
	unsigned char b[548];

	xid++;
	sendmsg_to(s, b, build(b, RELEASE, addr, 0, server, 0), server);
}
