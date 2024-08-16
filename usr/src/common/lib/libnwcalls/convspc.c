/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:convspc.c	1.4"
#include "ntypes.h"
#include "nwcaldef.h"
#include "nwclocal.h"

/*manpage*NWConvertToSpecChar***********************************************
SYNTAX:  void N_API NWConvertToSpecChar
         (
            nptr workString
         )

REMARKS: This function takes a path string and searches for characters that
         could potentially conflict with augmented wild cards, then converts
         them to some predetermined values (predetermined by the server).

         This function is obsolete. Avoid its use.

ARGS:

INCLUDE: nwundoc.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 20 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
void N_API NWConvertToSpecChar
(
   nptr workString
)
{
   pnstr8 pbstrTempStr;
   nint i;

   pbstrTempStr = (pnstr8) workString;

   for (i=0; pbstrTempStr[i] != '\0'; i++)
   {
      switch ((nuint8) pbstrTempStr[i])
      {
         case ('*' | 0x80):
            pbstrTempStr[i] = SPECIALCHAR3;
            break;
         case ('.' | 0x80):
            pbstrTempStr[i] = SPECIALCHAR2;
            break;
         case ('?' | 0x80):
            pbstrTempStr[i] = SPECIALCHAR1;
            break;
      }
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/convspc.c,v 1.6 1994/06/08 23:08:22 rebekah Exp $
*/
