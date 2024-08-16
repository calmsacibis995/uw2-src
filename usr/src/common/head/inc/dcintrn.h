/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/dcintrn.h	1.5"
#ifndef  _DCINTRN_HEADER_
#define  _DCINTRN_HEADER_
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

#define DCK_LIST              6
#define DCK_ERROR_BUFFER      7

#if (defined N_PLAT_MSW && defined N_ARCH_16 && !defined N_PLAT_WNT) || defined(N_PLAT_OS2)
# define MAX_DC_TABLE_ENTRIES 24
#else
# define MAX_DC_TABLE_ENTRIES 8
#endif

#include <npackon.h>

typedef struct
{
#ifdef N_PLAT_MAC
   nuint16        type;             /* type of context (0x00 = DS context) */
#endif
   nuint32        DCKFlags;
   nuint32        DCKConfidence;
   nuint32        DCKReferralScope;
   nuint32        DCKTransportType[2];
   punicode       DCKNameContext;
   pBuf_T         errorBuffer;
   RDN_List_T     DCKList;
   NWCONN_HANDLE  lastConnection;
#ifdef N_PLAT_MAC
   Handle         taskContext;
   Handle         globalContext;
#endif
} DCStruct_t;

NWDSCCODE N_API NWCGetContextInUnicode
(
   NWDSContextHandle context,
   punicode          name
);

#ifdef N_PLAT_NLM
NWDSCCODE _DSSetUserID
(
   NWDSContextHandle context,
   nuint32           userID
);

NWDSCCODE _DSUserIsOwner
(
   NWDSContextHandle context,
   nuint32           guessID
);
#endif

#ifdef N_PLAT_MAC
DCStruct_t N_FAR *_NDSGetDCTable
(
   NWDSContextHandle context
);
#endif

NWDSCCODE N_API GetContextInUnicode
(
   NWDSContextHandle context,
   punicode          name
);

#include <npackoff.h>

#endif                           /* #ifndef _DCINTRN_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/dcintrn.h,v 1.5 1994/09/26 17:09:15 rebekah Exp $
*/
