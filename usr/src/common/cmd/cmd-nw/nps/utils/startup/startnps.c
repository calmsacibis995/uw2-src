/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/startup/startnps.c	1.11"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: startnps.c,v 1.9 1994/09/01 21:37:08 vtag Exp $"
/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "nwmsg.h"
#include <limits.h>
#include <unistd.h>
#include <sys/types.h>
#include "nwconfig.h"
#include "npsmsgtable.h"

/* ARGSUSED */
void
main( int argc, char *argv[], char *envp[])
{
	char titleStr[80];          /* Uppercase name */
	char *program;              /* LowerCase name */
	char npsPathName[NWCM_MAX_STRING_SIZE];
	char pathStr[NWCM_MAX_STRING_SIZE+8]; /* Allow room for npsd */
	char *p, *q;
	int	  Cret;
	int ccode;

    /* basename of the program used for pid log */
    if ((program = strrchr(argv[0], '/')) == NULL)
        program = argv[0];
    else
        program++;

    for (p = program, q = titleStr; *p; p++, q++) {
        *q = (char)toupper(*p);
    }
	*q = '\0';

	ccode = MsgBindDomain(MSG_DOMAIN_NSTRT, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
    if(ccode != NWCM_SUCCESS) {
        fprintf(stderr,"%s: Unable to bind message domain. NWCM error = %d. Error exit.\n",
            titleStr, ccode);
        exit(1);
    }

    if( (Cret = NWCMGetParam("binary_directory", NWCP_STRING, npsPathName)) != SUCCESS)
 {
	fprintf(stderr, "\n");
        NWCMPerror( Cret, MsgGetStr(NSTRT_CFG_X), "binary_directory");
        exit(1);
    }
	sprintf(pathStr, "%s/npsd", npsPathName);

	if( argc > 1) {
		/* Pass options */
		if( execl( pathStr, "npsd (IPX Protocol Stack Daemon)", argv[1], NULL) == -1) {
			perror( MsgGetStr(NSTRT_NPS_EXEC_FAIL));
			exit(1);
		}
	} else {
		if( execl( pathStr, "npsd (IPX Protocol Stack Daemon)", NULL) == -1) {
			perror( MsgGetStr(NSTRT_NPS_EXEC_FAIL));
			exit(1);
		}
	}
	exit(0);
}
