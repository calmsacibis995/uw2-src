/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/dkbreak.c	1.1.1.3"
#ident	"$Header: $"

/*#ifndef DIAL
	static char	SCCSID[] = "dkbreak.c	2.2+BNU DKHOST 86/04/02";
#endif */
/*
 *	COMMKIT(TM) Software - Datakit(R) VCS Interface Release 2.0 V1
 *			Copyright 1984 AT&T
 *			All Rights Reserved
 *
 *	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF AT&T
 *     The copyright notice above does not evidence any actual
 *          or intended publication of such source code.
 */
#include	"dk.h"

	static short	sendbreak[3] = {
		72, 0, 0
	};	/* Asynch Break Level-D code */

GLOBAL void
dkbreak(fd)
{
	char	nothing[1];

	ioctl(fd, DIOCXCTL, sendbreak);

	write(fd, nothing, 0);
}
