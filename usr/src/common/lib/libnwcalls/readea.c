/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:readea.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpea.h"
#include "nwerror.h"
#include "nwcaldef.h"
#include "nwea.h"


/*manpage*NWReadEA**********************************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWReadEA
         (
            NW_EA_HANDLE NWPTR EAHandle,
            nuint32 bufferSize,
            pnuint8 buffer,
            pnuint32 totalEASize,
            pnuint32 amountRead
         )

REMARKS: Read the next block of data from the specified
         EA. The data block to be read is determined from the state
         information in the EAHandle.

         Before calling NWReadEA initially, the EAHandle and the keyInfo
         should have been obtained by making a call to NWFindFirstEA or
         NWFindNextEA.

         NWReadEA can be called multiple times until the amount of bytes of
         data read are equal to the totalDataLength.

         The maximum bufferSize can be 512 bytes.

ARGS: <> EAHandle
         Pointer to NW_EA_HANDLE structure returned by call to
         NWGetEAHandleStruct, NWFindFirstEA or NWFindNextEA.

      >  bufferSize
         The bufferSize MUST be at least 512 bytes long.

         EA's are read in 512 byte chunks, except for the last chunk.
         bufferSize's greater than 512 bytes should be a multiple of
         512 bytes.

      <  buffer
         Pointer to a data buffer.

      <  totalEASize
         Size of EA. If doing a write, must use this value.

      <  amountRead
         total amount of data read this call. This is NOT accumulative
         across multiple calls.

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     86 03 Read Extended Attribute

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1992 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWReadEA
(
   NW_EA_HANDLE NWPTR EAHandle,
   nuint32           bufferSize,
   pnuint8           buffer,
   pnuint32          totalEASize,
   pnuint32          amountRead
)
{
   NWCCODE  ccode;
   nuint32  bufferLeft;

   nuint16  suFlags;
   nuint32  luReadPosition;
   nuint32  luInspectSize;
   nuint16  suKeyLen;  /* will be set to zero */
   nuint32  luErrorCode;
   nuint32  luTtlValuesLen;
   nuint32  luNewEAHandle;
   nuint32  luAccessFlag;
   nuint16  suValuesLen = (nuint16) bufferSize;
   NWNCPEAHandle NCPEAHandle;

   NWCDeclareAccess(access);

   NWCSetConn(access, EAHandle->connID);

   if(amountRead)
      *amountRead = 0L;
   if(totalEASize)
      *totalEASize = 0L;

   if(bufferSize < 512)
      return(INVALID_BUFFER_LENGTH);

   luInspectSize   = (nuint32) -1;

   if(EAHandle->EAHandle)
   {
      NCPEAHandle.TYPE_10.luEAHandle = EAHandle->EAHandle;
      suFlags       = 2;
   }
   else
   {
      NCPEAHandle.TYPE_00.luVolNum  = EAHandle->volNumber;
      NCPEAHandle.TYPE_00.luDirBase = EAHandle->dirBase;
      suFlags       = 0;
   }

   if(EAHandle->keyUsed)
   {
      suKeyLen     = 0;
   }
   else
   {
      suKeyLen     = EAHandle->keyLength;
   }

   bufferLeft = bufferSize;
   for(;;)
   {
      luReadPosition  = EAHandle->rwPosition;
      ccode = (NWCCODE) NWNCP86s3EARead(  &access, suFlags,
                           &NCPEAHandle, luReadPosition,
                           luInspectSize, suKeyLen, EAHandle->key,
                           &luErrorCode, &luTtlValuesLen, &luNewEAHandle,
                           &luAccessFlag, &suValuesLen, buffer);
      if(ccode)
         return (ccode);

      if(luErrorCode)
         return ((NWCCODE)luErrorCode);

      if(!EAHandle->keyUsed)
      {
         EAHandle->keyUsed    = 1;
         suKeyLen             = 0;
         suFlags              = 2;
      }

      EAHandle->EAHandle = luNewEAHandle;
      NCPEAHandle.TYPE_10.luEAHandle = luNewEAHandle;
      EAHandle->rwPosition += (nuint32) suValuesLen;
      /*
         the rwPosition is accumulative between calls and the amountRead is
         applicable ONLY to the current call.
      */

      if(totalEASize)
         *totalEASize = luTtlValuesLen;
      if(amountRead)
         *amountRead += (nuint32) suValuesLen;

      buffer      += suValuesLen;
      bufferLeft  -= (nuint32) suValuesLen;

      if(EAHandle->rwPosition >= luTtlValuesLen)
         return (EA_EOF);

      if(!bufferLeft)
         break;

      if(bufferLeft < 512)
      {
         if( (luTtlValuesLen - EAHandle->rwPosition) > bufferLeft)
            break;
         else
            suValuesLen = (nuint16) bufferLeft;
      }
   }

   return (0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/readea.c,v 1.8 1994/09/26 17:48:47 rebekah Exp $
*/

