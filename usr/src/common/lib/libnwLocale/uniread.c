/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:uniread.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uniread.c,v 1.1 1994/09/26 17:21:46 rebekah Exp $"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#ifndef WIN32
#include "locdefs.h"

#if defined(N_PLAT_UNIX)
#include <fcntl.h>
#endif /* N_PLAT_UNIX */

#if defined(NWOS2)
# include <io.h>
#endif

#if defined(NWDOS)
# include <dos.h>

# define DOS_READ_FROM_FILE          0x3F

int far _NWUniIntdos(union REGS far *rregs,
                     struct SREGS far *sregs);
#endif

#include "ntypes.h"
#include "uniintrn.h"
#include "unicode.h"

int N_FAR NWUniRead(nuint16 fileHandle,
                  void N_FAR *buffer,
                  nuint16 N_FAR *readLength)
{

#if defined(NWOS2)
   *readLength = read(fileHandle, buffer, *readLength);

   return (*readLength == 0xFFFF) ? -1 : 0;

#elif defined(NWWIN)
   *readLength = _lread(fileHandle, buffer, *readLength);

   return (*readLength == 0xFFFF) ? -1 : 0;

#elif defined(N_PLAT_UNIX)

   *readLength = read(fileHandle, buffer, *readLength);
   return (*readLength == 0xFFFF) ? -1 : 0;

#else  /* NWDOS */
   union REGS regs;
   struct SREGS sregs;

   regs.h.ah = DOS_READ_FROM_FILE;
   regs.x.bx = fileHandle;
   regs.x.cx = *readLength;
   sregs.ds = FP_SEG(buffer);
   regs.x.dx = FP_OFF(buffer);

   *readLength = _NWUniIntdos(&regs, &sregs);

   return (regs.x.cflag != 0) ? -1 : 0;

#endif
}

#endif /* Don't compile this module for WIN32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uniread.c,v 1.1 1994/09/26 17:21:46 rebekah Exp $
*/
