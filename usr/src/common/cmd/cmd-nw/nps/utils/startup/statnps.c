/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/startup/statnps.c	1.9"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: statnps.c,v 1.7 1994/09/01 21:37:09 vtag Exp $"

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
#include <unistd.h>
#include "nwconfig.h"
#include "nwmsg.h"
#include "npsmsgtable.h"
#include "util_proto.h"

/* ARGSUSED */
void
main( int argc, char *argv[], char *envp[])
{
	char titleStr[80];          /* Uppercase name */
	char *program;              /* LowerCase name */
	const char *PidLogDir;
	char *p, *q;
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

	ccode = MsgBindDomain(MSG_DOMAIN_NSTAT, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
    if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
        fprintf(stderr,"%s: Cannot bind message domain. NWCM error = %d.  error exit.\n",
            titleStr, ccode);
        exit(1);
    }

	if( (PidLogDir = NWCMGetConfigDirPath()) == NULL) {
		fprintf(stderr, MsgGetStr(NSTAT_BAD_CONFIG));
		exit(1);
	}
	if( LogPidKill((char *)PidLogDir, "npsd", 0) == FAILURE) {
		/* Process does not exist */
		exit(1);
	}
	/* npsd is running */
	exit(0);
}
