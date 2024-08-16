/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getgrnam.c	1.24"

#include "synonyms.h"
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <grp.h>
#include "stdlock.h"

#ifdef __STDC__
	#pragma weak getgrnam = _getgrnam
	#pragma weak getgrgid = _getgrgid
	#pragma weak initgroups = _initgroups
	#pragma weak setgrent = _setgrent
	#pragma weak _abi_setgrent = _setgrent
	#pragma weak endgrent = _endgrent
	#pragma weak _abi_endgrent = _endgrent
	#pragma weak getgrent = _getgrent
	#pragma weak _abi_getgrent = _getgrent
	#pragma weak fgetgrent = _fgetgrent
	#pragma weak _abi_fgetgrent = _fgetgrent
#endif

#define VARYING_ALLOC
#include "statalloc.h"

struct grplus /* each thread's data */
{
	struct group	g;
	FILE		*fp;
	size_t		len;
	union
	{
		char	*list[1];	/* holds the array of pointers */
		char	line[1];	/* holds a single line */
	} u;
};

#define INITLEN	(128 * sizeof(char *))	/* reasonable initial length */

static struct grplus *
#ifdef __STDC__
space(size_t n)
#else
space(n)size_t n;
#endif
{
	struct group *grp;
	struct grplus *gp;

	n += offsetof(struct grplus, u) - 1;
	n /= sizeof(struct group);
	n++;
	STATALLOC(grp, struct group, n, return 0;);
	gp = (struct grplus *)grp;
	n *= sizeof(struct group);
	n -= offsetof(struct grplus, u);
	if (n > gp->len)
		gp->len = n;
	return gp;
}

static struct group *
#ifdef __STDC__
findgrp(struct grplus *gp, FILE *ofp, const char *name, gid_t gid)
#else
findgrp(gp,ofp,name,gid)struct grplus*gp;FILE*ofp;const char*name;gid_t gid;
#endif
{
	size_t cur, start, after;
	char *p, *q;
	FILE *fp;

	if ((fp = ofp) == 0)
		fp = gp->fp;
	gp->u.line[gp->len - 2] = '\n';
	for (;;)
	{
		cur = 0;
		for (;;) /* read entire line, growing if necessary */
		{
			if (fgets(&gp->u.line[cur], gp->len - cur, fp) == 0)
				return 0;
			if (gp->u.line[gp->len - 2] == '\n') /* line fit */
				break;
			cur = gp->len - cur - 1;
			if ((gp = space(gp->len << 1)) == 0)
				return 0;
			gp->u.line[gp->len - 2] = '\n';
		}
		/*
		* First check for special NIS entry handling.
		* They are always skipped unless a separate FILE *
		* is being scanned (i.e., from fgetpwent).
		*/
		if (gp->u.line[0] == '-') /* NIS "deny" entry */
		{
			if (ofp == 0)
				continue;
		}
		else if (gp->u.line[0] == '+') /* basic NIS entry */
		{
			static const struct group zero = {0};

			if (ofp == 0)
				continue;
			gp->g = zero; /* default values */
		}
		/*
		* Attempt to match this entry against name/gid.
		* If searching for a particular name/gid, skip any
		* that don't match.  If there aren't enough :s,
		* skip the entry except for the special case for
		* NIS "+" entries.  Note that an empty gid field
		* is always taken to be zero.
		*/
		if ((p = strchr(gp->u.line, ':')) == 0)
			goto missing;
		*p++ = '\0';
		if (name != 0 && strcmp(name, gp->u.line) != 0)
			continue;
		gp->g.gr_name = gp->u.line;
		gp->g.gr_passwd = p;
		if ((p = strchr(p, ':')) == 0)
			goto missing;
		*p++ = '\0';
		gp->g.gr_gid = atoi(p);
		if (gid != -1 && gp->g.gr_gid != gid)
			continue;
		if ((p = strchr(p, ':')) == 0)
			goto missing;
		if ((q = strchr(++p, '\n')) == 0)
			goto missing;
		if (q != p)
			*q++ = ',';	/* terminate list with comma */
		*q = '\0';
		break;
	missing:;
		if (ofp != 0 && gp->u.line[0] == '+')
			return &gp->g;
	}
	/*
	* Have the entry we're looking for.
	* Build the null-terminated list of strings.
	*/
	after = gp->len / sizeof(char *);
	start = after - (gp->len - (1 + q - gp->u.line)) / sizeof(char *);
	cur = start;
	for (;;)
	{
		if (cur >= after) /* need a longer list */
		{
			struct grplus *gp2;

			if ((gp2 = space(gp->len << 1)) == 0)
				return 0;
			/*
			* Strictly speaking, adjusting the internal
			* pointers after-the-fact is not portable
			* since the old address is "indeterminate",
			* but it's better than keeping the list as
			* indeces and then changing to pointers once
			* the list is complete--just in case the
			* list needs to grow.
			*/
			if (gp2 != gp)
			{
				ptrdiff_t off;
				size_t n;

				off = (char *)gp2 - (char *)gp;
				p += off;
				gp = gp2;
				gp->g.gr_name += off;
				gp->g.gr_passwd += off;
				for (n = start; n < cur; n++)
					gp->u.list[n] += off;
			}
			after = gp->len / sizeof(char *);
		}
		if ((q = strchr(p, ',')) == 0)
			break;
		*q++ = '\0';
		gp->u.list[cur++] = p;
		p = q;
	}
	gp->u.list[cur] = 0;
	gp->g.gr_mem = &gp->u.list[start];
	return &gp->g;
}

