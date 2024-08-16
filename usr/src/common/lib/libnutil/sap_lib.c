/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/sap_lib.c	1.15"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: sap_lib.c,v 1.44 1994/08/29 22:33:17 mark Exp $"

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

#include <fcntl.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <stropts.h>
#include <poll.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include "util_proto.h"
#include "memmgr_types.h"
#include "nwconfig.h"
#include <sys/lipmx_app.h>
#include <sys/nwtdr.h>
#include "sap_lib.h"
#include "nps.h"
#include <mt.h>
#ifdef _REENTRANT
#include "nwutil_mt.h"
#endif
#include <limits.h>

extern int errno;

/*
**  shared memory information
*/
static int	            shmid = -1;     	/* Shared memory id */
static key_t			shmkey = -1;		/* Shared memory key */
static SapShmHdr_t      *ShmBase = NULL;	/* Shared memory address */
static uint32     	    *HashBase = NULL;	/* Hash Table memory address */
static ServerEntry_t    *SrvBase;       	/* Server entry address base */
static SAPL				*LanBase;			/* Base of lan tables */
static AdvertList_t     *listHead = NULL;   /* List of Advert. servers head */
static AdvertList_t     *listTail = NULL;   /* List of Advert. servers tail */
static int				SAPDRunning = 0;	/* Is Sap Daemon present? */
static int				MemoryMapped = 0;	/* Has SAPMapMemory been called? */
static int 				ref_cnt = 0;		/* MemMap reference counter */
static char				testFile[] = "/tmp/persistTest";
static char				tmpPersist[] = "/tmp/tmpPersist";
static char				persistBuf[NWCM_MAX_STRING_SIZE];
const  char				persistFile[] = "sapouts";

/*
**	Doesn't get defined if POSIX on or strict ANSI on
*/
extern void (*sigset(int, void (*)(int)))(int);


/******************************************************************
**	SearchPersistList: Try to find the entry in the file
**						Return 1 if found, return 0 if not.
******************************************************************/
/*
 * int strcmpi(s1, s2)
 *
 *	Like strcmp(), but case insensitive.
 */

int
strcmpi(char * s1, char * s2)
{
	int	r;

	while (*s1 && *s2)
		if ((r = (int) (toupper(*s1++) - toupper(*s2++))) != 0)
			return r;
	return (int) (toupper(*s1) - toupper(*s2));
}

static int		
SearchPersistList(
	int 			pfd,
	PersistRec_t	*persist
)
{
	char	buf[sizeof(PersistRec_t)];
	int 	ret;
	
	
/*
**	Read the persist file, if name and type are duplicate, return
*/
	lseek(pfd, 0L, 0);

	while((ret = read(pfd, buf, sizeof(PersistRec_t))) != 0)
	{
		if(ret == -1)
		{
			if(errno == EINTR)
				continue;
			else
				return( 1 );
		}
		if(ret != sizeof(PersistRec_t))
			break;

		if(memcmp((char *)&persist->ListRec.ServerName, buf+sizeof(uint32),
							sizeof(PersistList_t)-sizeof(uint16)) == 0)
		{
			return( 1 );
		}
	}

	return( 0 );
}


/******************************************************************
**	RemoveFromPersistList:  Copy the already opened persist file to
**		a temporary	file, leaving out the record we want to remove.
**		Then copy back the temp file.
******************************************************************/
static void		
RemoveFromPersistList(
	int 			pfd,
	PersistRec_t	*persist,
	int 			nameIsNull
)
{
	char	buf[sizeof(PersistRec_t)];
	int 	ret;
	int		tmpFd;
	int 	flag = FALSE;


	persist->StartRec = STARTVAL;
	persist->StopRec = ENDVAL;

/*
**	If name is NULL, Check for UNIXWARE_TYPE, UNIXWARE_REMOTE_APP_TYPE,
**	and UNIXWARE_INSTALL_TYPE.  Set the appropriate NWCM flag.
*/
	if( nameIsNull )
	{
		if(persist->ListRec.ServerType == UNIXWARE_TYPE)
			ret = NWCMSetParam("sap_unixware", NWCP_BOOLEAN, (void *)&flag );
		else if(persist->ListRec.ServerType == UNIXWARE_INSTALL_TYPE)
			ret = NWCMSetParam("sap_install_server", NWCP_BOOLEAN, (void *)&flag );
		else if(persist->ListRec.ServerType == UNIXWARE_REMOTE_APP_TYPE)
			ret = NWCMSetParam("sap_remote_apps", NWCP_BOOLEAN, (void *)&flag );
	}

/*
**	Create a temporary persist file, copy all records to it except the
**	one that matches *persist and bad records
*/
	if((tmpFd = creat(tmpPersist, 0644)) < 0)
		return;

	lseek(pfd, 0L, 0);

	while((ret = read(pfd, buf, sizeof(PersistRec_t))) != 0)
	{
		if(ret == -1)
		{
			if(errno == EINTR)
				continue;
			else {
				close(tmpFd);
				unlink(tmpPersist);
				return;
			}
		}
		if(ret != sizeof(PersistRec_t))
		{
			close(tmpFd);
			unlink(tmpPersist);
			return;
		}

		if(memcmp((char *)&persist->ListRec.ServerName, buf+sizeof(uint32),
							sizeof(PersistList_t)-sizeof(uint16)) == 0)
		{
			continue;
		}
		else if(memcmp(buf,(char *)&persist->StartRec,sizeof(uint32)) ||
				memcmp(&buf[sizeof(PersistRec_t)-sizeof(uint32)],
						(char *)&persist->StopRec,sizeof(uint32)))
		{
			continue;
		} else {
			(void) write(tmpFd, buf, sizeof(PersistRec_t));
		}
	}

/*
**	Close and re-open temp. file so we can read it.
*/
	close(tmpFd);
	if((tmpFd = open(tmpPersist, O_RDONLY)) < 0) {
		unlink(tmpPersist);
		return;
	}

/*
**	Re-create the persist file, copy all records to it from temp. file.
*/
	close(pfd);
	if((pfd = open(persistBuf, O_TRUNC | O_RDWR)) < 0)
	{
		close(tmpFd);
		unlink(tmpPersist);
		return;
	}

	while((ret = read(tmpFd, buf, sizeof(PersistRec_t))) != 0)
	{
		if(ret == -1)
		{
			if(errno == EINTR)
				continue;
			else {
				close(pfd);
				close(tmpFd);
				unlink(tmpPersist);
				return;
			}
		}
		(void) write(pfd, buf, ret);
	}
	close(tmpFd);
	unlink(tmpPersist);
	return;
}


