/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/closefiles.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)closefiles.c	1.4 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	closeallfiles - close all open files

    SYNOPSIS
	void closeallfiles(int firstfd)

    DESCRIPTION
	closeallfiles() closes all open file descriptors
	starting with "firstfd".
*/

/* _NFILE used to be defined in stdio.h */
#ifdef SVR4
# undef _NFILE
# define _NFILE	ulimit(UL_GDESLIM)
#else
# ifdef USE_GETDTABLESIZE
#  undef _NFILE
#  define _NFILE getdtablesize()
# endif
#endif

void closeallfiles(firstfd)
int firstfd;
{
    register int i;
    register int nfile = _NFILE;

    for (i = firstfd; i < nfile; i++)
	(void) close(i);
}
