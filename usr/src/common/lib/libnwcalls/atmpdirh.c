/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:atmpdirh.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwcaldef.h"
#include "ncpafp.h"
#include "nwafp.h"

/*manpage*NWAFPAllocTemporaryDirHandle**************************************
SYNTAX:  NWCCODE N_API NWAFPAllocTemporaryDirHandle
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum,
            nuint32        APFEntryID,
            pnstr8         APFPathString,
            NWDIR_HANDLE NWPTR dirHandle,
            pnuint8        accessRights
         )

REMARKS: This call maps a NetWare directory handle to an AFP directory.

         The NetWare Access Rights field is a 1-byte mask that returns the
         effective rights that the calling station has in the target directory.
         This byte can be a combination of the following bits:
                        0x01  Read
                        0x02  Write
                        0x04  Open
                        0x08  Create
                        0x10  Delete
                        0x20  Parental
                        0x40  Search
                        0x80  Modify

ARGS: >  conn
      >  volNum
      >  APFEntryID
      >  APFPathString
      <  dirHandle (optional)
      <  accessRights (optional)

INCLUDE: nwafp.h

RETURN:  0x0000  Successful
         0x8983  Hard I/O Error
         0x8988  Invalid File Handle
         0x8993  No Read Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x899D  No Directory Handles
         0x89A1  Directory I/O Error
         0x89A2  I/O Lock Error
         0x89FD  Bad Station Number
         0x89FF  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 11  AFP Alloc Temporary Directory Handle

CHANGES: 18 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWAFPAllocTemporaryDirHandle
(
   NWCONN_HANDLE  conn,
   nuint16        volNum,
   nuint32        APFEntryID,
   pnstr8         AFPPath,
   NWDIR_HANDLE NWPTR dirHandle,
   pnuint8        accessRights
)
{
   NWCCODE ccode;
   nuint8  tempDirHandle;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if ((ccode = (NWCCODE) NWNCP35s11AFPAllocTempDirHan(&access,
         (nuint8) volNum, NSwap32(APFEntryID),
         (nuint8) AFPPath[0], (AFPPath+1), &tempDirHandle,
         accessRights)) == 0)
   {
      if (dirHandle)
         *dirHandle = tempDirHandle;
   }
   else
   {
      if (dirHandle)
         *dirHandle = 0;
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/atmpdirh.c,v 1.7 1994/09/26 17:44:12 rebekah Exp $
*/
