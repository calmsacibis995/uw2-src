/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:getconid.c	1.5"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include "ntintern.h"
#include <windows.h>
#include "ntypes.h"
#include "nwcint.h"
#else
#include "ntypes.h"
#endif

#include "nwclient.h"

#include "nwintern.h"
#include "nwcint.h"
#ifndef NW_PLAT_OS216
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwerror.h"
#endif

#include "nwconnec.h"

#undef NWGetConnectionID
   NWCCODE N_API NWGetConnectionID
   (
      pnuint8  serverName,
      nuint16  mode,
      NWCONN_HANDLE NWPTR conn,
      pnuint16 connScope
   );

/*manpage*NWGetConnectionHandle**********************************************
SYNTAX:  NWCCODE N_API NWGetConnectionHandle
         (
            pnuint8  serverName,
            nuint16  mode,
            NWCONN_HANDLE NWPTR conn,
            pnuint16 connScope
         )

REMARKS: Returns the connHandle for the specified file server.

         Under OS/2 this is a direct call to the Requester to find out the
         connection ID for the server.

         Under DOS-shell, the shell's server name table, and connection ID
         table are obtained then searched for the specified server name.

         Under DOS-VLM, this is also a direct call to the requester.

ARGS:    > srvName
           Pointer to the name of the server to get handle for

         > mode
           obsolete

         < conn
           Pointer to the connHandle

         < scopeFlag
           obsolete

INCLUDE: nwconnec.h

RETURN:  n/a

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT NLM

SEE:     NWAttachToFileServerByConn
         NWDetachFromFileServer

NCP:     n/a

CHANGES: 23 Apr 1993 - modified - jwoodbur
           added double buffering of server name
         17 Sep 1993 - hungarian notation added - lbendixs
****************************************************************************/
NWCCODE N_API NWGetConnectionHandle
(
   pnuint8        serverName,
   nuint16        mode,
   NWCONN_HANDLE NWPTR conn,
   pnuint16       connScope
)
{
#if defined(N_PLAT_UNIX) || \
    defined(N_PLAT_NLM) || \
    defined N_PLAT_MSW || \
    defined(N_PLAT_DOS)

   NWCConnString  name;
   NWCCODE        ccode;
   nstr           strServiceType[] = "NCP_SERVER";
   NWCDeclareAccess(access);

   /*  Lets avoid compiler warnings */
   mode = mode;
   connScope = connScope;

   name.pString = serverName;
   name.uStringType = NWC_STRING_TYPE_ASCII;
   name.uNameFormatType = NWC_NAME_FORMAT_BIND;

   ccode = (NWCCODE)NWCOpenConnByName(&access, 0, &name, strServiceType,
          NWC_OPEN_PUBLIC | NWC_OPEN_UNLICENSED | NWC_OPEN_EXISTING_HANDLE,
          NWC_TRAN_TYPE_IPX);

   if(ccode == 0x8800)
     ccode = 0;

   *conn = (NWCONN_HANDLE)NWCGetConn(access);

   return(ccode);


#elif defined(N_PLAT_OS2)
   NWCCODE ccode;
   NWCONN_HANDLE tConn;

   if(conn == NULL)
      conn = &tConn;

   mode = mode;
   serverName = serverName;

   ccode = NWCCallGate(_NWC_MAP_SERVER, &connScope);
   if(ccode)
      *conn = 0;
   else
      NWLockConnection(*conn);

   return (ccode);

#endif
}

NWCCODE N_API NWGetConnectionID
(
   pnuint8     serverName,
   nuint16     mode,
   NWCONN_HANDLE NWPTR conn,
   pnuint16    connScope
)
{
   return(NWGetConnectionHandle(serverName, mode, conn, connScope));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/getconid.c,v 1.7 1994/09/26 17:45:46 rebekah Exp $
*/
