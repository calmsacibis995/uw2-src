/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:datetime.c	1.4"
#include "nwcaldef.h"
#include "nwmisc.h"

/*manpage*NWUnpackDateTime**************************************************
SYNTAX:  void N_API NWUnpackDateTime
         (
           nuint32 dateTime,
           NW_DATE NWPTR sDate
           NW_TIME NWPTR sTime
         )

REMARKS: Converts the packed date and time to structures

ARGS:  > dateTime
            The date and time packed according to DOS rules
            (see int 21h - function 57h, Subfunction 00h)
            bits 31-25: year - 1980
               24-21: month
               20-16: day
               15-11: hours (0-23)
               10-5:  minutes
               4-0:   seconds/2

       < sDate (optional)
            Pointer to the structure to receive the unpacked date:

            typedef
            {
            nuint8 day;
            nuint8 month;
            nuint16 year;
            } NW_DATE;

       < sTime (optional)
            Pointer to the structure to recieve the unpacked time:

            typedef
            {
            nuint8 seconds;
            nuint8 minutes;
            nuint16 hours;
            } NW_TIME;

INCLUDE: nwmisc.h

RETURN:  n/a

SERVER:  n/a

CLIENT:  DOS WIN OS2 NT

SEE:     NWUnpackDate, NWUnpackTime, NWPackDate, NWPackTime, NWPackDateTime

NCP:     n/a

CHANGES: 10 Nov 1992 - written - jwoodbur
         13 Sep 1993 - NWNCP not used (hungarian added) - dromrell
***************************************************************************/
void N_API NWUnpackDateTime
(
   nuint32        dateTime,
   NW_DATE NWPTR  date,
   NW_TIME NWPTR  time
)
{
   NWUnpackTime((nuint16) dateTime, time);
   NWUnpackDate((nuint16) (dateTime >> 16), date);
}

/*manpage*NWUnpackDate******************************************************
SYNTAX:  void N_API NWUnpackDate
         (
           nuint16 suDate,
           NW_DATE NWPTR sDate
         )

REMARKS: Converts the packed date to a structure

ARGS: >  suDate
            The date packed according to DOS rules
            (see int 21h - function 57h, Subfunction 00h)
            bits 15-9: year - 1980
                  8-5:  month
                  4-0:  day

      <  sDate
            Pointer to the structure to receive the unpacked date:

            typedef
            {
               nuint8 day;
               nuint8 month;
               nuint16 year;
            } NW_DATE;

INCLUDE: nwmisc.h

RETURN:  n/a

CLIENT:  DOS WIN OS2 NT

SEE:     NWUnpackTime, NWUnpackDateTime, NWPackDate, NWPackTime,
         NWPackDateTime

CHANGES: 10 Nov 1992 - written - jwoodbur
         13 Sep 1993 - NWNCP not used (hungarian added) - dromrell
****************************************************************************/
void N_API NWUnpackDate
(
  nuint16         date,
  NW_DATE NWPTR   sDate
)
{
  if(sDate)
  {
    sDate->day   = (nuint8)(date & 0x1f);
    date >>= 5;
    sDate->month = (nuint8)(date & 0x0f);
    date >>= 4;
    sDate->year  = date + 1980;
  }
}

/*manpage*NWUnpackTime******************************************************
SYNTAX:  void N_API NWUnpackTime
         (
           nuint16 suTime,
           NW_TIME NWPTR sTime
         )

REMARKS: Converts the packed time to a structure

ARGS:  > suTime
           The time packed according to DOS rules
           (see int 21h - function 57h, Subfunction 00h)
           bits 15-11: hours (0-23)
                10-5:  minutes
                4-0:   seconds/2

       < sTime
           Pointer to the structure to receive the unpacked time:

           typedef
           {
             nuint8 seconds;
             nuint8 minutes;
             nuint16 hours;
           } NW_TIME;

INCLUDE: nwmisc.h

RETURN:  n/a

CLIENT:  DOS WIN OS2 NT

SEE:     NWUnpackDate, NWUnpackDateTime, NWPackTime, NWPackDate,
         NWPackDateTime

CHANGES: 10 Nov 1992 - written - jwoodbur
         13 Sep 1993 - NWNCP not used (hungarian added) - dromrell
****************************************************************************/
void N_API NWUnpackTime
(
  nuint16         time,
  NW_TIME NWPTR   sTime
)
{
  if(sTime)
  {
    sTime->seconds = (nuint8)((time & 0x1f) * 2);
    time >>= 5;
    sTime->minutes = (nuint8)(time & 0x3f);
    time >>= 6;
    sTime->hours   = time;
  }
}