#define CMD_REWIND	0x1	/* rewind already open file to start */
#define CMD_SEARCH	0x2	/* search for {name,gid} */

static const char etcgroup[] = "/etc/group";
static const char reading[] = "r";

static struct group *
#ifdef __STDC__
findone(const char *name, gid_t gid, int cmd)
#else
findone(name, gid, cmd)const char *name; gid_t gid; int cmd;
#endif
{
	struct group *grp;
	struct grplus *gp;

	if ((gp = space(INITLEN)) == 0)
		return 0;
	if (gp->fp == 0)
	{
		if ((cmd & CMD_SEARCH) == 0)
			return 0;
		if ((gp->fp = fopen(etcgroup, reading)) == 0)
			return 0;
	}
	else if (cmd & CMD_REWIND)
	{
		rewind(gp->fp);
		cmd &= ~CMD_REWIND; /* don't automatically fclose() */
	}
	grp = 0;
	if (cmd & CMD_SEARCH)
		grp = findgrp(gp, (FILE *)0, name, gid);
	if (cmd & CMD_REWIND || grp == 0)
	{
		fclose(gp->fp);
		gp->fp = 0;
	}
	return grp;
}

#ifdef DSHLIB

#include <dlfcn.h>

enum { GRNAM, GRGID, SETGR, ENDGR, GETGR, INITGR, NGR };

static int
#ifdef __STDC__
(*get_ns(int fcn))(void)
#else
(*get_ns(fcn))()int fcn;
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static const char path[] = "/usr/lib/ns.so.1";
	static const char fcnm[NGR][15] = /* in the enum's order */
	{
		"nis_getgrnam",	"nis_getgrgid",
		"nis_setgrent",	"nis_endgrent",
		"nis_getgrent",	"nis_initgroups",
	};
	static int (*fptr[NGR])();
	static void *handle;
	int i;

	STDLOCK(&lock);
	if (handle == 0) /* first time */
	{
		if ((handle = dlopen(path, RTLD_NOW)) == 0)
			handle = (void *)&handle; /* no other attempts */
		else
		{
			for (i = 0; i < NGR; i++)
				fptr[i] = (int (*)())dlsym(handle, fcnm[i]);
		}
	}
	STDUNLOCK(&lock);
	return fptr[fcn];
}

#define NS_VOID(fcn) \
	{ void (*fptr)(); \
	if ((fptr = (void (*)())get_ns(fcn)) != 0) { (*fptr)(); return; } \
	}

