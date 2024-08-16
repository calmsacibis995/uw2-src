/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/router/resetroute.c	1.10"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: resetroute.c,v 1.9 1994/09/01 21:32:23 vtag Exp $"

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
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stropts.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/param.h>
#include <sys/types.h>
#include <sys/nwportable.h>
#include <sys/ipx_app.h>
#include <sys/ripx_app.h>
#include "nps.h"
#include "nwmsg.h"
#include "npsmsgtable.h"
#include "nwconfig.h"

/* ARGSUSED */
void
main( int argc, char *argv[], char *envp[])
{
	char titleStr[]="RROUTER";   /* Uppercase name */
	char *program;              /* LowerCase name */
	int fdIpx;
	int ccode;

    /* basename of the program used for pid log */
    if ((program = strrchr(argv[0], '/')) == NULL)
        program = argv[0];
    else
        program++;

	ccode = MsgBindDomain(MSG_DOMAIN_RROUT, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
    if(ccode != NWCM_SUCCESS) {
        fprintf(stderr,"%s: Cannot bind message domain. NWCM error = %d. Exiting.\n", 
            titleStr, ccode);
        exit(-1);
    }

	if((fdIpx = open("/dev/ripx",O_RDWR)) == -1) {
		fprintf(stderr, MsgGetStr(RRT_OPEN_FAILED), "/dev/ripx");
		perror("");
		exit(-1);
	}
	
	if(PrivateIoctl(fdIpx,RIPX_RESET_ROUTER,NULL,0) == FAILURE) {
		fprintf(stderr, MsgGetStr(RRT_RESET_ROUTER));
		exit(-1);
	}
	exit(0);
}
