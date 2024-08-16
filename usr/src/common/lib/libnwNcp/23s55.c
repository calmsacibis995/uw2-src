/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s55.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s55ScanObj*************************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s55ScanObj
         (
            pNWAccess pAccess,
            nuint16  suSrchObjType,
            nuint8   buSrchObjNameLen,
            pnstr8   pbstrSrchObjName,
            pnuint32 pluObjID,
            pnuint16 psuObjType,
            pnstr8   pbstrObjName,
            pnuint8  pbuObjFlags,
            pnuint8  pbuObjSecurity,
            pnuint8  pbuObjHasProperties,
         );

REMARKS:
         This call allows a client to scan the server's bindery to determine what
         objects are there.

         The Search Object Type can be WILD (-1) or can be set to find only a
         specific type of object.

         The Object Search Name can contain wildcard characters.

         The Last Object Seen field should initially be set to -1L (long -1).  If the
         client wants to iteratively search through the server's bindery,
         then successive calls should set the Last Object Seen field to the Object ID
         field returned by the server on the previous call.

         The Object Has Properties field will always be set because all objects have
         a bindery level property that is used internally by the bindery.

         Any client can make this callsuccessfully if the object exists in the
         bindery and if the client has search privileges to the object.

ARGS: <> pAccess
      >  suSrchObjType
      >  buSrchObjNameLen
      >  pbstrSrchObjName
      <> pluObjID
      <  psuObjType (optional)
      <  pbstrObjName (optional)
      <  pbuObjFlags (optional)
      <  pbuObjSecurity (optional)
      <  pbuObjHasProperties (optional)

INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89EF  Illegal Name
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     23 67  Is Bindery Object In Set
         23 66  Delete Bindery Object From Set
         23 65  Add Bindery Object To Set
         23 71  Scan Bindery Object Trustee Paths
         23 50  Create Bindery Object
         23 51  Delete Bindery Object

NCP:     23 55  Scan Bindery Object

CHANGES: 26 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s55ScanObj
(
   pNWAccess pAccess,
   nuint16  suSrchObjType,
   nuint8   buSrchObjNameLen,
   pnstr8   pbstrSrchObjName,
   pnuint32 pluObjID,
   pnuint16 psuObjType,
   pnstr8   pbstrObjName,
   pnuint8  pbuObjFlags,
   pnuint8  pbuObjSecurity,
   pnuint8  pbuObjHasProperties
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 55)
   #define NCP_STRUCT_LEN  ((nuint16) 8)
   #define REQ_LEN         ((nuint) (2 + NCP_STRUCT_LEN))
   #define REPLY_LEN       ((nuint) 57)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   nint32   lCode;
   nuint8 abuReq[REQ_LEN], abuReply[REPLY_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   NCopyHiLo32(&abuReq[3], pluObjID);
   NCopyHiLo16(&abuReq[7], &suSrchObjType);
   abuReq[9] = buSrchObjNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrSrchObjName;
   reqFrag[1].uLen  = (nuint) buSrchObjNameLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   lCode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
            REPLY_FRAGS, replyFrag, NULL);
   if (lCode == 0)
   {
      NCopyHiLo32(pluObjID, &abuReply[0]);
      if (psuObjType)
         NCopyHiLo16(psuObjType, &abuReply[4]);
      if (pbstrObjName)
         NWCMemMove(pbstrObjName, &abuReply[6], (nuint) 48);
      if (pbuObjFlags)
         *pbuObjFlags = abuReply[54];
      if (pbuObjSecurity)
         *pbuObjSecurity = abuReply[55];
      if (pbuObjHasProperties)
         *pbuObjHasProperties = abuReply[56];
   }

   return ((NWRCODE) lCode);
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s55.c,v 1.7 1994/09/26 17:37:15 rebekah Exp $
*/
