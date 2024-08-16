/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:chartype.c	1.3"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/chartype.c,v 1.4 1994/09/26 17:20:26 rebekah Exp $"
/*========================================================================
  ==	library 	: NetWare Enabled Library
  ==
  ==    file            : CHARTYPE.C
  ==
  ==    routine         : NWCharType
  ==
  ==	author		: Phil Karren
  ==
  ==    date            : 19 July 1989
  ==
  ==	comments	: Check for single-byte or double-byte character
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

int N_API NWCharType(
   unsigned char ch)
{
   int answer = NWSINGLE_BYTE;
   VECTOR N_FAR *vector;

   if(_DBCSVector[0].lowValue)
   {
      for (vector = _DBCSVector; vector->lowValue != 0; vector++)
         if (((unsigned char)(vector->lowValue) <= (unsigned char)ch) &&
         ((unsigned char)ch <= (unsigned char)(vector->highValue)))
         {
            answer = NWDOUBLE_BYTE;
            break;
         }
   }
   return(answer);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/chartype.c,v 1.4 1994/09/26 17:20:26 rebekah Exp $
*/
