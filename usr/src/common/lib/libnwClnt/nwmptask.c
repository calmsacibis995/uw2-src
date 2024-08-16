/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwClnt:nwmptask.c	1.10"
#ident	"$Header: /SRCS/esmp/usr/src/nw/lib/libnwClnt/nwmptask.c,v 1.22.2.2 1994/11/21 02:52:19 ericw Exp $"

static char SccsID[] = "nwmptask.c	1.3 92/02/1416:38:51 92/02/1416:39:13";
/*
(C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

No part of this file may be duplicated, revised, translated, localized or modified
in any manner or compiled, linked or uploaded or downloaded to or from any
computer system without the prior written consent of Novell, Inc.
*/

#include <stdio.h>
#include <errno.h>
#include <sys/tiuser.h>
#include <nw/nwerror.h>
#include <sys/nwctypes.h>
#include <nw/ntypes.h>
#include <sys/nwmp.h>
#include <sys/nucerror.h>
#include <nucinit.h>
#include "nucrequester.h"
#include "nwClnt.h"

/*
**  This is wrong, but header files are a mess.
*/
#define NWC_TRAN_TYPE_IPX              0x0001
#define NWC_OPEN_PUBLIC                0x0008

extern ce_t *connFindAnyConnection();

/*
 *	NWMPOpenServiceTask - Create a connection instance to a service
 */
int32
NWMPOpenServiceTask(
	int fp,
	struct netbuf *serviceAddress,
	nuint32	flags
)
{
	struct openServiceTaskReq req;
	int ccode;

	req.address.maxlen = serviceAddress->maxlen;
	req.address.len = serviceAddress->len;
	req.address.buf = serviceAddress->buf;
	req.flags = flags;
	req.mode = 0;

	ccode = ioctl(fp,NWMP_OPEN_TASK,&req);

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return(ccode);
}

int32
NWMPCloseServiceTask( int fp )
{
	int ccode;

	ccode = ioctl( fp, NWMP_CLOSE_TASK, 0 );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return( ccode );
}

int32
NWMPCloseTaskWithForce( int fp )
{
	int ccode;

	ccode = ioctl( fp, NWMP_CLOSE_TASK_WITH_FORCE, 0 );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return( ccode );
}

int32
NWMPAuthenticateTask(
	int	fp,
	uint32	objectID,
	nuint	authType,
	uint8	*authKey,
	int	authKeyLength
)
{
	struct authTaskReq 	req;
	int 			ccode;

	req.objID = objectID;
	req.authType = authType;
	req.authKeyLength = authKeyLength;
	req.authKey = authKey;

	ccode = ioctl( fp, NWMP_AUTHENTICATE_TASK, &req );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return(ccode);
}

/*
 *	NWMPScanTasks -	Scan tasks of a particular service
 */
int32
NWMPScanTasks(
	int fp,
	struct netbuf *serviceAddress,
	int32 *uid,
	int32 *gid,
	uint32 *mode
)
{
struct scanTaskReq req;
	int	ccode;

	req.address.maxlen = serviceAddress->maxlen;
	req.address.len = serviceAddress->len;
	req.address.buf = serviceAddress->buf;
	req.userID = *uid;
	req.groupID = *gid;
	req.mode = *mode;

	if (ccode = ioctl(fp,NWMP_SCAN_TASK,&req))
	{
		ccode = map_nwmp_errors( ccode );
#ifdef REQ_DEBUG
		printf("NWMPScanTasks; ccode = %x \n",ccode);
#endif
	}
	else
	{
		*uid = req.userID;
		*gid = req.groupID;
		*mode = req.mode;
	}

	return(ccode);
}

NWMPSetChecksumLevel( int fd, uint32 level )
{
	struct checksumLevel req;
	int ccode;

	req.level = level;

	ccode = ioctl(fd, NWMP_SET_CHECKSUM_LEVEL, &req);

#ifdef REQ_DEBUG
	fprintf( stdout, "NWMPSetChecksumLevel ioctl returns 0x%x\n", ccode );
#endif
	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return(ccode);
}

NWMPSetSignatureLevel( int fd, uint32 level )
{
	struct signatureLevel req;
	int ccode;

	req.level = level;

	ccode = ioctl(fd, NWMP_SET_SIGNATURE_LEVEL, &req);

#ifdef REQ_DEBUG
	fprintf( stdout, "NWMPSetSignatureLevel ioctl returns 0x%x\n", ccode );
#endif
	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return(ccode);
}

NWMPMakeSignatureDecision( int fd )
{
	struct makeSignatureDecisionReq req;
	int ccode;

	ccode = ioctl(fd, NWMP_MAKE_SIGNATURE_DECISION, &req);

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return(ccode);
}

