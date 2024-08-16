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

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/sapd/sapd.c	1.25"
#ident	"$Id: sapd.c,v 1.50.2.1 1994/11/09 18:28:57 vtag Exp $"

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

#include "sapd.h"

#ifdef DEBUG
#define SAPDEBUG YES
#endif

#define SAP_INIT_TIMER	2	/* secs. between startup SAP alarms */
#define SAP_ALARM_TIMER	1	/* secs. between startup SAP alarms */

#define IN_PACKETDEBUG	NO	/* Trace incoming packets */
#define OUT_PACKETDEBUG	NO	/* Trace outgoing packets */
#define RAWDUMP			NO	/* If raw dump is set, the complete IPX packet */
							/* will be dumped in hex instead of intrepreted */
#define SERVICEDEBUG	NO	/* Trace add and delete services */
#define NSQDEBUG		NO	/* Trace add and nearest service requests */
#define MEMORYDEBUG		NO	/* Trace memory alloc/free usage */
#define SHMDEBUG		NO	/* Display shared memory information */
#define HASHDEBUG		NO	/* Name hashing debug */
#define HASH1DEBUG		NO	/* Name hashing debug */
#define CHANGEDEBUG		NO	/* Change list debug */
#define IOCTLDEBUG		NO	/* Log file descriptors at open and failure */

#if IN_PACKETDEBUG || OUT_PACKETDEBUG
#define PACKETDEBUG YES
#else
#define PACKETDEBUG NO
#endif

#if PACKETDEBUG
#define LINE "--------------------------------------------------------------------------------"
#endif
/* #if PACKETDEBUG || SERVICEDEBUG || NSQDEBUG */
#define INPKT  "< "
#define OUTPKT "> "
/* #endif */

#if MEMORYDEBUG
#ifdef NWALLOC
#undef NWALLOC
#undef NWFREE
#endif
#define NWALLOC(size)  my_malloc(size, __FILE__, __LINE__)
#define NWFREE(address) my_free(address, __LINE__)
#endif

STATIC const char    program[] = "sapd";		/* lower case name */
STATIC char    titleStr[80] = "SAPD";			/* upper case name */
STATIC char    SapLogPath[PATH_MAX];			/* Path to sap log file */
STATIC FILE		*SapLogFile = stderr;
STATIC int		SapLog = 0;
STATIC int 		SapFastInit = FALSE; /* Default if a problem */
STATIC FILE		*SapTrackFile = NULL;
STATIC pid_t	SAPPid;

/*
**	Broadcast address for node
*/
STATIC uint8	ALLHOSTS[IPX_NODE_SIZE] = {0xFF,0xFF,0xFF,0xFF,0xFF,0xFF };

/*
**	Time stamp to mark when files changed
**	newStamp is the time stamp entities get if changed on this event
*/
STATIC uint32 newStamp;
STATIC int IpxServerCount = 0;	/* Number of next servPktInfo entry to fill */
STATIC int validTime = 0;	/* Flag used by FPRINTF, controls doing localtime */
STATIC int initialized = 0;	/* Set if initialization complete */
STATIC int terminate = 0;	/* Set if in termination */
STATIC int max_messages = 0;/* Set from configuration */
/*
**	File descriptors 
*/
STATIC int	lipmxFd;
STATIC int	ripxFd;
STATIC int	ipxFd;

/*
**	Lan data structure pointers
*/
STATIC lanInfo_t *lanInfoTable = NULL;
STATIC lanSapInfo_t *lanSapInfo = NULL;
STATIC netInfo_t **LocalNet = NULL;
/* the maximum number of lans that can be connected to ipx */
STATIC uint32 ipxConfiguredLans = 0;
STATIC uint16 sapMaxHops = 0;

STATIC uint8	TrackFlag = 0;
STATIC uint32	ChangedLink = 0;

STATIC int16 sapTimerInterval = 60;
STATIC int16 GotAlarm = FALSE;
STATIC time_t lastAlarmHit;
STATIC ServerPacketStruct_t *readBufPtr = NULL;
STATIC ServerPacketStruct_t *sendBufPtr = NULL;

/*
**	shared memory information
*/
STATIC key_t				 shmid = -1;	/* Shared memory id */
STATIC SapShmHdr_t			*ShmBase;		/* Shared memory address */
STATIC SAPL					*LanBase;		/* Lan info table address base */
STATIC int32				*nHashBase;		/* Name hash table address base */
STATIC ServerEntry_t		*SrvBase;		/* Server entry address base */

/*
**	List of local servers requiring notification of change
*/
STATIC NotifyStruct_t *NotifyList = NULL;

/*********************************************************************/
/* forward declarations */

STATIC void Hangup( int);
STATIC void Term( int);
STATIC void GiveServerInformation(uint32, ServerPacketStruct_t *);
STATIC void GetNearestServer( uint32, ServerPacketStruct_t *);
STATIC void AcceptServerInformation( uint32, ServerPacketStruct_t *, uint8 *);
STATIC void SendDumpSapInfoPacket(void);
STATIC void SendKnownServerInformation( uint32, uint32, uint8 *,
	uint16, int, int32, uint16);
STATIC void SendChangedServerInformation( uint32, uint32, uint8 *,
	uint16, int, int32);
STATIC void AddServer( uint32, uint8 *, uint8 *, uint16 , uint16, uint8 *);
STATIC int	CheckSapSource( uint8 *, uint8 *, uint32 , uint16);
STATIC void KillSrvSource( ServerEntry_t *, InfoSourceEntry_t *);
STATIC void AddSrvSource( ServerEntry_t *, InfoSourceEntry_t *);
STATIC void TrackServerPacket( uint32, ServerPacketStruct_t *, uint32);
STATIC void AgeServers( void);
STATIC void CheckForLostSignal( void);
STATIC int WritePid( void);
STATIC void AlarmService( void);
STATIC void SapExit( int);
STATIC void PrintIpxAddress(FILE *, ipxAddr_t *);
STATIC void NotifyProcessOfChanges( void);
STATIC void ShutDownSap( void);
STATIC void logperror(const char *);

/*********************************************************************/
/*
**	FPRINTF so we do strftime and localtime only if we are going
**	to print something
*/
static int
FPRINTF( FILE *outfile, const char *fmt, ...)
{
	int			ret;
	time_t		now;
	struct tm  *tp;
	char		timeStr[30];
	va_list		args;
	static		int msgcount = 0;
	struct		stat sbuf;
	int			NWs;


	if( (outfile == SapLogFile) && (outfile != stderr)) {
		/*
		**	If logging disabled, check if it has been reenabled
		*/
		if( max_messages == 0) {
			if( validTime == 0) {
				/*
				**	Check if message count has been updated
				*/
				if( (NWs = NWCMGetParam( "sap_max_messages",
							NWCP_INTEGER, &max_messages)) != SUCCESS) {
					NWCMPerror(NWs, "");
					validTime = 1;
					return(-1);
				}
				msgcount = 0;
				if( max_messages == 0) {
					validTime = 1;
					return(0);
				}
			}
		}
		if( SapLog == 0) {
			/*
			**	If the file still exists, logging is still off
			*/
			if( validTime == 0) {
				if( stat( SapLogPath, &sbuf) == 0) {
					validTime = 1;
					return(0);
				}
				/*
				**	We can create the file, see if max messages changed
				*/
				if( (NWs = NWCMGetParam( "sap_max_messages",
							NWCP_INTEGER, &max_messages)) != SUCCESS) {
					NWCMPerror(NWs, "");
					validTime = 1;
					return(-1);
				}
				if( max_messages == 0) {
					validTime = 1;
					return(0);
				}
				/*
				**	We have non zero messages, create the file
				*/
				if ((outfile = fopen(SapLogPath,"w+")) == NULL) {
					validTime = 1;
					return(-1);
				}
				SapLogFile = outfile;
				setvbuf( outfile, NULL, _IOLBF, 0);
				SapLog = 1;
				msgcount = 0;
			} else {
				return(0);
			}
		}
	}
	if( validTime == 0) {
		time(&now);
		if( (tp = localtime(&now)) != NULL) {
			strftime(timeStr, sizeof(timeStr), "%x %X", tp);
		} else {
			strcpy(timeStr, MsgGetStr(SAP_NO_TIME));
		}
		titleStr[0] = '\0';
		strcpy(titleStr, timeStr);
		validTime = 1;
	}
	va_start( args, fmt);
	ret = vfprintf( outfile, fmt, args);
	va_end( args);

	/*
	**	Now that we have printed the line, check if it causes up to wrap
	*/
	if( outfile == SapLogFile) {
		if( fmt[strlen(fmt) - 1] == '\n') {
			/*
			**	Only count messages if ends with a newline
			*/
			msgcount++;
		}
		if( msgcount == max_messages) {
			/*
			**	Max messages exceeded on log file, end logging until
			**	re-enabled.
			*/
			FPRINTF(SapLogFile, MsgGetStr(SAP_END_FILE), titleStr,
				max_messages, SapLogPath);
			fclose( outfile);
			msgcount = 0;
			SapLog = 0;
			if( (NWs = NWCMGetParam( "sap_max_messages",
						NWCP_INTEGER, &max_messages)) != SUCCESS) {
				NWCMPerror(NWs, "");
				return(-1);
			}
		}
	}
	return( ret);
}

/*********************************************************************/

#if MEMORYDEBUG
/*ARGSUSED*/
STATIC void *
my_malloc(size_t size, char *file, int line)
{
	void *address;

#ifdef DEBUG
	address = nwalloc(size, file, line);
#else
	address = nwalloc(size);
#endif
	FPRINTF(SapLogFile, "%s: 0x%08x:> Malloc @ %4d: %4d bytes\n", 
		titleStr, (uint32)address, line, size);
	return(address);
}

/*********************************************************************/
/*ARGSUSED*/
STATIC void
my_free(void *address, int line)
{
	FPRINTF(SapLogFile,"%s: 0x%08x:<   Free @ %4d:\n", 
		titleStr, address, line);
	nwfree(address);
	return;
}
#endif /* MEMORYDEBUG */


/*********************************************************************/
/* ARGSUSED */
STATIC ServerEntry_t *
SrvAlloc( size_t size)
{
	ServerEntry_t *address;

#ifdef SAPDEBUG
	if( size != sizeof(ServerEntry_t)) {
		FPRINTF(SapLogFile, "SrvAlloc: Allocation size is incorrect, Requested %d bytes, expecting %d bytes\n",
			size, sizeof(ServerEntry_t));
		SapExit(1);
		/*NOTREACHED*/
	}
#endif

	if( ShmBase->D.ServerPoolIdx == 0) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_POOL_EMPTY), titleStr);
		return(NULL);
		/*NOTREACHED*/
	}

	/*
	** We must *never* run past the end of the memory pool!
	*/
	if( ShmBase->D.ServerPoolIdx > ShmBase->D.ConfigServers) {

		FPRINTF(SapLogFile, MsgGetStr(SAP_POOL_BAD), 
					titleStr, ShmBase->D.ServerPoolIdx);
#ifdef SAPDEBUG
		SapExit(1);
#else	/* !DEBUG*/
		return(NULL);	/* expected error return if no memory*/
#endif	/* !DEBUG*/
		/*NOTREACHED*/
	}

	/*
	**	Unlink an entry from the pool.  Linked together via NameLink.
	*/
	address = SrvBase + ShmBase->D.ServerPoolIdx;
#if MEMORYDEBUG
	FPRINTF(SapLogFile, "%s: SrvAlloc: alloc index %d, address 0x%X\n",
		titleStr, ShmBase->D.ServerPoolIdx, address);
#endif
	ShmBase->D.ServerPoolIdx = address->NameLink;
#ifdef MEMORYDEBUG
	if( ShmBase->D.ServerPoolIdx == 0) {
		FPRINTF(SapLogFile,
			"%s: Pool Empty: SrvBase = 0x%X, ConfigServers = 0x%X, Index = 0x%X\n",
			titleStr,SrvBase,ShmBase->D.ConfigServers,ShmBase->D.ServerPoolIdx);
	}
#endif
	return(address);
}

/*********************************************************************/
/*
**  Get a prime number.  Note n is assumed to be small (generally < 500)
*/
STATIC int16
SetPrime( int n)
{
    int     j, found;

	for( ;; ) {
        found = 1;
        for( j=2; j<( n/2 + 1); j++) {
            if( ( n % j) == 0) {
                /* divisor found - n is not prime */
                n++;
                found = 0;
                break;
            }
        }
        if( found == 1) {
            /* n is divisable only by itself and 1 */
            break;
        }
    }
    return( n);
}

/*********************************************************************/
STATIC int 
GetNameHash(uint8 *name)
{
	uint32 tmp;
	/*
	**	Name is not word aligned.  Include length of name (1st byte) in hash
	**	calculation
	*/
	tmp = PGETINT32(name);
	tmp += PGETINT32(&name[4]);
	tmp += PGETINT32(&name[8]);
	tmp = tmp % ShmBase->NameHashSize;
	return((int)tmp);
}

/*********************************************************************/
STATIC int 
GetNetEntry(uint32 network, netInfo_t *netPtr)
{	struct strioctl ioc;

	memset((char *)netPtr, (char)0, sizeof(netInfo_t));
	netPtr->netIDNumber = network;
	ioc.ic_cmd = RIPX_GET_NET_INFO;
	ioc.ic_timout = 5;
	ioc.ic_len = sizeof(netInfo_t);
	ioc.ic_dp = (char *)netPtr;

	if (ioctl(ripxFd, I_STR, &ioc) < 0) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_NET_LIST),
							titleStr, GETINT32(network));
		logperror("");
		return(-1);
	}
	return(0);
}

/*********************************************************************/
/*
**	Format an IPX address for output, returns address of formatted address
*/
char *
FormatIpxAddress(
	ipxAddr_t *ipxAddr)
{
	static char output[80];
	char temp[80];
	int	i;

	output[0] = '\0';
	for( i=0; i<IPX_NET_SIZE; i++) {
		sprintf( temp,"%02X",ipxAddr->net[i]);
		strcat( output, temp);
	}
	strcat( output," ");
	for( i=0; i<IPX_NODE_SIZE; i++) {
		sprintf( temp,"%02X",ipxAddr->node[i]);
		strcat( output, temp);
	}
	strcat( output," ");
	for( i=0; i<IPX_SOCK_SIZE; i++) {
		sprintf( temp,"%02X",ipxAddr->sock[i]);
		strcat( output,temp);
	}
	return( output);
}

/* #if PACKETDEBUG || SERVICEDEBUG || NSQDEBUG */
/*********************************************************************/
/*
**	Print Server Information, all parameters are in net order
*/
STATIC void
PrintSrvPacket(
	FILE		*outFp,
	uint16		 type,
	uint8		*name,
	ipxAddr_t	*addr,
	uint16		 hops)
{
	FPRINTF(outFp,"type 0x%04X @ ", GETINT16(type));
	PrintIpxAddress(outFp, addr);
	FPRINTF(outFp, ", hops %2d \"%s\"\n", GETINT16(hops), name);
	return;
}

/*********************************************************************/
/*
**	These functions are used, for debugging
*/
STATIC void 
PrintIpxAddress(
	FILE *outFp,
	ipxAddr_t *ipxAddr)
{
	int i;

	FPRINTF(outFp," 0x");
	for (i=0; i<IPX_NET_SIZE; i++)
		FPRINTF(outFp,"%02X",ipxAddr->net[i]);

	FPRINTF(outFp," ");
	for (i=0; i<IPX_NODE_SIZE; i++)
		FPRINTF(outFp,"%02X",ipxAddr->node[i]);

	FPRINTF(outFp," ");
	for (i=0; i<IPX_SOCK_SIZE; i++)
		FPRINTF(outFp,"%02X",ipxAddr->sock[i]);
	return;
}
/* #endif *//* PACKETDEBUG || SERVICEDEBUG */


#if PACKETDEBUG || NSQDEBUG
/*********************************************************************/
STATIC void
PrintIpxPacket(
	FILE *outFp,
	ServerPacketStruct_t *ipxPacketPtr,
	char *str)
{
	int i,co,sp,ix,hdrsize,packetLength;
	int rawflag = 0;
	ServPacketInfo_t *srvpkt;

#if RAWDUMP
	hdrsize = 0;
#else
	hdrsize = IPX_HDR_SIZE;
#endif

	/*
	**	Print IPX header
	*/
	FPRINTF(outFp,"%sipxPacket dump from 0x%X\n", str, ipxPacketPtr);
	FPRINTF(outFp,"%scheckSum: 0x%04X\n", 
		str, ipxPacketPtr->ipxHdr.chksum);
	FPRINTF(outFp,"%slength  : 0x%04X\n",
		str, GETINT16(ipxPacketPtr->ipxHdr.len));
	FPRINTF(outFp,"%spt      : 0x%02X\n", str, ipxPacketPtr->ipxHdr.pt);
	FPRINTF(outFp,"%stc      : 0x%02X\n", str, ipxPacketPtr->ipxHdr.tc);
	FPRINTF(outFp,"%sdest    : ", str);
		PrintIpxAddress(outFp, &ipxPacketPtr->ipxHdr.dest);
	FPRINTF(outFp,"\n");
	FPRINTF(outFp,"%ssrc     : ", str);
		PrintIpxAddress(outFp, &ipxPacketPtr->ipxHdr.src);
	FPRINTF(outFp,"\n");
	
	packetLength = GETINT16(ipxPacketPtr->ipxHdr.len) -
		(IPX_HDR_SIZE + sizeof(ipxPacketPtr->Operation));

	if( (packetLength <= 0) || (packetLength % sizeof(ServPacketInfo_t))) {
		rawflag++;
	}

	/*
	*	Print Operation
	*/
	if( (rawflag == 0) || (packetLength == 2) || (packetLength == 0)) {
		FPRINTF(outFp,"%sfunction: ", str);
		switch( GETINT16(ipxPacketPtr->Operation)) {
		case SAP_GSQ:
			FPRINTF(outFp,"GENERAL SERVICE REQUEST");
			break;
		case SAP_GSR:
			FPRINTF(outFp,"GENERAL SERVICE RESPONSE");
			break;
		case SAP_NSQ:
			FPRINTF(outFp,"NEAREST SERVICE REQUEST");
			break;
		case SAP_NSR:
			FPRINTF(outFp,"NEAREST SERVICE RESPONSE");
			break;
		default:
			FPRINTF( outFp, "0x%04x", GETINT16(ipxPacketPtr->Operation));
			rawflag++;
			break;
		}
	}

	/*
	**	Print Service Type
	*/
	if( packetLength == 2) {
		FPRINTF(outFp,", type 0x%04X\n", PGETINT16(&ipxPacketPtr->ServerTable));
		return;
	} else {
		FPRINTF(outFp, "\n");
	}

	/*
	**	Print Server Entries
	*/
	if( rawflag == 0) {
		co = packetLength / sizeof(ServPacketInfo_t);
		srvpkt = ipxPacketPtr->ServerTable;
		for( i = 0; i < co; i++) {
			FPRINTF(outFp,"%sserver %d: ", str, i+1);
			PrintSrvPacket( outFp, srvpkt->TargetType, srvpkt->TargetServer,
				(ipxAddr_t *)srvpkt->TargetAddress, srvpkt->ServerHops);
			srvpkt++;
		}
		return;
	}

	/*
	**	This wasn't an ordinary SAP packet, dump data in hex
	*/
	co = sp = 0;
	ix = hdrsize;
	FPRINTF(outFp, "%s%02X:", str, ix);
	for (i=((uint32)ipxPacketPtr + hdrsize); 
			i<(GETINT16(ipxPacketPtr->ipxHdr.len) + (uint32)ipxPacketPtr); 
			i++) {
		if( co == 16) {
			co = 0;
			sp = 0;
			FPRINTF(outFp,"\n");
			FPRINTF(outFp, "%s%02X:", str, ix);
		}
		if( sp == 4) {
			FPRINTF(outFp, " ");
			sp = 0;
		}
		FPRINTF(outFp," %02X",*(uint8 *)i); 
		co++;
		sp++;
		ix++;
	}
	FPRINTF(outFp,"\n");
	return;
}
#endif /* PACKETDEBUG */

/******************************************************************/
/*
**	Match a network with one of our lans
**	Network is in machine byte order
*/
STATIC int 
NetToMyLan( uint32 network)
{
	int i;

	for( i = 0; i < ipxConfiguredLans; i++) {
		if (lanInfoTable[i].network == network) {
			return(i);
		}
	}
	return(-1);
}

