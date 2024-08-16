/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s203.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWDisableFileServerLogin*********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s203DisableServerLogin
         (
            pNWAccess pAccess,
         )

REMARKS: This call allows an operator to instruct the file server to refuse new
         login requests.  This call is usually made during some crucial time
         (before taking the server down, for instance).

         We recommend that caution be used with this call. If, after making the
         call, the operator loses or destroys his or her service connection to
         the file server, he or she cannot create a new connection and,
         therefore, cannot log in again.  If no other user on the server has
         operator privileges, the server must be brought down from the console
         connected to the server and re-booted before any new users (including
         the operator) can access it.

         If the calling station lacks operator privileges, the No Console
         Rights completion code is returned and the server's log state remains
         unchanged.


ARGS: <> pAccess

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 204   Enable File Server Login
         23 205   Get File Server Login Status
         23 211   Down File Server

NCP:     23 203   Disable File Server Login

CHANGES: 17 Aug 1993 - written - dromrell
-------------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
******************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s203DisableServerLogin
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION            ((nuint)     23)
   #define NCP_SUBFUNCTION         ((nuint8)   203)
   #define NCP_STRUCT_LEN          ((nuint16)    1)

   return ((NWRCODE) NWCRequestSimple(pAccess, NCP_FUNCTION, NCP_STRUCT_LEN, NCP_SUBFUNCTION));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s203.c,v 1.7 1994/09/26 17:35:57 rebekah Exp $
*/
