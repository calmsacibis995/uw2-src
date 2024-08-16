/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnwutil:common/lib/libnutil/sapreq.c	1.9"
#ident	"$Id: sapreq.c,v 1.16 1994/09/22 18:39:16 vtag Exp $"

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
#include <time.h>
#include <fcntl.h>
#ifdef OS_AIX
#include <sys/poll.h>
#else
#include <poll.h>
#include <malloc.h>
#endif /* OS_AIX */
#include <memory.h>
#include <string.h>
#include <errno.h>
#include <stropts.h>
#include "nwconfig.h"
#include "memmgr_types.h"
#include "sap_lib.h"
#include "nps.h"
#include <mt.h>
#include <util_proto.h>

#ifdef _REENTRANT
#include "nwutil_mt.h"
#endif

/*
**	Global stuff
*/

/* global definitions */
#define PACKET_DELAY	2	/* seconds */

/* static definitions */
typedef struct SAPIList {
	struct SAPIList	*next;
	SAPI			sapi;
} SAPIList_t;
static SAPIList_t *SAPIListHead;
	
	
#ifdef _REENTRANT
static void
freeListHead(void)
{
	struct sap_tsd	*key_tbl;

	key_tbl = (struct sap_tsd *)_mt_get_thr_specific_storage(
					listHeadKey, SAP_TSD_KEYTBL_SIZE);
	if(key_tbl == NULL)
		return;

	key_tbl->sap_listp = NULL;
}
#endif

static SAPIList_t *
getListHead(int *firstTime)
{
	SAPIList_t	*LH;

#ifdef _REENTRANT
	struct sap_tsd	*key_tbl;

	if(FIRST_OR_NO_THREAD) {
		if( SAPIListHead == NULL )
		{
			*firstTime = TRUE;
			SAPIListHead = (SAPIList_t *)calloc( 1, sizeof( SAPIList_t ));
		} else {
			*firstTime = FALSE;
		}	

		LH = SAPIListHead;
	} else {

		key_tbl = (struct sap_tsd *)_mt_get_thr_specific_storage(
						listHeadKey, SAP_TSD_KEYTBL_SIZE);
		if(key_tbl == NULL)
			return( NULL );

		if(key_tbl->sap_listp == NULL)
		{
			*firstTime = TRUE;
			key_tbl->sap_listp = (SAPIList_t *)calloc(1, sizeof(SAPIList_t));
		} else {
			*firstTime = FALSE;
		}
		LH = key_tbl->sap_listp;
	}

#else /* ! _REENTRANT */

	if( SAPIListHead == NULL )
	{
		*firstTime = TRUE;
		SAPIListHead = (SAPIList_t *)calloc( 1, sizeof( SAPIList_t ));
	} else {
		*firstTime = FALSE;
	}
	LH = SAPIListHead;

#endif

	return( LH );
}


SapOp_t				sapOp;
static int			fileDesc = -1;

static char ipxDevice[] = "/dev/ipx";

/* Local subroutines */
static void FreeList();
static int RequestServers( uint16, uint16 );
static int GetServers( void );
static int AddToList( uint16 *, char *, ipxAddr_t *, uint16 * );
#ifdef HARD_DEBUG
static void od(uint8 *, int);
#endif

/*
**	Main Procedure
*/
int SAPRequestServers(
	uint16		queryType,
	uint16		serverType,
	int			*index,
	SAPI		*serverBuf,
	int			maxEntries )	
{
	int 		count = 0;
	int			i;
	SAPIList_t	*list;

/*
**	Request the server list
*/
	if((queryType == SAP_NSQ) || ((queryType == SAP_GSQ) && (*index == 0)))
	{
		FreeList();
		if (RequestServers( queryType, serverType ) == -1)
			return( 0 );
		if (GetServers() == 0)
			return( 0 );
	}

/*
**	Index to requested spot
*/
	list = getListHead(&i);
	for(i=0; i<*index; i++)
		list = list->next;

/*
**	Return requested number of entries
*/
	while (list && (count < maxEntries))
	{
		(void) memcpy((char *)&serverBuf[count], (char *)&list->sapi,
								sizeof(SAPI));
		count++;
		list = list->next;
	}

	*index = *index + count;
	return(count);
}


