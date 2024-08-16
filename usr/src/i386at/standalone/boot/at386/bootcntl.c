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

#ident	"@(#)stand:i386at/standalone/boot/at386/bootcntl.c	1.5.1.5"
#ident	"$Header: $"


#include <sys/types.h>
#include <sys/bootinfo.h>
#include <boothdr/boot.h>
#include <boothdr/bootcntl.h>
#include <boothdr/bootdef.h>

/* debug flags 	    */
#ifdef BOOT_DEBUG
#define DEBUG_FLAGS	LOADDBG|BOOTDBG|BOOTTALK|MEMDEBUG
#else
#define DEBUG_FLAGS	0			
#endif

struct	bootcntl	bootcntl = {
	BPRG_MAGIC,		/* boot program ID			    */
	BE_BIOSRAM,		/* Init value for bootstrap internal flags */
	30,			/* 30 sec delay for kernel path 	    */
	DEBUG_FLAGS,
	1,			/* Autoboot = TRUE 			    */
	4,			/* memory range count			    */
	{			/* Begin default memrng definitions 	    */
	0, 640 * 1024, B_MEM_BASE,		/* 640K 0-640K		    */
	0x100000, 15*1024*1024, B_MEM_EXPANS|B_MEM_FORCE,	/* 15M  1M-16M		    */
	0x1000000, 0x3000000, B_MEM_EXPANS|B_MEM_FORCE,	/* 48M  16M-64M		    */
	0x4000000, 0x4000000, B_MEM_EXPANS|B_MEM_FORCE,	/* 64M	64M-128M	    */
	0,0,0
	},
	"unix",
	"sip",
	"mip",
	"dcmp",
	"logo.img",
	"resmgr",
	"Booting the UNIX System...",
	"Entering BOOT interactive session...[? for help]",
};
