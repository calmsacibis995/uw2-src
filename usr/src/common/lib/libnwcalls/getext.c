/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getext.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwfile.h"
#include "nwcaldef.h"
#include "nwclocal.h"

/*manpage*NWGetExtendedFileAttributes2**************************************
SYNTAX:  NWCCODE N_API NWGetExtendedFileAttributes2
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            pnuint8        extAttrs
         );

REMARKS: Retrieves the NetWare "extended" attributes for the specified
         directory entry.

         The extended attribute byte is interpreted as follows:


         | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
            |   |   |   |       |   |   |
            |   |   |   |       +---+---+---> search mode bits
            |   |   |   |
            |   |   |   |
            |   |   |   +--> Transaction Bit
            |   |   |
            |   |   +------> Index Bit
            |   |
            |   +----------> Read Audit Bit
            |
            +--------------> Write Audit Bit

         If the Transaction bit is set, TTS tracks all writes to the file during
         a transaction. A transaction file cannot be deleted or renamed until
         the transaction bit is cleared.

         If the Index bit is set, NetWare indexes the file's File Allocation
         Tables (FATs), thus reducing the time it takes to access the file.
         Files that are randomly accessed and are larger than 2MB would greatly
         benefit from this feature.

         These "extended" attributes are NetWare's superset of file attributes.
         With NetWare v3.11, support for Extended Attributes was incorporated.
         Extended Attributes are blocks of data (up to 64K) associated with a
         directory entry. This call does NOT retrieve the 3.11 extended
         attributes. For 3.11 EA support refer to the NWCalls library reference.

ARGS: >  conn
      >  dirHandle
      >  path
      <  extAttrs

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 15  Scan File Information

CHANGES: 30 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetExtendedFileAttributes2
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   pnuint8        extAttrs
)
{
   NWCCODE ccode;
   nstr8 tempPath[255];
   nuint16 iterHandle;
   NWNCPFileInfo2 info;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCStrCpy(tempPath, path);
   NWConvertToSpecChar(tempPath);

   iterHandle = (nuint16) -1;

   /* Search for normal(0), hidden(2), and system(4) == (6) */
   if ((ccode = (NWCCODE)NWNCP23s15ScanFiles(&access, dirHandle, (nuint8) 6,
            &iterHandle, (nuint8)NWCStrLen(tempPath),
            tempPath, NULL, &info, NULL)) == 0)
   {
      *extAttrs = info.buExtAttrs;
   }

   return (ccode);
}

/*

$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getext.c,v 1.7 1994/09/26 17:45:58 rebekah Exp $
*/
