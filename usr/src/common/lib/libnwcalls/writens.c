/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:writens.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "ncpfile.h"

#include "nwcaldef.h"
#include "nwnamspc.h"

/*manpage*NWWriteNSInfo*****************************************************
SYNTAX:  NWCCODE N_API NWWriteNSInfo
         (
            NWCONN_HANDLE conn,
            NW_IDX NWPTR idxStruct,
            NW_NS_INFO NWPTR NSInfo,
            pnuint8 pNSInfoB512
         )

REMARKS: Sets the specific name space information.  For name spaces 1+, this
         call is passed to the appropriate name space NLM on the server.  For
         name space 0 (DOS), the server will process the name space.

         Avoid setting the first field of the name space information. This is
         generally the name and is intended to be read only. To rename a file
         use NWSetLongName.

ARGS:
      >  idxStruct
         the structure returned from the NWNSGetMiscInfo function.

      >  NSInfo
         Pointer to the struct returned by NWGetNSInfo.

      >  data
         Pointer to a 512 byte buffer for the data to be written to the name
         space. (The actual format of the data is determined by the NLM on
         the server. Case should be taken if the format is unknown.)

INCLUDE: nwnamspc.h

RETURN:

SERVER:  3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     87 25  Set NS Information

CHANGES: 14 Sep 1993 - NWNCP Enabled - rivie
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWWriteNSInfo
(
  NWCONN_HANDLE   conn,
  NW_IDX NWPTR    idxStruct,
  NW_NS_INFO      NWPTR NSInfo,
  pnuint8         pNSInfoB512
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, conn);

   return ((NWCCODE)NWNCP87s25NSSetInfo(&access, idxStruct->srcNameSpace,
                        idxStruct->dstNameSpace, idxStruct->volNumber,
                        idxStruct->srcDirBase, NSInfo->NSInfoBitMask,
                        pNSInfoB512));
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/writens.c,v 1.7 1994/09/26 17:50:36 rebekah Exp $
*/
