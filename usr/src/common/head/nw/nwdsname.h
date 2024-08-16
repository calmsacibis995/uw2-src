/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsname.h	1.4"
#ifndef  _NWDSNAME_HEADER_
#define  _NWDSNAME_HEADER_

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


#ifdef __cplusplus
   extern "C" {
#endif

NWDSCCODE N_API NWDSAbbreviateName
(
   NWDSContextHandle context,
   pnstr8            inName,
   pnstr8            abbreviatedName
);

NWDSCCODE N_API NWDSCanonicalizeName
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            canonName
);

NWDSCCODE N_API NWDSRemoveAllTypes
(
   NWDSContextHandle context,
   pnstr8            name,
   pnstr8            typelessName
);

NWDSCCODE N_API NWDSResolveName
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   NWCONN_HANDLE  N_FAR *conn,
   pnuint32             objectID
);

NWDSCCODE N_API NWDSCIStringsMatch
(
   NWDSContextHandle context,
   pnstr8            string1,
   pnstr8            string2,
   pnint             matches
);

#ifdef __cplusplus
   }
#endif

#endif                           /* #ifndef _NWDSNAME_HEADER_ */

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsname.h,v 1.6 1994/06/08 23:32:52 rebekah Exp $
*/
