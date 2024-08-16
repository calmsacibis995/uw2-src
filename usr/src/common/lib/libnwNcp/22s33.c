/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s33.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s33VolAddRestrict**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s33VolAddRestrict
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luObjID,
            nuint32  luSpaceLimit
         );

REMARKS: This function sets an object's volume disk space restriction.  All
         restrictions are in 4K blocks.  Valid restrictions are 0 to 0x40000000.

ARGS: <> pAccess
       > buVolNum
       > luObjID
       > luSpaceLimit

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x898C  No Set Privileges
         0x8996  Server Out of Memory
         0x8998  Invalid Volume

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 35  Get Directory Disk Space Restrictions
         22 34  Remove User Disk Space Restriction
         22 41  Get Object Disk Usage And Restrictions
         22 32  Scan Volume's User Disk Restrictions
         22 36  Set Directory Disk Space Restriction
         22 40  Scan Directory Disk Space

NCP:     22 33  Add User Disk Space Restriction

CHANGES: 21 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s33VolAddRestrict
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luObjID,
   nuint32  luSpaceLimit
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 33)
   #define NCP_STRUCT_LEN  ((nuint16) 10)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 0)

   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luObjID);
   NCopyLoHi32(&abuReq[8], &luSpaceLimit);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               NULL, NCP_REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s33.c,v 1.7 1994/09/26 17:34:16 rebekah Exp $
*/
