/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:sys/sigaction.c	1.7"

#ifdef __STDC__ 
	#pragma weak sigaction = _sigaction
#endif
#include "synonyms.h"
#include <signal.h>
#include <errno.h>
#include <siginfo.h>
#include <sys/user.h>
#include <ucontext.h>


void
_sigacthandler(int sig, siginfo_t *sip, ucontext_t *ucp, void (*handler)())
{
	(*handler)(sig, sip, ucp);
	setcontext(ucp);
	/*
	 * We have no context to return to.
	 */
	_exit(-1);
}

int
sigaction(int sig, const struct sigaction *nactp, struct sigaction *oactp)
{
	struct sigaction tact;

	if (nactp != 0) {
		tact = *nactp;			/* structure assignment */
		tact.sa_flags |= SA_NSIGACT;
		nactp = &tact;
	}

	return __sigaction(sig, nactp, oactp, _sigacthandler);
}