NWMPGetChecksumFlags(
	int fd,
	uint32 *u_defaultLevel,
	uint32 *u_baseLevel,
	uint32 *u_userLevel
)
{
    struct getSecurityFlagsReq req;
    int ccode;

    ccode = ioctl(fd, NWMP_GET_CHECKSUM_FLAGS, &req);

    /* map nwmp errors into "requester" errors */
    if( ccode ) {
        ccode = map_nwmp_errors( ccode );
    } else {
        *u_defaultLevel = req.defaultLevel;
        *u_baseLevel = req.baseLevel;
        *u_userLevel = req.userLevel;
    }

    return(ccode);
}

NWMPGetSecurityFlags(
	int fd,
	uint32 *u_defaultLevel,
	uint32 *u_baseLevel,
	uint32 *u_userLevel
)
{
	struct getSecurityFlagsReq req;
	int ccode;

	ccode = ioctl(fd, NWMP_GET_SECURITY_FLAGS, &req);

	/* map nwmp errors into "requester" errors */
	if( ccode ) {
		ccode = map_nwmp_errors( ccode );
	} else {
		*u_defaultLevel = req.defaultLevel;
		*u_baseLevel = req.baseLevel;
		*u_userLevel = req.userLevel;
	}

	return(ccode);
}

NWMPLicenseConn( int fd, uint32 flags )
{
	int ccode;

	ccode = (ioctl(fd, NWMP_LICENSE_CONN, flags));

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return( ccode );

}

NWMPMakeConnPermanent( int fd )
{
	int ccode;

	ccode = (ioctl(fd, NWMP_MAKE_CONNECTION_PERMANENT));

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return( ccode );
}

NWMPGetConnInfo(
	int fd,
	nuint uInfoLevel,
	nuint uInfoLen,
	uint8 *pConnInfo
)
{
	struct getConnInfoReq req;
	int ccode;

	req.uInfoLevel = uInfoLevel;
	req.uInfoLen = uInfoLen;
	req.buffer = (nptr)pConnInfo;

	ccode = (ioctl(fd, NWMP_GET_CONN_INFO, &req));

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return( ccode );

}

NWMPSetConnInfo(
	int fd,
	nuint uInfoLevel,
	nuint uInfoLen,
	uint8 *pConnInfo
)
{
	struct setConnInfoReq req;
	int ccode;

	req.uInfoLevel = uInfoLevel;
	req.uInfoLen = uInfoLen;
	req.buffer = (nptr)pConnInfo;

	ccode = (ioctl(fd, NWMP_SET_CONN_INFO, &req));

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return( ccode );

}

NWMPScanConnInfo(
	int fd,
	struct netbuf *serverAddress,
	nuint uScanInfoLevel,
	char *pScanConnInfo,
	nuint uScanInfoLen,
	nuint uScanFlags,
	nuint uInfoLevel,
	nuint uInfoLen,
	nuint32 *pluConnectionReference,
	char *pConnInfo
)
{
	struct scanConnInfoReq req;
	int ccode;

	req.address.maxlen = serverAddress->maxlen;
	req.address.len = serverAddress->len;
	req.address.buf = serverAddress->buf;

	req.uScanInfoLevel = uScanInfoLevel;
	req.pScanConnInfo = (uint8 *)pScanConnInfo;
	req.uScanInfoLen = uScanInfoLen;
	req.uScanFlags = uScanFlags;
	req.uInfoLevel = uInfoLevel;
	req.uInfoLen = uInfoLen;
	req.luConnectionReference = *pluConnectionReference;
	req.buffer = (uint8 *)pConnInfo;

	ccode = ioctl(fd, NWMP_SCAN_CONN_INFO, &req);

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	if (ccode == SUCCESS)
		*pluConnectionReference = req.luConnectionReference;

	return(ccode);
}

nuint32
NWMPAllocTDS(
	nuint tagID,
	nuint size,
	nuint flags
)
{
	struct TDSReq	req;
	nuint32		ccode;
	nuint32         fd;

	ce_t	*connection;

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

	req.tag = tagID;
	req.maxSize = size;
	req.flags = flags;

	ccode = ioctl( fd, NWMP_ALLOC_TDS_REQ, &req );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	if( connection == NULL )
		NWMPClose( fd );

	return( ccode );
}

nuint32
NWMPFreeTDS( nuint32 tagID )
{
	struct TDSReq	req;
	nuint32		ccode;
	nuint32         fd;

	ce_t *connection;

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

    req.tag = tagID;

    ccode = ioctl( fd, NWMP_FREE_TDS_REQ, &req );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	if( connection == NULL )
		NWMPClose( fd );

    return( ccode );

}

