/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/clock.c	1.6.3.1"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <sys/types.h>
#include <sys/times.h>
#include <sys/param.h>	/* for HZ (clock frequency in Hz) */
#include "stdlock.h"
#define TIMES(B)	(B.tms_utime+B.tms_stime+B.tms_cutime+B.tms_cstime)

extern int gethz();
static long first = 0L;

#ifdef _REENTRANT
static StdLock	__clock_lock;
#endif	/*  _REENTRANT defined  */

long
clock()
{
	struct tms buffer;
	static int Hz = 0;

#ifdef _REENTRANT
	long ret;
#endif	/*   _REENTRANT defined  */


	STDLOCK(&__clock_lock);

	if (!Hz && (Hz = gethz()) == 0)
		Hz = HZ;

	if (times(&buffer) != -1L && first == 0L)
		first = TIMES(buffer);
#ifdef _REENTRANT
	ret = first;
	STDUNLOCK(&__clock_lock);
	return ((TIMES(buffer) - ret) * (1000000L/Hz));
#else
	return ((TIMES(buffer) - first) * (1000000L/Hz));
#endif	/*  _REENTRANT defined  */
}
