/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:34s8.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncptts.h"

/*manpage*NWNCP34s8TTSSetThresholds**************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
         NWNCP34s8TTSSetThresholds
         (
            pNWAccess pAccess,
            nuint8   buLogLockThreshold,
            nuint8   buPhyLockThreshold,
         );

REMARKS: Allows an application to specify the number of logical and
         physical record locks allowed before an implicit transaction
         begins.  That is, if a threshold value is set to 0, an implicit
         transaction will begin when the first lock is made for that
         particular lock type; if a threshold value is set to 100, an
         implicit transaction will begin when the 101st lock is made, and
         so on.

         This call and TTS Get Workstation Thresholds (function 34,
         subfunction 7) can be used by applications to change the implicit
         workstation thresholds.

         The thresholds set by this function are valid not only for the
         requesting application, but for all applications at the requesting
         workstation.

         This call can be used to turn off implicit transactions or to
         allow applications that always keep one or more records locked to
         function.  For example, applications that use only explicit
         transactions, but sometimes generate unnecessary implicit
         transactions, can use this function to turn off all implicit
         transactions.

         The default threshold for logical and physical locks is 0.  A
         threshold value of OxFF means implicit transactions for that lock
         type are not in effect.

         Threshold numbers are set to zero when an End Of Job call is made.
         DOS workstation users normally use the SETTTS.EXE utility to set
         workstation thresholds.

ARGS: <> pAccess
      >  buLogLockThreshold
      >  buLogLockThreshold

INCLUDE: ncptts.h

RETURN:  0x00  Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 07  TTS Get Workstation Thresholds

NCP:     34 08  TTS Set Workstation Thresholds

CHANGES: 12 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP34s8TTSSetThresholds
(
   pNWAccess pAccess,
   nuint8   buLogLockThreshold,
   nuint8   buPhyLockThreshold
)
{
   #define NCP_FUNCTION    ((nuint) 34)
   #define NCP_SUBFUNCTION ((nuint8) 8)
   #define REQ_LEN         ((nuint) 3)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buLogLockThreshold;
   abuReq[2] = buPhyLockThreshold;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/34s8.c,v 1.7 1994/09/26 17:38:08 rebekah Exp $
*/
