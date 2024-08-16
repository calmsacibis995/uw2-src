/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:comppath.c	1.5"
#if defined N_PLAT_MSW
#include <windows.h>
#endif
#include "ntypes.h"
#include "unicode.h"
#include "nwclient.h"
#include "nwncp.h"
#include "ncpfile.h"
#include <string.h>
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "localias.h"
#include "nwlocale.h"

/*manpage*NWNCPPackCompPath*************************************************
SYNTAX:  N_EXTERN_LIBRARY( nuint16 )
         NWNCPPackCompPath
         (
            nint           iPathLen,
            pnstr          pstrPath,
            nint           iNamSpc,
            pNWNCPCompPath pCompPath,
            nflag32        flOptions
         )

REMARKS:

ARGS: >  iPathLen
      <  pstrPath
      <  pCompPath
      >  flOptions

INCLUDE: ncpfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
            Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( nuint16 )
NWNCPPackCompPath
(
   nint           iPathLen,
   pnstr          pstrPath,
   nint           iNamSpc,
   pNWNCPCompPath pCompPath,
   nflag32        flOptions
)
{
   if(iPathLen == -1 && pstrPath)
      iPathLen = nwstrlen(pstrPath);

   if(iNamSpc == -1)
   {
      iNamSpc = 0;
      /* put call to NWGetCurNS here when done in NWClient */
   }
   pCompPath->buNamSpc = (nuint8) iNamSpc;

   if (pCompPath->buHandleFlag == NWNCP_COMPPATH_USE_DIRHANDLE ||
       pCompPath->buHandleFlag == NWNCP_COMPPATH_NO_HANDLE)
   {
      pCompPath->buVolNum = (nuint8) 0x00;
   }

   pCompPath->abuPacked[0] = pCompPath->buVolNum;
   NCopyLoHi32(&pCompPath->abuPacked[1], &pCompPath->luDirBase);
   if(!pCompPath->luDirBase && !pCompPath->buHandleFlag)
      pCompPath->buHandleFlag = (nuint8) 0xff;

   pCompPath->abuPacked[5] = pCompPath->buHandleFlag;

   if(pstrPath == NULL)
   {
      pCompPath->buCompCnt = (nuint8) 0;
      pCompPath->suPackedLen  = (nuint16) 7;
   }
   else
   {
      NWNCPMakeACompPath(iPathLen,
                         pstrPath,
                         iNamSpc,
                         &pCompPath->buCompCnt,
                         &pCompPath->abuPacked[7],
                         &pCompPath->suPackedLen,
                         flOptions);

      pCompPath->suPackedLen += (nuint16) 7;
   }

   pCompPath->abuPacked[6] = (nuint8) pCompPath->buCompCnt;

   return(pCompPath->suPackedLen);
}

