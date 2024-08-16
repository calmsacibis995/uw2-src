/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:ltruncat.c	1.2"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/ltruncat.c,v 1.3 1994/09/26 17:21:11 rebekah Exp $"
/*========================================================================
  ==	library 	: NWLOCALE
  ==
  ==    file            : LTRUNCAT.C
  ==
  ==    routine         : NWLTruncateString
  ==
  ==	author		: Vance Campbell
  ==
  ==    date            : 11 April, 1994
  ==
  ==	comments	: Truncates a string if string length is greater than or
  ==             equal to MaxLen.
  ==
  ==			  This function is Double-Byte sensitive
  ==
  ==	modifications	: 
  ==
  ==	dependencies	: NWstrlen, NWPrevChar
  ==
  ========================================================================*/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------
SYNTAX:  N_GLOBAL_LIBRARY( nint )
         NWCTruncateStrA
         (
            pnchar8 pStr,
            nint  iMaxLen
         )

REMARKS: Returns the length of a string, truncating it if needed. If the
         truncation would chop a double byte character in half, it
         eliminates the first half.

ARGS: >  pStr
         pointer to first character in string.

      >  iMaxLen
         maximum length of string, in bytes

INCLUDE: ntypes.h nwlocale.h

RETURN:

CLIENT:  DOS WIN OS2 NT

CHANGES: 
----------------------------------------------------------------------------
*/
#if defined N_PLAT_MSW
#include <windows.h>
#endif

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#include "ntypes.h"
#include "nwlocale.h"

N_GLOBAL_LIBRARY( nint )
NWLTruncateString
(
   pnchar8 pStr,
   nint  iMaxLen
)
{
nint iLen;
pnstr8 pTmp;

   if((iLen = NWstrlen((char *)pStr)) >= iMaxLen)
   {
      pTmp = (pnstr8)NWPrevChar((char *)pStr, (char *)&pStr[iMaxLen]);	/* go out max chars. */
		*pTmp = (nstr8) '\0';				/* NULL terminate string */
		iLen = NWstrlen((char *)pStr);		/* Return new length */
   }

   return iLen;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/ltruncat.c,v 1.3 1994/09/26 17:21:11 rebekah Exp $
*/
