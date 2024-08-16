/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/netnamer.c	1.3.9.7"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * netnamer.c
 */

#define fopen _fopen	/* Create a prototype for _fopen using <stdio.h> */
#include <stdio.h>
#undef fopen

#include <sys/param.h>
#include <rpc/rpc.h>
#include <ctype.h>

#include <grp.h>
#include <pwd.h>
#include <string.h>
#include <syslog.h>
#include "trace.h"

static char    *OPSYS	  = "unix";
static char    *NETID	  = "netid.byname";
static char    *NETIDFILE = "/etc/netid";

extern char     *gettxt(const char *, const char *);

#ifndef NGROUPS
#define	NGROUPS 16
#endif

#define setgrent _abi_setgrent
#define getgrent _abi_getgrent
#define endgrent _abi_endgrent
/*
 * Convert network-name into unix credential
 */
netname2user(netname, uidp, gidp, gidlenp, gidlist)
	char netname[MAXNETNAMELEN + 1];
	uid_t *uidp;
	gid_t *gidp;
	int *gidlenp;
	gid_t *gidlist;
{
	char *p;
	int gidlen;
	uid_t uid;
	struct passwd *pwd;
	char val[1024];
	char *val1, *val2;
	char *domain;
	int vallen;
	char *last;		/* storage for reentrant strtok_r */

        trace1(TR_netname2user, 0);
	if (getnetid(netname, val)) {
		p = strtok_r(val, ":", &last);
		if (p == NULL) {
			trace1(TR_netname2user, 1);
			return (0);
		}
		*uidp = (uid_t) atol(val);
		p = strtok_r(NULL, "\n,", &last);
		*gidp = (gid_t) atol(p);
		if (p == NULL) {
			trace1(TR_netname2user, 1);
			return (0);
		}
		gidlen = 0;
		for (gidlen = 0; gidlen < NGROUPS; gidlen++) {
			p = strtok_r(NULL, "\n,", &last);
			if (p == NULL)
				break;
			gidlist[gidlen] = (gid_t) atol(p);
		}
		*gidlenp = gidlen;

		trace1(TR_netname2user, 1);
		return (1);
	}
	val1 = strchr(netname, '.');
	if ((val1 == NULL) || strncmp(netname, OPSYS, (val1-netname))) {
		trace1(TR_netname2user, 1);
		return (0);
	}
	val1++;
	val2 = strchr(val1, '@');
	if (val2 == NULL) {
		trace1(TR_netname2user, 1);
		return (0);
	}
	vallen = val2 - val1;
	if (vallen > (1024 - 1))
		vallen = 1024 - 1;
	(void) strncpy(val, val1, 1024);
	val[vallen] = 0;

	if (_rpc_get_default_domain(&domain)) {	/* change to rpc */
		trace1(TR_netname2user, 1);
		return (0);
	}

	if (strcmp(val2 + 1, domain)) {		/* wrong domain */
		trace1(TR_netname2user, 1);
		return (0);
	}

	/* XXX: uid_t have different sizes on different OS's. sigh! */
	if (sizeof (uid_t) == sizeof (short)) {
		if (sscanf(val, "%hd", &uid) != 1) {
			trace1(TR_netname2user, 1);
			return (0);
		}
	} else {
		if (sscanf(val, "%d", &uid) != 1) {
			trace1(TR_netname2user, 1);
			return (0);
		}
	}

	/* use initgroups method */
	pwd = getpwuid(uid);
	if (pwd == NULL) {
		trace1(TR_netname2user, 1);
		return (0);
	}
	*uidp = pwd->pw_uid;
	*gidp = pwd->pw_gid;
	*gidlenp = getgroups(pwd->pw_name, gidlist);
	trace1(TR_netname2user, 1);
	return (1);
}

/*
 * initgroups
 */
struct group *getgrent();

static
getgroups(uname, groups)
	char *uname;
	int groups[NGROUPS];
{
	gid_t ngroups = 0;
	register struct group *grp;
	register int i;
	register int j;
	int filter;

	trace1(TR_getgroups, 0);
	setgrent();
	while (grp = getgrent()) {
		for (i = 0; grp->gr_mem[i]; i++)
			if (!strcmp(grp->gr_mem[i], uname)) {
				if (ngroups == NGROUPS) {
#ifdef RPC_DEBUG
					fprintf(stderr,
		"initgroups: %s is in too many groups\n", uname);
#endif
					goto toomany;
				}
				/* filter out duplicate group entries */
				filter = 0;
				for (j = 0; j < ngroups; j++)
					if (groups[j] == grp->gr_gid) {
						filter++;
						break;
					}
				if (!filter)
					groups[ngroups++] = grp->gr_gid;
			}
	}
toomany:
	endgrent();
	trace1(TR_getgroups, 1);
	return (ngroups);
}

