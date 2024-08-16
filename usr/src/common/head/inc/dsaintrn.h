/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:inc/dsaintrn.h	1.3"
#ifndef  _DSAINTRN_HEADER_
#define  _DSAINTRN_HEADER_

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

/*
#include "nwdstype.h"
#include "nwdsbuft.h"
*/

NWDSCCODE N_API _NWCNDSReadObjectInfo
(
   NWDSContextHandle    context,
   NWCONN_HANDLE        conn,
   nuint32              objectHandle,
   pnstr8               distinguishedName,
   pObject_Info_T       objectInfo
);

NWDSCCODE N_API _NDSReadObjectInfo
(
   NWDSContextHandle    context,
   NWCONN_HANDLE        conn,
   nuint32              objectHandle,
   pnstr8               distinguishedName,
   pObject_Info_T       objectInfo
);

#endif                           /* #ifndef _DSAINTRN_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/inc/dsaintrn.h,v 1.3 1994/06/08 23:35:19 rebekah Exp $
*/
