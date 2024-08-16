/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:nwclient.c	1.8"
#include <stdio.h>
#include <nw/nwclient.h>
#include <nw/nwcaldef.h>
#include "nwaccess.h"

#include "nwapidef.h"
#include "nwxtypes.h"
#include <sys/nwtdr.h>
#include "tdr.h"

#include <nw/nwerror.h>
#include "nwerrors.h"
#include "ncpcodes.h"
#include "nwapi.h"

#include <sys/tiuser.h>
#include "nucinit.h"
#include "nucnwapi.h"

N_INTERN_FUNC_C	(nuint32) BadCheckFlags( nuint );


/*
**	Local Inititalization/Termination functions
*/

/********************************************************************
	BadCheckFlags: Verify sanity of connection flags
********************************************************************/
/* ARGSUSED */
N_INTERN_FUNC_C( nuint32 )
BadCheckFlags( nuint connFlags )
{
	if((connFlags & (NWC_OPEN_LICENSED | NWC_OPEN_UNLICENSED)) ==
							(NWC_OPEN_LICENSED | NWC_OPEN_UNLICENSED))
		return( TRUE );
	if((connFlags & (NWC_OPEN_PRIVATE | NWC_OPEN_PUBLIC)) ==
							(NWC_OPEN_PRIVATE | NWC_OPEN_PUBLIC))
		return( TRUE );
	if(!(connFlags & (NWC_OPEN_LICENSED | NWC_OPEN_UNLICENSED)))
		return( TRUE );
	if(!(connFlags & (NWC_OPEN_PRIVATE | NWC_OPEN_PUBLIC)))
		return( TRUE );

	return(FALSE);
}


/********************************************************************
	NWClientInit: Initialize requesters
********************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWClientInit( pNWAccess pAccess )
{
	NWRCODE		rCode;

	rCode = initreq( NULL, NULL );

	return( rCode );
}


/********************************************************************
	NWClientTerm: Terminate requesters
********************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWClientTerm( pNWAccess pAccess )
{
	return( 0 );
}


/*
	NCP Transmission Functions
*/
/********************************************************************
	NWCRequestSingle: Single Request Fragment NCP
********************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCRequestSingle                 /* NCPs which don't need fragments */
(
	pNWAccess	pAccess,
	nuint		uFunction,
	nptr		pReq,
	nuint		uReqLen,
	nptr		pReply,
	nuint		uReplyLen,
	pnuint		puActualReplyLen
)
{
	nuint32		ccode;
	pnuint8		packetPtr;
	NWU_FRAGMENT	reqFrag, replyFrag;


	if (NWGetTDRBuffers( &reqFrag.fragAddress, &replyFrag.fragAddress ) == -1)
		return(NWE_OUT_OF_HEAP_SPACE);

	NWBuildRequestHeader( reqFrag.fragAddress, NCP_REQUEST_RT, uFunction );

	NW_SETUP_REQUEST( reqFrag.fragAddress + NW_NCP_HEADER_SIZE );

	COPY_BYTES_FROM( pReq, uReqLen );

	NW_GET_FRAG_SIZE( reqFrag.fragSize, reqFrag.fragAddress );
	replyFrag.fragSize = MAXIMUM_PACKET_SIZE;

	ccode = NWNCPRequest( NWCGetConnP(pAccess), &reqFrag, &replyFrag );

	if(!ccode) {
		NW_SETUP_REPLY( replyFrag.fragAddress );
		COPY_BYTES_TO( pReply, uReplyLen, uReplyLen );
	}
	if(puActualReplyLen)
		*puActualReplyLen = replyFrag.fragSize;

	return(ccode);
}


/********************************************************************
	NWCRequestSimple: Single Request Fragment NCP
********************************************************************/
N_EXTERN_LIBRARY( NWRCODE )
NWCRequestSimple                 /* NCPs with only subfunction request */
(
   pNWAccess pAccess,
   nuint    uFunction,
   nuint16  suNCPLen,
   nuint8   buSubfunction
)
{
	nuint32		ccode;
	pnuint8		packetPtr;
	NWU_FRAGMENT	reqFrag, replyFrag;


	if (NWGetTDRBuffers( &reqFrag.fragAddress, &replyFrag.fragAddress ) == -1)
		return(NWE_OUT_OF_HEAP_SPACE);

	NWBuildRequestHeader( reqFrag.fragAddress, NCP_REQUEST_RT, uFunction );

	NW_SETUP_REQUEST( reqFrag.fragAddress + NW_NCP_HEADER_SIZE );

	if (suNCPLen) 
		HI_LO_UINT16_FROM( suNCPLen );
	COPY_UINT8_FROM( buSubfunction );
	NW_GET_FRAG_SIZE( reqFrag.fragSize, reqFrag.fragAddress );
	replyFrag.fragSize = MAXIMUM_PACKET_SIZE;

	ccode = NWNCPRequest( NWCGetConnP(pAccess), &reqFrag, &replyFrag );

	return(ccode);
}


