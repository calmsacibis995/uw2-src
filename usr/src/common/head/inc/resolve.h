/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/resolve.h	1.5"
#ifndef _RESOLVE_HEADER_
#define _RESOLVE_HEADER_

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

#define NETWORK_ADDRESS "Network Address"    /* Temp. !!          */
#define RF_AUTHENTICATE 0x00000001L
#define MAX_ULONG       0xFFFFFFFFL

#if defined(N_PLAT_DOS) || (defined N_PLAT_MSW && defined N_ARCH_16 && !defined N_PLAT_WNT)
#define NON_CONNECT_WEIGHT    18
#elif defined(N_PLAT_OS2)
#define NON_CONNECT_WEIGHT    1000
#else
#define NON_CONNECT_WEIGHT    0
#endif

typedef struct COSTTAG
{
   unsigned long  cost;
   uint8 N_FAR    *referral;
   int            first;
} COSTTAG;

/* NWClient DS prototypes */

N_EXTERN_LIBRARY( NWDSCCODE )
NWCDSGetNearestDirectoryServer
(
   NWCONN_HANDLE  N_FAR *conn
);

N_EXTERN_LIBRARY( NWDSCCODE )
NWCDSResolveName
(
   NWDSContextHandle    context,       /* input  */
   uint8                authenticate,  /* input  */
   uint8                createEntry,   /* input  */   
   uint32               replicaType,   /* input  */
   unicode  N_FAR       *objectName,   /* input  */
   NWCONN_HANDLE  N_FAR *conn,         /* output */
   uint32   N_FAR       *objectID      /* output */
);

N_EXTERN_LIBRARY( NWDSCCODE )
NWCResolveNameProtocol
(
   NWCONN_HANDLE  conn,
   unicode  N_FAR *objectName,
   uint32         flags,
   uint32         scopeOfReferral,
   Buf_T    N_FAR *respBuf
);

N_EXTERN_LIBRARY( NWDSCCODE )
NWDSGetNearestDirectoryServer
(
   NWCONN_HANDLE  N_FAR *conn
);

N_EXTERN_LIBRARY( NWDSCCODE )
ResolveName
(
   NWDSContextHandle    context,       /* input  */
   uint8                authenticate,  /* input  */
   uint8                createEntry,   /* input  */   
   uint32               replicaType,   /* input  */
   unicode  N_FAR       *objectName,   /* input  */
   NWCONN_HANDLE  N_FAR *conn,         /* output */
   uint32   N_FAR       *objectID      /* output */
);

N_EXTERN_LIBRARY( NWDSCCODE )
ResolveNameProtocol
(
   NWCONN_HANDLE  conn,
   unicode  N_FAR *objectName,
   uint32         flags,
   uint32         scopeOfReferral,
   Buf_T    N_FAR *respBuf
);

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/resolve.h,v 1.7 1994/09/26 17:09:34 rebekah Exp $
*/
