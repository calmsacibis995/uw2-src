/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcdrain.c	1.2"

#ifdef __STDC__
	#pragma weak tcdrain = _tcdrain
#endif
#include "synonyms.h"
#include <sys/termios.h>
#include <unistd.h>

/*
 * wait until all output on the filedes is drained
 */

int tcdrain(fildes)
int fildes;
{
	return(ioctl(fildes,TCSBRK,1));
}
