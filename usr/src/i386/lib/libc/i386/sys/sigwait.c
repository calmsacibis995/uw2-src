/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:sys/sigwait.c	1.1"

#ifdef __STDC__
	#pragma weak sigwait = _sigwait
#endif

#include "synonyms.h"
#include <sys/types.h>
#include <signal.h>
#include <siginfo.h>
#include <time.h>

int
sigwait(sigset_t *set)
{
	return __sigwait(set, (siginfo_t *)0, (struct timespec *)0);
}
