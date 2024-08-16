/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:mem/pse.cf/Stubs.c	1.5"
#ident	"$Header: $"

/* Stubs for segpse module */

#include <sys/types.h>
#include <sys/errno.h>
#include <sys/vnode.h>
#include <sys/vmparam.h>

#ifndef	NOWORKAROUND
uint_t pse_major = UINT_MAX;
#endif
uint_t pse_physmem = 0;
asm(".globl segpse_create ; segpse_create: ; jmp segdev_create;");
asm(".globl pse_segmap ; pse_segmap: ; jmp spec_segmap;");
ppid_t pse_mmap() { return NOPAGE; }
void pse_pagepool_init() { }
vnode_t *pse_makevp() { return NULL; }
caddr_t pse_physmap() { return NULL; }
boolean_t pse_physmap_free() { return B_FALSE; }
