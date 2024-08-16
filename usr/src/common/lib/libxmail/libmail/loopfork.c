/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/loopfork.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)loopfork.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	loopfork - looping version of fork(2)

    SYNOPSIS
	int loopfork()

    DESCRIPTION
	Do a fork(2), looping on errors with a sleep
	in between.
*/

pid_t loopfork()
{
    pid_t p;
    unsigned int count = 0;
    for (count = 0; (p = fork()) == (pid_t)-1; count++)
	if (count == 40)
	    return p;
	else
	    (void) sleep(1 + count / 10);
    return p;
}
