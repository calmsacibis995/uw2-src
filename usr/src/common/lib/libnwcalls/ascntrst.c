/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ascntrst.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwdentry.h"
#include "nwserver.h"
#include "nwclocal.h"

void __CleanTrusteeList__
(
   nuint16 suPos,
   TRUSTEE_INFO NWPTR trustees
);

/*manpage*NWIntScanForTrustees**********************************************
SYNTAX:  NWCCODE N_API NWIntScanForTrustees
         (
           NWCONN_HANDLE   conn,
           NWDIR_HANDLE    dirHandle,
           pnstr8          path,
           pnuint32        interHandle,
           pnuint16        numOfEntries,
           NWET_INFO NWPTR trusteeInfo,
           nuint16         augmentFlag
         )

REMARKS: Returns a trustee list for a directory entry. It replaces
         NWScanDirectoryForTrustees and is able to handle long names in OS/2.

         This call will work only for directories in 2.2, for 3.11 it will
         work for files and directories.

         Directories can have any number of bindery objects as trustees.
         These directory trustees are stored and retrieved in groups of 20.
         The sequenceNumber parameter should be set to zero (or NULL) to
         obtain the first trustee group; for subsequent calls, the interHandle
         number returned by the previous call is used. Scanning should
         continue until interHandle number comes back as -1L.

         Another behavior particular to 2.X servers is that the supporting OS
         call only returns 5 entries at a time. In order to provide
         consistency with the 3.x servers, this API will attempt to retrieve
         up to 20 trustee entries by calling the OS up to n times or the
         NO_MORE_TRUSTEE error is encountered. In doing so, this API will
         also skip over any "holes" in the trustee list and return only valid
         trustee assignments. Holes in the trutee list refer to the way
         NetWare 2.X will remove trustee assignments from a directory (by
         zero-ing out the entry), if the trustee entry is in the middle of
         the list, a "hole" is thus created.

         NOTE: A known side effect of this API is that the error code 0x899C
               is returned by the server for INVALID_PATH, as well as for
               NO_MORE_TRUSTEES, thus making it difficult to know whether the
               path parameter is invalid, or whether it is valid and there
               are no trustees associated with a directory entry. This side
               effect is not present for NetWare v3.11 and above.

ARGS: >  conn
      >  dirHandle
      >  path
         can contain wild cards, but only the first matching directory will
         be scanned (2.2 behavior)
      <> interHandle
         Pointer to the server maintained interHandle number.
         (initially set to 0)
      <  numOfEntries
         Pointer to buffer to receive number of entries being returned from
         this call.
      <  trusteeInfo
         Pointer to the trustee information structure:

         typedef struct
         {
            char    entryName[256];       - Name of the directory entry
            nuint32 creationDateAndTime;  - Date and time of creation
            nuint32 ownerID;              - Bindery object ID of the owner
            nuint32 sequenceNumber;       - Initially needs to be set to 0
            TRUSTEE_INFO trusteeList[20]; - List of trustees
         } NWET_INFO;

         typedef struct
         {
            nuint32 objectID;
            nuint16 objectRights;
         } TRUSTEE_INFO;

      >  augmentFlag
         If this flag equals USE_NW_WILD_MATCH all wild cards are augmented.

INCLUDE: nwdentry.h

RETURN:  n/a

SERVER:  2.2 3.11 4.0 PNW

CLIENT:  DOS WIN OS2

SEE:     NWScanDirectoryForTrustees2

NCP:     22 12  Scan Directory For Trustees
         22 38  Scan File or Directory For Extended Trustees
         87 05  Scan File or SubDirectory for Trustees

CHANGES: 20 May 1993 - modified - jwoodbur
           added 22 38 functionality back in. Rewrote routine to use more
           common code for other equivilant functions.
         14 Sep 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWIntScanForTrustees
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint32       interHandle,
   pnuint16       numOfEntries,
   NWET_INFO NWPTR trusteeInfo,
   nuint16        augmentFlag
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   if(serverVer >= 3110)
   {
      NWNCPCompPath compPath;

      compPath.luDirBase = (nuint32) dirHandle;
      compPath.buHandleFlag = NWNCP_COMPPATH_USE_DIRHANDLE;

      NWNCPPackCompPath(-1, path, -1, &compPath,
               augmentFlag ? augmentFlag : (nflag32) 0);

      if((ccode = (NWCCODE) NWNCP87s5TrusteesScan(&access, compPath.buNamSpc,
               (nuint8) 0, (nuint16) 0x8000, &compPath, interHandle,
               numOfEntries, (pNWNCPTrustees) trusteeInfo->trusteeList)) != 0)
         return (ccode);

      __CleanTrusteeList__(*numOfEntries, trusteeInfo->trusteeList);

      /* the following line clears the first three entries in the NWET_INFO
             structure
      */
      NWCMemSet(trusteeInfo, 0, sizeof(NWET_INFO)-(sizeof(TRUSTEE_INFO)*20));
      trusteeInfo->sequenceNumber = *interHandle;
   }
   else
   {
      nstr8 abstrTPath[256];

      if(path)
      {
         NWCStrCpy(abstrTPath, path);
         NWConvertAndAugment(abstrTPath, augmentFlag);
      }
      else
         abstrTPath[0] = '\0';

      if(serverVer >= 3000 || serverVer < 2000)
      {
         nuint32 aluTObjID[20];
         nuint16 asuTRights[20];
         nuint8  buTemp;
         nint i;

         if((ccode = (NWCCODE) NWNCP22s38TrusteesScanExt(&access,
                        (nuint8) dirHandle, (nuint8) *interHandle,
                        (nuint8) NWCStrLen(abstrTPath), abstrTPath,
                        &buTemp, aluTObjID, asuTRights)) != 0)
            return (ccode);

         *numOfEntries = (nuint16) buTemp;

         for(i=0; i < (nint) buTemp; i++)
         {
            trusteeInfo->trusteeList[i].objectID     = aluTObjID[i];
            trusteeInfo->trusteeList[i].objectRights = asuTRights[i];
         }

         __CleanTrusteeList__(*numOfEntries, trusteeInfo->trusteeList);

         (*interHandle)++;

         /* the following line clears the first three entries in the NWET_INFO
              structure */
         NWCMemSet(trusteeInfo, 0, sizeof(NWET_INFO)-(sizeof(TRUSTEE_INFO)*20));
         trusteeInfo->sequenceNumber = *interHandle;
      }
      else
      {
         nuint32 aluTObjID[5];
         nuint16 suTDate, suTTime, suTCount = 0;
         nuint8 abuTRights[5];
         nint i;

         if(*interHandle == 0)
            *interHandle = 1;

         do
         {
            if((ccode = (NWCCODE) NWNCP22s12TrusteesScanDir(&access, dirHandle,
                  (nuint8) *interHandle, (nuint8) NWCStrLen(abstrTPath),
                  abstrTPath, trusteeInfo->entryName, &suTDate, &suTTime,
                  &trusteeInfo->ownerID, aluTObjID, abuTRights)) == 0)
            {
               for( i = 0; i < 5; i++, suTCount++ )
               {
                  if(aluTObjID[i])
                  {
                     trusteeInfo->trusteeList[suTCount].objectID =
                        aluTObjID[i];

                     trusteeInfo->trusteeList[suTCount].objectRights =
                        abuTRights[i];
                  }
               }

               if(suTCount != 0)
               {
                  trusteeInfo->creationDateAndTime = NMake32( suTTime,
                     suTDate );
               }

               (*interHandle)++;
            }
         } while (ccode == 0 && (suTCount + 5) < 20);

         *numOfEntries = suTCount;

         trusteeInfo->ownerID = NSwap32(trusteeInfo->ownerID);
         trusteeInfo->sequenceNumber = *interHandle;
         ccode = (suTCount && ccode == 0x899C) ? 0 : ccode;

         __CleanTrusteeList__(suTCount, trusteeInfo->trusteeList);
      }
   }

   return (ccode);
}


void __CleanTrusteeList__
(
   nuint16 suPos,
   TRUSTEE_INFO NWPTR trustees
)
{
   nuint16 i;

   for (i = 0; i < 20; i++)
   {
      if (i >= suPos)
      {
         trustees[i].objectID     = (nuint32) 0;
         trustees[i].objectRights = (nuint16) 0;
      }
      else
         trustees[i].objectID     = NSwap32(trustees[i].objectID);
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ascntrst.c,v 1.7 1994/09/26 17:44:09 rebekah Exp $
*/
