/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/ctime.c	1.18"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <time.h>
#include "stdlock.h"
#include "timem.h"

static char ans_buf[26];
static struct tm ans_tm;

#ifdef _REENTRANT
static StdLock lock;
#endif

char *
#ifdef __STDC__
asctime(const struct tm *ptm)
#else
asctime(ptm)const struct tm *ptm;
#endif
{
	STDLOCK(&lock);
	asctime_r(ptm, &ans_buf[0]);
	STDUNLOCK(&lock);
	return ans_buf;
}

char *
#ifdef __STDC__
ctime(const time_t *tp)
#else
ctime(tp)const time_t *tp;
#endif
{
	STDLOCK(&lock);
	asctime_r(localtime_r(tp, &ans_tm), &ans_buf[0]);
	STDUNLOCK(&lock);
	return ans_buf;
}

struct tm *
#ifdef __STDC__
gmtime(const time_t *tp)
#else
gmtime(tp)const time_t *tp;
#endif
{
	STDLOCK(&lock);
	_time2tm(&ans_tm, (long)*tp);
	STDUNLOCK(&lock);
	return &ans_tm;
}

struct tm *
#ifdef __STDC__
localtime(const time_t *tp)
#else
localtime(tp)const time_t *tp;
#endif
{
	STDLOCK(&lock);
	localtime_r(tp, &ans_tm);
	STDUNLOCK(&lock);
	return &ans_tm;
}
