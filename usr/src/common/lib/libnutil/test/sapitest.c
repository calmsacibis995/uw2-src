/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/test/sapitest.c	1.3"
#include <stdio.h>
#include <fcntl.h>
#include <limits.h>
#include <sys/sap_app.h>

ipxAddr_t	holdAddr;
int 		verbose = FALSE;
int 		runon = FALSE;
int 		errOnly = FALSE;
int			serverChanges = FALSE;
static uint32	timeStamp;
extern int	optind;
extern char	*optarg;

int GetNearTest( void );
int GetByAddr( void );
int GetByName( void );
int NotifyOfChange( void );
static void SigHandler( int );
static void PrintChanges( void );

main(int argc, char *argv[] )
{
	char readBuf[10];
	int	opt;
	char	*ptr;
	int	fd, err = 0, sapPID;
	char	ServerName[48];
	uint16	Socket, ServerType;
	ipxAddr_t	addr;
	SAPI	sBuf;
	int		i, j, index = 0;
	int		count = 0;
	int 	loop = 1;
	int 	forever = FALSE;


	while((opt = getopt(argc, argv, "eEfFvVcCi:I:?")) != (char)-1)
	{
		switch(opt)
		{
			case 'v':	/* Verbose */
			case 'V':
				verbose = TRUE;
				break;

			case 'c':	/* Continuous */
			case 'C':
				runon = TRUE;
				break;

			case 'e':	/* Error Report only, Continuous */
			case 'E':
				errOnly = TRUE;
				runon = TRUE;
				break;

			case 'f':	/* Run Forever */
			case 'F':
				forever = TRUE;
				runon = TRUE;
				loop = 1;
				break;

			case 'i':	/* Iterations */
			case 'I':
				loop = atoi( optarg, &ptr, 0);
				if((loop < 1) || (loop > INT_MAX))
				{
					fprintf(stderr,
						"Invalid iteration count, using default = 1\n");
					loop = 1;
				}
				break;

			case '?':
				err++;
				break;

			default:
				err++;
				break;
		}
	}

	for( ; optind < argc; optind++ )
	{
		err++;
		fprintf(stderr, "Unknown argument: '%s'\n", argv[optind]);
	}

	if(err)
	{
		fprintf(stderr, "sapitest: usage:  sapitest <-vVcCiIeEfF>\n");	
		fprintf(stderr, "          v,V:  Verbose mode\n");
		fprintf(stderr, "          c,C:  Continuous mode (no stops)\n");
		fprintf(stderr, "          i,I:  Iteration count (default = 1)\n");
		fprintf(stderr, "          e,E:  Print Errors only\n");
		fprintf(stderr, "          f,F:  Run Forever\n");
		exit( 0 );
	}
		
/*
**	First get a baseline SAP database timestamp.
*/
	err = SAPGetChangedServers(ALL_SERVER_TYPE, &index, &sBuf, 1,
								ULONG_MAX, &timeStamp );

	j = 1;
	for(i=1; i<=loop; j++)
	{
		if(!errOnly)
		{
			fprintf(stdout, "======================================================\n");
			fprintf(stdout, "               TEST  ITERATION  #%d\n", j);
			fprintf(stdout, "======================================================\n");
		}

/*
**	Call the SAPNotifyOfChange.  The first iteration should complete
**	successfully, subsequent calls should fail with a DUPLICATE callback
**	registration error.
*/
		if(!errOnly)
			fprintf(stdout, "\nCalling SAPNotifyOfChange\n");
		err = SAPNotifyOfChange(16, SigHandler, ALL_SERVER_TYPE);
		if(i==1)	
		{
			if(err)
				count += 1;
		} else {
			if((!err) || (err != -SAPL_DUPCALLBACK))
			{
				count += 1;
				if(verbose || errOnly)
					SAPPerror(err, "SAPNotifyOfChange: ");
			} else {
				if(verbose)
					SAPPerror(err, "SAPNotifyOfChange: Got expected error: ");
			}
		}

/*
**	Call the SAPGetNearestServer test.
*/
		err = GetNearTest();
		count += err;

/*
**	Call the SAPGetServerByName test.
*/
		err = GetByName();
		count += err;

/*
**	Call the SAPGetServerByAddr test.
*/
		err = GetByAddr();
		count += err;

/*
**	Call the SAPGetAllServer test.
*/
		err = GetAll();
		count += err;

/*
**	Call the SAPMapMemory and SAPUnmapMemory test.
*/
		err = MapUnmap();
		count += err;

		if(forever == FALSE)
			i++;
		if(errOnly)
		{
			printf(".");
			fflush(stdout);
		}

/*
**	Check to see if we got notification of server changes
*/
		if(serverChanges)
		{
			PrintChanges();
			serverChanges = FALSE;
			if(errOnly)
			{
				printf("!");
				fflush(stdout);
			}
		}
	}

	printf("\n======================================================\n");
	printf("SAP API Test Error Total is %d\n", count);
	printf("======================================================\n\n");
}


