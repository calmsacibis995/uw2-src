/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/utils/stats/sapinfo.c	1.13"
#ident	"$Id: sapinfo.c,v 1.12.2.1 1994/10/13 18:12:11 ericw Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include "nwmsg.h"
#include <limits.h>
#include <errno.h>
#include <sys/types.h>
#include "nwconfig.h"
#include "npsmsgtable.h"
#include <sys/sap_app.h>

extern char *optarg;
extern int optind, opterr, optopt;

#define SERVER_ARRAY_SIZE 200	/* Size of types arrary */

static int copt = 0;	/* Servers changed since num */
static uint32 cnum = 0;	/* Input Revision Stamp */
static int Copt = 0;	/* Servers changed since num, */
						/* wait for additional notifications, use INT to quit */
static int dopt = 0;	/* Display server information */
static int fopt = 0;	/* 12 char name leading */
static int Fopt = 0;	/* full name trailing */
static int iopt = 0;	/* Get Sap statistical information */
static int lopt = 0;	/* Display local servers */
static int Lopt = 0;	/* Display LAN information */
static int nopt = 0;	/* Get nearest server */
static uint32 nnum = 0;	/* server type */
static int sopt = 0;	/* Server by name */
static uint8 *sname;	/* Server name */
static int topt = 0;	/* Summary of Types */
static int Topt = 0;	/* Set type for -s option */
static uint32 Tnum;		/* Type from -T option */
static int xopt = 0;	/* ipx address */
static int zopt = 0;	/* no verbiage option */

/******************************************************************/
void
ShowStats( void)
{
	int	total;
	SAPD	SapData;
	time_t	Chours;
	time_t	Cminutes;
	time_t	Cseconds;
	int		status;

	if( (status = SAPStatistics( &SapData)) < 0) {
		fprintf(stderr, MsgGetStr(A_SAP_STATS_UNAVAIL));
		SAPPerror(status, "");
		return;
		/* NOTREACHED */
	}

	Cseconds = time(NULL) - SapData.StartTime;
	Chours = Cseconds / 3600;
	Cseconds = Cseconds - (Chours * 3600);
	Cminutes = Cseconds / 60;
	Cseconds = Cseconds - (Cminutes * 60);
	printf(MsgGetStr(A_TIME_SAP_ACTIVE), Chours, Cminutes, Cseconds);

	if( (kill( SapData.SapPid, 0) != 0) && (errno != EPERM)) {
		printf(MsgGetStr(A_SAP_INACTIVE), SapData.SapPid);
	} else {
		printf(MsgGetStr(A_SAP_ACTIVE), SapData.SapPid);
	}

	if( SapData.ServerPoolIdx == 0)
		total = SapData.ConfigServers;
	else
		total = SapData.ServerPoolIdx -1;

	printf(MsgGetStr(A_KNOWN_SERVERS), total);
	printf(MsgGetStr(A_UNUSED_SERV_ENT), SapData.ConfigServers - total);
	printf(MsgGetStr(A_KNOWN_LANS), SapData.Lans);
	printf(MsgGetStr(A_REV_STAMP), SapData.RevisionStamp);
	printf(MsgGetStr(A_TOTAL_SAP_RCVD), SapData.TotalInSaps);
	printf(MsgGetStr(A_GSQ_RCVD), SapData.GSQReceived);
	printf(MsgGetStr(A_GSR_RCVD), SapData.GSRReceived);
	printf(MsgGetStr(A_NSQ_RCVD), SapData.NSQReceived);
	printf(MsgGetStr(A_ADVERT_REQS), SapData.SASReceived);
	printf(MsgGetStr(A_NOTIFY_REQS), SapData.SNCReceived);
	printf(MsgGetStr(A_GET_SHMEM_REQS), SapData.GSIReceived);
	printf(MsgGetStr(A_NOT_NEIGHBOR), SapData.NotNeighbor);
	printf(MsgGetStr(A_ECHO_OUTPUT), SapData.EchoMyOutput);
	printf(MsgGetStr(A_BAD_SIZE), SapData.BadSizeInSaps);
	printf(MsgGetStr(A_BAD_SAP_SOURCE), SapData.BadSapSource);
	printf(MsgGetStr(A_TOTAL_OUT_SAPS), SapData.TotalOutSaps);
	printf(MsgGetStr(A_NSR_SENT), SapData.NSRSent);
	printf(MsgGetStr(A_GSR_SENT), SapData.GSRSent);
	printf(MsgGetStr(A_GSQ_SENT), SapData.GSQSent);
	printf(MsgGetStr(A_SAS_ACKS), SapData.SASAckSent);
	printf(MsgGetStr(A_SAS_NACKS), SapData.SASNackSent);
	printf(MsgGetStr(A_SNC_ACKS), SapData.SNCAckSent);
	printf(MsgGetStr(A_SNC_NACKS), SapData.SNCNackSent);
	printf(MsgGetStr(A_GSI_ACKS), SapData.GSIAckSent);
	printf(MsgGetStr(A_BAD_DEST_OUT), SapData.BadDestOutSaps);
	printf(MsgGetStr(A_SRV_ALLOC_FAILED), SapData.SrvAllocFailed);
	printf(MsgGetStr(A_MALLOC_FAILED), SapData.MallocFailed);
	printf(MsgGetStr(A_IN_RIP_SAPS), SapData.TotalInRipSaps);
	printf(MsgGetStr(A_BAD_RIP_SAPS), SapData.BadRipSaps);
	printf(MsgGetStr(A_RIP_SERVER_DOWN), SapData.RipServerDown);
	printf(MsgGetStr(A_NOTIFY_PROCS), SapData.ProcessesToNotify);
	printf(MsgGetStr(A_NOTIFYS_SENT), SapData.NotificationsSent);
	return;
}

