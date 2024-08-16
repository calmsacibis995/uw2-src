/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scntrst2.c	1.6"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwdirect.h"
#include "nwclocal.h"
#include "nwserver.h"

void __CleanTrusteeList__
(
   nuint16 suPos,
   TRUSTEE_INFO NWPTR trustees
);

/*manpage*NWScanDirectoryForTrustees2***************************************
SYNTAX:  NWCCODE N_API NWScanDirectoryForTrustees2
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            pnstr8   pbstrSrchPath,
            pnuint32 pluIterHnd,
            pnstr8   pbstrDirName,
            pnuint32 pluDirDateTime,
            pnuint32 pluOwnerID,
            TRUSTEE_INFO NWPTR trusteeList
         );

REMARKS:

ARGS: >  conn
      >  dirHandle
      >  pbstrSrchPath
      <> pluIterHnd
      <  pbstrDirName
      <  pluDirDateTime
      <  pluOwnerID
      <  trusteeList

INCLUDE: nwdirect.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 05  Scan File or Subdirectory for Trustees
         22 12  Scan Directory For Trustees
         22 38  Scan File Or Directory For Extended Trustees

CHANGES: 15 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanDirectoryForTrustees2
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         pbstrSrchPath,
   pnuint32       pluIterHnd,
   pnstr8         pbstrDirName,
   pnuint32       pluDirDateTime,
   pnuint32       pluOwnerID,
   TRUSTEE_INFO NWPTR trusteeList
)
{
   NWCCODE ccode;
   nuint16 suTrusteeCount, suServerVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = NWGetFileServerVersion(conn, &suServerVer)) != 0)
      return (ccode);

   if (suServerVer >= 3110)
   {
      NWNCPCompPath cPath;

      cPath.luDirBase = (nuint32) dirHandle;
      cPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;
      NWNCPPackCompPath(-1, pbstrSrchPath, -1, &cPath, (nflag32) 0);

      if((ccode = (NWCCODE) NWNCP87s5TrusteesScan(&access, cPath.buNamSpc,
                  (nuint8) 0, 0x001F, &cPath, pluIterHnd, &suTrusteeCount,
                  (pNWNCPTrustees) trusteeList)) != 0)
         return (ccode);

      __CleanTrusteeList__(suTrusteeCount, trusteeList);
      if (pbstrDirName || pluDirDateTime || pluOwnerID)
      {
         NWNCPEntryStruct entryInfo;

         ccode = (NWCCODE) NWNCP87s6GetEntryInfo(&access, cPath.buNamSpc, cPath.buNamSpc,
                           (nuint16) FA_DIRECTORY, (nuint32) 0x0101L, &cPath, &entryInfo);

         if(pbstrDirName)
         {
            NWCMemMove(pbstrDirName, entryInfo.abuName, entryInfo.buNameLen);
            pbstrDirName[entryInfo.buNameLen] = (nuint8) 0;
         }

         if(pluDirDateTime)
            *pluDirDateTime = NMake32(entryInfo.suCreationTime, entryInfo.suCreationDate);

         if(pluOwnerID)
            *pluOwnerID = NSwap32(entryInfo.luCreatorID);
      }
   }
   else
   {
      nstr8 abstrTempPath[256];

      if (pbstrSrchPath)
      {
         NWCStrCpy(abstrTempPath, pbstrSrchPath);
         NWConvertAndAugment(abstrTempPath, 0);
      }
      else
         abstrTempPath[0] = '\0';

      if(suServerVer >= 3000 || suServerVer < 2000)
      {
         nint i;
         nuint32 aluTempObjIDs[20];
         nuint16 asuTempRights[20];
         nuint8  buTemp;

         if((ccode = (NWCCODE) NWNCP22s38TrusteesScanExt(&access,
                     (nuint8) dirHandle, (nuint8) *pluIterHnd,
                     (nuint8) NWCStrLen(abstrTempPath), abstrTempPath,
                     &buTemp, aluTempObjIDs, asuTempRights)) != 0)
            return ccode;

         for (i = 0; i < (nint) buTemp && i < 20; i++)
         {
            trusteeList[i].objectID     = aluTempObjIDs[i];
            trusteeList[i].objectRights = asuTempRights[i];
         }

         (*pluIterHnd)++;

         __CleanTrusteeList__((nuint16) buTemp, trusteeList);

         if (pbstrDirName || pluDirDateTime || pluOwnerID)
         {
            nuint32 temp[3];

            temp[0] = (nuint32) -1L;  /* scan sequence */
            ccode = NWScanDirectoryInformation2(conn, dirHandle,
                        pbstrSrchPath, (pnuint8) temp, pbstrDirName,
                        pluDirDateTime, pluOwnerID, NULL);
         }
      }
      else
      {
         nuint32 aluTempObjIDs[5];
         nuint16 suTempDate, suTempTime;
         nuint8  abuTempRights[5], buSetNum;
         nint i;

         buSetNum = (nuint8)(*pluIterHnd);
         suTrusteeCount = (nuint16) 0;

         do
         {
            ccode = (NWCCODE) NWNCP22s12TrusteesScanDir(&access,
                        (nuint8) dirHandle, buSetNum,
                        (nuint8) NWCStrLen(abstrTempPath), abstrTempPath,
                        pbstrDirName, &suTempDate, &suTempTime, pluOwnerID,
                        aluTempObjIDs, abuTempRights);
            if (ccode == 0)
            {
               if (!suTrusteeCount)
               {
                  if (pluDirDateTime)
                     *pluDirDateTime = NMake32(suTempTime, suTempDate);

                  if (pluOwnerID)
                     *pluOwnerID = NSwap32(*pluOwnerID);
               }

               for (i = 0; i < 5; i++)
               {
                  if (aluTempObjIDs[i])
                  {
                     trusteeList[suTrusteeCount].objectID =
                        aluTempObjIDs[i];
                     trusteeList[suTrusteeCount++].objectRights =
                        (nuint16) abuTempRights[i];
                  }
               }
               /* the buSetNum must be incremented manually with this NCP
                  because there is no incremental set number returned
               */

               buSetNum++;
            }
         } while (ccode == 0 && (suTrusteeCount + 5) < 20);

         ccode = (suTrusteeCount && ccode == 0x899C) ? 0 : ccode;
         *pluIterHnd = (nuint32) suTrusteeCount;

         __CleanTrusteeList__(suTrusteeCount, trusteeList);
      }
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scntrst2.c,v 1.8 1994/09/26 17:49:41 rebekah Exp $
*/