/*************************************************
**	SigHandler API Test
*************************************************/
static void SigHandler( int sig )
{
	serverChanges = TRUE;
	return;
}

static void PrintChanges()
{
	int err;
	uint32	newStamp;
	SAPI	serverBuf;
	int	index = 0;


	newStamp = 0;
	if(verbose)
	{
		printf("\n\nGot Notification of SAP SERVER changes\n");
		printf("-------------------------------------------\n");
	}

	while((err = SAPGetChangedServers(ALL_SERVER_TYPE, &index,
				&serverBuf, 1, timeStamp, &newStamp )) > 0)
	{
		if(verbose)
		{
			printf("Changed Server: %s, Type = 0x%04x, hops = '%d'\n",
						serverBuf.serverName, serverBuf.serverType,
						serverBuf.serverHops);
		}
	}
	if(err < 0)
	{
		if(verbose || errOnly)
			SAPPerror(err, "SigHandler: SAPGetChangedServer Error: ");
	}
	if(verbose)
	{
		printf("\n");
		sleep(2);
	}

	timeStamp = newStamp;
	
	return;
}

/*************************************************
**	SAPGetNearestServer API Test
*************************************************/
int GetNearTest()
{
	int	err, errCnt = 0, holdErr = 0;
	uint16	ServerType;
	SAPI	sapResp;


	if(!errOnly)
	{
		fprintf(stdout, "\nSAPGetNearestServer Tests\n");
		fprintf(stdout, "------------------------------------------------------\n");
	}

/*
**	Try a SAPGetNearestServer with valid parameters
*/
	if(!errOnly)
		fprintf(stdout, "TEST 1: Get a file server type (0x0004)\n");
	ServerType = FILE_SERVER_TYPE;

	err = SAPGetNearestServer( ServerType, &sapResp ); 

	if(verbose)
		fprintf(stdout, "Nearest server reply was: %d, %s\n", err, sapResp.serverName);

	if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetNearestServer 1: Unexpected error '%d'\n", err);
		errCnt++;
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

	memset((char *)&sapResp, 0, sizeof(SAPI));

/*
**	Try a SAPGetNearestServer with invalid type
*/
	if(!errOnly)
		fprintf(stdout, "TEST 2: Try to get a bad server type (0x0414)\n");
	ServerType = 0x0414;

	err = SAPGetNearestServer( ServerType, &sapResp ); 
	if(verbose)
		fprintf(stdout, "Nearest server reply was: %d, expected ( 0 )\n", err);
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "ServerName was '%s'\n", sapResp.serverName);
		errCnt++;
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetNearestServer 2: Unexpected error '%d'\n", err);
		errCnt++;
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetNearestServer with ALL_SERVER type
*/
	if(!errOnly)
		fprintf(stdout, "TEST 3: Try to get server with  ALL_SERVER type (0xFFFF)\n");
	ServerType = ALL_SERVER_TYPE;

	err = SAPGetNearestServer( ServerType, &sapResp ); 
	if(verbose)
		fprintf(stdout, "Nearest server reply was: %d, expected ( 0 )\n", err);
	if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetNearestServer 3: Unexpected error '%d'\n", err);
		errCnt++;
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
	} else { 
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

	if(!errOnly)
	{
		if(holdErr)
			fprintf(stdout, "%d Errors found\n\n", holdErr);
		else
			fprintf(stdout, "No Errors found\n\n");
	}

	if(!runon)
	{
		fprintf(stdout, "Press <ENTER> to continue. ");
		while((err = getchar()) != 0x0a);
	}

	return(holdErr);
}


