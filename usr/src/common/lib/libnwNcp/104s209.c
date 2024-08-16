/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:104s209.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpaudit.h"

/*manpage*NWNCP104s209ReadAuditFileCfgHdr**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP104s209ReadAuditFileCfgHdr
         (
            pNWAccess pAccess,
            nuint32  luContID,
            pNWNCPContAuditConfigFileHdr pHeader,
         )

REMARKS:

ARGS: <> pAccess
      >  luContID
      <  pHeader

INCLUDE: ncpaudit.h

RETURN:  0x0000   Successful
         0x89C6   No Console Rights

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     104 209  Read Audit File Configuration Header

CHANGES: 28 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP104s209ReadAuditFileCfgHdr
(
   pNWAccess pAccess,
   nuint32  luContID,
   pNWNCPContAuditConfigFileHdr pHeader
)
{
   #define NCP_FUNCTION    ((nuint) 104)
   #define NCP_SUBFUNCTION ((nuint8) 209)
   #define REQ_LEN         ((nuint) 5)
   #define REPLY_LEN       ((nuint) 128)

   nint32   lCode;
   nuint8 i, abuReq[REQ_LEN], abuReply[REPLY_LEN];

   abuReq[0] = NCP_SUBFUNCTION;
   NCopyLoHi32(&abuReq[1], &luContID);

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuReply, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NCopyLoHi16(&pHeader->suAuditFileVerDate,       &abuReply[0]);
      pHeader->buAuditFlag                            =abuReply[2];
      pHeader->buErrMsgDelayInMins                    =abuReply[3];
      NCopyLoHi32(&pHeader->luContainerID,            &abuReply[4]);
      NCopyLoHi32(&pHeader->luSpareLong,              &abuReply[8]);
      NCopyLoHi32(&pHeader->creationTS.luSeconds,     &abuReply[12]);
      NCopyLoHi16(&pHeader->creationTS.suReplicaNum,  &abuReply[16]);
      NCopyLoHi16(&pHeader->creationTS.suEvent,       &abuReply[18]);
      NCopyLoHi32(&pHeader->luBitMap,                 &abuReply[20]);
      NCopyLoHi32(&pHeader->luContAuditFileMaxSize,   &abuReply[24]);
      NCopyLoHi32(&pHeader->luContAuditFileSizeThresh,&abuReply[28]);
      NCopyLoHi32(&pHeader->luAuditRecCount,          &abuReply[32]);
      NCopyLoHi16(&pHeader->suReplicaNum,             &abuReply[36]);
      pHeader->buEnabledFlag                          =abuReply[38];
      NWCMemMove(&pHeader->abuSpareBytes[0],          &abuReply[39], 3);
      NCopyLoHi16(&pHeader->suNumReplicaEntries,      &abuReply[42]);

      for(i=0; i<9;i++)
         NCopyLoHi32(&pHeader->aluSpareLongs[i],      &abuReply[44+(i*4)]);

      NCopyLoHi32(&pHeader->luAuditDisabledCounter,   &abuReply[80]);
      NCopyLoHi32(&pHeader->luAuditEnabledCounter,    &abuReply[84]);
      NWCMemMove(&pHeader->abuCryptPassword1[0],      &abuReply[88],
            (nuint) 16);
      NWCMemMove(&pHeader->abuCryptPassword2[0],      &abuReply[104],
            (nuint) 16);
      NCopyLoHi32(&pHeader->luHdrModifiedCounter,     &abuReply[120]);
      NCopyLoHi32(&pHeader->luFileResetCounter,       &abuReply[124]);
   }
   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/104s209.c,v 1.7 1994/09/26 17:31:38 rebekah Exp $
*/
