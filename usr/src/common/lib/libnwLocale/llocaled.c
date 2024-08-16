/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:llocaled.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/llocaled.c,v 1.4 1994/09/26 17:20:48 rebekah Exp $"
/*========================================================================
  ==	  library 	      : NetWare Directory Services Platform Library
  ==
  ==    file            : LLOCALED.C
  ==
  ==	  author		      : Phil Karren
  ==
  ==    date            : 21 June 1989
  ==
  ==	  comments	      : data for NWLlocaleconv
  ==
  ==    modifications   :
  ==
  ==	  dependencies	   :
  ==
  ========================================================================*/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1
#define LOCALE_DATA_FILE

#if defined(N_PLAT_OS2)
#define INCL_DOSNLS
#define INCL_WINSHELLDATA
#define INCL_BASE
# include <os2.h>
#endif

#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

#if !defined(N_PLAT_UNIX)
unsigned long NWErrno;
#endif

VECTOR	_DBCSVector[NUMBER_VECTORS]={{0,0},{0,0},{0,0},{0,0}};

LCONV __lconvInfo = {{0,0,0,0}};

#if !(defined N_PLAT_MSW && defined N_ARCH_16) && !defined(N_PLAT_UNIX)

/* Windows uses its own upper case table. */

unsigned char upperCaseTable[256] = {0,0};

#endif

/*
   The following are global variables set by Lsetlocale, and used by
   locale-sensitive library routines.  The values are defaulted to the
   C locale.  They will be overridden by the current locale if Lsetlocale
   is called.
*/

NWLOCALE_DEFS localeInfo =
{
    "",           /* _collateTable[256] */
    0,            /* _countryID */
    437,          /* _codePage */
    0,            /* _dateFormat */
    "$",          /* _currencySymbol[5] */
    ",",          /* _thousandSeparator[2] */
    ".",          /* _decimalSeparator[2] */
    "/",          /* _dateSeparator[2] */
    ":",          /* _timeSeparator[2] */
    '0',          /* _currencyFormatFlags */
    '0',          /* _digitsInCurrency */
    0,            /* _timeFormat */
    ",",          /* _dataListSeparator[2] */
    "am",         /* _am[MERIDLEN] */
    "pm"          /* _pm[MERIDLEN] */
};

#if defined (N_PLAT_DOS)
   unsigned int  DosMajorVersion;
   unsigned int  DosMinorVersion;
#elif defined (N_PLAT_OS2)
   COUNTRYCODE country;
#endif

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/llocaled.c,v 1.4 1994/09/26 17:20:48 rebekah Exp $
*/