/*************************************************
**	SAPGetServerByAddr API Test
*************************************************/
int GetByAddr()
{
	SAPI	sapResp[4];
	ipxAddr_t	addr;
	uint16	ServerType;
	int	err, i, j, index;
	int 	holdCount, count = 0;
	int 	holdErr = 0, errCnt = 0;


	if(!errOnly)
	{
		fprintf(stdout, "\nSAPGetServerByAddr Tests  FILE_SERVER type\n");
		fprintf(stdout, "------------------------------------------------------\n");
	}

/*
**	Try a SAPGetServerByAddr of SAPADVERT_TEST, FILE_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 1: Get FILE_SERVER at SAPADVERT_TEST's address\n");
	ServerType = FILE_SERVER_TYPE;
	index = 0;

	err = SAPGetServerByAddr( &holdAddr, ServerType, &index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got FILE_SERVER (type 0x0004): %s\n", sapResp[0].serverName);
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stderr, "SAPGetServerByAddr 1: Got error number '%d'\n", err);
		errCnt++;
	}
	else
	{
		if(verbose)
			fprintf(stdout, "Got NO FILE_SERVER reply.\n");
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByAddr of SAPADVERT_TEST, DIRECTORY_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 2: Get DIRECTORY_SERVER at SAPADVERT_TEST's address\n");

	ServerType = DIRECTORY_SERVER_TYPE;
	index = 0;

	err = SAPGetServerByAddr( &holdAddr, ServerType, &index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got DIRECTORY_SERVER (type 0x0278): %s\n", sapResp[0].serverName);
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByAddr 2: Got error number '%d'\n", err);
		errCnt++;
	}
	else
	{
		if(verbose)
			fprintf(stdout, "Got NO DIRECTORY_SERVER reply.\n");
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByAddr of SAPADVERT_TEST, PRINT_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 3: Get PRINT_SERVER at SAPADVERT_TEST's address\n");

	ServerType = PRINT_SERVER_TYPE;
	index = 0;

	err = SAPGetServerByAddr( &holdAddr, ServerType, &index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got PRINT_SERVER (type 0x0047): %s\n", sapResp[0].serverName);
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByAddr 3: Got error number '%d'\n", err);
		errCnt++;
	}
	else
	{
		if(verbose)
			fprintf(stdout, "Got NO PRINT_SERVER reply.\n");
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByAddr of SAPADVERT_TEST, Invalid type
*/
	if(!errOnly)
		fprintf(stdout, "TEST 4: Get ACCESS_SERVER at SAPADVERT_TEST's address (Invalid)\n");

	ServerType = ACCESS_SERVER_TYPE;
	index = 0;

	err = SAPGetServerByAddr( &holdAddr, ServerType, &index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got ACCESS_SERVER (type 0x0098): %s, No hit expected!\n",
						sapResp[0].serverName);
		errCnt++;
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByAddr 4: Got error number '%d'\n", err);
		errCnt++;
	}
	else
	{
		if(verbose)
			fprintf(stdout, "Got NO ACCESS_SERVER replies, as expected.\n");
	}

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Get ALL_SERVERS, all at once
*/
	if(!errOnly)
		fprintf(stdout, "TEST 5: Get ALL_SERVER at SAPADVERT_TEST's address, 4 at once\n");

	ServerType = ALL_SERVER_TYPE;
	index = 0;
	count = 0;
	j = 1;
	do
	{
		err = SAPGetServerByAddr( &holdAddr, ServerType, &index, sapResp, 4 ); 
		if(err < 0)
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetServerByAddr 5: Got error number '%d'\n", err);
			errCnt++;
			break;
		}
		for(i=0; i<err; i++)
		{
			if(verbose)
			{	
				if(i == 0)
					fprintf(stdout, "Call #%d: ", j++);
				else
					fprintf(stdout, "         ");
				fprintf(stdout, "General service reply %d of %d is: %s, type is %04x\n",
					i+1, err, sapResp[i].serverName, sapResp[i].serverType);
			}
		}
		count += err;
	} while(err == 4);
	if(verbose)
		fprintf(stdout, "Got %d replies (SAPGetServerByAddr).\n", count);
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

	holdCount = count;

