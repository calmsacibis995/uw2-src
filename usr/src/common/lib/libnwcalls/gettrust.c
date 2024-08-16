/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gettrust.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpbind.h"

#include "nwbindry.h"
#include "nwcaldef.h"

/*manpage*NWScanObjectTrusteePaths******************************************
SYNTAX:  NWCCODE N_API NWScanObjectTrusteePaths
         (
            NWCONN_HANDLE  conn,
            nuint32        objID,
            nuint16        volNum,
            pnuint16       iterHandle,
            pnuint8        accessRights,
            pnstr8         dirPath
         );

REMARKS: Returns the directory paths to which an object has trustee rights.

         This function is used iteratively to determine all of a bindery
         object's trustee directory paths and corresponding access masks.

         The sequence parameter is an index to the next directory
         path and should initially be set to -1.  The sequence
         parameter is automatically incremented to point to the next
         directory path.  When all valid directory paths have been
         returned, the trusteePathName is set to NULL.

         Only SUPERVISOR, the object, or a bindery object that is security
         equivalent to SUPERVISOR or the object, can scan an object's
         trustee directory paths.

         Trustee Access Mask:
         TA_READ       - file reads allowed
         TA_WRITE      - file writes allowed
         TA_OPEN       - files can be opened
         TA_CREATE     - files can be created
         TA_DELETE     - files can be deleted
         TA_OWNERSHIP  - subdirectories can be created/deleted and
                           trustee rights granted/revoked
         TA_SEARCH     - directory can be searched
         TA_MODIFY     - file attributes can be modified
         TA_ALL        - trustee has all the above rights to the directory

ARGS:  > conn
       > objID
       > volNum
      <> iterHandle
      <  accessRights
      <  dirPath

INCLUDE: nwbindry.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 71  Scan Bindery Object Trustee Paths

CHANGES: 23 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWScanObjectTrusteePaths
(
   NWCONN_HANDLE  conn,
   nuint32        objID,
   nuint16        volNum,
   pnuint16       iterHandle,
   pnuint8        accessRights,
   pnstr8         dirPath
)
{
   NWCCODE ccode;
   nuint8 tempLen;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   ccode = (NWCCODE) NWNCP23s71ScanObjTrustPaths(&access, (nuint8) volNum,
         iterHandle, NSwap32(objID), NULL, accessRights, &tempLen, dirPath);

   if (ccode == 0)
   {
      dirPath[tempLen] = 0x00;
   }

   return (ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gettrust.c,v 1.7 1994/09/26 17:46:23 rebekah Exp $
*/
