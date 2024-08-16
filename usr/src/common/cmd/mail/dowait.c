/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/dowait.c	1.1.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)dowait.c	1.1 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	dowait - wait for a given process to end

    SYNOPSIS
	int dowait(pid_t pidval)

    DESCRIPTION
	dowait() uses wait(2) to wait for a given child
	process to end. It returns the exit code of the
	child, or an indication of error.
*/

int dowait(pidval)
pid_t	pidval;
{
    register pid_t w;
    int status;
    void (*istat)(), (*qstat)();

    /*
	Parent temporarily ignores signals so it will remain
	around for command to finish
    */
    istat = signal(SIGINT, SIG_IGN);
    qstat = signal(SIGQUIT, SIG_IGN);

    while ((w = wait(&status)) != pidval && w != CERROR)
	;

    if (w == CERROR)
	status = -errno;

    else
	status = ((status>>8)&0xFF);  		/* extract 8 high order bits */

    signal(SIGINT, istat);
    signal(SIGQUIT, qstat);
    return (status);
}