/*
**	Get ALL_SERVERS, one at a time.
*/
	if(!errOnly)
		fprintf(stdout, "TEST 6: Get ALL_SERVER at SAPADVERT_TEST's address, one at a time\n");

	ServerType = ALL_SERVER_TYPE;
	index = 0;
	count = 0;
	j = 1;
	do
	{
		err = SAPGetServerByAddr( &holdAddr, ServerType, &index, sapResp, 1 ); 
		if(err < 0)
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetServerByAddr 6: Got error number '%d'\n", err);
			errCnt++;
			break;
		}
		for(i=0; i<err; i++)
		{
			if(verbose)
			{
				if(i == 0)
					fprintf(stdout, "Call #%d: ", j++);
				else
					fprintf(stdout, "         ");
				fprintf(stdout, "General service reply %d of %d is: %s, type is %04x\n",
					i+1, err, sapResp[i].serverName, sapResp[i].serverType);
			}
		}
		count += err;
	} while(err == 1);
	if(verbose)
		fprintf(stdout,  "Got %d replies (SAPGetServerByAddr).\n", count);

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

	if(!errOnly)
	{
		if(holdErr)
			fprintf(stdout, "%d Errors found\n\n", holdErr);
		else
			fprintf(stdout, "No Errors found\n\n");
	}

	if(!runon)
	{
		fprintf(stdout, "Press <ENTER> to continue. ");
		while((err = getchar()) != 0x0a);
	}

	return( holdErr );
}


