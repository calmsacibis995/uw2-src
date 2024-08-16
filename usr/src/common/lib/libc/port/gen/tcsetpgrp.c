/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcsetpgrp.c	1.2"
#ifdef __STDC__
	#pragma weak tcsetpgrp = _tcsetpgrp
#endif
#include "synonyms.h"
#include <sys/termios.h>
#include <sys/types.h>
#include <unistd.h>

tcsetpgrp(fd,pgrp)
int fd;
pid_t pgrp;
{
	if (tcgetsid(fd) < 0)
		return -1;
	return ioctl(fd,TIOCSPGRP,&pgrp);
}
