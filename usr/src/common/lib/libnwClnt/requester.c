/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:requester.c	1.11"

#ifdef NW_UP
#include "ipx_app.h"
#include "ripx_app.h"
#include "sap_app.h"
#else /* !NW_UP */
#include <sys/ipx_app.h>
#include <sys/ripx_app.h>
#include <sys/sap_app.h>
#endif /* NW_UP */

#include <stdio.h>
#include <string.h>
#include <netconfig.h>
#include <netdir.h>
#include <sys/types.h>
#include <sys/tiuser.h>
#include <nw/nwclient.h>
#include "nwxtypes.h" 
#include <nucinit.h>
#include "nwapi.h"
#include "nucnwapi.h"
#include <nw/nwerror.h>

#ifdef NW_UP
#include "nwmp.h"
#else /* !NW_UP */
#include <sys/nwmp.h>
#endif /* NW_UP */

#include "nucrequester.h"
#include "nwClnt.h"

/*
**  Virtual Connection Table Routines
**
*/

static void connFree( ce_t *connection );
static ce_t connTable[MAX_CONNECTIONS_PER_PROCESS];
static nuint32 connHandle = 0;
static struct staticScanInfo {
	struct netbuf	serverAddress;
	char			buffer[NWMAX_INTERNET_ADDRESS_LENGTH];
	nuint32			x;
} *scanStaticInfo;

static struct staticScanInfo *
getStaticScanStruct()
{

	struct staticScanInfo *s;


#ifdef _REENTRANT

	if(FIRST_OR_NO_THREAD) {
		if( scanStaticInfo == NULL ) {
			scanStaticInfo = (struct staticScanInfo *)calloc( 1, 
				sizeof( struct staticScanInfo ));
			scanStaticInfo->serverAddress.buf = scanStaticInfo->buffer;
		}
		s = scanStaticInfo;
	} else {
		s = (struct staticScanInfo *)_mt_get_thr_specific_storage( 
			_nwClnt_scan_key, (sizeof( struct staticScanInfo )/ sizeof( void *)) );
		s->serverAddress.buf = s->buffer;
	}

#else /* ! _REENTRANT */

	if( scanStaticInfo == NULL ) {
		scanStaticInfo = (struct staticScanInfo *)calloc( 1, 
			sizeof( struct staticScanInfo ));
		scanStaticInfo->serverAddress.buf = scanStaticInfo->buffer;
	}
	s = scanStaticInfo;

#endif /* _REENTRANT */
	return( s );

}

/* Adds a new connection entry into the virtual connection table */
static ce_t *
newConnEntry(
	char *serverName,
	struct netbuf *serverAddress,
	int fd
)
{
	ce_t *e = NULL;
	int i;

	for (i = 0; i < MAX_CONNECTIONS_PER_PROCESS; i++)
	{
		if (connTable[i].ce_connHandle == 0)
		{
			e = &connTable[i];
			break;
		}
	}

	/* We don't have any empty slots so lets make sure they're
		all active 
	*/
	if (e == NULL)
	{
		for( i = 0; i < MAX_CONNECTIONS_PER_PROCESS; i++ ) {
			if( NWMPCheckConnection( connTable[i].ce_fd ) ) {
				connFree( (ce_t *) &connTable[i] );
			}
		}
		for (i = 0; i < MAX_CONNECTIONS_PER_PROCESS; i++)
		{
			if (connTable[i].ce_connHandle == 0)
			{
				e = &connTable[i];
				break;
			}
		}
	}

	if (e == NULL)
	{
		return(NULL);
	}

	if (serverName)
	{
		e->ce_serverName = (char *)strdup(serverName);
		if (e->ce_serverName == NULL)
		{
			return(NULL);
		}
	}
	if (serverAddress)
	{
		e->ce_serverAddress.len = serverAddress->len;
		memcpy(e->ce_buffer, serverAddress->buf, serverAddress->len);
		e->ce_serverAddress.buf = e->ce_buffer;
	}

	e->ce_fd = fd;
	e->ce_connHandle = ++connHandle;
	return(e);
}