/*************************************************
**	SAPGetServerByName API Test
*************************************************/
int GetByName()
{
	SAPI	sapResp[4];
	uint16	ServerType;
	char	ServerName[48];
	int 	err, i, j, index = 0;
	int 	holdCount, count = 0;
	int 	errCnt = 0, holdErr = 0;

/*
**	Do full name testing
*/
	if(!errOnly)
	{
		fprintf(stdout, "\nSAPGetServerByName Tests  Full Name (SAPADVERT_TEST)\n");
		fprintf(stdout, "------------------------------------------------------\n");
	}

/*
**	Try a SAPGetServerByName, type = FILE_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 1: Get FILE_SERVER by name, using (SAPADVERT_TEST)\n");
	strcpy(ServerName, "SAPADVERT_TEST");
	ServerType = FILE_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got FILE_SERVER (type 0x0004): %s\n", sapResp[0].serverName);
		memcpy((char *)&holdAddr, (char *)&sapResp[0].serverAddress, sizeof(ipxAddr_t));	
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 1: Got error number '%d'\n", err);
		errCnt++;
	}
	else
	{
		if(verbose)
			fprintf(stdout, "Got NO FILE_SERVER replies (using name 'SAPADVERT_TEST').\n");
	}

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByName, type = DIRECTORY_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 2: Get DIRECTORY_SERVER by name, using (SAPADVERT_TEST)\n");

	ServerType = DIRECTORY_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got DIRECTORY_SERVER (type 0x0278): %s\n", sapResp[0].serverName);
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 2; Got error number '%d'\n", err);
		errCnt++;
	} else {
		if(verbose)
			fprintf(stdout, "Got NO DIRECTORY_SERVER replies (using name 'SAPADVERT_TEST').\n");
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByName, type = PRINT_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 3: Get PRINT_SERVER by name, using (SAPADVERT_TEST)\n");

	ServerType = PRINT_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got PRINT_SERVER (type 0x0047): %s\n", sapResp[0].serverName);
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 3: Got error number '%d'\n", err);
		errCnt++;
	} else {
		if(verbose)
			fprintf(stdout, "Got NO PRINT_SERVER replies (using name 'SAPADVERT_TEST').\n");
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByName, type = ACCESS_SERVER_TYPE
**	sapadvert does not advertise one, should get no hits.
*/
	if(!errOnly)
		fprintf(stdout, "TEST 4: Get ACCESS_SERVER by name, using (SAPADVERT_TEST)\n");

	ServerType = ACCESS_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got ACCESS_SERVER (type 0x0098): %s, No hit expected!\n",
						sapResp[0].serverName);
		errCnt++;
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 4: Got error number '%d'\n", err);
		errCnt++;
	} else {
		if(verbose)
			fprintf(stdout, "Got NO ACCESS_SERVER replies (using SAPADVERT_TEST), as expected.\n");
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByName, type = ALL_SERVER
**	Get all servers at one time.
*/
	if(!errOnly)
		fprintf(stdout, "TEST 5: Get ALL_SERVER by name, Get all at once\n");

	ServerType = ALL_SERVER_TYPE;
	index = 0;
	count = 0;
	j = 1;
	do
	{
		err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
		if(err < 0)
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetServerByName 5: Got error number '%d'\n", err);
			errCnt++;
			break;
		}
		for(i=0; i<err; i++)
		{
			if(verbose)
			{
				if(i == 0)
					fprintf(stdout, "Call #%d: ", j++);
				else
					fprintf(stdout, "         ");
				fprintf(stdout, "General service reply %d of %d is: %s, type is %04x\n",
						i+1, err, sapResp[i].serverName, sapResp[i].serverType);
			}
		}
		count += err;
	} while(err == 4);
	if(verbose)
		fprintf(stdout, "Got %d SAPGetServerByName replies (using name 'SAPADVERT_TEST').\n", count);

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}
	holdCount = count;

