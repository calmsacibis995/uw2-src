/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:sys/execle.c	1.6.3.1"
/*
 *	execle(file, arg1, arg2, ..., 0, envp)
 */
#ifdef __STDC__
	#pragma weak execle = _execle
#endif
#include "synonyms.h"
#include <stdarg.h>

extern int execve();

int
#ifdef __STDC__
execle(char *file, ...)
#else
execle(file, va_alist) char *file; va_dcl
#endif
{
	register  char  *p;
	va_list args, sargs;

#ifdef __STDC__
	va_start(args,file);
#else
	va_start(args);
#endif
	sargs = args;
	while ((p = va_arg(args, char *)) != 0) ;
	p = va_arg(args, char *);
	return(execve(file, sargs, p));
}