/*********************************************************************/
/* ARGSUSED */
STATIC void 
AddToChangedList(ServerEntry_t *Server, char *str)
{
#ifdef SAPDEBUG
	ServerEntry_t *srv;

	for( srv = SrvBase + ChangedLink; srv != SrvBase;
			srv = SrvBase + srv->ChangedLink) {
		if( srv == Server ) {
	FPRINTF(SapLogFile, "%s: %s: Adding 0x%X to change list twice, next link 0x%X\n",
		titleStr, str, ChangedLink, Server->ChangedLink);
#if CHANGEDEBUG
			abort();
#else
			return;
#endif
		}
	}
#endif
	Server->ChangedLink = ChangedLink;
	ChangedLink = Server - SrvBase;
#if CHANGEDEBUG
	FPRINTF(SapLogFile, "%s: %s: Adding 0x%X to change list, next link 0x%X\n",
		titleStr, str, ChangedLink, Server->ChangedLink);
#endif
	return;
}

/*********************************************************************/
/*
**	The PruneSrvSource traverses the list of changed servers and
**	removes any downed sources.  When a source is aged out or marked
**	down because of information from the network, it is kept in the
**	server's list of sources until the split horizion broadcasts are
**	complete.  It is then removed from the server's sources by
**	this function.
*/
void
PruneSrvSource( void)
{
	ServerEntry_t			*Server;
	InfoSourceEntry_t		*Info,
							*NextInfo;

	if( ChangedLink == 0) {
		return;
	}

	for( Server = SrvBase + ChangedLink; Server != SrvBase;
			Server = SrvBase + Server->ChangedLink) {

		Info = Server->SourceListLink;
		while( Info != NULL) {
			if( Info->HopsToSource == SAP_DOWN) {
				/*	We have found a downed source, release it */
				NextInfo = Info->NextSource;
				KillSrvSource(Server, Info);
				NWFREE((char *)Info);
				Info = NextInfo;
			} else {
				Info = Info->NextSource;
			}
		}
	}
	return;
}

/*********************************************************************/

STATIC void 
DeleteServer(uint32 network)
{
	ServerEntry_t			*Server;
	InfoSourceEntry_t		*Info;
	netInfo_t				*Net;
	uint32					 count;
	int						 svrIndex;

	svrIndex = 1;
	for( Server = SrvBase + svrIndex; svrIndex <= ShmBase->D.ConfigServers;
			Server = SrvBase + svrIndex) {

		svrIndex++;
		if( IPXCMPNET(&network, Server->ServerAddress)) {
			/* Lost this server, mark all routes down */
			Info = Server->SourceListLink;
			while( Info != NULL) {
				Info->HopsToSource = SAP_DOWN;
				Info = Info->NextSource;
			}
			/* Lost this server */
			if( Server->HopsToServer != SAP_DOWN) {
				Server->HopsToServer = SAP_DOWN;
				Server->RevisionStamp = newStamp;
				ShmBase->D.RevisionStamp = newStamp;
				if( Server->LocalPid != 0) {
					FPRINTF(SapLogFile, MsgGetStr(SAP_RIP_DELETLE_LOCAL),
						titleStr);
					FPRINTF(SapLogFile, MsgGetStr(SAP_SERVER_INFO),
						GETINT16(Server->ServerType), 
						Server->LocalPid, &Server->ServerName[1]);
				}
#if SERVICEDEBUG
				FPRINTF(SapLogFile,"%s: %sDELSRV(rip_delete):   ",
					titleStr,INPKT);
				PrintSrvPacket( SapLogFile, Server->ServerType,
					&Server->ServerName[1], (ipxAddr_t *)Server->ServerAddress,
					GETINT16(Server->HopsToServer));
#endif
				/* Add to changed list */
				ShmBase->D.RipServerDown++;
				AddToChangedList( Server, "DeleteServer");
			}
		}
	}
	/*
	**  Notify local processes of any changes
	*/
	NotifyProcessOfChanges();
	/*
	**  Don't send info on internal lan
	*/
	for ( count = 1; count < ipxConfiguredLans; count++) {
		Net = LocalNet[count];
		if( Net == NULL)
			continue;

		if( ChangedLink == 0)
			continue;
		SendChangedServerInformation( count, Net->netIDNumber,
			ALLHOSTS, GETINT16(SAP_SAS), PACKET_DELAY, CHECK_SPLIT_HORIZ);
	}
	PruneSrvSource();
	ChangedLink = 0;
	return;
}

/*********************************************************************/
STATIC void
ReadRipPacket(void)
{
	int flags;
	int status;
	struct strbuf rDataBuf;
	uint32	network;


	rDataBuf.len = 
	rDataBuf.maxlen = sizeof(uint32);
	rDataBuf.buf = (char *)&network;

	flags = 0;
	if ( (status = getmsg(ripxFd, 0, &rDataBuf, &flags)) < 0) {
		if (errno == EINTR) {
			return;
		}
		FPRINTF(SapLogFile, MsgGetStr(SAP_READ_FAIL), titleStr);
		logperror("");
		SapExit(1);
		/* NOTREACHED */
	}
	ShmBase->D.TotalInRipSaps++;

	/*
	**	If we have received a hangup, just exit
	*/
	if(rDataBuf.len == 0) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_HANGUP), titleStr);
		SapExit(0);
		/* NOTREACHED */
	}

	/*
	**	If we are terminating, ignore input
	*/
	if( terminate ) {
		ShutDownSap();
		SapExit(0);
		/* NOTREACHED */
	}

    /*
    **  If extra data, throw it away and start over
    */
    if( (status == MORECTL) || (status == MOREDATA)) {
        ShmBase->D.BadRipSaps++;
        while( (status == MORECTL) || (status == MOREDATA)) {
            status = getmsg(ripxFd, &rDataBuf, &rDataBuf, &flags);
        }
        return;
    }
 
    CheckForLostSignal();

	newStamp = ShmBase->D.RevisionStamp + 1;
	if( newStamp == 0)
		newStamp++;
	DeleteServer(network);
	return;
}

/*********************************************************************/
/*
**	Read Packet from Net
*/
STATIC int
ReadSapPacket(int fd, ServerPacketStruct_t *ipxPacket, int readSize)
{
	int flags;
	int status;
	int packetLength;
	struct strbuf rDataBuf;
	int	connectedLan;
#if IN_PACKETDEBUG
	char data[80];
#endif

	flags = 0;

	rDataBuf.len = 
	rDataBuf.maxlen = readSize;
	rDataBuf.buf = (char *)ipxPacket;

#if IN_PACKETDEBUG
	FPRINTF(SapLogFile,"%s: %s\n", titleStr, LINE);
#endif
	if ( (status = getmsg(fd, 0, &rDataBuf, &flags)) < 0) {
		if (errno == EINTR) {
			return(0);
		}
		/*
		**	This is a real error, stop
		*/
		FPRINTF(SapLogFile, MsgGetStr(SAP_READ_FAIL), titleStr);
		logperror("");
		SapExit(1);
		/* NOTREACHED */
	}
	ShmBase->D.TotalInSaps++;

	/*
	**	If we have received a hangup, just exit
	*/
	if(rDataBuf.len == 0) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_HANGUP), titleStr);
		SapExit(0);
		/* NOTREACHED */
	}

	/*
	**	If we are terminating, ignore input
	*/
	if( terminate ) {
		ShutDownSap();
		SapExit(0);
		/* NOTREACHED */
	}

	newStamp = ShmBase->D.RevisionStamp + 1;
	if( newStamp == 0)
		newStamp++;

#if IN_PACKETDEBUG
	FPRINTF(SapLogFile,"%s: Received %d bytes, status %d\n",
		titleStr, rDataBuf.len, status);
#endif

	ShmBase->D.TotalInSaps++;

#if IN_PACKETDEBUG
	sprintf(data, "%s: %s", titleStr, INPKT);
	PrintIpxPacket(SapLogFile, ipxPacket, data);
#endif

	connectedLan = NetToMyLan(PGETINT32(ipxPacket->ipxHdr.src.net));
	if( connectedLan < 0) {
		/*
		**	Got from bad source ???
		*/
		ShmBase->D.BadSizeInSaps++;
		return(0);
	}

	/*
	**	If extra data, throw it away and start over
	*/
	if( (status == MORECTL) || (status == MOREDATA)) {
		ShmBase->D.BadSizeInSaps++;
		FPRINTF(SapLogFile, MsgGetStr(SAP_READ_SIZE), titleStr,
			PGETINT16(&ipxPacket->ipxHdr.len),
			FormatIpxAddress( (ipxAddr_t *)&ipxPacket->ipxHdr.src.net));
		while( (status == MORECTL) || (status == MOREDATA)) {
			status = getmsg(fd, &rDataBuf, &rDataBuf, &flags);
		}
		LanBase[connectedLan].BadPktsReceived++;
		return(0);
	}

	CheckForLostSignal();

	/*
	** compare #bytes from getmsg() to #bytes reported by ipx header
	*/
	packetLength = GETINT16(ipxPacket->ipxHdr.len);
	if (rDataBuf.len != packetLength) {
		ShmBase->D.BadSizeInSaps++;
		FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN_BAD), titleStr,
			packetLength, rDataBuf.len);
		LanBase[connectedLan].BadPktsReceived++;
		return(0);
	}
	LanBase[connectedLan].PacketsReceived++;
	return(1);
}

/*********************************************************************/
/*
**	Write Packet to Net
*/
STATIC void
WriteSapPacket(
		ServerPacketStruct_t *ipxPacket)
{
	struct strbuf txDataBuf;
	int connectedLan;
#if OUT_PACKETDEBUG
	char data[80];
#endif

	ShmBase->D.TotalOutSaps++;
	/*
	**	We can send only to an entity connected to one of
	**	our lans
	*/
	if( (connectedLan = NetToMyLan(PGETINT32(ipxPacket->ipxHdr.dest.net))) < 0) {
		switch( GETINT16(ipxPacket->Operation)) {
		case SAP_GSR:
			FPRINTF(SapLogFile, MsgGetStr(SAP_OUT_GSR), titleStr,
				FormatIpxAddress( (ipxAddr_t *)ipxPacket->ipxHdr.dest.net));
			break;
		case SAP_GSQ:
			FPRINTF(SapLogFile, MsgGetStr(SAP_OUT_GSQ), titleStr,
				FormatIpxAddress( (ipxAddr_t *)ipxPacket->ipxHdr.dest.net));
			break;
		case SAP_NSQ:
			FPRINTF(SapLogFile, MsgGetStr(SAP_OUT_NSQ), titleStr,
				FormatIpxAddress( (ipxAddr_t *)ipxPacket->ipxHdr.dest.net));
			break;
		case SAP_NSR:
			FPRINTF(SapLogFile, MsgGetStr(SAP_OUT_NSR), titleStr,
				FormatIpxAddress( (ipxAddr_t *)ipxPacket->ipxHdr.dest.net));
			break;
		default:
			FPRINTF(SapLogFile, MsgGetStr(SAP_OUT_TYPE), titleStr,
				GETINT16(ipxPacket->Operation),
					FormatIpxAddress( (ipxAddr_t *)ipxPacket->ipxHdr.dest.net));
			break;
		}
		ShmBase->D.BadDestOutSaps++;
		return;
	}

	LanBase[connectedLan].PacketsSent++;
#if OUT_PACKETDEBUG
	FPRINTF(SapLogFile,"%s: %s\n", titleStr, LINE);
#endif

	/*
	**	Set up Source Net/Node/Socket
	*/
	PPUTINT32(lanInfoTable[connectedLan].network, ipxPacket->ipxHdr.src.net);
	IPXCOPYNODE( &lanInfoTable[connectedLan].nodeAddress[0],
			&ipxPacket->ipxHdr.src.node[0]);
	PPUTINT16(SAP_SAS, ipxPacket->ipxHdr.src.sock);

	txDataBuf.len = GETINT16(ipxPacket->ipxHdr.len);
	txDataBuf.buf = (char *)ipxPacket;

#if OUT_PACKETDEBUG
	FPRINTF(SapLogFile,"%s: putmsg %d, destination lan %d\n",
		titleStr, txDataBuf.len, connectedLan);
	sprintf(data,"%s: %s", titleStr, OUTPKT);
	PrintIpxPacket(SapLogFile, ipxPacket, data);
#endif
	if (putmsg(ipxFd, NULL, &txDataBuf, 0 ) == -1) {
		FPRINTF(SapLogFile,MsgGetStr(SAP_PUTMSG_FAIL), titleStr);
		logperror("");
		SapExit(0);
	}
	return;
}

/*********************************************************************/

/*
** The following routine builds the ipx header in the packet
*/
STATIC void 
BuildPktHeader(
	ServerPacketStruct_t *ServPacket,
	uint32				 DestinationNet,
	uint8				*DestinationHost,
	uint16				 DestinationSocket)
{
	uint16				 Operation;
	/*
	**	Set up ipx packet
	*/
	ServPacket->ipxHdr.chksum = (uint16)IPX_CHKSUM;
	ServPacket->ipxHdr.tc = 0;
	/* Flag as a packet exchange packet */
	ServPacket->ipxHdr.pt = SAP_PACKET_TYPE;
	IPXCOPYNET(&DestinationNet,ServPacket->ipxHdr.dest.net);
	IPXCOPYNODE(DestinationHost,ServPacket->ipxHdr.dest.node);
	IPXCOPYSOCK(&DestinationSocket,ServPacket->ipxHdr.dest.sock);
	PPUTINT16(SAP_SAS, ServPacket->ipxHdr.src.sock);
	/*
	**	General Service Reply
	*/
	Operation = SAP_GSR;
	PPUTINT16(Operation, &ServPacket->Operation);
	IpxServerCount = 0;
	return;
}

/*********************************************************************/
/*
**	The following routine completes a packet and sends it out
*/
STATIC void
BuildPktEnd(
	uint32				 ConnectedLAN,
	ServerPacketStruct_t *ServPacket,
	int					 PacketDelay)
{
		if( IpxServerCount == 0)	/* Packet is empty */
			return;
		PPUTINT16( IPX_HDR_SIZE + sizeof(ServPacket->Operation)
					+ (IpxServerCount * sizeof(ServPacketInfo_t)),
				&(ServPacket->ipxHdr.len));
		TrackServerPacket(ConnectedLAN, ServPacket, FALSE);
		if( PacketDelay) {
			PacketDelay = lanSapInfo[ConnectedLAN].gap;
			MicroSleep( PacketDelay);
		}
		WriteSapPacket( ServPacket );
		IpxServerCount = 0;
		ShmBase->D.GSRSent++;
		return;
}

/*********************************************************************/
/*
**	Add a single server entry to the output packet
*/
STATIC void
BuildPktServer(
	ServerEntry_t       *Server,
	uint32				 ConnectedLAN,
	ServerPacketStruct_t *ServPacket,
    uint32               RouteLegDown,
	int					 PacketDelay)
{
    ServPacketInfo_t     *Target;
	int					pktEntries;
	/*
	**	Find out how many SAP entries will fit in packet
	*/
	pktEntries = ((lanInfoTable[ConnectedLAN].ripSapInfo.sap.maxPktSize
		- IPX_HDR_SIZE - sizeof(uint16)) / sizeof(ServPacketInfo_t));
	/*
	**	If no room left in packet, send it out
	*/
	if( IpxServerCount >= pktEntries) {
		BuildPktEnd( ConnectedLAN, ServPacket, PacketDelay);
	}

	Target = ServPacket->ServerTable + IpxServerCount;
	Target->TargetType = Server->ServerType;
	memcpy( Target->TargetServer, &(Server->ServerName[1]),
			Server->ServerName[0]);
	memset( &(Target->TargetServer[Server->ServerName[0]]), 0,
			NWMAX_SERVER_NAME_LENGTH - Server->ServerName[0]);
	IPXCOPYADDR(Server->ServerAddress, Target->TargetAddress);
	PPUTINT16(RouteLegDown || (Server->HopsToServer >= sapMaxHops) ?
			SAP_DOWN : Server->HopsToServer + 1,
			&(Target->ServerHops));
	IpxServerCount++;
	return;
}
/*********************************************************************/

/*
 * The following routine sends changed server information about the network's
 * file server addresses to anyone interested in listening.
 */

/*******************************************************************/
STATIC void 
SendChangedServerInformation(
		uint32				 ConnectedLAN,
		uint32				 DestinationNet,
		uint8				*DestinationHost,
		uint16				 DestinationSocket,
		int					 PacketDelay,	/* Delay Packets between pkts */
		int32				 SplitHoriz)	/* set to check, unless destination
										**	is broadcast from GSR */
{
    ServerEntry_t       *Server;
    InfoSourceEntry_t   *Source;
    netInfo_t           *Net;
    uint32               CheckRoute;
    uint32               RouteLegDown;

	/*
	**	if we aren't using this lan, we are done
	*/
    Net = LocalNet[ConnectedLAN];
    if (Net == NULL) {
        FPRINTF(SapLogFile, MsgGetStr(SAP_NO_NET), titleStr, ConnectedLAN);
        return;
    }
    /*
    **  Use netInfo data (RIPX) to determine if an outbound route is down
    */
    RouteLegDown = (Net->hopsToNet >= sapMaxHops) ? TRUE : FALSE;
    CheckRoute = ((Net->netStatus & NET_STAR_BIT) == 0) ? TRUE : FALSE;

	/*
	**	If no servers have changed, we are done
	*/
    if( ChangedLink == 0) {
#if OUT_PACKETDEBUG
        FPRINTF(SapLogFile,
            "%s: SendChangedServerInformation no changed server entries, return\n",
            titleStr);
#endif
        return;
    }

	BuildPktHeader( sendBufPtr, 
		DestinationNet, DestinationHost, DestinationSocket);

	for( Server = SrvBase + ChangedLink; Server != SrvBase;
			Server = SrvBase + Server->ChangedLink) {

#if CHANGEDEBUG
		FPRINTF( SapLogFile, "%s: SendChangedServerInformation: Processing 0x%X from change list, next link 0x%X\n",
			titleStr, Server - SrvBase, Server->ChangedLink);
#endif
		/*
		**	If broadcast about a server that I heard about on this
		**	lan, we don't report on the service because I'm not the
		**	best source. (Split Horizion).
		**
		**	If route is a not a star and destination host is a broadcast
		**		(broadcasting to a broadcast topology lan)
		**	if server more than 1 hop away or server not on my platform
		**		then 
		**	look thru all routes entries with route hops = server hops
		**		and find if there is one that is on the destination lan
		**	if there is, (and it's not the internal lan)
		**		don't report this entry, i.e., squelch it.
		*/
		if( CheckRoute && SplitHoriz && ((Server->HopsToServer > 1)
				|| (memcmp(Server->ServerAddress, ShmBase->D.MyNetworkAddress,
						IPX_NET_SIZE + IPX_NODE_SIZE) != 0))) {
			int flag = 0;
			/*
			 * Squelch report if 'best' source is on this net and
			 *	net is not internal net - we respond for all
			 *	servers on internal net
			 */
			
			for( Source = Server->SourceListLink;
					(Source != NULL)
						&& (Source->HopsToSource == Server->HopsToServer);
					Source = Source->NextSource) {
				if (Source->ServConnectedLAN == ConnectedLAN) {
					if (Source->ServConnectedLAN != 0) {
						/*
						**	Skip if Server Entry lan and destination lan
						**	are the same
						*/
						flag++;
						break;
					}
				}
				/* Don't report--my 'best' path will report itself */
			}
			if(flag)
				continue;	/* squelched */
		}

		BuildPktServer( Server, ConnectedLAN,  sendBufPtr,
			RouteLegDown, PacketDelay);
	}
	/*
	**	Send out rest of packet if anything there
	*/
	BuildPktEnd( ConnectedLAN,  sendBufPtr, PacketDelay);

	/*
	**	Update time last periodic update sent
	*/
	return;
}
/*********************************************************************/

/*
 * The following routine sends all known information about the network's
 * file server addresses to anyone interested in listening.
 */

