/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:xx/vsig.c	1.6.3.3"

/* 
 * This short program is called by a co-process if it wishes to 
 * communicate asynchronously with the controlling FMLI process during
 * the course of its execution.  It blocks til FMLI is ready for a
 * signal then sends a SIGUSR2. 
 */

#include <stdio.h>
#include <signal.h>
#include <fcntl.h>
#include "sizes.h"


/* the path of the FIFO for process synchronization */
char Semaphore[PATHSIZ] = "/tmp/FMLIsem.";

main(argc, argv)
int argc;
char *argv[];
{
	FILE *fp;
	char name[PATHSIZ];
	char *vpid;
	char *getenv();

	if ((vpid = getenv("VPID")) == NULL)
		exit(1);
	strcat(Semaphore, vpid);
	fflush(stdout);
	fflush(stderr);
	/*
	 * The reason for the close(open) is to
	 * block until FACE says its is OK
	 * to send a signal ... A signal when
	 * FACE is updating the screen
	 * can create garbage .....
	 */
	close(open(Semaphore, O_WRONLY));
	kill(strtol(vpid, (char **)NULL, 0), SIGUSR2); /* EFT abs k16 */
	exit(0);
}
