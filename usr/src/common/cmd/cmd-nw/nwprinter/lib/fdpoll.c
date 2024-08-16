/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/lib/fdpoll.c	1.1"
/*
 * Copyright 1989, 1991 Unpublished Work of Novell, Inc. All Rights Reserved.
 * 
 * THIS WORK IS AN UNPUBLISHED WORK AND CONTAINS CONFIDENTIAL, 
 * PROPRIETARY AND TRADE SECRET INFORMATION OF NOVELL, INC. ACCESS
 * TO THIS WORK IS RESTRICTED TO (I) NOVELL EMPLOYEES WHO HAVE A
 * NEED TO KNOW TO PERFORM TASKS WITHIN THE SCOPE OF THEIR
 * ASSIGNMENTS AND (II) ENTITIES OTHER THAN NOVELL WHO HAVE
 * ENTERED INTO APPROPRIATE AGREEMENTS. 
 * NO PART OF THIS WORK MAY BE USED, PRACTICED, PERFORMED,
 * COPIED, DISTRIBUTED, REVISED, MODIFIED, TRANSLATED, ABRIDGED,
 * CONDENSED, EXPANDED, COLLECTED, COMPILED, LINKED, RECAST,
 * TRANSFORMED OR ADAPTED WITHOUT THE PRIOR WRITTEN CONSENT
 * OF NOVELL.  ANY USE OR EXPLOITATION OF THIS WORK WITHOUT
 * AUTHORIZATION COULD SUBJECT THE PERPETRATOR TO CRIMINAL AND
 * CIVIL LIABILITY.
 */

#if !defined(NO_SCCS_ID) && !defined(lint)
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/lib/fdpoll.c,v 1.2 1994/03/15 21:11:40 eric Exp $";
#endif

#include <fcntl.h>
#include <stdlib.h>
#ifdef OS_AIX
#include <sys/poll.h>
#else
#include <poll.h>
#endif /* OS_AIX */
#include <errno.h>
#include <sys/nwportable.h>
#include <sys/nwctypes.h>
#include <sys/nwtypes.h>
#include "fdpoll_proto.h"

int
FDWaitForInput(FDWait_t *waitList, int listCount, int maxWaitTime )
{
	int i;
	int pollrc;
	unsigned long numOfFds;
	struct pollfd *fds;

	fds = (struct pollfd *) malloc( sizeof( struct pollfd ) *
		listCount );
	for (i = 0; i < listCount; i++) {
		fds[i].fd = waitList[i].fd;
		fds[i].events = POLLIN | POLLPRI;
		fds[i].revents = 0;
	}

	numOfFds = listCount;
	do {
		pollrc = poll( fds, numOfFds, maxWaitTime );
	} while (pollrc < 0 && errno == EINTR);

	if (pollrc > 0)
		for (i = 0; i < listCount; i++)
			if (fds[i].revents)
				waitList[i].hasInput = TRUE;

	free( (char *) fds );
	return pollrc;
}

