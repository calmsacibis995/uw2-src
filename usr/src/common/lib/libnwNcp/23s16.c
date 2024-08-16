/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s16.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP23s16FileSetInfo*********************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s16FileSetInfo
         (
            pNWAccess          pAccess,
            pNWNCPFileInfo2   pFileInfo2,
            pnuint8           pbuReservedB56,
            nuint8            buDirHandle,
            nuint8            buSrchAttrs,
            nuint8            buFileNameLen,
            pnstr8            pbstrFileName,
         );

REMARKS: This call allows a client to set a file's status information. Any client
         with file modification privileges in the target directory can set a file's
         attributes, execute type, file creation date, last access date, last update
         date and time, and last archive date and time. The File Size field is always
         ignored. Only a client that is an object supervisor of the file is allowed to
         set a file's owner ID, the Last Archive Date and Time, or the 56 bytes of
         currently undefined information.

         This call is used primarily by archive programs that need to restore a file's
         complete profile to some earlier state.

ARGS: <> pAccess
      >  pFileInfo2
      >  pbuReservedB56 (optional)
      >  buDirHandle
      >  buSrchAttrs
      >  buFileNameLen
      >  pbstrFileName

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8988  Invalid File Handle
         0x898C  No Set Privileges
         0x898E  All Files In Use
         0x8994  No Write Privileges
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x899B  Bad Directory Handle
         0x899C  Invalid Path
         0x89A1  Directory I/O Error
         0x89A2  IO Lock Error
         0x89FC  No Such Object
         0x89FD  Bad Station Number
         0x89FE  Directory Locked
         0x89FF  Failure, No Files Found

SERVER:  2.2 3.11 4.0

CLIENT:  DOS OS2 WIN NT

SEE:     79 --  Set File Extended Attributes
         23 15  Scan File Information
         70 --  Set File Attributes
         75 --  Set File Time Date Stamp

NCP:     23 16  Set File Information

CHANGES: 1 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s16FileSetInfo
(
   pNWAccess          pAccess,
   pNWNCPFileInfo2   pFileInfo2,
   pnuint8           pbuReservedB56,
   nuint8            buDirHandle,
   nuint8            buSrchAttrs,
   nuint8            buFileNameLen,
   pnstr8            pbstrFileName
)
{
   #define NCP_FUNCTION    ((nuint) 23)
   #define NCP_SUBFUNCTION ((nuint8) 16)
   #define NCP_STRUCT_LEN  ((nuint16) (82 + buFileNameLen))
   #define REQ_LEN_A       ((nuint) 25)
   #define RESERVED_LEN    ((nuint) 56)
   #define REQ_LEN_B       ((nuint) 3)
   #define REQ_FRAGS       ((nuint) 4)
   #define REPLY_FRAGS     ((nuint) 0)

   nuint8 abuReqA[REQ_LEN_A], abuReqB[REQ_LEN_B];
   nuint8 abuStackBuf[RESERVED_LEN];
   nuint16 suNCPLen;
   NWCFrag reqFrag[REQ_FRAGS];

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReqA[0], &suNCPLen);
   abuReqA[2] = NCP_SUBFUNCTION;
   abuReqA[3] = pFileInfo2->buAttrs;
   abuReqA[4] = pFileInfo2->buExtAttrs;
   NCopyHiLo32(&abuReqA[5],  &pFileInfo2->luSize);
   NCopyHiLo16(&abuReqA[9],  &pFileInfo2->suCreationDate);
   NCopyHiLo16(&abuReqA[11], &pFileInfo2->suAccessedDate);
   NCopyHiLo16(&abuReqA[13], &pFileInfo2->suModifiedDate);
   NCopyHiLo16(&abuReqA[15], &pFileInfo2->suModifiedTime);
   NCopyHiLo32(&abuReqA[17], &pFileInfo2->luOwnerID);
   NCopyHiLo16(&abuReqA[21], &pFileInfo2->suArchiveDate);
   NCopyHiLo16(&abuReqA[23], &pFileInfo2->suArchiveTime);

   abuReqB[0] = buDirHandle;
   abuReqB[1] = buSrchAttrs;
   abuReqB[2] = buFileNameLen;

   if (!pbuReservedB56)  /* pass in zeroes if pbuReservedB56 is NULL */
   {
      NWCMemSet(abuStackBuf, 0, RESERVED_LEN);
      pbuReservedB56 = abuStackBuf;
   }

   reqFrag[0].pAddr = abuReqA;
   reqFrag[0].uLen  = REQ_LEN_A;

   reqFrag[1].pAddr = pbuReservedB56;
   reqFrag[1].uLen  = RESERVED_LEN;

   reqFrag[2].pAddr = abuReqB;
   reqFrag[2].uLen  = REQ_LEN_B;

   reqFrag[3].pAddr = pbstrFileName;
   reqFrag[3].uLen  = buFileNameLen;

   return (NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
               REPLY_FRAGS, NULL, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s16.c,v 1.7 1994/09/26 17:35:47 rebekah Exp $
*/
