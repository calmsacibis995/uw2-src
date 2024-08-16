/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ttssetfl.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncptts.h"
#include "nwtts.h"
#include "nwcaldef.h"

/*manpage*NWTTSSetControlFlags**********************************************
SYNTAX:  NWCCODE N_API NWTTSSetControlFlags
         (
            NWCONN_HANDLE conn,
            nuint8 buFlags
         );

REMARKS:
         Allows a client to set the transaction bits in the
         Control Flags byte. Only bit 0 is used currently, if bit 0 is set
         forced record locking is "on".  The default setting is record
         locking "on".

ARGS: >  conn
      >  buFlags

INCLUDE: nwtts.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     34 09  TTS Get Transaction Bits

NCP:     34 10  TTS Set Transaction Bits

CHANGES: 13 Aug 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWTTSSetControlFlags
(
   NWCONN_HANDLE conn,
   nuint8 buFlags
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ( (NWCCODE) NWNCP34s10TTSSetTransBits(
                         &access, buFlags) );
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ttssetfl.c,v 1.7 1994/09/26 17:50:26 rebekah Exp $
*/
