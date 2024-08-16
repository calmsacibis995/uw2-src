/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s22.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpconn.h"

/*manpage*NWNCP23s22GetStationLoggedInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s22GetStationLoggedInfo
         (
            pNWAccess pAccess,
            nuint8   buTargetConnNum,
            pnuint32 pluUsrID,
            pnuint16 psuUsrType,
            pnuint8  pbuUsrNameB48,
            pnuint8  pbuLoginTimeB7,
            pnuint8  pbuReserved,
         )

REMARKS: This call returns information about a user at the given connection
         number.  This call may be used by any client.

         The Login Time field has the following format:

            Byte[0] = Year
            Byte[1] = Month
            Byte[2] = Day
            Byte[3] = Hour
            Byte[4] = Minute
            Byte[5] = Second
            Byte[6] = DayOfWeek

         The Year byte contains a value from 0 to 99: values from 80 to 99
         correspond to the years 1980-1999, 0 to 79 correspond to the years 2000-
         2079.

         The Month byte contains a value from 1 to 12 corresponding to
         months January to December.

         The Day byte contains a value from 1 to 31, indicating the current
         day of the month.

         The Hour byte contains a value from 0 to 23, indicating the hour of
         day (0 means 12:00 midnight; 23 means 11:00 p.m., etc.).

         The Minute byte contains a value from 0 to 59, indicating the
         current minute in the hour.

         The Second byte contains a value from 0 to 59, indicating the
         current second in the minute.

         The Day Of Week byte contains a value from 0 to 6 corresponding
         with the weekday (0 for Sunday, 6 for Saturday, etc.).

         This call is replaced by the NetWare 386 v3.11 call Get Station's Logged
         Info
         (0x2222  23  28).

ARGS: <> pAccess
      >  buTargetConnNum
      <  pluUsrID
      <  psuUsrType
      <  pbuUsrNameB48
      <  pbuLoginTimeB7
      <  pbuReserved

INCLUDE: ncpconn.h

RETURN:  0x0000    Successful
         0x8996    Server Out Of Space
         0x89FC    No Such Object
         0x89FD    Bad Station Number
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 28  Get Station's Logged Info
         23 21  Get Object Connection List

NCP:     23 22  Get Station's Logged Info (old)

CHANGES: 15 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s22GetStationLoggedInfo
(
   pNWAccess pAccess,
   nuint8   buTargetConnNum,
   pnuint32 pluUsrID,
   pnuint16 psuUsrType,
   pnuint8  pbuUsrNameB48,
   pnuint8  pbuLoginTimeB7,
   pnuint8  pbuReserved
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 22)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 62)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nint    i;

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   abuReq[3] = buTargetConnNum;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      if(pluUsrID)
         NCopyHiLo32(pluUsrID, &abuReply[0]);
      if(psuUsrType)
         NCopyHiLo16(psuUsrType, &abuReply[4]);
      if(pbuUsrNameB48)
      {
         for (i=0; i<48; i++)
            pbuUsrNameB48[i] = abuReply[i + 6];
      }
      if(pbuLoginTimeB7)
      {
         for (i=0; i<7; i++)
            pbuLoginTimeB7[i] = abuReply[i + 54];
      }
      if(pbuReserved)
         *pbuReserved = abuReply[61];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s22.c,v 1.7 1994/09/26 17:36:22 rebekah Exp $
*/
