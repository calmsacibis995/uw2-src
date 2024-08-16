/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_SEG_H	/* wrapper symbol for kernel use */
#define _MEM_SEG_H	/* subject to change without notice */

#ident	"@(#)kern:mem/seg.h	1.27"
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

#ifdef _KERNEL_HEADERS

#include <util/types.h>	/* REQUIRED */
#include <mem/faultcode.h> /* SVR4.0COMPAT */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h>	/* REQUIRED */
#include <vm/faultcode.h> /* SVR4.0COMPAT */

#else

#include <vm/faultcode.h> /* SVR4.0COMPAT */

#endif /* _KERNEL_HEADERS */

/*
 * VM - Segments.
 */

/*
 * An address space contains a set of segments, managed by drivers.
 * Drivers support mapped devices, sharing, copy-on-write, etc.
 *
 * The seg structure contains a lock to prevent races, the base virtual
 * address and size of the segment, a back pointer to the containing
 * address space, pointers to maintain a circularly doubly linked list
 * of segments in the same address space, and procedure and data hooks
 * for the driver.  The seg list on the address space is sorted by
 * ascending base addresses and overlapping segments are not allowed.
 *
 * After a segment is created, faults may occur on pages of the segment.
 * When a fault occurs, the fault handling code must get the desired
 * object and set up the hardware translation to the object.  For some
 * objects, the fault handling code also implements copy-on-write.
 *
 * When the hat wants to unload a translation, it can call the unload
 * routine which is responsible for processing reference and modify bits.
 */

/*
 * Fault information passed to the seg fault handling routine.
 * The F_SOFTLOCK and F_SOFTUNLOCK are used by software
 * to lock and unlock pages for physical I/O. F_MAXPROT_SOFTLOCK is
 * semantically identical to F_SOFTLOCK with the exception that if
 * the target segment allows write access (for either working or 
 * maximum permissions) then S_WRITE faults will cause the page(s) 
 * to be COWed (if the segment supports COW). The pages are still loaded
 * using only the working permissions. This semantic is used by /proc.
 */
enum fault_type {
	F_INVAL,		/* invalid page */
	F_PROT,			/* protection fault */
	F_SOFTLOCK,		/* software requested locking */
	F_SOFTUNLOCK,		/* software requested unlocking */
	F_MAXPROT_SOFTLOCK	/* like F_SOFTLOCK, but protection check */
};

/*
 * seg_rw gives the access type for a fault operation
 */
enum seg_rw {
	S_OTHER,		/* unknown or not touched */
	S_READ,			/* read access attempted */
	S_WRITE,		/* write access attempted */
	S_OVERWRITE,		/* write access with exact range */
	S_EXEC 			/* execution access attempted */
};

struct vnode;	/* To satisfy the compiler about the prototypes below */
struct page;

struct seg {
	vaddr_t	s_base;			/* base virtual address */
	uint_t	s_size;			/* size in bytes */
	struct	as *s_as;		/* containing address space */
	struct	seg *s_next;		/* next seg in this address space */
	struct	seg *s_prev;		/* prev seg in this address space */
	struct	seg_ops {
		int (*sop_unmap)(struct seg *, vaddr_t, uint_t);
		void (*sop_free)(struct seg *);
		faultcode_t (*sop_fault)(struct seg *, vaddr_t, uint_t,
					 enum fault_type, enum seg_rw);
		int (*sop_setprot)(struct seg *, vaddr_t, uint_t, uint_t);
		int (*sop_checkprot)(struct seg *, vaddr_t, uint_t);
		int (*sop_kluster)(struct seg *, vaddr_t, int);
		int (*sop_sync)(struct seg *, vaddr_t, uint_t, int, uint_t);
		int (*sop_incore)(struct seg *, vaddr_t, uint_t, char *);
		int (*sop_lockop)(struct seg *, vaddr_t, uint_t, int, int);
		int (*sop_dup)(struct seg *, struct seg *);
		void (*sop_childload)(struct seg *, struct seg *);
		int (*sop_getprot)(struct seg *, vaddr_t, uint_t *);
		off_t (*sop_getoffset)(struct seg *, vaddr_t);
		int (*sop_gettype)(struct seg *, vaddr_t);
		int (*sop_getvp)(struct seg *, vaddr_t, struct vnode **);
		void (*sop_age)(struct seg *, uint_t);
		boolean_t (*sop_lazy_shootdown)(struct seg *, vaddr_t);
		int (*sop_memory)(struct seg *, vaddr_t *basep, u_int *lenp);
	} *s_ops;
	void *s_data;			/* private data for instance */
};

