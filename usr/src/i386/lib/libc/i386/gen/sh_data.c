/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:gen/sh_data.c	1.3"

#ifdef DSHLIB
int	_environ;
#else
#ifdef __STDC__
	#pragma weak environ = _environ
int	_environ = 0;
#else
int	environ = 0;
#endif
#endif
