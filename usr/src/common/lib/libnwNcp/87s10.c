/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:87s10.c	1.4"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP87s10TrusteeAddSet******************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP87s10TrusteeAddSet
         (
            pNWAccess       pAccess,
            nuint8         buNamSpc,
            nuint8         buReserved,
            nuint16        suSrchAttrs,
            nuint16        suTrusteeRightsMask,
            nuint16        suTrusteeCnt,
            pNWNCPCompPath pCompPath,
            pNWNCPTrustees pTrustees
         );

REMARKS: Adds a trustee to the list of trustees in a file or a directory.

         If the TrusteeRightsMask field is set to -1, the individual
         TrusteeRights should be set to the appropriate rights for each entry
         in the Trustee Structure.

ARGS: <> pAccess
       > buNamSpc
       > buReserved
       > suSrchAttrs
       > suTrusteeRightsMask
       > suTrusteeCnt
       > pCompPath
       > pTrustees

INCLUDE: ncpfile.h

RETURN:

SERVER:  PNW 2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 10  Add Trustee Set To Directory File or Subdirectory

CHANGES: 7 Sep 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP87s10TrusteeAddSet
(
   pNWAccess       pAccess,
   nuint8         buNamSpc,
   nuint8         buReserved,
   nuint16        suSrchAttrs,
   nuint16        suTrusteeRightsMask,
   nuint16        suTrusteeCnt,
   pNWNCPCompPath pCompPath,
   pNWNCPTrustees pTrustees
)
{
   #define NCP_FUNCTION    ((nuint) 87)
   #define NCP_SUBFUNCTION ((nuint8) 10)
   #define REQ_LEN_A       ((nuint) 9)
   #define REQ_LEN_B       ((nuint) (suTrusteeCnt * 6))
   #define MAX_REQ_LEN_B   ((nuint) 198)
   #define REQ_FRAGS       ((nuint) 3)
   #define REPLY_FRAGS     ((nuint) 0)
   #define KLUDGE_COMP_PATH_LEN ((nuint) 307)

   nint i, n;
   nuint8 abuReqA[REQ_LEN_A], abuReqB[MAX_REQ_LEN_B];
   NWCFrag reqFrag[REQ_FRAGS];

   abuReqA[0] = NCP_SUBFUNCTION;
   abuReqA[1] = buNamSpc;
   abuReqA[2] = buReserved;
   NCopyLoHi16(&abuReqA[3], &suSrchAttrs);
   NCopyLoHi16(&abuReqA[5], &suTrusteeRightsMask);
   NCopyLoHi16(&abuReqA[7], &suTrusteeCnt);

   /* clear the comp path because the server expects a non-standard
      comp path here; it wants the comp path, even the bytes that aren't
      being used for comp path info; then comes the rest of the data.
      this is zeroed out to eliminate confusion on the server end of things. */

   for (i = pCompPath->suPackedLen;
        i < (nint) (KLUDGE_COMP_PATH_LEN - pCompPath->suPackedLen);
        i++)
   {
      pCompPath->abuPacked[i] = 0x00;
   }

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ_LEN_A;

   reqFrag[1].pAddr = pCompPath->abuPacked;
   reqFrag[1].uLen  = KLUDGE_COMP_PATH_LEN;

   for (i = n = 0; i < (nint)suTrusteeCnt; i++, n += 6)
   {
      NCopyHiLo32(&abuReqB[n], &pTrustees[i].luObjID);
      NCopyLoHi16(&abuReqB[n + 4], &pTrustees[i].suRights);
   }

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = REQ_LEN_B;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/87s10.c,v 1.6 1994/09/26 17:39:13 rebekah Exp $
*/
