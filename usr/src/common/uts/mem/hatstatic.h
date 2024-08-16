/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_HATSTATIC_H	/* wrapper symbol for kernel use */
#define _MEM_HATSTATIC_H	/* subject to change without notice */

#ident	"@(#)kern:mem/hatstatic.h	1.14"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _KERNEL_HEADERS

#include <mem/vmparam.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/vmparam.h> /* REQUIRED */
#include <sys/types.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#ifdef _KERNEL

struct page;

extern void	 hat_static_init(vaddr_t);
extern void	 hat_static_stopcalloc(void);
extern void	 hat_static_stoppalloc(void);
extern vaddr_t	 hat_static_nextvaddr(void);
extern void	*calloc(ulong_t);
extern void	*calloc_physio(ulong_t);
extern void	 callocrnd(ulong_t);
extern vaddr_t	 physmap0(paddr_t, size_t);
extern vaddr_t	 physmap1(paddr_t, paddr_t *);
extern void	 hat_statpt_alloc(vaddr_t, ulong_t);
extern void	 hat_statpt_memload(vaddr_t, ulong_t, struct page *, uint_t);
extern void	 hat_statpt_devload(vaddr_t, ulong_t, ppid_t, uint_t);
extern void	 hat_statpt_unload(vaddr_t, ulong_t);
extern void	*calloc_virtual(ulong_t);

/*
 * Flags for calloc_typed()
 */
#define PMEM_ANY	0	/* any phys mem acceptable */
#define PMEM_PHYSIO	1	/* need phys mem suitable for I/O */

/*
 * Availability of calloc:
 */
extern boolean_t hat_static_callocup;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_HATSTATIC_H */
