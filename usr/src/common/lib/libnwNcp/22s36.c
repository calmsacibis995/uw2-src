/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s36.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s36SetDirDiskSpcRest**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s36SetDirDiskSpcRest
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            nuint32  luDiskSpaceLimit,
         );

REMARKS: This function sets a disk restriction for a specific directory.  If the
         restriction is 0, the restriction for the directory is cleared.  If the
         restriction is a negative number, the disk space assigned will be 0.  All
         restrictions are in 4K blocks.

ARGS: <> pAccess
      >  buDirHandle
      >  luDiskSpaceLimit

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8901  Invalid Space Limit
         0x898C  No Set Privileges
         0x89BF  Invalid Name Space

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 35  Get Directory Disk Space Restrictions
         22 33  Add User Disk Space Restriction
         22 34  Remove User Disk Space Restriction
         22 41  Get Object Disk Usage And Restrictions
         22 32  Scan Volume's User Disk Restrictions

NCP:     22 36  Set Directory Disk Space Restriction

CHANGES: 15 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s36SetDirDiskSpcRest
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   nuint32  luDiskSpaceLimit
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 36)
   #define NCP_STRUCT_LEN  ((nuint16) 6)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;
   NCopyLoHi32(&abuReq[4], &luDiskSpaceLimit);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s36.c,v 1.7 1994/09/26 17:34:19 rebekah Exp $
*/
