/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:17s0.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpprint.h"

/*manpage*NWNCP17s0WriteToSpoolFile**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP17s0WriteToSpoolFile
         (
            pNWAccess pAccess,
            nuint8   buDataLen,
            pnuint8  pbuDataB256,
         );

REMARKS: This call appends the specified data to the end of the client's current
         spool file.  If the client does not have a current spool file, the print
         server creates the file in its work area and writes the data to it.  The
         print server makes no assumptions about the format or contents that it
         writes to a spool file; the client must create a data stream that produces
         the desired output when it is sent to the target printer.

ARGS: <> pAccess
      >  buDataLen
      >  pbuDataB256

INCLUDE: ncpprint.h

RETURN:  0x0000  Successful
         0x8901  Out Of Disk Space
         0x8980  Lock Fail
         0x8981  Out Of Handles
         0x8987  Create Filename Error
         0x8988  Invalid File Handle
         0x898D  Some Files In Use
         0x898E  All Files In Use
         0x898F  Some Read Only
         0x8990  All Read Only
         0x8994  No Write Privileges
         0x8995  File Detached
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x8999  Directory Full Error
         0x89A1  Directory I/O Error
         0x89A2  I/O Lock Error
         0x89FF  Failure, No Files Found

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     17 09  Create Spool File
         17 01  Close Spool File
         17 10  Get Printer's Queue

NCP:     17 00  Write To Spool File

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP17s0WriteToSpoolFile
(
   pNWAccess pAccess,
   nuint8   buDataLen,
   pnuint8  pbuDataB256
)
{
   #define NCP_FUNCTION    ((nuint) 17)
   #define NCP_SUBFUNCTION ((nuint) 0)
   #define NCP_STRUCT_LEN  ((nuint16) (2 + buDataLen))
   #define NCP_REQ_LEN     ((nuint) 4)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN];
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDataLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbuDataB256;
   reqFrag[1].uLen  = buDataLen;

   return(NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/17s0.c,v 1.7 1994/09/26 17:33:11 rebekah Exp $
*/
