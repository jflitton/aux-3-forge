/*
 * netsetup: sets up A/UX's network interfaces, by DHCP or by hand, now and at every boot.
 *
 *   netsetup [INTERFACE]               how each interface is set up, and its address now
 *   netsetup INTERFACE dhcp            its address by DHCP, from now on
 *   netsetup INTERFACE [address] ADDRESS [netmask MASK] [gateway GATEWAY|none]
 *            [dns SERVER[,SERVER...]|none]
 *                                      its address set by hand, from now on
 *   netsetup INTERFACE off             no address, from now on
 *   netsetup dns SERVER[,SERVER...]|none   the DNS servers
 *   netsetup boot                      sets up the gateways and DHCP; /etc/inittab runs it
 *
 * The settings are A/UX's own files: /etc/NETADDRS for an address set by hand (which
 * A/UX's startup scripts read; the gateway at the end of the line is netsetup's),
 * /etc/resolv.conf for the DNS servers, and /etc/dhcpc.conf, netsetup's, for the
 * interfaces that use DHCP. dhcpc gets and keeps their leases.
 */
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "net.h"

static void
usage(void)
{
	fprintf(stderr,
		"usage: netsetup [INTERFACE]                  show the settings\n"
		"       netsetup INTERFACE dhcp               its address by DHCP\n"
		"       netsetup INTERFACE [address] ADDRESS [netmask MASK]\n"
		"                [gateway GATEWAY|none] [dns SERVER[,SERVER...]|none]\n"
		"                                             its address set by hand\n"
		"       netsetup INTERFACE off                no address\n"
		"       netsetup dns SERVER[,SERVER...]|none  the DNS servers\n"
		"       netsetup boot                         what /etc/inittab runs at boot\n"
		"Changes last: they're made now and again at every boot.\n");
	exit(2);
}

static void
needroot(void)
{
	if (geteuid() != 0)
		die("changing the network needs root (log in as root, or use su)");
	signal(SIGHUP, SIG_IGN);	/* a telnet session over the interface may drop */
	signal(SIGINT, SIG_IGN);
}

static void
statusfile(const char *ifname, char *path)
{
	sprintf(path, STATUS, ifname);
}

static unsigned long
needip(const char *what, const char *s)
{
	unsigned long a;

	if (!s || !parseip(s, &a))
		die("%s %s isn't an address like 192.168.1.50", what, s ? s : "(none)");
	return a;
}

/* "a,b,c" or "none" into dns; how many */
static int
dnslist(const char *s, unsigned long *dns)
{
	char buf[128], *p, *e;
	int n = 0;

	if (!strcmp(s, "none"))
		return 0;
	strncpy(buf, s, sizeof buf - 1);
	buf[sizeof buf - 1] = 0;
	for (p = buf; p && *p && n < 3; p = e) {
		if ((e = strchr(p, ',')))
			*e++ = 0;
		dns[n++] = needip("DNS server", p);
	}
	return n;
}

static char *
duration(long s)
{
	static char b[32];

	if (s < 0)
		s = 0;
	if (s >= 2 * 3600)
		sprintf(b, "%ld hours", s / 3600);
	else
		sprintf(b, "%ld minutes", (s + 59) / 60);
	return b;
}

static void
showone(const struct iface *f)
{
	struct manual m;
	struct status st;
	const char *how = "";
	int dhcp = isdhcp(f->name), hand = readmanual(f->name, &m);

	if (dhcp)
		how = "DHCP";
	else if (hand)
		how = "by hand";
	printf("%-6s %-8s ", f->name, how);
	if (f->addr) {
		printf("%s, netmask %s", ip(f->addr), ip(f->mask));
		if (f->bcast)
			printf(", broadcast %s", ip(f->bcast));
	} else
		printf("no address");
	printf("%s\n", f->flags & IFF_UP ? "" : " (down)");
	if (dhcp) {
		printf("%15s", "");
		if (readstatus(f->name, &st) < 0 || st.pid <= 1 || kill(st.pid, 0) < 0)
			printf("dhcpc isn't running (netsetup %s dhcp starts it)\n", f->name);
		else if (!st.bound)
			printf("asking for a lease\n");
		else
			printf("lease from %s, renewed in %s\n", ip(st.l.server),
			       duration(st.since + (long)st.l.t1 - time(NULL)));
	} else if (hand && m.addr != f->addr)
		printf("%15s%s at the next boot\n", "", ip(m.addr));
}

