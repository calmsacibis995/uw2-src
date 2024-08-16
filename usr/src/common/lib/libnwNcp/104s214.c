/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:104s214.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP104s214WriteAuditFilCfgHdr**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP104s214WriteAuditFilCfgHdr
         (
            pNWAccess    pAccess,
            nuint32     luContID,
            nuint16     suAuditVerDate,
            pnuint8     pbuKeyB8,
            pNWNCPAuditConfigFileHdr pConfigHdr,
         )

REMARKS:

ARGS: <> pAccess
      >  luContID
      >  suAuditVerDate
      >  pbuKeyB8
      >  pConfigHdr

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     104 214   Write Audit File Configuration Header

CHANGES: 28 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP104s214WriteAuditFilCfgHdr
(
   pNWAccess    pAccess,
   nuint32     luContID,
   nuint16     suAuditVerDate,
   pnuint8     pbuKeyB8,
   pNWNCPContAuditConfigFileHdr pConfigHdr
)
{
   #define NCP_FUNCTION    ((nuint) 104)
   #define NCP_SUBFUNCTION ((nuint8) 214)
   #define REQ_LEN         ((nuint) 143)
   #define REPLY_LEN       ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nint i;

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luContID);
   NCopyLoHi16(&abuReq[5], &suAuditVerDate);
   for(i=0; i<8; i++)
   {
      abuReq[i+7] = pbuKeyB8[i];
   }
   NCopyLoHi16(&abuReq[15], &pConfigHdr->suAuditFileVerDate);
   abuReq[17] = pConfigHdr->buAuditFlag;
   abuReq[18] = pConfigHdr->buErrMsgDelayInMins;

   NCopyLoHi32(&abuReq[19], &pConfigHdr->luContainerID);
   NCopyLoHi32(&abuReq[23], &pConfigHdr->luSpareLong);
   NCopyLoHi32(&abuReq[27], &pConfigHdr->creationTS.luSeconds);
   NCopyLoHi16(&abuReq[31], &pConfigHdr->creationTS.suReplicaNum);
   NCopyLoHi16(&abuReq[33], &pConfigHdr->creationTS.suEvent);
   NCopyLoHi32(&abuReq[35], &pConfigHdr->luBitMap);

   NCopyLoHi32(&abuReq[39], &pConfigHdr->luContAuditFileMaxSize);
   NCopyLoHi32(&abuReq[43], &pConfigHdr->luContAuditFileSizeThresh);
   NCopyLoHi32(&abuReq[47], &pConfigHdr->luAuditRecCount);

   NCopyLoHi16(&abuReq[51], &pConfigHdr->suReplicaNum);
   abuReq[53] = pConfigHdr->buEnabledFlag;
   NWCMemMove(&abuReq[54], pConfigHdr->abuSpareBytes, (nuint) 3);
   NCopyLoHi16(&abuReq[57], &pConfigHdr->suNumReplicaEntries);
   for(i=0; i<9; i++)
   {
      NCopyLoHi32(&abuReq[(i*4)+59], &pConfigHdr->aluSpareLongs[i]);
   }
   NCopyLoHi32(&abuReq[95], &pConfigHdr->luAuditDisabledCounter);
   NCopyLoHi32(&abuReq[99], &pConfigHdr->luAuditEnabledCounter);
   NWCMemMove(&abuReq[103], pConfigHdr->abuCryptPassword1, (nuint) 16);
   NWCMemMove(&abuReq[119], pConfigHdr->abuCryptPassword2, (nuint) 16);
   NCopyLoHi32(&abuReq[135], &pConfigHdr->luHdrModifiedCounter);
   NCopyLoHi32(&abuReq[139], &pConfigHdr->luFileResetCounter);

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               NULL, REPLY_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/104s214.c,v 1.7 1994/09/26 17:31:44 rebekah Exp $
*/
