/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:35s17.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpafp.h"

/*manpage*NWNCP35s17AFPScanFileInfo******************************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP35s17AFPScanFileInfo
         (
            pNWAccess pAccess,
            nuint8   buVolNum,
            nuint32  luAFPBaseEntryID,
            nuint32  luLastSeenAFPEntryID,
            nuint16  suDesiredResponseCnt,
            nuint16  suSearchBitMap,
            nuint16  suReqBitMap,
            nuint8   buPathLen,
            pnstr8   pbstrPath,
            pnuint16 psuResponseCnt,
            pNWNCPAFPFileInfo pMacFileInfo
         )

REMARKS: Return information about an AFP entry (directory or file).
         Iterative scanning of a directory is supported, and up to 4 responses
         per request can be specified in DesiredResponseCount.

         To scan all files and subdirectories of a directory, the client should set
         Mac Last Seen ID to -1L on the first call; for each subsequent call, the
         client should set Mac Last Seen ID to the Entry ID of the previous call.

         If Path Mod String specifies a directory, all entries within the directory
         that fit the criteria in Search Bit Map and Request Bit Map will be returned.

         The bits in the Search Bit Map are defined as follows:

         0x0100      Search For Hidden Files/Directories
         0x0200      Search For System Files/Directories
         0x0400      Search For Subdirectories
         0x0800      Search For Files

         The bits in the Request Bit Map are defined as follows:

         0x0100      Return Attributes
         0x0200      Return Parent Directory ID
         0x0400      Return Create Date
         0x0800      Return Access Date
         0x1000      Return Modify Date/Time
         0x2000      Return Backup Date/Time
         0x4000      Return Finder Info
         0x8000      Return Long Name
         0x0001      Return Entry ID
         0x0002      Return Data Fork Length
         0x0004      Return Resource Fork Length
         0x0008      Return Number Of Offspring
         0x0010      Return Owner ID
         0x0020      Return Short Name
         0x0040      Return Access Rights

         The following information is returned in the reply message.

         The APF Entry ID field contains the Apple equivalent to a 1- byte NetWare
         directory handle, with one important exception: a NetWare directory handle
         points to a file server volume or directory; an AFP entry ID points to a file
         server volume, directory, or file.

         The Parent ID field contains the AFP entry ID for the parent directory of the
         target file or directory.

         The Attributes field indicates the attributes of the directory or file.  The
         following bits are defined:

         0x0100      Read Only
         0x0200      Hidden
         0x0400      System
         0x0800      Execute Only
         0x1000      Subdirectory
         0x2000      Archive
         0x4000      Undefined
         0x8000      Shareable File
         0x0001      Search Mode
         0x0002      Search Mode
         0x0004      Search Mode
         0x0008      Undefined
         0x0010      Transaction
         0x0020      Index
         0x0040      Read Audit
         0x0080      Write Audit

         The Data Fork Length field indicates the data size of the target AFP file.
         If the Path Mod String specifies an AFP directory, the Data Fork Length field
         returns a zero.

         The Resource Fork Length field indicates the resource fork size of the target
         AFP file.  If the Path Mod String specifies an AFP directory, the Resource
         Fork Length field returns a zero.

         The Total Offspring field indicates the number of files and subdirectories
         contained within the specified directory.  If the AFP directory or file path
         specifies an AFP file, the Total Offspring field returns a zero.

         The Creation Date field contains the creation date (in AFP format) of the
         target directory or file.

         The Access Date indicates when the target AFP file was last accessed
         (returned in AFP format).  If Path Mod String specifies an AFP directory, the
         Access Date field returns a zero.

         The Modify Date and Modify Time fields contain the last modified date and
         time (in AFP format) of the target AFP file.  If the Path Mod String
         specifies an AFP directory, these fields return zeros.

         The Backup Date and Backup Time fields contain the last backup date and time
         (in AFP format) of the specified directory or file.

         The Finder Info field contains the 32-byte finder information structure
         associated with each AFP directory or file.

         The Long Name field contains the AFP directory or filename of the specified
         directory or file.  An AFP directory or filename can be from 1 to 31
         characters long.

         The Owner ID field contains the 4-byte bindery object ID of the entity that
         created or last modified the file.

         The Short Name field contains the NetWare directory or filename of the
         specified directory or file.  A NetWare directory or filename is in the DOS
         "8.3" format.

         The Access Privileges field contains a one-word bit mask of the calling
         station's privileges for accessing the specified file or directory.  The
         Access Privileges field contains the following bits:

         0x0100      Read Privileges (files only)
         0x0200      Write Privileges (files only)
         0x0400      Open Privileges (files only)
         0x0800      Create Privileges (files only)
         0x1000      Delete Privileges (files only)
         0x2000      Parental Privileges (directories only:
                        create/delete/rename subdirectories)
         0x4000      Search Privileges (directories only)
         0x8000      Modify File Status Flags Privileges (both
                        files and directories)

         If the returned object is a file, the bits in the Access Privileges field
         indicate which rights the caller has in relation to the file.  These rights
         are the rights that the caller has in the file's parent directory,
         appropriately modified if the file itself has certain rights restrictions
         (that is, a read-only file would cause the Write and Delete bits to be
         cleared).

         If the returned object is a directory, the bits in the Access Privileges
         field indicate the rights that the caller has in that directory.

         Read the introduction to the Directory Services chapter for a complete
         discussion of directory and file access rights.

         The ProDOSInfo field is a 6-byte structure defined in Apple documentation.

ARGS: <> pAccess
      >  buVolNum
      >  luAFPBaseEntryID
      >  luLastSeenAFPEntryID
      >  suDesiredResponseCnt
      >  suSearchBitMap
      >  suRqstBitMap
      >  buPathLen
      >  strPath
      <  psuResponseCnt
      <  pMacFileInfo

INCLUDE: ncpafp.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:     AFP 2.0 Set File Information (0x2222  35  16)
         AFP Scan File Information (0x2222  35  10)

NCP:     35 17  AFP Scan File Information

CHANGES: 18 Aug 1993 - written - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP35s17AFPScanFileInfo
(
   pNWAccess pAccess,
   nuint8   buVolNum,
   nuint32  luAFPBaseEntryID,
   nuint32  luLastSeenAFPEntryID,
   nuint16  suDesiredResponseCnt,
   nuint16  suSearchBitMap,
   nuint16  suReqBitMap,
   nuint8   buPathLen,
   pnstr8   pbstrPath,
   pnuint16 psuResponseCnt,
   pNWNCPAFPFileInfo pMacFileInfo
)
{
   #define NCP_FUNCTION    ((nuint) 35)
   #define NCP_SUBFUNCTION ((nuint8) 17)
   #define NCP_STRUCT_LEN  ((nuint16) (17 + buPathLen))
   #define REQ_LEN         ((nuint) 19)
   #define REPLY_LEN       ((nuint) 122)
   #define REQ_FRAGS       ((nuint) 2)
   #define REPLY_FRAGS     ((nuint) 1)

   NWRCODE ccode;
   NWCFrag reqFrag[REQ_FRAGS], replyFrag[REPLY_FRAGS];
   nuint16 suNCPLen;
   nuint8  abuReq[REQ_LEN], abuReply[REPLY_LEN];

   suNCPLen  = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;
   abuReq[3] = buVolNum;
   NCopyHiLo32(&abuReq[4], &luAFPBaseEntryID);
   NCopyHiLo32(&abuReq[8], &luLastSeenAFPEntryID);
   NCopyHiLo16(&abuReq[12], &suDesiredResponseCnt);
   NCopyHiLo16(&abuReq[14], &suSearchBitMap);
   NCopyHiLo16(&abuReq[16], &suReqBitMap);
   abuReq[18] = buPathLen;

   reqFrag[0].pAddr = abuReq;
   reqFrag[0].uLen  = REQ_LEN;

   reqFrag[1].pAddr = pbstrPath;
   reqFrag[1].uLen  = (nuint) buPathLen;

   replyFrag[0].pAddr = abuReply;
   replyFrag[0].uLen  = REPLY_LEN;

   if ((ccode = NWCRequest(pAccess, NCP_FUNCTION, REQ_FRAGS, reqFrag,
         REPLY_FRAGS, replyFrag, NULL)) == 0)
   {
      if (psuResponseCnt)
         NCopyHiLo16(psuResponseCnt, &abuReply[0]);

      NWCUnpackAFPPacket(pMacFileInfo, &abuReply[2]);
   }

   return ccode;
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/35s17.c,v 1.7 1994/09/26 17:38:18 rebekah Exp $
*/
