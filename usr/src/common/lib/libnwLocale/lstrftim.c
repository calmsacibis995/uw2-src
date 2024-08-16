/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:lstrftim.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrftim.c,v 1.1 1994/09/26 17:21:01 rebekah Exp $"
/*========================================================================
  ==
  ==  file     : LSTRFTIM.C
  ==
  ==  routine  : NWLstrftime
  ==
  ==  author   : Phil Karren
  ==
  ==  date     : 21 June 1989
  ==
  ==  comments : Format date and time information based on locale.
  ==             This is an ANSII standard routine.
  ==
  ==  modifications :
  ==     Allan Neill - March 27, 1991
  ==        Fixed a few bugs
  ==     William E. Kline (WEK) - 06/04/91
  ==        Removed string.h
  ========================================================================*/
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#if ((defined N_PLAT_MSW && defined N_ARCH_32) || defined N_PLAT_UNIX || defined N_PLAT_NLM)
#define _fstrcpy strcpy
#define _fstrcat strcat
#define _fstrlen strlen
#endif

#if (defined WIN32 || defined N_PLAT_UNIX || N_PLAT_NLM)
#define _fmemset memset
#endif

#if (defined N_PLAT_MSW)
# include <windows.h>
#endif

#define NWL_EXCLUDE_TIME 1
#define NWL_EXCLUDE_FILE 1

#include <string.h>
#include <time.h>
#include "ntypes.h"
#include "nwlocale.h"
#include "enable.h"
#define MAX_DATE_LEN 50
#define MAXARGS 10


#define  putf(s,d,i) if(d >= 10){ \
                        s[i++] = (char) ((d / 10) + '0'); \
                        s[i++] = (char) ((d % 10) + '0');} \
                     else s[i++] = (char) (d + '0')

#define  put(s,d,i)  if(d >= 10){ \
                        s[i++] = (char) ((d / 10) + '0'); \
                        s[i++] = (char) ((d % 10) + '0');} \
                     else { \
                        s[i++] = (char) '0'; \
                        s[i++] = (char) (d + '0');} \

/****************************************************************************
 * Internal prototypes
 ****************************************************************************/

#if defined N_PLAT_MSW && defined N_ARCH_16 

static int  NEAR _NWFormatString(char N_FAR *,
                                 size_t,
                                 char N_FAR *,
                                 int, int, int, int );
static PSTR NEAR GetMem (WORD);
static VOID NEAR FreeMem (PSTR);

#else

int N_API _NWFormatString(
               char N_FAR *string,
					size_t maxSize,
               char N_FAR *separator,
               int order,
               int strseg1,
               int strseg2,
               int strseg3);

#endif

/****************************************************************************
 * Exported Function
 ****************************************************************************/

#if defined N_PLAT_MSW && defined N_ARCH_16

size_t N_API NWLstrftime(
   char N_FAR *string,
   size_t maxSize,
   char N_FAR *format,
   struct tm N_FAR *timePtr)
{
   /* Only the following parameters have been implemented:
         %x - date
         %X - time
         %c - date and time
   */

   BYTE far *cursor;
   size_t hour, length;
   BYTE *outString;
   BYTE *tempStr;

   /* No format string is a no-no. */

   if (maxSize <= 0 || format == (char N_FAR *) NULL)
      return 0;

   /*
    * Allocate memory for constructing the formatted time string.
    */

	maxSize = (maxSize > MAX_DATE_LEN) ? maxSize : MAX_DATE_LEN;

   outString = GetMem (maxSize);
   if (outString == NULL)
      return (0);

   tempStr = GetMem (maxSize);
   if (tempStr == NULL) {
      FreeMem ((PSTR) outString);
      return (0);
   }


   timePtr->tm_year = timePtr->tm_year % 100;

   *outString = '\0';

   /* Move through the format string and process the format requests. */