/******************************************************************/
static void
PrintServerEntry( SAPIP Server)
{
	int i;
	char serverName[NWMAX_SERVER_NAME_LENGTH];
	ipxAddr_t *ipxAddr;

	if( fopt ) {
		strcpy( serverName, (char *)Server->serverName);
		if( strlen((char *)Server->serverName) > (size_t)12)
			serverName[12] = '\0';
		fprintf(stdout, MsgGetStr(A_SERVER_NAME), serverName);
	}

	if( !zopt)
		fprintf(stdout, MsgGetStr(A_SERVER_TYPE), Server->serverType);
	if( !zopt)
		fprintf(stdout,  MsgGetStr(A_SERVER_HOPS), Server->serverHops);

	if( !zopt)
		fprintf(stdout,  MsgGetStr(A_SERVER_ADDR), " Address: ");
	ipxAddr = (ipxAddr_t *)&Server->serverAddress;
	fprintf(stdout, MsgGetStr(A_HEX_ADDR));
	for (i=0; i<4; i++)
		fprintf(stdout, MsgGetStr(A_HEX_BYTE), ipxAddr->net[i]);
	if( !zopt)
		fprintf(stdout, MsgGetStr(A_SPACE));
	else
		fprintf(stdout, MsgGetStr(A_HEX_ADDR));
	for (i=0; i<6; i++)
		fprintf(stdout, MsgGetStr(A_HEX_BYTE), ipxAddr->node[i]);
	if( !zopt)
		fprintf(stdout, MsgGetStr(A_SPACE));
	else
		fprintf(stdout, MsgGetStr(A_HEX_ADDR));
	for (i=0; i<2; i++)
		fprintf(stdout, MsgGetStr(A_HEX_BYTE), ipxAddr->sock[i]);
	if( Fopt )
		fprintf(stdout, MsgGetStr(A_STRING), Server->serverName);
	fprintf(stdout, MsgGetStr(A_NEWLINE));
}

/******************************************************************/
static void
GetAllServers( void)
{
	SAPI	Server;
	int		ServerEntry = 0;
	uint32	numServers = 0;
	uint16	type;
	int		status;
	SAPD	SapData;

	if( lopt) {
		if( (status = SAPStatistics( &SapData)) < 0) {
			fprintf(stderr, MsgGetStr(A_SAP_STATS_UNAVAIL));
			SAPPerror(status, "");
			return;
			/* NOTREACHED */
		}
	}

	if( Topt)
		type = (uint16)Tnum;
	else
		type = ALL_SERVER_TYPE;
	
	while( (status = SAPGetAllServers( type, &ServerEntry, &Server, 1)) > 0) {
		
		if( lopt) {
			if( ! IPXCMPNET( SapData.MyNetworkAddress,&Server.serverAddress) ||
					! IPXCMPNODE( &SapData.MyNetworkAddress[IPX_NET_SIZE],
					&Server.serverAddress.node)) {
				continue;
			}
			numServers++;
		} else {
			numServers++;
		}
		PrintServerEntry( &Server);
	}
	if( status < 0) {
		fprintf(stderr, MsgGetStr(A_GET_ALL_SERV_FAIL));
		SAPPerror( status, "");
		return;
	}

	if( ! zopt) {
		if( lopt)
			fprintf(stdout, MsgGetStr(A_NUM_LOC_SERVERS), numServers);
		else
			fprintf(stdout,MsgGetStr(A_NUM_SERVERS), numServers);
	}
	return;
}

