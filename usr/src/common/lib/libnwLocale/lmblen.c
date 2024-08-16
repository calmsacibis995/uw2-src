/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lmblen.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lmblen.c,v 1.1 1994/09/26 17:20:49 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==    file            : LMBLEN.C
  ==
  ==    routine         : NWLmblen
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 18 July 1989
  ==
  ==	comments	: Locale-sensitive version of ANSII standard
  ==			  function lmblen
  ==			  Count the number of characters in a
  ==			  NULL-terminated string.  The string can be a
  ==			  mixture of single- and multi-byte characters.
  ==
  ==    modifications   :
  ==
  ==	dependencies	: NWNextChar
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

#if (defined N_PLAT_UNIX || (defined N_PLAT_MSW && defined N_ARCH_32) || defined N_PLAT_NLM)
#define _fstrlen strlen
#endif

#include <string.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

int N_API NWLmblen(
   char N_FAR *string,
   size_t numBytes)
{
   int totalLength;

   if (_DBCSVector[0].lowValue)
   {
      if (string)
      {
         for (totalLength = 0;
         (numBytes > 0) && (*string != (char) NULL);
         numBytes--, string = NWNextChar(string))
            totalLength++;

         return (totalLength);
      }
      else
         return 0;
   }
   else
   {
      if (string)
      {
         totalLength = _fstrlen(string);
			if (totalLength > (int) numBytes)
				return (numBytes);
			else
         	return (totalLength);
      }
      else
         return 0;
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lmblen.c,v 1.1 1994/09/26 17:20:49 rebekah Exp $
*/
