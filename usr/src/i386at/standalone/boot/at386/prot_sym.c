/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)stand:i386at/standalone/boot/at386/prot_sym.c	1.1.2.1"
#ident	"$Header: $"

#include <sys/types.h>
#include <boothdr/bootdef.h>

size_t __SYMBOL__BOOT_STACKSIZ = BOOT_STACKSIZ;
size_t __SYMBOL__STK_SBML = STK_SBML;
size_t __SYMBOL__STK_SPC = STK_SPC;
size_t __SYMBOL__STK_SPT = STK_SPT;
size_t __SYMBOL__STK_BPS = STK_BPS;
size_t __SYMBOL__STK_PS = STK_PS;
size_t __SYMBOL__PROTMASK = PROTMASK;
size_t __SYMBOL__NOPROTMASK = NOPROTMASK;
size_t __SYMBOL__B_GDT = B_GDT;
size_t __SYMBOL__C_GDT = C_GDT;
size_t __SYMBOL__C16GDT = C16GDT;
size_t __SYMBOL__D_GDT = D_GDT;