/*******************************************************************/
STATIC void 
SendKnownServerInformation(
		uint32				 ConnectedLAN,
		uint32				 DestinationNet,
		uint8				*DestinationHost,
		uint16				 DestinationSocket,
		int					 PacketDelay,	/* Delay Packets between pkts */
		int32				 SplitHoriz,/* set to check, unless destination */
										/*	is broadcast from GSR */
		uint16				 RequestedType)
{
    ServerEntry_t       *Server;
    InfoSourceEntry_t   *Source;
    netInfo_t           *Net;
    uint32               CheckRoute;
    uint32               svrIndex;
    uint32               RouteLegDown;

	/*
	**	if we aren't using this lan, we are done
	*/
    Net = LocalNet[ConnectedLAN];
    if (Net == NULL) {
        FPRINTF(SapLogFile, MsgGetStr(SAP_NO_NET), titleStr, ConnectedLAN);
        return;
    }
    /*
    **  Use netInfo data (RIPX) to determine if an outbound route is down
    */
    RouteLegDown = (Net->hopsToNet >= sapMaxHops) ? TRUE : FALSE;
    CheckRoute = ((Net->netStatus & NET_STAR_BIT) == 0) ? TRUE : FALSE;

	/*
	**	If no servers are registered, we are done, i.e. no entries taken from
	**	server pool.
	*/
    if( ShmBase->D.ServerPoolIdx == 1) {
#if OUT_PACKETDEBUG
        FPRINTF(SapLogFile,
            "%s: SendPeriodicServerInformation we have no server entries, return\n",
            titleStr);
#endif
        return;
    }

	BuildPktHeader( sendBufPtr, 
		DestinationNet, DestinationHost, DestinationSocket);
	svrIndex = 1;

	for( Server = SrvBase + svrIndex; svrIndex <= ShmBase->D.ConfigServers;
			Server = SrvBase + svrIndex) {
			svrIndex++;
		/*
		**	Don't bother with old dead servers
		*/
		if( Server->HopsToServer == SAP_DOWN) {
			continue;
		}

		/* Report this server!
		** Code added 10/25/89,
		** avoid unintentional broadcast of "sapMaxHops"
		*/
		if( Server->HopsToServer == (sapMaxHops - 1))
			continue;
		if( (RequestedType != ALL_SERVER_TYPE)
				&& (RequestedType != Server->ServerType))
			continue;

		/*
		**	If broadcast about a server that I heard about on this
		**	lan, we don't report on the service because I'm not the
		**	best source. (Split Horizion).
		**
		**	If route is a not a star and destination host is a broadcast
		**		(broadcasting to a broadcast topology lan)
		**	if server more than 1 hop away or server not on my platform
		**		then 
		**	look thru all routes entries with route hops = server hops
		**		and find if there is one that is on the destination lan
		**	if there is, (and it's not the internal lan)
		**		don't report this entry, i.e., squelch it.
		*/
		if( CheckRoute && SplitHoriz && ((Server->HopsToServer > 1)
				|| (memcmp(Server->ServerAddress, ShmBase->D.MyNetworkAddress,
						IPX_NET_SIZE + IPX_NODE_SIZE) != 0))) {
			int flag = 0;
			/*
			 * Squelch report if 'best' source is on this net and
			 *	net is not internal net - we respond for all
			 *	servers on internal net
			 */
			
			for( Source = Server->SourceListLink;
					(Source != NULL)
						&& (Source->HopsToSource == Server->HopsToServer);
					Source = Source->NextSource) {
				if (Source->ServConnectedLAN == ConnectedLAN) {
					if (Source->ServConnectedLAN != 0) {
						/*
						**	Skip if Server Entry lan and destination lan
						**	are the same
						*/
						flag++;
						break;
					}
				}
				/* Don't report--my 'best' path will report itself */
			}
			if(flag)
				continue;	/* squelched */
		}

		BuildPktServer( Server, ConnectedLAN,  sendBufPtr,
			RouteLegDown, PacketDelay);
	}
	/*
	**	Send out rest of packet if anything in it
	*/
	BuildPktEnd( ConnectedLAN,  sendBufPtr, PacketDelay);

	return;
}

/*********************************************************************/
STATIC void 
GiveServerInformation(
		uint32 ConnectedLAN,
		ServerPacketStruct_t *packet)
{
	uint32 length;
	int32  SplitHoriz;
	uint16 SearchType;
	uint32 net;
	uint16 sock;

	length = PGETINT16(&(packet->ipxHdr.len));
	if (length == IPX_HDR_SIZE)
		SearchType = ALL_SERVER_TYPE;
	else
		SearchType = packet->ServerTable[0].TargetType;

	if( IPXCMPNODE( packet->ipxHdr.dest.node, ALLHOSTS)) {
		/* This was a broadcast packet */
		SplitHoriz = CHECK_SPLIT_HORIZ;
	} else {
		/* This wasn't a broadcast packet */
		SplitHoriz = NO_SPLIT_HORIZ;
	}

	GETALIGN16( packet->ipxHdr.src.sock, &sock);
	GETALIGN32( packet->ipxHdr.src.net, &net);

	/*
	**	Send all packets of type requested
	*/

	SendKnownServerInformation( ConnectedLAN, net, packet->ipxHdr.src.node, 
		sock, PACKET_DELAY, SplitHoriz, SearchType);
	return;
}


/*********************************************************************/
/*
**	Walk the chain of servers and find the one that matches
**	the name and type of the requested server
*/
STATIC 
ServerEntry_t *
FindServer(
		uint8	*Name,
		uint16	 Type)
{
	ServerEntry_t *Server;
	int32		   *HashEntry;
	int				hash;

#if HASH1DEBUG
		FPRINTF(SapLogFile, "%s: FindServer: name %s\n", titleStr, &Name[1]);
#endif
	hash = GetNameHash(Name);
	HashEntry = nHashBase + hash;
#if HASH1DEBUG
		FPRINTF(SapLogFile, "%s: FindServer hash 0x%X, nHashBase 0x%X, HashEntry 0x%X, *HashEntry 0x%X\n",
		titleStr, hash, nHashBase, HashEntry, *HashEntry);
#endif
	/*
	**	No servers hashed to this name
	*/
	if( *HashEntry == 0) {
#if HASH1DEBUG
		FPRINTF(SapLogFile, "%s: FindServer returns NULL, @ hash 0x%X\n",
			titleStr, hash);
#endif
		return( NULL);
	}
	/*
	**	Check each server hashed to this name
	*/
	for( Server = SrvBase + *HashEntry; Server != SrvBase;
			Server = SrvBase + Server->NameLink) {
#if HASH1DEBUG
		FPRINTF(SapLogFile, "%s: FindServer: @ Server 0x%X, NameLink 0x%X, type %d\n",
			titleStr, Server, Server->NameLink, Server->ServerType);
#endif
		/*
		**	Types must match and Server Name length must match
		*/
		if((Type == Server->ServerType) && (*Name == *Server->ServerName)) {
			/*
			**	Now do the expensive memcmp for the rest of the name
			*/
			if( memcmp(&Name[1], &Server->ServerName[1], *Name) == 0) {
				/* This is the server! */
#if HASH1DEBUG
		FPRINTF(SapLogFile, "%s: FindServer: found %s\n",
			titleStr, &Server->ServerName[1]);
#endif
				return(Server);
			}
		}
	}
#if HASH1DEBUG
		FPRINTF(SapLogFile, "%s: FindServer returns NULL, @ hash 0x%X\n",
			titleStr, hash);
#endif
	return(NULL);
}

/*********************************************************************/
STATIC void 
AddServer(
		uint32 ConnectedLAN,
		uint8 *Name,			/* LENGTH-PRECEDED!!! */
		uint8 *Address,
		uint16 Type,
		uint16 Hops,
		uint8 *Source)
{
	ServerEntry_t		*Server;
	InfoSourceEntry_t	*Info,
						*NextInfo;
	int					 netflag = 0;
	int					 checksrc = 0;
	netInfo_t			 Net,
						 SNet;
	int32				*HashEnt;
	int					 hash;
	uint32				 network;
	uint16				 oldhops = 0;

	GETALIGN32( Address, &network);
	/*
	**	At this point we previously called CheckSapSource to
	**	see if packet source is believable for this entry.
	**	However this results in 7 ioctl calls to RIPX for
	**	every packet (one for each source).  We therefore
	**	will believe the sap entry if it is the same as
	**	one previously received.  We check sap source only
	**	for new or changed entries.
	*/
#ifdef TOO_MUCH_WORK
	if (CheckSapSource(Address, Source, ConnectedLAN, Hops)) {
		ShmBase->D.BadSapSource++;
#if SERVICEDEBUG
		FPRINTF(SapLogFile,"%s: %sIGNR(chk_sap_source): ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Type, &Name[1], (ipxAddr_t *)Address,
			GETINT16(Hops));
#endif
		return;
	}
#endif

	/*
	**	Make sure Hops are sane
	*/
	if( Hops >= sapMaxHops)
		Hops = SAP_DOWN;

	/*
	**	Find the server
	*/
	Server = FindServer(Name, Type);
	if( Server == NULL) {
		/*
		**	Never heard of this server, we need to add it in
		*/
		goto NewServer;		/* Just to make this easy to read */
	} else {
		/*
		**	I already know about this server
		*/

		if( ! IPXCMPADDR(Address, Server->ServerAddress)) {
			/*
			**	Someone claims that the server's address changed!
			**	Check if source is believable
			*/
			if (CheckSapSource(Address, Source, ConnectedLAN, Hops)) {
				ShmBase->D.BadSapSource++;
#if SERVICEDEBUG
				FPRINTF(SapLogFile,"%s: %sIGNR(chk_sap_source): ", titleStr, INPKT);
				PrintSrvPacket( SapLogFile, Type, &Name[1], (ipxAddr_t *)Address,
					GETINT16(Hops));
#endif
				return;
			}
			checksrc++;

			/*
			**	We now need to determine if we can trust the source
			**	of information
			**
			**	If new source is SAP_DOWN hops, we don't trust it
			*/
			if( Hops == SAP_DOWN) {
				goto CheckInfoSource;
			}

			/*
			**	If new server address is invalid, don't trust it
			*/
			if( GetNetEntry(network, &Net) < 0) {
				/* Can't get to new address--shouldn't happen */
/* #if SERVICEDEBUG */
				FPRINTF(SapLogFile,"%sNEWSRV(no_net_entry): ", INPKT);
				PrintSrvPacket( SapLogFile, Type, &Name[1], (ipxAddr_t *)Address,
					GETINT16(Hops));
/* #endif */
				Hops = SAP_DOWN;
				goto CheckInfoSource;
			}
			netflag++;

			if (Server->HopsToServer != SAP_DOWN ) {
				/*
				**	If original server address is invalid, use the new address
				*/
				if( GetNetEntry(*((uint32 *)Server->ServerAddress), &SNet) < 0){
/* #if SERVICEDEBUG */
					FPRINTF(SapLogFile,"%sORGSRV(no_net_entry): ", INPKT);
					PrintSrvPacket( SapLogFile, Server->ServerType,
						&Server->ServerName[1], 
						(ipxAddr_t *)Server->ServerAddress,
						GETINT16(Server->HopsToServer));
/* #endif */
				} else {
					/*
					**	Both the new and the original server addresses are 
					** valid, we will chose as the best source the closest one
					**	If both are the same distance (hops), we will chose the
					**	new one.
					*/
					if( (SNet.timeToNet < Net.timeToNet)
						|| ((SNet.timeToNet == Net.timeToNet) 
							&& (SNet.hopsToNet < Net.hopsToNet))) {
						/*
						**	My old source still looks better to me
						**	Don't trust new source
						*/
						Hops = SAP_DOWN;
						goto CheckInfoSource;
					}
				}
			}
			/*
			**	A "best" source claims address change!
			*/
#if SERVICEDEBUG
			FPRINTF(SapLogFile,"%s: %sKILL(srvr_addr_chg):  ",titleStr,INPKT);
			PrintSrvPacket( SapLogFile, Server->ServerType,
				&Server->ServerName[1], (ipxAddr_t *)Server->ServerAddress,
				GETINT16(Server->HopsToServer));
#endif
			if( TrackFlag) {
				FPRINTF(SapLogFile, MsgGetStr(SAP_ADDR_CHG), titleStr, Name);
			}

			/*
			**	Reuse this Server record below.
			**	after releasing info structures
			*/
            Info = Server->SourceListLink;
			Server->SourceListLink = 0;
            while( Info != NULL) {
                NextInfo = Info->NextSource;
                NWFREE((char *)Info);
                Info = NextInfo;
            }
			goto NewServer;
		}

CheckInfoSource:
		/*
		**	We get here if we have found a server with the same name
		**	as the new one being advertised.
		**
		**	If the new server being advertised has an address different from
		**	the one in our server entry, we will only get here if we don't
		**	trust the new source.  In this case we set Hops = SAP_DOWN.
		**	We then check to see if this source is one we already know about
		**	for this server, and if so, we delete the source (InfoSourceEntry).
		**
		**	If the new server has the same address as the server in our server
		**	entry (the normal case), we update the hops if needed.  If the
		**	New hop count is "sapMaxHops", we will delete the source.
		**
		**	Note: hops are zero when we have a service on our local machine
		*/
		
		/*
		**	See if I already have the souce of this information
		*/
		for(Info=Server->SourceListLink; Info != NULL;Info = Info->NextSource) {
			/*
			**	To be the same source two conditions must be satisfied
			**	1) the source is on the same lan
			**	2) the source addresses are the same
			*/
			if( (Info->ServConnectedLAN == ConnectedLAN) && 
					IPXCMPNODE(Source, Info->SourceAddress)) {
				break;
			}
		}

		if( Info != NULL) {
			oldhops = Server->HopsToServer;
			/*
			**	I know of this info source.
			*/
			if (Hops == Info->HopsToSource) {
				/*
				**	If the hops are unchanged, make this
				**	server young again
				*/
				Info->ServTimer = 0;
#if SERVICEDEBUG
				FPRINTF(SapLogFile,"%s: %sCLRTIM(srvr_is_alive):", titleStr, INPKT);
				PrintSrvPacket( SapLogFile, Server->ServerType,
					&Server->ServerName[1],
					(ipxAddr_t *)Server->ServerAddress,
					GETINT16(Server->HopsToServer));
#endif
				/*
				**	We have finished routes for this server, we are done
				*/
				return;
			} else {
				/*
				**	Make sure source is believable
				*/
				if( (checksrc == 0)
						&& CheckSapSource(Address, Source, ConnectedLAN, Hops)){
					ShmBase->D.BadSapSource++;
#if SERVICEDEBUG
					FPRINTF(SapLogFile,"%s: %sIGNR(chk_sap_source): ",
						titleStr, INPKT);
					PrintSrvPacket( SapLogFile, Type, &Name[1],
						(ipxAddr_t *)Address, GETINT16(Hops));
#endif
					return;
				}
				checksrc++;

				/*
				**	Take out old source and update server hop count
				**	If updated source is down, release it
				*/
				KillSrvSource(Server, Info);
				if (Hops == SAP_DOWN) {
					/*
					**	If server also went down, mark changed
					*/
					if (Server->HopsToServer == SAP_DOWN) {
						Server->RevisionStamp = newStamp;;
						ShmBase->D.RevisionStamp = newStamp;
						/* Add to changed list */
						AddToChangedList( Server, "AddServer");
#if SERVICEDEBUG
						FPRINTF(SapLogFile,"%s: %sCHG(srvr_route_chg):  ", 
							titleStr, INPKT);
						PrintSrvPacket( SapLogFile, Server->ServerType,
							&Server->ServerName[1],
							(ipxAddr_t *)Server->ServerAddress,
							GETINT16(Server->HopsToServer));
#endif
					}
					/*
					**	Update source in with changed information
					**
					**	Add source back in for downed server so split horizion
					**	algorithm works, i.e. we need net it came from
					*/
					Info->HopsToSource = Hops;
					Info->ServTimer = 0;
					/*
					**	Put source back in list in best hops order
					*/
					AddSrvSource (Server, Info);
					/*
					**	We have finished routes for this server, we are done
					*/
					return;
				} else {
				oldhops = Info->HopsToSource;
					NWFREE( Info);
				}
			}
		}

		/*
		**	This is a new info source!
		*/

		/*
		**	Make sure source is believable
		*/
		if( (checksrc == 0)
				&& CheckSapSource(Address, Source, ConnectedLAN, Hops)){
			ShmBase->D.BadSapSource++;
#if SERVICEDEBUG
			FPRINTF(SapLogFile,"%s: %sIGNR(chk_sap_source): ",
				titleStr, INPKT);
			PrintSrvPacket( SapLogFile, Type, &Name[1],
				(ipxAddr_t *)Address, GETINT16(Hops));
#endif
			return;
		}

		if( Hops == SAP_DOWN) {
			/*
			**	This is a new source, but it is down, so just
			**	ignore it.
			*/
#if SERVICEDEBUG
			FPRINTF(SapLogFile,"%s: %sDROP(dead_source):", titleStr, INPKT);
			PrintSrvPacket( SapLogFile, Type, &Name[1], (ipxAddr_t *)Address,
				GETINT16(Hops));
#endif
			return;
		}

		/*
		**	Got new server info.  Need Net info for AddSrvSource.
		**	Get ticks for GetNearestServer
		*/
		if( netflag == 0) {
			if( GetNetEntry( network, &Net) < 0) {
#if SERVICEDEBUG
				FPRINTF(SapLogFile,"%sADDSRV(no_net_entry): ", INPKT);
				PrintSrvPacket( SapLogFile, Type, &Name[1], (ipxAddr_t *)Address,
					GETINT16(Hops));
#endif
				return;
			}
			netflag++;
		}

		/*
		**	Set up a new source info entry
		*/
		Info = (InfoSourceEntry_t *) NWALLOC(sizeof(InfoSourceEntry_t));

		if (Info == NULL)  {
			ShmBase->D.MallocFailed++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_INFO), titleStr);
			return;
		}

		IPXCOPYNODE( Source, Info->SourceAddress);
		Info->HopsToSource = Hops;
		Info->ServConnectedLAN = (uint8)ConnectedLAN;
		Info->ServTimer = 0;
		Info->LocalPid = 0;
		Info->N = Net;
		if( oldhops == 0) {
			oldhops = Server->HopsToServer;
		}
		/*
		**	Add to list of sources and update hop count
		*/
		AddSrvSource(Server, Info);
		if( oldhops >= sapMaxHops) {
			/*
			**	if server was previously down, mark server changed, down to up
			*/
			Server->RevisionStamp = newStamp;;
			ShmBase->D.RevisionStamp = newStamp;
			/* Add to changed list */
			AddToChangedList( Server, "AddServer");
#if SERVICEDEBUG
			FPRINTF(SapLogFile,"%s: %sCHG(srvr_gets_route): ", 
				titleStr, INPKT);
			PrintSrvPacket( SapLogFile, Server->ServerType, &Server->ServerName[1],
				(ipxAddr_t *)Server->ServerAddress, GETINT16(Server->HopsToServer));
#endif
		}
		return;
	} /* end if already know about server */


	/*
	**	We have discovered a new server
	*/
NewServer:

	/*
	**	First check if this is a believable source
	*/
	if( (checksrc == 0)
			&& CheckSapSource(Address, Source, ConnectedLAN, Hops)) {
		ShmBase->D.BadSapSource++;
#if SERVICEDEBUG
		FPRINTF(SapLogFile,"%s: %sIGNR(chk_sap_source): ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Type, &Name[1], (ipxAddr_t *)Address,
			GETINT16(Hops));
#endif
		return;
	}
	checksrc++;

	if( Hops == SAP_DOWN) {
		/*
		**	If it is down, we don't care about it
		*/
		return;
	}

	if( (Hops != 0) && IPXCMPADDR(Address, ShmBase->D.MyNetworkAddress)) {
		/*
		**	Someone else is using my address!
		**	We still add it to list
		*/
		FPRINTF(SapLogFile, MsgGetStr(SAP_DUP_ADDR), titleStr, Name);
	}

	/*
	**	If new server address is invalid, don't add it
	**	Get ticks for GetNearestServer
	*/
	if( netflag == 0) {
		if( GetNetEntry( network, &Net) < 0) {
/* #if SERVICEDEBUG */
			FPRINTF(SapLogFile,"%sADDSRV(no_net_entry): ", INPKT);
			PrintSrvPacket( SapLogFile, Type, &Name[1], (ipxAddr_t *)Address,
				GETINT16(Hops));
/* #endif */
			Hops = SAP_DOWN;
			return;
		}
	}

	/*
	**	Server != NULL, Reuse this server record from address chg above
	*/
	if( Server == NULL) {
		Server = SRVALLOC(sizeof(ServerEntry_t));
		if( Server == NULL) {
			ShmBase->D.SrvAllocFailed++;
			/*
			**	If no server entries left in memory, don't add it
			*/
			FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_SRVENT), titleStr);
			/* No space */