static ce_t *
connFindEntryByConnHandle( nuint32 connHandle )
{
	int i;

	if (connHandle == 0)
	{
		return(NULL);
	}

	for (i = 0; i < MAX_CONNECTIONS_PER_PROCESS; i++)
	{
		if (connTable[i].ce_connHandle == connHandle) {
			return (&connTable[i]);
		}
	}

	return(NULL);
}

int
getConnHandleList(
	NWCONN_HANDLE N_FAR *connList,
	nuint	numConns
)
{
	int i,j;

	for (i = 0, j = 0; i < MAX_CONNECTIONS_PER_PROCESS; i++)
	{
		if (connTable[i].ce_connHandle) {
			*connList = connTable[i].ce_connHandle;
			connList++;
			if(++j >= numConns)
				break;
		}
	}
	return(j);
}

ce_t *
connFindAnyConnection()
{
	int i;

#ifdef NEVER
	/* this doesn't work with multiple threads */

	for (i = 0; i < MAX_CONNECTIONS_PER_PROCESS; i++)
	{
		if (connTable[i].ce_connHandle)
			return(&connTable[i]);
	}
#endif
	return(NULL);
}

static void
connFree( ce_t *connection )
{
	int	ce_fd;

	if (connection)
	{
		ce_fd = connection->ce_fd;

		if( connection->ce_serverName )
			free(connection->ce_serverName);
		connection->ce_serverName = NULL;
		connection->ce_fd = 0;
		connection->ce_connHandle = 0;
		NWCMemSet(&(connection->ce_serverAddress), '\0', sizeof(struct netbuf));
		NWCMemSet(connection->ce_buffer, '\0', NWMAX_INTERNET_ADDRESS_LENGTH);
		if( ce_fd )
			NWMPClose(ce_fd);
	}
}

/*
**  Public Requester Functions
**
*/

N_EXTERN_FUNC_C (nint)
initreq( INIT_REQ_T *reqInit, DS_INIT_REQ_T *dsreqInit )
{

	nuint ccode;

	/* Is this the first time through for this process? 
	   I'm not sure that this question needs to be asked here. The
	   connTable is static so it is nulled out for a new process.
	   We'll just resolve the preferred name to an address and let
	   the kernel decide if we require a default connection.
	*/

	/* Allow for structures passed in to be set to NULL */
	ccode = NWMPInitRequester( reqInit, dsreqInit );

	return( ccode );
}

N_EXTERN_FUNC_C (nint)
sys_close_conn( nuint32 connHandle )
{
	ce_t *connection;
	int	 ce_fd;
	nint r;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if ((connection = connFindEntryByConnHandle( connHandle )) == NULL)
	{
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		return(NWE_CONN_INVALID);
	}
	ce_fd = connection->ce_fd;
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	if (r = NWMPCloseServiceTask( ce_fd ))
	{
		return(r);
	}

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connFree( connection );
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	return( SUCCESS );
}

N_EXTERN_FUNC_C (nint)
get_max_conns( pnuint32 puMaxConns, pnuint32 puNumConns )
{
	nuint32 luScanIndex = 0;
	nuint32 luMask;
	nuint32 maxConns;
	nuint uInfoLevel;
	nuint uInfoLen;
	nuint32 luConnectionReference = 0;
	char buffer[100];
	char address[30];
	int count = 0;
	int r;

	pNWCTranAddr p = (pNWCTranAddr)buffer;
	p->pbuBuffer = (pnuint8)address;
	p->uLen = sizeof(address);

	while ((r = scan_conn_info(&luScanIndex, (nuint)NWC_CONN_INFO_RETURN_ALL,
		(pnstr)NULL, (nuint)0, (nuint)0, (nuint)NWC_CONN_INFO_TRAN_ADDR,
		(nuint)100, &luConnectionReference, (pnstr)buffer)) == SUCCESS)
	{
		++count;
	}

	if (r == (NWE_SCAN_COMPLETE) )
	{
		if ((r = NWMPGetMaxConns(&maxConns)) == SUCCESS)
		{
			*puNumConns = count;
			*puMaxConns = maxConns;
			return(SUCCESS);
		}
	}

	return(r);
}


