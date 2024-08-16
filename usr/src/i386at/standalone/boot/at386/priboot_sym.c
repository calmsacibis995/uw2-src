/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/priboot_sym.c	1.1.1.2"
#ident	"$Header: $"

#include <sys/types.h>
#include <boothdr/bootdef.h>

size_t __SYMBOL__BOOT_STACKSIZ = BOOT_STACKSIZ;

#include "sbt_pconf.h"		 /* hard disk boot */

#ifdef WINI

size_t __SYMBOL__BOOTDRV_HD = BOOTDRV_HD;
size_t __SYMBOL__B_BOOTSZ = B_BOOTSZ;
size_t __SYMBOL__B_FD_NUMPART = B_FD_NUMPART;
size_t __SYMBOL__B_ACTIVE = B_ACTIVE;

#elif HDTST

size_t __SYMBOL__BOOTDRV_FP = BOOTDRV_FP;
size_t __SYMBOL__FDB_ADDR = FDB_ADDR;

#else

size_t __SYMBOL__BOOTDRV_FP = BOOTDRV_FP;
size_t __SYMBOL__FDB_ADDR = FDB_ADDR;

#endif

size_t __SYMBOL__SECBOOT_ADDR = SECBOOT_ADDR;
size_t __SYMBOL__PRIBOOT_ADDR = PRIBOOT_ADDR;
size_t __SYMBOL__PROG_SECT = PROG_SECT;