#if SERVICEDEBUG
			FPRINTF(SapLogFile,"%s: %sDROP(no_srv_entries)  ", titleStr, INPKT);
			PrintSrvPacket( SapLogFile, Type, &Name[1], (ipxAddr_t *)Address, Hops);
#endif
			return;
		}
		/*
		**	Fill in enough of packet so we can't get confused
		*/
		Server->SourceListLink = 0;
		Server->HopsToServer = SAP_DOWN;
		Server->LocalPid = 0;
		Server->RevisionStamp = 0;
		Server->ServerType = Type;
		memcpy(Server->ServerName, Name, *Name + 1);
		/*
		**	Make sure at least NAME_HASH_LENGTH in Name has predictible data
		*/
		if( *(int8 *)Name < NAME_HASH_LENGTH) {
			memset(&Server->ServerName[*Name +1], '\0', NAME_HASH_LENGTH - *Name);
		} else {
			Server->ServerName[*Name + 1] = '\0';
		}

		/*
		**	Add to hash table
		*/
		hash = GetNameHash(Name);
		HashEnt = nHashBase + hash;
#ifdef SAPDEBUG
		{
			/*
			**	DEBUG check, make sure we don't link to ourselves
			*/
			ServerEntry_t *tsrv;
			for(tsrv = SrvBase + *HashEnt; tsrv != SrvBase; tsrv = SrvBase + tsrv->NameLink) {
				if( (tsrv - SrvBase) == (Server - SrvBase) ) {
					FPRINTF(SapLogFile,"%s: Abort AddServer: Hash 0x%X, Server 0x%X, Entry 0x%X, Link 0x%X, *Hashent, name %s\n",
						titleStr, hash, Server, Server - SrvBase, Server->NameLink, *HashEnt, &Name[1]);
					abort();
				}
			}
		}
#endif
		Server->NameLink = *HashEnt;
		*HashEnt = Server - SrvBase;
#if HASHDEBUG
		FPRINTF(SapLogFile,"%s: AddServer: Hash 0x%X, Server 0x%X, type 0x%X, Entry 0x%X, Link 0x%X, name %s\n",
			titleStr, hash, Server, GETINT16(Type),
			Server - SrvBase, Server->NameLink, &Name[1]);
#endif

#if SERVICEDEBUG
		FPRINTF(SapLogFile,"%s: %sADD(new_server):      ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Server->ServerType, &Server->ServerName[1],
			(ipxAddr_t *)Address, GETINT16(Hops));
	} else {
		FPRINTF(SapLogFile,"%s: %sCHG(upd_server):      ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Server->ServerType, &Server->ServerName[1],
			(ipxAddr_t *)Address, GETINT16(Hops));
#endif
	}

	Info = (InfoSourceEntry_t *)NWALLOC(sizeof(InfoSourceEntry_t));
	if( Info == NULL) {
		ShmBase->D.MallocFailed++;
		/*
		**	If no malloc space left, don't add it
		*/
		FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_INFO), titleStr);
#if SERVICEDEBUG
		FPRINTF(SapLogFile,"%s: %sDROP(no_info_malloc)  ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Server->ServerType,
			&Server->ServerName[1], (ipxAddr_t *)Address, GETINT16(Hops));
#endif
		return;
	}

	IPXCOPYADDR(Address, Server->ServerAddress);
	Server->HopsToServer = Hops;
	Server->N.netIDNumber = Net.netIDNumber;	/* Save Net Info */
	Server->N.timeToNet = Net.timeToNet;
	Server->N.hopsToNet = Net.hopsToNet;
	Server->N.netStatus = Net.netStatus;
	Server->N.lanIndex = Net.lanIndex;
#if SERVICEDEBUG
		FPRINTF(SapLogFile,"%s: %s ADD(add_netinfo)    ", titleStr, INPKT);
#endif

	Server->RevisionStamp = newStamp;;
	ShmBase->D.RevisionStamp = newStamp;
	AddToChangedList( Server, "AddServer");
	Server->SourceListLink = Info;

	Info->NextSource = 0;
	IPXCOPYNODE( Source, Info->SourceAddress);
	Info->HopsToSource = Hops;
	Info->ServConnectedLAN = (uint8)ConnectedLAN;
	Info->ServTimer = 0;
	Info->LocalPid = 0;
	Info->N = Net;	/* Copy all netInfo data */

	return;
}

/*********************************************************************/

/*
**	Check to see if the service reporter is someone we should listen to
**	for information about the LAN under consideration!
**	Returns -1 - if something wrong with packet, ioctl returned NAK
**	result = -1 if sap source is no good
**	result = 0  if sap source is OK
*/
STATIC int 
CheckSapSource( 
		uint8 *ServerAddress,
		uint8 *ReporterAddress,
		uint32 ConnectedLAN,
		uint16 Hops)
{
	struct strioctl ioc;
	checkSapSource_t ioctlData;

	IPXCOPYADDR( ServerAddress, ioctlData.serverAddress);
	IPXCOPYNODE( ReporterAddress, ioctlData.reporterAddress);

	ioctlData.connectedLan = ConnectedLAN;
	ioctlData.hops = Hops;
	ioctlData.result = 0;

	ioc.ic_cmd = RIPX_CHECK_SAP_SOURCE;
	ioc.ic_timout = 5;
	ioc.ic_len = sizeof(checkSapSource_t);
	ioc.ic_dp = (char *)&ioctlData;

	if ( ioctl(ripxFd, I_STR, &ioc) == -1) {
		/*
		**	We should never get this, if we got it once we will always get it
		**	It indicates a coding error
		*/
		FPRINTF(SapLogFile, MsgGetStr(SAP_CHK_SRC), titleStr);
		logperror("");
		SapExit(-1);
	}

	return(ioctlData.result);
}

/*********************************************************************/
/*
**	The KillSrvSource is called from AgeServers and AddServers
**	It removes the source (identified by *Info) from the list of sources
**	and updates the hop count appropriately.  If no sources are left,
**	the Hops are set to SAP_DOWN.
*/
STATIC void 
KillSrvSource(
		ServerEntry_t *Server,
		InfoSourceEntry_t *Info)
{
	InfoPtrPtr_t *Last;
	InfoSourceEntry_t *Next;

	Last = (InfoPtrPtr_t *)&(Server->SourceListLink);
	while( Last->InfoPtr != NULL) {
		if( Last->InfoPtr == Info) {
			Last->InfoPtr = Info->NextSource;
			goto StopLoop;
		}
		Next = Last->InfoPtr;
		Last = (InfoPtrPtr_t *)&(Next->NextSource);
	}

StopLoop:
	Next = Server->SourceListLink;
	if( Next == NULL) {
		/* We just lost our last route to the server */
		Server->HopsToServer = SAP_DOWN;
		Server->N.timeToNet = 255;
	} else {
		Server->HopsToServer = Next->HopsToSource;
		Server->LocalPid = Next->LocalPid;
		Server->N = Next->N;	/* Copy netInfo_t data */
	}
	if( Server->HopsToServer >= sapMaxHops) {
		Server->HopsToServer = SAP_DOWN;
		Server->N.timeToNet = 255;
	}

	/*	Don't get excited about distance changes:  except for
	 *	"sapMaxHops", no one really cares about these.  The server's address
	 *	is used to determine how far away (in time & hops) the target
	 *	server really is.  If the distance changed without going down,
	 *	then other servers will find out in subsequent broadcasts.
	 */
	return;
}

/*********************************************************************/
/*
**	The AddSrvSource is called from AddServer
**	It adds the source (identified by *Info) to the list of sources
**	in hop order and updates the hop cound appropriately.
*/
STATIC void 
AddSrvSource(
		ServerEntry_t *Server,
		InfoSourceEntry_t *Info)
{
	InfoPtrPtr_t *Last;
	InfoSourceEntry_t *Next;

	Last = (InfoPtrPtr_t *)&(Server->SourceListLink);
	Next = Server->SourceListLink;
	while ((Next != NULL) && (Next->HopsToSource < Info->HopsToSource)) {
		Last = (InfoPtrPtr_t *)&(Next->NextSource);
		Next = Next->NextSource;
	}

	Info->NextSource = Next;
	Last->InfoPtr = Info;

	Next = Server->SourceListLink;
	if( Server->HopsToServer != Next->HopsToSource) {
		Server->HopsToServer = Next->HopsToSource;
		Server->LocalPid = Next->LocalPid;
		Server->N = Next->N;	/* Copy netInfo_t data */
		if( Server->HopsToServer >= sapMaxHops) {
			Server->HopsToServer = SAP_DOWN;
		}
#if SERVICEDEBUG
		FPRINTF(SapLogFile,"%s: %sCHG(add_route):       ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Server->ServerType,
			&Server->ServerName[1],
			(ipxAddr_t *)Server->ServerAddress,
			GETINT16(Server->HopsToServer));
#endif
		}
	return;
}

#ifdef SAPDEBUG
/*********************************************************************/
/*ARGSUSED*/
STATIC void
Ignore(int sig)
{
	FPRINTF(SapLogFile, "Received signal %d, ignored\n", sig);
	return;
}
#endif

/*********************************************************************/
/*ARGSUSED*/
STATIC void
AlarmHandler(int sig)
{
	static	numServers = 0;
	static	numServerTries = 0;

	time( &lastAlarmHit );

	if( initialized == 3) {
		/*
		**	Normal case after initialization complete
		*/
		GotAlarm = TRUE;
		validTime = 0;
	} else {
		/*
		**	Pre initialization action
		**	We are up when servers count don't change
		*/
		if( numServers != ShmBase->D.ServerPoolIdx) {
			/*
			**	Number of servers has changed
			*/
			numServers = ShmBase->D.ServerPoolIdx;
			initialized = 0;
		} else {
			/*
			**	Same as last time, increment initialized count
			*/
			initialized++;
		}
		if( initialized < 2) {
			/*
			**	Wait some more if not initialized
			*/
			alarm(SAP_ALARM_TIMER);
		} else {
			/*
			**	Initialize sapd
			**	If no servers at this point (SAP_INIT_TIMER + SAP_ALARM_TIMER),
			**	resend the GSQ,	wait a little longer, then give up
			*/
			if( (numServers == 1) && (numServerTries++ < 5)) {
				if(numServerTries == 1)
				{
					alarm(SAP_INIT_TIMER);
					SendDumpSapInfoPacket();
					initialized = 0;
				} else {
					alarm(SAP_ALARM_TIMER);
					initialized = 2;
				}
				return;
			}

			/*
			**  If not already done, Set up a pid file for the sapd process.
			**	Npsd uses it pid file as a indicator that we are initialized
			*/

			if(SapFastInit == FALSE) {
				if(WritePid())
					SapExit(-1);
			}

			validTime = 0;
			FPRINTF( SapLogFile, MsgGetStr(SAP_INITIALIZED), titleStr);

			/*
			**	Set real periodic interval
			*/
			alarm( sapTimerInterval);
			initialized = 3;
		}
	}
	return;
}

/*********************************************************************/
STATIC int
WritePid(void)
{
	char *PidLogDir;

	if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL) {
		FPRINTF( SapLogFile, MsgGetStr(SAP_BAD_CONFIG), titleStr);
		return(-1);
	}

	if (LogPidToFile(PidLogDir, (char *)program, SAPPid)) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_NOLOG), titleStr);
		return(-1);
	}
	return( 0 );
}

/*********************************************************************/
STATIC void
AlarmService(void)
{
	GotAlarm = FALSE;

	AgeServers();

	alarm(sapTimerInterval);
	return;
}

/*********************************************************************/
/*
 *	If there is a higher priority process running on the system
 *	we usually lose the alarm signal.  So we "piggyback" on top
 *	of the ReadPacket.  Each time we get a packet in we check to
 *	make sure that we haven't lost the alarm signal.  If we have we pretend
 *	we are the alarm and reset the signal.
 */
STATIC void
CheckForLostSignal(void)
{
	time_t secondsPassed;
	time_t now;
	
	time(&now);

	if (now>=lastAlarmHit)
		secondsPassed = now - lastAlarmHit;
	else /* rollover */
		secondsPassed = (0xFFFFFFFF - lastAlarmHit) + now + 1;
	if( (initialized < 3) && (secondsPassed >= (SAP_RESTART_FACTOR * 3))) {
		/*
		**	Call Alarm Handler now if not initialized
		*/
		FPRINTF(SapLogFile, MsgGetStr(SAP_ALARM), titleStr,
			secondsPassed, 3, SAP_RESTART_FACTOR);
		AlarmHandler(0);
	} else if (secondsPassed >= 
		(time_t)(SAP_RESTART_FACTOR * sapTimerInterval)) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_ALARM), titleStr,
			secondsPassed, sapTimerInterval, SAP_RESTART_FACTOR);
		AlarmHandler(0);
	}
	return;
}
				
/*********************************************************************/

STATIC void 
AgeServers(void)
{
	ServerEntry_t			*Server;
	InfoSourceEntry_t		*Info,
							*NextInfo,
							*SaveInfo;
	netInfo_t				*Net;
	uint32					 count;
	int						 svrIndex;
	int						 lan;
	uint32					maxAge;


	newStamp = ShmBase->D.RevisionStamp + 1;

#if SERVICEDEBUG
	FPRINTF(SapLogFile,"%s: enter age Code\n", titleStr);
#endif

	svrIndex = 1;
	for( Server = SrvBase + svrIndex; svrIndex <= ShmBase->D.ConfigServers;
			Server = SrvBase + svrIndex) {
		svrIndex++;

		if( Server->HopsToServer >= sapMaxHops)
			continue;

		if( Server->LocalPid != 0) {
			/*
			**	Check if local process is still alive
			*/
			if( kill( Server->LocalPid, 0) == -1) {
				/*
				**	Process dead, down server, release all info structures
				*/
				Info = Server->SourceListLink;
				while( Info != NULL) {
					NextInfo = Info->NextSource;
					NWFREE((char *)Info);
					Info = NextInfo;
				}
				FPRINTF(SapLogFile, MsgGetStr(SAP_AGE_LOCAL), titleStr);
				FPRINTF(SapLogFile, MsgGetStr(SAP_SERVER_INFO),
					GETINT16(Server->ServerType), 
					Server->LocalPid, &Server->ServerName[1]);
				Server->SourceListLink = 0;
				/* Lost this server */
				Server->HopsToServer = SAP_DOWN;
				Server->RevisionStamp = newStamp;
				Server->LocalPid = 0;
				ShmBase->D.RevisionStamp = newStamp;
				/* Add to changed list */
				AddToChangedList( Server, "AgeServers");
#if SERVICEDEBUG
				FPRINTF(SapLogFile,"%s: %sCHG(local_dead):      ", 
					titleStr, INPKT);
				PrintSrvPacket( SapLogFile, Server->ServerType,
					&Server->ServerName[1],
					(ipxAddr_t *)Server->ServerAddress,
					GETINT16(Server->HopsToServer));
#endif
				
			}
			continue;
		}
		/* Don't check for HopsToServer == 0, it is ok for NWU servers to
		**	have zero hops, but not for native
		*/
		SaveInfo = NULL;
		Info = Server->SourceListLink;
		while( Info != NULL) {
			if( Info->HopsToSource == SAP_DOWN) {
				/*	We have found downed sources, look no further */
				break;
			}
			lan = Info->ServConnectedLAN;
			Net = LocalNet[lan];
			maxAge = lanSapInfo[lan].maxAge;

			if( Net == NULL) {
				Info->ServTimer = maxAge;
			} else {
				/* 
				** if periodic Broadcast increment Age Timer
				** if no periodic Broadcast (SEND_CHANGE_ONLY) do not
				**  Age any information
				*/
				if (!(lanInfoTable[lan].ripSapInfo.sap.actions 
						& SAP_UPDATES_ONLY)) {
					Info->ServTimer++;
				}
			}

			/*
			**	Remove the route, when we do the split horizion,
			**	we need to tell all our routes because we aged it
			**	and thus become the best source
			*/
			if( Info->ServTimer >= maxAge) {
				/*
				**	Save killed sources on a private list
				**	We will add them back later for split horizion
				*/
				NextInfo = Info->NextSource;
				KillSrvSource(Server, Info);
				Info->NextSource = SaveInfo;
				SaveInfo = Info;
				Info = NextInfo;
			} else {
				Info = Info->NextSource;
			}
		}

		/*	Did we loose this server ? */
		if( Server->HopsToServer >= sapMaxHops) {
#if SERVICEDEBUG
			FPRINTF(SapLogFile,"%s: %sCHG(aged):            ", 
					titleStr, INPKT);
			PrintSrvPacket( SapLogFile, Server->ServerType,
				&Server->ServerName[1],
				(ipxAddr_t *)Server->ServerAddress,
				GETINT16(Server->HopsToServer));
#endif
			/* Lost this server */
			Server->HopsToServer = SAP_DOWN;
			Server->RevisionStamp = newStamp;
			ShmBase->D.RevisionStamp = newStamp;
			/* Add to changed list */
			AddToChangedList( Server, "AgeServers");
		}

		/*
		**	Add sources back in so split horizion works
		**	and we know which net it came in on
		*/
		Info = SaveInfo;
		while( Info != NULL) {
			NextInfo = Info->NextSource;
			Info->HopsToSource = SAP_DOWN;
			Info->ServTimer = 0;
			AddSrvSource (Server, Info);
			Info = NextInfo;
		}
	}
	/*
	**	Notify local processes of any changes
	*/
	NotifyProcessOfChanges();

	/*
	**	Don't send info on internal lan
	*/
	for ( count = 1; count < ipxConfiguredLans; count++) {
		Net = LocalNet[count];
		if( Net == NULL)
			continue;

		lanSapInfo[count].bcstTimer++;

		/*
		**	Send CHANGED_SERVICES every time, only send ALL_SERVICES if
		**	time for a periodic Broadcast and SEND_CHANGE_ONLY is not set.
		*/
		if( lanSapInfo[count].bcstTimer >= lanSapInfo[count].maxBcst) {
			if(lanInfoTable[count].ripSapInfo.sap.actions
					& SAP_UPDATES_ONLY) {
				if( ChangedLink == 0)
					continue;
				SendChangedServerInformation( count, Net->netIDNumber, ALLHOSTS,
					GETINT16(SAP_SAS), PACKET_DELAY, CHECK_SPLIT_HORIZ);
			} else {
				/* Make sure changes are sent, including down servers */
				if( ChangedLink != 0) {
					SendChangedServerInformation( count, Net->netIDNumber,
						ALLHOSTS, GETINT16(SAP_SAS), PACKET_DELAY,
						CHECK_SPLIT_HORIZ);
				}
				/* Send all tables to regular networks */
				SendKnownServerInformation( count, Net->netIDNumber,
					ALLHOSTS, GETINT16(SAP_SAS), PACKET_DELAY,
					CHECK_SPLIT_HORIZ, (uint16)ALL_SERVER_TYPE);
			}
			lanSapInfo[count].bcstTimer = 0;
		} else {
			if( ChangedLink == 0)
				continue;
			SendChangedServerInformation( count, Net->netIDNumber,
				ALLHOSTS, GETINT16(SAP_SAS), PACKET_DELAY, CHECK_SPLIT_HORIZ);
		}
	}

	/*
	**	Clear list of changed entries
	*/
#if CHANGEDEBUG
	FPRINTF( SapLogFile, "%s: AgeServers: Zero the change list was 0x%X\n",
		titleStr, ChangedLink);
#endif
	PruneSrvSource();
	ChangedLink = 0;

	return;
}

	
/*********************************************************************/
/*
**	Process a General Services Reply packet
*/
STATIC void 
AcceptServerInformation(
		uint32 ConnectedLAN,
		ServerPacketStruct_t *Packet,
		uint8 *PacketSource)
{
	uint16				  hops,
						  servType;
	int32			 	  count;
	uint8				 *name;
	int 			 	  len;
	ServPacketInfo_t	 *Target;
	netInfo_t			 *Net;

	/*
	**	If this was sent from lan 0, set appropriate lan
	**	Make it the same as destination lan, using ConnectedLAN index
	*/
	if( PGETINT32(Packet->ipxHdr.src.net) == 0) {
		/* This came from a local server */
		Net = LocalNet[ConnectedLAN];
		if (Net == NULL) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_RESOLVE), titleStr, ConnectedLAN);
			/* Can't resolve net number */
			return;
		}
		GETALIGN32(Net->netIDNumber, Packet->ipxHdr.src.net);
	}

	/*
	**	Compute number of ServPacketInfo structures in this packet
	*/
	count = (PGETINT16(&(Packet->ipxHdr.len)) - IPX_HDR_SIZE) /
			sizeof(ServPacketInfo_t);

	/*
	**	Process each ServPacketInfo structure in this packet
	*/
	for( Target = Packet->ServerTable; count > 0; Target++, count--) {
		if( PGETINT32((uint32 *)Target->TargetAddress) == 0) {
			IPXCOPYNET(Packet->ipxHdr.src.net, Target->TargetAddress);
		}
		hops = PGETINT16(&(Target->ServerHops));
		servType = Target->TargetType;
		/*
		**	Make sure name is null terminated, can only have 47 chars anyway
		*/
		name = Target->TargetServer;
		name[ NWMAX_SERVER_NAME_LENGTH - 1] = '\0';

		/*
		**	Convert name to length-preceded, clobber Target->TargetType
		*/
		len = strlen((int8 *)name);
		*(--name) = (uint8)len;

		/*
		**	Make sure at least NAME_HASH_LENGTH in name has predictible data
		*/
		if( *(int8 *)name < NAME_HASH_LENGTH) {
			memset(&name[*name + 1], '\0', NAME_HASH_LENGTH - *name);
		}

		/*
		**	If this server has too short a name, ignore server
		*/
		if( len < 2) {
#ifdef NOT_NOW
			FPRINTF(SapLogFile,
			"Server advertising with no name, type 0x%X, hops %d, address %s\n",
				GETINT16(servType), hops,
				FormatIpxAddress( (ipxAddr_t *)Target->TargetAddress));
#endif
#if SERVICEDEBUG
			FPRINTF(SapLogFile,"%s: %sDROP(server w/o name):", titleStr, INPKT);
			PrintSrvPacket( SapLogFile, servType, &name[1],
				(ipxAddr_t *)Target->TargetAddress, hops);
#endif
			continue;
		}

		/*
		**	Just add server, without checking for hops == zero
		**	since we are the transport SAP and we must add any services
		**	in our machine, i.e., hops can be zero.
		*/
		AddServer(ConnectedLAN, name, Target->TargetAddress,
				servType, hops, PacketSource);
	}

	/*
	**	Notify local processes if anything changed
	*/
	NotifyProcessOfChanges();

	/*
	**	If no changed servers, we are done
	*/
	if( ChangedLink == 0) {
		return;
	}

	/*
	**	Now propogate any updated services to our other lans
	**	Don't send info on internal lan
	*/
	for ( count = 1; count < ipxConfiguredLans; count++) {
		Net = LocalNet[count];
		if (Net != NULL) {
			SendChangedServerInformation( count, Net->netIDNumber,
				ALLHOSTS, GETINT16(SAP_SAS), PACKET_DELAY,
				CHECK_SPLIT_HORIZ);
		}
	}

	/*
	**	All changes propogated, empty list of changed servers
	*/
