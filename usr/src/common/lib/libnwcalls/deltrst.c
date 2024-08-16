/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:deltrst.c	1.4"
#include "ntypes.h"
#include "nwdentry.h"

/*manpage*NWDeleteTrusteeFromDirectory **************************************
SYNTAX:  NWCCODE N_API NWDeleteTrusteeFromDirectory
         (
            NWCONN_HANDLE  conn,
            NWDIR_HANDLE   dirHandle,
            pnstr8         path,
            nuint32        objID
         )

REMARKS:

INCLUDE: nwdentry.h

RETURN:  0x0000  Successful

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 18 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/

NWCCODE N_API NWDeleteTrusteeFromDirectory
(
   NWCONN_HANDLE  conn,
   NWDIR_HANDLE   dirHandle,
   pnstr8         path,
   nuint32        objID
)
{
  return NWDeleteTrustee(conn, dirHandle, path, objID);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/deltrst.c,v 1.6 1994/06/08 23:08:44 rebekah Exp $
*/
