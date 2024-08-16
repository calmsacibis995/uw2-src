/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/dsintrn.h	1.4"
#ifndef  __DSINTRN_HEADER_
#define  __DSINTRN_HEADER_

#include <npackon.h>

/****************************************************************************
 *
 * (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.
 *
 * No part of this file may be duplicated, revised, translated, localized
 * or modified in any manner or compiled, linked or uploaded or downloaded
 * to or from any computer system without the prior written permission of
 * Novell, Inc.
 *
 ****************************************************************************/

/*-----------------------------------------------------------------------------
 * NWClient DS prototypes
 */

NWDSCCODE N_API NWCGetCanonicalizedName
(
   NWDSContextHandle context,
   pnstr8            objectName,
   punicode          canonicalizedName
);

NWDSCCODE N_API _GetAbbreviatedAndTypelessName
(
   NWDSContextHandle context,
   nuint32           flags,
   punicode          inObjectName,
   nptr              outObjectName
);

NWDSCCODE N_API NWCGlobalResolveName
(
   NWDSContextHandle    context,
   nuint32              flags,
   punicode             dn,
   pnuint32             objectID,
   pnuint               resolvedOffset,
   NWCONN_HANDLE N_FAR  *connID

);

NWDSCCODE N_API NWCConnectionIsAuthenticated
(
   NWCONN_HANDLE connID
);

NWDSCCODE N_API NWCBeginLogin
(
   NWCONN_HANDLE  conn,
   nuint32        entryID,
   pnuint32       pseudoID,
   pnuint32       serverRandom
);

NWDSCCODE N_API NWCGetPublicKeyFromConnection
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   ppnstr8           certificate
);

NWDSCCODE N_API NWCGetServerName
(
   NWCONN_HANDLE  conn,
   nflag32        contextFlags,
   punicode       dn
);

NWDSCCODE N_API NWCInternalAuthenticate
(
   NWCONN_HANDLE  connID,
   nuint32        optionsFlag,
   nptr           sessionKey
);

NWDSCCODE N_API NWCWGetAPData
(
   ppnstr8  buffer,
   pnstr8   limit,
   ppnstr8  data
);

NWDSCCODE N_API NWCWPutAPData
(
   ppnstr8  buffer,
   pnstr8   limit,
   pnstr8   data
);

NWDSCCODE N_API NWCGlobalAuthenticateConnection
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   pnstr8            publicKey,
   pnstr8            credential,
   pnstr8            signature
);

NWDSCCODE N_API NWCReadPublicKey
(
   NWCONN_HANDLE  conn,
   nuint32        entryID,
   ppnstr8        certificate
);

NWDSCCODE N_API NWCGetPrivateKey
(
   NWCONN_HANDLE  conn,
   pnstr8         serverPublicKey,
   nuint32        serverRandom,
   nuint32        entryID,
   pnstr8         passwordHash,
   pnuint32       begin,
   pnuint32       end,
   ppnstr8        privateKey
);

NWDSCCODE N_API NWCGetTDS
(
   ppnstr8  tds,
   ppnstr8  publicKey,
   ppnstr8  credential,
   ppnstr8  signature,
   pnuint   bytesRead
);

nbool _NWCIsSpace(unicode c);
nstr8 N_FAR NWCDStoupper(nstr8 c);

/*-----------------------------------------------------------------------------
 * Authentication
 */

#define TDS_SIZE                    (2 * 1024)
#define TDS_RESOURCE_TAG            272
#define MAX_AUTHENTICATION_PACKET   4096

typedef struct
{
   nuint16 totalSize;
   nuint16 publicKeyLength;
   nuint16 credentialLength;
   nuint16 signatureLength;
} AUTHEN_CACHE;

NWDSCCODE N_API BeginLogin
(
   NWCONN_HANDLE  conn,
   nuint32        entryID,
   pnuint32       pseudoID,
   pnuint32       serverRandom
);

NWDSCCODE N_API ConnectionIsAuthenticated
(
   NWCONN_HANDLE connID
);

NWDSCCODE N_API GetTDS
(
   ppnstr8  tds,
   ppnstr8  publicKey,
   ppnstr8  credential,
   ppnstr8  signature,
   pnuint   bytesRead
);

NWDSCCODE N_API GlobalAuthenticateConnection
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   pnstr8            publicKey,
   pnstr8            credential,
   pnstr8            signature
);

NWDSCCODE N_API GetPublicKeyFromConnection
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   ppnstr8           certificate
);

NWDSCCODE N_API GetPrivateKey
(
   NWCONN_HANDLE  conn,
   pnstr8         serverPublicKey,
   nuint32        serverRandom,
   nuint32        entryID,
   pnstr8         passwordHash,
   pnuint32       begin,
   pnuint32       end,
   ppnstr8        privateKey
);

NWDSCCODE N_API InternalAuthenticate
(
   NWCONN_HANDLE  connID,
   nuint32        optionsFlag,
   nptr           sessionKey
);

