/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwchecksums:nwchecksums.c	1.6"
#ident  "@(#)nwchecksums.c     9.1 "
#ident  "$Id: nwchecksums.c,v 1.3.4.3 1995/02/13 19:24:04 hashem Exp $"

#include <stdio.h>
#include <sys/xti.h>
#include <sys/nwportable.h>
#include <nw/ntypes.h>
#include <nwconfig.h>
#include <sys/nwmp.h>
#include <nct.h>
#include <pfmt.h>
#include <locale.h>

main(argc, argv)
int argc;
char *argv[];
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
	struct checksumLevel args;

	setlocale( LC_ALL, "" );
	setlabel( "UX:nwchecksums" );
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
		if(NWCMGetParam("nuc_checksum_level", NWCP_INTEGER, (void *) &level)) {
			Error(":407:Unable to get nuc_checksum_level NWCM parameter.\n");
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
		if(ccode = ioctl(fd, NWMP_SET_CHECKSUM_LEVEL, &args)) {
			Error(":410:Unable to set baseline checksum level. (0x%x)\n", ccode);
			exit( 7 );
		}

		if(popt) {
			if((ccode = NWCMSetParam("nuc_checksum_level",
										NWCP_INTEGER, (void *)&level))) {
				Error(":411:Unable to set nuc_checksum_level NWCM parameter.\n");
				exit( 8 );
			}
		}
	} else {
		if(ccode = get_checksum_levels(&defaultLevel, &baseLevel, &userLevel)) {
			Error(":412:Unable to get requester checksum levels.\n");
			exit(7);
		}

		nprintf(":413:Default Checksum Level  : ");
		if(defaultLevel > 3)
			nprintf(":416:(Uninitialized)\n");
		else
			fprintf(stdout, "%d\n", defaultLevel);

		nprintf(":414:Baseline Checksum Level : ");
		if(baseLevel > 3)
			nprintf(":416:(Uninitialized)\n");
		else
			fprintf(stdout, "%d\n", baseLevel);

		nprintf(":415:User Checksum Level     : ");
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
	pfmt( stderr, MM_ACTION, ":417:Usage: nwchecksums [-q]|[[-s 0-3] [-p]]\n" );
}
