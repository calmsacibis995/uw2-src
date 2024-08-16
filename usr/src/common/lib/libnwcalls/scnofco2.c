/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:scnofco2.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwfile.h"
#include "nwserver.h"

/*manpage*NWScanOpenFilesByConn2********************************************
SYNTAX:  NWCCODE N_API NWScanOpenFilesByConn2
         (
            NWCONN_HANDLE  conn,
            NWCONN_NUM     connNum,
            pnint16        iterHandle,
            OPEN_FILE_CONN_CTRL NWPTR openCtrl,
            OPEN_FILE_CONN NWPTR openFile
         );

REMARKS:

ARGS: >  conn

      >  connNum
         The connection number of the logged-in object to be scanned

      <> iterHandle
         Should be set to 0 for first call. After each call returns the
         number of the next record to be scanned.  Returns -1 upon
         completion. Should not be changed during the scan.

      <> openCtrl
         Returns the OPEN_FILE_CONN_CTRL which is used by the call. This
         structure should not be written to by the user.

         typedef struct
         {
           nuint16 nextRequest;
           nuint16 openCount;
           nuint8 buffer[512];
           nuint16 curRecord;
         } OPEN_FILE_CONN_CTRL;

      <  openFiles
         Returns the OPEN_FILE_CONN structure as follows:

         typedef struct
         {
           nuint16  taskNumber;
           nuint8  lockType;
           nuint8  accessControl;
           nuint8  lockFlag;
           nuint8  volNumber;
           nuint32 parent;
           nuint32 dirEntry;
           nuint8  forkCount;
           nuint8  nameSpace;
           nuint8  nameLen;
           char  fileName[255];
         } OPEN_FILE_CONN;

         For 2.X, this structure will be only partially filled.

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 219  Get Connection's Open Files (2.x)
         23 235  Get Connection's Open Files (3.x)

CHANGES: 01 May 1992 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWScanOpenFilesByConn2
(
   NWCONN_HANDLE              conn,
   NWCONN_NUM                 connNum,
   pnint16                    iterHandle,
   OPEN_FILE_CONN_CTRL  NWPTR openCtrl,
   OPEN_FILE_CONN       NWPTR openFile
)
{
   nuint16 serverVer, currRecordSize;
   pnuint8 ptr;
   NWCCODE ccode = 0;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(*iterHandle < 0)
      return (0x88FF);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(*iterHandle == 0 || (nuint16) *iterHandle > openCtrl->openCount)
   {
      if(*iterHandle == 0)
      {
         openCtrl->openCount = (nuint16) 0;
         openCtrl->nextRequest = (nuint16) 1;
      }

      if(!openCtrl->nextRequest)
      {
         if(!openFile || (nuint16) *iterHandle > openCtrl->openCount)
         {
            *iterHandle = -1;
            return (0x88FF);
         }
      }

      openCtrl->nextRequest = (nuint16) (*iterHandle ? openCtrl->nextRequest : 0);

      if(serverVer >= 3000 || serverVer < 2000)
      {
         ccode = (NWCCODE) NWNCP23s235GetConnOpenFiles(&access,
                     (nuint16) connNum, &openCtrl->nextRequest,
                     &openCtrl->openCount, openCtrl->buffer);
      }
      else
      {
         nuint8 tempCount, pos, i;
         NWNCPConnOpenFiles2x fileBufs[24];

         ccode = (NWCCODE) NWNCP23s219GetConnOpenFiles(&access,
                     (nuint16) connNum, &openCtrl->nextRequest,
                     &tempCount, fileBufs);
         if (ccode == 0)
         {
            openCtrl->openCount = (nuint16) tempCount;
            for (i = pos = 0; i < tempCount && i < 24; i++, pos += 21)
            {
               openCtrl->buffer[pos+0] = fileBufs[i].buTaskNum;
               openCtrl->buffer[pos+1] = fileBufs[i].buLockType;
               openCtrl->buffer[pos+2] = fileBufs[i].buAccessControl;
               openCtrl->buffer[pos+3] = fileBufs[i].buLockFlag;
               openCtrl->buffer[pos+4] = fileBufs[i].buVolNum;
               NCopy16(&openCtrl->buffer[pos+5], &fileBufs[i].suDirEntry);
               NWCMemMove(&openCtrl->buffer[pos+7],
                     fileBufs[i].pbstrFileName, 14);
            }
         }
      }

      if(!openCtrl->openCount)
         ccode = 0x88ff;

      if(ccode)
      {
         *iterHandle = -1;
         openCtrl->curRecord = (nuint16) 0xffff;
         return (ccode);
      }

      openCtrl->curRecord = (nuint16) 0;
      *iterHandle = 1;
   }

   ptr = &openCtrl->buffer[openCtrl->curRecord];

   if (serverVer >= 3000 || serverVer < 2000)
   {
      NCopy16(&openFile->taskNumber, &ptr[0]);
      openFile->lockType = ptr[2];
      openFile->accessControl = ptr[3];
      openFile->lockFlag = ptr[4];
      openFile->volNumber = ptr[5];
      NCopy32(&openFile->parent, &ptr[6]);
      NCopy32(&openFile->dirEntry, &ptr[10]);
      openFile->forkCount = ptr[14];
      openFile->nameSpace = ptr[15];
      openFile->nameLen = ptr[16];
      NWCMemMove(openFile->fileName, &ptr[17], ptr[16]);
      openFile->fileName[ptr[16]] = 0x00;

      currRecordSize = 17 + ptr[16];
   }
   else
   {
      nuint16 temp;

      openFile->taskNumber    = (nuint16) ptr[0];
      openFile->lockType      = ptr[1];
      openFile->accessControl = ptr[2];
      openFile->lockFlag      = ptr[3];
      openFile->volNumber     = ptr[4];
      openFile->parent        = (nuint32) -1;
      NCopy16(&temp, &ptr[5]);
      openFile->dirEntry      = (nuint32) temp;
      openFile->forkCount     = (nuint8) -1;
      openFile->nameSpace     = (nuint8) 0;
      openFile->nameLen       = (nuint8) NWCStrLen(&ptr[7]);
      NWCStrCpy(openFile->fileName, &ptr[7]);

      currRecordSize = 21;
   }

   openCtrl->curRecord += currRecordSize;
   (*iterHandle)++;

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/scnofco2.c,v 1.7 1994/09/26 17:49:31 rebekah Exp $
*/
