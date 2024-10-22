/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/dkminor.c	1.1.1.3"
#ident	"$Header: $"

/* #ifndef DIAL
	static char	SCCSID[] = "dkminor.c	2.3+BNU DKHOST 86/12/02";
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
/*
   Return minor device number for a given Datakit device.
   The channel number is used since using the minor device returned
   by fstat gives wrong results for duplex systems.
*/

#include "dk.h"
GLOBAL
dkminor(fd)
{
	struct diocreq iocb;


	if (ioctl(fd, DIOCINFO, &iocb) < 0)
		return(-1);
	return(iocb.req_chmin); /* req_chmin contains channel number */
}
