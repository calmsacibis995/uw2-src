/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:ssignal.c	1.10"
/*LINTLIBRARY*/
/*
 *	ssignal, gsignal: software signals
 */
#ifdef __STDC__
	#pragma weak gsignal = _gsignal
	#pragma weak ssignal = _ssignal
#endif
#include "synonyms.h"
#include <signal.h>

/* Highest allowable user signal number */
#define MAXSIGNUM 17

/* Lowest allowable signal number (lowest user number is always 1) */
#define MINSIG (-4)

/* Table of signal values */
static int (*sigs[MAXSIGNUM-MINSIG+1])();

int
(*ssignal(sig, fn))()
register int sig, (*fn)();
{
	register int (*savefn)();

	if(sig >= MINSIG && sig <= MAXSIGNUM) {
		savefn = sigs[sig-MINSIG];
		sigs[sig-MINSIG] = fn;
	} else
		savefn = (int(*)())SIG_DFL;

	return(savefn);
}

int
gsignal(sig)
register int sig;
{
	register int (*sigfn)();

	if(sig < MINSIG || sig > MAXSIGNUM ||
				(sigfn = sigs[sig-MINSIG]) == (int(*)())SIG_DFL)
		return(0);
	else if(sigfn == (int(*)())SIG_IGN)
		return(1);
	else {
		sigs[sig-MINSIG] = (int(*)())SIG_DFL;
		return((*sigfn)(sig));
	}
}
