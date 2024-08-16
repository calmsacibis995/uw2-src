/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s17.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpserve.h"

/*manpage*NWNCP23s17GetServerInfo**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s17GetServerInfo
         (
            pNWAccess pAccess,
            pNWNCPVersionInfo pVerInfo,
         );

REMARKS: This call can be used by clients to gain information about a server to
         which they have a service connection.  Any client can use this call; a
         client need not call Login before using this call.

         Server Name contains the file server name.

         File Service Version contains the major NetWare version number.

         File Service SubVersion contains the minor NetWare version number.

         Maximum Service Connections indicates the maximum number of
         connections the file server can support.

         Connections In Use indicates how many connections are currently using
         the file server.

         Number Mounted Volumes contains the maximum number of volumes
         the file server can support.

         Revision contains the revision level of the NetWare version number.

         SFT Level indicates which SFT level the file server operating system is
         using.

         TTS Level indicates which TTS level the file server operating system is
         using.

         Max Connections Ever Used contains the maximum number of
         connections in use at one time.

         Account Version contains the Accounting version number.

         VAP Version contains the VAP version number.

         Queue Version contains the Queuing version number.

         Print Version contains the Print Server version number.

         Virtual Console Version contains the Virtual Console version number.

         Restriction Level contains the Security Restriction version number.

         Internet Bridge contains the Internet Bridge support version number.

         Reserved is reserved for future use.


ARGS: <> pAccess
      >  pVerInfo

INCLUDE: ncpserve.h

RETURN:  0x0000   Successful
         0x8996   Server Out Of Memory

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23  201  Get File Server Description Strings

NCP:     23  17  Get File Server Information

CHANGES: 10 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s17GetServerInfo
(
   pNWAccess pAccess,
   pNWNCPVersionInfo pVerInfo
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 17)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define SERVER_NAME_LEN ((nuint) 48)
   #define RESERVED_LEN    ((nuint) 60)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 128)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);

   if (lCode == 0)
   {
      NWCMemMove(pVerInfo->abstrServerName, &abuReply[0],
            SERVER_NAME_LEN);
      pVerInfo->buNetWareVer = abuReply[SERVER_NAME_LEN];
      pVerInfo->buNetWareSubVer = abuReply[SERVER_NAME_LEN+1];
      NCopyHiLo16(&pVerInfo->suMaxServConns, &abuReply[SERVER_NAME_LEN+2]);
      NCopyHiLo16(&pVerInfo->suConnsInUse, &abuReply[SERVER_NAME_LEN+4]);
      NCopyHiLo16(&pVerInfo->suNumMountedVols, &abuReply[SERVER_NAME_LEN+6]);
      pVerInfo->buRev = abuReply[SERVER_NAME_LEN+8];
      pVerInfo->buSFTLevel = abuReply[SERVER_NAME_LEN+9];
      pVerInfo->buTTSLevel = abuReply[SERVER_NAME_LEN+10];
      NCopyHiLo16(&pVerInfo->suMaxConnsEverUsed, &abuReply[SERVER_NAME_LEN+11]);
      pVerInfo->buAcctVer = abuReply[SERVER_NAME_LEN+13];
      pVerInfo->buVAPVer = abuReply[SERVER_NAME_LEN+14];
      pVerInfo->buQueueVer = abuReply[SERVER_NAME_LEN+15];
      pVerInfo->buPrintVer = abuReply[SERVER_NAME_LEN+16];
      pVerInfo->buVirtualConsVer = abuReply[SERVER_NAME_LEN+17];
      pVerInfo->buRestrictionLevel = abuReply[SERVER_NAME_LEN+18];
      pVerInfo->buInternetBridge = abuReply[SERVER_NAME_LEN+19];
      NWCMemMove(pVerInfo->abuReserved, &abuReply[SERVER_NAME_LEN+20],
            RESERVED_LEN);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s17.c,v 1.7 1994/09/26 17:35:48 rebekah Exp $
*/