/*manpage*NWPackDateTime****************************************************
SYNTAX:  nuint32 N_API NWPackDateTime
         (
           NW_DATE NWPTR sDate
           NW_TIME NWPTR sTime
         )

REMARKS: Converts the date and time structures to packed date and time

ARGS:  > sDate (optional)
           Pointer to the structure to receive the unpacked date:

           typedef
           {
             nuint8 day;
             nuint8 month;
             nuint16 year;
           } NW_DATE;

       > sTime (optional)
           Pointer to the structure to recieve the unpacked time:

           typedef
           {
             nuint8 seconds;
             nuint8 minutes;
             nuint16 hours;
           } NW_TIME;

INCLUDE: nwmisc.h

RETURN:  The date and time packed according to DOS rules
         (see int 21h - function 57h, Subfunction 00h)
         bits 31-25: year - 1980
              24-21: month
              20-16: day
              15-11: hours (0-23)
              10-5:  minutes
              4-0:   seconds/2

CLIENT:  DOS WIN OS2 NT

SEE:     NWPackDate, NWPackTime, NWUnpackDate, NWUnpackTime,
         NWUnpackDateTime

CHANGES: 10 Nov 1992 - written - jwoodbur
         13 Sep 1993 - NWNCP not used (hungarian added) - dromrell
****************************************************************************/
nuint32 N_API NWPackDateTime
(
  NW_DATE NWPTR date,
  NW_TIME NWPTR time
)
{
union
{
  struct
  {
    nuint16 time;
    nuint16 date;
  } words;
  nuint32 dateTime;
} dateTime;

  dateTime.words.time = NWPackTime(time);
  dateTime.words.date = NWPackDate(date);

  return dateTime.dateTime;
}

/*manpage*NWPackDate********************************************************
SYNTAX:  nuint16 N_API NWPackDate
         (
           NW_DATE NWPTR sDate
         )

REMARKS: Converts the packed date to a structure

ARGS:  > sDate
           Pointer to the structure to receive the unpacked date:

           typedef
           {
             nuint8 day;
             nuint8 month;
             nuint16 year;
           } NW_DATE;

INCLUDE: nwmisc.h

RETURN:  The date packed according to DOS rules
           (see int 21h - function 57h, Subfunction 00h)
           bits 15-9: year - 1980
                8-5:  month
                4-0:  day

CLIENT:  DOS WIN OS2 NT

SEE:     NWPackTime, NWPackDateTime, NWUnpackTime, NWUnpackDate,
         NWUnpackDateTime

CHANGES: 10 Nov 1992 - written - jwoodbur
         13 Sep 1993 - NWNCP not used (hungarian added) - dromrell
****************************************************************************/
nuint16 N_API NWPackDate
(
  NW_DATE NWPTR sDate
)
{
  nuint16 date;

  if(sDate)
  {
    date = sDate->year - 1980;
    date <<= 4;
    date |= (nuint16) (sDate->month & 0x0f);
    date <<= 5;
    date |= (nuint16) (sDate->day & 0x1f);
  }
  else
    return 0;

  return date;
}

/*manpage*NWPackTime********************************************************
SYNTAX:  void N_API NWPackTime
         (
           nuint16 time,
           NW_TIME NWPTR sTime
         )

REMARKS: Converts the packed time to a structure

ARGS:  > time
           The time packed according to DOS rules
           (see int 21h - function 57h, Subfunction 00h)
           bits 15-11: hours (0-23)
                10-5:  minutes
                4-0:   seconds/2

       < sTime
           Pointer to the structure to receive the unpacked time:

           typedef
           {
             nuint8 seconds;
             nuint8 minutes;
             nuint16 hours;
           } NW_TIME;

INCLUDE: nwmisc.h

RETURN:  n/a

CLIENT:  DOS WIN OS2 NT

SEE:     NWPackDate, NWPackDateTime, NWUnpackTime, NWUnpackDate,
         NWUnpackDateTime

CHANGES: 10 Nov 1992 - written - jwoodbur
         13 Sep 1993 - NWNCP not used (hungarian added) - dromrell
****************************************************************************/
nuint16 N_API NWPackTime
(
  NW_TIME NWPTR sTime
)
{
  nuint16 time;

  if(sTime)
  {
    time = sTime->hours;
    time <<= 6;
    time |= (nuint16)(sTime->minutes & 0x3f);
    time <<= 5;
    time |= (nuint16)((sTime->seconds/2) & 0x1f);
  }
  else
    return 0;

  return time;
}

/* the following functions are aliases for old functionality. The
   function names were changed to be consistant and these were kept around
   in case anyone had already been using them
 */

void N_API NWConvertDate
(
  nuint16         date,
  NW_DATE NWPTR   sDate
)
{
  NWUnpackDate(date, sDate);
}

void N_API NWConvertTime
(
  nuint16         time,
  NW_TIME NWPTR   sTime
)
{
  NWUnpackTime(time, sTime);
}

void N_API NWConvertDateTime
(
  nuint32         dateTime,
  NW_DATE NWPTR   sDate,
  NW_TIME NWPTR   sTime
)
{
  NWUnpackDateTime(dateTime, sDate, sTime);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/datetime.c,v 1.6 1994/06/08 23:08:35 rebekah Exp $
*/
