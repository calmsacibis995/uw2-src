/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:indexpth.c	1.5"
#include "ntypes.h"
#include "nwcaldef.h"
#include "nwintern.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwclient.h"
#include "nwclocal.h"
#include "nwlocale.h"

#define IsSlash(c)  (c == '/' || c == '\\')
pnstr8 N_API __LStrChr
(
   pnstr8 path,
   nstr8 c
);

/*manpage*IndexPath*********************************************************
SYNTAX:  NWCCODE N_API IndexPath
         (
            pnstr8   pbstrPath,
            pnuint16 psuStartServer,
            pnuint16 psuServerLen,
            pnuint16 psuStartVolume,
            pnuint16 psuVolumeLen,
            pnuint16 psuStartPath
         )

REMARKS: Splits path into parts by putting indexes of the start of
         servername, volume name, and path into areas pointed to by
         parameters.

         This function works on either UNC or NetWare paths

ARGS: >  pbstrPath
      <  psuStartServer
      <  psuServerLen
      <  psuStartVolume
      <  psuVolumeLen
      <  psuStartPath

INCLUDE: nwintern.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 20 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API IndexPath
(
   pnstr8   pbstrPath,
   pnuint16 psuStartServer,
   pnuint16 psuServerLen,
   pnuint16 psuStartVolume,
   pnuint16 psuVolumeLen,
   pnuint16 psuStartPath
)
#ifdef N_PLAT_UNIX

{
    pnstr8 basePtr;
    pnstr8 nextPtr;


/*
**  Check for no path sent in
*/
    if(!pbstrPath)
    {
        return(0x23);
    }

/*
**  Make all '/' into '\\'
*/
    for(nextPtr = pbstrPath; *nextPtr != '\0'; nextPtr = NWNextChar(nextPtr))
    {
        if(*nextPtr == '/')
            *nextPtr = '\\';
    }

    *psuStartServer = *psuVolumeLen = *psuServerLen =
					*psuStartVolume = *psuStartPath = 0;

