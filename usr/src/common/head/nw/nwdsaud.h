/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsaud.h	1.4"
#ifndef  _NWDSAUD_HEADER_
#define  _NWDSAUD_HEADER_

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

#ifndef NWCONNECT_INC
#ifdef N_PLAT_UNIX
#include <nw/nwconnec.h>
#else
#include <nwconnec.h>
#endif
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else
#include <npackon.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif

NWDSCCODE N_API NWDSAuditGetObjectID
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   NWCONN_HANDLE  N_FAR *conn,
   pnuint32             objectID
);

#ifdef __cplusplus
}
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else
#include <npackoff.h>
#endif

#endif                           /* #ifndef _NWDSAUD_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsaud.h,v 1.5 1994/06/08 23:32:43 rebekah Exp $
*/
