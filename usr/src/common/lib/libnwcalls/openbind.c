/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:openbind.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "nwcaldef.h"

#include "ncpbind.h"
#include "nwbindry.h"

/*manpage*NWOpenBindery*******************************************************
SYNTAX:  NWCCODE N_API NWOpenBindery
         (
            NWCONN_HANDLE conn
         )

REMARKS: This call reopens the bindery files that have been closed with Close
         Bindery (function 23, subfunction 68).

         Occasionally, bindery files must be closed so that network functions
         can take place.  When the network is backed up, for example, it is
         important to back up current bindery files as well.  Because files
         must be closed before they can be backed up, a Close Bindery call must
         be made.  Archive backup programs can then take temporary control of
         the bindery so that bindery files can be closed, then copied or
         restored.

         When the bindery is closed, only the network supervisor or an object
         that is security equivalent to the supervisor can read from or write
         to the bindery.

         Once the backup is complete, the bindery can be reopened with this
         call.

         The bindery will be reopened automatically if the station that closed
         it disconnects or if the station calls End of Job (function 24) for
         task 0.

         This call will be unsuccessful if the client is not logged in as the
         network supervisor or if the bindery is already open.

ARGS: >  conn

INCLUDE: nwbindry.h

RETURN:  0x0000  Successful
         0x89FF  Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     23 69  Open Bindery

CHANGES: 24 Aug 1993 - NWNCP Enabled - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWOpenBindery
(
  NWCONN_HANDLE conn
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s69OpenBindery(&access));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/openbind.c,v 1.7 1994/09/26 17:48:26 rebekah Exp $
*/
