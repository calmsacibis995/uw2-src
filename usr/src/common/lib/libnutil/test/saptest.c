/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/test/saptest.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: saptest.c,v 1.3 1994/09/27 21:47:08 mark Exp $"

#include <stdio.h>
#include <fcntl.h>
#include <sys/sap_dos.h>
#include <tiuser.h>


int
main( int argc, char* argv[] )
{
	uint16		serverType, queryType;
	char		serverName[48];
	int			ccode, temp, i, j;
	uint8		serverSocket[2];
	SAP_ID_PACKET	servBuf[3];
	int			fileDesc;
	struct		t_info	info;
	struct		t_bind	bindRet;
	ipxAddr_t	retAddr;
	

/*
**	Query for nearest server, bad length for return buffer
*/
	printf("\nQueryServices Tests\n");
	printf("--------------------------------------------------------\n");
	printf("TEST 1: QueryServices - bad return buffer length\n");
	queryType = NEAREST_SERVER_REQUEST;
	serverType = FILE_SERVER_TYPE;
	ccode = QueryServices( queryType, serverType, 40, servBuf );
	if(ccode < 0)
		printf("QueryServices: expect error (-3), received %d.\n\n", ccode);
	else
		printf("QueryServices: Nearest Server incorrectly succeeded\n\n");

/*
**	Query for nearest server, bad serverType
*/
	printf("TEST 2: QueryServices - bad server type\n");
	queryType = NEAREST_SERVER_REQUEST;
	serverType = 0x1234;
	ccode = QueryServices( queryType, serverType, sizeof(SAP_ID_PACKET),
								servBuf );
	if(ccode < 0)
		printf("QueryServices: expect error (%d), received.\n\n", ccode);
	else
		printf("QueryServices: Nearest Server incorrectly succeeded\n\n");

/*
**	Query for nearest server
*/
	printf("TEST 3: QueryServices - Nearest Server Request, File server type\n");
	queryType = NEAREST_SERVER_REQUEST;
	serverType = FILE_SERVER_TYPE;
	ccode = QueryServices( queryType, serverType, sizeof(SAP_ID_PACKET),
							servBuf );
	if(ccode < 0)
		printf("QueryServices: Received error %d\n\n", ccode);
	else
	{
		printf("QueryServices: Nearest Server\n\n");
		printf("serverName is '%s'\n", servBuf[0].serverName);
		printf("serverType is '0x%x'\n", servBuf[0].serverType);
		printf("hops is '%d'\n", servBuf[0].hops);
		printf("NetAddr is '");
		for(i=0; i<IPX_NET_SIZE; i++)
			printf("%x ", servBuf[0].network[i]);
		printf("'\n");
		printf("NodeAddr is '");
		for(i=0; i<IPX_NODE_SIZE; i++)
			printf("%x ", servBuf[0].node[i]);
		printf("'\n");
		printf("socket is '%x'\n\n", servBuf[0].socket);
	}

/*
**	Query for General servers
*/
	printf("TEST 4: QueryServices - General Service Request, File server type\n");
	queryType = GENERAL_SERVICE_REQUEST;
	serverType = FILE_SERVER_TYPE;
	ccode = QueryServices( queryType, serverType, (3*sizeof(SAP_ID_PACKET)),
							servBuf );
	if(ccode < 0)
		printf("QueryServices: General: Received error %d\n", ccode);
	else
	{
		printf("QueryServices: General Services FILE (%d servers)\n", ccode);
		for(j=0; j<ccode; j++)
		{
			printf("serverName is '%s'\n", servBuf[j].serverName);
			printf("serverType is '0x%x'\n", servBuf[j].serverType);
			printf("hops is '%d'\n", servBuf[j].hops);
			printf("NetAddr is '");
			for(i=0; i<IPX_NET_SIZE; i++)
				printf("%x ", servBuf[j].network[i]);
			printf("'\n");
			printf("NodeAddr is '");
			for(i=0; i<IPX_NODE_SIZE; i++)
				printf("%x ", servBuf[j].node[i]);
			printf("'\n");
			printf("socket is '%x'\n", servBuf[j].socket);
		}
		printf("\n");
	}


/*
**	Query for General servers, Wild card type 
*/
	printf("TEST 5: QueryServices - General Service Request, All server types\n");
	queryType = GENERAL_SERVICE_REQUEST;
	serverType = ALL_SERVER_TYPE;
	ccode = QueryServices( queryType, serverType, (3*sizeof(SAP_ID_PACKET)),
							servBuf );
	if(ccode < 0)
		printf("QueryServices: Received error %d\n\n", ccode);
	else
	{
		printf("QueryServices: General Services WILD (%d new servers)\n", ccode);
		for(j=0; j<ccode; j++)
		{
			printf("serverName is '%s'\n", servBuf[j].serverName);
			printf("serverType is '0x%x'\n", servBuf[j].serverType);
			printf("hops is '%d'\n", servBuf[j].hops);
			printf("NetAddr is '");
			for(i=0; i<IPX_NET_SIZE; i++)
				printf("%x ", servBuf[j].network[i]);
			printf("'\n");
			printf("NodeAddr is '");
			for(i=0; i<IPX_NODE_SIZE; i++)
				printf("%x ", servBuf[j].node[i]);
			printf("'\n");
			printf("socket is '%x'\n", servBuf[j].socket);
		}
		printf("\n");
	}

/*
**	Get an IPX channel open, bind a socket and advertise it
*/

	fileDesc = t_open("/dev/ipx", O_RDWR, &info);
	if(fileDesc == -1)
	{
		printf("t_open error\n");
		exit(1);
	}
	bindRet.qlen =0;
	bindRet.addr.maxlen = sizeof (ipxAddr_t);
	bindRet.addr.buf = (char *)&retAddr;

/*
**  Bind to a dynamic socket for the service socket.
*/
	if (t_bind(fileDesc,NULL,&bindRet) == -1)
	{
		printf("t_bind failure\n");
		exit( 2 );
	}

	printf("\nAdvertiseService Tests\n");
	printf("--------------------------------------------------------\n");
	printf("Input for tests 6, 7, 8, 9\n--------------------------------\n");
	printf("Enter new server name: ");
	scanf("%s", serverName);

	printf("Enter new server type: ");
	scanf("%d", &temp);
	serverType = (uint16)temp;
	printf("\n");

/*
**	Do an invalid socket
*/
	printf("TEST 6: AdvertiseService - Invalid socket number\n");
	serverSocket[0] = 0;
	serverSocket[1] = 0;

	ccode = AdvertiseService( serverType, serverName, serverSocket );
	if(ccode)
		printf("AdvertiseService failed. expect error (-8), received '%d'\n\n", ccode);
	else
		printf("AdvertiseService succeeded unexpectedly.\n\n");

/*
**	Do a valid socket
*/
	serverSocket[0] = retAddr.sock[0];
	serverSocket[1] = retAddr.sock[1];

	printf("TEST 7: AdvertiseService - Valid with user input.\n");
	ccode = AdvertiseService( serverType, serverName, serverSocket );
	if(ccode)
		printf("AdvertiseService failed, ccode = '%d'\n\n", ccode);
	else
		printf("AdvertiseService succeeded\n\n");
	
/*
**	Query for nearest server
*/
	printf("TEST 8: QueryServices - Find nearest server of input type\n");
	queryType = NEAREST_SERVER_REQUEST;
	ccode = QueryServices( queryType, serverType, sizeof(SAP_ID_PACKET),
							servBuf );
	if(ccode < 0)
		printf("QueryServices: Received error %d\n\n", ccode);
	else
	{
		printf("serverName is '%s'\n", servBuf[0].serverName);
		printf("serverType is '0x%x'\n", servBuf[0].serverType);
		printf("hops is '%d'\n", servBuf[0].hops);
		printf("NetAddr is '");
		for(i=0; i<IPX_NET_SIZE; i++)
			printf("%x ", servBuf[0].network[i]);
		printf("'\n");
		printf("NodeAddr is '");
		for(i=0; i<IPX_NODE_SIZE; i++)
			printf("%x ", servBuf[0].node[i]);
		printf("'\n");
		printf("socket is '%x'\n\n", servBuf[0].socket);
	}

/*
**	Query for our serverType server
*/
	printf("TEST 9: QueryServices - Find servers of input type.\n");
	queryType = GENERAL_SERVICE_REQUEST;
	ccode = QueryServices( queryType, serverType, (3*sizeof(SAP_ID_PACKET)),
							servBuf );
	if(ccode < 0)
		printf("QueryServices: General: Received error %d\n\n", ccode);
	else
	{
		printf("QueryServices: General Services (Type = 0x%x) (%d servers)\n", serverType, ccode);
		for(j=0; j<ccode; j++)
		{
			printf("serverName is '%s'\n", servBuf[j].serverName);
			printf("serverType is '0x%x'\n", servBuf[j].serverType);
			printf("hops is '%d'\n", servBuf[j].hops);
			printf("NetAddr is '");
			for(i=0; i<IPX_NET_SIZE; i++)
				printf("%x ", servBuf[j].network[i]);
			printf("'\n");
			printf("NodeAddr is '");
			for(i=0; i<IPX_NODE_SIZE; i++)
				printf("%x ", servBuf[j].node[i]);
			printf("'\n");
			printf("socket is '%x'\n", servBuf[j].socket);
		}
		printf("\n");
	}

	printf("\nShutdownSAP Test\n");
	printf("--------------------------------------------------------\n");

	ccode = ShutdownSAP( );
	if(ccode)
		printf("ShutdownSAP failed, ccode = '%d'\n", ccode);
	else
		printf("ShutdownSAP succeeded\n");

	printf("\nSAPTEST completed.\n\n");

	t_close(fileDesc);

}
