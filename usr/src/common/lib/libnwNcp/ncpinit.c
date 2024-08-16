/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwNcp:ncpinit.c	1.5"
#include "ntypes.h"
#include "unicode.h"
#include "nwclient.h"

/*manpage*NWNWCPInit********************************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
            NWNCPInit(void)

REMARKS: Initializes the NWNCP library.

INCLUDE: nwncp.h

RETURN:

CLIENT:  DOS WIN OS2 NT

CHANGES: 15 Jul 1993 - ported - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCPInit(pNWAccess pAccess)
{
   NWRCODE rCode;

   rCode = NWClientInit(pAccess);

   return rCode;
}

/*manpage*NWNCPTerm******************************************************
SYNTAX:  N_GLOBAL_LIBRARY( NWRCODE )
            NWNCPTerm(void)

REMARKS: Terminates the NWNCP library.

INCLUDE: nwncp.h

RETURN:  0x89ff if NtClose fails

CLIENT:  NT

CHANGES: 15 Jul 1993 - ported - jwoodbur
***************************************************************************/
N_GLOBAL_LIBRARY( NWRCODE )
NWNCPTerm(pNWAccess pAccess)
{
   NWRCODE ccode;

   ccode = NWClientTerm(pAccess);

   return ccode;
}

#if !(defined N_PLAT_MSW && defined N_ARCH_32)
N_GLOBAL_LIBRARY( nuint16 )
NWNCPEntryPoint
(
   void
)
{
   return (nuint16)(NWNCPInit(NULL) ? 1 : 0);
}
#endif
/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwNcp/ncpinit.c,v 1.7 1994/09/26 17:41:34 rebekah Exp $
*/
