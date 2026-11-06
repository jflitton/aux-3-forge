/* What netsetup and dhcpc share: interfaces, routes and the files with the settings */
#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>
#include "net.h"

extern char **environ;

const char *prog = "netsetup";

void
die(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", prog);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(1);
}

void
warn(const char *fmt, ...)
{
	va_list ap;

	fprintf(stderr, "%s: ", prog);
	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
}

char *
ip(unsigned long a)
{
	static char buf[8][16];
	static int n;
	char *b = buf[n++ & 7];

	sprintf(b, "%lu.%lu.%lu.%lu", a >> 24 & 255, a >> 16 & 255, a >> 8 & 255, a & 255);
	return b;
}

int
parseip(const char *s, unsigned long *a)
{
	unsigned long v = 0, p;
	int i;

	for (i = 0; i < 4; i++) {
		if (*s < '0' || *s > '9')
			return 0;
		for (p = 0; *s >= '0' && *s <= '9' && p <= 255; s++)
			p = p * 10 + (*s - '0');
		if (p > 255 || (i < 3 && *s++ != '.'))
			return 0;
		v = v << 8 | p;
	}
	*a = v;
	return *s == 0;
}

static int
sock(void)
{
	static int s = -1;

	if (s < 0 && (s = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
		die("can't make a socket: %s", strerror(errno));
	return s;
}

static int
ifioctl(const char *name, unsigned long req, struct ifreq *r)
{
	strncpy(r->ifr_name, name, sizeof r->ifr_name - 1);
	r->ifr_name[sizeof r->ifr_name - 1] = 0;
	return ioctl(sock(), req, r);
}

static int
ifaddr(const char *name, unsigned long req, unsigned long a)
{
	struct ifreq r;

	memset(&r, 0, sizeof r);
	r.u.addr.sin_family = AF_INET;
	r.u.addr.sin_addr = a;
	return ifioctl(name, req, &r);
}

int
getiface(const char *name, struct iface *f)
{
	struct ifreq r;

	memset(f, 0, sizeof *f);
	memset(&r, 0, sizeof r);
	if (ifioctl(name, SIOCGIFFLAGS, &r) < 0)
		return -1;
	strncpy(f->name, name, sizeof f->name - 1);
	f->flags = r.u.flags & 0xffff;
	/* no address: EADDRNOTAVAIL */
	if (ifioctl(name, SIOCGIFADDR, &r) == 0)
		f->addr = r.u.addr.sin_addr;
	if (f->addr || ifioctl(name, SIOCGIFADDR, &r) == 0) {
		if (ifioctl(name, SIOCGIFNETMASK, &r) == 0)
			f->mask = r.u.addr.sin_addr;
		if ((f->flags & IFF_BROADCAST) && ifioctl(name, SIOCGIFBRDADDR, &r) == 0)
			f->bcast = r.u.addr.sin_addr;
	}
	return 0;
}

/* SIOCGIFCONF gives an entry for each address, and one for an interface with none */
int
getifaces(struct iface *v, int max)
{
	struct ifreq req[32];
	struct ifconf c;
	int i, j, n = 0;

	c.ifc_len = sizeof req;
	c.ifc_req = req;
	if (ioctl(sock(), SIOCGIFCONF, &c) < 0)
		die("can't list the interfaces: %s", strerror(errno));
	for (i = 0; i < c.ifc_len / (int)sizeof req[0] && n < max; i++) {
		req[i].ifr_name[sizeof req[i].ifr_name - 1] = 0;
		for (j = 0; j < n && strcmp(v[j].name, req[i].ifr_name); j++)
			;
		if (j == n && getiface(req[i].ifr_name, &v[n]) == 0)
			n++;
	}
	return n;
}

/*
 * The netmask goes first: SIOCSIFADDR works out the routes from it, and the broadcast
 * address from both. SIOCSIFNETMASK needs an address already, so with none it has to
 * come between two SIOCSIFADDRs. The kernel makes the netmask at least the address's
 * class's (255.0.0.0 for 0.0.0.0). Only an interface that broadcasts has a broadcast
 * address (not lo0).
 */
int
setiface(const char *name, unsigned long addr, unsigned long mask, unsigned long bcast)
{
	struct ifreq r;

	if (mask && ifaddr(name, SIOCSIFNETMASK, mask) < 0 &&
	    (ifaddr(name, SIOCSIFADDR, addr) < 0 || ifaddr(name, SIOCSIFNETMASK, mask) < 0))
		return -1;
	if (ifaddr(name, SIOCSIFADDR, addr) < 0)
		return -1;
	memset(&r, 0, sizeof r);
	if (bcast && ifioctl(name, SIOCGIFFLAGS, &r) == 0 && (r.u.flags & IFF_BROADCAST) &&
	    ifaddr(name, SIOCSIFBRDADDR, bcast) < 0)
		return -1;
	return 0;
}

int
setflags(const char *name, int on, int off)
{
	struct ifreq r;

	memset(&r, 0, sizeof r);
	if (ifioctl(name, SIOCGIFFLAGS, &r) < 0)
		return -1;
	r.u.flags = (r.u.flags | on) & ~off;
	return ifioctl(name, SIOCSIFFLAGS, &r);
}

/* ---- an interface's Ethernet address, from the kernel's list through /dev/kmem ---- */

static unsigned long
get32(const unsigned char *p)
{
	return (unsigned long)p[0] << 24 | (unsigned long)p[1] << 16 | p[2] << 8 | p[3];
}

/* name's value in /unix's COFF symbol table, or 0 */
static unsigned long
ksym(const char *name)
{
	unsigned char h[20], e[18];
	long symptr, nsyms, i;
	unsigned long v = 0;
	FILE *f = fopen("/unix", "r");

	if (!f)
		return 0;
	if (fread(h, 1, 20, f) == 20 && h[0] == 0x01 && h[1] == 0x50) {
		symptr = get32(h + 8);
		nsyms = get32(h + 12);
		fseek(f, symptr, 0);
		/* a name of 8 or fewer is in the entry itself; ours is */
		for (i = 0; i < nsyms && !v && fread(e, 1, 18, f) == 18; i++)
			if (e[16] == 2 && !strncmp((char *)e, name, 8) &&
			    (strlen(name) == 8 || !e[strlen(name)]))
				v = get32(e + 8);
	}
	fclose(f);
	return v;
}

static int
kread(int fd, unsigned long addr, void *buf, int len)
{
	return lseek(fd, (off_t)addr, 0) == -1 || read(fd, buf, len) != len ? -1 : 0;
}

/*
 * The ifnet list: if_name at 0, if_unit 4, if_flags 8, if_next 0x50; an Ethernet one is
 * the start of a struct arpcom, whose ac_enaddr is at 0x54.
 */
int
getmac(const char *name, unsigned char mac[6])
{
	unsigned char ifn[0x5a], nm[16];
	unsigned long p, head = ksym("ifnet");
	char full[24];
	int fd, guard, found = 0;

	if (!head) {
		errno = ENOENT;
		return -1;
	}
	if ((fd = open("/dev/kmem", O_RDONLY)) < 0)
		return -1;
	if (kread(fd, head, ifn, 4) == 0)
		for (p = get32(ifn), guard = 0; p && guard < 32 && !found;
		    p = get32(ifn + 0x50), guard++) {
			if (kread(fd, p, ifn, sizeof ifn) < 0 || kread(fd, get32(ifn), nm, 16) < 0)
				break;
			nm[15] = 0;
			sprintf(full, "%.15s%d", (char *)nm, ifn[4] << 8 | ifn[5]);
			if (!strcmp(full, name) && (ifn[9] & IFF_BROADCAST)) {
				memcpy(mac, ifn + 0x54, 6);
				found = 1;
			}
		}
	close(fd);
	if (!found)
		errno = ENXIO;
	return found ? 0 : -1;
}

int
fields(char *line, char **f, int max)
{
	int n = 0;

	while (n < max) {
		while (*line == ' ' || *line == '\t')
			line++;
		if (!*line)
			break;
		f[n++] = line;
		while (*line && *line != ' ' && *line != '\t')
			line++;
		if (*line)
			*line++ = 0;
	}
	return n;
}

/* ---- routes ---- */

/* The default routes, from netstat -rn: gateway and interface */
int
defaults(unsigned long *gw, char (*ifname)[16], int max)
{
	static char *argv[] = {"netstat", "-rn", NULL};
	char buf[4096], *p, *line;
	int fd[2], n = 0, len = 0, r, st;
	pid_t pid;

	if (pipe(fd) < 0)
		return 0;
	if ((pid = fork()) == 0) {
		dup2(fd[1], 1);
		close(fd[0]);
		close(fd[1]);
		execve("/usr/bin/netstat", argv, environ);
		_exit(127);
	}
	close(fd[1]);
	while (pid > 0 && len < (int)sizeof buf - 1 &&
	    (r = read(fd[0], buf + len, sizeof buf - 1 - len)) > 0)
		len += r;
	close(fd[0]);
	if (pid > 0)
		waitpid(pid, &st, 0);
	buf[len] = 0;
	p = buf;
	while ((line = nextline(&p)) && n < max) {
		char *f[6];

		if (fields(line, f, 6) == 6 && (!strcmp(f[0], "default") ||
		    !strcmp(f[0], "0.0.0.0")) && parseip(f[1], &gw[n])) {
			strncpy(ifname[n], f[5], 15);
			ifname[n++][15] = 0;
		}
	}
	return n;
}

/* The default route through gw: a gateway, or with gw this machine's address on an
   interface, that interface itself */
int
route(int add, unsigned long gw, int flags)
{
	struct rtentry rt;

	memset(&rt, 0, sizeof rt);
	rt.rt_dst.sin_family = AF_INET;
	rt.rt_gateway.sin_family = AF_INET;
	rt.rt_gateway.sin_addr = gw;
	rt.rt_flags = flags;
	return ioctl(sock(), add ? SIOCADDRT : SIOCDELRT, &rt);
}

void
nodefaults(const char *ifname)
{
	unsigned long gw[8];
	char ifs[8][16];
	int i, n = defaults(gw, ifs, 8);

	for (i = 0; i < n; i++)
		if (!ifname || !strcmp(ifs[i], ifname))
			if (route(0, gw[i], 0) < 0 && errno != ESRCH)
				warn("can't delete the default route through %s: %s", ip(gw[i]),
				     strerror(errno));
}

/* ---- files ---- */

char *
readfile(const char *path)
{
	struct stat st;
	char *buf;
	int fd, n = 0, r;

	if ((fd = open(path, O_RDONLY)) < 0)
		return NULL;
	if (fstat(fd, &st) < 0 || !(buf = malloc(st.st_size + 1))) {
		close(fd);
		return NULL;
	}
	while (n < st.st_size && (r = read(fd, buf + n, st.st_size - n)) > 0)
		n += r;
	close(fd);
	buf[n] = 0;
	return buf;
}

int
writefile(const char *path, const char *text, int mode)
{
	char tmp[64];
	int fd, n = strlen(text);

	sprintf(tmp, "%.50s.new", path);
	if ((fd = open(tmp, O_WRONLY | O_CREAT | O_TRUNC, mode)) < 0)
		return -1;
	if (write(fd, text, n) != n) {
		close(fd);
		unlink(tmp);
		return -1;
	}
	close(fd);
	chmod(tmp, mode);
	return rename(tmp, path);
}

char *
nextline(char **p)
{
	char *s = *p, *e;

	if (!s || !*s)
		return NULL;
	if ((e = strchr(s, '\n'))) {
		*e = 0;
		*p = e + 1;
	} else
		*p = s + strlen(s);
	return s;
}

/* text, less the lines for which drop says so, with room for more at the end */
static char *
filter(const char *path, int (*drop)(const char *line, const void *arg), const void *arg,
       int more)
{
	char *text = readfile(path), *p = text, *line, *out;
	int n = 0;

	out = malloc((text ? strlen(text) : 0) + more + 2);
	if (!out)
		die("out of memory");
	while ((line = nextline(&p)))
		if (!drop(line, arg))
			n += sprintf(out + n, "%s\n", line);
	out[n] = 0;
	free(text);
	return out;
}

static int
word(const char *line, char *w, int max)
{
	int n = 0;

	while (*line == ' ' || *line == '\t')
		line++;
	while (*line && *line != ' ' && *line != '\t' && n < max - 1)
		w[n++] = *line++;
	w[n] = 0;
	return n;
}

static int
isfor(const char *line, const void *ifname)
{
	char w[32];

	return word(line, w, sizeof w) && !strcmp(w, ifname);
}

int
readmanual(const char *ifname, struct manual *m)
{
	char *text = readfile(NETADDRS), *p = text, *line, *f[5];
	int found = 0, k;

	memset(m, 0, sizeof *m);
	while (!found && (line = nextline(&p))) {
		k = fields(line, f, 5);
		if (k >= 2 && !strcmp(f[0], ifname) && parseip(f[1], &m->addr)) {
			found = 1;
			if (k >= 3)
				parseip(f[2], &m->bcast);
			if (k >= 4 && !parseip(f[3], &m->mask))	/* or 0xffffff00 */
				m->mask = strtoul(f[3], NULL, 16);
			if (k >= 5)
				parseip(f[4], &m->gw);
		}
	}
	free(text);
	return found;
}

/* The line A/UX's startup scripts read, /etc/startup.d/ae6's and others': the gateway
   after them is netsetup's (and the DaynaPORT's startup script reads it too) */
int
writemanual(const char *ifname, const struct manual *m)
{
	char *out = filter(NETADDRS, isfor, ifname, 100);

	if (m) {
		sprintf(out + strlen(out), "%s\t%s %s %s", ifname, ip(m->addr), ip(m->bcast),
			ip(m->mask));
		if (m->gw)
			sprintf(out + strlen(out), " %s", ip(m->gw));
		strcat(out, "\n");
	}
	if (writefile(NETADDRS, out, 0644) < 0) {
		warn("can't write %s: %s", NETADDRS, strerror(errno));
		free(out);
		return -1;
	}
	free(out);
	return 0;
}

int
isdhcp(const char *ifname)
{
	char *text = readfile(DHCPCONF), *p = text, *line;
	int found = 0;

	while (!found && (line = nextline(&p)))
		found = *line != '#' && isfor(line, ifname);
	free(text);
	return found;
}

int
setdhcp(const char *ifname, int on)
{
	char *out = filter(DHCPCONF, isfor, ifname, 40);

	if (!strstr(out, "# "))		/* a new file */
		sprintf(out + strlen(out), "# Interfaces that get their address by DHCP "
			"(netsetup changes this)\n");
	if (on)
		sprintf(out + strlen(out), "%s\n", ifname);
	if (writefile(DHCPCONF, out, 0644) < 0) {
		warn("can't write %s: %s", DHCPCONF, strerror(errno));
		free(out);
		return -1;
	}
	free(out);
	return 0;
}

int
getdns(unsigned long *dns, int max, char *domain, int dlen)
{
	char *text = readfile(RESOLV), *p = text, *line, *f[2];
	int n = 0;

	if (domain)
		*domain = 0;
	while ((line = nextline(&p)))
		if (fields(line, f, 2) == 2) {
			if (!strcmp(f[0], "nameserver") && n < max && parseip(f[1], &dns[n]))
				n++;
			else if (!strcmp(f[0], "domain") && domain) {
				strncpy(domain, f[1], dlen - 1);
				domain[dlen - 1] = 0;
			}
		}
	free(text);
	return n;
}

static int
isdns(const char *line, const void *alsodomain)
{
	char w[32];

	word(line, w, sizeof w);
	return !strcmp(w, "nameserver") || (alsodomain && !strcmp(w, "domain"));
}

/* resolv.conf's nameserver lines (up to three: the resolver reads no more), and its domain
   line unless domain is NULL; other lines are kept */
int
setdns(const unsigned long *dns, int n, const char *domain)
{
	char *rest = filter(RESOLV, isdns, domain, 0), *out;
	int i, len = 0;

	if (!(out = malloc(strlen(rest) + 200 + (domain ? strlen(domain) : 0))))
		die("out of memory");
	if (domain && *domain)
		len += sprintf(out + len, "domain %s\n", domain);
	for (i = 0; i < n && i < 3; i++)
		len += sprintf(out + len, "nameserver %s\n", ip(dns[i]));
	strcpy(out + len, rest);
	free(rest);
	if (writefile(RESOLV, out, 0644) < 0) {
		warn("can't write %s: %s", RESOLV, strerror(errno));
		free(out);
		return -1;
	}
	free(out);
	return 0;
}

int
run(const char *path, char *const argv[])
{
	pid_t pid;
	int st;

	if ((pid = fork()) < 0)
		return -1;
	if (pid == 0) {
		execve(path, argv, environ);
		fprintf(stderr, "%s: can't run %s: %s\n", prog, path, strerror(errno));
		_exit(127);
	}
	if (waitpid(pid, &st, 0) < 0)
		return -1;
	return (st & 0xff) ? -1 : (st >> 8) & 0xff;
}

/* ---- dhcpc's status: "key value" lines ---- */

int
readstatus(const char *ifname, struct status *st)
{
	char path[40], *text, *p, *line, *f[4];
	int n;

	memset(st, 0, sizeof *st);
	sprintf(path, STATUS, ifname);
	if (!(text = readfile(path)))
		return -1;
	p = text;
	while ((line = nextline(&p))) {
		if ((n = fields(line, f, 4)) < 2)
			continue;
		if (!strcmp(f[0], "pid"))
			st->pid = strtol(f[1], NULL, 10);
		else if (!strcmp(f[0], "since"))
			st->since = strtol(f[1], NULL, 10);
		else if (!strcmp(f[0], "lease"))
			st->l.time = strtoul(f[1], NULL, 10);
		else if (!strcmp(f[0], "renew"))
			st->l.t1 = strtoul(f[1], NULL, 10);
		else if (!strcmp(f[0], "rebind"))
			st->l.t2 = strtoul(f[1], NULL, 10);
		else if (!strcmp(f[0], "address"))
			st->bound = parseip(f[1], &st->l.addr);
		else if (!strcmp(f[0], "netmask"))
			parseip(f[1], &st->l.mask);
		else if (!strcmp(f[0], "broadcast"))
			parseip(f[1], &st->l.bcast);
		else if (!strcmp(f[0], "router"))
			parseip(f[1], &st->l.router);
		else if (!strcmp(f[0], "server"))
			parseip(f[1], &st->l.server);
		else if (!strcmp(f[0], "domain")) {
			strncpy(st->l.domain, f[1], sizeof st->l.domain - 1);
		} else if (!strcmp(f[0], "dns"))
			for (st->l.ndns = 0; st->l.ndns < n - 1 && st->l.ndns < 3; st->l.ndns++)
				parseip(f[st->l.ndns + 1], &st->l.dns[st->l.ndns]);
	}
	free(text);
	return 0;
}

int
writestatus(const char *ifname, const struct status *st)
{
	char path[40], buf[512];
	const struct lease *l = &st->l;
	int n, i;

	sprintf(path, STATUS, ifname);
	n = sprintf(buf, "pid %ld\n", st->pid);
	if (st->bound) {
		n += sprintf(buf + n, "address %s\nnetmask %s\nbroadcast %s\nserver %s\n",
			     ip(l->addr), ip(l->mask), ip(l->bcast), ip(l->server));
		if (l->router)
			n += sprintf(buf + n, "router %s\n", ip(l->router));
		if (l->ndns) {
			n += sprintf(buf + n, "dns");
			for (i = 0; i < l->ndns; i++)
				n += sprintf(buf + n, " %s", ip(l->dns[i]));
			n += sprintf(buf + n, "\n");
		}
		if (l->domain[0])
			n += sprintf(buf + n, "domain %s\n", l->domain);
		n += sprintf(buf + n, "lease %lu\nrenew %lu\nrebind %lu\nsince %ld\n", l->time,
			     l->t1, l->t2, st->since);
	}
	return writefile(path, buf, 0644);
}
