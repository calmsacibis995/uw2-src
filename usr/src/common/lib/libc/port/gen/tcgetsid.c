/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcgetsid.c	1.2"
#ifdef __STDC__
	#pragma weak tcgetsid = _tcgetsid
#endif
#include "synonyms.h"
#include <sys/termios.h>
#include <sys/types.h>
#include <errno.h>
#include <unistd.h>

pid_t
tcgetsid(fd)
int fd;
{
	pid_t ttysid,mysid;

	if ((ioctl(fd,TIOCGSID,&ttysid)) < 0
	  || (mysid = getsid(0)) < 0)
		return -1;
  	if (mysid != ttysid) {
		errno = ENOTTY;
		return -1;
	}
	return mysid;
}
