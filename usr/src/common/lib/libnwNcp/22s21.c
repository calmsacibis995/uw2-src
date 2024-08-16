/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:22s21.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP22s21VolGetInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP22s21VolGetInfo
         (
            pNWAccess pAccess,
            nuint8   buDirHandle,
            pnuint16 psuSectorsPerCluster,
            pnuint16 psuTotalVolClusters,
            pnuint16 psuAvailClusters,
            pnuint16 psuTotalDirSlots,
            pnuint16 psuAvailDirSlots,
            pnstr8   pbstrVolNameB16,
            pnuint16 psuRemovableFlag,
         );

REMARKS: This call allows a client to obtain information about the physical
         limitations of one of the file server's volumes.  All volumes use logical
         sector sizes of 512 bytes per sector.  If the physical media uses a
         different sector size, the server must perform appropriate mappings.  Volume
         space is allocated in groups of sectors called clusters.

         Sectors Per Cluster indicates how many 512-byte sectors are contained in each
         cluster.

         Total Volume Clusters indicates how many clusters make up the server volume.

         Available Clusters indicates how many clusters are not currently in use.

         Total Directory Slots indicates how many physical directory entries the
         volume contains.  If this information is meaningless under a given server's
         implementation, Total Directory Slots should be set to 0xFFFF.

         Available Directory Slots indicates how many of the total directory entries
         on the volume are still available for use.  If this information is
         meaningless under a given server implementation, Available Directory Slots
         should be set to 0xFFFF.

         Volume Name contains the name of the volume whose statistics are being
         reported.  This field will be null padded.

         Removable Flag is zero if the volume is on a fixed media and 0xFFFF if the
         volume is on a removable (mountable) media.

ARGS: <> pAccess
      >  buDirHandle
      <  psuSectorsPerCluster  (optional)
      <  psuTotalVolClusters   (optional)
      <  psuAvailClusters      (optional)
      <  psuTotalDirSlots      (optional)
      <  psuAvailDirSlots      (optional)
      <  pbstrVolNameB16       (optional)
      <  psuRemovableFlag      (optional)

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x89FF  Failure

SERVER:

CLIENT:  DOS OS2 WIN NT

SEE:     18 --  Get Volume Info With Number

NCP:     22 21  Get Volume Info With Handle

CHANGES: 14 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP22s21VolGetInfo
(
   pNWAccess pAccess,
   nuint8   buDirHandle,
   pnuint16 psuSectorsPerCluster,
   pnuint16 psuTotalVolClusters,
   pnuint16 psuAvailClusters,
   pnuint16 psuTotalDirSlots,
   pnuint16 psuAvailDirSlots,
   pnstr8   pbstrVolNameB16,
   pnuint16 psuRemovableFlag
)
{
   #define NCP_FUNCTION    ((nuint) 22)
   #define NCP_SUBFUNCTION ((nuint8) 21)
   #define NCP_STRUCT_LEN  ((nuint16) 2)
   #define NCP_REQ_LEN     ((nuint) (2 + NCP_STRUCT_LEN))
   #define NCP_REPLY_LEN   ((nuint) 30)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0],&suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buDirHandle;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN,
               abuReply, NCP_REPLY_LEN, NULL);
   if(lCode == 0)
   {
      if(psuSectorsPerCluster !=NULL)
         NCopyHiLo16(psuSectorsPerCluster, &abuReply[0]);

      if(psuTotalVolClusters !=NULL)
         NCopyHiLo16(psuTotalVolClusters, &abuReply[2]);

      if(psuAvailClusters !=NULL)
         NCopyHiLo16(psuAvailClusters, &abuReply[4]);

      if(psuTotalDirSlots !=NULL)
         NCopyHiLo16(psuTotalDirSlots, &abuReply[6]);

      if(psuAvailDirSlots !=NULL)
         NCopyHiLo16(psuAvailDirSlots, &abuReply[8]);

      if(pbstrVolNameB16!=NULL)
         NWCMemMove(pbstrVolNameB16, &abuReply[10],(nuint) 16);

      if(psuRemovableFlag !=NULL)
         NCopyLoHi16(psuRemovableFlag, &abuReply[26]);
   }
   return((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/22s21.c,v 1.7 1994/09/26 17:34:01 rebekah Exp $
*/
