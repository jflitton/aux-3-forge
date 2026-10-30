/* A few of A/UX 3's system parameters. */
#ifndef _SYS_PARAM_H
#define _SYS_PARAM_H

#include <limits.h>
#include <sys/types.h>

#define MAXPATHLEN	PATH_MAX
#define MAXHOSTNAMELEN	64
#define NOFILE		OPEN_MAX
#define NGROUPS		NGROUPS_MAX
#define HZ		60

#endif