/******************************************************************
**	AddToPersistList: Try to add a permanent advertisement to
**					the /etc/netware/sapouts file.
**					Set NWCM flag (if neccessary)
******************************************************************/
static void		
AddToPersistList(
	int 			pfd,
	PersistRec_t	*persist,
	int 			nameIsNull
)
{
	int 	flag = TRUE;
	

/*
**	If name is NULL, Check for UNIXWARE_TYPE, UNIXWARE_REMOTE_APP_TYPE,
**	and UNIXWARE_INSTALL_TYPE.  Set the appropriate NWCM flag.
*/
	if( nameIsNull )
	{
		if(persist->ListRec.ServerType == UNIXWARE_TYPE)
			NWCMSetParam("sap_unixware", NWCP_BOOLEAN, (void *)&flag );
		else if(persist->ListRec.ServerType == UNIXWARE_INSTALL_TYPE)
			NWCMSetParam("sap_install_server", NWCP_BOOLEAN, (void *)&flag );
		else if(persist->ListRec.ServerType == UNIXWARE_REMOTE_APP_TYPE)
			NWCMSetParam("sap_remote_apps", NWCP_BOOLEAN, (void *)&flag );
	}

/*
**	Set end-of-file and write persist record
*/
	persist->StartRec = STARTVAL;
	persist->StopRec = ENDVAL;
	lseek(pfd, 0L, 2);
	(void) write(pfd, (char *)persist, sizeof(PersistRec_t));

	return;
}

/******************************************************************/
static int
GetSAPDState( void )
{
	char	routType[NWCM_MAX_STRING_SIZE];

	if(NWCMGetParam( "router_type", NWCP_STRING, routType ) != SUCCESS)
		return( -SAPL_NWCM );

	if(!strcmp(routType, "FULL"))
		SAPDRunning = TRUE;
	else
		SAPDRunning = FALSE;
	
	return( 0 );
}

/******************************************************************/
static int
GetNetAndNode(
	int 	 	fdNetwork,
	SapOp_t		*OutPkt)
{
	struct strioctl ioc;   
	static int		gotit = 0;
	static char		myNode[IPX_NODE_SIZE];
	static char		myNet[IPX_NET_SIZE];

	if( gotit == 0) {
		/*
		**	build ioctls to get my net and node number
		*/
		ioc.ic_cmd = IPX_GET_NET;
		ioc.ic_timout = 5;
		ioc.ic_len = sizeof(IpxNetAddr_t);
		ioc.ic_dp = myNet;
		if (ioctl(fdNetwork, I_STR, &ioc) == -1) {
			return(-SAPL_SOCKET);
		}

		ioc.ic_cmd = IPX_GET_NODE_ADDR;
		ioc.ic_timout = 5;
		ioc.ic_len = sizeof(IpxNodeAddr_t);
		ioc.ic_dp = myNode;
		if (ioctl(fdNetwork, I_STR, &ioc) == -1) {
			return(-SAPL_SOCKET);
		}
		gotit = 1;
	}
	memcpy((char *)OutPkt->IpxHeader.dest.net, myNet, IPX_NET_SIZE);	
	memcpy((char *)OutPkt->IpxHeader.dest.node, myNode, IPX_NODE_SIZE);	

	return(0);
}

/******************************************************************/
SapPrivatePkt(
	SapOp_t *OutPkt,	/* Ipx packet area with private data struct filled in */
	uint16 TotalLen,	/* Length of ipx pkt + private data structure */
	uint16 Operation)	/* Operation to send (machine order) */
{
	int ipxFd = -1;
	int	ret;
	struct strbuf Data;
	struct pollfd fds[1];
	int	timeout_count;
	int flags = 0;
	uint16	socket = 0;	/* Dynamic socket request */


    /*
    **  Get a socket to ipx so we can send packet to sapd
    */
    if( (ipxFd=open("/dev/ipx", O_RDWR)) < 0) {
        ret = -SAPL_IPXOPEN;
        goto NotifyErr;
    }

    if( (ret = SetSocket( ipxFd, &socket, 5)) < 0) {
        goto NotifyErr;
    }

	/*
	**  fill in request packet
	*/
	OutPkt->IpxHeader.chksum = GETINT16( IPX_CHKSUM);
	OutPkt->IpxHeader.len = GETINT16( TotalLen);
	OutPkt->IpxHeader.tc = 0;
	OutPkt->IpxHeader.pt = SAP_PACKET_TYPE;

	/*
	**  fill in dest address (private, so this is my net/node)
	*/
	if((ret = GetNetAndNode(ipxFd, OutPkt)) < 0)
	{
		goto NotifyErr;
	}

	PPUTINT16( SAP_SAS, OutPkt->IpxHeader.dest.sock);
	OutPkt->SapOperation = GETINT16( Operation);

	Data.maxlen = TotalLen;
	Data.len = TotalLen;
	Data.buf = (char *)OutPkt;

	fds[0].fd = ipxFd;
	fds[0].events = POLLIN;
	fds[0].revents = 0;

	for( timeout_count = 0; timeout_count < 10; timeout_count++) {
		/*
		**	Send the message to sapd
		*/
		OutPkt->SapOperation = GETINT16( Operation);
		if( putmsg( ipxFd, NULL, &Data, flags) == -1) {
			if(errno == EINTR)
				continue;
			else {
				ret = -SAPL_PUTMSG;
				goto NotifyErr;
			}
		}

		/*
		**	Clear Operation so if error we don't get confused
		*/
		OutPkt->SapOperation = 0;

		/*
		**	Wait for five seconds for the response
		*/
		poll( fds, 1, 5000);

		if( (fds[0].revents & POLLIN) == 0) {
			continue;
		}
		
		if( getmsg( ipxFd, NULL, &Data, &flags) == -1) {
			if(errno == EINTR)
				continue;
			else {
				ret = -SAPL_GETMSG;
				goto NotifyErr;
			}
		}
		break;
	}

	if( OutPkt->SapOperation == GETINT16(SAP_NACK)) {
		ret = -OutPkt->Status;
		goto NotifyErr;
	}

	/*
	**	Timed out
	*/
	if( OutPkt->SapOperation != GETINT16(SAP_ACK)) {
		ret = -SAPL_NORESP;
		goto NotifyErr;
	}
	close( ipxFd);
	return(0);

NotifyErr:
	if( ipxFd != -1)
		close( ipxFd);
	return( ret);
}

