/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s232.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s232GetServerMiscInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s232GetServerMiscInfo
         (
            pNWAccess pAccess,
            pNWNCPMiscServerInfo pInfo,
         );

REMARKS: This call returns miscellaneous information about the file server.

         System Interval Marker indicates how long the file server has been up.
         This field is returned in units of approximately 1/18 of a second and is
         used to determine the amount of time that has elapsed between
         consecutive calls.  When this field reaches 0xFFFFFFFF, it wraps back
         to zero.

         Processor Type contains a number indicating the processor type:

            0 = Motorola 68000
            1 = Intel 8088 or 8086
            2 = 80286

         Number Of Service Processes indicates the number of processes in the
         server that handles incoming service requests.

         Server Utilization Percentage indicates the current server utilization
         percentage (0..100).  This value is updated once per second.

         Configured Max Bindery Objects indicates the maximum number of
         bindery objects that the file server will track (0 = unlimited).  If disk
         resource limitation is not installed on the file server, this value is 0 and
         an unlimited number of bindery objects can be created.  Otherwise, this
         field contains the number that was configured during installation.  If
         this field is nonzero, the file server will not allow the number of bindery
         objects to exceed this value.  The next two fields have no meaning if this
         field is zero.

         Actual Max Bindery Objects indicates the maximum number of bindery
         objects that have been used concurrently since the file server came up.

         Current Used Bindery Objects indicates the number of bindery objects
         currently in use on the server.

         Total Server Memory indicates the total amount of memory installed in
         the server.
         Wasted Server Memory indicates the amount of memory that the server
         has determined it is not using.

         Number Of Dynamic Memory Areas (1..3) indicates the number of
         dynamic memory areas.

         The following information is repeated up to 3 times, depending on the
         value in Number Of Dynamic Memory Areas.

            Total Dynamic Space indicates the total amount of memory in the
            dynamic memory area.

            Max Used Dynamic Space indicates the amount of memory in the
            dynamic memory area that has been in use since the server was
            brought up.

            Current Used Dynamic Space indicates the amount of memory in the
            dynamic memory area that is currently in use.

ARGS: <> pAccess
      <  pInfo

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory
         0x89C6   No Console Rights

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     23 232  Get File Server Misc Information

CHANGES: 13 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s232GetServerMiscInfo
(
   pNWAccess             pAccess,
   pNWNCPMiscServerInfo pInfo
)
{
   #define NCP_FUNCTION       ((nuint)          23)
   #define NCP_SUBFUNCTION    ((nuint8)        232)
   #define NCP_STRUCT_LEN     ((nuint16)         1)
   #define NCP_REQ_LEN        ((nuint)           3)
   #define NCP_REP_LEN        ((nuint)          56)

   nint32   lCode;
   nuint16 suNCPLen;
   nuint8 abuReq[NCP_REQ_LEN], abuReply[NCP_REP_LEN];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, abuReply,
          NCP_REP_LEN, NULL);
   if (lCode == 0)
   {
      nuint i;

      NCopyHiLo32(&pInfo->luUpTime, &abuReply[0]);

      pInfo->buProcessor       = abuReply[4];
      pInfo->buReserved        = abuReply[5];
      pInfo->buNumServiceProcs = abuReply[6];
      pInfo->buUtilizationPerc = abuReply[7];

      NCopyHiLo16(&pInfo->suConfigMaxObjs, &abuReply[8]);
      NCopyHiLo16(&pInfo->suActualMaxObjs, &abuReply[10]);
      NCopyHiLo16(&pInfo->suCurrUsedObjs, &abuReply[12]);
      NCopyHiLo16(&pInfo->suTotServerMem, &abuReply[14]);
      NCopyHiLo16(&pInfo->suWastedMem, &abuReply[16]);
      NCopyHiLo16(&pInfo->suNumMemAreas, &abuReply[18]);

      if(pInfo->suNumMemAreas > 3)
         pInfo->suNumMemAreas = 3;

      for (i = 0; i < pInfo->suNumMemAreas; i++)
      {
         NCopyHiLo32(&pInfo->aDynMem[i].luTotal, &abuReply[i*12+20]);
         NCopyHiLo32(&pInfo->aDynMem[i].luMaxUsed, &abuReply[i*12+24]);
         NCopyHiLo32(&pInfo->aDynMem[i].luCurrUsed, &abuReply[i*12+28]);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s232.c,v 1.7 1994/09/26 17:36:41 rebekah Exp $
*/
