/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s28.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s28DelRecover**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s28DelRecover
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint32  luIterHnd,
            nuint8   buFileNameLen,
            pnstr8   pbstrFileName,
         );

REMARKS: This routine will recover the specified salvageable file into the same
         directory.

         This call is replaced by the NetWare 386 v3.11 call Recover Salvageable File
         (0x2222  87  17).

ARGS: <> pAccess
      >  buDirHandle
      >  luIterHnd
      >  buFileNameLen
      >  pbstrFileName

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8984  No Create Privileges
         0x899C  Invalid Path
         0x89FE  File Name Already Exists In This Directory

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 29  Purge Salvageable File
         22 27  Scan Salvageable Files
         87 17  Recover Salvageable File

NCP:     22 28  Recover Salvageable File

CHANGES: 16 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s28DelRecover
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luIterHnd,
   nuint8   buFileNameLen,
   pnstr8   pbstrFileName
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 28)
   #define NCP_STRUCT_LEN  ((nuint16) 7)
   #define NCP_REQ_LEN     ((nuint) 9)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   NCopyLoHi32(&abuReq[4], &luIterHnd);
   abuReq[8] = buFileNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrFileName;
   reqFrag[1].uLen  = buFileNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s28.c,v 1.7 1994/09/26 17:34:09 rebekah Exp $
*/
