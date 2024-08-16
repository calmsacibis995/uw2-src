/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/sapd/test/sapdtest.c	1.2"
#ident	"$Id: sapdtest.c,v 1.1 1994/01/29 00:54:54 vtag Exp $"

#include <stdio.h>
#include <sys/types.h>
#include <signal.h>
#include <sys/procset.h>
#include <fcntl.h>

main(int argc, char *argv[] )
{
	char readBuf[10];
	int	fd, err, sapPID;

	system("../nucsapd 0x3e2 0x4 &");
	sleep(5);

/*
**	open and read the sapdpid file
*/
	if((fd = open("/var/spool/sap/sapdpid", O_RDONLY)) < 0)
	{
		printf("Cannot open sapdpid file\n");	
		exit(-1);
	}

	err = read(fd, readBuf, 10);
	sapPID = strtol(readBuf, NULL, 0);
	printf("sapPID is '%d'\n", sapPID);
	printf("Press any key to send SIGHUP\n");
	getchar();

/*
**	Send a signal, this should change nothing since startup
*/
	sigsend(P_PID, sapPID, SIGHUP);
	sleep(2);
	printf("Change 0x4\n");
	printf("Press any key to send SIGHUP\n");
	getchar();

/*
**	Send a signal, this should change 0x4.
*/
	sigsend(P_PID, sapPID, SIGHUP);
	sleep(2);
	printf("Change 0x4 with .orig file\n");
	printf("Press any key to end test and kill nucsapd\n");
	getchar();
	
	kill(sapPID, SIGTERM);
}
