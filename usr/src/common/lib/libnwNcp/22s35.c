/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s35.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s35GetDirSpcRest**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s35GetDirSpcRest
         (
            pNWAccess          pAccess,
            nuint8            buDirHandle,
            pnuint8           pbuNumEntries,
            pNWNCPDiskLvlRest pDiskRestB102,
         );

REMARKS: This function scans for the amount of disk space assigned to all directories
         between the current directory and the root directory.  The return buffer
         contains information about the restrictions along the directory path.

         LEVEL refers to the distance from the directory to the root.

         MAX refers to the maximum amount of space assigned to a directory.

         CURRENT refers to the amount of space assigned to a directory minus the
         amount of space used by a directory and its subdirectories.

         To find the actual amount of space available to a directory, scan all the
         current entries and use the smallest one.  Directories which have no
         restrictions will not return any information.  If no entries are returned,
         no space restrictions exist for the specified directory.  All restrictions
         are in 4K blocks.

         When the MAX field is 0x7fffffff, there is no restriction on the entry;
         however, you can still calculate the space in use by subtracting the
         CURRENT from the MAX entry.  When the MAX field is negative, the limit is
         zero.  When the CURRENT field is negative, CURRENT is really zero.  These are
         allowed to go negative so you can still generate a valid "IN USE" value.

ARGS: <> pAccess
      >  buDirHandle
      <  pbuNumEntries
      <  pDiskRestB102

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful

SERVER:  3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     22 33  Add User Disk Space Restriction
         22 34  Remove User Disk Space Restriction
         22 41  Get Object Disk Usage And Restrictions
         22 32  Scan Volume's User Disk Restrictions
         22 36  Set Directory Disk Space Restriction
         22 40  Scan Directory Disk Space

NCP:     22 35  Get Directory Disk Space Restriction

CHANGES: 14 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s35GetDirSpcRest
(
   pNWAccess          pAccess,
   nuint8            buDirHandle,
   pnuint8           pbuNumEntries,
   pNWNCPDiskLvlRest pDiskRest
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 35)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 512)

   nint32   lCode;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);

   if (lCode == 0)
   {
      nuint i, n;

      *pbuNumEntries = abuReply[0];

      for (i = 0, n = 1; i < (nuint)*pbuNumEntries; i++, n += 9)
      {
         pDiskRest->buLevel = abuReply[n];
         NCopyLoHi32(&pDiskRest->luMax, &abuReply[n+1]);
         NCopyLoHi32(&pDiskRest->luCurrent, &abuReply[n+5]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s35.c,v 1.7 1994/09/26 17:34:18 rebekah Exp $
*/
