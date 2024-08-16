/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/perror.c	1.18"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/uio.h>
#include "_locale.h"

void
#ifdef __STDC__
perror(const char *s)
#else
perror(s)const char *s;
#endif
{
	struct iovec vec[4];
	register struct iovec *vp = &vec[0];

	if (s != 0 && *s != '\0')
	{
		vp[0].iov_base = (void *)s;
		vp[0].iov_len = strlen(s);
		s = __gtxt(_str_uxsyserr, 2, _str_colonsp);
		vp[1].iov_base = (void *)s;
		vp[1].iov_len = strlen(s);
		vp += 2;
	}
	s = strerror(errno);
	vp[0].iov_base = (void *)s;
	vp[0].iov_len = strlen(s);
	vp[1].iov_base = (void *)_str_nlcolsp;
	vp[1].iov_len = 1;
	vp += 2;
	(void)writev(2, &vec[0], vp - &vec[0]);
}
