/*
 * getpwnam, getpwuid and getpwent, reading /etc/passwd as A/UX's do: name, password,
 * uid, gid, gecos, home and shell, separated by colons. A password field may end in a
 * comma and the password's age, which goes in pw_age. pw_comment is empty.
 */
#include <pwd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FILE *pwf;
static char line[512];
static struct passwd pw;

void
setpwent(void)
{
	if (pwf)
		fseek(pwf, 0L, SEEK_SET);
}

void
endpwent(void)
{
	if (pwf) {
		fclose(pwf);
		pwf = NULL;
	}
}

/* the next field of *p, ending it at sep; NULL past the end */
static char *
field(char **p, int sep)
{
	char *f = *p, *e;

	if (!f)
		return NULL;
	if ((e = strchr(f, sep))) {
		*e = '\0';
		*p = e + 1;
	} else
		*p = NULL;
	return f;
}

struct passwd *
getpwent(void)
{
	char *p, *f[7], *age;
	int i;

	if (!pwf && !(pwf = fopen("/etc/passwd", "r")))
		return NULL;
	while (fgets(line, sizeof line, pwf)) {
		line[strcspn(line, "\n")] = '\0';
		p = line;
		for (i = 0; i < 7 && (f[i] = field(&p, ':')); i++)
			;
		if (i < 7)
			continue;	/* not a whole entry */
		pw.pw_name = f[0];
		pw.pw_passwd = f[1];
		age = strchr(f[1], ',');
		if (age)
			*age++ = '\0';
		pw.pw_age = age ? age : f[1] + strlen(f[1]);
		pw.pw_uid = atoi(f[2]);
		pw.pw_gid = atoi(f[3]);
		pw.pw_comment = f[4] + strlen(f[4]);
		pw.pw_gecos = f[4];
		pw.pw_dir = f[5];
		pw.pw_shell = f[6];
		return &pw;
	}
	return NULL;
}

struct passwd *
getpwnam(const char *name)
{
	struct passwd *p;

	setpwent();
	while ((p = getpwent()) && strcmp(p->pw_name, name))
		;
	endpwent();
	return p;
}

struct passwd *
getpwuid(uid_t uid)
{
	struct passwd *p;

	setpwent();
	while ((p = getpwent()) && p->pw_uid != uid)
		;
	endpwent();
	return p;
}
