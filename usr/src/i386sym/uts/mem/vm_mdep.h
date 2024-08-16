/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_VM_MDEP_H	/* wrapper symbol for kernel use */
#define _MEM_VM_MDEP_H	/* subject to change without notice */

#ident	"@(#)kern-i386sym:mem/vm_mdep.h	1.11"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

#ifdef _KERNEL_HEADERS

#include <io/cfg.h>

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/cfg.h>

#endif /* _KERNEL_HEADERS */

/*
 * This file contains platform specific VM definitions.
 * Architecture family specific VM definitions are contained
 * in <mem/vmparam.h>.
 */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Fixed kernel virtual addresses.
 *
 * These addresses are all mapped by a level 2 page table which
 * is replicated for each engine to allow some mappings to
 * be different on a per-engine basis.  Any other mappings 
 * that go in this page table must be statically defined for
 * the life of the system at boot.
 *
 * The mappings defined in this file are all of the
 * "statically defined" category.
 *
 * These addresses are all in the top 4 Meg of kernel virtual.
 * Each of these addresses must begin on a page boundary.
 *
 * Note:
 *
 * KVLAST_ARCH is the last fixed kernel virtual address
 * (working down from high memory) allocated by architecture
 * family specific but platform-independent code.  This symbol
 * can be used by platform-specific code (us) to begin allocating
 * additional fixed kernel virtual addresses.
 */

#define KVSYNC_POINT	(KVLAST_ARCH - MMU_PAGESIZE)
#define KVLED		(KVSYNC_POINT - MMU_PAGESIZE)
#define KVETC		(KVLED - MMU_PAGESIZE)
#define KVSLIC		(KVETC - MMU_PAGESIZE)

#define KVLAST_PLAT	KVSLIC


/*
 * Other misc kernel virtual addresses.
 */

#define KVSBASE	     ((vaddr_t)0xD0000000L) /* Base for kernel text, data, bss.
				    	     * This is hardcoded based on where
				    	     * the linker and bootstrap place
				    	     * kernel text.
				    	     */
#define	KVCD_LOC     ((struct config_desc *)(KVSBASE + (vaddr_t)CD_LOC))

#endif /* _KERNEL || _KMEMUSER */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_VM_MDEP_H */
