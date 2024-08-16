/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:sighup.c	1.5"
#ident	"$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nucd/sighup.c,v 1.1.4.4 1995/02/13 19:13:32 hashem Exp $"

/******************************************************************************
 ******************************************************************************
 *
 *	SIGHUP.C
 *
 *	Handle IPX M_HANGUP messages
 *
 ******************************************************************************
 *****************************************************************************/

#include <stdio.h>
#include <ctype.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <sys/xti.h>
#include <sys/nwctypes.h>
#include <sys/nwmp.h>
#include <sys/signal.h>

#include <pfmt.h>
#include <locale.h>

#ifndef	FAILURE
#define	FAILURE	-1
#endif
#ifndef	SUCCESS
#define	SUCCESS	0
#endif

extern	int		nuc_debug;
extern	FILE		*log_fd;

/******************************************************************************
 *
 *	sighup ( int arg )
 *
 *	Handle IPX M_HANGUP
 *
 *	Entry :
 *		arg			passed from thread create -- nothing
 *
 *	Exit :
 *		0			cool
 *		x			error
 *
 *****************************************************************************/

int
sighup ( int arg )
{
	int	sighupFd;
	int	rc;

	setuid( 0 );

	/*
	 * Open the NUC driver.
	 */
	if ( (sighupFd = NWMPOpen()) == -1) {
		if (nuc_debug)
			(void)pfmt ( log_fd, MM_ERROR, ":437:open /dev/nuc00 failed.\n");
		perror ("nuc_sighup: No more /dev/nuc00 devices");
		return (1);
	}

	/*
	 * Make the NUC driver wait for a M_HANGUP from IPX driver
	 */
	if (ioctl (sighupFd, NWMP_HANDLE_IPX_HANGUP, &rc) != SUCCESS) {
		(void)pfmt (log_fd, MM_ERROR, ":438:sighup IOCTL failed\n");
		return (2);
	}

	setuid( 0 );
}
