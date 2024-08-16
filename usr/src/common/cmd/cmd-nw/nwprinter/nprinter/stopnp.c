/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/stopnp.c	1.5"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/stopnp.c,v 1.6 1994/09/23 15:28:49 mark Exp $";
#endif

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <signal.h>
#if !defined(PATH_MAX)
#define PATH_MAX 255
#endif

#include <nwconfig.h>
#include <nwmsg.h>
#include <printmsgtable.h>


static void
getNWCMStr(char *nwcmStr, char *targetStr, char *defaultStr)
{
	int status;

    status = NWCMGetParam(nwcmStr, NWCP_STRING, (void *)targetStr);

    if( (status != NWCM_SUCCESS) || (targetStr[0] == 0) ) {
		/* Error getting default console, use default */
		strcpy(targetStr, defaultStr);
#ifdef DEBUG
		printf("getNWCMstr: String %s not found using %s\n",
				nwcmStr, defaultStr);
#endif
    }
}


void
main(void)
{
	int fd;
	int	ret;
	int pid = 0;
	int status;
	char buf[100];
	char subDirName[NWCM_MAX_STRING_SIZE];
	char logDirPath[PATH_MAX];
	

	status = MsgBindDomain(MSG_DOMAIN_NPRINT, MSG_DOMAIN_PRINT_FILE, MSG_PRINT_REV_STR);
	if (status != SUCCESS) {
		fprintf(stderr, "FATAL ERROR: Error in message setup.\n");
	}


	strcpy(logDirPath, NWCMGetConfigDirPath());
	getNWCMStr("nprinter_config_directory", subDirName, "");

        strncat(logDirPath, "/", PATH_MAX-1);
        strncat(logDirPath, subDirName, PATH_MAX-1);

	status = LogPidKill( logDirPath, "nprinter", SIGTERM );

	if (status) {
		fprintf(stderr, MsgGetStr(NPSTOP_KILL_ERR), 
				(int)pid);
	} else
		(void) DeleteLogPidFile( logDirPath, "nprinter" );



}