nuint32
NWMPGetTDS(
	nuint		tagID,
	pnuint		maxSize,
	pnuint		dataSize,
	pnuint		flags
)
{
	struct TDSReq	req;
	nuint32		ccode;
	nuint32         fd;

	ce_t *connection;

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

    req.tag = tagID;

    ccode = ioctl( fd, NWMP_GET_TDS_REQ, &req );

	if( !ccode ) {
		*maxSize = req.maxSize;
		*dataSize = req.dataSize;
		*flags = req.flags;
	} else {
		/* map nwmp errors into "requester" errors */
		ccode = map_nwmp_errors( ccode );
	}

	if( connection == NULL )
		NWMPClose( fd );

    return( ccode );

}

/*
**	We are passing back the number of characters read rather than
**	store a null into the buffer.
*/
nuint32
NWMPReadTDS(
	nuint		tagID,
	nuint		length,
	nuint		offset,
	char		*bufPtr,
	pnuint		bytesRead
)
{
	struct TDSRWReq	req;
	nuint32		ccode;
	nuint32         fd;

	ce_t *connection;

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

    req.tag = tagID;
    req.length = length;
    req.offset = offset;
	req.buffer = bufPtr;

    ccode = ioctl( fd, NWMP_READ_TDS_REQ, &req );

	if( !ccode ) {
		*bytesRead = req.length;
	} else {
		/* map nwmp errors into "requester" errors */
		ccode = map_nwmp_errors( ccode );
	}

	if( connection == NULL )
		NWMPClose( fd );

    return( ccode );

}

nuint32
NWMPWriteTDS(
	nuint		tagID,
	nuint		length,
	nuint		offset,
	char		*bufPtr,
	pnuint		bytesWritten
)
{
	struct TDSRWReq	req;
	nuint32		ccode;
	nuint32         fd;

	ce_t *connection;

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

    req.tag = tagID;
    req.length = length;
    req.offset = offset;
    req.buffer = bufPtr;

    ccode = ioctl( fd, NWMP_WRITE_TDS_REQ, &req );

	*bytesWritten = req.length;

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	if( connection == NULL )
		NWMPClose( fd );

    return( ccode );

}

/* this is the same as the default name context */
nuint32
NWMPWriteDN( nuint length, pnstr buffer )
{
	struct gbufReq	req;
	nuint32 	ccode;
	nuint32		fd;

	ce_t *connection;

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

	req.buffer = buffer;
	req.bufLength = length;

	ccode = ioctl( fd, NWMP_DN_WRITE_REQ, &req );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	if( connection == NULL )
		NWMPClose( fd );

	return( ccode );

}

nuint32
NWMPReadDN( pnuint length, pnstr buffer )
{
	struct gbufReq	req;
	nuint32 	ccode;
	nuint32		fd;

	ce_t *connection;

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

	req.buffer = buffer;
	req.bufLength = *length;

	ccode = ioctl( fd, NWMP_DN_READ_REQ, &req );

	if( !ccode ) {
		*length = req.bufLength;
	} else {
		/* map nwmp errors into "requester" errors */
		ccode = map_nwmp_errors( ccode );
	}

	if( connection == NULL )
		NWMPClose( fd );

	return( ccode );

}

nuint32
NWMPSetPreferredTree( nuint length, pnstr buffer )
{
    struct gbufReq	req;
    nuint32            	ccode;
    nuint32            	fd;

    ce_t *connection;

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

    req.buffer = buffer;
	req.bufLength = length;

    ccode = ioctl( fd, NWMP_SET_PREF_TREE_REQ, &req );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	if( connection == NULL )
		NWMPClose( fd );

    return( ccode );

}

nuint32
NWMPGetPreferredTree( pnuint length, pnstr buffer )
{
    struct gbufReq  	req;
    int             	ccode;
    int             	fd;

    ce_t *connection;

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

    req.buffer = buffer;
	req.bufLength = *length;

    ccode = ioctl( fd, NWMP_GET_PREF_TREE_REQ, &req );

    *length = req.bufLength;
	buffer[ req.bufLength ] = '\0';

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	if( connection == NULL )
		NWMPClose( fd );

    return( ccode );

}

nuint32
NWMPReadClientNLSPath( pnuint length, pnstr buffer )
{
    struct gbufReq	req;
    int             	ccode;
    int             	fd;

    ce_t *connection;

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

    req.buffer = buffer;
    req.bufLength = *length;

    ccode = ioctl( fd, NWMP_READ_CLIENT_NLS_PATH_REQ, &req );

    *length = req.bufLength;

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	if( connection == NULL )
		NWMPClose( fd );

    return( ccode );

}