/*manpage*NWNCPPackCompPath2************************************************
SYNTAX:  N_EXTERN_LIBRARY( nuint16 )
         NWNCPPackCompPath2
         (
            nint              iSrcPathLen,
            pnstr             pstrSrcPath,
            nint              iSrcNamSpc,
            nint              iDstPathLen,
            pnstr             pstrDstPath,
            nint              iDstNamSpc,
            pNWNCPCompPath2   pCompPath2
         )

REMARKS:

ARGS: >  buSrcPathLen
      <  pbuSrcPath
      >  buDstPathLen
      <  pbuDstPath
      <  pCompPath2

INCLUDE: ncpfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( nuint16 )
NWNCPPackCompPath2
(
   nint              iSrcPathLen,
   pnstr             pstrSrcPath,
   nint              iSrcNamSpc,
   nint              iDstPathLen,
   pnstr             pstrDstPath,
   nint              iDstNamSpc,
   pNWNCPCompPath2   pCompPath2,
   nflag32           flOptions
)
{
   nuint16 suDstPackedLen;


   if(iSrcPathLen == -1 && pstrSrcPath)
      iSrcPathLen = nwstrlen(pstrSrcPath);

   if(iDstPathLen == -1 && pstrDstPath)
      iDstPathLen = nwstrlen(pstrDstPath);

   if(iSrcNamSpc == -1)
   {
      iSrcNamSpc = 0;
   }
   pCompPath2->buSrcNamSpc = (nuint8) iSrcNamSpc;

   if(iDstNamSpc == -1)
   {
      iDstNamSpc = 0;
   }
   pCompPath2->buDstNamSpc = (nuint8) iDstNamSpc;

   if (pCompPath2->buSrcHandleFlag == NWNCP_COMPPATH_USE_DIRHANDLE ||
       pCompPath2->buSrcHandleFlag == NWNCP_COMPPATH_NO_HANDLE)
   {
      pCompPath2->buSrcVolNum = (nuint8) 0x00;
   }

   pCompPath2->abuPacked[0] =  pCompPath2->buSrcVolNum;
   NCopyLoHi32(&pCompPath2->abuPacked[1], &pCompPath2->luSrcDirBase);
   if(!pCompPath2->luSrcDirBase && !pCompPath2->buSrcHandleFlag)
      pCompPath2->buSrcHandleFlag = (nuint8) 0xff;

   pCompPath2->abuPacked[5] =  pCompPath2->buSrcHandleFlag;

   if(pstrSrcPath == NULL)
   {
      pCompPath2->buSrcCompCnt = (nuint8) 0;
      pCompPath2->suPackedLen  = (nuint16) 14;
   }
   else
   {
      NWNCPMakeACompPath(iSrcPathLen,
                         pstrSrcPath,
                         iSrcNamSpc,
                         &pCompPath2->buSrcCompCnt,
                         &pCompPath2->abuPacked[14],
                         &pCompPath2->suPackedLen,
                         flOptions);

      pCompPath2->suPackedLen += (nuint16) 14;
   }
   pCompPath2->abuPacked[6] = pCompPath2->buSrcCompCnt;

   if (pCompPath2->buDstHandleFlag == NWNCP_COMPPATH_USE_DIRHANDLE ||
       pCompPath2->buDstHandleFlag == NWNCP_COMPPATH_NO_HANDLE)
   {
      pCompPath2->buDstVolNum = (nuint8) 0x00;
   }

   pCompPath2->abuPacked[7] = pCompPath2->buDstVolNum;
   NCopyLoHi32(&pCompPath2->abuPacked[8], &pCompPath2->luDstDirBase);
   if(!pCompPath2->luDstDirBase && !pCompPath2->buDstHandleFlag)
      pCompPath2->buDstHandleFlag = (nuint8) 0xff;

   pCompPath2->abuPacked[12] = pCompPath2->buDstHandleFlag;

   if(pstrDstPath == NULL)
   {
      pCompPath2->buDstCompCnt = (nuint8) 0;
      suDstPackedLen = 7;
   }
   else
   {
      NWNCPMakeACompPath(iDstPathLen,
                         pstrDstPath,
                         iDstNamSpc,
                         &pCompPath2->buDstCompCnt,
                         &pCompPath2->abuPacked[pCompPath2->suPackedLen],
                         &suDstPackedLen,
                         flOptions);
   }

   pCompPath2->suPackedLen += (nuint16) suDstPackedLen;
   pCompPath2->abuPacked[13] = pCompPath2->buDstCompCnt;

   return(pCompPath2->suPackedLen);
}

/*manpage*NWNCPMakeACompPath************************************************
SYNTAX:  N_EXTERN_LIBRARY( void )
         NWNCPMakeACompPath
         (
            nint     iPathlen,
            nptr     pInPath,
            nint     iNamSpc,
            pnuint8  pbuCompCnt,
            pnuint8  pabuCompPath,
            pnuint16 psuLen
         )

REMARKS:

ARGS: >  iPathLen
      >  pInPath
      >  iNamSpc
      <  pbuCompCnt (optional)
      <  pabuCompPath
      <  psuLen     (optional)

INCLUDE: ncpfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
            Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( void )
NWNCPMakeACompPath
(
   nint     iPathLen,
   nptr     pInPath,
   nint     iNamSpc,
   pnuint8  pbuCompCnt,
   pnuint8  pabuCompPath,
   pnuint16 psuLen,
   nflag32  flOptions
)
{
   pnuint8 pbuCompLen, pbuPath = (pnuint8) pInPath;
   nint iCompLen = 0, iNumComps = 0;
   nint iCharSize, iChar = 0, iChar2 = 0, iNewChar = 0;
   nuint16 suLen;


   iCharSize = (iNamSpc == NWNCP_NS_UNICODE) ? 2 : 1;

   suLen = (nuint16) 0;
   pbuCompLen = pabuCompPath++;

   while(iPathLen--)
   {
      if(iCharSize == 1)
         iChar = *pbuPath++;
      else
      {
         NCopy16(&iChar, pbuPath);
         pbuPath += 2;
      }

      if(!iChar)
         break;

      if(iChar == NWNCP_CHAR_COLON ||
         iChar == NWNCP_CHAR_SLASH ||
         iChar == NWNCP_CHAR_BACKSLASH)
      {
         /* check for special cases. A backslash may be followed by
            a colon, slash or backslash (an escaped character). */
         if(iChar == NWNCP_CHAR_BACKSLASH)
         {
            if(iCharSize == 1)
               iChar2 = *pbuPath;
            else
               NCopy16(&iChar2, pbuPath);

            if(iChar2 == NWNCP_CHAR_COLON ||
               iChar2 == NWNCP_CHAR_SLASH ||
               iChar2 == NWNCP_CHAR_BACKSLASH)
            {
               /* this is an escaped character */
               pbuPath += iCharSize;
               iCompLen += iCharSize;
               if(iCharSize == 1)
                  *pabuCompPath++ = (nuint8) iChar2;
               else
               {
                  NCopyLoHi16(pabuCompPath, &iChar2);
                  pabuCompPath += 2;
               }
               continue;
            }
         }

         if(iCompLen)
         {
            *pbuCompLen = (nuint8) iCompLen;
            pbuCompLen  = pabuCompPath++;
            suLen      += (nuint16) ++iCompLen; /* incs to count the len byte */
            iCompLen    = 0;
            iNumComps++;
         }
      }
      else
      {
         if(iCharSize == 2)
         {
            iCompLen += 2;
            NCopyLoHi16(pabuCompPath, &iChar);
            pabuCompPath += 2;
         }
         else
         {
            iNewChar = iChar;
            switch(iChar)
            {
               case (NWNCP_CHAR_STAR):
               case (NWNCP_CHAR_PERIOD):
               case (NWNCP_CHAR_QUESTION):
               case (NWNCP_CHAR_BREAK):
               {
                  if(flOptions & NCP_AUGMENT)
                  {
                     iNewChar += 0x80;
                     *pabuCompPath++ = (nuint8) NWNCP_CHAR_BREAK;
                     iCompLen++;
                  }
                  else if(iChar != NWNCP_CHAR_PERIOD)
                  {
                     *pabuCompPath++ = (nuint8) NWNCP_CHAR_BREAK;
                     iCompLen++;
                  }
                  iChar = 0;
                  break;
               }

               /* Yes, there are no other switches... */
               default:
                  break;
            }

            if(iChar && (NWCharType((nstr) iChar) == 2))
            {
               iCompLen++;
               *pabuCompPath++ = (nuint8) iNewChar;
               iNewChar = (nint) *pbuPath++;
            }
            *pabuCompPath++ = (nuint8) iNewChar;
            iCompLen++;
         }
      }
   }

   if(iCompLen)
   {
      *pbuCompLen = (nuint8) iCompLen;
      suLen += (nuint16) ++iCompLen; /* incs to count the len byte */
      iNumComps++;
   }

   if(pbuCompCnt)
      *pbuCompCnt = (nuint8) iNumComps;

   if(psuLen)
      *psuLen = suLen;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/comppath.c,v 1.7 1994/09/26 17:40:26 rebekah Exp $
*/
