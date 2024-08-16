/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:23s206.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCP23s206DelPurge**********************************
SYNTAX:  N_EXTERN_LIBRARY( NWRCODE )
         NWNCP23s206DelPurge
         (
            pNWAccess pAccess,
         );

REMARKS: This call permanently deletes all files that are marked for deletion.  The
         requesting client must have console operator rights.

         When a client deletes one or more files, the files are moved to a holding
         area and marked for deletion. Such files are not purged until the next file
         create or file delete operation.

ARGS: <> pAccess

INCLUDE: ncpfile.h

RETURN:  0x0000  Successful
         0x8981  Out Of Handles
         0x8996  Server Out Of Memory
         0x8998  Disk Map Error
         0x89A1  Directory I/O Error
         0x89FF  Failure

SERVER:  2.2

CLIENT:  DOS OS2 WIN NT

SEE:     22 17  Recover Erased File (old)
         22 29  Purge Salvageable File
         22 28  Recover Salvageable File

NCP:     23 206  Purge All Erased Files

CHANGES: 16 Sep 1993 - written - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCP23s206DelPurge
(
   pNWAccess pAccess
)
{
   #define NCP_FUNCTION        ((nuint)      23)
   #define NCP_SUBFUNCTION     ((nuint8)    206)
   #define NCP_STRUCT_LEN      ((nuint16)     1)
   #define NCP_REQ_LEN         ((nuint)       3)
   #define NCP_REP_LEN         ((nuint)       0)

   nuint8 abuReq[NCP_REQ_LEN];
   nuint16 suNCPLen;

   suNCPLen = NCP_STRUCT_LEN;
   NCopyHiLo16(&abuReq[0], &suNCPLen);
   abuReq[2] = NCP_SUBFUNCTION;

   return (NWCRequestSingle(pAccess, NCP_FUNCTION, abuReq, NCP_REQ_LEN, NULL,
               NCP_REP_LEN, NULL));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/23s206.c,v 1.7 1994/09/26 17:36:00 rebekah Exp $
*/
