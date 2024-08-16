/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_ipfmt.c	1.1.1.2"

#include "synonyms.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#ifndef NO_LFMT
#include <errno.h>
#include <fcntl.h>
#include <unistd.h>
#include <stropts.h>
#include <sys/strlog.h>
#include <sys/syslog.h>
#endif
#include "pfmtm.h"
#include "_locale.h"
#include "stdlock.h"

#ifndef NO_LFMT
static const char conslog[] = "/dev/conslog";
#endif
static const char msgoff[] = "MSGOFF";
static const char sevlist[] =
	"SEV=%d\0\0TO FIX\0\0ERROR\0\0\0HALT\0\0\0\0WARNING\0INFO\0\0\0";

#define SEVLEN	8			/* bytes/string, with the \0s */
#define SEVBASE	(SEVLEN + SEVLEN)	/* where 0 severity starts */
#define SEVACT	(-1)			/* index of "to fix" severity */
#define SEVUNK	(-2)			/* index of unknown severity */

#define TAG	0x4	/* don't print the "UX:"-ish part of the label */
#define LAB	0x2	/* don't print the label */
#define SEV	0x1	/* don't print the severity */

int
#ifdef __STDC__
_ipfmt(FILE *fp, long flags, const char *fmt, va_list ap)
#else
_ipfmt(fp, flags, fmt, ap)FILE *fp; long flags; const char *fmt; va_list ap;
#endif
{
#ifndef NO_LFMT
	union { long f; char m[256]; } log;
	struct strbuf dat, ctl;
	struct log_ctl hdr;
	char *logptr;
	int logroom;
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static int logfd = -1;
#endif /*NO_LFMT*/
	int n, sev, total, off;
	const char *p, *s, *sep;

	off = 0;
	total = 0;
	if (flags & MM_ACTION)
		sev = SEVACT;
	else
		sev = flags & 0xff;
#ifndef NO_LFMT
	if (fp == 0)
	{

		STDLOCK(&lock);
		if (logfd < 0 && ((logfd = open(conslog, O_WRONLY)) < 0
			|| fcntl(logfd, F_SETFD, 1) < 0))
		{
			return -2;
		}
		STDUNLOCK(&lock);
		log.f = flags;
		logroom = sizeof(log.m) - sizeof(long);
		logptr = &log.m[sizeof(long)];
		if (flags & MM_NOSTD)
		{
			logroom -= 2;
			logptr += 2;
			log.m[sizeof(long) + 0] = '\0';
			log.m[sizeof(long) + 1] = '\0';
			goto dofmt;
		}
		goto dolab;
	}
#endif /*NO_LFMT*/
	if (flags & MM_NOSTD)
		goto dofmt;
	if ((p = getenv(msgoff)) != 0)
	{
		for (;;)
		{
			switch (*p++)
			{
			default:
				continue;
			case '\0':
				break;
			case 't':
			case 'T':
				off |= TAG;
				continue;
			case 'l':
			case 'L':
				off |= LAB;
				continue;
			case 's':
			case 'S':
				off |= SEV;
				continue;
			reject:;
				if (fmt != 0 && strchr(fmt, '\n') != 0)
					return 0;
				continue; /* can't delete entire line */
			case 'f':
			case 'F':
				if (flags & MM_ACTION)
					goto reject;
				continue;
			case 'h':
			case 'H':
				if (sev == MM_HALT)
					goto reject;
				continue;
			case 'e':
			case 'E':
				if (sev == MM_ERROR)
					goto reject;
				continue;
			case 'w':
			case 'W':
				if (sev == MM_WARNING)
					goto reject;
				continue;
			case 'i':
			case 'I':
				if (sev == MM_INFO)
					goto reject;
				continue;
			case '0':
			case '1':
			case '2':
			case '3':
			case '4':
			case '5':
			case '6':
			case '7':
			case '8':
			case '9':
				n = p[-1] - '0';
				while (isdigit(*p))
				{
					n *= 10;
					n += *p++ - '0';
				}
				if (sev == n)
					goto reject;
				continue;
			}
			break;
		}
	}
	if ((off & (LAB | SEV)) == (LAB | SEV))
	{
#ifdef _REENTRANT
		flags |= MM_NOSTD;	/* so funlockfile() not called */
#endif
		goto dofmt;
	}
#ifdef _REENTRANT
	flockfile(fp);
#endif
	sep = __gtxt(_str_uxlibc, 3, _str_colonsp);
dolab:;
	/*
	* This code mostly ignores the chance that _pfmt_label will
	* be changed by a concurrent setlabel() call.  The lifetime
	* of its value is only to the memcpy() or the fprintf().
	*/
	if ((off & LAB) == 0 && (p = _pfmt_label) != 0)
	{
#ifndef NO_LFMT
		if (fp == 0)
		{
			n = strlen(p) + 1;
			if ((logroom -= n) < 0)
				goto toobig;
			(void)memcpy((void *)logptr, (void *)p, (size_t)n);
			logptr += n;
		}
		else
#endif /*NO_LFMT*/
		{
			if (off & TAG && (s = strchr(p, ':')) != 0)
			{
				while (isspace(*++s))
					;
				if (*s == '\0')
					goto dosev;
				p = s;
			}
			if ((n = fprintf(fp, "%s%s", p, sep)) < 0)
				goto err;
			total += n;
		}
	}
#ifndef NO_LFMT
	else if (fp == 0)
	{
		logroom--;
		*logptr++ = '\0';
	}
#endif /*NO_LFMT*/
dosev:;
	if ((off & SEV) == 0)
	{
		if ((n = sev) > MM_INFO)
		{
			register struct sev_tab *ps = _pfmt_sevtab;

			for (n = _pfmt_nsev; --n >= 0; ps++)
			{
				if (ps->level == sev)
				{
					p = ps->string;
					goto gotsev;
				}
			}
			n = SEVUNK;
		}
		p = __gtxt(_str_uxlibc, n + 74, &sevlist[SEVBASE + n * SEVLEN]);
	gotsev:;
#ifndef NO_LFMT
		if (fp == 0)
		{
			if ((n = snprintf(logptr, (size_t)logroom, p, sev)) < 0)
				goto toobig;
			if ((logroom -= ++n) < 0)
				goto toobig;
			logptr += n;
		}
		else
#endif /*NO_LFMT*/
		{
			if ((n = fprintf(fp, p, sev)) < 0)
				goto err;
			total += n;
			if ((n = fputs(sep, fp)) < 0)
				goto err;
			total += n;
		}
	}
dofmt:;
	if (fmt == 0)	/* allow partial line output; rest elsewhere */
		fmt = "";
	else if ((flags & MM_NOGET) == 0)
		fmt = gettxt(fmt, (char *)0);
#ifndef NO_LFMT
	if (fp == 0)
	{
		if ((n = vsnprintf(logptr, (size_t)logroom, fmt, ap)) < 0)
			goto toobig;
		logptr += n + 1;
		hdr.pri = LOG_LFMT | LOG_ERR;
		hdr.flags = SL_CONSOLE;
		hdr.level = 0;
		ctl.buf = (void *)&hdr;
		ctl.maxlen = ctl.len = sizeof(hdr);
		dat.buf = (void *)&log.m[0];
		dat.maxlen = dat.len = logptr - &log.m[0];
		if (putmsg(logfd, &ctl, &dat, 0) < 0)
			return -2;
		return dat.len;
	toobig:;
		errno = ERANGE;
		return -2;
	}
#endif /*NO_LFMT*/
	if ((n = vfprintf(fp, fmt, ap)) < 0)
	{
	err:;
		total = -1;
	}
	else
		total += n;
#ifdef _REENTRANT
	if ((flags & MM_NOSTD) == 0)
		funlockfile(fp);
#endif
	return total;
}