#if CHANGEDEBUG
	FPRINTF( SapLogFile, "%s: AcceptServerInformation: Zero the change list was 0x%X\n",
	titleStr, ChangedLink);
#endif
	PruneSrvSource();
	ChangedLink = 0;

	return;
}

/*********************************************************************/
/*
**	Process Get Nearest Server Query
*/
STATIC void 
GetNearestServer(
		uint32 ConnectedLAN,
		ServerPacketStruct_t *packet)
{
	ServerEntry_t		*Server, *Best;
	InfoSourceEntry_t	*Source, *Next;
	uint16 SearchType;
	netInfo_t			 BNet;
	ServPacketInfo_t	*Target;
	int				  	 svrIndex;
	int				  	 count;
	uint32				 net;
	uint16				 sock;
    netInfo_t           *Net;

	if( ShmBase->D.ServerPoolIdx == 1) {
		/* No good sending info you don't have */
		return;
	}

	/*
	**	If this was sent from lan 0, set appropriate lan
	**	Make it the same as destination lan, using ConnectedLAN index
	*/
	if( PGETINT32(packet->ipxHdr.src.net) == 0) {
		/* This came from a local server */
		Net = LocalNet[ConnectedLAN];
		if (Net == NULL) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_RESOLVE), titleStr, ConnectedLAN);
			/* Can't resolve net number */
			return;
		}
		GETALIGN32(Net->netIDNumber, packet->ipxHdr.src.net);
	}

	newStamp = ShmBase->D.RevisionStamp + 1;

	SearchType = packet->ServerTable[0].TargetType;

restartNSQ:
	/*
	**	Find the nearest server to me
	*/
	Best = (ServerEntry_t *)NULL;
	svrIndex = 1;
	for( Server = SrvBase + svrIndex; svrIndex <= ShmBase->D.ConfigServers;
			Server = SrvBase + svrIndex) {
		svrIndex++;
		/*
		**	Ignore server if wrong type
		*/

		if( (Server->ServerType != SearchType)
				|| (Server->HopsToServer == SAP_DOWN)) {
			continue;
		}
#if NSQDEBUG
		FPRINTF(SapLogFile,
			"%s: NSR(check server): tics %d hopstoServer %d name \"%s\"\n",
			titleStr, Server->N.timeToNet, 
			Server->HopsToServer, &Server->ServerName[1]);
#endif

		/*
		**	If we find a local server, it is the best
		*/
		if( (Server->HopsToServer <= 1) &&
			(memcmp(Server->ServerAddress, ShmBase->D.MyNetworkAddress,
				IPX_NET_SIZE + IPX_NODE_SIZE) == 0)) {

			/* This server lives in this machine! */
			Best = Server;
			goto BestServerFound;
		}

		/*
		**	Start off with first server in list
		*/
		if (Best == NULL) {
			Best = Server;
		}

		/*
		**	If time is less than hops, consider the time bogus and skip entry
		*/
		if( Server->N.timeToNet < (uint16)Server->N.hopsToNet) {
			continue;
		}
		/*
		**	New server is best server if
		**		time of new is less
		**	or 
		**		times is equal 
		**			and
		**		hops of new is less	
		**
		*/
		if ((Best->N.timeToNet > Server->N.timeToNet)
				|| ((Best->N.timeToNet == Server->N.timeToNet)
					&& (Best->N.hopsToNet > Server->N.hopsToNet))) {
			Best = Server;
		}
	}

	if (Best == NULL) {
		/* No server to report */
		goto SkipWrite;
	}

	/*
	**  See if the net is valid
	*/
	if( GetNetEntry( *((uint32 *)Best->ServerAddress), &BNet) < 0) {
#if SERVICEDEBUG | NSQDEBUG
		FPRINTF(SapLogFile,"%sNSQ(no_net_entry):   ", INPKT);
		PrintSrvPacket( SapLogFile, Best->ServerType, &Best->ServerName[1],
			(ipxAddr_t *)Best->ServerAddress, GETINT16(Best->HopsToServer));
#endif

		/*
		**  Invalidate Sources
		*/
		Source = Best->SourceListLink;
		Best->SourceListLink = 0;
		while( Source != NULL ) {
			Next = Source->NextSource;
			NWFREE((char *)Source);
			Source = Next;
		}
		/*
		**	Mark Server Down
		*/
		Best->SourceListLink = NULL;
		Best->HopsToServer = SAP_DOWN;
		Best->N.timeToNet = 255;
		Best->RevisionStamp = newStamp;
		ShmBase->D.RevisionStamp = newStamp;
		/* Add to list of changed servers */
		AddToChangedList( Server, "GetNearestServer");
#if SERVICEDEBUG | NSQDEBUG
		FPRINTF(SapLogFile,"%s: %sCHG(NSQ-NoNetEntry):  ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Best->ServerType, &Best->ServerName[1],
			(ipxAddr_t *)Best->ServerAddress, GETINT16(Best->HopsToServer));
#endif

		/*
		**	Advertise that this server is down
		*/
		NotifyProcessOfChanges();
		GETALIGN16( packet->ipxHdr.src.sock, &sock);
		GETALIGN32( packet->ipxHdr.src.net, &net);
		/*
		**	Don't send info on internal lan
		*/
		for ( count = 1; count < ipxConfiguredLans; count++) {
			Net = LocalNet[count];
			SendChangedServerInformation( count, Net->netIDNumber,
				ALLHOSTS, GETINT16(SAP_SAS), NO_PACKET_DELAY,
				NO_SPLIT_HORIZ);
		}
#if CHANGEDEBUG
	FPRINTF( SapLogFile, "%s: GetNearestServer: Zero the change list was 0x%X\n",
		titleStr, ChangedLink);
#endif
		PruneSrvSource();
		ChangedLink = 0;
#if NSQDEBUG
		FPRINTF(SapLogFile,"%s: %sNSR(no_net_for_best): ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Best->ServerType, &Best->ServerName[1],
			(ipxAddr_t *)Best->ServerAddress, GETINT16(Best->HopsToServer));
#endif
		goto restartNSQ;
	} else {
		/*
		**	If time to net has changed for the worse, find a different server
		**	Don't forget to adjust timeToNet info in source.
		*/
		if( BNet.timeToNet > Best->N.timeToNet) {
#if NSQDEBUG
			FPRINTF(SapLogFile,"%s: %sNSR(ticks have chgd): old %d new %d: restart:",
				titleStr, INPKT, Best->N.timeToNet, BNet.timeToNet);
			PrintSrvPacket( SapLogFile, Best->ServerType, &Best->ServerName[1],
				(ipxAddr_t *)Best->ServerAddress, GETINT16(Best->HopsToServer));
#endif
			Best->N.timeToNet = BNet.timeToNet;
			goto restartNSQ;
		} else {
			if( BNet.timeToNet < Best->N.timeToNet) {
#if NSQDEBUG
			FPRINTF(SapLogFile,"%s: %sNSR(ticks have chgd): old %d new %d: still best:",
				titleStr, INPKT, Best->N.timeToNet, BNet.timeToNet);
			PrintSrvPacket( SapLogFile, Best->ServerType, &Best->ServerName[1],
				(ipxAddr_t *)Best->ServerAddress, GETINT16(Best->HopsToServer));
#endif
				Best->N.timeToNet = BNet.timeToNet;
			}
		}
	}

	if( IPXCMPNODE( packet->ipxHdr.dest.node, ALLHOSTS)) {
		/* Broadcast--maybe my best source can answer itself. */

		Source = Best->SourceListLink;
		while ((Source != NULL) && (Source->HopsToSource ==
				Best->HopsToServer)) {
			if( Source->ServConnectedLAN == ConnectedLAN) {
#if NSQDEBUG
			FPRINTF(SapLogFile,"%s: %sNSR(not_best_src):    type %d:",
				titleStr, INPKT, SearchType);
			PrintSrvPacket( SapLogFile, Best->ServerType, &Best->ServerName[1],
				(ipxAddr_t *)Best->ServerAddress, GETINT16(Best->HopsToServer));
#endif
				goto SkipWrite;
			}
			/* don't report--my 'best' path will report itself */
			Source = Source->NextSource;
		}
	}

BestServerFound:
	/*
	**	fill in response packet
	*/
	SearchType = packet->ServerTable[0].TargetType;
	sendBufPtr->ipxHdr.chksum = (uint16)IPX_CHKSUM;
	PPUTINT16( IPX_HDR_SIZE + sizeof(ServPacketInfo_t)
		+ sizeof(sendBufPtr->Operation),
			&(sendBufPtr->ipxHdr.len));
	sendBufPtr->ipxHdr.tc = 0;
	sendBufPtr->ipxHdr.pt = SAP_PACKET_TYPE;
	IPXCOPYADDR(packet->ipxHdr.src.net, sendBufPtr->ipxHdr.dest.net);

	/* Record destination for packet */
	PPUTINT16(SAP_SAS, sendBufPtr->ipxHdr.src.sock);
	/* Response packet */
	PPUTINT16(SAP_NSR, &(sendBufPtr->Operation));

	Target = sendBufPtr->ServerTable;
	Target->TargetType = Best->ServerType;
	memcpy( Target->TargetServer, &(Best->ServerName[1]), Best->ServerName[0]);
	memset(&(Target->TargetServer[Best->ServerName[0]]), 0,
			NWMAX_SERVER_NAME_LENGTH - Best->ServerName[0]);
	IPXCOPYADDR(Best->ServerAddress, Target->TargetAddress);
	PPUTINT16( (Best->HopsToServer >= sapMaxHops) ?
			SAP_DOWN : Best->HopsToServer + 1,
			&(Target->ServerHops));

#if NSQDEBUG
	FPRINTF(SapLogFile,"%s: %sNSR(found_best_srvr): ", 
		titleStr, INPKT);
	PrintSrvPacket( SapLogFile, Best->ServerType, &Best->ServerName[1],
		(ipxAddr_t *)Best->ServerAddress, GETINT16(Best->HopsToServer));
#endif

	TrackServerPacket(ConnectedLAN, sendBufPtr, FALSE);
	WriteSapPacket( sendBufPtr);
	ShmBase->D.NSRSent++;
	return;

SkipWrite:
#if NSQDEBUG
	FPRINTF(SapLogFile,"%s: %sNSR(no_server_found): type 0x%X, %d servers checked\n", 
		titleStr, INPKT, GETINT16(SearchType), svrIndex - 1);
#endif
	return;
}

/*********************************************************************/
/*
**	This performs tracking if enabled
*/
STATIC void
TrackServerPacket( 
		uint32 ConnectedLAN,
		ServerPacketStruct_t *packet,
		uint32 IN)
{
	uint8 *msg1, *msg2;
	uint32 i, count;
	uint16 Operation;
	ServPacketInfo_t *ServInfo;
	time_t now;
	struct tm *tp;

	/*
	**	Tracking is not internationalized
	*/

	if (!TrackFlag) {
		/* Not showing */
		return;
	}

	time(&now);
	tp = localtime(&now);

	FPRINTF(SapTrackFile,"%s [%08X:%08X%04X] %2u:%02u:%02u%cm",
			IN ? "IN " : "OUT", 
			GETINT32(LocalNet[ConnectedLAN]->netIDNumber),
			PGETINT32((uint32 *)(IN ? packet->ipxHdr.src.node :
					packet->ipxHdr.dest.node)),
			PGETINT16((uint16 *)(IN ? &(packet->ipxHdr.src.node[IPX_NET_SIZE]) :
			&(packet->ipxHdr.dest.node[IPX_NET_SIZE]))),
		tp->tm_hour > 12 ? tp->tm_hour - 12 : (tp->tm_hour == 0 ?
		12 : tp->tm_hour), tp->tm_min, tp->tm_sec,
		tp->tm_hour > 11 ? 'p' : 'a');

	Operation = PGETINT16(&(packet->Operation));
	if (Operation != 2)
	{
		msg2 = (uint8 *)"";
		switch (Operation)
		{
		case 1:
			msg1 = (uint8 *)"Send All Server Info";
			break;

		case 3:
			msg1 = (uint8 *)"Get Nearest Server";
			break;

		case 4:
			msg1 = (uint8 *)"Give Nearest Server";
			ServInfo = packet->ServerTable;
			msg2 = ServInfo->TargetServer;
			break;

		case 9:
			msg1 = (uint8 *)"NWU IPC Set/UnSet Packet";
			msg2 = (uint8 *)NULL;
			break;

		default:
			msg1 = (uint8 *)"Unknown Packet Type!";
			break;
		}
		FPRINTF(SapTrackFile,"   %s %s\n",
				msg1, msg2);
		return;
	}


	i = 23 + 18;
	count = (int)(PGETINT16(&(packet->ipxHdr.len)) -
		IPX_HDR_SIZE) / SAP_INFO_LENGTH;
	ServInfo = packet->ServerTable;
	while (count-- > 0)
	{
		FPRINTF(SapTrackFile,"   %-12.12s %2d",
				ServInfo->TargetServer,
				PGETINT16(&(ServInfo->ServerHops)));

		i += 18;
		if ((i > 60) || (count == 0))
		{
			/* Line full */
			FPRINTF(SapTrackFile,"\n");
			if (count != 0)
				FPRINTF(SapTrackFile,"  ");
			i = 5;
		}
		ServInfo++;
	}
	return;
}

/******************************************************************/
STATIC int 
SetLanSapInfo(void)
{
	int		lan;
	int		gcd;	/* greatest common denominator of all lans sapBcstIntvs */
	int		found;

	lanSapInfo = 
		(lanSapInfo_t *)NWALLOC(ipxConfiguredLans * sizeof(lanSapInfo_t));
	if (!lanInfoTable)
		return(FAILURE);

	for( gcd=10; gcd > 1; gcd--) {
		for( lan = 1; lan < ipxConfiguredLans; lan++) {
			if(lanInfoTable[lan].ripSapInfo.sap.bcastInterval
					 % (uint16)gcd) {
				found = 0;
				break;
			} else {
				found = 1;
			}
		}
		if( found)
			break;
	}

	sapTimerInterval = gcd * 30;
#ifdef SAPDEBUG
	FPRINTF(SapLogFile,"SetLanSapInfo: setting TIME interval to %d\n",
		sapTimerInterval);
#endif
	for( lan = 0; lan < ipxConfiguredLans; lan++) {
		/*
		**  set number of timer intervals before sending periodic broadcast. 
		*/
		lanSapInfo[lan].maxBcst = 
			(lanInfoTable[lan].ripSapInfo.sap.bcastInterval
				/ (uint16)gcd);
		/*
		**  set number of timer intervals before marking Server Aged. 
		**	 ((sapBcstIntvs * sapAgeIntvs) / greatest common denominatior)
		*/
		lanSapInfo[lan].maxAge =
			((uint32)(lanInfoTable[lan].ripSapInfo.sap.bcastInterval
				* lanInfoTable[lan].ripSapInfo.sap.ageOutIntervals)
				/ (uint32)gcd);
		/*
		**  change the configured gap (millisec) init micro seconds
		*/
		lanSapInfo[lan].gap = (lanInfoTable[lan].ripSapInfo.sap.interPktGap * 1000);
#ifdef SAPDEBUG
	FPRINTF(SapLogFile,"SetLanSapInfo: LAN%d  BcstIntv %d, AgeIntv %d, gap %d\n",
			lan, lanSapInfo[lan].maxBcst, lanSapInfo[lan].maxAge, 
			lanSapInfo[lan].gap);
#endif
	}

	return(0);
}

/******************************************************************/
STATIC int 
GetLanInfo(void)
{
	struct strioctl ioc;
	int	lan;

	lanInfoTable = (lanInfo_t*)NWALLOC(ipxConfiguredLans * sizeof(lanInfo_t));
	if (!lanInfoTable) {
		return(FAILURE);
	}

	ioc.ic_cmd = IPX_GET_LAN_INFO;
	ioc.ic_timout = 0;

	for(lan = 0; lan < ipxConfiguredLans; lan++) {
		lanInfoTable[lan].lan = lan;
		ioc.ic_len =  sizeof(lanInfo_t);
		ioc.ic_dp = (char *)&lanInfoTable[lan];

		if (ioctl(lipmxFd, I_STR, &ioc) == -1) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_IOCTL_INFO), titleStr);
			logperror("");
			NWFREE(lanInfoTable);
			lanInfoTable = NULL;
			return(-1);
		}
	}

	return(0);
}

/******************************************************************/
STATIC int
SetUpSharedMemory(void)
{
	const	char *configDir;
	key_t	shmkey;
	int		NWs;
	int		numservers, nhash, ssize, nsize, shmsize, nlan;
	int		i;
	int32	*ip;
	SAPL   *LanInfo;

    if( (configDir = NWCMGetConfigFilePath()) == NULL) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_BAD_CONFIG), titleStr);
		return(FAILURE);
	}

	if( (shmkey = ftok( configDir, SAP_PACKET_TYPE)) == -1) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_SYSCALL_FAIL), titleStr, "ftok");
		logperror("");
		return(FAILURE);
	}

	if( (NWs = NWCMGetParam( "sap_servers", NWCP_INTEGER, &numservers))
			!= SUCCESS) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "sap_servers");
		NWCMPerror(NWs, "");
		return(FAILURE);
	}

	/*
	**	Compute Table Sizes
	*/
	nlan = sizeof(SAPL) * ipxConfiguredLans;
	nhash = max( SetPrime( numservers/2), 31);
	nsize = nhash * sizeof(int32);
	ssize = numservers * sizeof(ServerEntry_t);

	/*
	**	Compute Share memory size, allow room for magic numbers
	*/
	shmsize = sizeof(SapShmHdr_t) + nlan + nsize + ssize + (4 * sizeof(int32));

	if( (shmid = shmget( shmkey, shmsize, IPC_CREAT | IPC_EXCL | 0644)) == -1) {
		if( errno != EEXIST) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_SYSCALL_FAIL), titleStr, "shmget");
			logperror("");
			return(FAILURE);
		}
		/*
		**	Couldn't get shared memory, try to get rid of existing seg
		**	and try again
		*/
		if( (shmid = shmget( shmkey, 0, 0)) == -1) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_SYSCALL_FAIL),
										titleStr, "shmget");
			logperror("");
			return(FAILURE);
		}		
		/*
		**	Remove shared memory segment
		*/
		if( shmctl( shmid, IPC_RMID, NULL) == -1) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_SYSCALL_FAIL), titleStr,
						"shmctl IPC_RMID");
			logperror("");
			return(FAILURE);
		}
		if( (shmid = shmget( shmkey, shmsize, IPC_CREAT | IPC_EXCL | 0644)) == -1){
			FPRINTF(SapLogFile, MsgGetStr(SAP_SYSCALL_FAIL),
										titleStr, "shmget");
			logperror("");
			return(FAILURE);
		}
	}
	if( (ShmBase = (SapShmHdr_t *)shmat( shmid, NULL, 0)) == (void *)-1) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_SYSCALL_FAIL),
										titleStr, "shmat");
		logperror("");
		return(FAILURE);
	}
	