static void
show(const char *only)
{
	struct iface v[MAXIF];
	unsigned long gw[8], dns[3];
	char ifs[8][16], domain[64];
	int i, n = getifaces(v, MAXIF), k;

	for (i = 0; i < n; i++)
		if (only ? !strcmp(v[i].name, only) : !(v[i].flags & IFF_LOOPBACK))
			showone(&v[i]);
	if (only)
		return;
	k = defaults(gw, ifs, 8);
	printf("%-15s", "gateway");
	for (i = 0; i < k; i++)
		printf("%s%s, through %s", i ? "; " : "", gw[i] ? ip(gw[i]) : "the interface",
		       ifs[i]);
	printf("%s\n", k ? "" : "none");
	k = getdns(dns, 3, domain, sizeof domain);
	printf("%-15s", "DNS servers");
	for (i = 0; i < k; i++)
		printf("%s%s", i ? ", " : "", ip(dns[i]));
	printf("%s", k ? "" : "none");
	if (domain[0])
		printf(" (domain %s)", domain);
	printf("\n");
}

/* stop the interface's dhcpc, if it has one, giving its lease back */
static void
nodhcp(const char *ifname)
{
	char *argv[] = {"dhcpc", "-x", (char *)ifname, NULL};

	if (isdhcp(ifname)) {
		run(DHCPC, argv);
		setdhcp(ifname, 0);
	}
}

static int
dhcp(const char *ifname)
{
	char *argv[] = {"dhcpc", (char *)ifname, NULL};
	struct manual m;

	if (readmanual(ifname, &m))
		writemanual(ifname, NULL);
	setdhcp(ifname, 1);
	printf("%s: by DHCP from now on\n", ifname);
	fflush(stdout);
	return run(DHCPC, argv) == 0 ? 0 : 1;
}

static int
off(const char *ifname)
{
	nodhcp(ifname);
	writemanual(ifname, NULL);
	nodefaults(ifname);
	if (setflags(ifname, 0, IFF_UP) < 0)
		warn("can't take %s down: %s", ifname, strerror(errno));
	printf("%s: off, now and at every boot\n", ifname);
	return 0;
}

/* addr, mask, gw and dns as given, or 0 / -1 (not given) */
static int
byhand(const char *ifname, const char *addr, const char *mask, const char *gw,
       const char *dnss)
{
	struct manual m, old;
	struct iface f;
	unsigned long dns[3];
	int have = readmanual(ifname, &old), ndns = -1;

	if (addr)
		m.addr = needip("address", addr);
	else if (have)
		m.addr = old.addr;
	else
		die("give %s an address: netsetup %s address 192.168.1.50 ...", ifname, ifname);
	m.mask = mask ? needip("netmask", mask) : have && old.mask ? old.mask : 0xffffff00UL;
	if (gw)
		m.gw = !strcmp(gw, "none") ? 0 : needip("gateway", gw);
	else if (have)
		m.gw = old.gw;
	else {
		m.gw = (m.addr & 0xffffff00UL) | 1;
		printf("%s: gateway %s, the usual one (netsetup %s gateway ... for another)\n",
		       ifname, ip(m.gw), ifname);
	}
	m.bcast = (m.addr & m.mask) | ~m.mask;
	if (dnss)
		ndns = dnslist(dnss, dns);
	else if (getdns(dns, 3, NULL, 0) == 0 && m.gw) {
		dns[0] = m.gw;
		ndns = 1;
	}

	nodhcp(ifname);
	if (setiface(ifname, m.addr, m.mask, m.bcast) < 0)
		die("can't give %s the address %s: %s", ifname, ip(m.addr), strerror(errno));
	setflags(ifname, IFF_UP, 0);
	if (m.gw) {
		nodefaults(NULL);
		if (route(1, m.gw, RTF_UP | RTF_GATEWAY) < 0 && errno != EEXIST)
			warn("can't make %s the gateway: %s", ip(m.gw), strerror(errno));
	} else
		nodefaults(ifname);
	if (writemanual(ifname, &m) < 0)
		return 1;
	if (ndns >= 0)
		setdns(dns, ndns, NULL);
	getiface(ifname, &f);
	printf("%s: %s, netmask %s, gateway %s, from now on\n", ifname, ip(f.addr),
	       ip(f.mask), m.gw ? ip(m.gw) : "none");
	return 0;
}