/******************************************************************/
static int
SetUpMappedMemory()
{
	const	char *configDir;

	if( shmid == -1) {
		if( (configDir = NWCMGetConfigFilePath()) == NULL) {
			return(-SAPL_NWCM);
		}

		if( (shmkey = ftok( configDir, SAP_PACKET_TYPE)) == -1) {
			return(-SAPL_FTOK);
		}

		if( (shmid = shmget( shmkey, 0, 0444)) == -1) {
			return(-SAPL_SHMGET);
		}
	}

	if((ShmBase = (SapShmHdr_t *)shmat( shmid, NULL, SHM_RDONLY)) == (void *)-1){
		return(-SAPL_SHMAT);
	}
	LanBase = LanBase = (SAPL *)((char *)ShmBase + ShmBase->LanInfo);
	SrvBase = (ServerEntry_t *)((char *)ShmBase + ShmBase->ServerPool);
	HashBase = (uint32 *)((char *)ShmBase + ShmBase->NameHash);

	return(0);
}

/******************************************************************/
static void
RemoveElement( AdvertList_t *delElement )
{
	AdvertList_t	*currPtr, *prevPtr;

	currPtr = prevPtr = listHead;
	do {
		if(currPtr == delElement)	
		{
			if(currPtr == listHead)
				listHead = currPtr->next;
			else if(currPtr == listTail)
			{
				listTail = prevPtr;
				listTail->next = NULL;
			}
			else
				prevPtr->next = currPtr->next;

			(void) free( currPtr );
			break;
		}
		else
		{
			prevPtr = currPtr;
			currPtr = currPtr->next;
		}
	} while (currPtr != NULL);
}
					
/******************************************************************/
/*
**	The StopSapList stops advertising of all services advertised by
**	the user process.
*/
void
StopSapList(void)
{
    AdvertList_t *currPtr;


    if(listHead == NULL)
        return;

	MUTEX_LOCK( &sap_list_lock );
    currPtr = listHead;
    do
    { 
        (void)SAPAdvertiseMyServer(currPtr->ServerType, currPtr->ServerName,
                                currPtr->ServerSocket, SAP_STOP_ADVERTISING );

        currPtr = currPtr->next;
    } while (currPtr != NULL);

    listTail = listHead = NULL;
	MUTEX_UNLOCK( &sap_list_lock );

    return;
}

/******************************************************************/
/*
**	The SAPMapMemory function attaches to shared memory
*/
int
SAPMapMemory( void)
{
	int ret;

	MUTEX_LOCK( &mem_map_lock );
	++ref_cnt;

	if(MemoryMapped == FALSE)
	{
		if((ret = GetSAPDState()) < 0)
		{
			--ref_cnt;
			MUTEX_UNLOCK( &mem_map_lock );
			return( ret );
		}

		if(SAPDRunning == FALSE)
		{
			--ref_cnt;
			MUTEX_UNLOCK( &mem_map_lock );
			return( 0 );
		}

		if( ShmBase == NULL)
		{
			ret = SetUpMappedMemory();
			if(ret)	
			{
				SAPDRunning = FALSE;
				--ref_cnt;
				MUTEX_UNLOCK( &mem_map_lock );
				return( ret );
			} else {
				MemoryMapped = TRUE;
			}
		}
	}
	MUTEX_UNLOCK( &mem_map_lock );
	return( 0 );
}

/******************************************************************/
/*
**	The SAPUnmapMemory function detaches from shared memory
*/
void
SAPUnmapMemory( void)
{
	MUTEX_LOCK( &mem_map_lock );

	--ref_cnt;
	if(ref_cnt > 0)
	{
		MUTEX_UNLOCK( &mem_map_lock );
		return;
	}
	if(ref_cnt < 0)
		ref_cnt = 0;

	if( ShmBase == NULL) {
		MUTEX_UNLOCK( &mem_map_lock );
		return;
	}
	shmdt((char *)ShmBase);
	ShmBase = NULL;
	MemoryMapped = FALSE;

	MUTEX_UNLOCK( &mem_map_lock );

	return;
}

/******************************************************************/
/*
**	The SAPStatistics function returns a structure filled with
**	statistics about server management
**	Not supported if SAPD not running.
*/
int
SAPStatistics( SAPDP sapstats)
{
	int	ret;
	int	MemInitState;

/*
**	Call MapMemory
*/
	MemInitState = MemoryMapped;
	if((ret = SAPMapMemory()) < 0)
		return( ret );

	if(SAPDRunning == FALSE)
		return( -SAPL_NOT_SUPPORTED );

	*sapstats = ShmBase->D;

	if(MemInitState == FALSE)
		SAPUnmapMemory();

	return(0);
}

