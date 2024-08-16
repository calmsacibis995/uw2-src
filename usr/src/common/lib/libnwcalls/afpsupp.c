/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:afpsupp.c	1.4"
#include "ntypes.h"
#include "nwclient.h"
#include "nwcaldef.h"
#include "nwafp.h"
#include "nwvol.h"

/*manpage*NWAFPSupported****************************************************
SYNTAX:  NWCCODE N_API NWAFPSupported
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum
         );

REMARKS: Reports whether or not the Apple File System is supported

ARGS: >  conn
      >  volNum
         Volume number to check for AFP support

INCLUDE: nwafp.h

RETURN:

CLIENT:  DOS WIN OS2 NT

SEE:

CHANGES: 13 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWAFPSupported
(
   NWCONN_HANDLE  conn,
   nuint16        volNum
)
{
   NWCCODE  ccode;
   nuint32  AFPEntryID;
   nstr8    volName[18];

   if((ccode = NWGetVolumeName(conn, volNum, volName)) != 0)
      return (ccode);

   NWCStrCat(volName, (pnstr8) ":");

   return NWAFPGetEntryIDFromPathName(conn, (NWDIR_HANDLE) 0, volName,
                                      &AFPEntryID);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/afpsupp.c,v 1.6 1994/06/08 23:07:32 rebekah Exp $
*/