N_EXTERN_FUNC_C (nint)
get_server_context(
	nuint32		connHandle,
	pnuint		majorVersion,
	pnuint		minorVersion
)
{
	ce_t	*connection;
	int	ce_fd;
	nuint	ccode;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connection = connFindEntryByConnHandle(connHandle);
	if (connection)
	{
		ce_fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		ccode = NWMPGetServerContext(ce_fd, majorVersion, minorVersion);

		if (ccode == (NWE_INVALID_TASK_NUMBER))
		{
			MUTEX_LOCK( &_nwClnt_connTable_lock );
			connFree(connection);
			MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		}
		return(ccode);
	}
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	return(NWE_CONN_INVALID);
}

N_EXTERN_FUNC_C (nint)
license_conn( nuint32 connHandle )
{
	ce_t	*connection;
	int     ce_fd;
	nuint32	ccode;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connection = connFindEntryByConnHandle(connHandle);
	if (connection)
	{
		ce_fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		ccode = NWMPLicenseConn(ce_fd, (uint32) 1);

		if (ccode == (NWE_INVALID_TASK_NUMBER))
		{
			MUTEX_LOCK( &_nwClnt_connTable_lock );
			connFree(connection);
			MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		}
		return(ccode);
	}
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	return(NWE_CONN_INVALID);
}

N_EXTERN_FUNC_C (nint)
unlicense_conn( nuint32 connHandle )
{
	ce_t	*connection;
	int     ce_fd;
	nuint32	ccode;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connection = connFindEntryByConnHandle(connHandle);
	if (connection)
	{
		ce_fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		ccode = NWMPLicenseConn(ce_fd, (uint32) 0);

		if (ccode == (NWE_INVALID_TASK_NUMBER))
		{
			MUTEX_LOCK( &_nwClnt_connTable_lock );
			connFree(connection);
			MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		}
		return(ccode);
	}
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	return(NWE_CONN_INVALID);
}

N_EXTERN_FUNC_C (nint)
ncp_request(
	nuint32		connHandle,
	nuint8		function,
	pnuint8		reqBuf,
	nuint		reqLen,
	pnuint8		repBuf,
	pnuint		repLen
)
{
	ce_t	*connection;
	int     ce_fd;
	nuint32	ccode=NWE_CONN_INVALID;
	int32	tmpReplyLen;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if (connection = connFindEntryByConnHandle(connHandle))
	{
		ce_fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );

		if (reqLen < NCP_HEADER_SIZE)
		{
			/*
			 *  Request packet is less than max header length.
			 */

			tmpReplyLen = *repLen;
			ccode = NWMPRawPacket(ce_fd, reqBuf, reqLen,
					0, repBuf, 0, &tmpReplyLen);
			*repLen = tmpReplyLen;
		}
		else
		{
			/*
			 *  Request packet is greater than max header length,
			 *  and reply is less than max header length.
			 */

			char tmp[NCP_HEADER_SIZE];
			uint32 replyHeaderLen = NCP_HEADER_SIZE;

			NWCMemCpy(tmp, reqBuf, NCP_HEADER_SIZE);
			ccode = NWMPRawPacket(ce_fd, tmp, NCP_HEADER_SIZE,
					&replyHeaderLen, reqBuf + NCP_HEADER_SIZE,
					reqLen - NCP_HEADER_SIZE, 0);

			if (ccode == SUCCESS)
			{
				*repLen = replyHeaderLen;
				NWCMemCpy(repBuf, tmp, replyHeaderLen);
			}
		}
		if (ccode == (NWE_INVALID_TASK_NUMBER))
		{
			ccode = NWE_CONN_INVALID;
			MUTEX_LOCK( &_nwClnt_connTable_lock );
			connFree(connection);
			MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		}
	} else {
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}
	return(ccode);
}


