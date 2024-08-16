/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:123s13.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfse.h"

/*manpage*NWNCP123s13GetOSVersionInfo**************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP123s13GetOSVersionInfo
         (
            pNWAccess                pAccess,
            pNWNCPFSEVConsoleInfo   pVConsoleInfo,
            pnuint16                psuReserved,
            pNWNCPFSEOSVerInfo      pOSVersion,
         )

REMARKS:

ARGS: <> pAccess
      <  pVConsoleInfo
      <  psuReserved
      <  pOSVersion
         typedef struct tagNWNCPFSEOSVerInfo
         {
            nuint8                  buMajorVer;
            nuint8                  buMinorVer;
            nuint8                  buRev;
            nuint8                  buAcctVer;
            nuint8                  buVAPVer;
            nuint8                  buQueueVer;
            nuint8                  buSecurityRestLevel;
            nuint8                  buBridgingSupport;
            nuint32                 luMaxNumVols;
            nuint32                 luMaxNumConns;
            nuint32                 luMaxNumUsers;
            nuint32                 luMaxNumNameSpaces;
            nuint32                 luMaxNumLANs;
            nuint32                 luMaxNumMedias;
            nuint32                 luMaxNumStacks;
            nuint32                 luMaxDirDepth;
            nuint32                 luMaxDataStreams;
            nuint32                 luMaxNumOfSpoolPr;
            nuint32                 luServerSerialNum;
            nuint16                 suServerAppNum;
         } NWNCPFSEOSVerInfo, N_FAR *pNWNCPFSEOSVerInfo;

INCLUDE: ncpfse.h

RETURN:

SERVER:  4.0

CLIENT:  DOS OS2 WIN NT

SEE:

NCP:     123 13  Get Operating System Version Information

CHANGES: 23 Sep 1993 - written - dromrell
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP123s13GetOSVersionInfo
(
   pNWAccess                pAccess,
   pNWNCPFSEVConsoleInfo   pVConsoleInfo,
   pnuint16                psuReserved,
   pNWNCPFSEOSVerInfo      pOSVersion
)
{
   #define NCP_FUNCTION    ((nuint) 123)
   #define NCP_SUBFUNCTION ((nuint8) 13)
   #define NCP_STRUCT_LEN  ((nuint16) 1)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 62)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuRep[REPLY_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   lCode = NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, REQ_LEN,
               abuRep, REPLY_LEN, NULL);
   if (lCode == 0)
   {
      NWNCP_UNPACK_VCONS_INF(pVConsoleInfo, abuRep);
      if(psuReserved)
         NCopyLoHi16(psuReserved,                  &abuRep[6]);

      pOSVersion->buMajorVer                       =abuRep[8];
      pOSVersion->buMinorVer                       =abuRep[9];
      pOSVersion->buRev                            =abuRep[10];
      pOSVersion->buAcctVer                        =abuRep[11];
      pOSVersion->buVAPVer                         =abuRep[12];
      pOSVersion->buQueueVer                       =abuRep[13];
      pOSVersion->buSecurityRestLevel              =abuRep[14];
      pOSVersion->buBridgingSupport                =abuRep[15];
      NCopyLoHi32(&pOSVersion->luMaxNumVols,       &abuRep[16]);
      NCopyLoHi32(&pOSVersion->luMaxNumConns,      &abuRep[20]);
      NCopyLoHi32(&pOSVersion->luMaxNumUsers,      &abuRep[24]);
      NCopyLoHi32(&pOSVersion->luMaxNumNameSpaces, &abuRep[28]);
      NCopyLoHi32(&pOSVersion->luMaxNumLANs,       &abuRep[32]);
      NCopyLoHi32(&pOSVersion->luMaxNumMedias,     &abuRep[36]);
      NCopyLoHi32(&pOSVersion->luMaxNumStacks,     &abuRep[40]);
      NCopyLoHi32(&pOSVersion->luMaxDirDepth,      &abuRep[44]);
      NCopyLoHi32(&pOSVersion->luMaxDataStreams,   &abuRep[48]);
      NCopyLoHi32(&pOSVersion->luMaxNumOfSpoolPr,  &abuRep[52]);
      NCopyLoHi32(&pOSVersion->luServerSerialNum,  &abuRep[56]);
      NCopyLoHi16(&pOSVersion->suServerAppNum,     &abuRep[60]);
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/123s13.c,v 1.7 1994/09/26 17:32:09 rebekah Exp $
*/