/******************************************************************/
static int
SAPGetServers(
	uint16			 ServerType,
	int				*ServerEntry,
	SAPI			*ServerBuf,
	int				 MaxEntries,
	uint32			 TimeStamp,
	uint32			*NewTimeStamp,
	int				 downflag)
{
	ServerEntry_t	*Server;
	SAPI		 	*DServer;
	uint32			 numServers = 0;
	int				 svrIndex;

	if( MaxEntries == 0)
		return(0);

	/*
	**      Set new time stamp
	*/
	if( *NewTimeStamp == 0)
		*NewTimeStamp = ShmBase->D.RevisionStamp;

	svrIndex = *ServerEntry;
	if( svrIndex > ShmBase->D.ConfigServers) {
		return(0); 
	}
	if( svrIndex == 0)
		svrIndex = 1;
	for( Server = SrvBase + svrIndex; svrIndex <= ShmBase->D.ConfigServers;
			Server = SrvBase + svrIndex) {

		svrIndex++;
		/*
		**	Skip dead servers if down flag clear
		*/
		if( (downflag == 0) && (Server->HopsToServer == SAP_DOWN))
			continue;

		/*
		**	Skip if not a type we are requesting
		*/
		if( (ServerType != ALL_SERVER_TYPE)
				&& (ServerType != GETINT16(Server->ServerType))) {
			continue;
		}

		/*
		**	If this is an unused entry
		*/
		if( Server->RevisionStamp == 0) {
			continue;
		}
		/*
		**	Go to next entry if already reported this one
		*/
		if( TimeStamp > ShmBase->D.RevisionStamp) {
			/* We have wrapped the revision stamp */
			if( (Server->RevisionStamp > ShmBase->D.RevisionStamp)
					&& (Server->RevisionStamp <= TimeStamp)) {
				/* Seen this server before */
				continue;
			}
		} else {
			if( (Server->RevisionStamp <=TimeStamp)
					|| (Server->RevisionStamp > ShmBase->D.RevisionStamp)) {
				/* Seen this server before */
				continue;
			}
		}

		/*
		**	We have a match, fill in server info for this entry
		**	Make sure all data is in machine order
		*/
		DServer = &ServerBuf[numServers];
		DServer->serverType = GETINT16(Server->ServerType);
		strcpy((char *)DServer->serverName, (char *)&Server->ServerName[1]);
		IPXCOPYADDR(Server->ServerAddress, &DServer->serverAddress);
		DServer->serverHops = Server->HopsToServer;
		DServer->netInfo = Server->N;	/* Copy all of net information */
		DServer->netInfo.netIDNumber = GETINT32(Server->N.netIDNumber);
		numServers++;

		if( numServers >= MaxEntries)
			break;
	}
	/*
	**	All done, return next server entry to use, and number servers in buf
	*/
	*ServerEntry = svrIndex;
	return(numServers);
}

/******************************************************************/
/*
**	The SAPGetAllServers function fills ServerBuf with information about
**	active services of the type requested.  The service information is returned
**	as at most MaxEntries SAPI structures, in the buffer ServerBuf.
*/
int
SAPGetAllServers(
	uint16	 ServerType,
	int		*ServerEntry,
	SAPI	*ServerBuf,
	int		 MaxEntries)
{
	uint32	newtime;
	int s, ret;
	int MemInitState;

/*
**	Get SAPD state.  If error, return.  If SAPD is not running,
**	Hit the wire with a GNS request.
*/
	MemInitState = MemoryMapped;
	if((ret = SAPMapMemory()) < 0)
		return( ret );
	
	if(SAPDRunning)
	{
	/*
	**	Just call GetServers with a zero time stamp
	**	Ignore dead servers
	*/
		s = SAPGetServers( ServerType, ServerEntry, ServerBuf,
				MaxEntries, 0 ,&newtime, 0);

		if(MemInitState == FALSE)
			SAPUnmapMemory();
	} else {
		MUTEX_LOCK( &sap_list_lock );
		s = SAPRequestServers( SAP_GSQ, ServerType, ServerEntry, ServerBuf,
				MaxEntries );
		MUTEX_UNLOCK( &sap_list_lock );
	}
	return(s);
}

/******************************************************************/
/*
**	The SAPGetChangedServers function fills ServerBuf with information about
**	all services of the type requested that have changed since that last
**	time that the function was called..  The service information is returned
**	as at most MaxEntries SAPI structures, in the buffer ServerBuf.
**	All Server entries have been returned when the function return value is
**	less than MaxEntries or zero.
*/
int
SAPGetChangedServers(
	uint16	 ServerType,
	int		*ServerEntry,
	SAPI	*ServerBuf,
	int		 MaxEntries,
	uint32	 TimeStamp,
	uint32	*NewTimeStamp)
{
	int s, ret;
	int	MemInitState;

/*
**	Get SAPD state.  If error, return.  If SAPD is not running,
**	return NOT_SUPPORTED.
*/
	MemInitState = MemoryMapped;
	if((ret = SAPMapMemory()) < 0)
		return( ret );

	if(!SAPDRunning)
		return( -SAPL_NOT_SUPPORTED );

	/*
	**	Just call GetServers with a passed in time stamp
	**	Return dead servers
	*/
	s = SAPGetServers( ServerType, ServerEntry, ServerBuf,
			MaxEntries, TimeStamp , NewTimeStamp, 1);

	if(MemInitState == FALSE)
		SAPUnmapMemory();

	return(s);
}

/******************************************************************/
/*
**	The SAPGetNearestServer function returns the nearest server
**	of type ServerType.
*/
int
SAPGetNearestServer(
	uint16	 ServerType,
	SAPI	*ServerBuf)
{
	ServerEntry_t *Server, *Best;
	int			svrIndex;
	int			ret;
	int			MemInitState;


/*
**	Get SAPD state.  If error, return.  If SAPD is not running,
**	Hit the wire with a GNS request.
*/
	MemInitState = MemoryMapped;
	if((ret = SAPMapMemory()) < 0)
		return( ret );

	if(SAPDRunning)
	{
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

			if( (Server->ServerType != GETINT16(ServerType))
					|| (Server->HopsToServer == SAP_DOWN)) {
				continue;
			}

			/*
			**	If we find a local server, it is the best
			*/
			if( (Server->HopsToServer <= 1) &&
				(memcmp(Server->ServerAddress, ShmBase->D.MyNetworkAddress,
					IPX_NET_SIZE + IPX_NODE_SIZE) == 0)) {

				/* This server lives in this machine! */
				Best = Server;
				break;
			}

			/*
			**	To get started, assume nearest server is first server in list
			*/
			if (Best == NULL) {
				Best = Server;
			}

			/*
			**	If time is less than hops, time is bogus, skip this entry
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
			*/
			if ((Best->N.timeToNet > Server->N.timeToNet)
					|| ((Best->N.timeToNet == Server->N.timeToNet)
						&& (Best->N.hopsToNet > Server->N.hopsToNet))) {
					Best = Server;
			}
		}

		/*
		**	We didn't find anything, tell user the bad news
		*/
		if (Best == NULL) {
			if(MemInitState == FALSE)
				SAPUnmapMemory();
			return(0);
		}

		/*
		**	fill in response packet
		*/
		ServerBuf->serverType = GETINT16(Best->ServerType);
		strcpy((char *)ServerBuf->serverName, (char *)&Best->ServerName[1]);
		IPXCOPYADDR(Best->ServerAddress, &ServerBuf->serverAddress);
		ServerBuf->serverHops = Best->HopsToServer;
		ServerBuf->netInfo = Best->N;	/* Copy all of net information */
		ServerBuf->netInfo.netIDNumber = GETINT32(Best->N.netIDNumber);

		if(MemInitState == FALSE)
			SAPUnmapMemory();
		return(1);

	} else {
		MUTEX_LOCK( &sap_list_lock );
		svrIndex = 0;
		ret = SAPRequestServers( SAP_NSQ, ServerType, &svrIndex,
											ServerBuf, 1 );
		MUTEX_UNLOCK( &sap_list_lock );
		return( ret );
	}
}

