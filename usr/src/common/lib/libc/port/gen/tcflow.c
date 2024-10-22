/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/tcflow.c	1.2"

#ifdef __STDC__
	#pragma weak tcflow = _tcflow
#endif
#include "synonyms.h"
#include <sys/termios.h>
#include <unistd.h>

/* 
 *suspend transmission or reception of input or output
 */

/*
 * TCOOFF (0) -> suspend output 
 * TCOON  (1) -> restart suspend output
 * TCIOFF (2) -> suspend input 
 * TCION  (3) -> restart suspend input
 */

int tcflow(fildes,action)
int fildes;
int action;
{
	return(ioctl(fildes,TCXONC,action));

}
