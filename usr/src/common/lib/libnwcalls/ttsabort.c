/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttsabort.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncptts.h"

#include "nwcaldef.h"
#include "nwtts.h"

/*manpage*NWTTSAbortTransaction**********************************************
SYNTAX:  NWCCODE N_API NWTTSAbortTransaction
         (
            NWCONN_HANDLE conn
         )

REMARKS: Aborts all explicit and implicit transactions.  If a
         transaction is aborted, all writes amde since the beginning of the
         transaction are cancelled and all files are returned to the state
         they were in before the transaction started.

ARGS:  > conn

INCLUDE: ncptts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     34 03  TTS Abort Transaction

CHANGES: 13 Aug 1993 - NWNCP Enabled - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSAbortTransaction
(
   NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP34s3TTSAbortTrans(&access));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ttsabort.c,v 1.6 1994/09/26 17:50:17 rebekah Exp $
*/
