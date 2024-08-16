/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s39.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s39TrusteeAddExt******************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s39TrusteeAddExt
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint32  luObjectID,
            nuint16  suTrusteeRights,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
         )

REMARKS: This function adds a trustee and sets the trustee rights for a file or
         directory.

ARGS: <> pAccess
      >  buDirHandle
      >  luObjectID
      >  suTrusteeRights
      >  buPathLen
      >  pbstrPath

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8990  Volume Is Read Only

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     22 13  Add Trustee To Directory
         22 14  Delete Trustee From Directory

NCP:     22 39  Add Extended Trustee To Directory or File

CHANGES: 8 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s39TrusteeAddExt
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luObjectID,
   nuint16  suTrusteeRights,
   nuint8   buPathLen,
   pnstr8   pbstrPath
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 39)
   #define NCP_STRUCT_LEN  ((nuint16) 9 + buPathLen)
   #define NCP_REQ_LEN     ((nuint) 11)
   #define NCP_REQ_FRAGS   ((nuint) 2)
   #define NCP_REPLY_FRAGS ((nuint) 0)

   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN];
   NWCFrag reqFrag[NCP_REQ_FRAGS];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);

   abuReq[2]  = NCP_SUBFUNCTION;
   abuReq[3]  = buDirHandle;
   NCopyHiLo32(&abuReq[4], &luObjectID);
   NCopyLoHi16(&abuReq[8], &suTrusteeRights);
   abuReq[10] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = NCP_REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = buPathLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, NCP_REQ_FRAGS, reqFrag,
               NCP_REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s39.c,v 1.7 1994/09/26 17:34:23 rebekah Exp $
*/