/* At boot, after A/UX's startup scripts have set the addresses NETADDRS gives */
static int
boot(void)
{
	struct iface v[MAXIF];
	unsigned long gw;
	char path[40], *text, *p, *line, *f[5];
	int i, n = getifaces(v, MAXIF);

	for (i = 0; i < n; i++) {	/* from before the restart: their pids mean nothing */
		statusfile(v[i].name, path);
		unlink(path);
	}
	text = readfile(NETADDRS);
	for (p = text; (line = nextline(&p));)
		if (fields(line, f, 5) == 5 && *f[0] != '#' && parseip(f[4], &gw) &&
		    route(1, gw, RTF_UP | RTF_GATEWAY) < 0 && errno != EEXIST)
			warn("can't make %s %s's gateway: %s", ip(gw), f[0], strerror(errno));
	free(text);
	text = readfile(DHCPCONF);
	for (p = text; (line = nextline(&p));)
		if (fields(line, f, 1) == 1 && *f[0] != '#') {
			char *argv[] = {"dhcpc", f[0], NULL};
			struct iface x;

			if (getiface(f[0], &x) == 0)
				run(DHCPC, argv);
			else
				printf("netsetup: %s uses DHCP, but this kernel hasn't got it\n", f[0]);
		}
	free(text);
	return 0;
}

int
main(int argc, char **argv)
{
	const char *ifname, *addr = NULL, *mask = NULL, *gw = NULL, *dnss = NULL, *how = NULL;
	struct iface f;
	unsigned long a;
	int i;

	if (argc < 2) {
		show(NULL);
		return 0;
	}
	if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "help"))
		usage();
	if (!strcmp(argv[1], "boot") && argc == 2) {
		needroot();
		return boot();
	}
	if (!strcmp(argv[1], "dns") && argc == 3) {
		unsigned long dns[3];
		int n = dnslist(argv[2], dns);

		needroot();
		return setdns(dns, n, NULL) < 0;
	}
	ifname = argv[1];
	if (getiface(ifname, &f) < 0)
		die("there's no interface %s (netsetup alone lists them)", ifname);
	if (argc == 2) {
		show(ifname);
		return 0;
	}
	/* the rest in any order, as Commando may give them */
	for (i = 2; i < argc; i++) {
		const char *w = argv[i], *v = i + 1 < argc ? argv[i + 1] : NULL;

		if (!strcmp(w, "dhcp") || !strcmp(w, "off"))
			how = w;
		else if (parseip(w, &a))
			addr = w;
		else if (!v)
			usage();
		else if (!strcmp(w, "address"))
			addr = v, i++;
		else if (!strcmp(w, "netmask"))
			mask = v, i++;
		else if (!strcmp(w, "gateway"))
			gw = v, i++;
		else if (!strcmp(w, "dns"))
			dnss = v, i++;
		else
			usage();
	}
	if (how && (addr || mask || gw || dnss))
		die("%s goes alone: an address, netmask, gateway or DNS server is for one set by "
		    "hand", how);
	needroot();
	if (how && !strcmp(how, "dhcp"))
		return dhcp(ifname);
	if (how)
		return off(ifname);
	return byhand(ifname, addr, mask, gw, dnss);
}