#if SHMDEBUG || MEMORYDEBUG
	FPRINTF(SapLogFile, "%s: Memory ends at 0x%X\n",
		titleStr, sbrk(0));
	FPRINTF(SapLogFile, "%s: attached to shm key 0x%x, id %d @ 0x%X, size = 0x%X, end 0x%X\n",
		titleStr, shmkey, shmid, ShmBase, shmsize, (char *)ShmBase + shmsize);
	FPRINTF(SapLogFile,"%s: allocating structure for %d servers, size %d\n",
		titleStr, numservers, sizeof(ServerEntry_t));
#endif
	/*
	**	Initialize shared memory control structure
	**	Note: ServerPool lower index starts at 1
	*/
	memset( ShmBase, 0, sizeof(SapShmHdr_t));

	ShmBase->D.Lans = (int32)ipxConfiguredLans;
	ShmBase->LanInfo = sizeof(SapShmHdr_t) + sizeof(uint32);

	ShmBase->NameHashSize = nhash;
	ShmBase->NameHash = ShmBase->LanInfo + 
			(ShmBase->D.Lans * sizeof(SAPL)) + sizeof(uint32);

	ShmBase->D.ServerPoolIdx = 1;
	ShmBase->ServerPool = ShmBase->NameHash + ((nhash + 1) * sizeof(int32))
		- sizeof(ServerEntry_t);

	ShmBase->D.StartTime = time( NULL);
	ShmBase->D.ConfigServers = numservers;

	/*
	**	Initialize LanInfo magic number
	*/
	LanBase = (SAPL *)((char *)ShmBase + ShmBase->LanInfo);
	ip = (int32 *)LanBase;
	ip[-1] = 0x4C4C4C4C;			/* magic number LLLL */

	/*
	**	Initialize name magic number
	*/
	nHashBase = (int32 *)((char *)ShmBase + ShmBase->NameHash);
	nHashBase[-1] = 0x4E4E4E4E;	/* magic number NNNN */
		
	/*
	**	Initialize SrvBase hash magic number
	**	Note: ServerPool lower index starts at 1
	*/
	SrvBase = (ServerEntry_t *)((char *)ShmBase + ShmBase->ServerPool);
	ip = (int32 *)&SrvBase[1];
	ip[-1] = 0x53535353;			/* magic number SSSS */
	ip = (int32 *)((char *)ip + (numservers * sizeof(ServerEntry_t)));
	*ip = 0x53535353;			/* magic number SSSS */

#if SHMDEBUG || MEMORYDEBUG
	FPRINTF(SapLogFile, "%s: nHashBase 0x%X, SrvBase 0x%X\n",
		titleStr, nHashBase, SrvBase);
#endif
	/*
	**	Initialize LanInfo
	*/
	LanInfo = (SAPL *)((char *)ShmBase + ShmBase->LanInfo);
	for( i=0; i< (int)ShmBase->D.Lans; i++) {
		LanInfo->LanNumber = lanInfoTable[i].lan;
		LanInfo->Network = lanInfoTable[i].network;
		LanInfo->UpdateInterval = (lanSapInfo[i].maxBcst * sapTimerInterval);
		LanInfo->AgeFactor
			= lanInfoTable[i].ripSapInfo.sap.ageOutIntervals;
		LanInfo->PacketGap = lanInfoTable[i].ripSapInfo.sap.interPktGap;
		LanInfo->LineSpeed = lanInfoTable[i].ripSapInfo.lanSpeed;
		LanInfo->PacketSize = lanInfoTable[i].ripSapInfo.sap.maxPktSize;
		LanInfo->PacketsSent = 0;
		LanInfo->PacketsReceived = 0;
		LanInfo->BadPktsReceived = 0;
		LanInfo++;
	}

	/*
	**	Initialize server entry pool (index starts at 1)
	*/
	for( i = 1; i <= ShmBase->D.ConfigServers; i++) {
		(SrvBase + i)->NameLink = i+1;
		(SrvBase + i)->HopsToServer = SAP_DOWN; /* Don't advertise this */
		(SrvBase + i)->RevisionStamp = 0;		 /* Don't advertise this */
		(SrvBase + i)->ServerName[0] = '\0';
		(SrvBase + i)->ServerType = 0;
		(SrvBase + i)->SourceListLink = NULL;
	}
	(SrvBase + --i)->NameLink = 0;			/* Last Entry */

	/*
	**	Zero hash table
	*/
	for( i = 0; i < ShmBase->NameHashSize; i++) {
		*(nHashBase + i) = 0;
	}

	/*
	**	Set up my network address
	**	If no internal net, we must use lan1
	*/
	i = 0;
	if( IPXCMPNET( &i, &lanInfoTable[0].network)) {
		i = 1;
	}
	PPUTINT32( lanInfoTable[i].network, ShmBase->D.MyNetworkAddress);
	IPXCOPYNODE( lanInfoTable[i].nodeAddress, &ShmBase->D.MyNetworkAddress[4]);
	PPUTINT16( SAP_SAS, &ShmBase->D.MyNetworkAddress[10]);

	ShmBase->D.SapPid = SAPPid; /* Flag that Shared memory is initialized */

	return(SUCCESS);
}

/******************************************************************/
/*
**	Get RIP information about each of our configured lans
**	Save in LocalNet array
*/
STATIC int 
SetUpNetEntryList(void)
{
	uint32		 i;
	netInfo_t	*lePtr;
	uint32		 net;

	LocalNet = (netInfo_t **)NWALLOC(ipxConfiguredLans * sizeof(netInfo_t *));
	if( LocalNet == NULL) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_LOCALNET), titleStr);
		return(-1);
	}

	for (i=0; i<ipxConfiguredLans; i++)
		LocalNet[i] = NULL;

	for (i=0; i<ipxConfiguredLans; i++) {
		if ((lePtr=(netInfo_t *) NWALLOC(sizeof(netInfo_t))) == NULL) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_LIST), titleStr);
			return(-1);
		}


		net = GETINT32(lanInfoTable[i].network);
		if( net == 0) {
			memset( lePtr, 0, sizeof(netInfo_t));
		} else {
			if(GetNetEntry( net, lePtr) < 0) {
				return(-1);
			}
		}
		LocalNet[i] = lePtr;
	}

	return(0);
}
/******************************************************************/
STATIC int 
SetUpSapParameters(void)
{
	FILE *errorFile;
	int	sapRunPri;
	char trackOutValue[NWCM_MAX_STRING_SIZE];
	char path[PATH_MAX];
	char logOutValue[NWCM_MAX_STRING_SIZE];
	int	 NWs;
	struct stat sbuf;

	if( (NWs = NWCMGetParam( "sap_track_file", NWCP_STRING, trackOutValue)) 
			!= SUCCESS) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "sap_track_file");
		NWCMPerror(NWs, "");
		return(FAILURE);
	}

	if( strchr( trackOutValue, '/') == NULL) {
		if( (NWs = NWCMGetParam( "log_directory", NWCP_STRING, path))
				!= SUCCESS) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "log_directory");
			NWCMPerror(NWs, "");
			return(FAILURE);
		}
		strcat(path,"/");
		strcat(path, trackOutValue);
	} else {
		strcpy(path, trackOutValue);
	}

	if ((SapTrackFile = fopen(path,"w+")) == NULL) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_OPEN_FAIL), titleStr, path);
		logperror("");
		return(FAILURE);
	}
	setvbuf( SapTrackFile, NULL, _IOLBF, 0);

	if( (NWs = NWCMGetParam( "sap_priority", NWCP_INTEGER, &sapRunPri))
			!= SUCCESS) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "sap_priority");
		NWCMPerror(NWs, "");
		return(FAILURE);
	}

	if( (NWs = NWCMGetParam( "sap_fast_init", NWCP_BOOLEAN, &SapFastInit))
			!= SUCCESS) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_FAST_INIT), titleStr);
		SapFastInit = TRUE;
	}

	if (sapRunPri > SAPS_MIN_PRIORITY)	{
		FPRINTF(SapLogFile, MsgGetStr(SAP_PRIOR_ADJUST), titleStr,
			sapRunPri, SAPS_MIN_PRIORITY, SAPS_MIN_PRIORITY);
		sapRunPri = SAPS_MIN_PRIORITY;
	} else {
		if (sapRunPri < SAPS_MAX_PRIORITY) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_PRIOR_VALUE), titleStr,
				sapRunPri, SAPS_MAX_PRIORITY, SAPS_MAX_PRIORITY);
			sapRunPri = SAPS_MAX_PRIORITY;
		}
	}
		
	errno = 0;
	if (nice( sapRunPri - SAPS_DEFAULT_NICE_VALUE ) == -1) {
		/*
		 *	-1 is a valid return value from nice(), if errno is zero.
		 */
		if( errno != 0) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_NICE_FAIL), titleStr,
				sapRunPri - SAPS_DEFAULT_NICE_VALUE);
			logperror("");
			return(FAILURE);
		}
	}

	if( (NWs = NWCMGetParam( "sap_max_messages",
				NWCP_INTEGER, &max_messages)) != SUCCESS) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "sap_max_messages");
		NWCMPerror(NWs, "");
		return(FAILURE);
	}

	if( (NWs = NWCMGetParam( "sap_log_file", NWCP_STRING, logOutValue))
			!= SUCCESS) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "sap_log_file");
		NWCMPerror(NWs, "");
		return(FAILURE);
	}

	if( strchr( logOutValue, '/') == NULL) {
		if( (NWs = NWCMGetParam( "log_directory", NWCP_STRING, path))
				!= SUCCESS) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "log_directory");
			NWCMPerror(NWs, "");
			return(FAILURE);
		}
		strcat(path,"/");
		strcat(path, logOutValue);
	} else {
		strcpy(path, logOutValue);
	}
	strcpy( SapLogPath, path);

	if( max_messages > 0) {
		if( stat(SapLogPath, &sbuf) == 0) {
			strcat(path, ".old");
			(void) rename(SapLogPath, path);
		}
		if ((errorFile = fopen(SapLogPath,"w+")) == NULL) {
			FPRINTF(stderr, MsgGetStr(SAP_OPEN_FAIL), titleStr, SapLogPath);
			logperror("");
			return(FAILURE);
		}
		setvbuf( errorFile, NULL, _IOLBF, 0);
		SapLog = 1;
	} else {
		unlink( SapLogPath);
		errorFile = NULL;
		SapLog = 0;
	}
	fclose(SapLogFile);
	SapLogFile = errorFile;

	FPRINTF( SapLogFile, MsgGetStr(SAP_START), titleStr);
	return( SUCCESS);
}

/******************************************************************/
STATIC void
SendDumpSapInfoPacket(void)
{
	int i;

	sendBufPtr->ipxHdr.chksum = (uint16)IPX_CHKSUM;
	PPUTINT16(IPX_HDR_SIZE + 2 * sizeof(uint16), &sendBufPtr->ipxHdr.len);
	sendBufPtr->ipxHdr.pt = SAP_PACKET_TYPE;
	sendBufPtr->ipxHdr.tc = 0;

/*
 * There are at least two nets 0 and 1, or ipx wouldn't have
 *	come up.  lan 1 is the connected lan
 */
	IPXCOPYNODE(ALLHOSTS, sendBufPtr->ipxHdr.dest.node);
	PPUTINT16(SAP_SAS, sendBufPtr->ipxHdr.dest.sock);

	sendBufPtr->Operation = GETINT16(SAP_GSQ);
	sendBufPtr->ServerTable[0].TargetType = GETINT16(ALL_SERVER_TYPE);

	/*
	**	Start with one, because we don't need to send request
	**	on the internal net
	*/
	for ( i=1; i<ipxConfiguredLans; i++) {
		PPUTINT32(lanInfoTable[i].network, sendBufPtr->ipxHdr.dest.net);
		if( GETINT32(lanInfoTable[i].network) ) {
			WriteSapPacket(sendBufPtr);
			ShmBase->D.GSQSent++;
		}
	}
	return;
}

/********************************************************************/

/*ARGSUSED*/
STATIC void
DumpTables(int sig)
{
#define SERVER_ARRAY_SIZE 200
	ServerEntry_t *Server;
	int i;
	uint8 serverName[60];
	uint16 typeCount[SERVER_ARRAY_SIZE];
	uint16 serverTypes[SERVER_ARRAY_SIZE];
	int serverTypesCounted =0;
	InfoSourceEntry_t *info;
	FILE *fp;
	ipxAddr_t *ipxAddr;
	uint32 numServers = 0;
	char path[PATH_MAX];
	int	 NWs;
	char dumpOutValue[NWCM_MAX_STRING_SIZE];
	int	svrIndex;

	validTime = 0;

	if( (NWs = NWCMGetParam( "sap_dump_file", NWCP_STRING, dumpOutValue)) 
			!= SUCCESS) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "sap_dump_file");
		NWCMPerror(NWs, "");
		return;
	}

	if( strchr( dumpOutValue, '/') == NULL) {
		if( (NWs = NWCMGetParam( "log_directory", NWCP_STRING, path))
				!= SUCCESS) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_MAP), titleStr, "log_directory");
			NWCMPerror(NWs, "");
			return;
		}
		strcat(path,"/");
		strcat(path, dumpOutValue);
	} else {
		strcpy(path, dumpOutValue);
	}

	if ((fp = fopen(path,"w+t")) == NULL) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_OPEN_FAIL), titleStr, path);
		logperror("");
		return;
	}

	/*
	 *	Dump tables is not internationalized
	 */
	svrIndex = 1;
	for (Server = SrvBase + svrIndex; svrIndex <= ShmBase->D.ConfigServers;
			Server = SrvBase + svrIndex) {
		svrIndex++;
		numServers++;

		for (i=0; i<serverTypesCounted; i++)
			if (serverTypes[i] == Server->ServerType)
				break;

		if (Server->ServerType == serverTypes[i])
			typeCount[i]++;
		else {
			if( serverTypesCounted <  (SERVER_ARRAY_SIZE - 2)) {
				serverTypes[serverTypesCounted] = Server->ServerType;
				typeCount[serverTypesCounted] = 1;
				serverTypesCounted++;
			}
		}
		
		strncpy((int8 *)serverName, (int8 *)&Server->ServerName[1],
			Server->ServerName[0]);
		if (Server->ServerName[0] >10)
			serverName[10] = '\0';
		else
			serverName[Server->ServerName[0]] = '\0';
		FPRINTF(fp," %+11s ",serverName);
		FPRINTF(fp," Type: %04X Hops: %04X Address: ", 
			(uint16)GETINT16(Server->ServerType),
			(uint16)(Server->HopsToServer));
			ipxAddr = (ipxAddr_t *)Server->ServerAddress;

		FPRINTF(fp," ");
		for (i=0; i<4; i++)
			FPRINTF(fp,"%02X",ipxAddr->net[i]);
		FPRINTF(fp," ");
		for (i=0; i<6; i++)
			FPRINTF(fp,"%02X",ipxAddr->node[i]);
		FPRINTF(fp," ");
		for (i=0; i<2; i++)
			FPRINTF(fp,"%02X",ipxAddr->sock[i]);
		FPRINTF(fp,"\n");

		for(info = Server->SourceListLink; info != NULL;
			info = info->NextSource) {
			FPRINTF(fp," %+11s ",serverName);
			FPRINTF(fp," Hops: %04d Timer: %04d connectedLan: %04d ",
				(uint16)(info->HopsToSource), 
				(uint8)(info->ServTimer), 
				(uint8)(info->ServConnectedLAN));
			for(i=0; i<IPX_NODE_SIZE; i++)
				FPRINTF(fp,"%02X",info->SourceAddress[i]);
			FPRINTF(fp,"\n");
		}
		FPRINTF(fp," \n");
	}

	for (i=0; i<serverTypesCounted; i++)
		FPRINTF(fp," Type 0x%04X Count %d \n",
			GETINT16(serverTypes[i]), typeCount[i]);

	FPRINTF(fp," total number of servers %d, server types %d\n",
		numServers, serverTypesCounted);
	fclose(fp);

	return;
}

/******************************************************************/
#if MEMORYDEBUG
/*ARGSUSED*/
STATIC void
CheckMemory(int sig)
{
	int i;
	validTime = 0;
	FPRINTF(SapLogFile, "%s: Memory Ends at 0x%X\n", titleStr, sbrk(0));
#ifdef DEBUG
	if( LocalMemoryCheck() == NULL) {
		FPRINTF(SapLogFile, "%s: Memory checks OK\n", titleStr);
		i = GetAllocRunningTotal();
		FPRINTF(SapLogFile, "Memory usage is %d bytes\n", i);
		if( i > 0) {
			ShowOutstandingBlocks( SapLogFile);
		}
	} else {
		FPRINTF(SapLogFile, "%s: Memory check failed\n", titleStr);
	}
#endif
	return;
}
#endif

/******************************************************************/
/*ARGSUSED*/
STATIC void
TrackFlagOn(int sig)
{
	TrackFlag = TRUE;
	validTime = 0;
	return;
}

/******************************************************************/
/*ARGSUSED*/
STATIC void
TrackFlagOff(int sig)
{
	TrackFlag = FALSE;
	validTime = 0;
	return;
}

/******************************************************************/
/*
**	Remove local process from NotifyList and release buffer
**
**	Returns address of previons structure if removed
**			&NotifyList if it was first in the list
**			-1	Not removed, cannot be found
*/
STATIC NotifyStruct_t *
RemoveFromNotifyList( pid_t Pid)
{
	NotifyStruct_t *np, *lnp;

	if( NotifyList == NULL)
		return(0);
	/*
	**	See if first entry
	*/
	if( Pid == NotifyList->Pid) {
		lnp = NotifyList;
		NotifyList = NotifyList->Next;
		NWFREE( lnp);
		ShmBase->D.ProcessesToNotify--;
		return( (NotifyStruct_t *)NotifyList);	/* Assues Next is first item */
	}

	/*
	**	Not first entry, look thru list, find entry and remove it
	*/
	for( lnp = NotifyList; lnp->Next != NULL; lnp = lnp->Next) {
		if( lnp->Next->Pid == Pid) {
			np = lnp->Next;
			lnp->Next = lnp->Next->Next;
			NWFREE( np);
			ShmBase->D.ProcessesToNotify--;
			return(lnp);
		}
	}
	return((NotifyStruct_t *)-1);
}

/******************************************************************/
/*
**	When server status has changed, we may need to notify local
**	process of that fact.  We look through the change list
**	and signal any that need to know
*/
STATIC void
NotifyProcessOfChanges( void)
{
	NotifyStruct_t *np;
	ServerEntry_t *Server;

	if( (NotifyList == NULL) || (ChangedLink == 0)) {
		return;
	}

	for( np = NotifyList; np != NULL; np = np->Next) {
		/*
		**	If this process wants to know about all servers, just send
		**	notification
		*/
		if( np->ServerType == ALL_SERVER_TYPE) {
			if( kill( np->Pid, np->Signal) == -1) {
				np = RemoveFromNotifyList( np->Pid);
				if( np == (NotifyStruct_t *)-1) {
					abort();
				}
			}
			ShmBase->D.NotificationsSent++;
			if(np==NULL)
				return;
			continue;
		}
		/*
		**	If this process want to know about one server type, 
		**	send notification if we find one server of that type changed
		*/
		for( Server = SrvBase + ChangedLink; Server != SrvBase;
				Server = SrvBase + Server->ChangedLink) {
			if( np->ServerType == Server->ServerType) {
				if( kill( np->Pid, np->Signal) == -1) {
					np = RemoveFromNotifyList( np->Pid);
					if( np == (NotifyStruct_t *)-1) {
						abort();
					}
				}
				ShmBase->D.NotificationsSent++;
				if(np==NULL)
					return;
				break;	/* We need find only one */
			}
		}
		continue;
	}
}

