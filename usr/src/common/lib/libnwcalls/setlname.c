/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:setlname.c	1.4"
#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWSetLongName*****************************************************
SYNTAX:  NWCCODE N_API NWSetLongName
         (
            NWCONN_HANDLE conn,
            NWDIR_HANDLE dirHandle,
            nuint8 buNameSpace,
            pnstr8 pbstrDestPath,
            nuint16 suDestType,
            pnstr8 pbstrLongName
         )

REMARKS: Given a path specifying an entry name the entry will be renamed in
         the specified name space. This call may rename files in a seemingly
         unpredictable way. It is advised to experiment with it to make sure
         it gives the desired results.

         The dirHandle MUST point to the parent directory.

         The pbstrDestPath and pbstrLongName must be valid names containing
         only one component. No relative paths should be used.

ARGS: >  conn
      >  dirHandle
         The NetWare directory handle pointing to the parent directory.

      >  buNameSpace
         The name space of the pbstrDestPath.

      >  pbstrDestPath
         Pointer to the name of the directory or file to rename.

      >  suDestType
         Whether pbstrDestPath is file (NW_TYPE_FILE) or subdirectory
         (NW_TYPE_SUBDIR).

      >  pbstrLongName
         Pointer to the new name (256 bytes MAX).

INCLUDE: nwnamspc.h

RETURN:  BAD_DIRECTORY_HANDLE   directory handle must point to parent
         INVALID_FILENAME       old or new name were invalid

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 22 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWSetLongName
(
   NWCONN_HANDLE conn,
   NWDIR_HANDLE dirHandle,
   nuint8 buNameSpace,
   pnstr8 pbstrDestPath,
   nuint16 suDestType,
   pnstr8 pbstrLongName
)
{
  return(NWNSRename(conn, dirHandle, buNameSpace, pbstrDestPath,
            suDestType, pbstrLongName, NW_NAME_CONVERT));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/setlname.c,v 1.6 1994/06/08 23:13:29 rebekah Exp $
*/