   for ( cursor = (char N_FAR *) format;
   cursor != (char N_FAR *) NULL && *cursor &&
   ((size_t) lstrlen ((LPSTR) outString) <= maxSize); )
   {
       cursor = NWLstrchr (cursor, '%');

      /*
       * If cursor is not NULL, then the character is '%', which is
       * single-byte, so increment by one byte to point to the formatting
       * character
       */

      if (cursor != (char N_FAR *) NULL && *cursor)
      {
         cursor = AnsiNext (cursor);

         if ( *cursor == (BYTE) 'x' || *cursor == (BYTE) 'c' )
         {
            _NWFormatString ( (LPSTR) tempStr, maxSize,
                              localeInfo._dateSeparator,
                              localeInfo._dateFormat, timePtr->tm_mon+1,
                              timePtr->tm_mday, timePtr->tm_year );

            length = lstrlen ((LPSTR) outString) + lstrlen ((LPSTR) tempStr);

            if (length <= maxSize)
            {
               lstrcat ((LPSTR) outString, (LPSTR) tempStr);
               if (*cursor == (BYTE) 'c' && length < maxSize)
                  lstrcat ((LPSTR) outString, (LPSTR) " ");
            }
            else
               break;
         }

         if ( *cursor == (BYTE) 'X' || *cursor == (BYTE) 'c')
         {
            /*
             * Account for 12-hour clocks versus 24-hour clocks by using
             * modulo 12 arithmetic.
             */
            hour = (localeInfo._timeFormat == 0) ?
                     ((timePtr->tm_hour == 0) ? 12 :
                        (timePtr->tm_hour-1) % 12 + 1) : timePtr->tm_hour;
            _NWFormatString ( (LPSTR) tempStr, maxSize,
                              localeInfo._timeSeparator,
                              0, hour, timePtr->tm_min, timePtr->tm_sec );
            length = lstrlen ((LPSTR) outString) + lstrlen ((LPSTR) tempStr);
            if (length <= maxSize + 2)
               lstrcat ((LPSTR) outString, (LPSTR) tempStr);
            else
               break;

            if (localeInfo._timeFormat == 0)
            {
					lstrcpy((LPSTR) tempStr, " " ); /* put space between number and AM or PM */
               if (12 <= timePtr->tm_hour && timePtr->tm_hour <= 23)
                  lstrcat ((LPSTR) tempStr, localeInfo._pm);
               else
                  lstrcat ((LPSTR) tempStr, localeInfo._am);

               if ((size_t) lstrlen ((LPSTR) outString) +
                  ((size_t) lstrlen ((LPSTR) tempStr)) <= maxSize)
               {
                  lstrcat ((LPSTR) outString, (LPSTR) tempStr);
               }
               else
                  *cursor = (char) NULL;
            }
         }
      }
   } /* end for () */

   /*
    * Copy the string into the user's buffer if there is one and free up
    * memory. Return the string length.
    */

   if (string)
      NWstrncpy ( (LPSTR) string, (LPSTR) outString, maxSize - 1);

   length = lstrlen ( (LPSTR) outString );

   FreeMem ( (PSTR) outString );
   FreeMem ( (PSTR) tempStr );

   return (length);
}

/****************************************************************************
 * Static Functions
 ****************************************************************************/

static PSTR NEAR GetMem (wSize)
WORD wSize;

{
   LOCALHANDLE hMem;

   /* Allocate some memory off of the local heap. */

   hMem = LocalAlloc (LMEM_MOVEABLE | LMEM_ZEROINIT, wSize);
   return ( (hMem == (LOCALHANDLE) 0) ? (PSTR) NULL :
                                        LocalLock (hMem) );

} /* GetMem() */

static VOID NEAR FreeMem (pMem)
PSTR pMem;

{
   LOCALHANDLE hMem;

   /* Get the handle and free the memory block. */

   hMem = LocalHandle ((WORD) pMem);
   LocalUnlock (hMem);
   LocalFree (hMem);

} /* FreeMem() */

#else /* N_PLAT_DOS and N_PLAT_OS2 */

size_t N_API NWLstrftime(
   char N_FAR *string,
   size_t maxSize,
   char N_FAR *format,
   struct tm N_FAR *timePtr)
{
   /* Only the following parameters have been implemented:
         %x - date
         %X - time
         %c - date and time
   */

/*   unsigned char N_FAR *cursor;
   unsigned char outString[MAX_DATE_LEN];
   unsigned char tempStr[MAX_DATE_LEN];
   size_t hour, length;
*/
   char N_FAR *cursor;
   char outString[MAX_DATE_LEN];
   char tempStr[MAX_DATE_LEN];
   size_t hour, length;

	/* If the user passed a return string, insert the information otherwise,
      allocate a string and put the data in it.  This allows the function
      to return the output length whether a string was passed or not */


   /* No format string is a no-no. */

   if (maxSize <= 0 || format == (char N_FAR *) NULL)
      return 0;

	maxSize = (maxSize > MAX_DATE_LEN) ? maxSize : MAX_DATE_LEN;

