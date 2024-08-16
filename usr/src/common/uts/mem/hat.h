/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_HAT_H	/* wrapper symbol for kernel use */
#define _MEM_HAT_H	/* subject to change without notice */

#ident	"@(#)kern:mem/hat.h	1.35"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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

/*
 * VM - Hardware Address Translation management.
 *
 * This file describes the machine independent interfaces to
 * the hardware address translation management routines.  Other
 * machine specific interfaces and structures are defined
 * in <mem/vm_hat.h>.  The HAT layer manages the address
 * translation hardware as a cache driven by calls from the
 * higher levels of the VM system.
 */

#ifdef _KERNEL_HEADERS

#include <mem/as.h>	/* REQUIRED */
#include <mem/seg.h>	/* REQUIRED */
#include <mem/vm_hat.h> /* REQUIRED */
#include <svc/clock.h>  /* REQUIRED */
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <vm/as.h>	/* REQUIRED */
#include <vm/seg.h>	/* REQUIRED */
#include <vm/vm_hat.h>  /* REQUIRED */
#include <sys/clock.h>  /* REQUIRED */
#include <sys/types.h>	/* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

/* Structures used in prototypes below */
struct as;
struct seg;
struct page;

/*
 * One time HAT initialization routines:
 */
void hat_kas_mapping_init(ulong_t);
void hat_vis_ptaddr_init(vaddr_t);
void hat_init(void);

/*
 * Per-engine operations:
 */
void hat_online(void);
void hat_offline(void);

/*
 * Operations on hat resources for an address space:
 *	- initialize any needed hat structures for the address space
 *	- free all hat resources now owned by this address space
 *	- initialize any needed hat structures when the process is
 *	  swapped in.
 *	- free all hat resources that are not needed while the process
 *	  is swapped out.
 *	- load/unload address space.
 */
void hat_alloc(struct as *);
void hat_free(struct as *);
void hat_swapout(struct as *);
void hat_swapin(struct as *);
void hat_asunload(struct as *, boolean_t);
void hat_asload(struct as *);

/* Operation to allocate/reserve mapping structures
 *	- allocate/reserve mapping structures for a segment.
 *	- move stack segment for exec.
 *	- dup a segment
 */
uint_t hat_map(struct seg *, struct vnode *, off_t, uint_t,
		uint_t);
int hat_exec(struct as *, vaddr_t, ulong_t, struct as *,
		vaddr_t, uint_t);
vaddr_t hat_dup(struct seg *, struct seg *, uint_t);

/*
 * Operations on a named address with in a segment:
 *	- load/lock the given page struct
 *	- load/lock the given page frame number
 *	- unlock the given address
 */
void hat_memload(struct seg *, vaddr_t, struct page *, uint_t, uint_t);
void hat_devload(struct seg *, vaddr_t, uint_t, uint_t, uint_t);
void hat_unlock(struct seg *, vaddr_t);

/*
 * Operations over an address range:
 *	- age one or more segments
 *	- unload mapping
 *	- change protections
 */
vaddr_t hat_agerange(struct as *, vaddr_t, vaddr_t, enum age_type);
void hat_unload(struct seg *, vaddr_t, ulong_t, uint_t);
boolean_t hat_chgprot(struct seg *, vaddr_t, ulong_t, uint_t, boolean_t);

/*
 * Operations that work on all active translation for a given page:
 *	- unload all translations to page
 *	- get hw modbit from hardware into page struct and reset hw modbit
 * 	- get hw modbit from hardware into page struct
 */
void hat_pageunload(struct page *);
void hat_pagesyncmod(struct page *);
void hat_pagelistsyncmod(struct page **, u_int);
void hat_pagegetmod(struct page *);

/*
 * Operations that return physical page IDs (or physical addresses):
 *	- return the ppid for a kernel virtual address
 *	- return the ppid for an arbitrary physical address
 *	- return the ppid for an arbitrary virtual address of an address space
 *	- return the physical address for an arbitrary virtual address
 *		of a process
 */
