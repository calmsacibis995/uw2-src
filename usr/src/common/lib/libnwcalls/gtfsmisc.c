/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtfsmisc.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpserve.h"

#include "nwcaldef.h"
#include "nwserver.h"

/*manpage*NWGetFileServerMiscInfo*******************************************
SYNTAX:  NWCCODE N_API NWGetFileServerMiscInfo
         (
            NWCONN_HANDLE conn,
            NW_FS_INFO N_FAR * fsInfo
         );

REMARKS: Gets miscellaneous information about a 2.2 server

ARGS: >  conn
      <  fsInfo
         typedef struct tNW_MEM_AREAS
         {
           nuint32 total;    total amount of memory in dynamic memory area
           nuint32 max;      amount of memory in dynamic memory area that has been in use since server was brought up
           nuint32 cur;      amount of memory in dynamic memory area currently in use
         } NW_DYNAMIC_MEM;

         typedef struct tNW_FS_MISC
         {
           nuint32 upTime;        how long file server's been up in 1/18 ticks (wraps at 0xffffffff)
           nuint8 processor;      1 = 8086/8088, 2 = 80286
           nuint8 reserved;
           nuint8 numProcs;       number processes that handle incoming service requests
           nuint8 utilization;    server utilization percentage (0-100), updated once/sec
           nuint16 configuredObjs; max number of bindery objects file server will track - 0=unlimited & next 2 fields have no meaning
           nuint16 maxObjs;        max number of bindery objects that have been used concurrently since file server came up
           nuint16 curObjs;        actual number of bindery objects currently in use on server
           nuint16 totalMem;       total amount of memory installed on server
           nuint16 unusedMem;      amount of memory server has determined it is not using
           nuint16 numMemAreas;    number of dynamic memory areas (1-3)
           NW_DYNAMIC_MEM dynamicMem[3];
         } NW_FS_INFO;

INCLUDE: nwserver.h

RETURN:  0x89C6   NO_CONSOLE_PRIVILEGES

SERVER:  2.2

CLIENT:  DOS WIN OS2 NT

SEE:     23 232  Get File Server Misc Information

CHANGES: 28 May 1993 - written - jwoodbur
         10 Sep 1993 - NWNCP Enabled - jsumsion
****************************************************************************/
NWCCODE N_API NWGetFileServerMiscInfo
(
   NWCONN_HANDLE     conn,
   NW_FS_INFO N_FAR *  fsInfo
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE) NWNCP23s232GetServerMiscInfo(&access,
               (pNWNCPMiscServerInfo) fsInfo));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtfsmisc.c,v 1.7 1994/09/26 17:46:59 rebekah Exp $
*/
