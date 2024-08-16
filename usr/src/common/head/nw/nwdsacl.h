/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsacl.h	1.4"
#ifndef _NWDSACL_HEADER_
#define _NWDSACL_HEADER_

#ifndef __NWDSTYPE_H
#ifdef N_PLAT_UNIX
#include <nw/nwdstype.h>
#else /* !N_PLAT_UNIX */
#include <nwdstype.h>
#endif /* N_PLAT_UNIX */
#endif

#ifndef _NWDSBUFT_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsbuft.h>
#else /* !N_PLAT_UNIX */
#include <nwdsbuft.h>
#endif /* N_PLAT_UNIX */
#endif

#ifndef  _NWDSDC_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsdc.h>
#else /* !N_PLAT_UNIX */
#include <nwdsdc.h>
#endif /* N_PLAT_UNIX */
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackon.h>
#else /* !N_PLAT_UNIX */
#include <npackon.h>
#endif /* N_PLAT_UNIX */

#ifdef __cplusplus
   extern "C" {
#endif

NWDSCCODE N_API NWDSGetEffectiveRights
(
   NWDSContextHandle context,
   pnstr8            subjectName,
   pnstr8            objectName,
   pnstr8            attrName,
   pnuint32          privileges
);

NWDSCCODE N_API NWDSListAttrsEffectiveRights
(
   NWDSContextHandle    context,
   pnstr8               objectName,
   pnstr8               subjectName,
   nbool8               allAttrs,
   pBuf_T               attrNames,
   pnint32              iterationHandle,
   pBuf_T               privilegeInfo
);

#ifdef __cplusplus
   }
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else /* !N_PLAT_UNIX */
#include <npackoff.h>
#endif /* N_PLAT_UNIX */

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsacl.h,v 1.5 1994/06/08 23:32:40 rebekah Exp $
*/
