/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s68.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s68CloseBindery****************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s68CloseBindery
         (
            pNWAccess pAccess,
         )

REMARKS: Closes the bindery on the file server associated
         with the given connection identification.

         Because the bindery files contain all information about the file
         server's clients, the bindery should be archived on a regular
         basis. However, the file server keeps bindery files open and
         locked at all times so that they cannot be accessed directly.  For
         bindery files to be archived, the bindery must be closed with the
         NWCloseBindery function.

         This function allows SUPERVISOR, or an object that has security
         equivalence to SUPERVISOR, to close and unlock the bindery files,
         thus allowing the bindery to be archived.

         After the bindery files have been archived, the NWOpenBindery
         function is used to give control of the bindery files back to the
         file server.  While the bindery is closed, much of the
         functionality of the network is disabled.

ARGS: <> pAccess,

INCLUDE: ncpbind.h

RETURN:  0x0000 Success
         0x89FF Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     23 69  Open Bindery

NCP:     23 68  Close Bindery

CHANGES: 21 Aug 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s68CloseBindery
(
   pNWAccess pAccess
)

{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 68)
   #define NCP_STRUCT_LEN  ((nuint16) 1)

   return(NWCRequestSimple(pAccess, NCP_FUNCTION, NCP_STRUCT_LEN,
            NCP_SUBFUNCTION));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s68.c,v 1.7 1994/09/26 17:37:30 rebekah Exp $
*/