NWDSCCODE N_API ReadPublicKey
(
   NWCONN_HANDLE  conn,
   nuint32        entryID,
   ppnstr8        certificate
);

NWDSCCODE N_API WGetAPData
(
   ppnstr8  buffer,
   pnstr8   limit,
   ppnstr8  data
);

NWDSCCODE N_API WPutAPData
(
   ppnstr8  buffer,
   pnstr8   limit,
   pnstr8   data
);

/*-----------------------------------------------------------------------------
 * ResolveName
 */
NWDSCCODE N_API GlobalResolveName
(
   NWDSContextHandle    context,
   nuint32              flags,
   punicode             dn,
   pnuint32             objectID,
   pnuint               resolvedOffset,
   NWCONN_HANDLE N_FAR  *connID

);

/*-----------------------------------------------------------------------------
 * Naming
 */
NWDSCCODE N_API GetAbbreviatedAndTypelessName
(
   NWDSContextHandle context,
   nuint32           flags,
   punicode          inObjectName,
   nptr              outObjectName
);

NWDSCCODE N_API GetCanonicalizedName
(
   NWDSContextHandle context,
   pnstr8            objectName,
   punicode          canonicalizedName
);

NWDSCCODE N_API DNToWire
(
   NWDSContextHandle context,
   ppnstr8           buf,
   pnstr8            limit,
   pnstr8            dn,
   nuint32           flags
);

NWDSCCODE N_API DNToUnicode
(
   NWDSContextHandle context,
   pnstr8            dn,
   punicode          unicodeDN
);


/*-----------------------------------------------------------------------------
 */
NWDSCCODE N_API GlobalRequest
(
   NWCONN_HANDLE  conn,
   nint16         verb,
   size_t         reqSize,
   nptr           reqData,
   size_t         maxReplySize,
   size_t   N_FAR *actualReplySize,
   nptr           replyData
);

/*-----------------------------------------------------------------------------
 * Strings
 */

nstr8 N_FAR DStoupper(nstr8 c);
nbool _IsSpace(unicode c);

/*-----------------------------------------------------------------------------
 * Protocol builders
 */
NWDSCCODE N_API BuildAndSendListMessage
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   nuint32           objectHandle,
   pnint32           iterationHandle,
   pBuf_T            buf,
   pnstr8            className,
   pnstr8            subordinateName,
   nuint32           infoFlags,
   nuint32           protocolFlags
);

NWDSCCODE N_API BuildAndSendReadMessage
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   nuint32           objectID,
   pnstr8            subjectName,
   nuint32           operation,
   nuint32           infoType,
   nbool8            allFlag,
   pnint32           iterationHandle,
   pBuf_T            namesBuffer,
   pBuf_T            defsBuffer
);

NWDSCCODE N_API ListAndRead
(
   NWDSContextHandle context,
   pnstr8            objectName,
   nuint32           operation,
   pnint32           iterationHandle,
   nuint32           infoType,
   nbool8            allFlag,
   pBuf_T            nameBuffer,
   pBuf_T            returnBuffer
);

/*-----------------------------------------------------------------------------
 */
NWDSCCODE N_API TDRPutSchemaName
(
   pnuint8  N_FAR *buf,
   pnuint8        limit,
   nuint32        flags,
   pnstr8         schemaName,
   nbool          checkAbbreviation
);

NWDSCCODE N_API UnicodeSchemaName
(
   nflag32  flags,
   pnstr8   schemaName,
   punicode unicodeName,
   nbool    checkAbbreviation
);

/*---------------------------------------------------------------------------
 */
NWDSCCODE N_API GetServerName
(
   NWCONN_HANDLE  conn,
   nflag32        contextFlags,
   punicode       dn
);

NWDSCCODE N_API SplitParentAndChild
(
   NWDSContextHandle context,
   pnstr8            objectName,
   punicode          parentDN,
   punicode          RDN,
   pnbool            isRoot
);

NWDSCCODE N_API GetDRObjectIDandConnID
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   NWCONN_HANDLE  N_FAR *conn,
   pnuint32             objectHandle,
   nuint32              replicaType
);

NWDSCCODE N_API _ChangePreferredDSConn
(
   NWCONN_HANDLE        oldConn,
   NWCONN_HANDLE N_FAR  *newConn
);

NWDSCCODE N_API _UnicodeToLocal
(
   nuint32  context,
   punicode uniStr,
   pnstr8   locStr
);

NWDSCCODE N_API _LocalToUnicode
(
   nuint32  context,
   pnstr8   locStr,
   punicode uniStr
);

#ifdef N_PLAT_DOS

extern nint N_CDECL _DSIPXGLT
(
   pnuint8  networkAddress,
   pnuint8  immediateAddress,
   pnint    transportTime
);

#endif

#include <npackoff.h>

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/dsintrn.h,v 1.4 1994/09/26 17:09:16 rebekah Exp $
*/
