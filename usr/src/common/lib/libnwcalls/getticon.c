/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getticon.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpserve.h"

#include "nwundoc.h"
#include "nwmisc.h"
#include "nwserver.h"

/*manpage*NWGetTaskInformationByConn****************************************
SYNTAX:  NWCCODE N_API NWGetTaskInformationByConn
         (
            NWCONN_HANDLE conn,
            NWCONN_NUM connNum,
            CONN_TASK_INFO NWPTR taskInfo
         )

REMARKS: Gets information about the active tasks a specified connection has

ARGS: >  conn
      >  connNum
      <  taskInfo

INCLUDE: nwmisc.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights
         0x89FD   Bad Station Number

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 218  Get Connection's Task Information
         23 234  Get Connection's Task Information

CHANGES: 23 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
NWCCODE N_API NWGetTaskInformationByConn
(
  NWCONN_HANDLE   conn,
  NWCONN_NUM      connNum,
  CONN_TASK_INFO  NWPTR taskInfo
)
{
   nint i;
   NWCCODE ccode;
   nuint16 suServerVer;
   NWNCPWaitRec3x waitRec3x;
   NWNCPWaitRec2x waitRec2x;
   NWNCPTaskStruct2x aTasks2x[191];

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &suServerVer)) != 0)
      return ccode;

   NWCMemSet(taskInfo, 0, sizeof(CONN_TASK_INFO));

   taskInfo->serverVersion = suServerVer;

   if(suServerVer >= 3000)
   {
      if((ccode = (NWCCODE) NWNCP23s234GetConnTaskInfo(&access, connNum,
            &taskInfo->lockState, (pNWNCPWaitRec3x)&waitRec3x, &taskInfo->taskCount,
            (pNWNCPTaskStruct3x)&taskInfo->tasks)) != 0)
         return (ccode);

      if (taskInfo->lockState == (nuint8) 1)
      {
         taskInfo->recordStart = waitRec3x.u.case1.luRecStart;
         taskInfo->recordEnd = waitRec3x.u.case1.luRecEnd;
         taskInfo->volNumber = waitRec3x.u.case1.buVolNum;
         taskInfo->dirEntry  = waitRec3x.u.case1.luDirEntry;
         taskInfo->nameSpace = waitRec3x.u.case1.buNamSpc;
         for (i=0; i < (nint) waitRec3x.u.case1.buLockedFileNameLen; i++)
         {
            taskInfo->lockedName[i] = waitRec3x.u.case1.pbstrLockedFileName[i];
         }
      }
      if (taskInfo->lockState == (nuint8) 2)
      {
         taskInfo->volNumber = waitRec3x.u.case2.buVolNum;
         taskInfo->dirEntry  = waitRec3x.u.case2.luDirEntry;
         taskInfo->nameSpace = waitRec3x.u.case2.buNamSpc;
         for (i=0; i < (nint) waitRec3x.u.case2.buLockedFileNameLen; i++)
         {
            taskInfo->lockedName[i] = waitRec3x.u.case2.pbstrLockedFileName[i];
         }
      }
      if (taskInfo->lockState == (nuint8) 3)
      {
         for (i=0; i < (nint) waitRec3x.u.case3.buLockedRecNameLen; i++)
         {
            taskInfo->lockedName[i] = waitRec3x.u.case3.pbstrLockedRecName[i];
         }
      }
      if (taskInfo->lockState == (nuint8) 4)
      {
         for (i=0; i < (nint) waitRec3x.u.case4.buLockedSemNameLen; i++)
         {
            taskInfo->lockedName[i] = waitRec3x.u.case4.pbstrLockedSemName[i];
         }
      }
   }
   else
   {
      if((ccode = (NWCCODE) NWNCP23s218GetConnTaskInfo(&access, connNum,
            &taskInfo->lockState, &waitRec2x, &taskInfo->taskCount,
            aTasks2x)) != 0)
         return (ccode);

      if (taskInfo->lockState == (nuint8) 1)
      {
         taskInfo->recordStart = waitRec2x.u.case1.luRecStart;
         taskInfo->recordEnd = waitRec2x.u.case1.luRecEnd;
         taskInfo->volNumber = waitRec2x.u.case1.buVolNum;
         taskInfo->dirID  = waitRec2x.u.case1.suDirID;
         NWCStrCpy(taskInfo->lockedName, waitRec2x.u.case1.pbstrLockedFileName);
      }
      if (taskInfo->lockState == (nuint8) 2)
      {
         taskInfo->volNumber = waitRec2x.u.case2.buVolNum;
         taskInfo->dirID  = waitRec2x.u.case2.suDirID;
         NWCStrCpy(taskInfo->lockedName, waitRec2x.u.case2.pbstrLockedFileName);
      }
      if (taskInfo->lockState == (nuint8) 3)
      {
         NWCStrCpy(taskInfo->lockedName, waitRec2x.u.case3.pbstrLockedRecName);
      }
      if (taskInfo->lockState == (nuint8) 4)
      {
         NWCStrCpy(taskInfo->lockedName, waitRec2x.u.case4.pbstrLockedSemName);
      }
      for (i=0; i < (nint) taskInfo->taskCount; i++)
      {
         taskInfo->tasks[i].taskNumber = (nuint16) aTasks2x[i].buTaskNum;
         taskInfo->tasks[i].taskState = aTasks2x[i].buTaskState;
      }
   }

   return ((NWCCODE) 0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getticon.c,v 1.7 1994/09/26 17:46:21 rebekah Exp $
*/