N_EXTERN_FUNC_C (nint)
make_conn_permanent( nuint32 connHandle )
{
	ce_t	*connection;
	int		ce_fd;
	nuint32	ccode;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connection = connFindEntryByConnHandle(connHandle);
	if (connection)
	{
		ce_fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		ccode = NWMPMakeConnPermanent(ce_fd);

		if (ccode == (NWE_INVALID_TASK_NUMBER))
		{
			MUTEX_LOCK( &_nwClnt_connTable_lock );
			connFree(connection);
			MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		}
		return(ccode);
	}
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	return(NWE_CONN_INVALID);
}


N_EXTERN_FUNC_C (nint)
get_conn_info(
	nuint32	 connHandle,
	nuint	 uInfoLevel,
	nuint	 uInfoLen,
	pnuint8	 pConnInfo
)
{
	ce_t	*connection;
	int     ce_fd;
	int		r;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connection = connFindEntryByConnHandle(connHandle);
	if (connection)
	{
        ce_fd = connection->ce_fd;
        MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		r = NWMPGetConnInfo(ce_fd, uInfoLevel, uInfoLen, pConnInfo);
		if (r != (NWE_INVALID_TASK_NUMBER) )
		{
			return(r);
		}
		MUTEX_LOCK( &_nwClnt_connTable_lock );
		connFree(connection);
        MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	} else {
    	MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}

	return(NWE_CONN_INVALID);
}

N_EXTERN_FUNC_C (nint)
close_conn( nuint32 connHandle )
{
	ce_t	*connection;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connection = connFindEntryByConnHandle(connHandle);
	if (connection)
	{
		connFree(connection);
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		return(SUCCESS);
	}
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	return(NWE_CONN_INVALID);
}


N_EXTERN_FUNC_C (nint)
set_conn_info(
	nuint32 connHandle,
	nuint   uInfoLevel,
	nuint   uInfoLen,
	pnuint8	pConnInfo
)
{
	ce_t	*connection;
    int     ce_fd;
	nuint32	ccode;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connection = connFindEntryByConnHandle(connHandle);
	if (connection)
	{
        ce_fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		ccode = NWMPSetConnInfo(ce_fd, uInfoLevel, uInfoLen, pConnInfo);

		if (ccode == (NWE_INVALID_TASK_NUMBER))
		{
			MUTEX_LOCK( &_nwClnt_connTable_lock );
			connFree(connection);
			MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		}
		return(ccode);
	}
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	return(NWE_CONN_INVALID);
}