/*
**  Check for UNC type path
*/
    if(IsSlash(*pbstrPath) && IsSlash(*(pbstrPath+1)))
    {

/*
**  Skip the leading slashes
*/
        *psuStartServer = 2;
        nextPtr = basePtr = pbstrPath+2;

/*
**  Find where the volume is
*/
        while(*basePtr != 0 && !IsSlash(*basePtr) && *basePtr != ':')
            basePtr = NWNextChar(basePtr);

/*
**  If no slash found, this is an invalid UNC path
*/
        if(*basePtr == 0 || *basePtr == ':')
            return(0x34);

        *psuServerLen = (nuint16)(basePtr - nextPtr);
        basePtr = NWNextChar(basePtr);
        *psuStartVolume = (nuint16)(basePtr - pbstrPath);

        nextPtr = basePtr;

/*
**  Look for the end of the volume
*/
        while(*basePtr != 0 && !IsSlash(*basePtr) && *basePtr != ':')
            basePtr = NWNextChar(basePtr);

/*
**  UNC style names do not allow colons
*/
        if(*basePtr == ':')
            return(0x45);

        *psuVolumeLen = (nuint16)(basePtr - nextPtr);

/*
**  Look for the rest of the path
*/
        if(*basePtr != 0)
            basePtr = NWNextChar(basePtr);

        *psuStartPath = (nuint16)(basePtr - pbstrPath);

    }
    else
    {
        /*
         ** Assume now a NetWare path
         ** NetWare path will have a slash before a colon
         */
        basePtr = (pnstr8)strchr((char *)pbstrPath, '\\');
        nextPtr = (pnstr8)strchr((char *)pbstrPath, ':');

        /*
         ** If we have a colon, there is the possibility of server\vol:path,
         ** or vol:path, or drive:path.  In NWU, drive:path is invalid
         */
        if( nextPtr )
        {
            if( (basePtr - pbstrPath) == 1 )
                return(0x56);

            if( basePtr )
            {
                if(nextPtr > basePtr)    /* colon after slash */
                {
                    *psuServerLen = (nuint16)(basePtr - pbstrPath);
                    *psuStartVolume = *psuServerLen+1;
                    *psuVolumeLen = (nuint16)(nextPtr - basePtr - 1);
                    *psuStartPath = *psuServerLen + *psuVolumeLen + 2;
                }
                else    /* if / after :, assume a volume */
                {
                    *psuVolumeLen = (nuint16)(nextPtr - pbstrPath);
                    nextPtr++;  /* move past the ':' */
                    *psuStartPath = *psuVolumeLen+1;
                }
            }
            else    /* just a vol:path */
            {
                *psuVolumeLen = (nuint16)(nextPtr - pbstrPath);
                nextPtr++;
                *psuStartPath = *psuVolumeLen+1;
            }
        }
    }
    return( 0 );
}
#else
{
   pnstr8 pbstrBasePtr;
   pnstr8 nextPtr;

   if(!pbstrPath)
      return ((NWCCODE) 0x2345);           /* invalid path    */

   for(nextPtr = pbstrPath; *nextPtr != '\0'; nextPtr = NWNextChar(nextPtr))
      if(*nextPtr == '/')           /* make all '/' into back slashes */
         *nextPtr = '\\';

   *psuStartServer = *psuVolumeLen = *psuServerLen = *psuStartVolume =
      *psuStartPath = 0;

   if(IsSlash(*pbstrPath) && IsSlash(*(pbstrPath+1)))
   {
      /* This is a UNC style path */
      *psuStartServer = 2;
      nextPtr = pbstrBasePtr = pbstrPath+2; /* Skip the leading slashes */

      /* Let's find the volume */
      while(*pbstrBasePtr != 0 && !IsSlash(*pbstrBasePtr) &&
         *pbstrBasePtr != ':')
      {
         pbstrBasePtr = NWNextChar(pbstrBasePtr);
      }

      /* if we didn't find a slash, this is an invalid UNC path */
      if(*pbstrBasePtr == 0 || *pbstrBasePtr == ':')
         return ((NWCCODE) 0x3456);

      *psuServerLen =(nuint16)(pbstrBasePtr - nextPtr);
      pbstrBasePtr = NWNextChar(pbstrBasePtr);
      *psuStartVolume = (nuint16)(pbstrBasePtr - pbstrPath);

      nextPtr = pbstrBasePtr;        /* save to calculate volume length*/

      /* Now let's look for the end of the volume */
      while(*pbstrBasePtr != 0 && !IsSlash(*pbstrBasePtr) &&
         *pbstrBasePtr != ':')
      {
         pbstrBasePtr = NWNextChar(pbstrBasePtr);
      }

      /* UNC style names do not allow colons */
      if(*pbstrBasePtr == ':')
         return ((NWCCODE) 0x3456);

      *psuVolumeLen = (nuint16)(pbstrBasePtr - nextPtr);

      /* let's look for the rest of the path */
      if(*pbstrBasePtr != 0)
         pbstrBasePtr = NWNextChar(pbstrBasePtr);

      *psuStartPath = (nuint16)(pbstrBasePtr - pbstrPath);
   }
   else  /* let's assume we have a NetWare path */
   {
      /* A NetWare path will have a slash before the colon */

      pbstrBasePtr = __LStrChr(pbstrPath,'\\');
      nextPtr = __LStrChr(pbstrPath,':');

      /*
         if we have a colon, there is the possibility of server\vol:path, or
         vol:path, or drive:path.  (drive:vol:path not valid anymore?).
         Otherwise, we would just have a straigh path, so return 0 for
         everything.
      */
      if (nextPtr)
      {
         if ((pbstrBasePtr - pbstrPath) == 1) /* if drive: */
            if (*(pbstrBasePtr+1))
            {
               *psuStartPath = 2;
               return ((NWCCODE) 0);
            }

         /* check if we also have a slash, possibility of server\vol: */
         if (pbstrBasePtr)
         {
            if (nextPtr > pbstrBasePtr)    /* if colon is after slash */
            {
                  /* first check for a drive:\ */
               *psuServerLen = (nuint16) (pbstrBasePtr - pbstrPath);
               *psuStartVolume = *psuServerLen+1;
               *psuVolumeLen = (nuint16) (nextPtr - pbstrBasePtr - 1);
               *psuStartPath = *psuServerLen + *psuVolumeLen + 2;
            }
            else    /* if slash after colon, we assume a volume (or drive)*/
            {
               *psuVolumeLen = (nuint16) (nextPtr - pbstrPath);
               nextPtr++;      /* move past the ':' (no need for DBCS) */
               *psuStartPath = *psuVolumeLen + 1;
            }
         }
         else  /* just a vol:path */
         {
            *psuVolumeLen = (nuint16) (nextPtr - pbstrPath);
            nextPtr++;      /* move past the ':' (no need for DBCS) */
            *psuStartPath = *psuVolumeLen + 1;
         }
      }
   }
   return ((NWCCODE) 0);
}
#endif

pnstr8 N_API __LStrChr
(
   pnstr8 pbstrPath,
   nstr8  c
)
{
   pnstr8 pbstrTmp;

   if(!pbstrPath)
      return (NULL);

   for(pbstrTmp = pbstrPath;
         pbstrTmp && (*pbstrTmp != (nstr) 0) && (*pbstrTmp != c);
         pbstrTmp = NWNextChar(pbstrTmp));

   /* At this point, we either found it or not */
   if(*pbstrTmp == c)
      return (pbstrTmp);
   else
      return (NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/indexpth.c,v 1.7 1994/09/26 17:47:38 rebekah Exp $
*/
