/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:openfork.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpafp.h"

#include "nwcaldef.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwafp.h"

/*manpage*NWAFPOpenFileFork*************************************************
SYNTAX:  NWCCODE N_API NWAFPOpenFileFork
         (
            NWCONN_HANDLE  conn,
            nuint16        volNum,
            nuint32        AFPEntryID,
            nuint8         forkIndicator,
            nuint8         accessMode,
            pnstr8         AFPPathString,
            pnuint32       fileID,
            pnuint32       forkLength,
            pnuint8        NWHandle,
            NWFILE_HANDLE NWPTR DOSFileHandle
         )

REMARKS: This call opens an AFP file fork (data fork or resource fork).  If a
         nonexistent file fork is specified, a file fork will automatically be
         created and opened.

ARGS: >  conn
      >  volNum
         Volume number of directory entry

      >  AFPEntryID
         AFP base ID

      >  forkIndicator
         Data or Resource fork indicator
          0 = data
          1 = resource

      >  accessMode
         File access mode indicator. READ or WRITE access mode MUST be set.

      >  AFPPathString
         AFP style directory path, relative to AFPbaseID

      <  fileID (optional)
         File entry ID

      <  forkLength (optional)
         Length of the opened fork

      <  NWHandle (optional)
         NW file handle

      <  DOSFileHandle (optional)
         DOS file handle

INCLUDE: nwafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     35 08  AFP Open File Fork

CHANGES: 27 Aug 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWAFPOpenFileFork
(
   NWCONN_HANDLE        conn,
   nuint16              volNum,
   nuint32              AFPEntryID,
   nuint8               forkIndicator,
   nuint8               accessMode,
   pnstr8               AFPPathString,
   pnuint32             fileID,
   pnuint32             forkLength,
   pnuint8              NWHandle,
   NWFILE_HANDLE NWPTR  DOSFileHandle
)
{
   NWCCODE ccode;
   nuint32 nativeLen;
   nuint8	tempNetWareHandle6[6];

   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   if(!(accessMode & 3))
      accessMode |= 0x13;

   if((ccode = (NWCCODE)NWNCP35s8AFPOpenFileFork(&access,
                                                 (nuint8)volNum,
                                                 NSwap32(AFPEntryID),
                                                 forkIndicator,
                                                 accessMode,
                                                 AFPPathString[0],
                                                 AFPPathString + 1,
                                                 fileID,
                                                 &nativeLen,
                                                 tempNetWareHandle6)) == 0)
   {

#ifdef N_PLAT_DOS
      if (_NWShellIsLoaded == _NETX_COM && forkLength)
         *forkLength = nativeLen;  /* LoHi in DOS w/ NETX */
      else
      {
#endif
         /* anti-native for other platforms and other shells in DOS */
         if (forkLength)
            *forkLength = NSwap32(nativeLen);
#ifdef N_PLAT_DOS
      }
#endif

	  if ( fileID )
		*fileID = NSwap32(*fileID);

	  if (NWHandle)
		NWCMemMove( NWHandle, tempNetWareHandle6, 6);

      if (DOSFileHandle)
      {
         ccode = NWConvertHandle(conn,
                  accessMode,
                  tempNetWareHandle6,
                  6,
#ifdef N_PLAT_DOS
                  _NWShellIsLoaded == _NETX_COM ?
                     nativeLen :         /* LoHi */
                     NSwap32(nativeLen), /* HiLo */
#else
                  nativeLen,  /* anti-native */
#endif
                  DOSFileHandle);
       }
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/openfork.c,v 1.7 1994/09/26 17:48:30 rebekah Exp $
*/

