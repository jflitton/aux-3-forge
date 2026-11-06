/*
 * dhcpc: gets an interface its address by DHCP, and keeps it.
 *
 *   dhcpc [-v] [-w SECONDS] IF   ask for a lease, waiting up to SECONDS (15) for one, then
 *                                carry on in the background: renew it, or keep asking
 *   dhcpc -x IF                  give up IF's lease and stop its dhcpc
 *
 * netsetup runs it, at boot and when an interface is set to use DHCP. Its pid and lease
 * are in /etc/dhcpc.IF, and the background part notes what happens in /usr/adm/dhcpc.log.
 *
 * To ask, the interface goes to 0.0.0.0 with broadcast address 255.255.255.255, and the
 * default routes are taken out: SIOCSIFADDR makes 0.0.0.0's network route, which is
 * then the default route, so the broadcasts leave through this interface. Then the
 * lease's address, netmask, broadcast address, gateway and DNS servers are set.
 */
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "net.h"

#define YEAR	31536000UL

static const char *ifname;
static struct status st;
static int bg;				/* in the background: notes go to the log */
static unsigned long saved[8];		/* other interfaces' default routes while asking */
static char savedif[8][16];
static int nsaved;

static void
note(const char *fmt, ...)
{
	static const char *mon[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug",
				    "Sep", "Oct", "Nov", "Dec"};
	va_list ap;
	FILE *f = stdout;
	time_t t = time(NULL);
	struct tm *tm = localtime(&t);

	if (bg) {
		if (!(f = fopen(DHCPLOG, "a")))
			return;
		fprintf(f, "%s %2d %02d:%02d:%02d ", mon[tm->tm_mon], tm->tm_mday, tm->tm_hour,
			tm->tm_min, tm->tm_sec);
	}
	fprintf(f, "%s: ", ifname);
	va_start(ap, fmt);
	vfprintf(f, fmt, ap);
	va_end(ap);
	fputc('\n', f);
	if (bg)
		fclose(f);
	else
		fflush(f);
}

static char *
hours(unsigned long s)
{
	static char b[32];

	if (s >= YEAR)
		return "as long as it likes";
	if (s % 3600 == 0)
		sprintf(b, "%lu hour%s", s / 3600, s == 3600 ? "" : "s");
	else
		sprintf(b, "%lu minutes", (s + 59) / 60);
	return b;
}

/* the interface ready to ask, at 0.0.0.0 */
static void
asking(void)
{
	int i;

	nsaved = defaults(saved, savedif, 8);
	for (i = 0; i < nsaved; i++)
		route(0, saved[i], 0);
	if (setiface(ifname, 0, 0, 0xffffffffUL) < 0)
		note("can't set it to 0.0.0.0: %s", strerror(errno));
	route(1, 0, RTF_UP);	/* if SIOCSIFADDR's network route didn't take */
}

/* no lease: the other interfaces' default routes back */
static void
unasking(void)
{
	int i;

	route(0, 0, 0);
	for (i = 0; i < nsaved; i++)
		if (strcmp(savedif[i], ifname) && saved[i])
			route(1, saved[i], RTF_UP | RTF_GATEWAY);
}

static void
apply(struct lease *l)
{
	unsigned long mask = l->mask ? l->mask : 0xffffff00UL;
	int i;

	if (!l->bcast)
		l->bcast = (l->addr & mask) | ~mask;
	l->mask = mask;
	if (setiface(ifname, l->addr, mask, l->bcast) < 0)
		note("can't set its address to %s: %s", ip(l->addr), strerror(errno));
	route(0, 0, 0);
	if (l->router) {
		if (route(1, l->router, RTF_UP | RTF_GATEWAY) < 0 && errno != EEXIST)
			note("can't make %s the gateway: %s", ip(l->router), strerror(errno));
	} else
		for (i = 0; i < nsaved; i++)
			if (strcmp(savedif[i], ifname) && saved[i])
				route(1, saved[i], RTF_UP | RTF_GATEWAY);
	if (l->ndns || l->domain[0])
		setdns(l->dns, l->ndns, l->domain[0] ? l->domain : NULL);
}

static void
bound(struct lease *l)
{
	if (l->time > YEAR)
		l->time = l->t1 = l->t2 = YEAR;
	st.bound = 1;
	st.l = *l;
	st.since = time(NULL);
	writestatus(ifname, &st);
}

/* a lease within seconds, put in place: 1, or 0 with the interface left at 0.0.0.0 */
static int
acquire(int seconds)
{
	struct lease l;
	int s, r = 0;

	asking();
	if ((s = dhcp_open()) < 0)
		note("can't use UDP port 68: %s", strerror(errno));
	else {
		r = dhcp_get(s, seconds, &l);
		close(s);
	}
	if (r == -1)
		note("the DHCP server refused the address it offered");
	if (r != 1) {
		unasking();
		st.bound = 0;
		writestatus(ifname, &st);
		return 0;
	}
	apply(&l);
	bound(&l);
	note("%s from DHCP server %s: netmask %s, gateway %s, DNS %s, for %s", ip(l.addr),
	     ip(l.server), ip(l.mask), l.router ? ip(l.router) : "none",
	     l.ndns ? ip(l.dns[0]) : "none", hours(l.time));
	return 1;
}

