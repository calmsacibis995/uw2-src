/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:cleanpth.c	1.6"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwerror.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"

#ifdef N_PLAT_UNIX

/* Static Functions */
N_INTERN_FUNC_C (nuint8) CleanPartPath( pnstr8 );

#endif

/*manpage*CleanPath*********************************************************
SYNTAX:  NWCCODE N_API CleanPath
         (
            pnstr8 pbstrPath
         )

REMARKS: Cleans up/resolves dots in path

ARGS:

INCLUDE: nwintern.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 10 Jul 1992 - rewrote - jwoodbur
            Now works properly with multiple slashes (except for ones
            preceeding volume) and handles dots correctly.
         20 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API CleanPath
(
   pnstr8   pbstrPath
)
#ifdef N_PLAT_UNIX
{
    pnstr8 basePtr = pbstrPath;
    nuint16 i = 0;

    for( ; *basePtr != '\0'; basePtr = (pnstr8)NWNextChar(basePtr))
    {
        if(*basePtr == '/')
            *basePtr = '\\';
    }

    basePtr = pbstrPath;

    while(*basePtr && (*basePtr != ':'))
    {                       /* find volume end */
        i++;
        basePtr = (pnstr8)NWNextChar(basePtr);
    }
    if(*basePtr == ':')
        basePtr = (pnstr8)NWNextChar(basePtr);
    else
        basePtr = pbstrPath;

    if(*basePtr == '\\')
        basePtr = (pnstr8)NWNextChar(basePtr);        /* skip leading slash */
    return(CleanPartPath(basePtr));
}
#else
{
   pnstr8   pbstrSrcPtr, pbstrDstPtr, pbstrTrueBasePtr;
   nint8    abTempPath[260];
   nuint16  suNumDots, suClusterFlag;

   if(pbstrPath == NULL)
      return (INVALID_PATH);

   /* convert all slashes to back-slashes */

   for(pbstrSrcPtr = pbstrPath; *pbstrSrcPtr; pbstrSrcPtr = NWNextChar(pbstrSrcPtr))
      if(*pbstrSrcPtr == '/')
         *pbstrSrcPtr = '\\';

   pbstrSrcPtr = pbstrPath;

   /* Start after the volume or drive letter */

   while(*pbstrSrcPtr && (*pbstrSrcPtr != ':')) /* find drive or vol end          */
      pbstrSrcPtr = NWNextChar(pbstrSrcPtr);

   if(*pbstrSrcPtr == ':')
      pbstrTrueBasePtr = pbstrSrcPtr + 1;
   else
      pbstrTrueBasePtr = pbstrPath;

   abTempPath[0]   = ' ';
   NWCStrCpy(&abTempPath[1], pbstrTrueBasePtr);

   pbstrSrcPtr = &abTempPath[1];
   suClusterFlag = 1;

   while(*pbstrSrcPtr)
   {
      if(*pbstrSrcPtr == '\\')
      {
         pbstrDstPtr = pbstrSrcPtr + 1;
         do
         {
            pbstrSrcPtr++;
         } while(*pbstrSrcPtr == '\\');

         if(*pbstrSrcPtr == '\0')
         *(--pbstrDstPtr) = '\0';
         else if(pbstrSrcPtr != pbstrDstPtr)
         NWCStrCpy(pbstrDstPtr, pbstrSrcPtr);

         pbstrSrcPtr = pbstrDstPtr;
         suClusterFlag = 1;
         continue;
      }
      else if(suClusterFlag && *pbstrSrcPtr == '.')
      {
         pbstrDstPtr = pbstrSrcPtr++;
         suNumDots = (*pbstrSrcPtr == '.') ? 1 : 0;

         while(*pbstrSrcPtr == '.')
         {
            pbstrSrcPtr++;
            suNumDots++;
         }
         if(*pbstrSrcPtr == '\\' || *pbstrSrcPtr == '\0')
         {
            while(*pbstrSrcPtr == '\\')
               pbstrSrcPtr++;

            while(suNumDots)
            {
               pbstrDstPtr = NWPrevChar(abTempPath, pbstrDstPtr);
               if(pbstrDstPtr == abTempPath)
                  return (INVALID_PATH);

               if(*pbstrDstPtr == '\\' || pbstrDstPtr == &abTempPath[1])
                  suNumDots--;
            }
            if(*pbstrDstPtr == '\\')
               pbstrDstPtr++;
            NWCStrCpy(pbstrDstPtr, pbstrSrcPtr);
            pbstrSrcPtr = pbstrDstPtr;
            continue;
         }
      }

      suClusterFlag = 0;
      pbstrSrcPtr = NWNextChar(pbstrSrcPtr);
   }
   pbstrSrcPtr = NWPrevChar(abTempPath, pbstrSrcPtr);
   if(*pbstrSrcPtr == '\\' && pbstrSrcPtr != &abTempPath[1])
      *pbstrSrcPtr = '\0';

   NWCStrCpy(pbstrTrueBasePtr, &abTempPath[1]);
   return (0);
}
#endif

#ifdef N_PLAT_UNIX
N_INTERN_FUNC_C (nuint8)
CleanPartPath ( pnstr8 pbstrPath )
{
    pnstr8 nextCharPtr = pbstrPath;
    pnstr8 basePtr = pbstrPath;
    nuint16 i;

    if(*basePtr == '.')
    {
        basePtr = (pnstr8)NWNextChar(basePtr);
        if(*basePtr == '.')
            return((nuint8)INVALID_PATH);

        if(*basePtr == '\0')
        {
            *pbstrPath = 0;
            return( 0 );
        }
    }

    while(*basePtr != '\0')
    {
        if(*basePtr == '\\')
        {
            nextCharPtr = basePtr;
            nextCharPtr = (pnstr8)NWNextChar(nextCharPtr);
            if(*nextCharPtr == '.')
            {
                /*
                 Count the number of dots starting with the second one.
                */
                for((nextCharPtr = (pnstr8)NWNextChar(nextCharPtr)), i=0;
                    *nextCharPtr == '.';
                    (nextCharPtr = (pnstr8)NWNextChar(nextCharPtr)), i++);

                /*
                  Now backup the number of directories indicated by the count
                */
                for(;;)
                {
                    basePtr = NWPrevChar(pbstrPath, basePtr);
                    if((*basePtr == '\\') || (basePtr == pbstrPath))
                    {
                        if(--i == 0)
                            break;
                        else if(basePtr == pbstrPath)
                            return((nuint8)INVALID_PATH);
                    }
                }
                (void) NWCStrCpy((char *)basePtr, (char *)nextCharPtr);
            } else
                basePtr = (pnstr8)NWNextChar(basePtr);
        } else
            basePtr = (pnstr8)NWNextChar(basePtr);
    }

    while(*pbstrPath == '\\')
        (void) NWCStrCpy((char *)pbstrPath, (char *)&pbstrPath[1]);
					/* get rid of leadin g slash */

    if(*pbstrPath == 0)                  /* if NULL string, return */
        return( 0 );

    basePtr = &pbstrPath[NWCStrLen((char *)pbstrPath)];

	/* start of last character */
    basePtr = NWPrevChar(pbstrPath, basePtr);

    while(*basePtr == '\\')
    {
        *basePtr = 0;               /* remove trailing slashes */
        basePtr = NWPrevChar(pbstrPath, basePtr);
    }

    return( 0 );
}
#endif


/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/cleanpth.c,v 1.9 1994/09/26 17:44:23 rebekah Exp $
*/

