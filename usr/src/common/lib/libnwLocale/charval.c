/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:charval.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/charval.c,v 1.1 1994/09/26 17:20:27 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Directory Services Platform Library
  ==
  ==	file		: CHARVAL.C
  ==
  ==	routine 	: NWChar
  ==
  ==	author		: Allan Neill
  ==
  ==	date		: 1 Nov 1990
  ==
  ==	comments	: Check for single-byte or double-byte character
  ==			  and return the character in an integer.
  ==
  ==    modifications   :
  ==
  ==	dependencies	:
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

#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"

int N_API NWCharVal(
   char N_FAR *ptr)
{
   UINT16 return_char;
   UINT16 N_FAR *int_ptr;
   VECTOR N_FAR *vector;

   return_char = (unsigned char) *ptr;

   if(_DBCSVector[0].lowValue)
   {
      for (vector = _DBCSVector; vector->lowValue != 0; vector++)
         if (((unsigned char)(vector->lowValue) <= (unsigned char) *ptr) &&
         ((unsigned char) *ptr <= (unsigned char)(vector->highValue)))
         {
            int_ptr = (UINT16 N_FAR *) ptr;
            return_char = *int_ptr;
            break;
         }
   }

   return (return_char);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/charval.c,v 1.1 1994/09/26 17:20:27 rebekah Exp $
*/
