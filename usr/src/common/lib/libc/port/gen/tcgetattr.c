/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcgetattr.c	1.2"

#ifdef __STDC__
	#pragma weak tcgetattr = _tcgetattr
#endif
#include "synonyms.h"
#include <sys/termios.h>
#include <unistd.h>

/* 
 * get parameters associated with fildes and store them in termios
 */

int tcgetattr (fildes, termios_p)
int fildes;
struct termios *termios_p;
{
	return(ioctl(fildes,TCGETS,termios_p));
}