/***********************************************************/
#ifdef HARD_DEBUG
static
void od(
	uint8	*ptr,
	int		len
)
{
	printf("od: ptr=%x len=%d\n", ptr, len);
	while (len)
	{
		printf("%02X ", 0xFF & *ptr++);
		len--;
	}
	printf("\n");
	return;
}
#endif


/***********************************************************/
static
void FreeList()
{
	SAPIList_t	*list;
	SAPIList_t	*listHead;
	int 		firstTime;

	listHead = getListHead( &firstTime );

	while (listHead)
	{
		list = listHead->next;
		free(listHead);
		listHead = list;
	}

#ifdef _REENTRANT
	if (FIRST_OR_NO_THREAD)
		SAPIListHead = NULL;
	else
		freeListHead();
#else
	SAPIListHead = NULL;
#endif

	return;
}


/***********************************************************/
static int
OpenSocket()
{
	uint32			hi, lo;
	uint16			socket = 0;


/*
**  Open the service socket for ncp.
*/
	if ((fileDesc = open(ipxDevice, O_RDWR)) == -1)
		return( -1 );

	if ((SetSocket( fileDesc, &socket, 2 )) < 0)
		return( -1 );

/*
**	Try to set a big high water mark for IPX
*/
	hi = lo = (uint32)0xFFFFFFFF;
	if((NWCMGetParam( "sap_servers", NWCP_INTEGER, &lo))==SUCCESS)
		(void) SetHiLoWater( fileDesc, hi, lo, 1);

	return( 0 );
}
 

/***********************************************************/
static
int SetupSAPPacket(
	uint16	queryType,
	uint16	serverType )
{
	uint16				temp16;
#ifdef HARD_DEBUG
	int					i;
	char		*tmp;
#endif

/*
**	Make the SAP request
*/
	sapOp.IpxHeader.chksum = (uint16)IPX_CHKSUM;
	sapOp.IpxHeader.tc = 0;
	sapOp.IpxHeader.pt = 0;
	sapOp.IpxHeader.len = GETINT16(sizeof(ipxHdr_t) + sizeof(SAPQ));
	(void) memset((char *)&sapOp.IpxHeader.src, 0, IPX_ADDR_SIZE );

/*
**	Get the destination network address
*/
	(void) memset((char *)sapOp.IpxHeader.dest.net, 0x00, IPX_NET_SIZE);
	(void) memset((char *)sapOp.IpxHeader.dest.node, 0xFF, IPX_NODE_SIZE);
	PPUTINT16( SAP_SAS, sapOp.IpxHeader.dest.sock );

/*
**	Build the destination message
*/
	sapOp.SapOperation = GETINT16( queryType );
	temp16 = GETINT16( serverType );
	(void) memcpy((char *)&sapOp.Status, (char*)&temp16, sizeof(uint16));

#ifdef HARD_DEBUG
	tmp = (char *)&sapOp;
	fprintf(stderr, "\nPacket addr.buf is :\n");
	for(i=0; i<sizeof(SapOp_t); i++)
		fprintf(stderr, "%x ", (uint8)(*tmp++));
#endif

	return( 0 );
}


/***********************************************************/
static
int SAPBroadcast()
{
	struct strbuf txDataBuf;

/*
**	Send the message
*/
	txDataBuf.len = sizeof(ipxHdr_t) + sizeof(SAPQ);
	txDataBuf.buf = (char *)&sapOp;

	if (putmsg(fileDesc, NULL, &txDataBuf, 0) == -1)
	{
#ifdef DEBUG
		printf("Error sending sap request\n");
#endif
		return(-1);
	}

	return(0);
}

/***********************************************************/
static
int RequestServers(
	uint16	queryType,
	uint16	serverType
)
{
	int					err;


/*
**	If not open already, open a TLI socket
*/
	if(fileDesc < 0)
	{
		if((err = OpenSocket()) < 0)
			return( -1 );
	}

/*
**	Setup the SAP Packet
*/
	if((err == SetupSAPPacket( queryType, serverType )) < 0)
		return( -1 );

	if((err == SAPBroadcast()) < 0)
		return( -1 );

	return(0);
}


