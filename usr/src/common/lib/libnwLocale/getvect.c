/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:getvect.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/getvect.c,v 1.4 1994/09/26 17:20:34 rebekah Exp $"
 /*========================================================================
  ==    library   : NetWare Directory Services Platform Library
  ==
  ==    file      : GETVECT.C
  ==
  ==    routine   : NWGetDBCSVector
  ==
  ==    author    : Phil Karren
  ==
  ==    date      : 21 June 1989
  ==
  ==    comments  : Get DBCS Vector from DOS or OS2.  The DBCS
  ==              vector is used to tell what characters
  ==              in a string are single or double-byte
  ==              characters.
  ==
  ==    modifications :
  ==
  ==    dependencies   :
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

#if defined(NWDOS)
# include <stdlib.h>
#elif defined(NWOS2)
#define INCL_BASE
# include <os2.h>
#endif

#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

/*****************************************************************************/

N_GLOBAL_LIBRARY(int)
_NWGetDBCSVector(
   VECTOR N_FAR *vector,
   size_t maxSize )
{
   int rcode = 0;

#if defined(NWOS2)

   COUNTRYCODE country;
   COUNTRYINFO countryinfo;
   USHORT returnLength;

   country.country = 0;
   country.codepage = 0;

   if(rcode = DosGetCtryInfo(sizeof(COUNTRYINFO), &country,
         (PCOUNTRYINFO) &countryinfo, &returnLength))
      return -1;

   country.country = countryinfo.country;
   country.codepage = countryinfo.codepage;

   rcode = DosGetDBCSEv((USHORT) maxSize, &country, (PCHAR)vector);

#elif defined N_PLAT_UNIX

	maxSize = maxSize;

   switch (localeInfo._countryID)
	 {

	/* EUC has high order bit set for Asian locales */

	case JAPAN:
	case KOREA:
	case TAIWAN:
   case PRC:
		vector->lowValue = 0x80;
      vector->highValue = 0xff;
      vector++;
			goto Terminate;


   default:
	 Terminate:
      vector->lowValue = 0x00;
      vector->highValue = 0x00;
   }
#else    /* NWDOS and NWWIN */

   maxSize = maxSize;

   switch (localeInfo._countryID)
	 {
   case JAPAN:
      vector->lowValue = (nuint8) 0x81;
      vector->highValue = (nuint8) 0x9f;
      vector++;
      vector->lowValue = (nuint8) 0xe0;
      vector->highValue = (nuint8) 0xfc;
      vector++;
			goto Terminate;

   case KOREA:
      vector->lowValue = (nuint8) 0xA1;
      vector->highValue = (nuint8) 0xFE;
      vector++;
			goto Terminate;

   case TAIWAN:
      vector->lowValue = (nuint8) 0xA1;
      vector->highValue = (nuint8) 0xF9;
      vector++;
			goto Terminate;

   case PRC:
      vector->lowValue = (nuint8) 0xA1;
      vector->highValue = (nuint8) 0xFE;
      vector++;
			goto Terminate;

   default:
	 Terminate:
      vector->lowValue = (nuint8) 0x00;
      vector->highValue = (nuint8) 0x00;
   }
#endif

   return rcode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/getvect.c,v 1.4 1994/09/26 17:20:34 rebekah Exp $
*/