N_EXTERN_FUNC_C (nint)
scan_conn_info( 
	pnuint32	pluScanIndex,
	nuint  		uScanInfoLevel,
	pnstr		pScanConnInfo,
	nuint  		uScanInfoLen,
	nuint  		uScanFlags,
	nuint  		uInfoLevel,
	nuint  		uInfoLen,
	pnuint32	pluConnectionReference,
	pnstr		pConnInfo
)
{
	ce_t *connection;

	struct staticScanInfo *staticScanInfo;

	nint fd, r, j1, j2;
	nuint16 connectionNumber;


	if((staticScanInfo = getStaticScanStruct()) == NULL ) {
		return( NWE_OUT_OF_HEAP_SPACE );
	}

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if (connection = connFindAnyConnection())
	{
		fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}
	else
	{
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		if ((fd = NWMPOpen()) < 0)
		{
			return( NWE_INVALID_SERVICE_REQUEST );
		}
	}

	if (*pluScanIndex == 0)
	{
		staticScanInfo->serverAddress.len = 0;
		staticScanInfo->serverAddress.maxlen = MAX_ADDRESS_SIZE;
		if ((r = NWMPScanServices(fd, &staticScanInfo->serverAddress, 
			&j1, &j2)) == SUCCESS)
		{
			staticScanInfo->x = 0;
		}
		else {
			r = NWE_SCAN_COMPLETE ;
			goto out;
		}
	}

	while (1)
	{
		r = NWMPScanConnInfo(fd, &staticScanInfo->serverAddress, 
			uScanInfoLevel, pScanConnInfo, uScanInfoLen, uScanFlags, 
			uInfoLevel, uInfoLen, &staticScanInfo->x, pConnInfo);
		if (r == (NWE_SCAN_COMPLETE) )
		{
			r = NWMPScanServices(fd, &staticScanInfo->serverAddress, &j1, &j2);
			if(r == (NWE_SCAN_COMPLETE) )
			{
				*pluScanIndex = NWC_CONN_INFO_RETURN_ALL;
				r = NWE_SCAN_COMPLETE;
				goto out;
			}
			else
			{
				staticScanInfo->x = 0;
			}
		}
		else if (r == (NWE_REQUESTER_FAILURE) )
		{
			goto out;
		}
		else
		{
			*pluScanIndex = staticScanInfo->x;
			*pluConnectionReference = staticScanInfo->x;
			r = SUCCESS;
			goto out;
		}
	}

out:
	if (connection == (ce_t *)NULL)
		NWMPClose(fd);

	return(r);
}


N_EXTERN_FUNC_C (nint)
open_conn_by_reference(
	nuint32		reference,
	pnuint32	pluConnHandle,
	nuint32		flags
)
{
	nuint32 luScanIndex = 0;
	nuint32 luMask;
	nuint uInfoLevel;
	nuint uInfoLen;
	nuint32 luConnectionReference = 0;
	char buffer[100];
	char address[30];
	nint r;

	pNWCTranAddr p = (pNWCTranAddr)buffer;
	p->pbuBuffer = (pnuint8)address;
	p->uLen = sizeof(address);
	p->uType = NWC_TRAN_TYPE_IPX;

	flags &= ~(NWC_OPEN_PUBLIC | NWC_OPEN_PRIVATE);
	if ((r = scan_conn_info(&luScanIndex, (nuint)NWC_CONN_INFO_CONN_REF,
		(pnstr)&reference, (nuint)sizeof(nuint32),
		(nuint)(NWC_MATCH_EQUALS | NWC_RETURN_PUBLIC),
		(nuint)NWC_CONN_INFO_TRAN_ADDR, (nuint)100, &luConnectionReference, 
		(pnstr)buffer)) == SUCCESS)
	{
		r = open_conn_by_addr( p, pluConnHandle, flags | NWC_OPEN_PUBLIC );
	}
	else {
		if ((r = scan_conn_info(&luScanIndex, (nuint)NWC_CONN_INFO_CONN_REF,
			(pnstr)&reference, (nuint)sizeof(nuint32),
			(nuint)(NWC_MATCH_EQUALS | NWC_RETURN_PRIVATE),
			(nuint)NWC_CONN_INFO_TRAN_ADDR, (nuint)100, &luConnectionReference, 
			(pnstr)buffer)) == SUCCESS)
		{
			r = open_conn_by_addr( p, pluConnHandle, flags | NWC_OPEN_PRIVATE );
		}
	}
	return(r);
}