/******************************************************************/
static void
DumpSapTypes( void)
{
	SAPI	Server;
	int		ServerEntry = 0;
	int		i;
	uint16	typeCount[SERVER_ARRAY_SIZE];
	uint16	serverTypes[SERVER_ARRAY_SIZE];
	int		serverTypesCounted =0;
	uint32	numServers = 0;
	uint16	type;
	int		status;

	if( Topt)
		type = (uint16)Tnum;
	else
		type = ALL_SERVER_TYPE;

	while( (status = SAPGetAllServers( type, &ServerEntry, &Server, 1)) > 0) {

		numServers++;
		
		for (i=0; i<serverTypesCounted; i++)
			if (serverTypes[i] == Server.serverType)
				break;

		if (Server.serverType == serverTypes[i])
			typeCount[i]++;
		else {
			if( serverTypesCounted <  (SERVER_ARRAY_SIZE - 2)) {
				serverTypes[serverTypesCounted] = Server.serverType;
				typeCount[serverTypesCounted] = 1;
				serverTypesCounted++;
			}
		}
	}

	if( status < 0) {
		fprintf(stderr, MsgGetStr(A_GET_ALL_SERV_FAIL));
		SAPPerror( status, "");
		return;
	}

	for (i=0; i<serverTypesCounted; i++) {
		if( !zopt)
			fprintf(stdout, MsgGetStr(A_SERVER_TYPE), serverTypes[i]);
		if( !zopt)
			fprintf(stdout, MsgGetStr(A_TYPE_COUNT), typeCount[i]);
	}

	if( !zopt)
		fprintf(stdout, MsgGetStr(A_SERVER_AND_TYPE), numServers, serverTypesCounted);
	return;
}

/******************************************************************/
/*
**	All we really want to do is wake up from pause
*/
/*ARGSUSED*/
void
Wakeup( int signal) {
	return;
}
/******************************************************************/
static void
GetChangedServers( int flag)
{
	/*
	**	If flag is set we wait for revisions, printing them as they
	**	occur, otherwise we just print revision since indicated revision
	*/
	SAPI	Server;
	int		ServerEntry = 0;
	uint32	newstamp = cnum;
	uint32	numServers = 0;
	uint16	type;
	int		status;

	if( Topt)
		type = (uint16)Tnum;
	else
		type = ALL_SERVER_TYPE;

	if( flag) {
		if( (status = SAPNotifyOfChange( SIGUSR1, Wakeup, type)) < 0) {
			fprintf(stderr, MsgGetStr(A_NOTIFY_FAILED));
			SAPPerror( status, "");
			return;
		}
	}

	do {
		/*
		**	Go to next revision
		*/
		cnum = newstamp;
		newstamp = 0;
		numServers = 0;
		ServerEntry = 0;
		while( (status = SAPGetChangedServers( type, &ServerEntry, &Server, 1,
				cnum, &newstamp)) >0) {
			numServers++;
			PrintServerEntry( &Server);
		}
		if( status < 0) {
			fprintf(stderr, MsgGetStr(A_GET_CHANGED_FAILED));
			SAPPerror( status, "");
			return;
		}
		fprintf(stdout, MsgGetStr(A_TOTAL_CHANGED),
				cnum, newstamp, numServers);
		if( flag) {
			pause();
			fprintf(stdout, MsgGetStr(A_NEWLINE));	/* Extra line feed to make readable */
		}
	} while( flag);

	return;
}

/******************************************************************/
static void
GetNearestServer( void)
{
	SAPI Server;
	int status;

	status = SAPGetNearestServer( (uint16)nnum, &Server);
	if( status < 0) {
		fprintf(stderr, MsgGetStr(A_GET_NEAREST_FAILED));
		SAPPerror( status, "");
		return;
	}
	if( status == 0) {
		fprintf(stderr, MsgGetStr(A_TYPE_NOT_FOUND), nnum);
		return;
	}

	PrintServerEntry( &Server);
	return;
}

