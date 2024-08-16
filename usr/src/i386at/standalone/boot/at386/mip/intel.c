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

#ident	"@(#)stand:i386at/standalone/boot/at386/mip/intel.c	1.1"
#ident	"$Header: $"

#include <sys/types.h>
#include <sys/cram.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/mip.h>

#define	INTL_SETREG	0x28	/* CMOS setup register */
#define	INTL_16MBLIM	0x40	/* CMOS setup reg above 16MB disabled flag */

intel(lpcbp, machine)
struct	lpcb *lpcbp;
unsigned char machine;
{
	int	i;
	unsigned char intel_setreg;

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & BOOTTALK)
		printf("Begin Intel 386 initalization.\n");
#endif

	a20();		/* set A20 to address above 1MB */

	i=0;
	BOOTENV->memrng[i].base = 0;
	BOOTENV->memrng[i].extent = 640 * 1024;
	BOOTENV->memrng[i++].flags = B_MEM_BASE;
	BOOTENV->memrng[i].base = 0x100000;
	BOOTENV->memrng[i].extent = 0xF00000;
	BOOTENV->memrng[i++].flags = B_MEM_EXPANS;

	intel_setreg = CMOSread(INTL_SETREG);
	if (!(intel_setreg & INTL_16MBLIM)) {
		BOOTENV->memrng[i].base = MEM16M;
		BOOTENV->memrng[i].extent = MEM16M;
		BOOTENV->memrng[i++].flags = B_MEM_EXPANS;
	}
	BOOTENV->memrngcnt = i;

	/* No final startup procedure support */
	MIP_end = (ulong) NULL;
}
