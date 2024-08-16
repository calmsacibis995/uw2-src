/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:20.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP20GetServerDateAndTime**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP20GetServerDateAndTime
         (
            pNWAccess pAccess,
            pnuint8  pbuYear,
            pnuint8  pbuMonth,
            pnuint8  pbuDay,
            pnuint8  pbuHour,
            pnuint8  pbuMinute,
            pnuint8  pbuSecond,
            pnuint8  pbuDayOfWeek,
         );

REMARKS: This call returns the current date and time being kept by the file server.
         The system time clock is a 7-byte clock.

         Year contains a value from 0 to 179; values from 80 to 99 correspond to
         the years 1980 through 1999, while values from 100  to 179 correspond
         to the years 2000 through 2079.

         Month contains a value from 1 to 12 corresponding to months January to
         December.

         Day contains a value from 1 to 31 indicating the current day of the
         month.

         Hour contains a value from 0 to 23.  Zero (0) is 12 midnight; 23 is 11
         p.m.

         Minute contains a value from 0 to 59, indicating the minute in the hour.

         Second contains a value from 0 to 59, indicating the second in the
         minute.

         Week Day contains a value from 0 to 6.  Zero (0) is Sunday; six is
         Saturday.

         This call can be used by any client.


ARGS: <> pAccess
      <  pbuYear (optional)
      <  pbuMonth (optional)
      <  pbuDay (optional)
      <  pbuHour (optional)
      <  pbuMinute (optional)
      <  pbuSecond (optional)
      <  pbuDayOfWeek (optional)

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 202  Set File Server Date And Time

NCP:     20 --  Get File Server Date And Time

CHANGES: 9 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP20GetServerDateAndTime
(
   pNWAccess pAccess,
   pnuint8  pbuYear,
   pnuint8  pbuMonth,
   pnuint8  pbuDay,
   pnuint8  pbuHour,
   pnuint8  pbuMinute,
   pnuint8  pbuSecond,
   pnuint8  pbuDayOfWeek
)
{
   #define NCP_FUNCTION    ((nuint) 20)
   #define NCP_REQ_LEN     ((nuint) 0)
   #define NCP_REPLY_LEN   ((nuint) 7)

   nint32   lCode;
   nuint8 abuReply[7];

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, NULL, NCP_REQ_LEN, abuReply,
         NCP_REPLY_LEN, NULL);;
   if (lCode == 0)
   {
      if (pbuYear)
         *pbuYear = abuReply[0];
      if (pbuMonth)
         *pbuMonth = abuReply[1];
      if (pbuDay)
         *pbuDay = abuReply[2];
      if (pbuHour)
         *pbuHour = abuReply[3];
      if (pbuMinute)
         *pbuMinute = abuReply[4];
      if (pbuSecond)
         *pbuSecond = abuReply[5];
      if (pbuDayOfWeek)
         *pbuDayOfWeek = abuReply[6];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/20.c,v 1.7 1994/09/26 17:33:25 rebekah Exp $
*/