/******************************************************************/
/*
**	The SAPAdvertiseMyServer function causes the named service
**	to be advertised by the SAP daemon.  This call need be made
**	only once.  As long as the process is active that made the
**	SAPAdvertiseMyServer function call, the services for that
**	process will be advertised.  When the process terminates,
**	the services for that process will be marked as SAP_DOWN.
*/
int
SAPAdvertiseMyServer(
	uint16	 ServerType,
	uint8	*ServerName,
	uint16	 Socket,
	int		 Action)
{
	SapAdvertise_t	OutPkt;
	AdvertList_t	*newElement, *currPtr;
	PersistRec_t	persist;
	char	upName[NWMAX_SERVER_NAME_LENGTH];
	char	serverName[NWCM_MAX_STRING_SIZE];
	char	*dirPtr;
	size_t	len;
	uint16	operation;
	int		ret, nameIsNull;
	int 	serviceInFile = FALSE;
	int 	tfd, pfd, opFlag;
	struct stat stbuf;
	time_t	curtime;
	int 	PersistExist = TRUE;

/*
**	Get SAPD state.  If error, return.  If SAPD is not running,
**	return NOT_SUPPORTED.
*/
	if((ret = GetSAPDState()) < 0)
		return( ret );
	
	if(!SAPDRunning)
		return( -SAPL_NOT_SUPPORTED );

	if( ServerType == 0)
		return(-SAPL_SERVTYPE);
	if( (len = strlen((char *)ServerName)) < 2)
		return(-SAPL_SERVNAME);
	if( len > (size_t)(NWMAX_SERVER_NAME_LENGTH - 1))
		return(-SAPL_SERVNAME);
	switch( Action) {
		case SAP_ADVERTISE_FOREVER:
			operation = SAP_ADVERTISE_PERMANENT;
			break;
		case SAP_ADVERTISE:
			operation = SAP_ADVERTISE_SERVICE;
			break;
		case SAP_STOP_ADVERTISING:
			operation = SAP_UNADVERTISE_SERVICE;
			break;
		default:
			return(-SAPL_INVALFUNC);
	}

/*
**	Build a path for the persist file
*/
	if((dirPtr = (char *)NWCMGetConfigDirPath()) == NULL)
		return(-SAPL_NWCM);
	strcpy(persistBuf, dirPtr);
	strcat(persistBuf, "/");
	strcat(persistBuf, persistFile);

/*
**	Stat the persist file do determine if it exists
*/
	while( stat(persistBuf, &stbuf) < 0)
	{
		if(errno != EINTR) {
			PersistExist = FALSE;	
			break;
		}
	}

/*
**	Open a file to guarantee a single process through this code at a time.
**	If the ADVERTISE_PERMANENT flag is set, try to open the file of
**	persistent SAP entries for read/write.  This will verify root permission.
**	If the UNADVERTISE_SERVICE flag is set, open read only to see if the
**	service was advertised as permanent.
*/
	if(operation == SAP_ADVERTISE_PERMANENT ||
							operation == SAP_UNADVERTISE_SERVICE)
	{
		while((tfd = open(testFile, O_CREAT | O_EXCL)) == -1)
		{
			if(errno == EACCES)
				return(-SAPD_INSUF_PERM);
			else {
/*
**	Stat the file to see how long it has been around.  It could be left over from
**	a failed or aborted call.  If more than 5 seconds, delete the test file.
*/
				curtime = time(0);
				if(stat(testFile, &stbuf) < 0)
					return(-SAPL_TRYAGAIN);

				if((curtime - stbuf.st_atime) < 5)
					return(-SAPL_TRYAGAIN);
				else
					unlink(testFile);
			}
		}

/*
**	If persist file does not exist, try to create it.  Success indicates
**	a root user.
*/
		if(!PersistExist)
		{
			if((pfd = creat(persistBuf, 0644)) < 0)
			{
				close(tfd);
				unlink(testFile);
				serviceInFile = FALSE;
			} else {
				PersistExist = TRUE;
				close(pfd);
			}
		}

		if(PersistExist)
		{
			opFlag = O_RDWR;
			while((pfd = open(persistBuf, opFlag)) == -1)
			{
				if(errno != EINTR)
				{
					if(opFlag == O_RDONLY)
					{
						close(tfd);
						unlink(testFile);
						return(-SAPD_INSUF_PERM);
					} else
						opFlag = O_RDONLY;	
				}
			}
		}
	}
/*
**	If not root privileged, no permanent ad. allowed.
*/
	if(operation == SAP_ADVERTISE_PERMANENT && opFlag == O_RDONLY)
	{
		close(tfd);
		unlink(testFile);
		return(-SAPD_INSUF_PERM);
	}

/*
**	Upper case the name
*/
	for(ret=0; ret<=len; ret++)
		upName[ret] = (char)toupper(ServerName[ret]);	

/*
**	Get the system name.  If same as passed in name, store a NULL name
*/
	if((operation == SAP_ADVERTISE_PERMANENT ||
				operation == SAP_UNADVERTISE_SERVICE) && PersistExist)
	{
/*
**	Do a Remove of a NULL record.  This will force a compression of
**	the persistFile, thus removing bad records and ensuring a good state
*/
		if(operation == SAP_ADVERTISE_PERMANENT && opFlag == O_RDWR)
		{
			nameIsNull = FALSE;
			memset((char *)&persist, 0, sizeof(PersistRec_t));
			RemoveFromPersistList( pfd, &persist, nameIsNull );
		}
	
		if(NWCMGetParam( "server_name", NWCP_STRING, serverName ) != SUCCESS) 
		{
			close(pfd);
			close(tfd);
			unlink(testFile);
			return( -SAPL_NWCM );
		}
		memset((char *)persist.ListRec.ServerName, 0, NWMAX_SERVER_NAME_LENGTH);
		if(strcmpi(serverName, upName) == 0)
		{
			nameIsNull = TRUE;
		} else {
			strcpy((char *)persist.ListRec.ServerName, upName);
			nameIsNull = FALSE;
		}
	}

/*
**	Then read the persist file to determine if the service is listed
**	there.  If it is, then we must be a root user to unadvertise it.
*/
	if((operation == SAP_UNADVERTISE_SERVICE) && PersistExist)
	{
		persist.ListRec.ServerType = ServerType;
		if((SearchPersistList( pfd, &persist )) == 1)
		{
			if(opFlag != O_RDWR)
			{
				close(pfd);
				close(tfd);
				unlink(testFile);
				return(-SAPD_INSUF_PERM);
			}
			serviceInFile = TRUE;
		} else {
			close(pfd);
			close(tfd);
			unlink(testFile);
			serviceInFile = FALSE;
		}
	}
	
/*
**	Issue the private packet to tell it to do it's thing.
*/
	memset( (char *)&OutPkt, 0, sizeof(OutPkt));
	PPUTINT16( Socket, OutPkt.ServerSocket);
	strcpy( (char *)OutPkt.ServerName, upName);
	OutPkt.Pid = getpid();
	OutPkt.Uid = geteuid();
	OutPkt.ServerType =  GETINT16( ServerType);

	if((ret = SapPrivatePkt( (SapOp_t *)&OutPkt,
					(uint16)sizeof( OutPkt), operation)) != 0)
	{
		if(operation == SAP_ADVERTISE_PERMANENT ||
							operation == SAP_UNADVERTISE_SERVICE)
		{
			if(serviceInFile && (ret == -SAPD_NOFIND))
				ret = 0;
			else {
				close(pfd);
				close(tfd);
				unlink(testFile);
				return( ret );
			}
		}
	}

/*
**	Advertise worked OR was a startup Unadvertise, if a permanent advertise,
**	write to the file.
*/ 
	if((operation == SAP_ADVERTISE_PERMANENT) && PersistExist)
	{
		persist.ListRec.ServerSocket = Socket;
		persist.ListRec.ServerType = ServerType;
		if((SearchPersistList( pfd, &persist )) == 0)
		{
		 	AddToPersistList( pfd, &persist, nameIsNull );
		}
		close(pfd);
		close(tfd);
		unlink(testFile);
	}
	else if(operation == SAP_UNADVERTISE_SERVICE && opFlag == O_RDWR)
	{
		if(serviceInFile)
			RemoveFromPersistList( pfd, &persist, nameIsNull );
		close(pfd);
		close(tfd);
		unlink(testFile);
	}

/*
**	The mutex lock may already be held if this function is being
**	called iteratively from StopSapList().  If so, do not release the
**	lock upon completion.  If _REENTRANT is not defined, MUTEX_TRYLOCK
**	is defined to be 0 (SUCCESS).
*/
	ret = MUTEX_TRYLOCK( &sap_list_lock );

/*
**	If advertising, add to the list of advertised services
**	else remove it (of course)
*/

	if( Action == SAP_ADVERTISE  || Action == SAP_ADVERTISE_FOREVER )
	{
		newElement = (AdvertList_t *)malloc((unsigned)sizeof(AdvertList_t) );
		if(newElement == NULL)
		{
			MUTEX_UNLOCK( &sap_list_lock );
			return (-SAPL_ENOMEM );
		}

		newElement->next = NULL;
		strcpy((char *)newElement->ServerName, upName );
		newElement->ServerType = ServerType;
		newElement->ServerSocket = Socket;

/*
**	If this is the first entry to the process's list, make it the head.
**	Otherwise, throw it on the end.
*/
		if(!listHead)
			listHead = newElement;
		else
			listTail->next = newElement;

		listTail = newElement;

	} else {
/*
**	Remove the element from the advertised list
*/
		if(listHead != NULL)
		{	
			currPtr = listHead;
			do {
				if((currPtr->ServerType == ServerType) &&	
					(!strcmp(upName, (char *)currPtr->ServerName)))
				{
					RemoveElement( currPtr );
					break;
				} else
					currPtr = currPtr->next;

			} while (currPtr != NULL);
		}
	}
	if(ret == 0)	/* mutex not previously locked */
	{
		MUTEX_UNLOCK( &sap_list_lock );
	}

	return( 0 ); 
}