#define NS_TYPE(type, fcn, args) \
	{ type (*fptr)(); \
	if ((fptr = (type (*)())get_ns(fcn)) != 0) return (*fptr)args; \
	}

#define NS_PTR(fcn, args) NS_TYPE(struct group *, fcn, args)
#define NS_INT(fcn, args) NS_TYPE(int, fcn, args)

#else /*!DSHLIB*/

#define NS_VOID(fcn)
#define NS_PTR(fcn, args)
#define NS_INT(fcn, args)

#endif /*DSHLIB*/

struct group *
#ifdef __STDC__
getgrnam(const char *name)
#else
getgrnam(name)const char *name;
#endif
{
	if (name == 0)
		return 0;
	NS_PTR(GRNAM, (name));
	return findone(name, (gid_t)-1, CMD_REWIND | CMD_SEARCH);
}

struct group *
#ifdef __STDC__
getgrgid(gid_t gid)
#else
getgrgid(gid)gid_t gid;
#endif
{
	if (gid < 0)
		return 0;
	NS_PTR(GRGID, (gid));
	return findone((char *)0, gid, CMD_REWIND | CMD_SEARCH);
}

void
#ifdef __STDC__
setgrent(void)
#else
setgrent()
#endif
{
	struct grplus *gp;

	NS_VOID(SETGR);
	(void)findone((char *)0, (gid_t)-1, CMD_REWIND);
}

void
#ifdef __STDC__
endgrent(void)
#else
endgrent()
#endif
{
	struct grplus *gp;

	NS_VOID(ENDGR);
	(void)findone((char *)0, (gid_t)-1, 0);
}

struct group *
#ifdef __STDC__
getgrent(void)
#else
getgrent()
#endif
{
	struct grplus *gp;

	NS_PTR(GETGR, ());
	return findone((char *)0, (gid_t)-1, CMD_SEARCH);
}

int
#ifdef __STDC__
initgroups(const char *name, gid_t gid)
#else
initgroups(name, gid)const char *name; gid_t gid;
#endif
{
	gid_t smlist[100];	/* usually big enough */
	struct group *grp;
	struct grplus *gp;
	int n, ans, cls;
	gid_t *list;
	char **p;
	long max;

	NS_INT(INITGR, (name, gid));
	if ((gp = space(INITLEN)) == 0 || (max = sysconf(_SC_NGROUPS_MAX)) <= 0)
		return -1;
	if (max <= sizeof(smlist) / sizeof(gid_t))
		list = &smlist[0];
	else if ((list = (gid_t *)malloc(max * sizeof(gid_t))) == 0)
		return -1;
	if (gp->fp == 0)
	{
		if ((gp->fp = fopen(etcgroup, reading)) == 0)
		{
			ans = -1;
			goto out;
		}
		cls = 1;
	}
	else
	{
		rewind(gp->fp);
		cls = 0;
	}
	n = 0;
	if (gid >= 0)
		list[n++] = gid;
	while ((grp = findgrp(gp, (FILE *)0, (char *)0, (gid_t)-1)) != 0)
	{
		if (grp->gr_gid == gid)
			continue;
		if (n >= max)
			break;
		for (p = grp->gr_mem; *p != 0; p++)
		{
			if (strcmp(name, *p) == 0)
			{
				list[n++] = grp->gr_gid;
				break;
			}
		}
	}
	if (cls)
	{
		fclose(gp->fp);
		gp->fp = 0;
	}
	ans = setgroups(n, list);
out:;
	if (list != &smlist[0])
		free((void *)list);
	return ans;
}

struct group *
#ifdef __STDC__
fgetgrent(FILE *fp) /* does not use NS library */
#else
fgetgrent(fp)FILE *fp;
#endif
{
	struct grplus *gp;

	if ((gp = space(INITLEN)) == 0)
		return 0;
	return findgrp(gp, fp, (char *)0, (gid_t)-1);
}
