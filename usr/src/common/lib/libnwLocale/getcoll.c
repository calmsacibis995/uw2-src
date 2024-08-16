/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:getcoll.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/getcoll.c,v 1.4 1994/09/26 17:20:33 rebekah Exp $"
/*========================================================================
  ==  $Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/getcoll.c,v 1.4 1994/09/26 17:20:33 rebekah Exp $
  ==  library        : NetWare Enabled Library
  ==
  ==  file           : GETCOLL.C
  ==
  ==  routine        : NWGetCollateTable
  ==
  ==  author         : Phil Karren
  ==
  ==  date           : 21 June 1989
  ==
  ==  comments       : Get collation table. This table is used by these
  ==                   routines:
  ==
  ==                       strcoll
  ==                       strxfrm
  ==
  ==  A collation table is a table of 256 values. Each value contains a
  ==  collation weight for a corresponding character.
  ==
  ==  modifications  :
  ==
  ==  dependencies   :
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

#if defined(N_PLAT_DOS) || (defined N_PLAT_MSW && defined N_ARCH_16)
extern  unsigned int  DosMajorVersion;
extern  unsigned int  DosMinorVersion;
#elif defined(N_PLAT_OS2)
#define INCL_BASE
# include <os2.h>
#endif

#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

#ifndef MIN
#define MIN(a,b) ((a)>(b)?(b):(a))
#endif


int N_API NWGetCollateTable(
   char N_FAR *retCollateTable, size_t maxLen)
{
   int rcode = 0;

#if defined(N_PLAT_DOS)
   NWDOS_EXT_COUNTRY_TABLE collateTable;
   unsigned int i;
   unsigned min;

   if (((DosMajorVersion == 3) && (30 <= DosMinorVersion))
   || (4 <= DosMajorVersion))
   {
      _NWGetMSDOSCollateTable ((UINT8 N_FAR *)&collateTable,
         sizeof(NWDOS_EXT_COUNTRY_TABLE));

      /*The table is a max of 256*/
      min = (unsigned) MIN(maxLen, 256);
      for (i = 0; i < min; i++)
      {
         /* First two bytes of the DOS table are length*/
         retCollateTable[i] = collateTable.countryTable[i+2];
      }
   }
   else
   {
      for (i = 0; i < (unsigned int) maxLen; i++)
         retCollateTable[i] = (char) i;
   }

#elif defined(N_PLAT_OS2)

   COUNTRYCODE country;
   USHORT tableLength;

   country.country = 0;
   country.codepage = 0;

   rcode = DosGetCollate ( (USHORT)maxLen, &country,
                           retCollateTable, &tableLength );

#endif

   return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/getcoll.c,v 1.4 1994/09/26 17:20:33 rebekah Exp $
*/
