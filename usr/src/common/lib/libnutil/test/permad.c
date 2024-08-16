/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/test/permad.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: permad.c,v 1.2 1994/09/27 21:47:06 mark Exp $"

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
	uint16		mySocket, mySocket1;
	

/*
**	Advertise the name of our server
*/
	if(NWCMGetParam( "server_name", NWCP_STRING, serverName ) != 0)
	{
		printf("NWCMGet of server_name failed, exiting.\n");
		exit( 1 );
	}
	printf("serverName is '%s'\n", serverName);

	serverType1 = UNIXWARE_TYPE;
	serverType2 = UNIXWARE_REMOTE_APP_TYPE;
	serverType3 = UNIXWARE_INSTALL_TYPE;
	mySocket1 = 0x0000;
	mySocket = 0x2345;

	printf("SAPAdvertiseMyServer Tests\n");
	printf("--------------------------------------------------\n");
	uid = getuid();
	if(uid == 0)
	{
		printf("1. Advertise UNIXWARE_TYPE, using SAP_ADVERTISE_FOREVER flag\n");
		ccode = SAPAdvertiseMyServer( serverType1, (uint8 *)serverName,
								mySocket1, SAP_ADVERTISE_FOREVER );
		if(ccode)
			printf("     SAPAdvertiseMyServer failed, ccode = '%d'\n", ccode);
		else
			printf("     SAPAdvertiseMyServer succeeded\n");

		printf("2. Advertise UNIXWARE_REMOTE_APP_TYPE, using SAP_ADVERTISE_FOREVER flag\n");
		ccode = SAPAdvertiseMyServer( serverType2, (uint8 *)serverName,
								mySocket, SAP_ADVERTISE_FOREVER );
		if(ccode)
			printf("     SAPAdvertiseMyServer failed, ccode = '%d'\n", ccode);
		else
			printf("     SAPAdvertiseMyServer succeeded\n");

		printf("3. Advertise UNIXWARE_INSTALL_TYPE, using SAP_ADVERTISE flag\n");
		ccode = SAPAdvertiseMyServer( serverType3, (uint8 *)serverName,
								mySocket, SAP_ADVERTISE );
		if(ccode)
			printf("     SAPAdvertiseMyServer failed, ccode = '%d'\n", ccode);
		else
			printf("     SAPAdvertiseMyServer succeeded\n");

	} else {
		printf("Trying unadvertise of permanent services by non-root\n");
		ccode = SAPAdvertiseMyServer( serverType1, (uint8 *)serverName,
								mySocket, SAP_STOP_ADVERTISING );
		if(ccode)
			printf("Expected error (-103), got '%d'\n", ccode);
		else
			printf("Expected error (-103), got no error\n");
		ccode = SAPAdvertiseMyServer( serverType2, (uint8 *)serverName,
								mySocket, SAP_STOP_ADVERTISING );
		if(ccode)
			printf("Expected error (-103), got '%d'\n", ccode);
		else
			printf("Expected error (-103), got no error\n");
		printf("Trying unadvertise of non-permanent service by non-root\n");
		ccode = SAPAdvertiseMyServer( serverType3, (uint8 *)serverName,
								mySocket, SAP_STOP_ADVERTISING );
		if(ccode)
			printf("Got unexpected error '%d'\n", ccode);
		else
			printf("Unadvertise succeeded\n");

		exit(0);
	}

	printf("\nUse the 'nwsaputil -q' command to view all permanently advertised servers.\n");
	printf("This test has permanently advertised service types 0x3e4 and 0x3e1.\n");
	printf("Use the 'nwsapinfo -l' command to view all locally advertised services.\n");
	printf("In addition to 0x3e1 and 0x3e4, this test has advertised type 0x3ee.\n");
	printf("\nPress <ENTER> to terminate advertising\n");
	getchar();

/*
**	Cancel advertising of all services
*/
	printf("\nCanceling SAP advertisements\n");
	printf("--------------------------------------------------\n");

	printf("1. Trying unadvertise of permanent services by root\n");
	ccode = SAPAdvertiseMyServer( serverType1, (uint8 *)serverName,
							mySocket, SAP_STOP_ADVERTISING );
	if(ccode)
		printf("     Unadvertise UNIXWARE_TYPE server.  Got unexpected error '%d'\n", ccode);
	else
		printf("     Unadvertise UNIXWARE_TYPE server succeeded\n");
	ccode = SAPAdvertiseMyServer( serverType2, (uint8 *)serverName,
							mySocket, SAP_STOP_ADVERTISING );
	if(ccode)
		printf("     Unadvertise UNIXWARE_REMOTE_APP_TYPE server.  Got unexpected error '%d'\n", ccode);
	else
		printf("     Unadvertise UNIXWARE_REMOTE_APP_TYPE succeeded\n");

	printf("\n2. Trying unadvertise of non-permanent service by root\n");
	ccode = SAPAdvertiseMyServer( serverType3, (uint8 *)serverName,
							mySocket, SAP_STOP_ADVERTISING );
	if(ccode)
		printf("     Unadvertise UNIXWARE_INSTALL_TYPE server.  Got unexpected error '%d'\n", ccode);
	else
		printf("     Unadvertise UNIXWARE_INSTALL_TYPE server succeeded\n");

}

