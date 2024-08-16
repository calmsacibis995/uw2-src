/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/main.c	1.2"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/main.c,v 1.2 1994/10/06 22:00:55 nick Exp $";
#endif

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "rprinter.h"
#include "inform.h"
#include "misc_proto.h"
#include "status_proto.h"
#include "commands_proto.h"
#include "upcntrl_proto.h"
#include "upconfig_proto.h"
#include "main_proto.h"

#define SD_GRACEFUL		SIGTERM		/* signal sent by shutdown script */
#define SD_GRACEFUL1	SIGQUIT		/* signal sent by shutdown script */

static int				 rpCount;
static RPrinterInfo_t	*rpList[MAX_PRINTERS];

char	*program;


/*********************************************************************
** BEGIN_MANUAL_ENTRY( main, \
**                     api/utils/rprinter/main )
**			RPRINTER main routine
**
** SYNOPSIS
**			(void) main( argc, argv )
**			int   argc;
**			char *argv[];
**
** INPUT
**			argc				(see Initialize)
**
**			argv				(see Initialize)
**
** DESCRIPTION
**			argc and argv are passed to Initialize where their
**			use is defined. We then go into an infinite loop
**			doing two things: we look for packets from PSERVER's
**			and at appropriate intervals we check to see if our
**			configuration should be changed and do a status check
**			on it.
**
**			main is meant to be run as a daemon and can only be
**			shut down by some external stimulus provoking a call
**			to Shutdown.
**
**			Significant global routines called are:
**				Initialize
**				HavePServerCommand
**				ProcessPServerCommands
**				IsTimeForCheckup
**				HaveNewRPConfiguration
**				UpdateRPConfiguration
**				HaveNewRPControlInfo
**				UpdateRPControlInfo
**				StatusCheckRPrinters
**
** NOTE
**			Nothing really starts happening until
**			UpdateRPControlInfo is called for the first time.
**
** SEE ALSO
**			Initialize, ShutDown
**
** END_MANUAL_ENTRY ( 10/29/90 )
**/
void
main(int	argc,
	 char	*argv[])
{
	if ((program = strrchr(argv[0], '/')) == NULL)
		program = argv[0];
	else
		program++;
	
	Initialize( argc, argv );

	for ( ;; ) {
		if (HavePServerCommand( rpList, rpCount ))
			ProcessPServerCommands( rpList, rpCount );

		if (IsTimeForCheckup()) {
			if (HaveNewRPConfiguration())
				UpdateRPConfiguration();

			if (HaveNewRPControlInfo())
				UpdateRPControlInfo( rpList, &rpCount );

			StatusCheckRPrinters( rpList, &rpCount );
		}
	}
}


/*********************************************************************
** BEGIN_MANUAL_ENTRY( Shutdown, \
**                     api/utils/rprinter/shutdown )
**			RPRINTER shut down routine
**
** SYNOPSIS
**			void Shutdown( shutdownType )
**			int shutdownType;
**
** INPUT
**			shutdownType		if SD_GRACEFUL is passed it means a
**								graceful shutdown is requested;
**								otherwise we do a quick and dirty
**								shutdown
**
** DESCRIPTION
**			To shut down gracefully, Shutdown first marks all entries
**			in the remote printer list "toBeDeleted = TRUE". It then
**			cycles somewhat normally, giving print jobs a chance to
**			complete, until the remote printer count goes to zero.
**
**			Significant global routines called are:
**				HavePServerCommand
**				ProcessPServerCommands
**				StatusCheckRPrinters
**				InformTerminate
**				exit
**
** SEE ALSO
**			main, Initialize, InformTerminate
**
** END_MANUAL_ENTRY ( 11/26/90 )
**/

void
ShutDown(int shutdownType )
{
	int i;

	if ((shutdownType == SD_GRACEFUL) || (shutdownType == SD_GRACEFUL1)) {
		Inform( (RPrinterInfo_t *) 0, RPMSG_QUITING, MSG_VERBOSE );

		for (i = 0; i < rpCount; i++)
			rpList[i]->toBeDeleted = TRUE;

		while (rpCount) {
			StatusCheckRPrinters( rpList, &rpCount );

			if (HavePServerCommand( rpList, rpCount ))
				ProcessPServerCommands( rpList, rpCount );
		}

		Inform( (RPrinterInfo_t *) 0, RPMSG_QUIT, MSG_VERBOSE );
	} else {	/* not a graceful shut down */
		Inform( (RPrinterInfo_t *) 0, RPMSG_TERMINATED, MSG_NORMAL );
	}

	InformTerminate();		/* close and flush the debug file */
	exit( 0 );
}
