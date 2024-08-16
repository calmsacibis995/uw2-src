/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:convhand.c	1.5"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#endif

#ifdef N_PLAT_OS2
# include <os2def.h>
# include <bsedos.h>

extern char _NWNetSpecialFile[];
#endif

#include "ntypes.h"
#if defined N_PLAT_WNT || defined(N_PLAT_DOS) || defined(N_PLAT_MSW)
#include "nwclient.h"
#include "nwcint.h"
#endif

#include "nwclient.h"
#include "nwcaldef.h"
#include "nwintern.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwerror.h"

#if defined(N_PLAT_NLM)
#define _NWTYPES_H_INCLUDED
# include <malloc.h>
#endif

/*manpage*NWConvertHandle***************************************************
SYNTAX:  NWCCODE N_API NWConvertHandle
         (
            NWCONN_HANDLE conn,
            nuint8  buAccessMode,
            nptr NWHandle,
            nuint16 suHandleSize,
            nuint32 luFileSize,
            NWFILE_HANDLE NWPTR fileHandle
         )

REMARKS:

ARGS:  > conn
       > buAccessMode
       > NWHandle
       > suHandleSize
       > luFileSize
      <  fileHandle

INCLUDE: nwmisc.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT NLM

SEE:

NCP:

CHANGES: 24 Aug 1992 - written - jwoodbur
         09 Dec 1992 - modified (for OS/2) - jwoodbur
           If high bit is set on handleSize, handle will only be closed
           on client when it is closed. The server handle will remain open.
           This applies ONLY to OS/2.
         27 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWConvertHandle
(
   NWCONN_HANDLE conn,
   nuint8  buAccessMode,
   nptr NWHandle,
   nuint16 suHandleSize,
   nuint32 luFileSize,
   NWFILE_HANDLE N_FAR *fileHandle
)
{
#if defined N_PLAT_WNT  || \
    defined(N_PLAT_DOS) || \
    defined(N_PLAT_MSW)

   nuint8  tNWHandle[6];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if (suHandleSize == 6)
      NWCMemMove(tNWHandle, NWHandle, 6);
   else if (suHandleSize == 4)
   {
      *(pnuint16)tNWHandle     = (*(pnuint16)NWHandle) + 1;
      *(pnuint32)&tNWHandle[2] = *(pnuint32)NWHandle;
   }
   else
      return(INVALID_PARAMETER);

   return ((NWCCODE)NWCConvertNetWareFileHandle(&access, buAccessMode, tNWHandle,
            luFileSize, fileHandle));

#elif defined(N_PLAT_NLM)

   nuint8  tNWHandle[6];
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if (suHandleSize == 6)
      NWCMemMove(tNWHandle, NWHandle, 6);
   else if (suHandleSize == 4)
	   _NWConvert4ByteTo6ByteHandle( (pnuint8) NWHandle, tNWHandle);
   else
      return(INVALID_PARAMETER);

   return ((NWCCODE)NWCConvertNetWareFileHandle(&access, buAccessMode, tNWHandle,
            luFileSize, fileHandle));

#elif defined(N_PLAT_UNIX)
   /* Clear up a compiler warning */
   luFileSize = luFileSize;
   buAccessMode = buAccessMode;


   *fileHandle = (NWFILE_HANDLE)malloc(sizeof(UNIX_NWFILE_STRUCT));

   ((UNIX_NWFILE_STRUCT *)(*fileHandle))->conn = conn;
   ((UNIX_NWFILE_STRUCT *)(*fileHandle))->suHandleType = suHandleSize;

   if (suHandleSize == 6)
      NWCMemCpy(((UNIX_NWFILE_STRUCT *)(*fileHandle))->pbuNWHandle,
											NWHandle, 6);
   else if (suHandleSize == 4)
   {
	  _NWConvert4ByteTo6ByteHandle( (pnuint8) NWHandle,
					((UNIX_NWFILE_STRUCT *)(*fileHandle))->pbuNWHandle);

#ifdef NEVER
      *(pnuint16)((UNIX_NWFILE_STRUCT *)(*fileHandle))->pbuNWHandle =
            (*(pnuint16)NWHandle) + 1;
      *(pnuint32)&((UNIX_NWFILE_STRUCT *)(*fileHandle))->pbuNWHandle[2] =
            *(pnuint32)NWHandle;
#endif
   }
   else {
	  free((void *) *fileHandle);
      return(INVALID_PARAMETER);
   }

   ((UNIX_NWFILE_STRUCT *)(*fileHandle))->luOffset = 0;

   return(0);

#elif defined(N_PLAT_OS2)

   NWCCODE ccode;
   nuint8  tNWHandle[6];
   nint8   aiDataBuf[10];
   nuint16 suAction, suDataLen, suTmpHndl;
   nuint16  openMode;

   suTmpHndl = suHandleSize & 0x8000;

   suHandleSize &= 0x7FFF;

   if (suHandleSize == 6)
      NWCMemMove(tNWHandle, NWHandle, 6);
   else if (suHandleSize == 4)
   {
      *(pnuint16)tNWHandle     = (*(pnuint16)NWHandle) + 1;
      *(pnuint32)&tNWHandle[2] = *(pnuint32)NWHandle;
   }
   else
      return(INVALID_PARAMETER);

   openMode = buAccessMode & 0x03;
   openMode = openMode ? openMode - 1 : OPEN_ACCESS_READWRITE;

   if(buAccessMode & AR_DENY_READ && buAccessMode & AR_DENY_WRITE)
      openMode |= OPEN_SHARE_DENYREADWRITE;
   else if(buAccessMode & AR_DENY_READ)
      openMode |= OPEN_SHARE_DENYREAD;
   else if(buAccessMode & AR_DENY_WRITE)
      openMode |= OPEN_SHARE_DENYWRITE;
   else
      openMode |= OPEN_SHARE_DENYNONE;

   /* what about this one: AR_COMPATIBILITY  0x0010 */

   if(buAccessMode & AR_WRITE_THROUGH)
      openMode |= OPEN_FLAGS_WRITE_THROUGH;

   ccode = DosOpen(_NWNetSpecialFile, (PHFILE) fileHandle,
               (pnuint16)&suAction, luFileSize, 0, 0x01,
               OPEN_SHARE_DENYNONE | OPEN_ACCESS_READWRITE, 0L);
   if(ccode)
      return ccode;

   /* Attach network handle to special file */
   *(pnuint16)aiDataBuf = conn;
   NWCMemMove(&aiDataBuf[2], tNWHandle, 6);

   /* if the high bit of the handle size is set, the NW handle will NOT
      be closed when the client handle is closed */
   *(pnuint16)&aiDataBuf[8] = suTmpHndl ? 0 : 1;
   suDataLen = 10;
   return DosFSCtl(NULL, 0, NULL, aiDataBuf, 10, (pnuint16) &suDataLen,
                     0xC000 | 0x0005, NULL, (HFILE) (*fileHandle), 1, 0L);

#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/convhand.c,v 1.7 1994/09/26 17:44:48 rebekah Exp $
*/
