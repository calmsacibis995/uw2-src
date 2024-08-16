/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:loc_rite.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/loc_rite.c,v 1.1 1994/09/26 17:20:52 rebekah Exp $"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#include "ntypes.h"

#if defined(WIN32)
char N_API DLLVersionString[]="VeRsIoN=4.50 NWLOCALE.DLL ALPHA Client SDK 2.00";
char N_API DLLCopyrightString[]="CoPyRiGhT=(c) Copyright 1991-1993 Novell, Inc.  "
                                "All rights reserved.";
#elif defined(NWWIN)
char N_API DLLVersionString[]="VeRsIoN=4.50 NWLOCALE.DLL ALPHA Client SDK 2.00";
char N_API DLLCopyrightString[]="CoPyRiGhT=(c) Copyright 1991-1993 Novell, Inc.  "
                                "All rights reserved.";
#elif defined(NWOS2)
static char DLLVersionString[]="VeRsIoN=4.50 NWLOCALE.DLL ALPHA Client SDK 2.00";
static char DLLCopyrightString[]="CoPyRiGhT=(c) Copyright 1991-1993 Novell, Inc.  "
                                 "All rights reserved.";
#elif defined( N_PLAT_NLM )

#else  /* NWDOS */
static char LIBVersionString[]="VeRsIoN=4.50 NWLOCALE.LIB ALPHA Client SDK 2.00";
static char LIBCopyrightString[]="CoPyRiGhT=(c) Copyright 1991-1993 Novell, Inc.  "
                                 "All rights reserved.";
#endif

void N_API NWGetNWLOCALEVersion(unsigned char N_FAR *majorVersion,
                                unsigned char N_FAR *minorVersion,
                                unsigned char N_FAR *revisionLevel,
                                unsigned char N_FAR *betaReleaseLevel)
{
#if defined(NWOS2)
   char *ptr;

   ptr = DLLVersionString;
   ptr = DLLCopyrightString;
   ptr = ptr;

#elif defined(NWDOS)
   char *ptr;

   ptr = LIBVersionString;
   ptr = LIBCopyrightString;
   ptr = ptr;

#endif

   *majorVersion = 4;
   *minorVersion = 50;
   *revisionLevel = 0;
   *betaReleaseLevel = 0;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/loc_rite.c,v 1.1 1994/09/26 17:20:52 rebekah Exp $
*/
