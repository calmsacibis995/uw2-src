/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:closefil.c	1.5"
#ifdef N_PLAT_OS2
#include <os2def.h>
#include <bsedos.h>
#endif

#if defined N_PLAT_MSW
#include <windows.h>
#endif

#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwfile.h"
#include "nwerror.h"

#if defined(N_PLAT_NLM)
#define _NWTYPES_H_INCLUDED
# include <malloc.h>
#endif

/*manpage*NWCloseFile*******************************************************
SYNTAX:  NWCCODE N_API NWCloseFile
         (
            NWFILE_HANDLE  fileHandle
         )

REMARKS: Closes a file using a OS handle or a 4 or 6 byte netware handle

ARGS: >  handleSize
         2 = OS handle
         4 = 4 byte NetWare handle
         6 = 6 byte NetWare handle

INCLUDE: nwmisc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT NLM

SEE:

NCP:     66 Close File

CHANGES: 24 Aug 1992 - written - jwoodbur
         27 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWCloseFile
(
   NWFILE_HANDLE  fileHandle
)
{
   NWCONN_HANDLE  conn;
   nuint8         NWHandle[6];
   NWCCODE        ccode;
   nuint8         reserved;
   NWCDeclareAccess(access);

   ccode = NWConvertFileHandle(fileHandle, 6, NWHandle, &conn);
   if (ccode)
      return(ccode);

   NWCSetConn(access, conn);

   reserved = (nuint8) 0;

   ccode = (NWCCODE)NWNCP66FileClose(&access, reserved, NWHandle);
#if defined(N_PLAT_UNIX) || defined( N_PLAT_NLM )
   if (ccode == 0)
      free((void *) fileHandle);
#endif

   return(ccode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/closefil.c,v 1.7 1994/09/26 17:44:26 rebekah Exp $
*/