/******************************************************************/
STATIC void
RegisterProcessForChanges( SapNotify_t *packet)
{
	NotifyStruct_t *np;
	uint16	Operation;
	pid_t	returnStatus;

	/*
	**	This packet must come from our internal network, otherwise
	**	throw it away
	*/
	if( ! IPXCMPNET(ShmBase->D.MyNetworkAddress, packet->IpxHeader.src.net) ||
			!IPXCMPNODE( &ShmBase->D.MyNetworkAddress[IPX_NET_SIZE],
				packet->IpxHeader.src.node)) {
		ShmBase->D.BadSapSource++;
		return;
	}

	/*
	**	if can't malloc, let process time out and try again
	*/
	np = NWALLOC( sizeof(NotifyStruct_t));
	if( np == NULL) {
		ShmBase->D.MallocFailed++;
		returnStatus = SAPD_ENOMEM;
		goto SAPEndNotify;
	}

	/*
	**	If pid is invalid, nak packet
	*/
	if( kill( packet->Pid, 0) == -1) {
		Operation = SAP_NACK;
		NWFREE( np);
		ShmBase->D.SNCNackSent++;
		returnStatus = SAPD_PID_INVAL;
	} else {
	
		/*
		**	Fill in notify structure and add to list
		*/
		np->Pid = packet->Pid;
		np->ServerType = packet->ServerType;
		np->Signal = packet->Signal;
		np->Next = NotifyList;
		NotifyList = np;
		Operation = SAP_ACK;
		ShmBase->D.SNCAckSent++;
		ShmBase->D.ProcessesToNotify++;
		returnStatus = 0;
	}

SAPEndNotify:
	/*
	**	Return an Ack, just use input pkt, put source in destination
	**	and change operation to ACK or NACK
	*/
	IPXCOPYADDR( &packet->IpxHeader.src, &packet->IpxHeader.dest);
	packet->SapOperation = GETINT16(Operation);
	packet->Pid = returnStatus;
	WriteSapPacket( (ServerPacketStruct_t *)packet);
	return;
}
	
/******************************************************************/
STATIC void
UnregisterProcessForChanges( SapNotify_t *packet)
{
	uint16	Operation;
	pid_t	returnStatus;

	/*
	**	This packet must come from our internal network, otherwise
	**	throw it away
	*/
	if( ! IPXCMPNET(ShmBase->D.MyNetworkAddress, packet->IpxHeader.src.net) ||
			!IPXCMPNODE( &ShmBase->D.MyNetworkAddress[IPX_NET_SIZE],
				packet->IpxHeader.src.node)) {
		ShmBase->D.BadSapSource++;
		return;
	}

	/*
	**	Remove the register entry, following which we are done.
	*/
	/*
	**	Send a NACK if we don't find the pid
	*/
	if( RemoveFromNotifyList( packet->Pid) == (NotifyStruct_t *)-1) {
		Operation = SAP_NACK;
		returnStatus = SAPD_PID_INVAL;
	} else {
		Operation = SAP_ACK;
		returnStatus = 0;
	}

	/*
	**	Return an Ack, just use input pkt, put source in destination
	**	and change operation to ACK or NACK
	*/
	IPXCOPYADDR( &packet->IpxHeader.src, &packet->IpxHeader.dest);
	packet->SapOperation = GETINT16(Operation);
	packet->Pid = returnStatus;
	WriteSapPacket( (ServerPacketStruct_t *)packet);
	return;
}

/******************************************************************/
STATIC void
AdvertiseLocalServer( SapAdvertise_t *packet, pid_t localpid)
{
	uint16				 Operation;
	ServerEntry_t		*Server;
	InfoSourceEntry_t   *Info, *NextInfo;
	netInfo_t           *Net;
	int					 count;
	pid_t				 returnStatus;
	uint8				 name[NWMAX_SERVER_NAME_LENGTH];
	int32		   	    *HashEnt;
	int					 hash;

	/*
	**	This packet must come from our internal network, otherwise
	**	throw it away
	*/
	if( ! IPXCMPNET(ShmBase->D.MyNetworkAddress, packet->IpxHeader.src.net) ||
			!IPXCMPNODE( &ShmBase->D.MyNetworkAddress[IPX_NET_SIZE],
				packet->IpxHeader.src.node)) {
		ShmBase->D.BadSapSource++;
		return;
	}

	/*
	**	convert server name to cononical form where name[0] is length of name
	*/
	packet->ServerName[NWMAX_SERVER_NAME_LENGTH - 1] = '\0';
	strcpy((char *)&name[1], (char *)packet->ServerName);
	*name = (uint8)strlen((char *)packet->ServerName);
	if( *name == 0) {
		Operation = SAP_NACK;
		returnStatus = SAPD_NAME_ZERO;
		goto SAPAdvRet;
	}
	if( *name < NAME_HASH_LENGTH) {
		memset( &name[*name +1], '\0', NAME_HASH_LENGTH - *name);
	} else {
		name[*name + 1] = '\0';
	}

	/*
	**	If a forever packet, use pid passed to function
	*/
	if( localpid != 0) {
		packet->Pid = localpid;
	}

	Server = FindServer( name, packet->ServerType);
	if( Server != NULL) {
		/*
		**	Server already exists
		*/
		if( Server->HopsToServer < sapMaxHops) {
			/*
			**	Already advertised, if pid is the same, just ack request
			*/
			if( packet->Pid == Server->LocalPid) {
				Operation = SAP_ACK;
				returnStatus = 0;
				goto SAPAdvRet;
			}

			if( Server->LocalPid != 0) {
				/*
				**	Pid is different
				**	if first process is local and is still up, first is best,
				**	If existing server not local, new local is best
				*/
				if( kill( Server->LocalPid, 0) != -1) {
					Operation = SAP_NACK;
					returnStatus = SAPD_BUSY;
					goto SAPAdvRet;
				} 
			}
		}
		/*
		**	First request no longer up or is not local
		**	change to new request, release source
		*/
		Info = Server->SourceListLink;
		Server->SourceListLink = 0;
		while( Info != NULL) {
			NextInfo = Info->NextSource;
			NWFREE((char *)Info);
			Info = NextInfo;
		}
	} else {
		/*
		**	This is a new entry, we need to allocate it
		*/
		if( (Server = SRVALLOC( sizeof(ServerEntry_t))) == NULL) {
			ShmBase->D.SrvAllocFailed++;
			/*
			**	If no server entries left in memory, don't add it
			*/
			FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_SRVENT), titleStr);
			/* No space */
#if SERVICEDEBUG
			FPRINTF(SapLogFile,"%s: %sDROP(no_srv_entries)  ", titleStr, INPKT);
			PrintSrvPacket( SapLogFile, packet->ServerType, &name[1], 
				(ipxAddr_t *)Server->ServerAddress, Server->HopsToServer);
#endif
			Operation = SAP_NACK;
			returnStatus = SAPD_ENOMEM;
			goto SAPAdvRet;
		}
		/*
		**	Fill in enough of packet so we can't get confused
		*/
		Server->SourceListLink = 0;
		Server->HopsToServer = SAP_DOWN;
		Server->LocalPid = 0;
		Server->RevisionStamp = 0;
		Server->ServerType = packet->ServerType;
		memcpy(Server->ServerName, name, sizeof( name));

		/*
		**	Add to hash table
		*/
		hash = GetNameHash(name);
		HashEnt = nHashBase + hash;
#ifdef SAPDEBUG
		{
			/*
			**	DEBUG check, make sure we don't link to ourselves
			*/
			ServerEntry_t *tsrv;
			for(tsrv = SrvBase + *HashEnt; tsrv != SrvBase; tsrv = SrvBase + tsrv->NameLink) {
				if( (tsrv - SrvBase) == (Server - SrvBase) ) {
					FPRINTF(SapLogFile,"%s: Abort AddServer: Hash 0x%X, Server 0x%X, Entry 0x%X, Link 0x%X, *Hashent, name %s\n",
						titleStr, hash, Server, Server - SrvBase, Server->NameLink, *HashEnt, &name[1]);
					abort();
				}
			}
		}
#endif
		Server->NameLink = *HashEnt;
		*HashEnt = Server - SrvBase;
	}
	
	Info = (InfoSourceEntry_t *)NWALLOC(sizeof(InfoSourceEntry_t));
	if( Info == NULL) {
		ShmBase->D.MallocFailed++;
		/*
		**  If no malloc space left, don't add it
		*/
		FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_INFO), titleStr);
#if SERVICEDEBUG
		FPRINTF(SapLogFile,"%s: %sDROP(no_info_malloc)  ", titleStr, INPKT);
		PrintSrvPacket( SapLogFile, Server->ServerType, &Server->ServerName[1],
			(ipxAddr_t *)Server->ServerAddress, GETINT16(Server->HopsToServer));
#endif
		Operation = SAP_NACK;
		returnStatus = SAPD_ENOMEM;
		goto SAPAdvRet;
	}

	/*
	**	If pid is invalid, nak packet
	**	Fill in just enough of Server so we don't get confused
	*/
	if( kill( packet->Pid, 0) == -1) {
		Operation = SAP_NACK;
		NWFREE( Info);
		ShmBase->D.SNCNackSent++;
		returnStatus = SAPD_PID_INVAL;
		goto SAPAdvRet;
	} 

	/*
	**	Set up info structure, we don't need to ask, we know info
	*/
	Info->NextSource = 0;
	IPXCOPYNODE( packet->IpxHeader.src.node, Info->SourceAddress);
	Info->HopsToSource = 0;
	Info->ServTimer = 0;
	Info->ServConnectedLAN = 0;
	Info->LocalPid = packet->Pid;
	IPXCOPYNET( packet->IpxHeader.src.net, &Info->N.netIDNumber);
	Info->N.timeToNet = 0;
	Info->N.hopsToNet = 0;
	Info->N.netStatus = 0;
	Info->N.lanIndex = 0;
	
	/*
	**	Set up server entry
	*/
	Server->SourceListLink = Info;
	Server->N = Info->N;
	IPXCOPYADDR( packet->IpxHeader.src.net, Server->ServerAddress);
	IPXCOPYSOCK( packet->ServerSocket, 
		&Server->ServerAddress[IPX_NET_SIZE + IPX_NODE_SIZE]);
	Server->LocalPid = packet->Pid;
	Server->RevisionStamp = newStamp;
	ShmBase->D.RevisionStamp = newStamp;
	Server->HopsToServer = 0;

	AddToChangedList( Server, "AdvertiseLocalServer");
	FPRINTF(SapLogFile, MsgGetStr(SAP_ADD_LOCAL), titleStr);
	FPRINTF(SapLogFile, MsgGetStr(SAP_SERVER_INFO), 
		GETINT16(Server->ServerType), Server->LocalPid, &Server->ServerName[1]);

#if SERVICEDEBUG
	FPRINTF(SapLogFile,"%s: %sCHG(local_server_add):", titleStr, INPKT);
	PrintSrvPacket( SapLogFile, Server->ServerType, &Server->ServerName[1],
		(ipxAddr_t *)Server->ServerAddress, GETINT16(Server->HopsToServer));
#endif
	/*
	**	Advertise that this server is up
	*/
	NotifyProcessOfChanges();
	/*
	**	Don't send info on internal lan
	*/
	for ( count = 1; count < ipxConfiguredLans; count++) {
		Net = LocalNet[count];
		SendChangedServerInformation( count,
			 Net->netIDNumber, ALLHOSTS, GETINT16(SAP_SAS),
			 NO_PACKET_DELAY, NO_SPLIT_HORIZ);
	}
	PruneSrvSource();
	ChangedLink = 0;	/* Done processing changes */
	Operation = SAP_ACK;
	returnStatus = 0;

SAPAdvRet:
	if( returnStatus == 0)
		ShmBase->D.SASAckSent++;
	else
		ShmBase->D.SASNackSent++;
	/*
	**	Return an Ack, just use input pkt, put source in destination
	**	and change operation to ACK or NACK
	*/
	IPXCOPYADDR( &packet->IpxHeader.src, &packet->IpxHeader.dest);
	packet->SapOperation = GETINT16(Operation);
	packet->Pid = returnStatus;
	returnStatus = 0;
	WriteSapPacket( (ServerPacketStruct_t *)packet);
	return;
}

/******************************************************************/
STATIC void
UnAdvertiseLocalServer( SapAdvertise_t *packet)
{
	uint16				 Operation;
	ServerEntry_t		*Server;
	InfoSourceEntry_t   *Info, *NextInfo;
	netInfo_t           *Net;
	int					 count;
	pid_t				 returnStatus;
	uint8				 name[NWMAX_SERVER_NAME_LENGTH];

	/*
	**	This packet must come from our internal network, otherwise
	**	throw it away
	*/
	if( ! IPXCMPNET(ShmBase->D.MyNetworkAddress, packet->IpxHeader.src.net) ||
			!IPXCMPNODE( &ShmBase->D.MyNetworkAddress[IPX_NET_SIZE],
				packet->IpxHeader.src.node)) {
		ShmBase->D.BadSapSource++;
		return;
	}

	/*
	**	Make sure ServerName is NULL terminated and convert to cononical form
	**	where name[0] is length of name
	*/
	packet->ServerName[NWMAX_SERVER_NAME_LENGTH - 2] = '\0';
	strcpy((char *)&name[1], (char *)packet->ServerName);
	*name = (uint8)strlen((char *)packet->ServerName);
	if( *name == 0) {
		Operation = SAP_NACK;
		returnStatus = SAPD_NAME_ZERO;
		goto SAPUnadvRet;
	}
	if( *name < NAME_HASH_LENGTH) {
		memset( &name[*name +1], '\0', NAME_HASH_LENGTH - *name);
	} else {
		name[*name + 1] = '\0';
	}

	Server = FindServer( name, packet->ServerType);
	/*
	**	Send a NACK if we don't find the server
	*/
	if( Server == NULL) {
		Operation = SAP_NACK;
		returnStatus = SAPD_NOFIND;
	} else {
		if( Server->HopsToServer == SAP_DOWN) {
			Operation = SAP_NACK;
			returnStatus = SAPD_NOFIND;
		} else {

			/*
			**	Process must own this service or we ignore request
			**	If SAPD owns the service, it is a PERMANENT advertisement.
			**	In this case, a root user may unadvertise it, otherwise fail
			*/
			if(Server->LocalPid == SAPPid)
				if(packet->Uid == 0)
					packet->Pid = SAPPid;

			if( packet->Pid != Server->LocalPid )
			{
				Operation = SAP_NACK;
				returnStatus = SAPD_NOPERM;
			} else {
				/*
				**	Release all source entries and down server
				*/
				Info = Server->SourceListLink;
				Server->SourceListLink = 0;
				while( Info != NULL) {
					NextInfo = Info->NextSource;
					NWFREE((char *)Info);
					Info = NextInfo;
				}
				FPRINTF(SapLogFile, MsgGetStr(SAP_DELETLE_LOCAL), titleStr);
				FPRINTF(SapLogFile, MsgGetStr(SAP_SERVER_INFO),
					GETINT16(Server->ServerType), 
					Server->LocalPid, &Server->ServerName[1]);
				Server->HopsToServer = SAP_DOWN;
				Server->RevisionStamp = newStamp;
				Server->LocalPid = 0;
				ShmBase->D.RevisionStamp = newStamp;
				AddToChangedList( Server, "AdvertiseLocalServer");

	#if SERVICEDEBUG
				FPRINTF(SapLogFile,"%s: %sCHG(local_srvr_down): ", titleStr, INPKT);
				PrintSrvPacket( SapLogFile, Server->ServerType, &Server->ServerName[1],
					(ipxAddr_t *)Server->ServerAddress,
					GETINT16(Server->HopsToServer));
	#endif
				/*
				**	Advertise that this server is down
				*/
				NotifyProcessOfChanges();
				/*
				**	Don't send info on internal lan
				*/
				for ( count = 1; count < ipxConfiguredLans; count++) {
					Net = LocalNet[count];
					SendChangedServerInformation( count,
						 Net->netIDNumber, ALLHOSTS, GETINT16(SAP_SAS),
						 NO_PACKET_DELAY, NO_SPLIT_HORIZ);
				}
				PruneSrvSource();
				ChangedLink = 0;	/* Done processing changes */
				Operation = SAP_ACK;
				returnStatus = 0;
			}
		}
	}

SAPUnadvRet:
	/*
	**	Return an Ack or Nack, just use input pkt, put source in destination
	**	and change operation to ACK or NACK
	*/
	IPXCOPYADDR( &packet->IpxHeader.src, &packet->IpxHeader.dest);
	packet->SapOperation = GETINT16(Operation);
	packet->Pid = returnStatus;
	WriteSapPacket( (ServerPacketStruct_t *)packet);
	return;
}

/******************************************************************/
STATIC void 
ConsumeAdvertisingPacket(
	ServerPacketStruct_t *packet,
	uint8 *PacketSource)
{
#if NSQDEBUG
	char  data[80];
#endif
	int32 length;
	uint16 operation;
	int ConnectedLAN;

	length = PGETINT16(&(packet->ipxHdr.len));
	length -= IPX_HDR_SIZE + sizeof(packet->Operation);

	/*
	**	Make sure we have an operation field
	*/
	if( length <= 0) {
		ShmBase->D.BadSizeInSaps++;
		FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr, length, "???");
		return;
	}
	operation = PGETINT16(&(packet->Operation));

	/*
	**	We only listen to our direct neighbor, i.e.
	**	the packet must come from an entity on our own lan
	*/
	if( (ConnectedLAN = NetToMyLan( PGETINT32( packet->ipxHdr.src.net))) < 0) {
		switch( operation) {
		case SAP_GSR:
			FPRINTF(SapLogFile, MsgGetStr(SAP_IN_GSR), titleStr,
				FormatIpxAddress( (ipxAddr_t *)PacketSource));
			break;
		case SAP_GSQ:
			FPRINTF(SapLogFile, MsgGetStr(SAP_IN_GSQ), titleStr,
				FormatIpxAddress( (ipxAddr_t *)PacketSource));
			break;
		case SAP_NSQ:
			FPRINTF(SapLogFile, MsgGetStr(SAP_IN_NSQ), titleStr,
				FormatIpxAddress( (ipxAddr_t *)PacketSource));
			break;
		default:
			FPRINTF(SapLogFile, MsgGetStr(SAP_IN_TYPE), titleStr,
				operation, FormatIpxAddress( (ipxAddr_t *)PacketSource));
			break;
		}
		ShmBase->D.NotNeighbor++;
		return;
	}

	/*
	**	If this is an echo of a packet we sent, just drop it
	*/
	if( IPXCMPNODE( PacketSource, lanInfoTable[ConnectedLAN].nodeAddress)
			&& (PGETINT16(packet->ipxHdr.src.sock) == SAP_SAS)) {
		ShmBase->D.EchoMyOutput++;
#if IN_PACKETDEBUG
		FPRINTF(SapLogFile,"%s: Packet sent by me and echoed back, drop\n",
			titleStr);
#endif
		return;
	}

	TrackServerPacket(ConnectedLAN, packet, TRUE);
	/*
	**	Process the packet
	**
	** The following operations are recognized:
	**	SAP_GSQ  = Send caller all server information
	**	SAP_GSR  = Here is some server information
	**	SAP_NSQ  = Give me the address of the closest server
	**	SAP_NWU_SOCK = Private function - register NWU socket (obselete)
	**	SAP_ADVERTISE_SERVICE - Request to Advertise a Service
	**	SAP_ADVERTISE_PERMANENT - Request to Advertise a Service forever
	**	SAP_UNADVERTISE_SERVICE - Request to Stop Advertising a Service
	**	SAP_NOTIFY_ME - Request for notification of changes
	**	SAP_UNNOTIFY_ME - Request to stop notification of changes
	*/
	switch( operation) {
	case SAP_GSR:
		if( length % sizeof(ServPacketInfo_t)) {
			ShmBase->D.BadSizeInSaps++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr,
								length, "SAP_GSR");
			break;
		}
		ShmBase->D.GSRReceived++;
		AcceptServerInformation(ConnectedLAN, packet, PacketSource);
		break;
	case SAP_GSQ:
		if( (length != 2) && (length != 0)) {
			ShmBase->D.BadSizeInSaps++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr,
								length, "SAP_GSQ");
			break;
		}
		ShmBase->D.GSQReceived++;
		GiveServerInformation(ConnectedLAN, packet);
		break;
	case SAP_NSQ:
		if( length != 2) {
			ShmBase->D.BadSizeInSaps++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr,
								length, "SAP_NSR");
			break;
		}