/******************************************************************/
static void
GetServerByName( void)
{
	SAPI	Server;
	int 	ServerEntry = 0;
	uint16	Type;
	int		status;

	if( Topt)
		Type = (uint16)Tnum;
	else
		Type = ALL_SERVER_TYPE;

	while( (status = SAPGetServerByName( sname, Type, &ServerEntry, &Server, 1)) > 0) {
		PrintServerEntry( &Server);
	}
	if( status < 0) {
		fprintf(stderr, MsgGetStr(A_GET_BY_NAME_FAILED));
		SAPPerror( status, "");
		return;
	}
	return;
}

/******************************************************************/
void
DumpIPXAddress()
{
	SAPD	SapData;
	int		i;
	int		status;

	if( (status = SAPStatistics( &SapData)) < 0) {
		fprintf(stderr, MsgGetStr(A_SAP_STATS_UNAVAIL));
		SAPPerror(status, "");
		perror("");
		return;
		/* NOTREACHED */
	}

	printf(MsgGetStr(A_HEX_ADDR));
	for (i=0; i<IPX_NET_SIZE; i++)
		printf(MsgGetStr(A_HEX_BYTE), SapData.MyNetworkAddress[i]);

	printf(MsgGetStr(A_SPACE));
	for ( ; i<(IPX_NET_SIZE + IPX_NODE_SIZE); i++)
		printf(MsgGetStr(A_HEX_BYTE), SapData.MyNetworkAddress[i]);

	printf(MsgGetStr(A_SPACE));
	for ( ; i<IPX_ADDR_SIZE; i++)
		printf(MsgGetStr(A_HEX_BYTE), SapData.MyNetworkAddress[i]);

	if( !zopt)
		printf(MsgGetStr(A_IPX_ADDRESS));
	else
		printf(MsgGetStr(A_NEWLINE));

	return;
}

/******************************************************************/
void
PrintLANInfo()
{
	int		i;
	SAPD	SapData;
	SAPL	LanData;
	int		status;

	if( (status = SAPStatistics( &SapData)) < 0) {
		fprintf(stderr, MsgGetStr(A_SAP_STATS_UNAVAIL));
		SAPPerror(status, "");
		return;
		/* NOTREACHED */
	}

	for( i = 0; i < (int)SapData.Lans; i++) {
		if( SAPGetLanData( i, &LanData) > 0) {
			if( LanData.LanNumber == 0) {
				continue;
			}
			printf(MsgGetStr(A_NETWORK), LanData.Network);
			printf(MsgGetStr(A_LAN_NUMBER), LanData.LanNumber);
			printf(MsgGetStr(A_UPDATE_INTERVAL), LanData.UpdateInterval);
			printf(MsgGetStr(A_AGE_FACTOR), LanData.AgeFactor);
			printf(MsgGetStr(A_PACKET_GAP), LanData.PacketGap);
			printf(MsgGetStr(A_PACKET_SIZE), LanData.PacketSize);
			printf(MsgGetStr(A_PACKETS_SENT), LanData.PacketsSent);
			printf(MsgGetStr(A_PACKETS_RCVD), LanData.PacketsReceived);
			printf(MsgGetStr(A_BAD_PACKETS), LanData.BadPktsReceived);
			printf(MsgGetStr(A_NEWLINE));
		} else {
			printf(MsgGetStr(A_GET_LAN_DATA_FAILED), SAPGetLanData( i, &LanData));
		}
	}
}

