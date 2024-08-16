/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:unpackin.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"
#include "unicode.h"
#include "nwclient.h"
#include "ncpfile.h"

/*manpage*NWNCPUnpackFileInfo*********************************************
SYNTAX:  N_EXTERN_LIBRARY( void )
         NWNCPUnpackFileInfo
         (
            pNWNCPFileInfo  pFileInfo,
            pnuint8         pbuDataB136
         )

REMARKS: Unpacks File Information

ARGS:  < pFileInfo
       > pbuDataB30

INCLUDE: ncpfile.h

CHANGES: 4 Oct 1993 - written - lbendixs
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( void )
NWNCPUnpackFileInfo
(
   pNWNCPFileInfo  pFileInfo,
   pnuint8         pbuDataB30
)
{
   if (pFileInfo != NULL)
   {
      NCopyLoHi16(&pFileInfo->suReserved, &pbuDataB30[0]);
      NWCMemMove(&pFileInfo->abuFileName[0], &pbuDataB30[2], 14);
      pFileInfo->buAttrs   = pbuDataB30[16];
      pFileInfo->buExeType = pbuDataB30[17];
      NCopyLoHi32(&pFileInfo->luSize, &pbuDataB30[18]);
      NCopyLoHi16(&pFileInfo->suCreationDate, &pbuDataB30[22]);
      NCopyLoHi16(&pFileInfo->suAccessedDate, &pbuDataB30[24]);
      NCopyLoHi16(&pFileInfo->suModifiedDate, &pbuDataB30[26]);
      NCopyLoHi16(&pFileInfo->suModifiedTime, &pbuDataB30[28]);
   }
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/unpackin.c,v 1.7 1994/09/26 17:41:37 rebekah Exp $
*/
