/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsbuft.h	1.4"
#ifndef  _NWDSBUFT_HEADER_
#define  _NWDSBUFT_HEADER_

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

#include <time.h>

#ifndef __NWDSTYPE_H
#ifdef N_PLAT_UNIX
#include <nw/nwdstype.h>
#else
#include <nwdstype.h>
#endif
#endif

#ifndef  _NWDSDC_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsdc.h>
#else
#include <nwdsdc.h>
#endif
#endif

#ifndef  __NWDSDEFS_H
#ifdef N_PLAT_UNIX
#include <nw/nwdsdefs.h>
#else
#include <nwdsdefs.h>
#endif
#endif

#ifndef  _NWDSATTR_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsattr.h>
#else
#include <nwdsattr.h>
#endif
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else
#include <npackon.h>
#endif

#define  INPUT_BUFFER   0x00000001

typedef struct
{
   nuint32  operation;
   nuint32  flags;
   nuint32  maxLen;
   nuint32  curLen;
   pnuint8  lastCount;
   pnuint8  curPos;
   pnuint8  data;
} Buf_T, N_FAR *pBuf_T, N_FAR * N_FAR *ppBuf_T;

typedef struct
{
   nuint32  objectFlags;
   nuint32  subordinateCount;
   time_t   modificationTime;
   char     baseClass[MAX_SCHEMA_NAME_BYTES + 2];
} Object_Info_T, N_FAR *pObject_Info_T;

typedef struct
{
   nuint32  length;
   nuint8   data[MAX_ASN1_NAME];
} Asn1ID_T, N_FAR *pAsn1ID_T;

typedef struct
{
   nuint32  attrFlags;
   nuint32  attrSyntaxID;
   nuint32  attrLower;
   nuint32  attrUpper;
   Asn1ID_T asn1ID;
} Attr_Info_T, N_FAR *pAttr_Info_T;

/* an object identifier created allocated   */
/* according to the rules specified in the  */
/* ASN.1 standard; if no object identifier  */                                       
/* has been registered for the class, a     */                                       
/* zero-length octet string is specified    */
typedef struct
{
   nuint32  classFlags;
   Asn1ID_T asn1ID;                    
} Class_Info_T, N_FAR *pClass_Info_T;  

typedef struct
{
   nuint32  ID;
   char     defStr[MAX_SCHEMA_NAME_BYTES + 2];
   nflag16  flags;
} Syntax_Info_T, N_FAR *pSyntax_Info_T;

#define NWDSPutClassName(c, b, n) NWDSPutClassItem(c, b, n) /*
                                                               c -- context
                                                               b -- buf
                                                               n -- itemName
                                                            */

#define NWDSPutSyntaxName(c, b, n) NWDSPutClassItem(c, b, n) /*
                                                               c -- context
                                                               b -- buf
                                                               n -- itemName
                                                            */
#ifdef __cplusplus
   extern "C" {
#endif

/* NWClient DS Prototypes */
NWDSCCODE N_API NWCDSFreeBuf
(
   pBuf_T   buf
);

NWDSCCODE N_API NWCDSAllocBuf
(
   size_t   size,
   ppBuf_T  buf
);

NWDSCCODE N_API NWDSAllocBuf
(
   size_t   size,
   ppBuf_T  buf
);

NWDSCCODE N_API NWDSComputeAttrValSize
(
   NWDSContextHandle context,
   pBuf_T            buf,
   nuint32           syntaxID,
   pnuint32          attrValSize
);

NWDSCCODE N_API NWDSFreeBuf
(
   pBuf_T   buf
);

NWDSCCODE N_API NWDSGetAttrCount
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnuint32          attrCount
);

NWDSCCODE N_API NWDSGetAttrDef
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            attrName,
   pAttr_Info_T      attrInfo
);

NWDSCCODE N_API NWDSGetAttrName
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            attrName,
   pnuint32          attrValCount,
   pnuint32          syntaxID

);

NWDSCCODE N_API NWDSGetAttrVal
(
   NWDSContextHandle context,
   pBuf_T            buf,
   nuint32           syntaxID,
   nptr              attrVal
);

NWDSCCODE N_API NWDSGetClassDef
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            className,
   pClass_Info_T     classInfo
);

NWDSCCODE N_API NWDSGetClassDefCount
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnuint32          classDefCount
);

NWDSCCODE N_API NWDSGetClassItem
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            itemName
);

NWDSCCODE N_API NWDSGetClassItemCount
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnuint32          itemCount
);

NWDSCCODE N_API NWDSGetObjectCount
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnuint32          objectCount
);

NWDSCCODE N_API NWDSGetObjectName
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            objectName,
   pnuint32          attrCount,
   pObject_Info_T    objectInfo
);

NWDSCCODE N_API NWDSGetPartitionInfo
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            partitionName,
   pnuint32          replicaType
);

NWDSCCODE N_API NWDSGetServerName
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            serverName,
   pnuint32          partitionCount
);

NWDSCCODE N_API NWDSGetSyntaxCount
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnuint32          syntaxCount
);

NWDSCCODE N_API NWDSGetSyntaxDef
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            syntaxName,
   pSyntax_Info_T    syntaxDef
);

NWDSCCODE N_API NWDSInitBuf
(
   NWDSContextHandle context,
   nuint32           operation,
   pBuf_T            buf
);

NWDSCCODE N_API NWDSPutAttrName
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            attrName
);

NWDSCCODE N_API NWDSPutAttrVal
(
   NWDSContextHandle context,
   pBuf_T            buf,
   nuint32           syntaxID,
   nptr              attrVal
);

NWDSCCODE N_API NWDSPutChange
(
   NWDSContextHandle context,
   pBuf_T            buf,
   nuint32           changeType,
   pnstr8            attrName
);

NWDSCCODE N_API NWDSPutClassItem
(
   NWDSContextHandle context,
   pBuf_T            buf,
   pnstr8            itemName
);

NWDSCCODE N_API NWDSBeginClassItem
(
   NWDSContextHandle context,
   pBuf_T            buf
);

#ifdef __cplusplus
   }
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else
#include <npackoff.h>
#endif

#endif                           /* #ifndef _NWDSBUFT_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsbuft.h,v 1.5 1994/06/08 23:32:45 rebekah Exp $
*/
