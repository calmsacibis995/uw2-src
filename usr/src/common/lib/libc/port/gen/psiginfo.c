/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/psiginfo.c	1.9"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <limits.h>
#include <signal.h>
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "siginfom.h"
#include "_locale.h"

#ifdef __STDC__
	#pragma weak psiginfo = _psiginfo
#endif

void
#ifdef __STDC__
psiginfo(register const siginfo_t *sip, const char *s)
#else
psiginfo(sip, s)register const siginfo_t *sip; const char *s;
#endif
{
	static const char frompid[] = " (from process %ld)";
	static const char paraddr[] = ")[%lx] ";
	static const char uxlibc[] = "uxlibc";
	static const char nlcolsp[] = "\n: ";
	char pidbuf[17 + sizeof(long) * CHAR_BIT / 3];
	char addrbuf[4 + sizeof(long) * CHAR_BIT / 4];
	struct iovec vec[8];
	register struct iovec *vp = &vec[0];
	register const struct siginfolist *slp;
	int i;

	if (s != 0 && *s != '\0')
	{
		vp[0].iov_base = (void *)s;
		vp[0].iov_len = strlen(s);
		s = __gtxt(uxlibc, 3, &nlcolsp[1]);
		vp[1].iov_base = (void *)s;
		vp[1].iov_len = strlen(s);
		vp += 2;
	}
	s = __gtxt(uxlibc, sip->si_signo + 4, _sys_siglist[sip->si_signo]);
	vp->iov_base = (void *)s;
	vp->iov_len = strlen(s);
	vp++;
	if (sip->si_code == 0)
	{
		s = __gtxt(uxlibc, 71, frompid);
		i = snprintf(pidbuf, sizeof(pidbuf), s, (long)sip->si_pid);
		if (i < 0)
			i = sizeof(pidbuf) - 1;
		vp->iov_base = (void *)pidbuf;
		vp->iov_len = i;
		vp++;
	}
	else if ((slp = &_sys_siginfolist[sip->si_signo - 1]) != 0
		&& sip->si_code > 0 && sip->si_code <= slp->nsiginfo)
	{
		vp->iov_base = (void *)frompid;
		vp->iov_len = 2;
		vp++;
		switch (sip->si_signo)
		{
		case SIGSEGV:
		case SIGBUS:
		case SIGILL:
		case SIGFPE:
			i = snprintf(addrbuf, sizeof(addrbuf), &paraddr[1],
				(unsigned long)sip->si_addr);
			if (i < 0)
				i = sizeof(addrbuf) - 1;
			vp->iov_base = (void *)addrbuf;
			vp->iov_len = i;
			vp++;
			break;
		}
		s = __gtxt(uxlibc, _siginfo_msg_offset[sip->si_signo - 1]
			+ sip->si_code - 1, slp->vsiginfo[sip->si_code - 1]);
		vp[0].iov_base = (void *)s;
		vp[0].iov_len = strlen(s);
		vp[1].iov_base = (void *)paraddr;
		vp[1].iov_len = 1;
		vp += 2;
	}
	vp->iov_base = (void *)nlcolsp;
	vp->iov_len = 1;
	vp++;
	(void)writev(2, &vec[0], vp - &vec[0]);
}
