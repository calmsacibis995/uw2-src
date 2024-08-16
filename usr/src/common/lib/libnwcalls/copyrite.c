/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:copyrite.c	1.1"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/
#include "nwcaldef.h"
#include "nwundoc.h"

#define MAJOR_VERSION      4
#define MINOR_VERSION      50
#define REVISION_LEVEL     0
#define BETA_RELEASE_LEVEL 0

#if defined(N_PLAT_MSW) || \
    defined(N_PLAT_OS2)

#if defined(N_PLAT_MSW) || defined(N_PLAT_OS2)
#define STR_TYPE N_API
#else
#define STR_TYPE
#endif

nint8 STR_TYPE DLLVersionString[]="VeRsIoN=4.50 NWCALLS.DLL ALPHA Client SDK 2.00";
nint8 STR_TYPE DLLCopyrightString[]="CoPyRiGhT=(c) Copyright 1989-1993 Novell, Inc.  All rights reserved.";
#elif defined( N_PLAT_NLM )

#else
static char LIBVersionString[]="VeRsIoN=4.50 NWCALLS.LIB ALPHA Client SDK 2.00";
static char LIBCopyrightString[]="CoPyRiGhT=(c) Copyright 1989-1993 Novell, Inc.  All rights reserved.";
#endif

void N_API NWGetNWCallsVersion(
  pnuint8 majorVersion,
  pnuint8 minorVersion,
  pnuint8 revisionLevel,
  pnuint8 betaReleaseLevel)
{
  *majorVersion     = MAJOR_VERSION;
  *minorVersion     = MINOR_VERSION;
  *revisionLevel    = REVISION_LEVEL;
  *betaReleaseLevel = BETA_RELEASE_LEVEL;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/copyrite.c,v 1.1 1994/09/26 17:44:50 rebekah Exp $
*/

