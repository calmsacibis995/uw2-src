/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nwprinter/nprinter/restartnp.c	1.1"
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
static char sccsid[] = "@(#)$Header: /SRCS/esmp/usr/src/nw/cmd/cmd-nw/nwprinter/nprinter/restartnp.c,v 1.1 1994/09/27 16:59:05 novell Exp $";
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


int countNprinterEntries(char *entryFile)
{
    FILE    *fp;
    int     count = 0;
    char    buf[256];

    if (fp = fopen(entryFile, "r")){
        while (fgets(buf, 255, fp)){
            if (strchr(buf, '#') == NULL)
                count++;
        }
    }
    return(count);
}


void
main(void)
{
	int  status;
	char subDirName[NWCM_MAX_STRING_SIZE];
	char fileName[NWCM_MAX_STRING_SIZE];
	char filePath[PATH_MAX];

	status = MsgBindDomain(MSG_DOMAIN_NPRINT, MSG_DOMAIN_PRINT_FILE, MSG_PRINT_REV_STR);
	if (status != SUCCESS) {
		fprintf(stderr, "FATAL ERROR: Error in message setup.\n");
	}


	strcpy(filePath, NWCMGetConfigDirPath());
	getNWCMStr("nprinter_config_directory", subDirName, "");
	getNWCMStr("nprinter_control_file", fileName, "RPControl");

	strncat(filePath, "/", PATH_MAX-1);
	strncat(filePath, subDirName, PATH_MAX-1);
	strncat(filePath, "/", PATH_MAX-1);
	strncat(filePath, fileName, PATH_MAX-1);


	if (countNprinterEntries(filePath) != 0){
		system("/usr/sbin/nprinter &");
	}
}