N_EXTERN_FUNC_C (nint)
open_conn_by_addr(
	pNWCTranAddr	pTranAddr,
	pnuint32	pluConnHandle,
	nuint32		flags
)
{
	ce_t 	*connection;
	int		ce_fd;
	nint 	fd, ccode;

	struct netbuf serverAddress;

	if( !(pTranAddr->uType & NWC_TRAN_TYPE_WILD) &&
	    (pTranAddr->uType != NWC_TRAN_TYPE_IPX))
	{
		return( NWE_PARAMETERS_INVALID );
	}

	serverAddress.len = pTranAddr->uLen;
	serverAddress.buf = (char *)pTranAddr->pbuBuffer;

	if ((fd = NWMPOpen()) == -1)
	{
		return(NWE_NO_RESPONSE_FROM_SERVER);
	}

	ccode = NWMPOpenServiceTask( fd, &serverAddress, flags );
	switch (ccode)
	{

		case (NWE_EA_SCAN_DONE):
#ifdef PROVO_DO_THEY_WANT_THIS
			ccode = NWE_ALREADY_ATTACHED;
#else
			ccode = SUCCESS;
#endif
		case SUCCESS:
		{
			if (NWMPRegisterRaw(fd, &serverAddress, flags))
			{
				NWMPClose(fd);
				return(NWE_NO_RESPONSE_FROM_SERVER);
			}
			MUTEX_LOCK( &_nwClnt_connTable_lock );
			if ((connection = newConnEntry(NULL, &serverAddress, fd)) == NULL)
			{
				MUTEX_UNLOCK( &_nwClnt_connTable_lock );
				NWMPClose(fd);
				return(NWE_NO_FREE_CONN_SLOTS);
			}
			*pluConnHandle = connection->ce_connHandle;
			MUTEX_UNLOCK( &_nwClnt_connTable_lock );
			break;
		}

		default:
		{
			NWMPClose(fd);
			if(ccode == CONNECTION_TABLE_FULL)
				return(ccode);
			else
				return(NWE_NO_RESPONSE_FROM_SERVER);
		}
	}
	
	return( ccode );
}


N_EXTERN_FUNC_C (nint)
open_conn_by_name(
	char		*pName,
	nuint		uTranType,
	pnuint32	pluConnHandle,
	nuint32		flags
)
{
	ce_t *connection;
	nint fd, ccode;

	struct netconfig *getnetconfigent();
	struct netconfig *np;
	struct nd_hostserv hs;
	struct nd_addrlist *addrs;
	struct netbuf *serverAddress;

	if( !(uTranType & NWC_TRAN_TYPE_WILD) &&
	    (uTranType != NWC_TRAN_TYPE_IPX))
	{
		return( NWE_PARAMETERS_INVALID );
	}

	if ((fd = NWMPOpen()) == -1)
	{
		return(NWE_NO_RESPONSE_FROM_SERVER);
	}

	errno = 0;
	if ((np = getnetconfigent("ipx")) == NULL)
	{
		NWMPClose(fd);
		return(NWE_NO_RESPONSE_FROM_SERVER);
	}

	hs.h_host = pName;
	hs.h_serv = "1105";

	if (netdir_getbyname(np, &hs, &addrs))
	{
		freenetconfigent(np);
		NWMPClose(fd);
		return(NWE_NO_RESPONSE_FROM_SERVER);
	}

	if (addrs->n_cnt == 0)
	{
		freenetconfigent(np);
		netdir_free((char *)addrs, ND_ADDRLIST);
		NWMPClose(fd);
		return(NWE_NO_RESPONSE_FROM_SERVER);
	}
	serverAddress = addrs->n_addrs;
	freenetconfigent(np);

	ccode = NWMPOpenServiceTask( fd, serverAddress, flags );
	switch (ccode)
	{

		case (NWE_EA_SCAN_DONE):
#ifdef PROVO_DO_THEY_WANT_THIS
			ccode = NWE_ALREADY_ATTACHED;
#else
			ccode = SUCCESS;
#endif
		case SUCCESS:
		{

			if (NWMPRegisterRaw(fd, serverAddress, flags))
			{
				netdir_free((char *)addrs, ND_ADDRLIST);
				NWMPClose(fd);
				return(NWE_NO_RESPONSE_FROM_SERVER);
			}
			MUTEX_LOCK( &_nwClnt_connTable_lock );
			if ((connection = newConnEntry(pName, serverAddress, fd)) == NULL)
			{
				MUTEX_UNLOCK( &_nwClnt_connTable_lock );
				netdir_free((char *)addrs, ND_ADDRLIST);
				NWMPClose(fd);
				return(NWE_NO_FREE_CONN_SLOTS);
			}
			*pluConnHandle = connection->ce_connHandle;
			MUTEX_UNLOCK( &_nwClnt_connTable_lock );
			break;
		}

		default:
		{
			netdir_free((char *)addrs, ND_ADDRLIST);
			NWMPClose(fd);
			if(ccode == CONNECTION_TABLE_FULL)
				return(ccode);
			else
				return(NWE_NO_RESPONSE_FROM_SERVER);
		}
	}
	
	netdir_free((char *)addrs, ND_ADDRLIST);

	return(ccode);
}

