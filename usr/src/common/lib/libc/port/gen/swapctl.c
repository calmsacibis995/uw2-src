/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/swapctl.c	1.1"
#ifdef __STDC__
	#pragma weak swapctl = _swapctl
#endif
#include "synonyms.h"
#include	"sys/uadmin.h"

swapctl(cmd, arg)
int cmd;
int arg;
{
	return uadmin(A_SWAPCTL, cmd, arg);
}
