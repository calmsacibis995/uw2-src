/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:79.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP79FileSetExtAttr*********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP79FileSetExtAttr
         (
            pNWAccess pAccess,
            nuint8   buNewExtAttrs,
            nuint8   buDirHandle,
            nuint8   buSrchAttrs,
            nuint8   buFileNameLen,
            pnstr8   pbstrFileName
         );

REMARKS: This call is equivalent to Set File Attributes (0x2222  70  --), except that
         it sets the NetWare extended attributes byte. The bits of the extended
         attribute byte are defined as follows:

               Bit   Bit Field Description

                  0-3   Search mode (see below)

                  4     Transaction Bit

                  5     Index Bit

                  6     Read Audit Bit
                        (not currently implemented)

                  7     Write Audit Bit
                           (not currently implemented)

         The Search Mode, which is comprised of the first three bits, is only valid
         with NetWare v2.0a and above. The possible values of the Search Mode are

               Bit  Decimal   Mode
                  2 1 0

                  0 0 0         0           Shell Default Search Mode

                  0 0 1         1           Search On All Opens With No Path

                  0 1 0         2           Do Not Search

                  0 1 1         3           Search On Read Only Opens With No Path

                  1 0 0         4           Reserved--Do Not Use

                  1 0 1         5           Search On All Opens

                  1 1 0         6           Reserved--Do Not Use

                  1 1 1         7           Search On All Read Only Opens


         The Transaction bit, if set, prompts NetWare's Transaction Tracking System
         (TTS) to track all writes to the file during a transaction. A transaction
         file cannot be deleted or renamed.

         The Index bit, if set, prompts NetWare to index the file's File Allocation
         Tables (FATs) thereby reducing the time it takes to access the file.

         The Read Audit and Write Audit bits (flags) can only be set by a user with
         security equivalence to the supervisor.

ARGS: <> pAccess
       > buNewExtAttrs
       > buDirHandle
       > buSrchAttrs
       > buFileNameLen
       > pbstrFileName

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x898C  No Set Privileges
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89FD  Bad Station Number
         0x89FF  Failure, No Files Found

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     23 15  Scan File Information
         70 --  Set File Attributes

NCP:     79 --  Set File Extended Attribute

CHANGES: 30 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP79FileSetExtAttr
(
   pNWAccess pAccess,
   nuint8   buNewExtAttrs,
   nuint8   buDirHandle,
   nuint8   buSrchAttrs,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
)
{
   #define NCP_FUNCTION    ((nuint) 79)
   #define NCP_REQ_LEN     ((nuint) 4)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   abuReq[0] = buNewExtAttrs;
   abuReq[1] = buDirHandle;
   abuReq[2] = buSrchAttrs;
   abuReq[3] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrFileName;
   reqFrag[1].uLen  = buFileNameLen;

   return NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/79.c,v 1.7 1994/09/26 17:39:02 rebekah Exp $
*/
