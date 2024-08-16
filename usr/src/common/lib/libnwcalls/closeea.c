/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:closeea.c	1.5"
#include "ntypes.h"
#include "nwaccess.h"

#include "ncpea.h"
#include "nwcaldef.h"
#include "nwea.h"

/*manpage*NWCloseEA*********************************************************
SYNTAX:  NWCCODE N_API NWCloseEA
         (
            NW_EA_HANDLE N_FAR * EAHandle
         )

REMARKS: Closes the EA specified by EAHandle.

         NWCloseEA must be called to save any changes made to an EA.

ARGS:

INCLUDE: nwnamspc.h

RETURN:

SERVER:  2.2 3.11 4.0

CLIENT:  DOS WIN OS2 NT

SEE:

NCP:     86 01  Close Extended Attribute Handle

CHANGES: 27 Apr 1992 - fixed - jwoodbur
            If EAHandle is 0 and keyUsed is 0, then call returns 0,
            since a NWReadEA or NWWriteEA was probably NOT attempted.
         10 Aug 1993 - NWNCP Enabled - jwoodbur
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
****************************************************************************/
NWCCODE N_API NWCloseEA
(
   NW_EA_HANDLE N_FAR * EAHandle
)
{
   NWCDeclareAccess(access);

   NWCSetConn(access, EAHandle->connID);

   if(EAHandle->EAHandle)
   {
      return (NWCCODE) NWNCP86s1EACloseHandle(&access, (nuint16) 0,
                                           EAHandle->EAHandle);
   }
   else if(!EAHandle->keyUsed)
      return(0);
   else
      return(INVALID_EA_HANDLE);

}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/closeea.c,v 1.7 1994/09/26 17:44:25 rebekah Exp $
*/