#define SOP_UNMAP(seg, addr, len) \
	((*(seg)->s_ops->sop_unmap)((seg), (addr), (len))) 

#define SOP_FREE(seg) \
	((*(seg)->s_ops->sop_free)((seg)))

#define SOP_FAULT(seg, addr, len, type, rw) \
	((*(seg)->s_ops->sop_fault)((seg), (addr), (len), (type), (rw)))

#define SOP_SETPROT(seg, addr, len, prot) \
	((*(seg)->s_ops->sop_setprot)((seg), (addr), (len), (prot)))

#define SOP_CHECKPROT(seg, addr, prot) \
	((*(seg)->s_ops->sop_checkprot)((seg), (addr), (prot)))

#define SOP_KLUSTER(seg, addr, delta) \
	((*(seg)->s_ops->sop_kluster)((seg), (addr), (delta)))

#define SOP_SYNC(seg, addr, len, attr, flags) \
	((*(seg)->s_ops->sop_sync)((seg), (addr), (len), (attr), (flags)))

#define SOP_INCORE(seg, addr, len, vec) \
	((*(seg)->s_ops->sop_incore)((seg), (addr), (len), (vec)))

#define SOP_LOCKOP(seg, addr, len, attr, op) \
	((*(seg)->s_ops->sop_lockop)((seg), (addr), (len), (attr), (op)))

#define	SOP_DUP(seg, nseg) \
	((*(seg)->s_ops->sop_dup)((seg), (nseg))) 

#define	SOP_CHILDLOAD(pseg, cseg) \
	((*(pseg)->s_ops->sop_childload)((pseg), (cseg))) 

#define SOP_GETPROT(seg, addr, protv) \
	((*(seg)->s_ops->sop_getprot)((seg), (addr), (protv)))

#define SOP_GETOFFSET(seg, addr) \
	((*(seg)->s_ops->sop_getoffset)((seg), (addr)))

#define SOP_GETTYPE(seg, addr) \
	((*(seg)->s_ops->sop_gettype)((seg), (addr)))

#define SOP_GETVP(seg, addr, vpp) \
	((*(seg)->s_ops->sop_getvp)((seg), (addr), (vpp)))

#define SOP_AGE(seg, flag) \
	((*(seg)->s_ops->sop_age)((seg), (flag)))

#define SOP_LAZY_SHOOTDOWN(seg, addr) \
	((*(seg)->s_ops->sop_lazy_shootdown)((seg), (addr)))

#define SOP_MEMORY(seg, basep, lenp) \
	((*(seg)->s_ops->sop_memory)((seg), (basep), (lenp)))

#ifdef _KERNEL

/*
 * Generic segment operations
 */
struct seg *seg_alloc(struct as *, vaddr_t, uint_t);
int seg_attach(struct as *, vaddr_t, uint_t, struct seg *);
void seg_free(struct seg *);
void seg_detach(struct seg *);

#ifdef DEBUG

uint_t seg_page(struct seg *, vaddr_t);
uint_t seg_pages(struct seg *);

#else

#define seg_page(seg, addr)	btop((addr) - (seg)->s_base)
#define seg_pages(seg)		btopr((seg)->s_size)

#endif

extern void seg_unmap(struct seg *);

#endif	/* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif	/* _MEM_SEG_H */