/* 2.1 non-ds candidates (to add) include
	a. max packet size to begin negotiations
	b. max NCP Retries
	c. NCP timeout value
*/
nuint32
NWMPInitRequester( INIT_REQ_T *args1, DS_INIT_REQ_T *args2 )
{
	struct initReq	req;
	int 		ccode;
	int             fd;
	nuint32 reference;
	nuint32 connHandle;
	
	ce_t *connection;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if (connection = connFindAnyConnection())
	{
		fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
	} else {
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		if ((fd = NWMPOpen()) < 0)
		{
			return( NWE_INVALID_SERVICE_REQUEST );
		}
	}

	if( args1 == NULL ) {
		req.securityLevel = 0xFF;
		req.checksumLevel = 0xFF;
	} else {
		req.securityLevel = args1->securityPreference;
		req.checksumLevel = args1->checksumPreference;
	}

	if( args2 == NULL ) {
		req.DNBufferLength = 0;
		req.prefTreeNameLen = 0;
		req.NLSPathLen = 0;
	} else {
		if( args2->defaultContext != NULL ) {
			req.DNBuffer = args2->defaultContext;
			req.DNBufferLength = strlen( (char *) args2->defaultContext );
		} else {
			req.DNBufferLength = 0;
		}
		if( args2->preferredTree != NULL ) {
			req.preferredTree = args2->preferredTree;
			req.prefTreeNameLen = strlen( (char *) args2->preferredTree );
		} else {
			req.prefTreeNameLen = 0;
		}
		if( args2->clientNLSPath != NULL ) {
			req.NLSPath = args2->clientNLSPath;
			req.NLSPathLen = strlen( (char *) args2->clientNLSPath );
		} else {
			req.NLSPathLen = 0;
		}
	}

	ccode = ioctl( fd, NWMP_SET_CONFIG_PARMS_REQ, &req );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

#ifdef REQ_DEBUG
	fprintf( stdout, "REQ_DEBUG: NWMPInitRequester defaultContext length = %d\n",
		req.DNBufferLength );
	fprintf( stdout, "REQ_DEBUG: NWMPInitRequester preferredTree length = %d\n",
		req.prefTreeNameLen );
	fprintf( stdout, "REQ_DEBUG: NWMPInitRequester NLSPath length = %d\n",
		req.NLSPathLen );
#endif /* REQ_DEBUG */

	/*
	**  Establish primary connection if one does not already exist.
	*/
	if (get_primary_conn_reference(&reference) != SUCCESS)
	{
		if (args1 && args1->preferredServer)
		{
			if (open_conn_by_name(args1->preferredServer, NWC_TRAN_TYPE_IPX,
				&connHandle, NWC_OPEN_PUBLIC) == SUCCESS)
			{
				close_conn(connHandle);
			} else {
				(void) open_conn_using_sap_file(&reference);
			}
		} else {
			(void) open_conn_using_sap_file(&reference);
		}
	}

	if( connection == NULL )
		NWMPClose( fd );

	return( ccode );
}

nuint32
NWMPGetRequesterVersion(
	pnuint8	majorVersion,
	pnuint8	minorVersion,
	pnuint8	revision
)
{
	struct	reqVersReq	req;
	nuint32             ccode;
	nuint32             fd;

	ce_t *connection;

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

	ccode = ioctl( fd, NWMP_GET_REQUESTER_VERSION_REQ, &req );

	if( !ccode ) {
		*majorVersion = req.majorVersion;
		*minorVersion = req.minorVersion;
		*revision	  = req.revision;
	} else {
		/* map nwmp errors into "requester" errors */
		ccode = map_nwmp_errors( ccode );
	}

	if( connection == NULL )
		NWMPClose( fd );

    return( ccode );

}

nuint32
NWMPCheckConnection( nuint32 fd )
{
	nuint32	ccode;

	ccode = ioctl( fd, NWMP_CHECK_CONNECTION );

    /* map nwmp errors into "requester" errors */
    if( ccode )
        ccode = map_nwmp_errors( ccode );

    return( ccode );

}

nuint32
NWMPSetMonitoredConn( nuint32 fd )
{
	nuint32 ccode;

	ccode = ioctl( fd, NWMP_SET_MONITORED_CONN_REQ, NULL );

	/* map nwmp errors into "requester" errors */
	if( ccode )
		ccode = map_nwmp_errors( ccode );

	return( ccode );
	
}