/*
**	Try a SAPGetServerByName, type = ALL_SERVER
**	Get servers one at a time
*/
	if(!errOnly)
		fprintf(stdout, "TEST 6: Get ALL_SERVER by name, Get one at a time.\n");

	ServerType = ALL_SERVER_TYPE;
	index = 0;
	count = 0;
	j = 1;
	do
	{
		err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 1 ); 
		if(err < 0)
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetServerByName 6: Got error number '%d'\n", err);
			errCnt++;
			break;
		}
		for(i=0; i<err; i++)
		{
			if(verbose)
			{
				if(i == 0)
					fprintf(stdout, "Call #%d: ", j++);
				else
					fprintf(stdout, "         ");
				fprintf(stdout, "General service reply %d of %d is: %s, type is %04x\n",
						i+1, err, sapResp[i].serverName, sapResp[i].serverType);
			}
		}
		count += err;
	} while(err == 1);
	if(verbose)
		fprintf(stdout, "Got %d SAPGetServerByName replies (using name 'SAPADVERT_TEST').\n", count);

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Do wildcard testing
**	Try a SAPGetServerByName, type = FILE_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 7: Get FILE_SERVER by name, using wildcard (SAPAD*)\n");

	strcpy(ServerName, "SAPAD*");
	ServerType = FILE_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got FILE_SERVER (type 0x0004): %s\n", sapResp[0].serverName);
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 7: Got error number '%d'\n", err);
		errCnt++;
	} else {
		if(verbose)
			fprintf(stdout, "Got NO FILE_SERVER replies (using name 'SAPAD*').\n");
	}

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByName, type = DIRECTORY_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 8: Get DIRECTORY_SERVER by name, using wildcard (SAPAD*)\n");

	ServerType = DIRECTORY_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got DIRECTORY_SERVER (type 0x0278): %s\n", sapResp[0].serverName);
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 8: Got error number '%d'\n", err);
		errCnt++;
	} else {
		if(verbose)
			fprintf(stdout, "Got NO DIRECTORY_SERVER replies (using name 'SAPAD*').\n");
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByName, type = PRINT_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 9: Get PRINT_SERVER by name, using wildcard (SAPAD*)\n");

	ServerType = PRINT_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got PRINT_SERVER (type 0x0047): %s\n", sapResp[0].serverName);
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 9: Got error number '%d'\n", err);
		errCnt++;
	} else {
		if(verbose)
			fprintf(stdout, "Got NO PRINT_SERVER replies (using name 'SAPAD*').\n");
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByName, type = ACCESS_SERVER_TYPE
**	sapadvert does not advertise one, should get no hits.
*/
	if(!errOnly)
		fprintf(stdout, "TEST 10: Get ACCESS_SERVER by name, using wildcard (SAPAD*)\n");

	ServerType = ACCESS_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose)
			fprintf(stdout, "Got PRINT_SERVER (type 0x0047): %s, No hit expected!\n",
						sapResp[0].serverName);
		errCnt++;
	}
	else if(err < 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 10: Got error number '%d'\n", err);
		errCnt++;
	} else {
		if(verbose)
			fprintf(stdout, "Got NO PRINT_SERVER replies (using name 'SAPAD*'), as expected.\n");
	}

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
/*
**	Try a SAPGetServerByName, type = ALL_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 11: Get ALL_SERVER by name, using wildcard (SAPAD*)\n");

	ServerType = ALL_SERVER_TYPE;
	index = 0;
	count = 0;
	do
	{
		err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
		if(err < 0)
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetServerByName 11: Got error number '%d'\n", err);
			errCnt++;
			break;
		}
		for(i=0; i<err; i++)
		{
			if(verbose)
				fprintf(stdout, "General service reply %d of %d is: %s, type is %04x\n",
					i+1, err, sapResp[i].serverName, sapResp[i].serverType);
		}
		count += err;
	} while(err == 4);

	if(verbose)
		fprintf(stdout, "Got %d SAPGetServerByName replies (using name 'SAPAD*').\n", count);

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try error conditions
**	Try a SAPGetServerByName, Too long name. 
*/
	if(!errOnly)
		fprintf(stdout, "TEST 12: Get FILE_SERVER with too long name.\n");

	strcpy(ServerName, "1234567890123456789012345678901234567890123456789");
	ServerType = FILE_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 12: Got FILE_SERVER (type 0x0004): %s, expected error (-2).\n",
					sapResp[0].serverName);
		errCnt++;
	}
	else if(err < 0)
	{
		if(err == -2)
		{
			if(verbose)
				fprintf(stdout, "Got expected error number (-2).\n", err);
		}
		else
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetServerByName 12: Got error number '%d', expected error (-2).\n", err);
			errCnt++;
		}
	} else {
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 12: Got NO FILE_SERVER replies, expected error (-2).\n");
		errCnt++;
	}

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetServerByName, Too short name. 
*/
	if(!errOnly)
		fprintf(stdout, "TEST 13: Get FILE_SERVER with too short name.\n");

	strcpy(ServerName, "*");
	ServerType = FILE_SERVER_TYPE;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 13: Got FILE_SERVER (type 0x0004): %s, expected error (-2).\n",
					sapResp[0].serverName);
		errCnt++;
	}
	else if(err < 0)
	{
		if(err == -2)
		{
			if(verbose)
				fprintf(stdout, "Got expected error number (-2).\n", err);
		} else {
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetServerByName 13: Got error number '%d', expected error (-2).\n", err);
			errCnt++;
		}
	} else {
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 13: Got NO FILE_SERVER replies, expected error (-2).\n");
		errCnt++;
	}
	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Invalid serverType
