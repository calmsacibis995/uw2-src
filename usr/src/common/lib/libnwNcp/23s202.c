/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s202.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s202SetServerDateAndTime**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s202SetServerDateTime
         (
            pNWAccess pAccess,
            nuint8   buYear,
            nuint8   buMonth,
            nuint8   buDay,
            nuint8   buHour,
            nuint8   buMinute,
            nuint8   buSecond,
         );

REMARKS: This call sets the file server's date and time.

         If an invalid value is specified (for example, 250 for Current Month), an
         error is not returned; instead, the file server is set to an undefined but
         valid date and time.  The requesting client must have console operator
         or supervisor rights.

         Year contains a value from 0 to 99; values from 80 to 99 correspond to
         the years 1980 through 1999, while values from 0 to 79 correspond to
         the years 2000 through 2079.

         Month contains a value from 1 to 12 corresponding to months January to
         December.

         Day contains a value from 1 to 31 indicating the current day of the
         month.

         Hour contains a value from 0 to 23.  Zero (0) is midnight; 23 is 11 p.m.

         Minute contains a value from 0 to 59 indicating the current minute in
         the hour.

         Second contains a value from 0 to 59 indicating the current second in
         the minute.

         Week Day contains a value from 0 to 6.  Zero (0) is Sunday; six is
         Saturday.

ARGS: <> pAccess
      >  buYear
      >  buMonth
      >  buDay
      >  buHour
      >  buMinute
      >  buSecond

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     20 --  Get File Server Date And Time

NCP:     23 202  Set File Server Date And Time

CHANGES: 13 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s202SetServerDateTime
(
   pNWAccess pAccess,
   nuint8   buYear,
   nuint8   buMonth,
   nuint8   buDay,
   nuint8   buHour,
   nuint8   buMinute,
   nuint8   buSecond
)
{
   #define NCP_FUNCTION        ((nuint)      23)
   #define NCP_SUBFUNCTION     ((nuint8)    202)
   #define NCP_STRUCT_LEN      ((nuint16)     7)
   #define NCP_REQ_LEN         ((nuint)       9)
   #define NCP_REP_LEN         ((nuint)       0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = (nuint8) buYear;
   abuReq[4] = (nuint8) buMonth;
   abuReq[5] = (nuint8) buDay;
   abuReq[6] = (nuint8) buHour;
   abuReq[7] = (nuint8) buMinute;
   abuReq[8] = (nuint8) buSecond;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, NULL,
               NCP_REP_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s202.c,v 1.7 1994/09/26 17:35:55 rebekah Exp $
*/