   timePtr->tm_year = timePtr->tm_year % 100;

   *outString = '\0';

   /* Move through the format string and process the format requests. */
   for (cursor = (char N_FAR *) format;
   cursor != (char N_FAR *) NULL && *cursor &&
   (_fstrlen(outString) <= maxSize); )
   {
         cursor = NWLstrchr(cursor, '%');
      /* If cursor is not NULL, then the character is '%', which is single-
         byte, so increment by one byte to point to the formatting character */

      if (cursor != (char N_FAR *) NULL && *cursor)
      {
         cursor = NWNextChar(cursor);

         if( *cursor == (unsigned char) 'x'
            || *cursor == (unsigned char) 'c')
         {
            _NWFormatString (tempStr, MAX_DATE_LEN, localeInfo._dateSeparator,
                              localeInfo._dateFormat, timePtr->tm_mon+1,
                              timePtr->tm_mday, timePtr->tm_year);

            length = _fstrlen(outString) + _fstrlen(tempStr);

            if (length <= maxSize)
            {
               _fstrcat(outString, tempStr);
               if(*cursor == (unsigned char) 'c' && length < maxSize)
               _fstrcat(outString, " ");
            }
            else
               break;
         }

         if( *cursor == (unsigned char) 'X'
            || *cursor == (unsigned char) 'c')
         {
            /* Account for 12-hour clocks versus 24-hour clocks
               by using modulo 12 arithmetic. */
            hour = localeInfo._timeFormat == 0 ? (timePtr->tm_hour
               == 0 ? 12 : (timePtr->tm_hour-1) % 12 + 1) :
            timePtr->tm_hour;
            _NWFormatString (tempStr, MAX_DATE_LEN, localeInfo._timeSeparator,
                              0, hour, timePtr->tm_min, timePtr->tm_sec);
            length = _fstrlen(outString) + _fstrlen(tempStr);
            if (length <= maxSize + 2)
               _fstrcat(outString, tempStr);
            else
               break;

            if (localeInfo._timeFormat == 0)
            {
					_fstrcpy(tempStr, " " ); /* put space between number and AM or PM */
               if (12 <= timePtr->tm_hour && timePtr->tm_hour <= 23)
                  _fstrcat(tempStr, localeInfo._pm);
               else
                  _fstrcat(tempStr, localeInfo._am);

               if (_fstrlen(outString) + _fstrlen(tempStr) <= maxSize)
                  _fstrcat(outString, tempStr);
               else
                  *cursor = (char) NULL;
            }
         }
      }
   }

	length = _fstrlen(outString);
   if (length > maxSize)
   {
      length = maxSize;
      outString[length] = 0;
   }

	if (string)
      NWstrncpy (string, outString, length+1);

   return(length);
}

#endif

/****************************************************************************
 * Static Functions
 ****************************************************************************/

#if defined N_PLAT_MSW && defined N_ARCH_16
static int NEAR _NWFormatString(
#else
int N_API _NWFormatString(
#endif
   char N_FAR *string,
   size_t maxSize,
   char N_FAR *separator,
   int order,
   int strseg1,
   int strseg2,
   int strseg3)
{
   int i = 0;

   _fmemset(string, (unsigned char) 0x00, maxSize);

   i = 0;
   if (*separator == '\0')
      _fstrcpy(separator,"/");

   switch (order)
   {
      case 0:        /* MDY date format.  Formatted time is always H:M:S */
         putf(string, strseg1, i);
         _fstrcat(string, separator);
         i += _fstrlen(&string[i]);
         put(string, strseg2, i)
         _fstrcat(string,separator);
         i += _fstrlen(&string[i]);
         put(string, strseg3, i)
         break;

      case 1:    /* DMY date format */
         putf(string, strseg2, i);
         _fstrcat(string,separator);
         i += _fstrlen(&string[i]);
         put(string, strseg1, i)
         _fstrcat(string,separator);
         i += _fstrlen(&string[i]);
         put(string, strseg3, i);
         break;

      case 2: /* YMD date format */
         putf(string, strseg3, i);
         _fstrcat(string,separator);
         i += _fstrlen(&string[i]);
         put(string, strseg1, i)
         _fstrcat(string,separator);
         i += _fstrlen(&string[i]);
         put(string, strseg2, i);
         break;
   }

   return 0;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/lstrftim.c,v 1.1 1994/09/26 17:21:01 rebekah Exp $
*/
