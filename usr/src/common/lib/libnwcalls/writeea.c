/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:writeea.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpea.h"
#include "nwerror.h"
#include "nwcaldef.h"
#include "nwea.h"

/*manpage*NWWriteEA*********************************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWCCODE ) NWWriteEA
         (
            NW_EA_HANDLE NWPTR EAHandle,
            nuint32 totalWriteSize,
            nuint32 bufLen,
            pnuint8 buffer,
            pnuint32 amountWritten
         )

REMARKS: Writes data to an EA. If the EA does not exist, the server will
         attempt to create it.

         NWGetEAHandleStruct or NWFindFirst/NextEA MUST be called prior to
         calling NWWriteEA (or NWReadEA).  If the NW_EA_HANDLE structure is
         not initialized properly this call may cause the server to crash.

         If the data to be written is larger than the bufLen, this
         function should be called multiple times to write all the data to
         the EA.

         The bufLen should be in multiples of 512 bytes.

ARGS: <> EAHandle
         Pointer to a NW_EA_HANDLE structure returned by
         NWGetEAHandleStruct, NWFindFirstEA or NWFindNextEA.

       > totalWriteSize
         Size of total write.

       > bufLen
         The application uses this parameter to specify the length of
         buffer.

       > buffer
         Pointer to a data buffer

       > amountWritten
         The amount of data written this call. This is NOT
         accumulative across multiple calls!

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     86 02  Write Extended Attribute

CHANGES:
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWCCODE )
NWWriteEA
(
   NW_EA_HANDLE NWPTR EAHandle,
   nuint32           totalWriteSize,
   nuint32           bufLen,
   pnuint8           buffer,
   pnuint32          amountWritten
)
{
   NWCCODE  ccode;
   nuint32  bufferLeft, writeLeft;
   nuint32  luTotalAmountWritten;
   nuint16  suFlags;
   nuint32  luWritePosition;
   nuint32  luTtlWriteSize;
   nuint32  luAccessFlag;
   nuint16  suValueLen;
   nuint16  suKeyLen;  /* will be set to 0 */
   nuint32  luErrorCode;
   nuint32  luBytesWritten;
   nuint32  luNewEAHandle;
   NWNCPEAHandle NCPEAHandle;

   NWCDeclareAccess(access);

   NWCSetConn(access, EAHandle->connID);

   luAccessFlag = 0L;

   if(amountWritten)
      *amountWritten    = 0L;
   luTotalAmountWritten = 0L;

   if(bufLen < 512 && totalWriteSize > bufLen)
      return(INVALID_BUFFER_LENGTH);

   if(EAHandle->EAHandle)
   {
      NCPEAHandle.TYPE_10.luEAHandle = EAHandle->EAHandle;
      suFlags        = (nuint16) 2;
   }
   else
   {
      NCPEAHandle.TYPE_00.luVolNum  = EAHandle->volNumber;
      NCPEAHandle.TYPE_00.luDirBase = EAHandle->dirBase;
      suFlags        = (nuint16) 0;
   }

   luTtlWriteSize = totalWriteSize;

   if(EAHandle->keyUsed)
   {
      suKeyLen       = (nuint16) 0;
      suValueLen     = (nuint16) 512;
   }
   else
   {
      suKeyLen       = EAHandle->keyLength;
      suValueLen     = (nuint16) 256;
   }

   bufferLeft = bufLen;

   do
   {
      writeLeft = totalWriteSize - EAHandle->rwPosition;
      if(writeLeft < 512)
      {
         if(writeLeft > bufferLeft)
         {
            return (0);   /* will never happen first time through since buffer */
         }
         else             /* must be 512 bytes or smaller than totalWriteSize  */
         {
            suValueLen = (nuint16) writeLeft;
         }
      }

      luWritePosition  = EAHandle->rwPosition;

      if ((ccode = (NWCCODE)NWNCP86s2EAWrite( &access, suFlags,
            &NCPEAHandle, luTtlWriteSize, luWritePosition, luAccessFlag,
            suValueLen, suKeyLen, EAHandle->key, buffer, &luErrorCode,
            &luBytesWritten, &luNewEAHandle)) != 0)
      {
         return ccode;
      }

      if(luErrorCode)
         return((NWCCODE) luErrorCode);

      EAHandle->EAHandle   = luNewEAHandle;
      NCPEAHandle.TYPE_10.luEAHandle = luNewEAHandle;
      EAHandle->rwPosition += luBytesWritten;
      luTotalAmountWritten += luBytesWritten;
      if(amountWritten)
         *amountWritten    = luTotalAmountWritten;
      buffer               += (int) luBytesWritten;

      if(!EAHandle->keyUsed)
      {
         EAHandle->keyUsed = 1;
         suKeyLen          = (nuint16) 0;
         suValueLen        = (nuint16) (bufferLeft - luBytesWritten);
         suFlags           = (nuint16) 2;
      }
      bufferLeft -= luBytesWritten;

      if(EAHandle->rwPosition >= totalWriteSize)
         return(EA_EOF);

   } while(luTotalAmountWritten < bufLen);

   return(0);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/writeea.c,v 1.8 1994/09/26 17:50:33 rebekah Exp $
*/