/******************************************************************
/*
**	The SAPGetServerByName function requests the service entry
**	by name and type.  If type is ALL_SERVER_TYPE, then all services for
**	by that name are returned.
**	All Server entries have been returned when the function return value is
**	less than MaxEntries or zero.
*/
int
SAPGetServerByName(
	uint8	*ServerName,
	uint16	 ServerType,
	int		*ServerEntry,
	SAPI	*ServerBuf,
	int		 MaxEntries)
{
	ServerEntry_t	*Server;
	SAPIP		 	 DServer;
	SAPI	server;
	int		 len, wildlen;
	uint32	 hash;
	uint8	 Name[NWMAX_SERVER_NAME_LENGTH];
	uint32	*HashEntry;
	int		wild = FALSE;
	int		numServers = 0;
	int		ret;
	int		MemInitState;		

/*
**	Get SAPD state.  If error, return.  If SAPD is not running,
**	Hit the wire with a GNS request.
*/
	MemInitState = MemoryMapped;
	if((ret = SAPMapMemory()) < 0)
		return( ret );

	/*
	**	If not asking for anything, we are done
	*/
	if( MaxEntries == 0)
		return(0);

	if( ServerType == 0)
		return(-SAPL_SERVTYPE);
	/*
	**	Check name length
	*/
	if( (len = strlen( (char *)ServerName)) > (NWMAX_SERVER_NAME_LENGTH - 1))
		return(-SAPL_SERVNAME);
	if( len < 2)
		return(-SAPL_SERVNAME);

	if( ServerName[len - 1] == '*')
		wild = TRUE;	

	if(SAPDRunning)
	{

		if( (*ServerEntry < 0) || (*ServerEntry > ShmBase->D.ConfigServers)) {
			if(MemInitState == FALSE)
				SAPUnmapMemory();
			return(0);
		}
	
	/*
	**	We can do it the easy way if not a wild card search, i.e. use the
	**	name hash.
	**	Note: name is stored in shared memory with length as first character
	*/
		if( wild == FALSE) {
			/*
			**	Format the name and compute the hash
			*/
			memset( Name, 0, 14);
			Name[0] = (uint8)len;
			strcpy( (char *)&Name[1], (char *)ServerName);
			hash = PGETINT32(Name);
			hash += PGETINT32(&Name[4]);
			hash += PGETINT32(&Name[8]);
			hash = hash % ShmBase->NameHashSize;
			HashEntry = HashBase + hash;
			if( *HashEntry == 0) {
				if(MemInitState == FALSE)
					SAPUnmapMemory();
				return(0);
			}
			/*
			**	If ServerEntry is zero, start at beginning of hash
			*/
			if( *ServerEntry == 0)
				*ServerEntry = *HashEntry;
			/*
			**	Check each Server in the list for the correct name
			*/
			for( Server = SrvBase + *ServerEntry; Server != SrvBase;
						Server = SrvBase + Server->NameLink) {
				/*
				**	Skip dead servers
				*/
				if( Server->HopsToServer == SAP_DOWN)
					continue;

				/*
				** If not correct type, go on
				*/
				if( (ServerType != ALL_SERVER_TYPE)
						&& (ServerType != GETINT16(Server->ServerType))) {
					continue;
				}

				/*
				**	Size must match exactly
				*/
				if( *Name != *Server->ServerName)
					continue;
				if( strcmp( (char *)&Name[1], (char *)&Server->ServerName[1]) == 0){
					/*
					**	Name matches, copy the entry
					*/
					DServer = &ServerBuf[numServers];
					DServer->serverType = GETINT16(Server->ServerType);
					strcpy((char *)DServer->serverName, (char *)&Server->ServerName[1]);
					IPXCOPYADDR(Server->ServerAddress, &DServer->serverAddress);
					DServer->serverHops = Server->HopsToServer;
					DServer->netInfo = Server->N;	/* Copy all of net info */
					DServer->netInfo.netIDNumber = 
						GETINT32(DServer->netInfo.netIDNumber);
					numServers++;
				
					*ServerEntry = Server->NameLink;
					if( *ServerEntry == 0)
						*ServerEntry = -1;	/* All done */
					if( numServers >= MaxEntries)
						break;
				}
			}
		} else {
			/*
			**	This is a wild card search, we have to do it the hard way
			**	and search each entry
			**
			**	Don't try to compare length portion of name
			*/
			len--;

			/*
			**	If ServerEntry is zero, start at beginning
			*/
			if( *ServerEntry == 0)
				*ServerEntry = 1;
			for( Server = SrvBase + *ServerEntry;
					*ServerEntry <= ShmBase->D.ConfigServers;
					Server = SrvBase + *ServerEntry) {

				*ServerEntry += 1;
				/*
				**	Skip dead servers
				*/
				if( Server->HopsToServer == SAP_DOWN)
					continue;

				/*
				**	Skip if not a type we are requesting
				*/
				if( (ServerType != ALL_SERVER_TYPE)
						&& (ServerType != GETINT16(Server->ServerType))) {
					continue;
				}

				/*
				**	If target server name is too short, skip it
				*/
				if( (int)*Server->ServerName < len)
					continue;
				/*
				**	If we have a match up to wild card, 
				**	fill in server info for this entry.
				*/
				if( strncmp( (char *)ServerName, 
						(char *)&Server->ServerName[1], len) == 0) {
					DServer = &ServerBuf[numServers];
					DServer->serverType = GETINT16(Server->ServerType);
					strcpy((char *)DServer->serverName,
						(char *)&Server->ServerName[1]);
					IPXCOPYADDR(Server->ServerAddress, &DServer->serverAddress);
					DServer->serverHops = Server->HopsToServer;
					DServer->netInfo = Server->N;	/* Copy all of net info */
					DServer->netInfo.netIDNumber = GETINT32(Server->N.netIDNumber);
					numServers++;

					if( numServers >= MaxEntries)
						break;
				}
			}
		}
		if(MemInitState == FALSE)
			SAPUnmapMemory();

	} else {	/* No SAPD Running */

		wildlen = len - 1;
		while (numServers < MaxEntries)
		{
			ret = SAPGetAllServers(ServerType, (int *)ServerEntry, &server, 1);
			if(ret == 0)
				break;
			else if(ret < 0)
				return( ret );

			if(wild == TRUE)
			{
				if (!strncmp((char *)ServerName,
									(char *)server.serverName, wildlen))
				{
					(void)memcpy((char *)&ServerBuf[numServers],
									(char *)&server, sizeof(SAPI));
					numServers++;
				}
			} else {
				if (!strcmp((char *)ServerName, (char *)server.serverName))
				{
					(void)memcpy((char *)&ServerBuf[numServers],
									(char *)&server, sizeof(SAPI));
					numServers++;
				}
			}
		}
	}
	return( numServers);
}