*/
	if(!errOnly)
		fprintf(stdout, "TEST 14: Get server using invalid serverType.\n");

	strcpy(ServerName, "SAPAD*");
	ServerType = 0;
	index = 0;
	err = SAPGetServerByName( (uint8 *)ServerName, ServerType,
						&index, sapResp, 4 ); 
	if(err > 0)
	{
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 14: Got server: %s, expected error (-1).\n", sapResp[0].serverName);
		errCnt++;
	}
	else if(err < 0)
	{
		if(err == -1)
		{
			if(verbose)
				fprintf(stdout, "Got expected error number (-1).\n", err);
		} else {
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetServerByName 14: Got error number '%d', expected error (-1).\n", err);
			errCnt++;
		}
	} else {
		if(verbose || errOnly)
			fprintf(stdout, "SAPGetServerByName 14: Got no replies, expected error (-1).\n");
		errCnt++;
	}

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

	if(!errOnly)
	{
		if(holdErr)
			fprintf(stdout,  "%d Errors found\n\n", holdErr);
		else
			fprintf(stdout, "No Errors found\n\n");
	}

	if(!runon)
	{
		fprintf(stdout, "Press <ENTER> to continue. ");
		while((err = getchar()) != 0x0a);
	}

	return(holdErr);
}

/*************************************************
**	SAPGetAllServers API Test
*************************************************/
int GetAll()
{
	SAPI	sapResp[4];
	uint16	ServerType;
	int 	err, i, index = 0;
	int 	count = 0;
	int 	holdErr = 0, errCnt = 0;

	if(!errOnly)
	{
		fprintf(stdout, "\nSAPGetAllServers Tests\n");
		fprintf(stdout, "------------------------------------------------------\n");
	}

/*
**	Try a SAPGetAllServers, type = FILE_SERVER
*/
	if(!errOnly)
		fprintf(stdout, "TEST 1: Get all FILE_SERVER types, 4 at a time.\n");

	ServerType = FILE_SERVER_TYPE;
	index = 0;
	count = 0;
	do
	{
		err = SAPGetAllServers( ServerType, &index, sapResp, 4 ); 
		if(err < 0)
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetAllServers 1: Got error number '%d'\n\n", err);
			errCnt++;
			break;
		}
		for(i=0; i<err; i++)
		{
			if(verbose)
				fprintf(stdout, "FILE_SERVER: %s\n", sapResp[i].serverName);
		}
		count += err;
	} while(err == 4);

	if(verbose)
		fprintf(stdout, "Got %d SAPGetAllServers FILE_SERVER replies.\n\n", count);

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetAllServers, type = UNIXWARE_TYPE
*/
	if(!errOnly)
		fprintf(stdout, "TEST 2: Get all UNIXWARE_TYPE types, 4 at a time.\n");

	ServerType = UNIXWARE_TYPE;
	index = 0;
	count = 0;
	do
	{
		err = SAPGetAllServers( ServerType, &index, sapResp, 4 ); 
		if(err < 0)
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetAllServers 2: Got error number '%d'\n\n", err);
			errCnt++;
			break;
		}
		for(i=0; i<err; i++)
		{
			if(verbose)
				fprintf(stdout, "UNIXWARE_TYPE: %s\n", sapResp[i].serverName);
		}
		count += err;
	} while(err == 4);

	if(verbose)
		fprintf(stdout, "Got %d SAPGetAllServers UNIXWARE_TYPE replies.\n\n", count);

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

