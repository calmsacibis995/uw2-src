/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:ffirstea.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpea.h"

#include "nwcaldef.h"
#include "nwea.h"

NWCCODE N_API _enumEA
(
   NW_EA_FF_STRUCT N_FAR * ffStruct,
   NW_IDX N_FAR * idxStruct
);

/*manpage*NWFindFirstEA*****************************************************
SYNTAX:  NWCCODE N_API NWFindFirstEA
         (
            NWCONN_HANDLE conn,
            NW_IDX N_FAR * idxStruct,
            NW_EA_FF_STRUCT N_FAR * ffStruct,
            NW_EA_HANDLE N_FAR * EAHandle,
            pnstr8 EAName
         )

REMARKS: Initializes find first/next for EA's. The proper ffStruct and
         EAHandle structure will be returned for the first EA (if any).

         The NW_EA_FF_STRUCT and NW_EA_HANDLE structures are used to
         maintain state information for the find first/next and read, write
         EA calls.  These structures are maintained by the API and the
         application should not directly manipulate any of it's data.

ARGS: >  idxStruct
         Pointer to a structure containing the directory entry index

      <  ffStruct
         Pointer to a structure returning a handle to the first EA

      <  EAHandle
         EAHandle structure for EA. NWReadEA and NWWriteEA can be
         called with this structure.  (NWGetEAHandleStruct does NOT
         need to be called after a NWFindFirstEA.)

      <  EAName (optional)
         Name of EA. This will need to be used if NWGetEAHandleStruct
         will be called in preparation for a write (ie. when the
         findfirst/nextEA calls are being used to perform a copy.)

INCLUDE: nwnamspc.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 8 Oct 1991 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWFindFirstEA
(
   NWCONN_HANDLE         conn,
   NW_IDX          N_FAR * idxStruct,
   NW_EA_FF_STRUCT N_FAR * ffStruct,
   NW_EA_HANDLE    N_FAR * EAHandle,
   pnstr8                EAName
)
{
   NWCCODE ccode;

   if(idxStruct == NULL || ffStruct == NULL)
      return (0x89ff);

   EAHandle->connID    = conn;
   EAHandle->EAHandle  = 0L;
   EAHandle->volNumber = idxStruct->volNumber;
   EAHandle->dirBase   = idxStruct->srcDirBase;

   ffStruct->connID      = conn;
   ffStruct->numKeysRead = 0L;
   ffStruct->totalKeys   = 0L;
   ffStruct->sequence    = 0;

   if((ccode = _enumEA(ffStruct, idxStruct)) != 0)
      return (ccode);

   return NWFindNextEA(ffStruct, EAHandle, EAName);
}

/*manpage*NWFindNextEA******************************************************
SYNTAX:  NWCCODE N_API NWFindNextEA(
            NW_EA_FF_STRUCT N_FAR * ffStruct,
            NW_EA_HANDLE N_FAR * EAHandle,
            pnstr8  EAName)

REMARKS: Returns the ffStruct and EAhandle for the next EA. Before calling
         this function, the application is required to call NWFindFirstEA.
         The NWFindNextEA function can then be called multiple times until
         all EA's have been "found".

         A EA_DONE will be returned when their are no more EAs.

ARGS: <> ffStruct
         Pointer to NW_EA_FF_STRUCT structure returned by
         NWFindFirstEA.  Will contain a handle of the next EA.

      <  EAHandle
         EAHandle structure for EA. NWReadEA and NWWriteEA can be
         called with this structure.  (NWGetEAHandleStruct does NOT
         need to be called after a NWFindNextEA.)

      <  EAName
         Name of EA. This will need to be used if NWGetEAHandleStruct
         will be called in preparation for a write (ie. when the
         findfirst/nextEA calls are being used to perform a copy.)

INCLUDE: nwea.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:

CHANGES: 8 Oct 1991 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWFindNextEA
(
   NW_EA_FF_STRUCT N_FAR * ffStruct,
   NW_EA_HANDLE N_FAR * EAHandle,
   pnstr8   EAName
)
{
   pnuint8 theKey;
   NWCCODE ccode;

   if(ffStruct == NULL)
      return (0x89ff);

   if(ffStruct->nextKey >= ffStruct->numKeysInBuffer)
   {
      if((ccode = _enumEA(ffStruct, NULL)) != 0)
         return (ccode);
   }

   EAHandle->rwPosition= 0L;
   EAHandle->EAHandle  = 0L;
   EAHandle->keyUsed   = 0;

   theKey = &ffStruct->enumBuffer[ffStruct->nextKeyOffset];
   EAHandle->keyLength = *theKey++;
   NWCMemMove(EAHandle->key, theKey, EAHandle->keyLength + 1);
   ffStruct->nextKeyOffset += EAHandle->keyLength + 2;
   ffStruct->nextKey++;

   if(EAName)
      NWCStrCpy(EAName, EAHandle->key);

   return(0);
}

/*manpage*_enumEA***********************************************************
SYNTAX:  NWCCODE N_API _enumEA
         (
            NW_EA_FF_STRUCT N_FAR * ffStruct,
            NW_IDX N_FAR * idxStruct
         )

REMARKS:

ARGS: <> ffStruct
         Pointer to a structure returning a handle to the next EA.

      >  idxStruct
         Pointer to idx struct for initial call

INCLUDE:

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     86 04  Enumerate Extended Attribute

CHANGES: 8 Oct 1991 - written - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API _enumEA
(
   NW_EA_FF_STRUCT N_FAR * ffStruct,
   NW_IDX N_FAR *   idxStruct
)
{
   NWCCODE ccode;
   nuint16 suFlags;
   nuint32 luInspectSize;
   nuint16 suEnumSeq;
   nuint16 suKeyLen;
   nuint32 luErrorCode;
   nuint32 luTtlEAs;
   nuint32 luTtlEAsDataSize;
   nuint32 luTtlEAsKeySize;
   nuint32 luNewEAHandle;
   NWNCPEAHandle NCPEAHandle;
   NWNCPEAEnum   NCPEAEnum;

   NWCDeclareAccess(access);

   NWCSetConn(access, ffStruct->connID);

   if(idxStruct == NULL)
   {
      if(ffStruct->numKeysRead >= ffStruct->totalKeys)
         return EA_DONE;

      NCPEAHandle.TYPE_10.luEAHandle = ffStruct->EAHandle;
      suFlags = 0xF2;
   }
   else
   {
      NCPEAHandle.TYPE_00.luVolNum  = idxStruct->volNumber;
      NCPEAHandle.TYPE_00.luDirBase = idxStruct->srcDirBase;
      suFlags = 0xF0;
   }

   luInspectSize = 512;
   suEnumSeq     = ffStruct->sequence;
   suKeyLen      = (nuint16) 0;

   if ((ccode = (NWCCODE) NWNCP86s4EAEnum(&access, suFlags,
                  &NCPEAHandle, luInspectSize, suEnumSeq, suKeyLen,
                  NULL, &luErrorCode, &luTtlEAs, &luTtlEAsDataSize,
                  &luTtlEAsKeySize, &luNewEAHandle, &NCPEAEnum,
                  ffStruct->enumBuffer)) != 0)
   {
      return (ccode);
   }

   if(luErrorCode)
      return ((NWCCODE)luErrorCode);

   ffStruct->totalKeys     = luTtlEAs;
   ffStruct->numKeysInBuffer = NCPEAEnum.lvl_1_7.suEnumEAStructCount;
   ffStruct->numKeysRead  += ffStruct->numKeysInBuffer;
   ffStruct->nextKey       = 0;
   ffStruct->nextKeyOffset = 0;
   ffStruct->EAHandle      = 0L;

   if (luTtlEAs == 0)
      return EA_DONE;

   return 0;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/ffirstea.c,v 1.7 1994/09/26 17:45:27 rebekah Exp $
*/

