/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:1.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpsync.h"

/*manpage*NWNCP1SyncSetFileLock**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP1SyncSetFileLock
         (
            pNWAccess pAccess,
         );

REMARKS: This request locks sharable files that the calling station has open.  It
         is provided for compatibility with previous versions of NetWare.

ARGS: <> pAccess

INCLUDE: ncpsync.h

RETURN:  0x0000  Successful

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     02 --  File Release Lock

NCP:     01 --  File Set Lock

CHANGES: 28 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP1SyncSetFileLock
(
   pNWAccess pAccess
)
{
   return (NWCRequestSingle(pAccess, (nuint) 1, NULL, (nuint) 0,
                        NULL, (nuint) 0, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/1.c,v 1.7 1994/09/26 17:31:28 rebekah Exp $
*/
