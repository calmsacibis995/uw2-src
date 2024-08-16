/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libNwLocale:uniopen.c	1.1"
#ident "$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uniopen.c,v 1.1 1994/09/26 17:21:42 rebekah Exp $"
/*--------------------------------------------------------------------------
     (C) Unpublished Copyright of Novell, Inc.  All Rights Reserved.

 No part of this file may be duplicated, revised, translated, localized or
 modified in any manner or compiled, linked, or uploaded or downloaded to or
 from any computer system without the prior written consent of Novell, Inc.
--------------------------------------------------------------------------*/

#if !(defined N_PLAT_MSW && defined N_ARCH_32)
#include "locdefs.h"

#if defined(N_PLAT_UNIX)
#include <fcntl.h>
#endif /* N_PLAT_UNIX */

#if defined(N_PLAT_OS2)
# include <io.h>
# include <fcntl.h>
# include <errno.h>
#endif

#if defined(N_PLAT_DOS)
# include <dos.h>

# define DOS_OPEN_HANDLE             0x3D
# define DOS_CLOSE_HANDLE            0x3E
# define DOS_READ_ONLY               0x00
# define DOS_FILE_NOT_FOUND          0x02
# define DOS_PATH_NOT_FOUND          0x03
# define DOS_NO_HANDLES_AVAILABLE    0x04
# define DOS_ACCESS_DENIED           0x05

int far _NWUniIntdos(union REGS far *rregs,
                     struct SREGS far *sregs);
#endif

#include "ntypes.h"
#include "uniintrn.h"
#include "unicode.h"

int N_FAR NWUniOpenReadOnly(nuint16 N_FAR *fileHandle,
                          char N_FAR *filePath)
{

#if defined(N_PLAT_OS2)

	 *fileHandle = open(filePath, O_RDONLY | O_BINARY);

   return (*fileHandle == 0xFFFF) ? UNI_OPEN_FAILED : 0;
	/* JBI
		The DosOpen call opens the unicode files in text mode and
		since it is a mostly binary file I thought the call above
		would work better.  It also fixed the problem when the GUI
		team saw it.
   	nuint16 actionTaken;

   	return (DosOpen(filePath, (PHFILE) fileHandle,
				(PUSHORT) &actionTaken, 0L,
      	FILE_READONLY, FILE_OPEN, OPEN_SHARE_DENYWRITE, 0) != 0)
      	? UNI_OPEN_FAILED : 0;
	JBI */

#elif ((defined N_PLAT_MSW && defined N_ARCH_16) || \
   (defined N_PLAT_MSW && defined N_ARCH_16 && defined N_PLAT_WNT))
   *fileHandle = _lopen(filePath, READ);

   return (*fileHandle == 0xFFFF) ? UNI_OPEN_FAILED : 0;

#elif defined(N_PLAT_UNIX)

   *fileHandle = open(filePath, O_RDONLY);
   return (*fileHandle == 0xFFFF) ? UNI_OPEN_FAILED : 0;

#elif defined (N_PLAT_DOS)
   int ccode;
   union REGS regs;
   struct SREGS sregs;

   regs.h.ah = DOS_OPEN_HANDLE;
   regs.h.al = DOS_READ_ONLY;
   sregs.ds = FP_SEG(filePath);
   regs.x.dx = FP_OFF(filePath);

   *fileHandle = _NWUniIntdos(&regs, &sregs);

   if (regs.x.cflag == 0)
      ccode = 0;
   else
      switch (*fileHandle)
      {
		   /*
			   No such file or directory
		   */
         case DOS_FILE_NOT_FOUND:
         case DOS_PATH_NOT_FOUND:
			   ccode = UNI_NO_SUCH_FILE;
			   break;

		   /*
			   Too many open files		
		   */
         case DOS_NO_HANDLES_AVAILABLE:
			   ccode = UNI_TOO_MANY_FILES;
			   break;

		   /*
			   Access denied
		   */
         case DOS_ACCESS_DENIED:
			   ccode = UNI_NO_PERMISSION;
			   break;

		   /*
			   Who knows?
		   */
		   default:
			   ccode = UNI_OPEN_FAILED;
			   break;
      }

   return ccode;

#endif
}
/* NWUniOpenReadOnly */


int N_FAR NWUniClose(nuint16 fileHandle)
{

#if defined(N_PLAT_OS2)

	return close(fileHandle);

	/* JBI
		Since we are doing an open() call above, I thought it
		kosher to do a close() call down here.

 	 return DosClose(fileHandle);
	JBI */

#elif ((defined N_PLAT_MSW && defined N_ARCH_16) || \
       (defined N_PLAT_MSW && defined N_ARCH_16 && defined N_PLAT_WNT))
  return _lclose(fileHandle);

#elif defined (N_PLAT_UNIX)

  return close(fileHandle);

#elif defined (N_PLAT_DOS)
   union REGS regs;

   regs.h.ah = DOS_CLOSE_HANDLE;
   regs.x.bx = fileHandle;

   return _NWUniIntdos(&regs, NULL);

#endif
}

#endif /* Don't compile this module for defined N_PLAT_MSW && defined N_ARCH_32 */

/*
$Header: /SRCS/esmp/usr/src/nw/lib/libnwLocale/uniopen.c,v 1.1 1994/09/26 17:21:42 rebekah Exp $
*/
