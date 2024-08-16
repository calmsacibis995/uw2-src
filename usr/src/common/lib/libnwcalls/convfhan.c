/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:convfhan.c	1.5"
#ifdef N_PLAT_OS2
# include <os2def.h>
# include <bsedos.h>
#endif

#include "ntypes.h"
#include "nwclient.h"
#include "nwaccess.h"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwerror.h"
#include "nwconnec.h"

/*manpage*NWConvertFileHandle***********************************************
SYNTAX:  NWCCODE N_API NWConvertFileHandle
         (
            NWFILE_HANDLEINT  fileHandle,
            nuint16           handleType,
            pnuint8           NWHandle,
            NWCONN_HANDLE NWPTR conn
         )

REMARKS: Converts an OS handle to a four or six byte NetWare handle.
         The handle returned by this call should not be used to
         call NWConvertHandle. Doing so will create a new OS file handle.

         If this function is used with only the shell (NETX) running, the
         connection handle will NOT be returned. The function will return
         an INVALID_CONNECTION error in this case - however the netware
         handle will still be valid and conn will be set to zero. If NULL is
         passed for conn, no error will be returned.

ARGS:  > fileHandle
       > handleType
         4 - create a 4 byte netware handle
         6 - create a 6 byte netware handle
      <  NWHandle
      <  conn (optional)

INCLUDE: nwmisc.h

RETURN:  0x0006  invalid handle
         0x8801  INVALID_CONNECTION
           if a pointer is passed for the conn parameter and the shell is
           running, a valid netware handle will be returned as well as this
           error code.

SERVER:  n/a

CLIENT:  DOS WIN OS2 NT

SEE:     NWConvertHandle

NCP:     n/a

CHANGES: 22 Feb 1993 - written - jwoodbur
         27 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWConvertFileHandle
(
   NWFILE_HANDLEINT  fileHandle,
   nuint16           handleType,
   pnuint8           NWHandle,
   NWCONN_HANDLE NWPTR conn
)
{
#if defined(N_PLAT_WNT)
   
   NWCCODE ccode;
   nuint32 connRef;
   NWCDeclareAccess(access);

   if((ccode = NWConvertFileHandleConnRef(fileHandle, handleType, NWHandle, &connRef)) == 0)
   {
      ccode = (NWCCODE)NWCOpenConnByReference(&access, connRef,
            NWC_OPEN_PUBLIC | NWC_OPEN_LICENSED | NWC_OPEN_EXISTING_HANDLE);
      if (ccode != 0)
         return(ccode);

      *conn =  (NWCONN_HANDLE)NWCGetConn(access);
   }

   return (ccode);

#elif defined(N_PLAT_UNIX)
   if (fileHandle == 0)
		return(INVALID_FILE_HANDLE);


   *conn = ((UNIX_NWFILE_STRUCT *)fileHandle)->conn;
   if(handleType == 4)
      NWCMemCpy(NWHandle,
         &(((UNIX_NWFILE_STRUCT *)fileHandle)->pbuNWHandle[2]), 4);
   else
      NWCMemCpy(NWHandle,
         ((UNIX_NWFILE_STRUCT *)fileHandle)->pbuNWHandle, 6);

   return(0);

#elif defined(N_PLAT_OS2)

   NWCCODE  ccode;
   nuint16  retLength;
   SFD_INFO srcFileBuf;

   if((ccode = DosFSCtl((pnuint8) &srcFileBuf, sizeof(srcFileBuf),
                        (pnuint16) &retLength, NULL, 0, NULL,
                        0xC000 | 0x0003, NULL, fileHandle, 1, 0L)) != 0)
   {
      return(ccode == 0x0032 ? 0xff03 : ccode);
   }

   if(handleType == 4)
      NWCMemMove(NWHandle, &srcFileBuf.netwareFileHandle[2], 4);
   else
      NWCMemMove(NWHandle, srcFileBuf.netwareFileHandle, 6);

   if(conn)
      *conn = (NWCONN_HANDLE) srcFileBuf.connID;

   return 0;

#elif defined(N_PLAT_NLM)

   nuint32 tempConn;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   N_UNUSED_VAR (handleType);

   tempConn=*conn;

   ccode = (NWCCODE)NWCConvertLocalFileHandle(&access, fileHandle,&tempConn,NWHandle);

   *conn =(NWCONN_HANDLE) tempConn;

   return(ccode);

#elif defined(N_PLAT_DOS) || defined (N_PLAT_MSW)

   nuint32 tempConn;
   NWCCODE ccode;

   NWCDeclareAccess(access);

   N_UNUSED_VAR (handleType);

   tempConn=*conn;

   ccode = (NWCCODE)NWCConvertLocalFileHandle(&access, fileHandle,&tempConn,NWHandle);

    
   *conn = (NWCONN_HANDLE)(0x0000FFFF & (((nuint32)tempConn) >> 16));
       

   return(ccode);


#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/convfhan.c,v 1.7 1994/09/26 17:44:46 rebekah Exp $
*/
