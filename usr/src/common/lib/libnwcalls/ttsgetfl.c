/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttsgetfl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncptts.h"
#include "nwcaldef.h"
#include "nwtts.h"
#include "nwmisc.h"

/*manpage*NWTTSGetControlFlags**********************************************
SYNTAX:  NWCCODE N_API NWTTSGetControlFlags(
            NWCONN_HANDLE conn,
            pnuint8 buFlags)

REMARKS: Returns the transaction bits (Control Flags) for the task.
         The bits are defined as follows:

         Bit    Description
         ---    -----------
         0     0 - forced record locking is off
               1 - forced record locking is on
         1-7    reserved

ARGS:

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     34 09  TTS Get Transaction Bits

CHANGES:  13  Aug 1993 - NWNCP Enabled - rivie

----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSGetControlFlags(
   NWCONN_HANDLE conn,
   pnuint8 buFlags)
{
NWCDeclareAccess(access)
   NWCSetConn(access, conn);

   return (NWCCODE)NWNCP34s9TTSGetTransBits (&access, buFlags);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ttsgetfl.c,v 1.7 1994/09/26 17:50:23 rebekah Exp $
*/
