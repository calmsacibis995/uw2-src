/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:servejo2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwintern.h"
#include "nwmisc.h"
#include "nwserver.h"

/*manpage*NWServiceQueueJob2************************************************
SYNTAX:  NWCCODE N_API NWServiceQueueJob2
         (
            NWCONN_HANDLE           conn,
            nuint32                 queueID,
            nuint16                 targetJobType,
            NWQueueJobStruct NWPTR  userJobStruct,
            NWFILE_HANDLE NWPTR     fileHandle
         )

REMARKS: Allows a queue server client to select a new NWQueueJobStruct
         for servicing.

ARGS:  > conn
       > queueID
       > targetJobType
      <  userJobStruct
      <  fileHandle

INCLUDE: nwqms.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 124  Service Queue Job
         23 113  Service Queue Job

CHANGES: Art 9/20/93
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWServiceQueueJob2
(
   NWCONN_HANDLE           conn,
   nuint32                 queueID,
   nuint16                 targetJobType,
   NWQueueJobStruct NWPTR  userJobStruct,
   NWFILE_HANDLE NWPTR     fileHandle
)
{
   nuint32 fileSize;
   NWCCODE ccode;
   nuint16 serverVer;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if((ccode = NWGetFileServerVersion(conn, &serverVer)) != 0)
      return (ccode);

   if(serverVer >= 3110 || serverVer < 2000)
   {
      NWNCPQMSJobStruct2 localJobStruct;

      /* swap the queue id here */

      if ((ccode = (NWCCODE) NWNCP23s124ServiceQJob(&access,
         NSwap32(queueID), targetJobType, &localJobStruct)) == 0)
      {
         /*
         **  The first 3 fields of localJobStruct are not returned from
         **  NWCalls, therefore, they are not copied
         */

         NWCMemMove(userJobStruct, &localJobStruct.luClientStation,
            sizeof(NWQueueJobStruct));

      /*****************************************
         __NWSwapNWJobStructIDs(userJobStruct);

         We dont need to do this here because this
         is not the structure that is getting
         returned to the user.  That structure
         is obtained with the call to
         NWReadQueueJobEntry2 below.
      *****************************************/
      }
   }
   else
   {
      QueueJobStruct tmpJob;

      /* and swap the queue id here */

      if ((ccode = (NWCCODE) NWNCP23s113ServiceQJob(&access,
         NSwap32(queueID), targetJobType,
         (pNWNCPQMSJobStruct) &tmpJob)) == 0)
      {
      /*****************************************
         __NWSwapJobStructIDs(&tmpJob);

         We dont need to do this here because this
         is not the structure that is getting
         returned to the user.  That structure
         is obtained with the call to
         NWReadQueueJobEntry2 below.
      *****************************************/

         ConvertQueueToNWQueue(userJobStruct, &tmpJob);
      }
   }

   if (ccode != 0)
      return ccode;

   /******
   ** but don't swap the queue id here, it will be swapped inside
   ** the other NWCalls function
   ******/

   if ((ccode = NWGetQueueJobFileSize2(conn, queueID,
            userJobStruct->jobNumber, &fileSize)) != 0)
   {
      return (ccode);
   }

   if ((ccode = NWConvertHandle(conn, 0x01,
            (pnuint8)&(userJobStruct->jobFileHandle), 4, fileSize,
            fileHandle)) != 0)
   {
      nuint8   abuSixByteHandle[6];

      _NWConvert4ByteTo6ByteHandle((pnuint8)&userJobStruct->jobFileHandle,
            abuSixByteHandle);

      NWNCP66FileClose(&access, 0, abuSixByteHandle);

      return (ccode);
   }

   /******
   ** and don't swap the queue id here either, it will be swapped inside
   ** the other NWCalls function
   ******/

   return NWReadQueueJobEntry2(conn, queueID, userJobStruct->jobNumber,
            userJobStruct);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/servejo2.c,v 1.7 1994/09/26 17:49:45 rebekah Exp $
*/