#if NSQDEBUG
		sprintf(data, "%s: %s", titleStr, INPKT);
		PrintIpxPacket(SapLogFile, packet, data);
#endif
		ShmBase->D.NSQReceived++;
		if (lanInfoTable[ConnectedLAN].ripSapInfo.sap.actions
				& SAP_REPLY_GNS)
			GetNearestServer(ConnectedLAN, packet);
		break;
	case SAP_NSR:
		ShmBase->D.BadSizeInSaps++;
		FPRINTF(SapLogFile, MsgGetStr(SAP_UNXP_NSR), titleStr,
			FormatIpxAddress( (ipxAddr_t *)PacketSource));
		break;
	case SAP_ADVERTISE_SERVICE:
		if( length != (sizeof(SapAdvertise_t) - IPX_HDR_SIZE - sizeof(uint16))){
			ShmBase->D.BadSizeInSaps++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr,
				length, "SAP_ADVERTISE_SERVICE");
			break;
		}
		ShmBase->D.SASReceived++;
		AdvertiseLocalServer( (SapAdvertise_t *)packet, 0);
		break;
	case SAP_ADVERTISE_PERMANENT:
		if( length != (sizeof(SapAdvertise_t) - IPX_HDR_SIZE - sizeof(uint16))){
			ShmBase->D.BadSizeInSaps++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr,
				length, "SAP_ADVERTISE_SERVICE");
			break;
		}
		ShmBase->D.SASReceived++;
		AdvertiseLocalServer( (SapAdvertise_t *)packet, SAPPid);
		break;
	case SAP_UNADVERTISE_SERVICE:
		if( length != (sizeof(SapAdvertise_t) - IPX_HDR_SIZE - sizeof(uint16))){
			ShmBase->D.BadSizeInSaps++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr,
				length, "SAP_UNADVERTISE_SERVICE");
			break;
		}
		ShmBase->D.SASReceived++;
		UnAdvertiseLocalServer( (SapAdvertise_t *)packet);
		break;
	case SAP_NOTIFY_ME:
		if( length != (sizeof(SapNotify_t) - IPX_HDR_SIZE - sizeof(uint16))) {
			ShmBase->D.BadSizeInSaps++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr,
				length, "SAP_NOTIFY_ME");
			break;
		}
		ShmBase->D.SNCReceived++;
		RegisterProcessForChanges( (SapNotify_t *)packet);
		break;
	case SAP_UNNOTIFY_ME:
		if( length != (sizeof(SapNotify_t) - IPX_HDR_SIZE - sizeof(uint16))) {
			ShmBase->D.BadSizeInSaps++;
			FPRINTF(SapLogFile, MsgGetStr(SAP_PACKET_LEN), titleStr,
				length, "SAP_UNNOTIFY_ME");
			break;
		}
		ShmBase->D.SNCReceived++;
		UnregisterProcessForChanges( (SapNotify_t *)packet);
		break;
	default:
		ShmBase->D.BadSizeInSaps++;
		FPRINTF(SapLogFile, MsgGetStr(SAP_UNK), titleStr,
			operation, FormatIpxAddress( (ipxAddr_t *)PacketSource));
		break;
	}
	return;
}

/******************************************************************/
/*ARGSUSED*/
main(int argc, char *argv[], char *envp[])
{	char *ipxDevice="/dev/ipx";
	char *lipmxDevice="/dev/ipx";
	char *ripxDevice="/dev/ripx";
	char car;
	int i, pid;
	unsigned char localServerName[NWCM_MAX_STRING_SIZE];
	IpxConfiguredLans_t	configLan;	
	struct strioctl ioc;
	int		maxSize;
	struct pollfd	fds[2];
	char *PidLogDir;
	uint16	sapSocket = SAP_SAS;
	int ccode;

	/*
	 *	Close all open file descriptors
	 */
	for(i=3; i<20; i++)
		close(i);
	errno = 0;	/* clear probable EBADF from bogus close */

	ccode =  MsgBindDomain(MSG_DOMAIN_SAPD, MSG_DOMAIN_NPS_FILE, MSG_NPS_REV_STR);
	if(ccode != NWCM_SUCCESS) {
		/* Do not internationalize */
		fprintf(stderr,"%s: Unable to bind message domain. NWCM error = %d. Exiting.\n",
			titleStr, ccode);
		SapExit(1);
	}


    if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL) {
        FPRINTF( stderr, MsgGetStr(SAP_BAD_CONFIG), titleStr);
        FPRINTF( stderr, MsgGetStr(SAP_ERROR_EXIT), titleStr);
        return(FAILURE);
    }

    /*
    **  Make sure SAPd is not running
    */
	if( LogPidKill(PidLogDir, (char *)program, 0) == SUCCESS) {
		/*
		**	Sap is already running, just exit
		*/
        FPRINTF( stderr, MsgGetStr(SAP_ACTIVE));
		exit(1);
	}
	
	if (!getenv("NWUENV")) {
		switch ((int)fork()) {
		case -1:
			FPRINTF(SapLogFile, MsgGetStr(SAP_FORK_FAIL), titleStr);
			logperror("");
			SapExit(1);
			/* NOTREACHED */
		case 0:
			if(setpgrp() == -1) {
				FPRINTF(SapLogFile, MsgGetStr(SAP_SESSION), titleStr);
				logperror("");
				SapExit(1);
				/* NOTREACHED */
			}

			if((pid = fork()) < 0) {
				FPRINTF(SapLogFile, MsgGetStr(SAP_FORK_FAIL), titleStr);
				logperror("");
				SapExit(1);
				/* NOTREACHED */
			}
			else if(pid > 0)
				exit(0);	/* second child */

			umask(022);
			break;
		default:
			exit(0);
		}
	}

	/*
	**	Set up signal handlers
	*/
	sigignore( SIGCLD );
	sigset(SIGHUP, Hangup);
	sigset(SIGTERM, Term);
	sigset(SIGQUIT, Term);
	sigset(SIGINT, Term);
	
	localServerName[0] = 0;

	while ((car=(char)getopt(argc, argv, "t")) != (char)-1) {
		switch (car) {
			case 't' : 
				TrackFlag = TRUE;
				break;
		}
	}

	if( SetUpSapParameters() != 0 )  {
		SapExit(1);
		/* NOTREACHED */
	}

	/*
	**	Must open one or the other of lipmx or ipx
	**	After open, use same FD for both.
	*/
	if( (ipxFd=open(ipxDevice, O_RDWR)) < 0) {
		if( (lipmxFd=open(lipmxDevice, O_RDWR)) < 0) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_OPEN_FAIL), titleStr, ipxDevice);
			logperror("");
			SapExit(1);
			/* NOTREACHED */
		}
		ipxFd = lipmxFd;
	} else {
		lipmxFd = ipxFd;
	}
#if IOCTLDEBUG
	FPRINTF(SapLogFile, "%s: lipmxFd at open is '%d'\n", titleStr, lipmxFd);
#endif


	if( (ripxFd=open(ripxDevice, O_RDWR))<0) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_OPEN_FAIL), titleStr, ripxDevice);
		logperror("");
		SapExit(1);
		/* NOTREACHED */
	}
#if IOCTLDEBUG
	FPRINTF(SapLogFile, "%s: ripxFd at open is '%d'\n", titleStr, ripxFd);
#endif

	ioc.ic_cmd = RIPX_SET_SAPQ;
	ioc.ic_timout = 5;
	ioc.ic_len = 0;
	ioc.ic_dp = NULL;
	if (ioctl(ripxFd, I_STR, &ioc) < 0) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_SET_RIPQ), titleStr);
		logperror("");
		SapExit(1);
		/* NOTREACHED */
	}

	ioc.ic_cmd = IPX_GET_CONFIGURED_LANS;
	ioc.ic_timout = 0;
	ioc.ic_dp = (char *)&configLan;
	ioc.ic_len = sizeof(IpxConfiguredLans_t);

	if( ioctl(lipmxFd, I_STR, &ioc) < 0) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_IOCTL_MAXLAN), titleStr);
		logperror("");
		SapExit(1);
		/* NOTREACHED */
	}
	ipxConfiguredLans = configLan.lans;
	sapMaxHops = configLan.maxHops;

	/* Save our PID in a static for use in PERMANENT advertised servers */
	SAPPid = getpid();

	if( GetLanInfo() != 0) {
		SapExit(1);
		/* NOTREACHED */
	}

	if( SetLanSapInfo() != 0) {
		SapExit(1);
		/* NOTREACHED */
	}

	if( SetUpNetEntryList() != 0) {
		SapExit(1);
		/* NOTREACHED */
	}

	if( SetSocket(ipxFd, &sapSocket, 5) == FAILURE) {
		SapExit(1);
		/* NOTREACHED */
	}

	if( SetHiLoWater(ipxFd, (uint32)UINT_MAX, (uint32)UINT_MAX, 5) == FAILURE) {
		SapExit(1);
		/* NOTREACHED */
	}

	if( SetUpSharedMemory() == FAILURE) {
		   SapExit(1);
		   /* NOTREACHED */
	}
	/*
	** Allocate read and write buffers, use the largest maxSDU from our
	** connected LANS.  If sapMaxPkt size is larger than maxSDU set 
	** sapMaxPkt to maxSDU
	*/
	maxSize = 0;
	for ( i = 1; i < ipxConfiguredLans; i++) {
		if (lanInfoTable[i].dlInfo.SDU_max > maxSize)
			maxSize = lanInfoTable[i].dlInfo.SDU_max; 
		if (lanInfoTable[i].ripSapInfo.sap.maxPktSize > 
				lanInfoTable[i].dlInfo.SDU_max) {
			lanInfoTable[i].ripSapInfo.sap.maxPktSize =
				lanInfoTable[i].dlInfo.SDU_max;
			FPRINTF(SapLogFile, MsgGetStr(SAP_MAX_BUF), titleStr, 
				lanInfoTable[i].dlInfo.SDU_max, i);
		}
	}

	/* ALLOC read buffer */
	readBufPtr = NWALLOC(maxSize);
	if( readBufPtr == NULL) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_BUF), titleStr);
		SapExit(1);
	}
	sendBufPtr = NWALLOC(maxSize);
	if( sendBufPtr == NULL) {
		FPRINTF(SapLogFile, MsgGetStr(SAP_ALLOC_BUF), titleStr);
		SapExit(1);
	}

	sigset( SIGALRM, AlarmHandler);
	sigset( SIGUSR1, TrackFlagOn);
	sigset( SIGUSR2, TrackFlagOff);
	sigset( SIGPIPE, DumpTables);
#if MEMORYDEBUG
	sigset( SIGTRAP, CheckMemory);
#else /* MEMORYDEBUG */
#ifdef SAPDEBUG
	sigset(SIGTRAP, Ignore);
#else
	sigignore(SIGTRAP);
#endif /* SAPDEBUG */
#endif /* MEMORYDEBUG */

/*
**	Ignore signals that just cause process to exit
*/
#ifdef SAPDEBUG
	sigset(SIGPOLL, Ignore);
	sigset(SIGVTALRM, Ignore);
	sigset(SIGPROF, Ignore);
#else
	sigignore(SIGPOLL);
	sigignore(SIGVTALRM);
	sigignore(SIGPROF);
#endif /* SAPDEBUG */

	time( &lastAlarmHit );
	alarm(SAP_INIT_TIMER);	/* Allow sap to get initial server list */

	SendDumpSapInfoPacket();

/*
**	If the user has set the option to not wait for SAP responses, write
**	the SapPid file now so NPSD can continue.
*/
	if(SapFastInit == TRUE) {
		if(WritePid())
			SapExit(1);
	}
			
	fds[0].fd = ripxFd;
	fds[0].events = POLLIN;
	fds[1].fd = ipxFd;
	fds[1].events = POLLIN;

	for( ;; ) {
		/*
		 *	Since we are about to block let us first service the clock if
		 *	we have gotten a hit.
		 */
		if (GotAlarm)
			AlarmService();
		if (poll(fds, 2, -1) <= 0) {
			validTime = 0;
			if (errno == EINTR)
				continue;
			FPRINTF(SapLogFile, MsgGetStr(SAP_POLL_ERR), titleStr, errno);
			SapExit(1);
		}
		validTime = 0;

		if (fds[0].revents & POLLIN) {	/* data to read from ripx */
			ReadRipPacket();
			validTime = 0;
		}
		if (fds[0].revents & POLLHUP) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_HANGUP), titleStr);
			SapExit(0);
			/* NOTREACHED */
		}
		if (GotAlarm)
			AlarmService();

		if (fds[1].revents & POLLIN) {	/* data to read from ipx */
			if( ReadSapPacket(ipxFd, readBufPtr, maxSize)) {
				ConsumeAdvertisingPacket( readBufPtr, 
					readBufPtr->ipxHdr.src.node);
				validTime = 0;
			}
		}
		if (fds[1].revents & POLLHUP) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_HANGUP), titleStr);
			SapExit(0);
			/* NOTREACHED */
		}
	}
	/*NOTREACHED*/
}	

/******************************************************************/
/*
**	Indicate we are to terminate
*/
/*ARGSUSED*/
STATIC void
Term( int sig)
{
	terminate = 1;
	return;
}

/******************************************************************/
/*
**	Shut Down this process, clean up and inform the world we are going down
*/
STATIC void
ShutDownSap( void)
{
    ServerEntry_t		*Server;
	InfoSourceEntry_t	*Info;
    netInfo_t           *Net;
    int					svrIndex;
	int					count;

	validTime = 0;
	FPRINTF(SapLogFile, MsgGetStr(SAP_TERM_SIG), titleStr);

    /*
     *  Mark all servers down
     */
    svrIndex = 1;
    for (Server = SrvBase + svrIndex; svrIndex <= ShmBase->D.ConfigServers;
            Server = SrvBase + svrIndex) {
        svrIndex++;
		if( Server->HopsToServer < sapMaxHops) {
            Info = Server->SourceListLink;
            while( Info != NULL) {
				Info->HopsToSource = SAP_DOWN;
                Info = Info->NextSource;
            }
			Server->RevisionStamp = newStamp;
			Server->HopsToServer = SAP_DOWN;
			AddToChangedList( Server, "term     ");
		}
	}

	if( ChangedLink != 0) {
		/*
		**  Now propogate any downed services to our other lans
		**  Don't send info on internal lan
		*/
		for ( count = 1; count < ipxConfiguredLans; count++) {
			Net = LocalNet[count];
			if (Net != NULL) {
				/*
				**  Send all packets of type requested
				*/
				SendChangedServerInformation( count,Net->netIDNumber,ALLHOSTS,
					GETINT16(SAP_SAS), PACKET_DELAY, CHECK_SPLIT_HORIZ);
			}
		}

		/*
		**  All changes propogated, empty list of changed servers
		*/
#if CHANGEDEBUG
		FPRINTF( SapLogFile, "%s: Term: Zero the change list was 0x%X\n",
		titleStr, ChangedLink);
#endif
		PruneSrvSource();
		ChangedLink = 0;
	}

	return;
}

/******************************************************************/
/*ARGSUSED*/
STATIC void
Hangup( int sig)
{
	validTime = 0;
	FPRINTF(SapLogFile, MsgGetStr(SAP_HANGUP_SIG), titleStr);
	SapExit(0);
	/* NOTREACHED */
}

/******************************************************************/
STATIC void
SapExit( int status)
{
	ServerEntry_t *Server;
	int i;
	uint16 serverTypes[SERVER_ARRAY_SIZE];
	uint16 typeCount[SERVER_ARRAY_SIZE];
	int serverTypesCounted = 0;
	int serverTypesNotCounted = 0;
	int	numServers = 0;
	InfoSourceEntry_t *Info, *NextInfo;
	int	svrIndex;
	NotifyStruct_t *np, *lnp;
	char *PidLogDir;

#ifdef SAPDEBUG
	FPRINTF(SapLogFile, "%s: SapExit: shmid is %d\n", titleStr, shmid);
#endif

   if( (PidLogDir = (char *)NWCMGetConfigDirPath()) == NULL) {
		FPRINTF( SapLogFile, MsgGetStr(SAP_BAD_CONFIG), titleStr);
	} else {
        (void) DeleteLogPidFile(PidLogDir, (char *)program);
	}

	if( shmid != -1) {
#ifdef SAPDEBUG
		FPRINTF(SapLogFile,
			"%s: SrvBase = 0x%X, ConfigServers = 0x%X, Index = 0x%X\n",
			titleStr,SrvBase,ShmBase->D.ConfigServers,ShmBase->D.ServerPoolIdx);
#endif
		numServers = ShmBase->D.ServerPoolIdx;
		svrIndex = 1;
		for( Server = SrvBase + svrIndex; svrIndex <= ShmBase->D.ConfigServers;
				Server = SrvBase + svrIndex) {
			svrIndex++;
			/*
			**	Check if alread in array of types
			*/
			for (i=0; i<serverTypesCounted; i++) {
				if (serverTypes[i] == Server->ServerType)
					break;
			}
			if (Server->ServerType == serverTypes[i])
				typeCount[i]++;
			else {
				if( serverTypesCounted <  (SERVER_ARRAY_SIZE - 2)) {
					serverTypes[serverTypesCounted] = Server->ServerType;
					typeCount[serverTypesCounted] = 1;
					serverTypesCounted++;
				} else {
					serverTypesNotCounted++;
				}
			}
			Info = Server->SourceListLink;
			Server->SourceListLink = 0;
			while (Info != NULL) {
				NextInfo = Info->NextSource;
				NWFREE((char *)Info);
				Info = NextInfo;
			}
		}

		for (i=0; i<ipxConfiguredLans; i++) {
			if (LocalNet[i] != NULL) {
				NWFREE(LocalNet[i]);
				LocalNet[i] = NULL;
			}
		}
		if( lanInfoTable)
			NWFREE(lanInfoTable);
		if( lanSapInfo)
			NWFREE(lanSapInfo);
		if( LocalNet)
			NWFREE(LocalNet);

		
		np = NotifyList;
		while( np != NULL ) {
			lnp = np->Next;
			NWFREE( np);
			np = lnp;
		}
		if (readBufPtr)
			NWFREE((char *)readBufPtr);
		if (sendBufPtr)
			NWFREE((char *)sendBufPtr);
#if MEMORYDEBUG
		CheckMemory(0);
#endif
		/*
		**	Release shared memory
		*/
		if( shmid != -1) {
			shmdt( (void *)ShmBase);
			shmctl( shmid, IPC_RMID, NULL);
		}
		FPRINTF(SapLogFile, MsgGetStr(SAP_SERVERS), titleStr,
			numServers, serverTypesCounted);
		if( serverTypesNotCounted) {
			FPRINTF(SapLogFile, MsgGetStr(SAP_NOT_COUNTED), titleStr,
				serverTypesNotCounted);
		}
	}
	if( status) {
		/*
		**	When SAPD dies, take down NPSD as well
		*/
		FPRINTF(SapLogFile, MsgGetStr(SAP_KILLING_NPSD), titleStr);
		for( i=0; i<20; i++) {
			if( killNPSD() == SUCCESS) {
				break;
			}
			sleep(1);
		}
		FPRINTF(SapLogFile, MsgGetStr(SAP_ERROR_EXIT), titleStr);
	} else {
		FPRINTF(SapLogFile, MsgGetStr(SAP_NORMAL_EXIT), titleStr);
	}
	exit(status);
}

STATIC void
logperror(const char *errmess)
{
	char	*message;

	if(errmess[0] != NULL) {
		FPRINTF(SapLogFile, "%s: %s\n", errmess, strerror(errno));
	} else {
		FPRINTF(SapLogFile, "%s\n", strerror(errno));
	}
#if IOCTLDEBUG
	FPRINTF(SapLogFile, "%s: ripxFd at failure is '%d'\n", titleStr, ripxFd);
	FPRINTF(SapLogFile, "%s: lipmxFd at failure is '%d'\n", titleStr, lipmxFd);
#endif
}