/*
 * Convert network-name to hostname
 */
netname2host(netname, hostname, hostlen)
	char netname[MAXNETNAMELEN + 1];
	char *hostname;
	int hostlen;
{
	char valbuf[1024];
	char *val;
	char *val2;
	int vallen;
	char *domain;

	trace1(TR_netname2host, 0);
	if (getnetid(netname, valbuf)) {
		val = valbuf;
		if ((*val == '0') && (val[1] == ':')) {
			(void) strncpy(hostname, val + 2, hostlen);
			trace1(TR_netname2host, 1);
			return (1);
		}
	}
	val = strchr(netname, '.');
	if ((val == NULL) || strncmp(netname, OPSYS, (val - netname))) {
		trace1(TR_netname2host, 1);
		return (0);
	}
	val++;
	val2 = strchr(val, '@');
	if (val2 == NULL) {
		trace1(TR_netname2host, 1);
		return (0);
	}
	vallen = val2 - val;
	if (vallen > (hostlen - 1))
		vallen = hostlen - 1;
	(void) strncpy(hostname, val, vallen);
	hostname[vallen] = 0;

	if (_rpc_get_default_domain(&domain)) {	/* change to rpc */
		trace1(TR_netname2host, 1);
		return (0);
	}

	if (strcmp(val2 + 1, domain)) {		/* wrong domain */
		trace1(TR_netname2host, 1);
		return (0);
	} else {
		trace1(TR_netname2host, 1);
		return (1);
	}
}

/*
 * reads the file /etc/netid looking for a + to optionally go to the
 * network information service.
 */
int
getnetid(key, ret)
	char *key, *ret;
{
	char buf[1024];	/* big enough */
	char *res;
	char *mkey;
	char *mval;
	FILE *fd;
	char *last;		/* storage for reentrant strtok_r */
#ifdef YP
	char *domain;
	int err;
	char *lookup;
	int len;
#endif

	trace1(TR_getnetid, 0);
	fd = _fopen(NETIDFILE, "r");
	if (fd == (FILE *) 0) {
#ifdef YP
		res = "+";
		goto getnetidyp;
#else
		trace1(TR_getnetid, 1);
		return (0);
#endif
	}
	for (;;) {
		if (fd == (FILE *) 0) {	/* getnetidyp brings us here */
			trace1(TR_getnetid, 1);
			return (0);
		}
		res = fgets(buf, 1024, fd);
		if (res == 0) {
			fclose(fd);
			trace1(TR_getnetid, 1);
			return (0);
		}
		if (res[0] == '#')
			continue;
		else if (res[0] == '+') {
#ifdef YP
	getnetidyp:
			err = yp_get_default_domain(&domain);
			if (err) {
				continue;
			}
			lookup = NULL;
			err = yp_match(domain, NETID, key,
				strlen(key), &lookup, &len);
			if (err) {
#ifdef RPC_DEBUG
				fprintf(stderr, "match failed error %d\n", err);
#endif
				continue;
			}
			lookup[len] = 0;
			strcpy(ret, lookup);
			free(lookup);
			fclose(fd);
			trace1(TR_getnetid, 1);
			return (2);
#else	/* YP */
#ifdef RPC_DEBUG
			fprintf(stderr,
"Bad record in %s '+' -- NIS not supported in this library copy\n",
				NETIDFILE);
#endif
			continue;
#endif	/* YP */
		} else {
			mkey = strtok_r(buf, "\t ", &last);
			if (mkey == NULL) {
				syslog(LOG_WARNING,
				  gettxt("uxnsl:83",
				   "netname2user/host: bad record in %s -- %s"),
				  NETIDFILE, buf);
				continue;
			}
			mval = strtok_r(NULL, " \t#\n", &last);
			if (mval == NULL) {
				syslog(LOG_WARNING,
				    gettxt("uxnsl:84",
		     "netname2user/host: bad record in %s - value problem: %s"),
				    NETIDFILE, buf);
				continue;
			}
			if (strcmp(mkey, key) == 0) {
				strcpy(ret, mval);
				fclose(fd);
				trace1(TR_getnetid, 1);
				return (1);

			}
		}
	}
}
