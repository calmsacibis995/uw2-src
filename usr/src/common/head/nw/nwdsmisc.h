/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head-nuc:nw/nwdsmisc.h	1.5"
#ifndef  _NWDSMISC_HEADER_
#define  _NWDSMISC_HEADER_
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

#ifndef _NWDSBUFT_HEADER_
#ifdef N_PLAT_UNIX
#include <nw/nwdsbuft.h>
#else
#include <nwdsbuft.h>
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

#define DS_SYNTAX_NAMES    0
#define DS_SYNTAX_DEFS     1

#define DS_STRING             0x0001   /* string, can be used in names */
#define DS_SINGLE_VALUED      0x0002
#define DS_SUPPORTS_ORDER     0x0004
#define DS_SUPPORTS_EQUAL     0x0008
#define DS_IGNORE_CASE        0x0010   /* Ignore case          */
#define DS_IGNORE_SPACE       0x0020   /* Ignore white space   */
#define DS_IGNORE_DASH        0x0040   /* Ignore dashes        */
#define DS_ONLY_DIGITS        0x0080
#define DS_ONLY_PRINTABLE     0x0100
#define DS_SIZEABLE           0x0200

#ifdef __cplusplus
   extern "C" {
#endif

NWDSCCODE N_API NWDSCloseIteration
(
   NWDSContextHandle context,
   nint32            iterationHandle,
   nuint32           operation
);

NWDSCCODE N_API NWDSGetSyntaxID
(
   NWDSContextHandle context,
   pnstr8            attrName,
   pnuint32          syntaxID
);

NWDSCCODE N_API NWDSReadSyntaxes
(
   NWDSContextHandle context,
   nuint32           infoType,
   nbool8            allSyntaxes,
   pBuf_T            syntaxNames,
   pnint32           iterationHandle,
   pBuf_T            syntaxDefs
);

NWDSCCODE N_API NWDSReadSyntaxDef
(
   NWDSContextHandle context,
   nuint32           syntaxID,
   pSyntax_Info_T    syntaxDef
);

NWDSCCODE N_API NWDSReplaceAttrNameAbbrev
(
   NWDSContextHandle context,
   pnstr8            inStr,
   pnstr8            outStr
);

NWDSCCODE N_API NWDSGetObjectHostServerAddress
(
   NWDSContextHandle context,
   pnstr8            objectName,
   pnstr8            serverName,
   pBuf_T            netAddresses
);

void N_API NWGetNWNetVersion
(
   pnuint8 majorVersion,
   pnuint8 minorVersion,
   pnuint8 revisionLevel,
   pnuint8 betaReleaseLevel
);

NWDSCCODE N_API NWIsDSServer
(
   NWCONN_HANDLE  conn,
   pnstr8         treeName
);

NWDSCCODE N_API NWDSGetBinderyContext
(
   NWDSContextHandle context,
   NWCONN_HANDLE     conn,
   pnuint8           BinderyEmulationContext
);

NWDSCCODE N_API NWDSRepairTimeStamps
(
   NWDSContextHandle context,
   pnstr8            partitionRoot
);

nint N_API NWGetFileServerUTCTime
(
   NWCONN_HANDLE  conn,
   pnuint32       time
);


#ifdef __cplusplus
   }
#endif

#ifdef N_PLAT_UNIX
#include <nw/npackoff.h>
#else
#include <npackoff.h>
#endif

#endif

/*
$Header: /SRCS/esmp/usr/src/nw/head/nw/nwdsmisc.h,v 1.6 1994/09/26 17:12:07 rebekah Exp $
*/
