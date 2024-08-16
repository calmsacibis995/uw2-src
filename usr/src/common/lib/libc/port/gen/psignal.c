/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/psignal.c	1.7"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <signal.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "siginfom.h"
#include "_locale.h"

#ifdef __STDC__
	#pragma weak psignal = _psignal
#endif

void
#ifdef __STDC__
psignal(int sig, const char *s)
#else
psignal(sig, s)int sig; const char *s;
#endif
{
	static const char uxlibc[] = "uxlibc";
	static const char nlcolsp[] = "\n: ";
	struct iovec vec[4];
	register struct iovec *vp = &vec[0];

	if (sig < 0 || sig >= _sys_nsig)
		sig = 0;
	if (s != 0 && *s != '\0')
	{
		vp[0].iov_base = (void *)s;
		vp[0].iov_len = strlen(s);
		s = __gtxt(uxlibc, 3, &nlcolsp[1]);
		vp[1].iov_base = (void *)s;
		vp[1].iov_len = strlen(s);
		vp += 2;
	}
	s = __gtxt(uxlibc, sig + 4, _sys_siglist[sig]);
	vp[0].iov_base = (void *)s;
	vp[0].iov_len = strlen(s);
	vp[1].iov_base = (void *)nlcolsp;
	vp[1].iov_len = 1;
	vp += 2;
	(void)writev(2, &vec[0], vp - &vec[0]);
}