/********************************************************************
	NWCRequest: Multiple Request Fragment NCP
********************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCRequest                 /* NCPs which don't need fragments */
(
	pNWAccess	pAccess,
	nuint		uFunction,
	nuint		uNumReqFrags,
	pNWCFrag	pReqFrags,
	nuint		uNumReplyFrags,
	pNWCFrag	pReplyFrags,
	pnuint		puActualReplyLen
)
{
#ifdef REQ_DEBUG
	nuint	j;
#endif
	nuint			i, numBytes=0;
	nuint32			ccode;
	pnuint8			packetPtr;
	NWU_FRAGMENT	reqFrag, replyFrag;
	char			*tmpPtr;


	if (NWGetTDRBuffers( &reqFrag.fragAddress, &replyFrag.fragAddress ) == -1)
		return(NWE_OUT_OF_HEAP_SPACE);

	NWBuildRequestHeader( reqFrag.fragAddress, NCP_REQUEST_RT, uFunction );

	NW_SETUP_REQUEST( reqFrag.fragAddress + NW_NCP_HEADER_SIZE );

	for(i=0; i<uNumReqFrags; i++)
	{
		numBytes += pReqFrags[i].uLen;
	}

	if( numBytes > MAXIMUM_PACKET_SIZE )
		return(NWE_PARAMETERS_INVALID);

	for(i=0; i<uNumReqFrags; i++)
	{
		COPY_BYTES_FROM( pReqFrags[i].pAddr, pReqFrags[i].uLen );
	}

	NW_GET_FRAG_SIZE( reqFrag.fragSize, reqFrag.fragAddress );
	replyFrag.fragSize = MAXIMUM_PACKET_SIZE;

	ccode = NWNCPRequest( NWCGetConnP(pAccess), &reqFrag, &replyFrag );

	if(!ccode && uNumReplyFrags)
	{
		NW_SETUP_REPLY( replyFrag.fragAddress );
		for(i=0; i<uNumReplyFrags; i++)
		{
			COPY_BYTES_TO( pReplyFrags[i].pAddr, pReplyFrags[i].uLen,
				pReplyFrags[i].uLen );

			tmpPtr = pReplyFrags[i].pAddr;
#ifdef REQ_DEBUG
			printf("replyFrags = ");
			for(j=0; j<pReplyFrags[i].uLen; j++)
				printf("%x ", tmpPtr[j]);
			printf("\n");
#endif
		}
	}

	if(puActualReplyLen)
		*puActualReplyLen = replyFrag.fragSize;

	return(ccode);
}


/*********************************************
	Connection interface functions
*********************************************/


