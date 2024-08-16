/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setext.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwfile.h"
#include "nwclocal.h"

/*manpage*NWSetExtendedFileAttributes2**************************************
SYNTAX:  NWCCODE N_API NWSetExtendedFileAttributes2
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHnd,
            pnstr8         path,
            nuint8         extAttrs
         );

REMARKS: Assigned the NetWare extended attributes to a file.

         This is NOT the same as the EAs as associated with OS/2. Use
         the EA set of calls for this functionality.

ARGS:
      >  conn
      >  dirHnd
      >  path
      >  extAttrs
         Bits:
         | 7 | 6 | 5 | 4 | 3 | 2 | 1 | 0 |
           |   |   |   |       |   |   |
           |   |   |   |       +---+---+---> search mode bits
           |   |   |   |
           |   |   |   +--> Transaction Bit
           |   |   |
           |   |   +------> Index Bit
           |   |
           |   +----------> Read Audit Bit
           |
           +--------------> Write Audit Bit

         If the Transaction bit is set, TTS tracks all writes to the file
         during a transaction.  A transaction file cannot be deleted or
         renamed until the transaction bit is cleared.

         If the Index bit is set, NetWare indexes the file's File Allocation
         Tables (FATs), thus reducing the time it takes to access the file.
         Files that are randomly accessed and are larger than 2MB would
         greatly benefit from this feature.

INCLUDE: nwfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     79 --  Set File Extended Attribute

CHANGES: 30 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetExtendedFileAttributes2
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHnd,
   pnstr8         path,
   nuint8         extAttrs
)
{
   nstr8 tempPath[256];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   NWCStrCpy(tempPath, path);
   NWConvertToSpecChar(tempPath);

   /* Srch for normal(0), hidden(2), and system(4) == (6) */

   return ((NWCCODE) NWNCP79FileSetExtAttr(&access, extAttrs, (nuint8) dirHnd,
                        (nuint8) 6, (nuint8) NWCStrLen(tempPath),
                        tempPath));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setext.c,v 1.7 1994/09/26 17:49:54 rebekah Exp $
*/