nuint32
NWMPGetMonitoredConn( pnuint32 connRef )
{
	struct	monConnReq	req;
	nuint32             ccode;
	nuint32             fd;

	ce_t *connection;

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

	ccode = ioctl( fd, NWMP_GET_MONITORED_CONN_REQ, &req );

	if( !ccode ) {
		*connRef = req.connReference;
	} else {
		/* map nwmp errors into "requester" errors */
		ccode = map_nwmp_errors( ccode );
	}

	if( connection == NULL )
		NWMPClose( fd );

	return( ccode );
	
}

nuint32
NWMPGetMaxConns( pnuint32 maxConns )
{
	struct	getLimitsReq	req;
	nuint32			ccode;
	nuint32			fd;

	ce_t *connection;

	MUTEX_LOCK( &_nwClnt_connTable_lock );
	if (connection = connFindAnyConnection())
	{
		fd = connection->ce_fd;
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );

	} else {
		MUTEX_UNLOCK( &_nwClnt_connTable_lock );
		if ((fd = NWMPOpen()) < 0)
		{
			return( NWE_INVALID_SERVICE_REQUEST );
		}
	}

	ccode = ioctl( fd, NWMP_GET_LIMITS, &req );

	if ( !ccode )
	{
		*maxConns = req.maxClientTasks;

	} else {
		/* map nwmp errors into "requester" errors */
		ccode = map_nwmp_errors( ccode );
	}

	if ( connection == NULL )
		NWMPClose( fd );

	return( ccode );
}

int
map_nwmp_errors( int errorCode )
{
    nuint32         ccode;

    if( errorCode < GTS_RANGE ) {
        switch( errorCode ) {
            case SPI_ACCESS_DENIED:
            case SPI_REQUEST_NOT_SUPPORTED:
                ccode = NWE_INVALID_SERVICE_REQUEST ;
                break;
            case SPI_AUTHENTICATION_FAILURE:
                ccode = NWE_NOT_AUTHENTICATED ;
                break;
            case SPI_CLIENT_RESOURCE_SHORTAGE:
            case SPI_MEMORY_EXHAUSTED:
                ccode = NWE_OUT_OF_HEAP_SPACE ;
                break;
            case SPI_FILE_TOO_BIG:
                ccode = NWE_INVALID_MESSAGE_LENGTH ;
                break;
            case SPI_GENERAL_FAILURE:
            case SPI_INVALID_OFFSET:
            case SPI_INVALID_PATH:
            case SPI_NAME_TOO_LONG:
            case SPI_USER_MEMORY_FAULT:
                ccode = NWE_PARAM_INVALID ;
                break;
            case SPI_INACTIVE:
            case SPI_INVALID_SPROTO:
                ccode = NWE_SOCKET_OPEN_FAILED ;
                break;
			/* we rely on distinguishing this "error" */
            case SPI_TASK_EXISTS:
				ccode = NWE_EA_SCAN_DONE;
				break;
            case SPI_NO_MORE_SERVICE:
            case SPI_NO_MORE_TASK:
				ccode = NWE_SCAN_COMPLETE;
				break;
            case SPI_NO_SUCH_SERVICE:
            case SPI_NO_SUCH_TASK:
            case SPI_TASK_TERMINATING:
                ccode = NWE_INVALID_TASK_NUMBER ;
                break;
            case SPI_SERVER_UNAVAILABLE:
                ccode = NWE_SERVER_CONNECTION_LOST ;
                break;
            case SPI_SERVICE_EXISTS:
                ccode = NWE_ALREADY_IN_USE ;
                break;
            case SPI_MORE_ENTRIES_EXIST:
            case SUCCESS:
                ccode = SUCCESS ;
                break;
            case SPI_FILE_ALREADY_EXISTS:
                ccode = NWE_TDS_TAG_IN_USE ;
                break;
            case SPI_CANT_OPEN_DIRECTORY:
                ccode = NWE_TDS_INVALID_TAG ;
                break;
            case SPI_BAD_BYTE_RANGE:
                ccode = NWE_TDS_WRITE_TRUNCATED ;
                break;
			case SPI_TASK_HOLDING_RESOURCES:
				ccode = NWE_BIND_SECURITY_INVALID ;
            default:
                ccode = NWE_REQUESTER_FAILURE ;
                break;
        }
    } else {
		if(ccode == NWD_GTS_NO_RESOURCE) {
			ccode = CONNECTION_TABLE_FULL;
		} else {
        	ccode = NWE_REQUESTER_FAILURE ;
		}
    }

    return( ccode );
}
