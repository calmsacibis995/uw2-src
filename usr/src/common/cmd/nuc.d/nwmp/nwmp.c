/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwmp:nwmp.c	1.7"
#ident  "$Header: /SRCS/esmp/usr/src/nw/cmd/nuc.d/nwmp/nwmp.c,v 1.7.4.1 1995/01/10 00:20:11 hashem Exp $"

/*
**  Netware Unix Client
**
**	  MODULE: main.c (nwmp a.out)
**	ABSTRACT: Main routine for the Netware Unix client starter/stopper
*/ 
#include <stdio.h>
#include <fcntl.h>

#include <sys/xti.h>
#include <sys/nwctypes.h>
#include <sys/nwmp.h>
#include <pfmt.h>
#include <locale.h>
#include <sys/nucerror.h>

#define FALSE 0

extern int errno;

void usage(void);

main( int	argc, char	*argv[], char	*envp[] )
{
	int		fd;
	int		ccode;
	char	*arg;
	int 	i, enable = -1, burst = -1;

	setlocale(LC_ALL, "");
	setlabel("UX:nwmp");
	setcat("uvlnuc");

	if (argc < 2)
		usage();

	for ( i = 1; i < argc; i++ ) {
		arg = argv[i];

		/* Lower case the string */
		while (*arg) {
			*arg = tolower(*arg);
			arg++;
		}

		if (strcmp(argv[i], "start") == 0)
			enable = TRUE;
		if (strcmp(argv[i], "stop") == 0)
			enable = FALSE;
		if (strcmp(argv[i], "burst") == 0)
			burst = 1;
		if (strcmp(argv[i], "noburst") == 0)
			burst = 0;
	}

	if (burst == -1 && enable == -1)
		usage();

	if ( (fd = NWMPOpen()) == -1) {
		pfmt(stderr, MM_ERROR, ":01:Management Portal open failure\n"); 
		exit(2);
	}		
		
	if ( enable != -1 ) {
		if ( enable == TRUE ) {
			if (ccode = StartCoreServices(fd))
				goto finished;
		} else {
			if (ccode = StopCoreServices(fd))
				goto finished;
		}
	}


	if ( burst != -1 )
		SetBurstFlag( fd, burst );

finished:

	NWMPClose(fd);
	exit(0);
}

void
usage(void) 
{
	pfmt(stderr, MM_ERROR, ":12:Invalid Usage\n");
	pfmt(stderr, MM_ACTION, 
		":02:Usage: nwmp start|stop|burst|noburst\n");
	exit(3);
}

StartCoreServices( int fd )
{
	int ccode;

	if (ccode = NWMPInitSPI(fd)) {
		if (ccode == SPI_ACCESS_DENIED) {
			pfmt(stderr, MM_ERROR, 
				":13:Start NetWare Services error: Access denied\n"); 
		} else {
			pfmt(stderr, MM_ERROR, 
				":03:Start NetWare Services failed with return code %d\n", 
				ccode); 
		}
	} else {
		if (ccode = NWMPInitIPC(fd)) {
			if (ccode == SPI_ACCESS_DENIED) {
				pfmt(stderr, MM_ERROR, 
					":14:Start NetWare IPC error: Access denied\n"); 
			} else {
				pfmt(stderr, MM_ERROR, 
					":04:INIT IPC failed with return code %d\n", ccode); 
				NWMPDownSPI(fd);
			}
		}
	}
	initreq(NULL, NULL);
	return;
}

StopCoreServices( int fd )
{
	int ccode;

	if (ccode = NWMPDownSPI(fd)) {
		if (ccode == SPI_ACCESS_DENIED) {
			pfmt(stderr, MM_ERROR, 
				":16:Stop NetWare Services error: Access denied\n"); 
		} else {
			pfmt(stderr, MM_ERROR, 
				":06:STOP SPIL failed with return code %d\n", ccode); 
		}
	}

	if (ccode = NWMPDownIPC(fd)) {
		if (ccode == SPI_ACCESS_DENIED) {
			pfmt(stderr, MM_ERROR, 
				":15:Stop NetWare IPC error: Access denied\n"); 
		} else {
			pfmt(stderr, 
				MM_ERROR, ":26:STOP IPC failed with return code %d\n",
				ccode); 
		}
	}	

	return;
}

/* 
 *	SetBurstFlag sets the internal flag doBurstFlag to TRUE/FALSE to 
 *	enable/disable packet burst from this workstation
 */
int
SetBurstFlag( int fd, int burstFlag )
{
	int ccode;

	ccode = ioctl( fd, NWMP_NCP_PACKET_BURST_FLAG, burstFlag );
	if (ccode) {
		if (ccode == SPI_ACCESS_DENIED) {
			pfmt(stderr, MM_ERROR, 
				":17:Burst Enable/Disable error: Access denied\n"); 
		} else {
			pfmt(stderr, MM_ERROR, 
				":11:error enabling/disabling Packet Burst: %s\n",
				strerror(errno)); 
		}
	}
}