int
authenticate_task(
	nuint32	connHandle,
	pnuint8	buffer,
	nuint32	objectID, 
	nuint	authType
)
{
	ce_t	*connection;
	int	ce_fd;
	nuint32	ccode;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if( (connection = connFindEntryByConnHandle( connHandle )) == NULL ) {
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		return( NWE_CONN_INVALID );
	}
	ce_fd = connection->ce_fd;
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	if( authType == NWC_AUTH_STATE_BINDERY ) {
    	ccode = NWMPAuthenticateTask( ce_fd, 
			objectID, authType, buffer, AUTH_KEY_LENGTH );
	} else {
    	ccode = NWMPAuthenticateTask( ce_fd, 
			objectID, authType, buffer, SESSION_KEY_LENGTH );
	}

	if( ccode == ( NWE_INVALID_TASK_NUMBER ) ) {
		MUTEX_LOCK( &_nwClnt_connTable_lock );
		connFree( connection );
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}

    return( ccode );

}

int 
set_monitored_conn( nuint32 connHandle )
{
	ce_t		*connection;
	int		ce_fd;
	nuint32		ccode;
	nuint32		connRef;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if( (connection = connFindEntryByConnHandle( connHandle )) == NULL ) {
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		return( NWE_CONN_INVALID );
	}
	ce_fd = connection->ce_fd;
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	ccode = NWMPSetMonitoredConn( ce_fd );

	if( ccode == ( NWE_INVALID_TASK_NUMBER ) ) {
		MUTEX_LOCK( &_nwClnt_connTable_lock );
		connFree( connection );
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}

	return( ccode );

}

int
get_security_levels(
	nuint32 *defaultLevel,
	nuint32 *baseLevel, 
	nuint32 *userLevel
)
{
	ce_t	*connection;
	int	fd;
	nint	r;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if (connection = connFindAnyConnection())
	{
		fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}
	else
	{
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		if ((fd = NWMPOpen()) < 0)
		{
			return( NWE_INVALID_SERVICE_REQUEST );
		}
	}

	r = NWMPGetSecurityFlags( fd, defaultLevel, baseLevel, userLevel );

	if (connection == (ce_t *)NULL)
		NWMPClose(fd);

	return( r );


}

int
get_checksum_levels(
	nuint32 *defaultLevel,
	nuint32 *baseLevel, 
	nuint32 *userLevel
)
{
	ce_t	*connection;
	int		fd;
	nint	r;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if (connection = connFindAnyConnection())
	{
		fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}
	else
	{
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		if ((fd = NWMPOpen()) < 0)
		{
			return( NWE_INVALID_SERVICE_REQUEST );
		}
	}

	r = NWMPGetChecksumFlags( fd, defaultLevel, baseLevel, userLevel );

	if (connection == (ce_t *)NULL)
		NWMPClose(fd);

	return( r );


}

