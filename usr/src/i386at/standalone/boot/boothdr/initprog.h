/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef _INITPROG_H
#define _INITPROG_H

#ident	"@(#)stand:i386at/standalone/boot/boothdr/initprog.h	1.2.1.5"
#ident  "$Header: $"

/*
 * Copyrighted as an unpublished work.
 * (c) Copyright INTERACTIVE Systems Corporation 1986, 1988, 1990
 * All rights reserved.
 *
 * RESTRICTED RIGHTS
 *
 * These programs are supplied under a license.  They may be used,
 * disclosed, and/or copied only as permitted under such license
 * agreement.  Any copy must contain the above copyright notice and
 * this restricted rights notice.  Use, copying, and/or disclosure
 * of the programs is strictly prohibited unless otherwise provided
 * in the license agreement.
 */

#ifndef _BOOT_BOOTLINK_H
#include <boothdr/bootlink.h>
#endif	/* _BOOT_BOOTLINK_H */

/*
 *	The following must be declared and set by the initprog 
 *	program when it is called from boot.
 */
extern	struct	bootfuncs	*bfp;

#define bootabort	(bfp->b_abort)
#define bprintf	(bfp->b_printf)
#define bstrcpy	(bfp->b_strcpy)
#define bstrncpy	(bfp->b_strncpy)
#define bstrcat	(bfp->b_strcat)
#define bstrlen	(bfp->b_strlen)
#define bmalloc	(bfp->b_malloc)
#define bmemcpy	(bfp->b_memcpy)
#define bmemcmp	(bfp->b_memcmp)
#define bmemset	(bfp->b_memset)
#define bgetchar	(bfp->b_getchar)
#define bputchar	(bfp->b_putchar)
#define bgets	(bfp->b_bgets)
#define ischar	(bfp->b_ischar)
#define doint	(bfp->b_doint)
#define goany	(bfp->b_goany)
#define CMOSread	(bfp->b_CMOSread)
#define shomem	(bfp->b_shomem)
#define bread	(bfp->b_read)
#define getfhdr	(bfp->getfhdr)
#define checkkbd	(bfp->checkkbd)


#define BL_file_open	(bfp->file_open)
#define BL_file_close	(bfp->file_close)
#define BL_file_read	(bfp->file_read)
#define BL_file_lseek	(bfp->file_lseek)

#endif	/* _BOOT_INITPROG_H */
