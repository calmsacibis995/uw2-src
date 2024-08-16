/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:subaccch.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"

#include "nwacct.h"
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwclocal.h"

#include "ncpacct.h"

/*manpage*NWSubmitAccountCharge*********************************************
SYNTAX:  NWCCODE N_API NWSubmitAccountCharge
         (
            NWCONN_HANDLE  conn,
            nuint16        objType,
            pnstr8         objName,
            nuint16        serviceType,
            nint32         chargeAmt,
            nint32         holdCancelAmt,
            nuint16        noteType,
            pnstr8         note
         );

REMARKS:

ARGS:  > conn

       > objType
         The type of the bindery object to charge

       > objName
         The name of the bindery object to charge

       > serviceType
         Usually the server object type

       > chargeAmt
         The amount to be charged to the object account balance

       > holdCancelAmt
         This should be zero if no holds were made. Otherwise this amount
         is substracted from the hold amounts previously accumulated by
         the server that submits this charge

       > noteType
         The note type

       > note
         A note that is associated with the charge and stored as an
         audit record

INCLUDE: nwacct.h

RETURN:  0x0000    Successful
         0x8901    Out Of Disk Space
         0x8988    Invalid File Handle
         0x8994    No Write Privileges
         0x8996    Server Out Of Memory
         0x89A2    I/O Lock Error
         0x89C0    No Account Privileges
         0x89C1    No Account Balance
         0x89C2    Credit Limit Exceeded
         0x89C4    Account Disabled
         0x89E8    Write To Group
         0x89EA    No Such Member
         0x89EB    Property Not Set Property
         0x89EC    No Such Set
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 151  Submit Account Charge

CHANGES: 21 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSubmitAccountCharge
(
   NWCONN_HANDLE  conn,
   nuint16        objType,
   pnstr8         objName,
   nuint16        serviceType,
   nint32         chargeAmt,
   nint32         holdCancelAmt,
   nuint16        noteType,
   pnstr8         note
)
{
   nuint8 nameLen, noteLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   nameLen = (nuint8) NWLTruncateString(objName, 48);
   noteLen = (nuint8) NWLTruncateString(note, 255);

   return ((NWCCODE) NWNCP23s151SubmitAccountCharge(&access,
            NSwap16(serviceType), chargeAmt, holdCancelAmt,
            NSwap16(objType), noteType, nameLen,
            objName, noteLen, note));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/subaccch.c,v 1.7 1994/09/26 17:50:10 rebekah Exp $
*/

