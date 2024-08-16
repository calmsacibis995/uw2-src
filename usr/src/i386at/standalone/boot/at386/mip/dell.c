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

#ident	"@(#)stand:i386at/standalone/boot/at386/mip/dell.c	1.1"
#ident	"$Header: $"

/*
 *	Initalization program for dell 80386 machines.
 */

#include <sys/types.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/initprog.h>
#include <boothdr/libfm.h>
#include <boothdr/mip.h>

#ifdef BOOT_DEBUG
#define IDENT325	(char *)0xfe082
#endif /* BOOT_DEBUG */

char	id325[] = "325";

dell(lpcbp, machine)
struct	lpcb *lpcbp;
unsigned char machine;
{

#ifdef BOOT_DEBUG
	if (BOOTENV->db_flag & BOOTTALK)
		printf("Begin Dell initalization.\n");
#endif

	BTE_INFO.machflags |= CPQ_CACHE;

	/* Check for a model 325 for Weitek workaround */

#ifdef BOOT_DEBUG
	if (memcmp(IDENT325,id325,3) == 0) {

		if (BOOTENV->db_flag & BOOTTALK)
			printf("Have a Dell model 325\n");
	}
#endif /* BOOT_DEBUG */
	a20();		/* set A20 to address above 1MB */

/*	No final startup procedure support			*/
	MIP_end = (ulong) NULL;
}

