/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s204.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWEnableFileServerLogin*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s204EnableServerLogin
         (
            pNWAccess pAccess,
         )

REMARKS: This call allows an operator to instruct the server to begin accepting
         new login requests from clients.  Clients that have already logged in
         will continue to receive service.

         Enabling the server's log state also unlocks the Supervisor's account
         if it has been locked because of intruder detection.

         If the calling station does not have operator priviliges, the No
         Console Rights completion code is returned, and the file server's log
         state remains unchanged.

ARGS: <> pAccess

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 203   Disable File Server Login

NCP:     23 204   Enable File Server Login

CHANGES: 17 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
*****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s204EnableServerLogin
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION         ((nuint)     23)
   #define NCP_SUBFUNCTION      ((nuint8)   204)
   #define NCP_STRUCT_LEN       ((nuint16)    1)

   return (NWCRequestSimple(pAccess, NCP_FUNCTION, NCP_STRUCT_LEN,
                  NCP_SUBFUNCTION));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s204.c,v 1.7 1994/09/26 17:35:58 rebekah Exp $
*/
