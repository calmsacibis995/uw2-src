/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nucd:message.c	1.10"
/******************************************************************************
 ******************************************************************************
 *
 *	MESSAGE.C
 *
 *	NetWare Message Handler Thread
 *
 * 	MODULE:	message.c - The NetWare UNIX Client Message Daemon for 
 *		handling all broadcast messages from attached NetWare servers.
 *
 * 	ABSTRACT:
 *		nucmessagedd.c is a SETUID root daemon that continuously 
 *		requests broadcast messages from the NUC via the Management Portal.
 *		Only one such daemon should be executing on the client.  Others will
 *		terminate once they have detected the duplication.  
 *
 ******************************************************************************
 *****************************************************************************/

#include <stdio.h>
#include <fcntl.h>
#include <string.h>
#include <ctype.h>
#include <syslog.h>
#include <memory.h>
#include <time.h>
#include <errno.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/param.h>
#include <signal.h>

#include <sys/nwctypes.h>
#include <sys/nucerror.h>
#include <sys/slstruct.h>
#include <sys/spilcommon.h>
#include <sys/nwmp.h>
#include <thread.h>

#include <pfmt.h>
#include <locale.h>

extern int 		errno;

int 			messaged ( int arg );

/******************************************************************************
 *
 *	message ( int arg )
 *
 *	Constantly request messages from the Management Portal and write them
 *	out to the SYSLOG
 *
 *	Entry :
 *		arg		not used
 *
 *	Exit :
 *		1		something went wrong
 *
 *****************************************************************************/

int
message ( int arg )
{
	int 			fp, fd, ccode = 0, pid;
	uint16 			serverConnID, clientConnID;
	struct 			getServiceMessageReq request;
	char 			sprotoServiceName[64];
	char 			sprotoUserName[64];
	char 			messageText[128];
	struct tm 		timer;
	time_t			currentTime;
	char 			dateTime[36];
	uint32			priority = LOG_NOTICE|LOG_LOCAL6;
	struct	passwd	*password;
	char			buffer[MAX_ADDRESS_SIZE];

	/*
	 *	Init SYSLOG
	 */

	openlog("nucmessaged", LOG_PID|LOG_CONS, LOG_DAEMON);

	/*
	 *	Open the Management Portal
	 */

	if ((fp = NWMPOpen()) == -1) {
		syslog(priority,
			"nucd: Message Management Portal open failure errno=%XH\n",
			errno);
		return (1);
	}
		
	/*
	 *	Until terminated by signal or NWMP shutdown, retrieve any messages
	 *	from service protocol NCP and write those messages to the system
	 *	console.
	 */

	while( 1 ) {
		memset( sprotoServiceName, '\0', sizeof(sprotoServiceName));
		memset( sprotoUserName, '\0', sizeof(sprotoUserName));
		request.serviceProtocol = SPROTO_NCP; 
		request.sprotoServiceName = sprotoServiceName;
		request.sprotoUserName = sprotoUserName;
		request.messageText = messageText;
		request.messageLength = sizeof(messageText);
		request.spilServiceAddress.maxlen = MAX_ADDRESS_SIZE;
		request.spilServiceAddress.buf = buffer;
		ccode = ioctl(fp, NWMP_GET_SERVICE_MESSAGE, &request);
		if ( ccode ) {
			syslog(priority,
				"nucd: NWMP_GET_SERVICE_MESSAGE returned %xH, errno=%xH, diag=%xH\n",
				ccode, errno, request.diagnostic);
		} else {
			currentTime = time(NULL);
			strncpy(dateTime, asctime(localtime(&currentTime)), 19);
			dateTime[19] = '\0';
			messageText[request.messageLength] = '\0';
			password = getpwuid(request.uid);
			syslog(priority, "NetWare userid: %s  UNIX UID: %s  Message: %s\n",
				sprotoUserName, password->pw_name, 
				messageText);
		}	
	}

	NWMPClose(fp);
	return (0);
}

/******************************************************************************
 *
 *	messagekill ( void )
 *
 *	Kill the message daemon
 *
 *	Entry :
 *		Nothing
 *
 *	Exit :
 *		Nothing
 *
 *****************************************************************************/

void
messagekill ( void )
{
}