/******************************************************************/
/*
**	Insulate the calling process from signal mechanism, do it all
**	under the covers.  The function is a simple callback, it gets
**	called when changes are available.  It needs only do its work
**	and return.
*/
static void (*CallBackFunction)(int sig) = (void(*)())0;

static void
SAPSigHandler( int sig)
{
	if( *CallBackFunction == (void(*)())0 )
		return;
	(*CallBackFunction)(sig);
}

/******************************************************************/
/*
**	The SAPNotifyOfChange function gives control to the specified function
**	when a change to one or more sap entries has occurred.
**	If Signal is SAP_STOP_NOTIFICATION, notification is halted.
*/
int
SAPNotifyOfChange(
	int		 Signal,
	void   (*Function)(int),
	uint16	 ServerType)
{
	SapNotify_t OutPkt;
	uint16 operation;
	int ret;

/*
**	Get SAPD state.  If error, return.  If SAPD is not running,
**	return NOT_SUPPORTED.
*/
	if((ret = GetSAPDState()) < 0)
		return( ret );

	if(!SAPDRunning)
		return( -SAPL_NOT_SUPPORTED );

	if( Signal == SAP_STOP_NOTIFICATION) {
		sigset( Signal, SIG_DFL);
		CallBackFunction = (void(*)())0;
	} else {
		if( CallBackFunction != (void(*)())0)
			return(-SAPL_DUPCALLBACK);
		if( sigset( Signal, SAPSigHandler) == SIG_ERR) {
			return(-SAPL_SIGNAL);
		}
		CallBackFunction = Function;
	}

	OutPkt.Pid = getpid();
	OutPkt.Signal = Signal;
	OutPkt.ServerType =  GETINT16( ServerType);
	if( Signal == SAP_STOP_NOTIFICATION) {
		operation = SAP_UNNOTIFY_ME;
	} else {
		operation = SAP_NOTIFY_ME;
	}

	if( (ret = SapPrivatePkt( (SapOp_t *)&OutPkt,
			(uint16)sizeof(OutPkt), operation)) != 0) {
		sigset( Signal, SIG_DFL);
		CallBackFunction = (void(*)())0;
		return( ret);
	}
	return( ret);
}

