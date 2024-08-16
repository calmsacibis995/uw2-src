/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/startup/stopnps.c	1.12"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: stopnps.c,v 1.11 1994/09/23 17:54:45 vtag Exp $"
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
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <utmp.h>
#include <errno.h>
#include <unistd.h>
#include "nwconfig.h"
#include "nwmsg.h"
#include "npsmsgtable.h"
#include "util_proto.h"
#include "microsleep.h"

#define IPX "/dev/ipx0"
#ifdef DEBUG
char optstr[] = "uq";
#else
char optstr[] = "q";
#endif

/* ARGSUSED */
void
main( int argc, char *argv[], char *envp[])
{
	char titleStr[80];          /* Uppercase name */
	char *program;              /* LowerCase name */
	const char *PidLogDir;
	char *p, *q;
	int	 opt, qopt = 0;
	int	 uopt = 0;
	int  i;
	int ccode;
	int	 stopped;
	struct utmp *up, cup;

    /* basename of the program used for pid log */
    if ((program = strrchr(argv[0], '/')) == NULL)
        program = argv[0];
    else
        program++;

    for (p = program, q = titleStr; *p; p++, q++) {
        *q = (char)toupper(*p);
    }
	*q = '\0';

	ccode = MsgBindDomain(MSG_DOMAIN_NSTOP, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) { 
		/* Do not internationalize */
        fprintf(stderr,"%s: Cannot bind message domain. NWCM error = %d. Error exit.\n",
            titleStr, ccode);
        exit(1);
    }

	if( (PidLogDir = NWCMGetConfigDirPath()) == NULL) {
		fprintf(stderr, MsgGetStr(NSTOP_BAD_CONFIG));
		exit(1);
	}

	while ((opt = getopt(argc, argv, optstr)) != (char)-1) {
		switch (opt) {
			case 'q':
				qopt++;
				break;
#ifdef DEBUG
			case 'u':
				uopt++;
				break;
#endif
			default:
				break;
		}
	}

	if( !uopt) {
		setutent();
		cup.ut_type = RUN_LVL;
		up = getutid( &cup);

		/*
		**	Don't allow stopnps when in multi user mode
		*/
		switch( up->ut_exit.e_termination) {
		case '2':
		case '3':
		case '4':
			fprintf(stderr, MsgGetStr(NSTOP_BAD_INIT));
			exit(1);
		default:
			break;
		}
		endutent();
	}
	/*
	**	Send the actual signal to stop npsd
	*/
	LogPidKill((char *)PidLogDir, "npsd", SIGTERM);

	/*
	**	Wait for NPSD to exit, it really should finish, so
	**	wait longer than needed to make sure it really finishes (2 minutes).
	**	If it down't finish in that time, something is wrong.
	*/
	stopped = 0;
	for(i = 0; i < 120*10; i++) {
		if( LogPidKill((char *)PidLogDir, "npsd", 0) == FAILURE) {
			stopped++;
			break;
		}
		MicroSleep(100000); /* 1/10 second waits */
	}
	/*
	**	If q option is set, don't wait for all other devices to
	**	be closed.  Used by shutdown script to prevent shutdown hang 
	**	when badly written applications have the ipx device open
	*/
	if( stopped ) {
		if( qopt == 0) {

			/*
			**	Wait for all IPX clones to close
			*/
			for(i = 0; i < 60; i++) {
				if ( open(IPX, O_RDWR) == -1 ) {
					if( errno != EAGAIN)
						break;
					MicroSleep(1000000); /* 1 second waits */
				} else {
					/*
					**	Opened /dev/ipx0, all clones are gone from previous open
					*/
					stopped++;
					break;
				}
			}

			if( stopped != 2) {
				/*	Fell thru loop without successful open of /dev/ipx0 */
				exit(2);
			}
		}
	} else {
		/*	Fell thru loop without npsd going away */
		exit(1);
	}
	exit(0);
}
