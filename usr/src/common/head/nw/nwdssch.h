/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdssch.h	1.4"
#ifndef  _NWDSSCH_HEADER_
#define  _NWDSSCH_HEADER_

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

#ifndef __NWDSTYPE_H
#include <nwdstype.h>
#endif

#ifndef _NWDSBUFT_HEADER_
#include <nwdsbuft.h>
#endif

#ifndef  _NWDSATTR_HEADER_
#include <nwdsattr.h>
#endif


#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE N_API NWDSDefineAttr
(
   NWDSContextHandle context,
   pnstr8            attrName,
   pAttr_Info_T      attrDef
);

NWDSCCODE N_API NWDSDefineClass
(
   NWDSContextHandle context,
   pnstr8            className,
   pClass_Info_T     classInfo,
   pBuf_T            classItems
);

NWDSCCODE N_API NWDSListContainableClasses
(
   NWDSContextHandle context,
   pnstr8            parentObject,
   pnint32           iterationHandle,
   pBuf_T            containableClasses
);

NWDSCCODE N_API NWDSModifyClassDef
(
   NWDSContextHandle context,
   pnstr8            className,
   pBuf_T            optionalAttrs
);

NWDSCCODE N_API NWDSReadAttrDef
(
   NWDSContextHandle context,
   nuint32           infoType,
   nbool8            allAttrs,
   pBuf_T            attrNames,
   pnint32           iterationHandle,
   pBuf_T            attrDefs
);

NWDSCCODE N_API NWDSReadClassDef
(
   NWDSContextHandle context,
   nuint32           infoType,
   nbool8            allClasses,
   pBuf_T            classNames,
   pnint32           iterationHandle,
   pBuf_T            classDefs
);

NWDSCCODE N_API NWDSRemoveAttrDef
(
   NWDSContextHandle context,
   pnstr8            attrName
);

NWDSCCODE N_API NWDSRemoveClassDef
(
   NWDSContextHandle context,
   pnstr8            className
);

NWDSCCODE N_API NWDSSyncSchema
(
   NWDSContextHandle context,
   pnstr8            server,
   nuint32           seconds
);

#ifdef __cplusplus
}
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdssch.h,v 1.6 1994/06/08 23:32:54 rebekah Exp $
*/
