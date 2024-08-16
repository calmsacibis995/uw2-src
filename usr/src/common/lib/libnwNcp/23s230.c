/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s230.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s230GetObjFreeDiskSpace*************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s230GetObjFreeDiskSpace
         (
            pNWAccess pAccess,
            nuint32  luObjID,
            pnuint32 pluSysIntervalMarker,
            pnuint32 pluRetObjID,
            pnuint32 pluUnusedDiskBlocks,
            pnuint8  pbuRestrictionsEnforced
         )

REMARKS: This call returns the number of unused disk blocks available to the
         specified Object ID.  A client can only obtain the information for the
         Object ID by which that client logged in; clients must have console
         operator rights to make this call for any other Object ID.

         The Object ID indicates which Object ID requested the information.

         System Interval Marker indicates how long the file server has been up.
         This value is returned in units of approximately 1/18 of a second and
         is used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, the value
         wraps back to zero.

         Configured Max Open Files contains the number of files the server can
         open simultaneously.

         Unused Disk Blocks indicates how many blocks the file server has
         available to allocate to a bindery object.

         Restrictions Enforced indicates whether the file server operating
         system can place limitations on disk resources (0x00 = enforced;
         0xFF = not enforced).

ARGS: <> pAccess
      >  luObjID
      <  pluSysIntervalMarker (optional)
      <  pluRetObjID (optional)
      <  pluUnusedDiskBlocks
      <  pbuRestrictionsEnforced (optional)

INCLUDE: ncpbind.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 230  Get Object's Remaining Disk Space

CHANGES: 25 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s230GetObjFreeDiskSpace
(
   pNWAccess pAccess,
   nuint32  luObjID,
   pnuint32 pluSysIntervalMarker,
   pnuint32 pluRetObjID,
   pnuint32 pluUnusedDiskBlocks,
   pnuint8  pbuRestrictionsEnforced
)
{
   #define NCP_FUNCTION       ((nuint)    23)
   #define NCP_SUBFUNCTION    ((nuint8)  230)
   #define NCP_STRUCT_LEN     ((nuint16)   5)
   #define NCP_REQ_LEN        ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REP_LEN        ((nuint)    13)

   nint32  lCode;
   nuint16 suNCPLen;
   nuint8  abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo32(&abuReq[3], &luObjID);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
         abuReply, NCP_REP_LEN, NULL);

   if (lCode == 0)
   {
      if (pluSysIntervalMarker)
         NCopyHiLo32(pluSysIntervalMarker, &abuReply[0]);

      if (pluRetObjID)
         NCopyHiLo32(pluRetObjID, &abuReply[4]);

      NCopyLoHi32(pluUnusedDiskBlocks, &abuReply[8]);

      if (pbuRestrictionsEnforced)
         *pbuRestrictionsEnforced = abuReply[12];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s230.c,v 1.7 1994/09/26 17:36:38 rebekah Exp $
*/
