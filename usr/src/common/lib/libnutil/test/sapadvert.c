/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/test/sapadvert.c	1.1"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

# /* ident	"@(#)lib/libnwutil/test/sapadvert.c	1.0" */
# /* ident	"$Id: sapadvert.c,v 1.1 1994/10/03 14:43:09 mark Exp $" */

#include <stdio.h>
#include <fcntl.h>
#include <sys/sap_app.h>
#include <tiuser.h>
#include "nwconfig.h"


main()
{
	uint16		serverType1, serverType2, serverType3;
	int			ccode, uid;
	char		serverName[NWCM_MAX_STRING_SIZE];
	uint16		mySocket;
	
	strcpy(serverName, "SAPADVERT_TEST");
	serverType1 = FILE_SERVER_TYPE;
	serverType2 = DIRECTORY_SERVER_TYPE;
	serverType3 = PRINT_SERVER_TYPE;
	mySocket = 0x2345;

	printf("Advertise FILE_SERVER_TYPE\n");
	ccode = SAPAdvertiseMyServer( serverType1, (uint8 *)serverName,
							mySocket, SAP_ADVERTISE );
	if(ccode)
		printf("     SAPAdvertiseMyServer failed, ccode = '%d'\n", ccode);
	else
		printf("     SAPAdvertiseMyServer succeeded\n");

	printf("Advertise DIRECTORY_SERVER_TYPE\n");
	ccode = SAPAdvertiseMyServer( serverType2, (uint8 *)serverName,
							mySocket, SAP_ADVERTISE );
	if(ccode)
		printf("     SAPAdvertiseMyServer failed, ccode = '%d'\n", ccode);
	else
		printf("     SAPAdvertiseMyServer succeeded\n");

	printf("Advertise PRINT_SERVER_TYPE\n");
	ccode = SAPAdvertiseMyServer( serverType3, (uint8 *)serverName,
							mySocket, SAP_ADVERTISE );
	if(ccode)
		printf("     SAPAdvertiseMyServer failed, ccode = '%d'\n", ccode);
	else
		printf("     SAPAdvertiseMyServer succeeded\n");

	printf("\nPress <ENTER> to terminate advertising\n");
	getchar();

/*
**	Cancel advertising of all services
*/
	printf("\nCanceling SAP advertisements\n");
	printf("--------------------------------------------------\n");

	ccode = SAPAdvertiseMyServer( serverType1, (uint8 *)serverName,
							mySocket, SAP_STOP_ADVERTISING );
	if(ccode)
		printf("     Unadvertise FILE_SERVER_TYPE.  Got unexpected error '%d'\n", ccode);
	else
		printf("     Unadvertise FILE_SERVER_TYPE succeeded\n");
	ccode = SAPAdvertiseMyServer( serverType2, (uint8 *)serverName,
							mySocket, SAP_STOP_ADVERTISING );
	if(ccode)
		printf("     Unadvertise DIRECTORY_SERVER_TYPE.  Got unexpected error '%d'\n", ccode);
	else
		printf("     Unadvertise DIRECTORY_SERVER_TYPE succeeded\n");

	ccode = SAPAdvertiseMyServer( serverType3, (uint8 *)serverName,
							mySocket, SAP_STOP_ADVERTISING );
	if(ccode)
		printf("     Unadvertise PRINT_SERVER_TYPE.  Got unexpected error '%d'\n", ccode);
	else
		printf("     Unadvertise PRINT_SERVER_TYPE succeeded\n");

}