/*
**	Try a SAPGetAllServers, type = ALL_SERVER_TYPE
*/
	if(!errOnly)
		fprintf(stdout, "TEST 3: Get all ALL_SERVER_TYPE types, 4 at a time.\n");

	ServerType = ALL_SERVER_TYPE;
	index = 0;
	count = 0;
	do
	{
		err = SAPGetAllServers( ServerType, &index, sapResp, 4 ); 
		if(err < 0)
		{
			if(verbose || errOnly)
				fprintf(stdout, "SAPGetAllServers 3: Got error number '%d'\n\n", err);
			errCnt++;
			break;
		}
		for(i=0; i<err; i++)
		{
			if(verbose)
				fprintf(stdout, "ALL_SERVER_TYPE: Type: 0x%04x, %s\n",
					sapResp[i].serverType, sapResp[i].serverName);
		}
		count += err;
	} while(err == 4);

	if(verbose)
		fprintf(stdout, "Got %d SAPGetAllServers ALL_SERVER_TYPE replies.\n\n", count);

	if(errCnt)
	{
		if(!errOnly)
			fprintf(stdout, "          STATUS = FAILED\n");
		holdErr += errCnt;
		errCnt = 0;
	} else {
		if(!errOnly)
			fprintf(stdout, "          STATUS = PASSED\n");
	}

	if(!errOnly)
	{
		if(holdErr)
			fprintf(stdout, "%d Errors found\n\n", holdErr);
		else
			fprintf(stdout, "No Errors found\n\n");
	}

	if(!runon)
	{
		fprintf(stdout, "Press <ENTER> to continue. ");
		while((err = getchar()) != 0x0a);
	}

	return(holdErr);
}

/*************************************************
**	SAPMapMemory and SAPUnmapMemory API Test
*************************************************/
int MapUnmap()
{
	SAPI	sapResp[4];
	uint16	ServerType;
	int 	err, i, index = 0;
	int 	count = 0;
	int 	errCnt = 0;

	if(!errOnly)
	{
		fprintf(stdout, "\nSAPMapMemory and SAPUnmapMemory Tests\n");
		fprintf(stdout, "------------------------------------------------------\n");
	}

/*
**	Map memory, then get all servers.
*/
	if(!errOnly)
		fprintf(stdout, "TEST 1: Map Memory, get ALL_SERVER_TYPE types, Unmap Memory\n");

	err = SAPMapMemory();
	if(err < 0)
	{
		if(verbose)
			fprintf(stdout, "SAPMapMemory error number '%d'\n\n", err);
		errCnt++;
	}
	
/*
**	Try a SAPGetAllServers, type = ALL_SERVER_TYPE
*/
	if(!errCnt)
	{
		ServerType = ALL_SERVER_TYPE;
		index = 0;
		count = 0;
		do
		{
			err = SAPGetAllServers( ServerType, &index, sapResp, 4 ); 
			if(err < 0)
			{
				if(verbose || errOnly)
					fprintf(stdout, "MapMem, SAPGetAllServers: Got error number '%d'\n\n", err);
				errCnt++;
				break;
			}
			for(i=0; i<err; i++)
			{
				if(verbose)
					fprintf(stdout, "ALL_SERVER_TYPE: Type: 0x%04x, %s\n",
						sapResp[i].serverType, sapResp[i].serverName);
			}
			count += err;
		} while(err == 4);
		if(verbose)
			fprintf(stdout, "Got %d SAPGetAllServers ALL_SERVER_TYPE replies.\n\n", count);
	}

	SAPUnmapMemory();

	if(!errOnly)
	{
		if(errCnt)
			fprintf(stdout, "%d Errors found\n\n", errCnt);
		else
			fprintf(stdout, "No Errors found\n\n");
	}

	if(!runon)
	{
		fprintf(stdout, "Press <ENTER> to continue. ");
		while((err = getchar()) != 0x0a);
	}

	return(errCnt);
}
