/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s50.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpbind.h"

/*manpage*NWNCP23s50CreateObj*******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s50CreateObj
         (
            pNWAccess pAccess,
            nuint8   buStatusFlags,
            nuint8   buSecurityLevel,
            nuint16  suObjType,
            nuint8   buObjNameLen,
            pnstr8   pbstrObjName,
         );

REMARKS: Creates a bindery object.

         Bit 0 of Status Flags should be set if the object being created is
         dynamic; bit 0 should be cleared if the object being created is
         static.

         The Security Level mask should have the low (search) and high
         (property create) nibbles set appropriately.  A client cannot set
         security levels to "file server" access.

         Only supervisors of the specified object can make this service
         request.  Clients in the WORKGROUP_MANAGERS property of an object
         are also considered supervisors of the object.  (See "Workgroup
         Managers" at the beginning of this section.)

ARGS: <> pAccess
      >  buStatusFlags
      >  buSecurityLevel
      >  suObjType
      >  buObjectNameLen
      >  pbstrObjName
         (47 characters)


INCLUDE: ncpbind.h

RETURN:  0x0000  Successful
         0x8996  Server Out Of Memory
         0x89E7  No Disk Track
         0x89EE  Object Exists
         0x89EF  Illegal Name
         0x89F1  Bindery Security
         0x89F5  No Object Create
         0x89FC  No Such Object
         0x89FE  Directory Locked
         0x89FF  Hard Failure

SERVER:  DOS OS2 WIN

CLIENT:  2.2 3.11 4.0

SEE:     23 51  Delete Bindery Object

NCP:     23 50  Create Bindery Object

CHANGES: 24 Aug 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s50CreateObj
(
   pNWAccess pAccess,
   nuint8   buStatusFlags,
   nuint8   buSecurityLevel,
   nuint16  suObjType,
   nuint8   buObjNameLen,
   pnstr8   pbstrObjName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 50)
   #define NCP_STRUCT_LEN  ((nuint16) (6 + buObjNameLen))
   #define REQ_LEN         ((nuint) 8)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReq[REQ_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[2];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buStatusFlags;
   abuReq[4] = buSecurityLevel;
   NCopyHiLo16(&abuReq[5], &suObjType);
   abuReq[7] = buObjNameLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrObjName;
   reqFrag[1].uLen  = (nuint) buObjNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s50.c,v 1.7 1994/09/26 17:37:09 rebekah Exp $
*/