ppid_t kvtoppid(caddr_t);
ppid_t phystoppid(paddr_t);
ppid_t hat_vtoppid(struct as *, vaddr_t);
paddr_t vtop(caddr_t vaddr, void *procp);

/*
 * Operation to find and read-lock the page mapped, via a visible mapping,
 * at a particular virtual address within an address space.
 * If there is no such page, or it cannot be locked w/o blocking, return NULL.
 */
struct page *hat_vtopp(struct as *, vaddr_t, enum seg_rw);

/*
 * Various kernel operations:
 *	- Refresh the hat resource pools.
 *	- TLB Shootdown service for kernel segment drivers.
 *	- Unload/load visible kernel mappings.
 *	- Fast fault code for restoring user level 1 PTEs.
 *	- Get information about the translation for a given address, if any.
 */
void hat_refreshpools(void);
void hat_shootdown(TLBScookie_t, uint_t);
void hat_kas_unload(vaddr_t addr, ulong_t nbytes, uint_t flags);
void hat_kas_memload(vaddr_t addr, struct page *pp, uint_t prot);
boolean_t hat_kas_agerange(vaddr_t addr, vaddr_t endaddr);
uint_t hat_xlat_info(struct as *as, vaddr_t addr);
void TLBSflushtlb(void);
void hat_uas_shootdown(struct as *as);
#ifndef UNIPROC
int hat_fastfault(struct as *as, vaddr_t addr, enum seg_rw rw);
#endif

/*
 * Merge stats interfaces.
 */
void hat_start_stats(struct as *, vaddr_t, ulong_t);
void hat_stop_stats(struct as *, vaddr_t, ulong_t);
void hat_check_stats(struct as *, vaddr_t, ulong_t, uint_t *, boolean_t);

/*
 * Flags to pass to HAT routines.
 *
 * Certain flags only apply to some interfaces:
 *
 * 	HAT_NOFLAGS   No flags specified.
 * 	HAT_LOCK      Lock down mapping resources; hat_map(), hat_memload(),
 * 	              and hat_devload().
 * 	HAT_UNLOCK    Unlock mapping resources; hat_memload(), hat_devload(),
 * 	              and hat_unload().
 * 	HAT_PRELOAD   Pre-load pages for new segment; hat_map().
 *	HAT_CLRMOD    Clear the translation mod bit and the mod bit in page
 *		      struct.
 *	HAT_DONTNEED  Pass dontneed hint to page_free(); hat_kas_unload().
 */
#define	HAT_NOFLAGS	0x0
#define	HAT_LOCK	0x1
#define	HAT_UNLOCK	0x2
#define	HAT_PRELOAD	0x4	/* For hat_map: preload vnode pages */
#define HAT_NOPROTW	0x8	/* For hat_dup: skip writable translations */
#define HAT_HIDDEN	0x10	/*
				 * For hat_dup: the mappings are hidden
				 * (i.e., not on any p_mapping chain); there
				 * might not even be a page_t for translations.
				 * Absence of this flag implies visible
				 * mappings and corresponding page_t's.
				 */
#define HAT_CLRMOD	0x20	/* For hat_kas_unload() */
#define HAT_DONTNEED	0x40	/* For hat_kas_unload() calling page_free() */

#define HAT_HASCOOKIE	0x200	/* For hat_shootdown: cookie arg provided */
#define HAT_NOCOOKIE	0x400	/* For hat_shootdown: no cookie, force TLBS */

/*
 * Flags returned from hat_xlat_info().
 */
#define HAT_XLAT_EXISTS	(1 << 0)	/* A translation has been loaded */
#define HAT_XLAT_READ	(1 << 1)	/* The translation is user-readable */
#define HAT_XLAT_WRITE	(1 << 2)	/* The translation is user-writeable */

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_HAT_H */