/*******************************************************************
	NWCOpenConnByAddr
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCOpenConnByAddr(
	pNWAccess		pAccess,
	pnstr			pstrServiceType,
	nuint			uConnFlags,
	pNWCTranAddr	pTranAddr
)
{
	nuint32			cCode;
	nuint32			connHandle;

	if(BadCheckFlags(uConnFlags))
		return(NWE_PARAMETERS_INVALID);

	cCode = (nuint32)open_conn_by_addr(pTranAddr, &connHandle, 
									(nuint32)uConnFlags);

	NWCSetConnP( pAccess, connHandle );

	return( cCode );
}

	
/*******************************************************************
	NWCOpenConnByName
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCOpenConnByName(
	pNWAccess		pAccess,
	nuint32			luConnHandle,
	pNWCConnString	pName,
	pnstr			pstrServiceType,
	nuint			uConnFlags,
	nuint			uTranType
)
{
	nuint32			cCode;
	nuint32			connHandle;
	
	if(BadCheckFlags(uConnFlags))
		return(NWE_PARAMETERS_INVALID);

	cCode = (nuint32)open_conn_by_name((char *)pName->pString, uTranType,
										&connHandle, (nuint32)uConnFlags);

	NWCSetConnP( pAccess, connHandle );

	return( cCode );
}

/*******************************************************************
	NWCOpenConnByReference
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCOpenConnByReference(
	pNWAccess	pAccess,
	nuint32		luConnectionReference,
	nuint		uConnFlags
)
{
	nuint32		cCode;
	nuint32		connHandle;

	if(BadCheckFlags(uConnFlags))
		return(NWE_PARAMETERS_INVALID);

	cCode = (nuint32)open_conn_by_reference( luConnectionReference,
										&connHandle, (nuint32)uConnFlags );

	NWCSetConnP( pAccess, connHandle );

	return( cCode );
}


/*******************************************************************
	NWCCloseConn
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCCloseConn(
	pNWAccess	pAccess
)
{
	nuint32			cCode;

	cCode = (nuint32)close_conn((nuint32)NWCGetConnP(pAccess));

	return( cCode );
}

/*******************************************************************
	NWCSysCloseConn
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCSysCloseConn(
	pNWAccess	pAccess
)
{
	NWRCODE		cCode;

	cCode = (nuint32)sys_close_conn((nuint32)NWCGetConnP(pAccess));

	return( cCode );
}


/*******************************************************************
	NWCMakeConnPermanent
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCMakeConnPermanent(
	pNWAccess	pAccess
)
{
	NWRCODE			cCode;

	cCode = make_conn_permanent((nuint32)NWCGetConnP(pAccess));

	return( cCode );
}
	
/*******************************************************************
	NWCLicenseConn
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCLicenseConn(
	pNWAccess	pAccess
)
{
	NWRCODE			cCode;

	cCode = (NWRCODE)license_conn((nuint32)NWCGetConnP(pAccess));

	return( cCode );

}


/*******************************************************************
	NWCUnlicenseConn
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCUnlicenseConn(
	pNWAccess	pAccess
)
{
	NWRCODE			cCode;

	cCode = (NWRCODE)unlicense_conn((nuint32)NWCGetConnP(pAccess));

	return( cCode );
}


/*******************************************************************
	NWCGetConnInfo
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetConnInfo(
	pNWAccess		pAccess,
	nuint			uInfoLevel,
	nuint			uInfoLen,
	nptr			pConnInfo
)
{
	nuint32		cCode;

	cCode = (nuint32)get_conn_info( (nuint32)NWCGetConnP(pAccess),
									(nuint16)uInfoLevel, (nuint16)uInfoLen,
									(pnuint8)pConnInfo );

	return( cCode );
}


/******************************************************************
	NWCScanConnInfo
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCScanConnInfo(
	pNWAccess		pAccess,
	pnuint32		pluScanIndex,
	nuint			uScanInfoLevel,
	nuint			uScanInfoLen,
	nptr			pScanConnInfo,
	nuint			uScanFlags,
	nuint			uReturnInfoLevel,
	nuint			uReturnInfoLen,
	pnuint32		pluConnectionReference,
	nptr			pReturnConnInfo
)
{
	nuint32		cCode;

	switch( uScanInfoLevel ) {
		case NWC_CONN_INFO_TREE_NAME:
		case NWC_CONN_INFO_SERVER_NAME:
		case NWC_CONN_INFO_SERVICE_TYPE:
		{
			uScanInfoLen = NWCStrLen(pScanConnInfo);
			break;
		}

		case NWC_CONN_INFO_CONN_NUMBER:
		{
			uScanInfoLen = sizeof( uint16 );
			break;
		}

		case NWC_CONN_INFO_TRAN_ADDR:
		{
			uScanInfoLen = sizeof( struct netbuf );
			break;
		}

		case NWC_CONN_INFO_RETURN_ALL:
		case NWC_CONN_INFO_AUTH_STATE:
		case NWC_CONN_INFO_BCAST_STATE:
		case NWC_CONN_INFO_CONN_REF:
		case NWC_CONN_INFO_SECURITY_STATE:
		case NWC_CONN_INFO_USER_ID:
		case NWC_CONN_INFO_NDS_STATE:
		case NWC_CONN_INFO_MAX_PACKET_SIZE:
		case NWC_CONN_INFO_LICENSE_STATE:
		case NWC_CONN_INFO_PUBLIC_STATE:
		case NWC_CONN_INFO_DISTANCE:
		{
			uScanInfoLen = sizeof( uint32 );
            break;
		}
		
		case NWC_CONN_INFO_INFO_VERSION:
		case NWC_CONN_INFO_WORKGROUP_ID:
		default:
		{
			*pluScanIndex = 0xFFFF;
			return( NWE_PARAMETERS_INVALID );
		}
	}

	cCode = (nuint32)scan_conn_info( pluScanIndex, (nuint16)uScanInfoLevel,
							pScanConnInfo, uScanInfoLen, (nuint16)uScanFlags,
							(nuint16)uReturnInfoLevel,
							(nuint16)uReturnInfoLen, pluConnectionReference,
							pReturnConnInfo );

	return( cCode );
}


/*******************************************************************
	NWCSetConnInfo
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCSetConnInfo(
	pNWAccess	pAccess,
	nuint		uInfoLevel,
	nuint		uInfoLength,
	nptr		pConnInfo
)
{
	nuint32		cCode;

	cCode = (nuint32)set_conn_info((nuint32)NWCGetConnP(pAccess), uInfoLevel,
									uInfoLength, (pnuint8)pConnInfo);

	return( cCode );
}

/* Authentication calls */
/*******************************************************************
	NWCAuthenticateBind
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCAuthenticateBind(
	pNWAccess	pAccess,
	pnstr		pstrObjectName,
	nuint16		uObjType,
	pnstr		pstrPassword
)
{
	nuint32			ccode;
	
#ifdef REQ_DEBUG
	printf("NWCAuthenticateBind\n");
#endif
	
/*
**	Send Service Request
*/
	ccode = _NWLoginToFileServer( pAccess, pstrObjectName,
									uObjType, pstrPassword );	
	
	return( ccode );
}


