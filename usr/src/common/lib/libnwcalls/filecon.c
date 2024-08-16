/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:filecon.c	1.5"
#include "nwcaldef.h"
#include "nwmisc.h"
#include "nwundoc.h"
#include "nwerror.h"

/*manpage*NWGetFileConnectionID*********************************************
SYNTAX:  NWCCODE N_API NWGetFileConnectionID
         (
            NWFILE_HANDLE fileHandle,
            NWCONN_HANDLE NWPTR conn
         )

REMARKS: This function returns the connection handle of the file server
         on which the specified file resides. It calls NWConvertFileHandle
         to do so.

         A connection handle cannot be obtained from NETX and
         UNKNOWN_REQUEST will be returned.

ARGS: >  fileHandle
      <  conn

INCLUDE: nwmisc.h

RETURN:  UNKNOWN_REQUEST  DOS/NETX only

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     NWConvertFileHandle

CHANGES: 27 Aug 1993 - Bible Converted - srydalch
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetFileConnectionID
(
   NWFILE_HANDLE fileHandle,
   NWCONN_HANDLE NWPTR conn
)
{
   nuint8 tNWHandle[6];

#ifdef N_PLAT_DOS
   if(_NWShellIsLoaded == _NETX_COM)
      return UNKNOWN_REQUEST;
#endif

   return ((NWCCODE) NWConvertFileHandle( (NWFILE_HANDLEINT) fileHandle,
                        (nuint16) 6, &tNWHandle[0], conn) );
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/filecon.c,v 1.7 1994/09/26 17:45:28 rebekah Exp $
*/
