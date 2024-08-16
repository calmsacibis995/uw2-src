/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:crteqjo2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwqms.h"
#include "nwmisc.h"
#include "nwfile.h"
#include "nwserver.h"

/*manpage*NWCreateQueueFile2************************************************
SYNTAX:  NWCCODE N_API NWCreateQueueFile2
         (
            NWCONN_HANDLE           conn,
            nuint32                 queueID,
            NWQueueJobStruct NWPTR  userJobStruct,
            NWFILE_HANDLE NWPTR     fileHandle
         )

REMARKS:
         Enters a new job on the queue and creates a Job File who's name
         and handle are returned in the QueueJobStruct.  This file is then
         attached to a special file opened at the workstation. This special
         file handle is returned in 'fileHandle'.

         NOTE: The parameters required in the QueueJobStruct are:

           QueueJobStruct.targetServerID

           QueueJobStruct.targetExecutionTime[6]

           QueueJobStruct.jobType

           QueueJobStruct.jobControlFlags -  Operator Hold,
                                             User Hold,
                                             Service Restart,
                                             Auto Start

           QueueJobStruct.jobDescription[50] - text

           QueueJobStruct.clientRecordArea[152] - client defined

ARGS:  > conn
       > queueID
      <> userJobStruct
      <  fileHandle

INCLUDE: nwqms.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 121  Create Queue Job And File
         23 104  Create Queue Job And File

CHANGES: 02 Dec 1993 - removed non-portable structure
            initialization - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWCreateQueueFile2
(
   NWCONN_HANDLE           conn,
   nuint32                 queueID,
   NWQueueJobStruct NWPTR  userJobStruct,
   NWFILE_HANDLE NWPTR     fileHandle
)
{
   NWCCODE ccode;
   nuint16 serverVer;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return ccode;

   queueID = NSwap32(queueID);

   if(serverVer >= 3110 || serverVer < 2000)
   {
     /*
      **  The first 3 fields of localJobStruct are not part of the
      **  NWQueueJobStruct defined in nwqms.h.  They can not be
      **  changed by this call, therefore they can contain garbage.
      **  They are being set to zeros, however, for consistency.
      */
      NWNCPQMSJobStruct2 localJobStruct;

      localJobStruct.suRecordInUseFlag = (nuint16) 0;
      localJobStruct.luPreviousRecord  = (nuint32) 0;
      localJobStruct.luNextRecord      = (nuint32) 0;

      __NWSwapNWJobStructIDs(userJobStruct);

      NWCMemMove(&localJobStruct.luClientStation, userJobStruct,
         sizeof(NWQueueJobStruct));

      if ((ccode = (NWCCODE) NWNCP23s121CreateQJobAndFile(&access, queueID,
         &localJobStruct, &localJobStruct)) == 0)
      {
         NWCMemMove(userJobStruct, &localJobStruct.luClientStation,
            sizeof(NWQueueJobStruct));

         __NWSwapNWJobStructIDs(userJobStruct);
      }
   }
   else
   {
      QueueJobStruct tmpJob;

      ConvertNWQueueToQueue(&tmpJob, userJobStruct);
      __NWSwapJobStructIDs(&tmpJob);

      if ((ccode = (NWCCODE) NWNCP23s104CreateQJobAndFile(&access, queueID,
         (pNWNCPQMSJobStruct) &tmpJob, (pNWNCPQMSJobStruct) &tmpJob)) == 0)
      {
         __NWSwapJobStructIDs(&tmpJob);
         ConvertQueueToNWQueue(userJobStruct, &tmpJob);
      }
   }

   if (ccode != 0)
      return ccode;

   if ((ccode = NWConvertHandle(conn, 0x02,
         (pnuint8)&(userJobStruct->jobFileHandle),
         4, 0L, fileHandle)) != 0)
   {
      nuint8   abuSixByteHandle[6];

      _NWConvert4ByteTo6ByteHandle((pnuint8)&userJobStruct->jobFileHandle,
         abuSixByteHandle);

      NWNCP66FileClose(&access, 0, abuSixByteHandle);
   }

   return ccode;
}


/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/crteqjo2.c,v 1.7 1994/09/26 17:45:00 rebekah Exp $
*/