/***********************************************************/
static
int GetServers( void )
{
	SapRespHeader_t		sapHead;
	SAPS				sapData;
	int 				flags = 0;
	int 				rtn;
	uint16				temp16, temp2;
	int					count = 0;
	struct pollfd		fds[1];
	struct strbuf		rDataBuf;


	fds[0].fd = fileDesc;
	fds[0].events = POLLIN;

/*
**	If there is no data in 10 seconds, return
*/
	if ((rtn = poll(fds, 1, 10000)) <= 0)
	{
		return(0);
	} 
	if(fds[0].revents & POLLHUP)
	{
		return( -1 );
	}

	do
	{
/*
**	Read off the header and operation
*/
		rDataBuf.len = rDataBuf.maxlen = sizeof(SapRespHeader_t);
		rDataBuf.buf = (char *)&sapHead;
		if((rtn = getmsg(fileDesc, 0, &rDataBuf, &flags)) < 0)
		{
#ifdef HARD_DEBUG
			printf("getmsg errno is '%d'\n", errno);
#endif
			return(-1);
		}
#ifdef HARD_DEBUG
		printf("Got %d bytes, status is %d\n", rDataBuf.len, rtn);
		printf("Sap head data: ");
		od((uint8*)&sapHead, (int)sizeof(SapRespHeader_t));
#endif

		while(rtn == MOREDATA)
		{
/*
**	I assume that once data is flowing it will not stop
**	for more than a couple seconds
*/
			rDataBuf.len = rDataBuf.maxlen = sizeof(SAPS);
			rDataBuf.buf = (char *)&sapData;
			if((rtn = getmsg(fileDesc, 0, &rDataBuf, &flags)) < 0)
				if(errno == EINTR)
					continue;
				else {
#ifdef HARD_DEBUG
					printf("getmsg errno is '%d'\n", errno);
#endif
					return(-1);
				}
#ifdef HARD_DEBUG
			printf("Got %d bytes, status is %d\n", rDataBuf.len, rtn);
			printf("Sap response data: ");
			od((uint8*)&sapData, (int)sizeof(SAPS));
#endif

/*
**	Add this entry to the list, but only if hop count is < SAP_DOWN.
*/
			temp16 = GETINT16(sapData.serverHops);
			if(temp16 < SAP_DOWN)
			{
				temp2 = GETINT16(sapData.serverType);
				count += AddToList( &temp2, (char *)sapData.serverName,
						&sapData.serverAddress, &temp16);
			}
		}

	} while((rtn = poll(fds, 1, (PACKET_DELAY * 1000))) > 0);

	return( count );
}


/***********************************************************/
static int
AddToList(
	uint16		*type,
	char		*name,
	ipxAddr_t	*addr,
	uint16		*hops )
{
	static SAPIList_t	*listTail = NULL;
	SAPIList_t	*newPtr;
	int			firstTime;


/*
**	Make sure we don't have the same server multiple times.
*/
	if((newPtr = getListHead( &firstTime )) == NULL)
		return( 0 );

	if(!firstTime)
	{
		while(newPtr)
		{
			if((!strcmp((char *)newPtr->sapi.serverName, name)) &&
						(newPtr->sapi.serverType == *type))
				return( 0 );

			newPtr = newPtr->next;
		}
	}

	if(!firstTime)
	{
		if((newPtr = (SAPIList_t *)calloc(1, sizeof(SAPIList_t))) == NULL)
		{
#ifdef DEBUG
			printf("Unable to calloc SAPIList_t element\n");
#endif
			return(0);
		}
	}

	newPtr->next = NULL;
	(void) strncpy((char *)newPtr->sapi.serverName, name, NWMAX_SERVER_NAME_LENGTH);
	(void) memcpy((char *)&newPtr->sapi.serverAddress, (char *)addr,
									IPX_ADDR_SIZE);
	newPtr->sapi.serverHops = *hops;
	newPtr->sapi.serverType = *type;
	(void) memset((char *)&newPtr->sapi.netInfo, 0, sizeof(netInfo_t));

#ifdef HARD_DEBUG
	printf("New Entry:");
	od((uint8 *)newPtr, sizeof(SAPIList_t));
#endif

/*
**	If this is the first entry, just add it
*/
	if (!firstTime)
		listTail->next = newPtr;

	listTail = newPtr;
		
	return(1);
}