static void
sleepuntil(long t)
{
	long now;

	while ((now = time(NULL)) < t)
		sleep(t - now > 3600 ? 3600 : t - now);
}

/* Renew at T1 straight from the server that gave it; from T2 any server, by broadcast;
   at the end of the lease, start again. 1 if it was renewed, 0 if it ran out. */
static int
keep(void)
{
	struct lease l;
	long t1 = st.since + st.l.t1, t2 = st.since + st.l.t2, end = st.since + st.l.time;
	int s, r;

	sleepuntil(t1);
	while (time(NULL) < end) {
		r = 0;
		if ((s = dhcp_open()) >= 0) {
			r = dhcp_renew(s, st.l.addr, time(NULL) < t2 ? st.l.server : 0, 10, &l);
			close(s);
		}
		if (r == 1) {
			if (l.addr != st.l.addr) {
				note("the server changed its address to %s", ip(l.addr));
				apply(&l);
			} else {
				if (!l.mask)
					l.mask = st.l.mask;
				if (!l.bcast)
					l.bcast = st.l.bcast;
			}
			bound(&l);
			return 1;
		}
		if (r == -1) {
			note("the DHCP server took %s back", ip(st.l.addr));
			return 0;
		}
		sleep(60);
	}
	note("the lease on %s ran out", ip(st.l.addr));
	return 0;
}

/* an earlier dhcpc for the interface stopped, and with release its lease given back */
static void
stop(int release)
{
	struct status old;
	unsigned char mac[6];
	char path[40];
	int s;

	if (readstatus(ifname, &old) < 0) {
		if (release)
			printf("%s: no dhcpc is running for it\n", ifname);
		return;
	}
	if (old.pid > 1 && old.pid != getpid() && kill(old.pid, 0) == 0)
		kill(old.pid, SIGTERM);
	if (release && old.bound && old.l.server && getmac(ifname, mac) == 0 &&
	    (s = dhcp_open()) >= 0) {
		dhcp_client(mac, NULL);
		dhcp_release(s, old.l.addr, old.l.server);
		close(s);
		printf("%s: gave %s back to DHCP server %s\n", ifname, ip(old.l.addr),
		       ip(old.l.server));
	}
	sprintf(path, STATUS, ifname);
	unlink(path);
}

static void
quit(int sig)
{
	_exit(0);
}

static void
usage(void)
{
	fprintf(stderr, "usage: dhcpc [-v] [-w SECONDS] INTERFACE\n"
		"       dhcpc -x INTERFACE\n");
	exit(2);
}

int
main(int argc, char **argv)
{
	unsigned char mac[6];
	char host[64] = "", *t;
	struct iface f;
	int i, wait = 15, release = 0, got, delay = 10, fd;
	FILE *h;

	prog = "dhcpc";
	signal(SIGHUP, SIG_IGN);	/* a telnet session over the interface may drop */
	for (i = 1; i < argc && argv[i][0] == '-'; i++)
		if (!strcmp(argv[i], "-v"))
			dhcp_verbose = 1;
		else if (!strcmp(argv[i], "-x"))
			release = 1;
		else if (!strcmp(argv[i], "-w") && i + 1 < argc)
			wait = atoi(argv[++i]);
		else
			usage();
	if (i != argc - 1)
		usage();
	ifname = argv[i];
	if (geteuid() != 0)
		die("only root can change the network");
	if (getiface(ifname, &f) < 0)
		die("there's no interface %s", ifname);
	if (release) {
		stop(1);
		return 0;
	}
	if (getmac(ifname, mac) < 0)
		die("can't read %s's Ethernet address from /dev/kmem: %s", ifname,
		    strerror(errno));
	stop(0);
	if ((h = fopen("/etc/HOSTNAME", "r"))) {	/* "host domain", as sethost writes it */
		if (fgets(host, sizeof host, h) && (t = strpbrk(host, " \t\n")))
			*t = 0;
		fclose(h);
	}
	dhcp_client(mac, host);
	st.pid = getpid();
	writestatus(ifname, &st);
	got = acquire(wait);
	if (!got)
		note("no DHCP server has answered yet; dhcpc keeps asking in the background");

	fflush(stdout);
	if (fork() > 0)
		return 0;
	setsid();
	signal(SIGHUP, SIG_IGN);
	signal(SIGINT, SIG_IGN);
	signal(SIGTERM, quit);
	if ((fd = open("/dev/null", O_RDWR)) >= 0) {
		dup2(fd, 0);
		dup2(fd, 1);
		dup2(fd, 2);
		if (fd > 2)
			close(fd);
	}
	bg = 1;
	st.pid = getpid();
	writestatus(ifname, &st);
	for (;;) {
		if (got) {
			delay = 10;
			got = keep();
			continue;
		}
		sleep(delay);
		if (delay < 300)
			delay *= 2;
		got = acquire(20);
	}
}
