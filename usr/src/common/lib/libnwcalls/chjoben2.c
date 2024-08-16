/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:chjoben2.c	1.5"
#define THISISAQMSFUNCTION

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpqms.h"

#include "nwcaldef.h"
#include "nwqms.h"
#include "nwintern.h"
#include "nwserver.h"

/*manpage*NWChangeQueueJobEntry2********************************************
SYNTAX:  NWCCODE N_API NWChangeQueueJobEntry2
         (
            NWCONN_HANDLE           conn,
            nuint32                 queueID,
            NWQueueJobStruct NWPTR  userJobStruct
         )

REMARKS: This call allows a client to change the information in for a queue
         job entry (uses the NWQueueJobStruct).

ARGS:  > conn
       > queueID
       > userJobStruct

INCLUDE: nwqms.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 123  Change Queue Job Entry
         23 109  Change Queue Job Entry

CHANGES: 02 Dec 1993 - removed non-portable structure
            initialization - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWChangeQueueJobEntry2
(
   NWCONN_HANDLE           conn,
   nuint32                 queueID,
   NWQueueJobStruct NWPTR  userJobStruct
)
{
   nuint16 serverVer;
   NWCCODE ccode;

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

      NWCMemMove(&localJobStruct.luClientStation,
                 userJobStruct,
                 sizeof(NWQueueJobStruct));

      localJobStruct.luClientID = NSwap32(localJobStruct.luClientID);
      localJobStruct.luTargetServerID = NSwap32(localJobStruct.luTargetServerID);
      localJobStruct.luServicingServerID = NSwap32(localJobStruct.luServicingServerID);


      return((NWCCODE)NWNCP23s123ChangeQJobEntry(&access,
                                                 queueID,
                                                 &localJobStruct));
   }
   else
   {
      QueueJobStruct tmpJob;

      ConvertNWQueueToQueue(&tmpJob, userJobStruct);

      __NWSwapJobStructIDs(&tmpJob);

      return((NWCCODE)NWNCP23s109ChangeQJobEntry(&access,
                                              queueID,
                                              (pNWNCPQMSJobStruct)&tmpJob));
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/chjoben2.c,v 1.7 1994/09/26 17:44:17 rebekah Exp $
*/