N_EXTERN_FUNC_C (nint)
make_signature_decision( nuint32 connHandle )
{
	ce_t *connection;
	int	 ce_fd;
	nint r;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if ((connection = connFindEntryByConnHandle( connHandle )) == NULL)
	{
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		return(NWE_CONN_INVALID);
	}
	ce_fd = connection->ce_fd;
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	r = NWMPMakeSignatureDecision( ce_fd );

	return( r );
}

N_EXTERN_FUNC_C (nint)
get_primary_conn_reference( pnuint32 pluConnectionReference )
{
	ce_t	*connection;

	nint	fd, r;
	nuint16	connectionNumber;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if (connection = connFindAnyConnection())
	{
		fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}
	else
	{
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		if ((fd = NWMPOpen()) < 0)
		{
			return( NWE_INVALID_SERVICE_REQUEST );
		}
	}

	r = NWMPGetPrimaryService(fd, pluConnectionReference);

	if (connection == (ce_t *)NULL)
	{
		NWMPClose(fd);
	}
	return(r);
}

int 
set_primary_conn( nuint32 connHandle )
{
	ce_t	*connection;
	int		ce_fd;
	nuint32	ccode;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if ((connection = connFindEntryByConnHandle( connHandle )) == NULL)
	{
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		return( NWE_CONN_INVALID );
	}
	ce_fd = connection->ce_fd;
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	ccode = NWMPSetPrimaryService(ce_fd);

	if (ccode == ( NWE_INVALID_TASK_NUMBER ))
	{
		MUTEX_LOCK( &_nwClnt_connTable_lock );
		connFree(connection);
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	}
	return(ccode);

}

/*
**  Open connection to the first server listed
**  in the sap table with a hop count of 1.
**  If the open fails then try the
**  second server and so on.
*/
open_conn_using_sap_file( pnuint32 reference )
{
	SAPI		serverEntry;
	uint32		serverIndex = 0;
	int		status;
	NWCTranAddr	tranAddr;
	nuint32		connHandle;

	while( (status = SAPGetAllServers((uint16)0x4, (int *) &serverIndex, &serverEntry, 1)) > 0)
	{
		if (serverEntry.serverHops == 1) {

			uint8 address[12];

			address[0] = serverEntry.serverAddress.net[0];
			address[1] = serverEntry.serverAddress.net[1];
			address[2] = serverEntry.serverAddress.net[2];
			address[3] = serverEntry.serverAddress.net[3];

			address[4] = serverEntry.serverAddress.node[0];
			address[5] = serverEntry.serverAddress.node[1];
			address[6] = serverEntry.serverAddress.node[2];
			address[7] = serverEntry.serverAddress.node[3];
			address[8] = serverEntry.serverAddress.node[4];
			address[9] = serverEntry.serverAddress.node[5];

			address[10] = serverEntry.serverAddress.sock[0];
			address[11] = serverEntry.serverAddress.sock[1];

			tranAddr.uType = NWC_TRAN_TYPE_IPX;
			tranAddr.uLen = 12;
			tranAddr.pbuBuffer = (pnuint8)address;

			if (open_conn_by_addr(&tranAddr, &connHandle, NWC_OPEN_PUBLIC) == 0)
			{
				close_conn(connHandle);
				break;
			}
		}
	}

	if ( status < 0)
	{
		return(-1);
	}

	if (get_primary_conn_reference(reference) == SUCCESS)
	{
		return(SUCCESS);
	}
	return(-1);
}

N_EXTERN_FUNC_C (nint)
sys_close_conn_with_force( nuint32 connHandle )
{
	ce_t *connection;
	int	 ce_fd;
	nint r;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if ((connection = connFindEntryByConnHandle( connHandle )) == NULL)
	{
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		return(NWE_CONN_INVALID);
	}
	ce_fd = connection->ce_fd;
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	if (r = NWMPCloseTaskWithForce( ce_fd ))
	{
		return(r);
	}

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	connFree( connection );
	MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	return( SUCCESS );
}
