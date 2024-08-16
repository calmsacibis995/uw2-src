/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttsend.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncptts.h"
#include "nwcaldef.h"
#include "nwtts.h"
#include "nwmisc.h"

/*manpage*NWTTSEndTransaction***********************************************
SYNTAX:  NWCCODE N_API NWTTSEndTransaction
         (
            NWCONN_HANDLE conn,
            pnuint32 pluTransNum
         )

REMARKS: Ends an explicit transaction and returns a Transaction
         Number.  The Transaction Number is used to verify a successful
         transaction conpletion to disk.

ARGS:    >  conn
         <  pluTransNum

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 03  TTS Abort Transaction
         34 01  TTS Begin Transaction
         34 04  TTS Transaction Status

NCP:     34 02  TTS End Transaction

CHANGES: 12 Aug 1993 NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSEndTransaction
(
   NWCONN_HANDLE conn,
   pnuint32 pluTransNum
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP34s2TTSEndTrans(&access, pluTransNum));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ttsend.c,v 1.7 1994/09/26 17:50:21 rebekah Exp $
*/
