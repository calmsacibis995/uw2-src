/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/getpwnam.c	1.28"

#include "synonyms.h"
#include <pwd.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include "stdlock.h"

#ifdef __STDC__
	#pragma weak getpwnam = _getpwnam
	#pragma weak getpwuid = _getpwuid
	#pragma weak setpwent = _setpwent
	#pragma weak _abi_setpwent = _setpwent
	#pragma weak endpwent = _endpwent
	#pragma weak _abi_endpwent = _endpwent
	#pragma weak getpwent = _getpwent
	#pragma weak _abi_getpwent = _getpwent
	#pragma weak fgetpwent = _fgetpwent
	#pragma weak _abi_fgetpwent = _fgetpwent
#endif

#define VARYING_ALLOC
#include "statalloc.h"

struct pwplus /* each thread's data */
{
	struct passwd	pwd;
	FILE		*fp;
	size_t		len;
	char		buf[1];	/* holds a single line */
};

#define INITLEN	100	/* reasonable initial buf[] length */

static struct pwplus *
#ifdef __STDC__
space(size_t n)
#else
space(n)size_t n;
#endif
{
	struct passwd *pwd;
	struct pwplus *pwp;

	n += offsetof(struct pwplus, buf) - 1;
	n /= sizeof(struct passwd);
	n++;
	STATALLOC(pwd, struct passwd, n, return 0;);
	pwp = (struct pwplus *)pwd;
	n *= sizeof(struct passwd);
	n -= offsetof(struct pwplus, buf);
	if (n > pwp->len)
		pwp->len = n;
	return pwp;
}

static struct passwd *
#ifdef __STDC__
findpwd(struct pwplus *pwp, FILE *ofp, const char *name, uid_t uid)
#else
findpwd(pwp,ofp,name,uid)struct pwplus*pwp;FILE*ofp;const char*name;uid_t uid;
#endif
{
	size_t cur;
	FILE *fp;
	char *p;

	if ((fp = ofp) == 0)
		fp = pwp->fp;
	pwp->buf[pwp->len - 2] = '\n';
	for (;;)
	{
		cur = 0;
		for (;;) /* read entire line, growing if necessary */
		{
			if (fgets(&pwp->buf[cur], pwp->len - cur, fp) == 0)
				return 0;
			if (pwp->buf[pwp->len - 2] == '\n') /* line fit */
				break;
			cur = pwp->len - cur - 1;
			if ((pwp = space(pwp->len << 1)) == 0)
				return 0;
			pwp->buf[pwp->len - 2] = '\n';
		}
		/*
		* First check for special NIS entry handling.
		* They are always skipped unless a separate FILE *
		* is being scanned (i.e., from fgetpwent).
		*/
		if (pwp->buf[0] == '-') /* NIS "deny" entry */
		{
			if (ofp == 0)
				continue;
		}
		else if (pwp->buf[0] == '+') /* basic NIS entry */
		{
			static const struct passwd zero = {0};

			if (ofp == 0)
				continue;
			pwp->pwd = zero; /* default values */
		}
		/*
		* Attempt to match this entry against name/uid.
		* If searching for a particular name/uid, skip any
		* that don't match.  If there aren't enough :s,
		* skip the entry except for the special case for
		* NIS "+" entries.  Note that empty numeric fields
		* are always taken to be zero.
		*/
		if ((p = strchr(pwp->buf, ':')) == 0)
			goto missing;
		*p++ = '\0';
		if (name != 0 && strcmp(name, pwp->buf) != 0)
			continue;
		pwp->pwd.pw_name = pwp->buf;
		pwp->pwd.pw_passwd = p;
		if ((p = strchr(p, ':')) == 0)
			goto missing;
		*p++ = '\0';
		pwp->pwd.pw_uid = atoi(p);
		if (uid != -1 && pwp->pwd.pw_uid != uid)
			continue;
		if ((p = strchr(p, ':')) == 0)
			goto missing;
		pwp->pwd.pw_gid = atoi(++p);
		if ((p = strchr(p, ':')) == 0)
			goto missing;
		pwp->pwd.pw_comment = ++p;
		pwp->pwd.pw_gecos = p;
		if ((p = strchr(p, ':')) == 0)
			goto missing;
		*p++ = '\0';
		pwp->pwd.pw_dir = p;
		if ((p = strchr(p, ':')) == 0)
			goto missing;
		*p++ = '\0';
		pwp->pwd.pw_shell = p;
		if ((p = strchr(p, '\n')) == 0)
			goto missing;
		*p = '\0';
		pwp->pwd.pw_age = p;
		if ((p = strchr(pwp->pwd.pw_passwd, ',')) != 0)
		{
			*p++ = '\0';
			pwp->pwd.pw_age = p;
		}
		return &pwp->pwd;
	missing:;
		if (ofp != 0 && pwp->buf[0] == '+')
			return &pwp->pwd;
	}
}

