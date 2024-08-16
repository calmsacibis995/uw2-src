/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwCalls:gtreqver.c	1.5"
#if defined N_PLAT_WNT && defined N_ARCH_32
#include <windows.h>
#endif
#include "ntypes.h"
#include "nwcaldef.h"
#include "nwcint.h"
#include "nwundoc.h"
#include "nwmisc.h"
#include "nwclient.h"


/*manpage*NWGetRequesterVersion*********************************************
SYNTAX:  NWCCODE N_API NWGetRequesterVersion
         (
            pnuint8 majorVer,
            pnuint8 minorVer,
            pnuint8 rev
         )

REMARKS:

ARGS: <  majorVer
      <  minorVer
      <  rev

INCLUDE: nwmisc.h

RETURN:

CLIENT:  DOS WIN OS2 NT NLM

SEE:

CHANGES: 20 Sep 1993 - bible enabled - jsumsion
----------------------------------------------------------------------------
         Copyright (c) 1993 by Novell, Inc. All rights reserved
***************************************************************************/
NWCCODE N_API NWGetRequesterVersion
(
   pnuint8 majorVer,
   pnuint8 minorVer,
   pnuint8 rev
)
{
#if defined N_PLAT_MSW || \
    defined(N_PLAT_UNIX) || \
    defined(N_PLAT_NLM) || \
    defined(N_PLAT_DOS)

	nuint          uMajorVersion;
	nuint          uMinorVersion;
	nuint          uRevision;
   NWCCODE        ccode;

   NWCDeclareAccess(access);

   if ( (ccode = (NWCCODE)NWCGetRequesterVersion(&access,
                                        &uMajorVersion,
                                        &uMinorVersion,
                                        &uRevision)) == 0 )
   {
      if(majorVer)
         *majorVer = (nuint8) uMajorVersion;
      if(minorVer)
         *minorVer = (nuint8) uMinorVersion;
      if(rev)
         *rev = (nuint8) uRevision;
   }

   return(ccode);

#elif defined(N_PLAT_OS2)

   nuint8 version[3];
   NWCCODE ccode;

   if((ccode = NWCCallGate(_NWC_GET_SHELL_VERSION, version)) == 0)
   {
      if(majorVer)
         *majorVer = version[0];
      if(minorVer)
         *minorVer = version[1];
      if(rev)
         *rev = version[2];
   }
   return (ccode);

#endif
}

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwcalls/gtreqver.c,v 1.7 1994/09/26 17:47:27 rebekah Exp $
*/