/******************************************************************/
/*ARGSUSED*/
main(int argc, char *argv[], char *envp[])
{
	int		car;
	int		errflg = 0;
	int		optcnt = 0;
	char   *ptr;
	int		status;
	int ccode;

	ccode = MsgBindDomain(MSG_DOMAIN_SAPI, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
		fprintf(stderr,"%s: Cannot bind message domain. NWCM erorr = %d. Error exit.\n",
			argv[0], ccode);
		exit(1);
	}

	while ((car = getopt(argc, argv, "ac:C:dfFilLn:s:tT:xz?")) != (char)-1) {
		switch (car) {
		case 'a': 	/* Everything */
			dopt++;
			fopt++;
			Lopt++;
			topt++;
			xopt++;
			optcnt++;
			break;
		case 'c': 	/* Changed since revision stamp */
			copt++;
			optcnt++;
			cnum = strtoul( optarg, &ptr, 0);
			if( (ptr - optarg) != strlen(optarg)) {
				fprintf(stderr, MsgGetStr(A_INVALID_NUMERIC), argv[0], optarg);
				errflg++;
			}
			break;
		case 'C': 	/* Changed since revision stamp, wait for new revisions */
			Copt++;
			optcnt++;
			cnum = strtoul( optarg, &ptr, 0);
			if( (ptr - optarg) != strlen(optarg)) {
				fprintf(stderr, MsgGetStr(A_INVALID_NUMERIC), argv[0], optarg);
				errflg++;
			}
			break;
		case 'd': 	/* Sap Dump */
			dopt++;
			optcnt++;
			break;
		case 'f': 	/* 12 char name name on left */
			fopt++;
			optcnt++;
			break;
		case 'F': 	/* full name on right */
			Fopt++;
			optcnt++;
			break;
		case 'i':  /* Sap Statistical Information */
			iopt++;
			optcnt++;
			break;
		case 'l':  /* List local servers */
			lopt++;
			optcnt++;
			break;
		case 'L':  /* List LAN information */
			Lopt++;
			optcnt++;
			break;
		case 'n':  /* Get nearest server */
			nopt++;
			optcnt++;
			nnum = strtoul( optarg, &ptr, 0);
			if( (ptr - optarg) != strlen(optarg)) {
				fprintf(stderr, MsgGetStr(A_INVALID_NUMERIC), argv[0], optarg);
				errflg++;
			}
			if( nnum > USHRT_MAX) {
				fprintf(stderr, MsgGetStr(A_BAD_NUMERIC), argv[0], nnum, USHRT_MAX);
				errflg++;
			}
			break;
		case 's':  /* Get server information by name */
			sopt++;
			optcnt++;
			sname = (uint8 *)optarg;
			break;
		case 't':  /* Sap Statistics */
			topt++;
			optcnt++;
			break;
		case 'T':  /* Server Type */
			Topt++;
			optcnt++;
			Tnum = strtoul( optarg, &ptr, 0);
			if( (ptr - optarg) != strlen(optarg)) {
				fprintf(stderr, MsgGetStr(A_INVALID_NUMERIC), argv[0], optarg);
				errflg++;
			}
			break;
		case 'x':	/* Ipx address */
			xopt++;
			optcnt++;
			break;
		case 'z':	/* No verbiage */
			zopt++;
			optcnt++;
			break;
		case '?':	/* Error */
			errflg++;
			break;
		default:	/* Error */
			errflg++;
			break;
		}
	}
	for( ;optind < argc; optind++) {
		errflg++;
		fprintf(stderr, MsgGetStr(A_UNKNOWN_ARG), argv[optind]);
	}

	if( errflg ) {
		fprintf(stderr, MsgGetStr(A_USAGE_HEAD));
		fprintf(stderr, MsgGetStr(A_USAGE_1));
		fprintf(stderr, MsgGetStr(A_USAGE_2));
		fprintf(stderr, MsgGetStr(A_USAGE_3));
		fprintf(stderr, MsgGetStr(A_USAGE_4));
		fprintf(stderr, MsgGetStr(A_USAGE_5));
		fprintf(stderr, MsgGetStr(A_USAGE_6));
		fprintf(stderr, MsgGetStr(A_USAGE_7));
		fprintf(stderr, MsgGetStr(A_USAGE_8));
		fprintf(stderr, MsgGetStr(A_USAGE_9));
		fprintf(stderr, MsgGetStr(A_USAGE_10));
		fprintf(stderr, MsgGetStr(A_NEWLINE));
		fprintf(stderr, MsgGetStr(A_USAGE_11));
		fprintf(stderr, MsgGetStr(A_USAGE_12));
		fprintf(stderr, MsgGetStr(A_USAGE_13));
		fprintf(stderr, MsgGetStr(A_USAGE_14));
		fprintf(stderr, MsgGetStr(A_USAGE_15));
		fprintf(stderr, MsgGetStr(A_USAGE_16));
		fprintf(stderr, MsgGetStr(A_USAGE_17));
		fprintf(stderr, MsgGetStr(A_USAGE_18));
		return(0);
	}
	if( (status = SAPMapMemory()) < 0) {
		SAPPerror( status, "");
		return(-1);
	}
	if( optcnt == 0)
		iopt++;
	if( (fopt == 0) && (Fopt == 0))
		fopt++;
	if( iopt)
		ShowStats();
	if( lopt)
		GetAllServers();
	if( Lopt)
		PrintLANInfo();
	if( dopt)
		GetAllServers();
	if( topt)
		DumpSapTypes();
	if( nopt)
		GetNearestServer();
	if( copt)
		GetChangedServers(0);
	if( Copt)
		GetChangedServers(1);
	if( sopt)
		GetServerByName();
	if( xopt)
		DumpIPXAddress();
	SAPUnmapMemory();
	return(0);
}