/******************************************************************/
/*
**	The SAPGetLanData function returns data about the LANs known by
**	SAP.
**
**	Returns 1 if successful
**			0 if lan index is out of range
**			a negative number if an error occurred.
*/
int
SAPGetLanData(int lan, SAPL *lanbuf)
{
	SAPL	*sapInfo;
	int 	ret;
	int		MemInitState;

/*
**	Get SAPD state.  If error, return.  If SAPD is not running,
**	return NOT_SUPPORTED.
*/
	MemInitState = MemoryMapped;
	if((ret = SAPMapMemory()) < 0)
		return( ret );

	if(!SAPDRunning)
		return( -SAPL_NOT_SUPPORTED );

	if( lan < 0 || lan > (int)ShmBase->D.Lans)
		return(0);

	sapInfo = &LanBase[lan];

	*lanbuf = *sapInfo;	/* Copy whole structure */

	if(MemInitState == MemoryMapped)
		SAPUnmapMemory();
	return(1);
}

/******************************************************************/
/*
**	The SAPListPermanentServers function requests a list of
**	servers that have been advertised with the SAP_ADVERTISE_FOREVER
**	Action flag.
*/
int
SAPListPermanentServers(
	int 			*ServerEntry,
	PersistList_t	*ServerBuf,
	int 			MaxEntries)
{
	char	*dirPtr;
	char	persistBuf[NWCM_MAX_STRING_SIZE];
	char	serverName[NWCM_MAX_STRING_SIZE];
	int 	tfd, permFd, ret, len, i;
	int 	numServers = 0;
	struct stat	stbuf;
	PersistRec_t persist;

/*
**	Build a path for the persist file
*/
	if((dirPtr = (char *)NWCMGetConfigDirPath()) == NULL)
		return(-SAPL_NWCM);
	strcpy(persistBuf, dirPtr);
	strcat(persistBuf, "/");
	strcat(persistBuf, persistFile);

/*
**	Get the size of the persist file.  If the size of the file is
**	less than the size of the structure multiplied by the offset,
**	return 0
*/
	ret = stat(persistBuf, &stbuf);
	if(ret < 0)
	{
		return( 0 );
	} else {
		if(stbuf.st_size <= (*ServerEntry * sizeof(PersistRec_t)))
		{
			return( 0 );
		}
	}

/*
**	Open a file to guarantee a single process through this code at a time.
*/
	if((tfd = open(testFile, O_CREAT | O_EXCL)) == -1)
	{
		if(errno == EACCES)
			return(-SAPD_INSUF_PERM);
		else
			return(-SAPL_TRYAGAIN);
	}

/*
**	Open the file for reading, seek to selected location.
*/
	if( (permFd = open(persistBuf, O_RDONLY )) == -1 ) {
		close(tfd);
		unlink(testFile);
		return ( -SAPL_OSAPOUTS ); 
	}

	lseek(permFd, (sizeof(PersistRec_t)*(*ServerEntry)), 0);
	while(numServers < MaxEntries)
	{
		ret = read(permFd, (char *)&persist, sizeof(PersistRec_t));
		if(ret != sizeof(PersistRec_t))
		{
			break;
		}
		else if(ret == 0)
		{
			break;
		}
		else if(ret == -1) {
			if(errno == EINTR)
				continue;
			else {
				close(tfd);
				close(permFd);
				unlink(testFile);
				return(-SAPL_RWSAPOUT);
			}
		}

/*
**	Check the Begin/End of record flags, if they are bad, skip it
*/
		if(persist.StartRec != STARTVAL || persist.StopRec != ENDVAL)
		{
			continue;
		}

/*
**	If server name in persist file is NULL, replace with system name
*/
		memset(serverName, 0, NWCM_MAX_STRING_SIZE);
		if(persist.ListRec.ServerName[0] == 0)
		{
			if(NWCMGetParam( "server_name", NWCP_STRING, serverName )!= 0)
			{
				close(permFd);
				close(tfd);
				unlink(testFile);
				return(-SAPL_NWCM);
			}
			len = strlen(serverName);
			for(i=0; i<len; i++)
				persist.ListRec.ServerName[i] = (char)toupper(serverName[i]);
		}
		memcpy((char *)&ServerBuf[numServers], (char *)persist.ListRec.ServerName,
						sizeof(PersistList_t));
		numServers++;

	}
	close(permFd);
	close(tfd);
	unlink(testFile);

	*ServerEntry += numServers;
	return(numServers);
}


/******************************************************************/
/*
**	The SAPGetServerByAddr function requests the service entry
**	by address and type.
*/
int
SAPGetServerByAddr(
	ipxAddr_t	*ServerAddr,
	uint16	 	ServerType,
	int 		*ServerEntry,
	SAPI		*ServerBuf,
	int 		MaxEntries)
{
	SAPI server;
	int err, numServers = 0;

	while(numServers < MaxEntries)
	{
		err = SAPGetAllServers(ServerType, ServerEntry, &server, 1);
		if(err == 0)
			break;
		else if(err < 0)
			return(err);

		if (IPXCMPNET(server.serverAddress.net, ServerAddr->net) &&
			IPXCMPNODE(server.serverAddress.node, ServerAddr->node))
		{
			(void)memcpy((char *)&ServerBuf[numServers],
							(char *)&server, sizeof(SAPI));
			numServers++;
		}
	}
	return( numServers );
}
