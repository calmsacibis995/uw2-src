/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


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

#ident	"@(#)stand:i386at/standalone/boot/at386/mip/necpm.c	1.1"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/mip.h>

necpm(lpcbp, machine)
struct	lpcb *lpcbp;
unsigned char machine;
{
#ifdef BOOT_DEBUG
	if ( BOOTENV->db_flag & BOOTTALK )
		printf("Begin NEC Powermate initalization.\n");
#endif

	/* establish potential memory availability */

	BOOTENV->memrng[0].base = 0;
	BOOTENV->memrng[0].extent = 640 * 1024;	/* base 640K 		   */
	BOOTENV->memrng[0].flags = B_MEM_BASE;

	BOOTENV->memrng[1].base = 0x100000;	/* expansion memory at 1MB */
	BOOTENV->memrng[1].extent = 0xEC0000;	/* up to 14.75Mbyte	   */
	BOOTENV->memrng[1].flags = B_MEM_EXPANS;

	BOOTENV->memrngcnt = 2;

	a20();		/* set A20 to address above 1MB */

/*	No final startup procedure support			*/
	MIP_end = (ulong) NULL;
}

