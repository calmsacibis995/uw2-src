/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s1.c	1.6"
#include "ntypes.h"
#include "unicode.h"
#include "nwaccess.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s1EntryOpenCreate*********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s1EntryOpenCreate
         (
            pNWAccess          pAccess,
            nuint8            buNamSpc,
            nuint8            buOpenCreateMode,
            nuint16           suSrchAttrs,
            nuint32           luRetMask,
            nuint32           luCreateAttrs,
            nuint16           suAccessRights,
            pNWNCPCompPath    pCompPath,
            pnuint8           pbuNWHandleB4,
            pnuint8           pbuOpenCreateAction,
            pnuint8           pbuReserved,
            pNWNCPEntryStruct pEntry
         );

REMARKS: Creates or opens the file depending on the OpenCreate Mode field.
         Note, however, that subdirectories may be created by the client but
         not opened by the client.

         The CreateAttributes field is used to set the attributes in the DOS
         name space. More information on this field in given in the
         Introduction to Directory Services.

         The SearchAttributes field, ReturnInfoMask field, NetWareInfoStruct
         field, and NetWareFileNameStruct field are explained in more detail
         in the Introduction to Directory Services.

         OpenCreateMode

                           Bits
               7   6   5   4   3   2   1   0

               *********************************
               * 0 * 0 * 0 * 0 * x * 0 * x * x *
               *********************************
                                 *       *   *
                Action Create ****       *   *
                Action Replace ***********   *
                Action Open ******************

         OpenCreateAction

                                    Bits
                        7   6   5   4   3   2   1   0

                    *********************************
                    * 0 * 0 * 0 * 0 * 0 * x * x * x *
                    *********************************
                                          *   *   *
                                          *   *   *
              No Action Taken *********** 0   0   0
              File Open ***************** 0   0   1
              File\SubDirectory Created * 0   1   0
              File Replaced ************* 1   0   0


         DesiredAccessRights
                                       Bits
         15  14  13  12  11  10   9   8   7   6   5   4    3   2   1  0
         *****************************************************************
         * 0 * 0 * 0 * 0 * 0 * 0 * 0 * 0 * 0 * x * 0 * x * x * x * x * x *
         *****************************************************************                                           *       *
                                               *       *   *   *   *   *                                   *       *   *   *   *   *
                                               *       *   *   *   *   *
                                               *       *   *   *   *   *
         File Write Through Mode****************       *   *   *   *   *
                                                       *   *   *   *   *
         Compatibility Mode ****************************   *   *   *   *
         Deny Write Mode ***********************************   *   *   *
         Deny Read Mode ****************************************   *   *
         Write Only Mode *******************************************   *
         Read Only Mode ************************************************

ARGS: <> pAccess
       > buNamSpc
       > buOpenCreateMode
       > suSrchAttrs
       > luRetMask
       > luCreateAttrs
       > suAccessRights
       > pCompPath
      <  pbuNWHandleB4           (optional)
      <  pbuOpenCreateAction     (optional)
      <  pbuReserved             (optional)
      <  pEntry                  (optional)

INCLUDE: ncpfile.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     67 --  Create File
         76 --  Open File
         77 --  Create New File
         10 --  Create Directory

NCP:     87 01  Open Create File or Subdirectory

CHANGES: 13 Sep 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s1EntryOpenCreate
(
   pNWAccess          pAccess,
   nuint8            buNamSpc,
   nuint8            buOpenCreateMode,
   nuint16           suSrchAttrs,
   nuint32           luRetMask,
   nuint32           luCreateAttrs,
   nuint16           suAccessRights,
   pNWNCPCompPath    pCompPath,
   pnuint8           pbuNWHandleB4,
   pnuint8           pbuOpenCreateAction,
   pnuint8           pbuReserved,
   pNWNCPEntryStruct pEntry
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 1)
   #define REQ_LEN         ((nuint) 15)
   #define REPLY_LEN       ((nuint) 83)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 2)
   #define MAX_NAME_LEN    ((nuint) 256)

   nint32  lCode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN], abuBucket[MAX_NAME_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   abuReq[1] = buNamSpc;
   abuReq[2] = buOpenCreateMode;
   NCopyLoHi16(&abuReq[3], &suSrchAttrs);
   NCopyLoHi32(&abuReq[5], &luRetMask);
   NCopyLoHi32(&abuReq[9], &luCreateAttrs);
   NCopyLoHi16(&abuReq[13], &suAccessRights);

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = pCompPath->suPackedLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   replyFrag[1].pAddr = pEntry ? pEntry->abuName : abuBucket;
   replyFrag[1].uLen  = MAX_NAME_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, replyFrag, NULL);

   if(lCode == 0)
   {
      if(pbuNWHandleB4)
		 NCopyHiLo32( pbuNWHandleB4, &abuReply[0]);
      /*   NWCMemMove(pbuNWHandleB4, &abuReply[0], 4);i*/

      if(pbuOpenCreateAction)
         *pbuOpenCreateAction = abuReply[4];

      if(pbuReserved)
         *pbuReserved = abuReply[5];

      if(pEntry)
      {
         NWNCPUnpackEntryStruct(pEntry, &abuReply[6], luRetMask);
      }
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s1.c,v 1.8 1994/09/26 17:39:12 rebekah Exp $
*/
