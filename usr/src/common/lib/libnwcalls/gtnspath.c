/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtnspath.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWGetNSPath*******************************************************
SYNTAX:  NWCCODE N_API NWGetNSPath
         (
            NWCONN_HANDLE conn,
            nuint8   dirHandle,
            nuint16  fileFlag,
            nuint8   srcNamSpc,
            nuint8   dstNamSpc,
            NW_NS_PATH NWPTR NSPath
         );

REMARKS: Given a specific path, this API will return the full netware path
         for the desired name space. (ie. volume:path\path)

         The newPath buffer should be long enough to hold the longest path
         possible for the dstNamSpc PLUS two extra bytes for working space.

ARGS: >  conn
      >  dirHandle
      >  fileFlag
         1 if sourcePath has a file name in it, 0 if not

      >  srcNamSpc
         Name space used to define sourcePath in NSPath structure

      >  dstNamSpc
         Name space to return the path for.

      <> NSPath
         The following structure is used

         typedef struct
         {
           > pnstr8 srcpath;  valid path
           < pnstr8 dstPath;  buffer to receive full name space path
           > nuint16 dstPathSize;     length of dstPath buffer
         } NW_NS_PATH;

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 28  Get Full Path String

CHANGES: 14 Sep 1993 - NWNCP Enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWGetNSPath
(
   NWCONN_HANDLE  conn,
   nuint8         dirHandle,
   nuint16        fileFlag,
   nuint8         srcNamSpc,
   nuint8         dstNamSpc,
   NW_NS_PATH NWPTR NSPath
)
{
   NWCCODE ccode;
   pnuint8 curComp;
   NWNCPCompPath cPath;
   NWNCPPathCookie cookie;
   nuint8  compLen, components[512];
   nuint16 compSize, compCount, i;
   pnstr8 pDstPath, pEndDstPath;
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);


   cPath.luDirBase    = (nuint32) dirHandle;
   cPath.buHandleFlag = (nuint8)  NWNCP_COMPPATH_USE_DIRHANDLE;
   NWNCPPackCompPath(-1, NSPath->srcPath, srcNamSpc, &cPath, (nflag32) 0);

   cookie.suFlags   = fileFlag;
   cookie.luCookie1 = (nuint32) -1L;
   cookie.luCookie2 = (nuint32) -1L;

   pDstPath = pEndDstPath = &NSPath->dstPath[NSPath->dstPathSize];

   do
   {
      ccode = (NWCCODE) NWNCP87s28GetFullPath(&access, srcNamSpc,
                        dstNamSpc, &cPath, &cookie, &compSize,
                        &compCount, components);
      if (ccode != 0)
         return (ccode);

      if((pDstPath - compSize) <= NSPath->dstPath)
         return (0x89ff);

      curComp = components;
      for(i = compCount; i > 0; i--)
      {
         compLen = *curComp++;

         if(cookie.luCookie2 == (nuint32) -1L && i == 1)
            *--pDstPath = ':';
         else
            *--pDstPath = '\\';

         pDstPath -= compLen;
         NWCMemMove(pDstPath, curComp, compLen);
         curComp += compLen;
      }
   } while(!ccode && cookie.luCookie2 != (nuint32) -1L);

   *(pEndDstPath - 1) = 0;
   NWCMemMove(NSPath->dstPath, pDstPath, (nuint16) (pEndDstPath - pDstPath));

   return (NWCCODE) 0;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtnspath.c,v 1.7 1994/09/26 17:47:19 rebekah Exp $
*/
