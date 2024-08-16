/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:subaccho.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"

#include "nwcaldef.h"   /* needs to be included before nwclocal.h for N_API */
#define NWL_EXCLUDE_TIME
#define NWL_EXCLUDE_FILE
#include "nwlocale.h"
#include "nwacct.h"

#include "ncpacct.h"

/*manpage*NWSubmitAccountHold***********************************************
SYNTAX:  NWCCODE N_API NWSubmitAccountHold
         (
            NWCONN_HANDLE  conn,
            nuint16        objType,
            pnstr8         objName,
            nint32         holdAmt
         )

REMARKS:

ARGS: >  conn

      >  objType
         The type of the bindery object for which the status is desired

      >  objName
         The name of the bindery object for which the status is desired

      >  holdAmt
         The amount to be held against the object's account balance

INCLUDE: nwacct.h

RETURN:  0x0000    Successful
         0x8801    Invalid Connection ID
         0x8901    Out Of Disk Space
         0x8988    Invalid File Handle
         0x8994    No Write Privileges
         0x8996    Server Out Of Memory
         0x89A2    I/O Lock Error
         0x89C0    No Account Privileges
         0x89C1    No Account Balance
         0x89C2    Credit Limit Exceeded
         0x89C3    Too Many Holds
         0x89C4    Account Disabled
         0x89E8    Write To Group
         0x89EA    No Such Member
         0x89EB    Property Not Set Property
         0x89EC    No Such Set
         0x89FC    No Such Object
         0x89FE    Directory Locked
         0x89FF    Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 152  Submit Account Hold

CHANGES: 21 Sep 1993 - NWNCP Enabled - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSubmitAccountHold
(
   NWCONN_HANDLE  conn,
   nuint16        objType,
   pnstr8         objName,
   nint32         holdAmt
)
{
   nuint8 nameLen;

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   nameLen = (nuint8) NWLTruncateString(objName, 48);

   return ((NWCCODE) NWNCP23s152SubmitAccountHold(&access, holdAmt,
            NSwap16(objType), nameLen, objName));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/subaccho.c,v 1.7 1994/09/26 17:50:11 rebekah Exp $
*/
