/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwsignatures:nwsignatures.c	1.7"
#ident  "@(#)nwsignatures.c	9.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nwsignatures/nwsignatures.c,v 1.4.4.3 1995/02/13 19:35:58 hashem Exp $"

#include <stdio.h>
#include <sys/xti.h>
#include <sys/nwportable.h>
#include <nw/ntypes.h>
#include <nwconfig.h>
#include <sys/nwmp.h>
#include <nct.h>
#include <pfmt.h>
#include <locale.h>


main( int argc, char *argv[] )
{
	int 	fd;
	int 	ccode;
	int 	level, c;
	int 	sopt = 0;
	int 	qopt = 0;
	int 	popt = 0;
	uint32	defaultLevel;
	uint32	baseLevel;
	uint32	userLevel;
	struct signatureLevel args;

	setlocale( LC_ALL, "" );
	setlabel( "UX:nwsignatures" );
	setcat( "uvlnuc" );

	if (argc > 1) {
		while( (c = getopt(argc, argv, "s:qp?")) != EOF) {
			switch( c ) {
			case 's':
				if(qopt) {
					nprintf(":403:The 's' and 'q' options may not be used together.\n");
					exit( 1 );
				}
				sopt++;
				level = atoi( optarg );
				break;

			case 'q':
				if(sopt) {
					nprintf(":403:The 's' and 'q' options may not be used together.\n");
					exit( 1 );
				}
				qopt++;
				break;

			case 'p':
				if(qopt) {
					nprintf(":404:The 'p' and 'q' options may not be used together.\n");
					exit( 2 );
				}
				popt++;
				break;

			case '?':
				usage();
				exit( 0 );

			default:
				break;
			}
		}

		if(popt && !sopt) {
			nprintf(":406:The 'p' option must be used with the 's' option.\n");
			exit( 3 );
		}

	} else {
		if(NWCMGetParam("nuc_signature_level", NWCP_INTEGER, (void *) &level)) {
			Error(":418:Unable to get nuc_signature_level NWCM parameter.\n");
			exit( 4 );
		}
		sopt++;
	}


	if(sopt) {
		if(level < 0 || level > 3) {
			Error(":408:Invalid level selected. (%d)\n", args.level);
			exit( 5 );
		}
		args.level = level;
	}

	if ((fd = NWMPOpen()) == -1) {
		Error(":409:Unable to open NWMP\n");
		exit( 6 );
	}

	if(sopt) {
		if(ccode = ioctl(fd, NWMP_SET_SIGNATURE_LEVEL, &args)) {
			Error(":419:Unable to set baseline signature level. (0x%x)\n", ccode);
			exit( 7 );
		}

		if(popt) {
			if((ccode = NWCMSetParam("nuc_signature_level",
										NWCP_INTEGER, (void *)&level))) {
				Error(":420:Unable to set nuc_signature_level NWCM parameter.\n");
				exit( 8 );
			}
		}
	} else {
		if(ccode = get_security_levels(&defaultLevel, &baseLevel, &userLevel)) {
			Error(":421:Unable to get requester signature levels.\n");
			exit(7);
		}

		nprintf(":422:Default Signature Level  : ");
		if(defaultLevel > 3)
			nprintf(":416:(Uninitialized)\n");
		else
			fprintf(stdout, "%d\n", defaultLevel);

		nprintf(":423:Baseline Signature Level : ");
		if(baseLevel > 3)
			nprintf(":416:(Uninitialized)\n");
		else
			fprintf(stdout, "%d\n", baseLevel);

		nprintf(":424:User Signature Level     : ");
		if(userLevel > 3)
			nprintf(":416:(Uninitialized)\n");
		else
			fprintf(stdout, "%d\n", userLevel);
	}

	NWMPClose(fd);
	exit(0);
}

int usage()
{
	pfmt( stderr, MM_ACTION, ":425:Usage: nwsignatures [-q]|[[-s 0-3] [-p]]\n" );
}