#define CMD_REWIND	0x1	/* rewind already open file to start */
#define CMD_SEARCH	0x2	/* search for {name,uid} */

static struct passwd *
#ifdef __STDC__
findone(const char *name, uid_t uid, int cmd)
#else
findone(name, uid, cmd)const char *name; uid_t uid; int cmd;
#endif
{
	static const char etcpasswd[] = "/etc/passwd";
	static const char reading[] = "r";
	struct passwd *pwd;
	struct pwplus *pwp;

	if ((pwp = space(INITLEN)) == 0)
		return 0;
	if (pwp->fp == 0)
	{
		if ((cmd & CMD_SEARCH) == 0)
			return 0;
		if ((pwp->fp = fopen(etcpasswd, reading)) == 0)
			return 0;
	}
	else if (cmd & CMD_REWIND)
	{
		rewind(pwp->fp);
		cmd &= ~CMD_REWIND; /* don't automatically fclose() */
	}
	pwd = 0;
	if (cmd & CMD_SEARCH)
		pwd = findpwd(pwp, (FILE *)0, name, uid);
	if (cmd & CMD_REWIND || pwd == 0)
	{
		fclose(pwp->fp);
		pwp->fp = 0;
	}
	return pwd;
}

#ifdef DSHLIB

#include <dlfcn.h>

enum { PWNAM, PWUID, SETPW, ENDPW, GETPW, NPW };

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
	static const char fcnm[NPW][13] = /* in the enum's order */
	{
		"nis_getpwnam",	"nis_getpwuid",
		"nis_setpwent",	"nis_endpwent",
		"nis_getpwent",
	};
	static int (*fptr[NPW])();
	static void *handle;
	int i;

	STDLOCK(&lock);
	if (handle == 0) /* first time */
	{
		if ((handle = dlopen(path, RTLD_NOW)) == 0)
			handle = (void *)&handle; /* no other attempts */
		else
		{
			for (i = 0; i < NPW; i++)
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

#define NS_PTR(fcn, args) NS_TYPE(struct passwd *, fcn, args)

#else /*!DSHLIB*/

#define NS_VOID(fcn)
#define NS_PTR(fcn, args)

#endif /*DSHLIB*/

struct passwd *
#ifdef __STDC__
getpwnam(const char *name)
#else
getpwnam(name)
char *name;
#endif
{
	if (name == 0)
		return 0;
	NS_PTR(PWNAM, (name));
	return findone(name, (uid_t)-1, CMD_REWIND | CMD_SEARCH);
}

struct passwd *
#ifdef __STDC__
getpwuid(uid_t uid)
#else
getpwuid(uid)
uid_t uid;
#endif
{
	if (uid < 0)
		return 0;
	NS_PTR(PWUID, (uid));
	return findone((char *)0, uid, CMD_REWIND | CMD_SEARCH);
}

void
#ifdef __STDC__
setpwent(void)
#else
setpwent()
#endif
{
	struct pwplus *pwp;

	NS_VOID(SETPW);
	(void)findone((char *)0, (uid_t)-1, CMD_REWIND);
}

void
#ifdef __STDC__
endpwent(void)
#else
endpwent()
#endif
{
	struct pwplus *pwp;

	NS_VOID(ENDPW);
	(void)findone((char *)0, (uid_t)-1, 0);
}

struct passwd *
#ifdef __STDC__
getpwent(void)
#else
getpwent()
#endif
{
	NS_PTR(GETPW, ());
	return findone((char *)0, (uid_t)-1, CMD_SEARCH);
}

struct passwd *
#ifdef __STDC__
fgetpwent(FILE *fp) /* does not use NS library */
#else
fgetpwent(fp)FILE *fp;
#endif
{
	struct pwplus *pwp;

	if ((pwp = space(INITLEN)) == 0)
		return 0;
	return findpwd(pwp, fp, (char *)0, (uid_t)-1);
}
