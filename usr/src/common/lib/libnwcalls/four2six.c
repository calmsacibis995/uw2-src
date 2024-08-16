/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:four2six.c	1.5"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwmisc.h"

/*manpage*_NWConvert4ByteTo6ByteHandle**************************************
SYNTAX:  void N_API _NWConvert4ByteTo6ByteHandle
         (
            pnuint8 NWHandleB4,
            pnuint8 NWHandleB6,
         )

REMARKS:

ARGS: >  NWHandleB4
      <  NWHandleB6

INCLUDE: nwmisc.h

RETURN:

CLIENT:  DOS WIN OS2 NT NLM

SEE:

CHANGES: 20 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
void N_API _NWConvert4ByteTo6ByteHandle
(
   pnuint8 NWHandleB4,
   pnuint8 NWHandleB6
)
{

   #if defined(N_PLAT_UNIX)

    UINT16  temp;
    UINT32  temp4Handle;

    NWCMemCpy((char *)&temp4Handle, (char *)NWHandleB4, 4);
    temp4Handle = NSwapHiLo32(temp4Handle);

    (void) NWCMemCpy((void *)&temp, (void *)&temp4Handle, sizeof(UINT16));
    temp += 1;

    (void) NWCMemCpy((void *)NWHandleB6, (void *)&temp, sizeof(UINT16));
    (void) NWCMemCpy((void *)&NWHandleB6[2], (void *)&temp4Handle,
                                 sizeof(UINT32));

   #elif defined(N_PLAT_NLM)

   nuint16  suTemp1;

   suTemp1 = *(nuint16 *)&NWHandleB4[0];
   suTemp1 += 1;
   NCopy16(&NWHandleB6[0], &suTemp1);

   NCopySwap32(&NWHandleB6[2], &NWHandleB4[0]);

   #else


   nuint16 suTemp1, suTemp2;

   NCopyLoHi16(&suTemp1, &NWHandleB4[0]);
   NCopyLoHi16(&suTemp2, &NWHandleB4[2]);

   NCopy16(&NWHandleB6[2], &suTemp1);
   suTemp1++;
   NCopy16(&NWHandleB6[0], &suTemp1);
   NCopy16(&NWHandleB6[4], &suTemp2);

   #endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/four2six.c,v 1.7 1994/09/26 17:45:33 rebekah Exp $
*/
