/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:geteahan.c	1.4"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwea.h"

/*manpage*NWGetEAHandleStruct***********************************************
SYNTAX:  NWCCODE N_API NWGetEAHandleStruct
         (
            NWCONN_HANDLE conn,
            pnstr8 EAName,
            NW_IDX N_FAR * idxStruct,
            NW_EA_HANDLE N_FAR * EAHandle
         )

REMARKS: Fills the NW_EA_HANDLE structure so that it can be used by NWReadEA
         and NWWriteEA.

         The findfirst/next functions will also return an NW_EA_HANDLE
         structure, properly filled in.

ARGS:

INCLUDE: nwnamspc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetEAHandleStruct
(
   NWCONN_HANDLE  conn,
   pnstr8         EAName,
   NW_IDX N_FAR *   idxStruct,
   NW_EA_HANDLE N_FAR * EAHandle
)
{
   if(EAName == NULL || idxStruct == NULL || EAHandle == NULL)
      return (0x89ff);

   EAHandle->connID    = conn;
   EAHandle->rwPosition= 0L;
   EAHandle->EAHandle  = 0L;
   EAHandle->volNumber = idxStruct->volNumber;
   EAHandle->dirBase   = idxStruct->srcDirBase;
   EAHandle->keyUsed   = 0;
   EAHandle->keyLength = NWCStrLen(EAName);
   NWCMemMove(EAHandle->key, EAName, EAHandle->keyLength + 1);

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/geteahan.c,v 1.6 1994/06/08 23:09:28 rebekah Exp $
*/