/*******************************************************************
	NWCUnauthenticateConnection
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCUnauthenticateConnection(
	pNWAccess	pAccess
)
{
	NWRCODE	rCode;
	nuint	authState;


/*
**	Get conn. Info to determine connection type
*/
	rCode = NWCGetConnInfo( pAccess, NWC_CONN_INFO_AUTH_STATE,
							sizeof(nuint32), (nptr)&authState);

	if( !rCode ) {

		if(authState == NWC_AUTH_STATE_BINDERY) {
			rCode = NWNCP25Logout(pAccess);
			if( !rCode ) {
				/* Now set the conn. auth. state */
				authState = NWC_AUTH_STATE_NONE;
				rCode = NWCSetConnInfo( pAccess, NWC_CONN_INFO_AUTH_STATE,
								(nuint)sizeof(nuint32), (nptr)&authState);
			}
		} 
	}

	return( rCode );

}


/*****************************************************************
	Tagged Data Store APIs

	NWCAllocTds
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCAllocTds(
	pNWAccess	pAccess,
	nuint		uTag,
	nuint		uTdsSize,
	nuint		uTdsFlags
)
{
	NWRCODE	rcode;

	if(uTdsSize >= 64*1024)
		return( NWE_BUFFER_INVALID_LEN );

	rcode = NWMPAllocTDS( uTag, uTdsSize, uTdsFlags );	

	return(rcode);
}
	

/*****************************************************************
	NWCGetTdsInfo
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetTdsInfo(
	pNWAccess	pAccess,
	nuint		uTag,
	pnuint		puMaxSize,
	pnuint		puDataSize,
	pnuint		puTdsFlags
)
{
	NWRCODE	rcode;

	rcode = NWMPGetTDS( uTag, puMaxSize, puDataSize, puTdsFlags );	

	return(rcode);
}

/*****************************************************************
	NWCFreeTds
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCFreeTds(
	pNWAccess	pAccess,
	nuint		uTag
)
{
	NWRCODE	rcode;

	rcode = NWMPFreeTDS( uTag );

	return( rcode );
}


/*****************************************************************
	NWCReadTds
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCReadTds(
	pNWAccess	pAccess,
	nuint		uTag,
	nuint		uBytesToRead,
	nuint		uReadOffset,
	pnchar		pchBuffer,
	pnuint		puBytesRead
)
{
	NWRCODE	rcode;

	rcode = NWMPReadTDS( uTag, uBytesToRead, uReadOffset, pchBuffer,
							puBytesRead );

	return( rcode );
}


/*****************************************************************
	NWCWriteTds
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCWriteTds(
	pNWAccess	pAccess,
	nuint		uTag,
	nuint		uBytesToWrite,
	nuint		uWriteOffset,
	pnchar		pchBuffer,
	pnuint		puBytesWritten
)
{
	NWRCODE	rcode;

	rcode = NWMPWriteTDS( uTag, uBytesToWrite, uWriteOffset, pchBuffer,
							puBytesWritten );

	return( rcode );
}


/*****************************************************************
	NWCGetDefaultNameContext
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetDefNameContext(
	pNWAccess	pAccess,
	pnuint		puNameLen,
	pnstr16		pstrNameContext
)
{
	NWRCODE	rcode;

	rcode = NWMPReadDN( puNameLen, (pnstr)pstrNameContext );

	return( rcode );
}


/*****************************************************************
	NWCSetDefaultNameContext
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCSetDefNameContext(
	pNWAccess	pAccess,
	nuint		uNameLen,
	pnstr16		pstrNameContext
)
{
	NWRCODE	rcode;

	rcode = NWMPWriteDN( uNameLen, (pnstr)pstrNameContext );

	return( rcode );
}


/*****************************************************************
	NWCGetPreferredDsTree
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetPrefDsTree(
	pNWAccess	pAccess,
	pnuint		puNameLen,
	pnstr		pstrDsTreeName
)
{
	NWRCODE	rcode;

	rcode = NWMPGetPreferredTree( puNameLen, pstrDsTreeName );

	return( rcode );
}


/*****************************************************************
	NWCSetPreferredDsTree
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCSetPrefDsTree(
	pNWAccess	pAccess,
	nuint		uNameLen,
	pnstr		pstrDsTreeName
)
{
	NWRCODE	rcode;

	rcode = NWMPSetPreferredTree( uNameLen, pstrDsTreeName );

	return( rcode );
}


/*******************************************************************
	NWCGetMonitoredConnReference
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetMonitoredConnReference(
	pNWAccess	pAccess,
	pnuint32	pluConnReference
)
{
	nuint32		cCode;

	cCode = NWMPGetMonitoredConn( pluConnReference );

	return(cCode);
}


/*******************************************************************
	NWCSetMonitoredConn
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCSetMonitoredConn(
	pNWAccess	pAccess
)
{
	nuint32		cCode;

	cCode = set_monitored_conn( NWCGetConnP(pAccess) );

	return(cCode);
}


/*******************************************************************
	NWCGetNumConnections
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetNumConns(
	pNWAccess	pAccess,
	pnuint		puMaxConns,
	pnuint		puPublicConns,
	pnuint		puMyPrivateConns,
	pnuint		puOtherPrivateConns
)
{
	nuint32		cCode;

	nuint32	uCurrentConns;
	nuint32	uMaxConns;

	cCode = (nuint32)get_max_conns( &uMaxConns, &uCurrentConns);
	if(cCode)
		return(cCode);

	if (puMaxConns)
		*puMaxConns = -1;
	if (puPublicConns) 
		*puPublicConns = uCurrentConns;
	if (puMyPrivateConns) 
		*puMyPrivateConns = 0;	
	if (puOtherPrivateConns)
		*puOtherPrivateConns = 0;

	return( cCode );
}


/*******************************************************************
	NWCGetRequesterVersion
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetRequesterVersion(
	pNWAccess	pAccess,
	pnuint		puMajorVersion,
	pnuint		puMinorVersion,
	pnuint		puRevision
)
{
	nuint32		cCode;

	cCode = (nuint32)NWMPGetRequesterVersion( puMajorVersion,
									puMinorVersion, puRevision );

	return( cCode );
}


/*******************************************************************
	NWCGetPrimConnRef
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetPrimConnRef(
	pNWAccess	pAccess,
	pnuint32	pluConnRef
)
{
	NWRCODE	rcode;

	rcode = get_primary_conn_reference(pluConnRef);

	return( rcode );
}


/*******************************************************************
	NWCSetPrimConn
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCSetPrimConn(
	pNWAccess	pAccess
)
{
	nuint32		cCode;

	cCode = set_primary_conn((nuint32)NWCGetConnP(pAccess));

	return( cCode );
}


/*******************************************************************
	NWCGetServerVersion
*******************************************************************/
/* ARGSUSED */
N_EXTERN_LIBRARY( NWRCODE )
NWCGetServerVersion(
	pNWAccess	pAccess,
	pnuint		puMajorVersion,
	pnuint		puMinorVersion
)
{
	nuint32		cCode;

	cCode = (nuint32)get_server_context( (nuint32)NWCGetConnP(pAccess), 
						puMajorVersion, puMinorVersion);

	return( cCode );
}
