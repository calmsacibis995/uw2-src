/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)profiler:prfstat.c	1.6.4.3"
#ident	"$Header: $"

/*
 *	prfstat - change and/or report profiler status
 */

#include <stdio.h>
#include <fcntl.h>
#include <sys/prf.h>

#ifdef sgn
#undef sgn
#endif

#define sgn( x )	( (x) ? ( (x) > 0 ? 1 : -1 ) : 0 )

main( int argc, char **argv ) {
	int prf;
	int prfmax;
	int prfstat;
	char *namelist;

	void prf_start( const int prf_fd, const char *namelist );

	if((prf = open("/dev/prf", O_RDWR)) < 0)
		error("cannot open /dev/prf\n");

	if(argc > 3) {
		fprintf(stderr,"usage: prfstat [off | on [system_namelist]]\n");
		exit(1);
	}
	namelist = NULL;
	if( argc == 3 ) {
		namelist = argv[2];
		argc--;
	}
	if(argc == 2) {
		if(strcmp("off", argv[1]) == 0) {
			if( namelist )
				fprintf(stderr,"prfstat: namelist argument ignored\n");
			ioctl(prf, PRF_DISABLE, 0);
		}
		else if(strcmp("on", argv[1]) == 0)
			prf_start( prf, namelist );
		else {
			fprintf(stderr,"prfstat: unrecognized argument.\n");
			fprintf(stderr,"usage: prfstat [off | on [system_namelist]]\n");
			exit(1);
		}
	}

	prfstat = ioctl(prf, PRF_STAT, 0);

	switch( sgn( prfstat ) ) {
	case 0:
		printf("profiling disabled.\n");
		break;
	case 1:
		printf("profiling enabled.\n");
		break;
	default:
		error("cannot determine profiling status.\n");
	}

	prfmax = ioctl(prf, PRF_MAX, 0);

	switch( sgn( prfmax ) ) {
	case 1:
		printf("%d kernel text addresses loaded.\n", prfmax );
		break;
	case 0:
		printf("no kernel text addresses loaded.\n");
		break;
	default:
		error("cannot determine number of text addresses.\n");
	}
	exit( 0 );
}

