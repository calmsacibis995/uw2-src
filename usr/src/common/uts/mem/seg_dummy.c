/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/seg_dummy.c	1.8"
#ident	"@(#)kern:mem/seg_dummy.c	1.7"

/*
 * VM - dummy segment driver.
 *
 * This segment driver is used to reserve sections of address space
 * without actually making any mappings.
 */

#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/seg.h>
#include <mem/seg_dummy.h>
#include <proc/mman.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Private seg op routines.
 */
STATIC int segdummy_dup(struct seg *, struct seg *);
STATIC int segdummy_unmap(struct seg *, vaddr_t, uint_t);
STATIC faultcode_t segdummy_fault(struct seg *, vaddr_t, uint_t,
				  enum fault_type, enum seg_rw);
STATIC int segdummy_incore(struct seg *, vaddr_t, uint_t, char *);
STATIC int segdummy_getprot(struct seg *, vaddr_t, uint_t *);
STATIC off_t segdummy_getoffset(struct seg *, vaddr_t);
STATIC int segdummy_gettype(struct seg *, vaddr_t);
STATIC int segdummy_getvp(struct seg *, vaddr_t, vnode_t **);
STATIC void segdummy_badop(void);
STATIC int segdummy_nop(void);
STATIC int segdummy_memory(struct seg *, vaddr_t *, u_int *);

STATIC struct seg_ops segdummy_ops = {
	segdummy_unmap,
	(void (*)())segdummy_nop,	/* free */
	segdummy_fault,
	(int (*)())segdummy_nop,	/* setprot */
	(int (*)())segdummy_nop,	/* checkprot */
	(int (*)())segdummy_badop,	/* kluster */
	(int (*)())segdummy_nop,	/* sync */
	segdummy_incore,
	(int (*)())segdummy_nop,	/* lockop */
	segdummy_dup,
	(void (*)())segdummy_nop,	/* childload */
	segdummy_getprot,
	segdummy_getoffset,
	segdummy_gettype,
	segdummy_getvp,
	(void (*)())NULL,		/* age */
	(boolean_t (*)())segdummy_nop,	/* lazy_shootdown */
	segdummy_memory
};

STATIC u_int	segdummy_data;


/*
 * int
 * segdummy_create(struct seg *seg, void *argsp)
 *	Create a dummy user segment.
 *
 * Calling/Exit State:
 *	Called with the AS write locked.
 *	Returns with the AS write locked.
 *
 * Description:
 *	A dummy segment never loads any translations.  Faults always fail.
 *	This can be used as a placeholder, to reserve specific addresses
 *	and prevent them from being used.
 */
/*ARGSUSED*/
int
segdummy_create(struct seg *seg, void *argsp)
{
	seg->s_ops = &segdummy_ops;

	/* s_data isn't used, so just point all of them to a single location */
	seg->s_data = &segdummy_data;
	seg->s_as->a_isize += seg->s_size;

	return 0;
}

/*
 * STATIC void
 * segdummy_badop(void)
 *	Illegal operation.
 *
 * Calling/Exit State:
 *	Always panics.
 */
STATIC void
segdummy_badop(void)
{
	/*
	 *+ A segment operation was invoked which is not supported by the
	 *+ segdummy segment driver.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "segdummy_badop");
	/*NOTREACHED*/
}

/*
 * STATIC void
 * segdummy_nop(void)
 *	Do-nothing operation.
 *
 * Calling/Exit State:
 *	Always returns success w/o doing anything.
 */
STATIC int
segdummy_nop(void)
{
	return 0;
}

/*
 * STATIC int
 * segdummy_memory(struct seg *, vaddr_t *basep, uint *lenp)
 *      This is a no-op for segdummy.
 *
 * Calling/Exit State:
 *      returns ENOMEM.
 */
/*ARGSUSED*/
STATIC int
segdummy_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
{
	return ENOMEM;
}

/*
 * STATIC int
 * segdummy_dup(struct seg *pseg, struct seg *cseg)
 *	Called from as_dup to replicate segment specific data structures.
 *
 * Calling/Exit State:
 *	The parent's address space is read locked on entry to the call and
 *	remains so on return.
 *
 *	The child's address space is not locked on entry to the call since
 *	there can be no active LWPs in it at this point in time.
 *
 * Description:
 *	Since seg_dummy never loads translations, we just need to create
 *	a new seg_dummy segment in the childs address-space.
 */
/*ARGSUSED*/
STATIC int
segdummy_dup(struct seg *pseg, struct seg *cseg)
{
	return segdummy_create(cseg, NULL);
}

/*
 * STATIC int
 * segdummy_unmap(struct seg *seg, vaddr_t addr, size_t len)
 *	Unmap a portion (possibly all) of the specified segment.
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusivley locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 *
 *	On success, 0 is returned and the request chunk of the address
 *	space has been deleted. On failure, non-zero is returned and
 *	indicates the appropriate errno.
 *
 * Remarks:
 *	If the range unmapped falls into the middle of a segment the
 *	result will be the creation of a hole in the address space and
 *	the creation of a new segment.
 */
/*ARGSUSED*/
STATIC int
segdummy_unmap(struct seg *seg, vaddr_t addr, size_t len)
{
	struct seg *nseg;	/* new segment, for split case */
	vaddr_t nbase;		/* base addr of new seg */
	size_t nsize;		/* size of new seg */

	/*
	 * Check for entire segment
	 */
	if (addr == seg->s_base && len == seg->s_size) {
		seg_free(seg);
		return 0;
	}

	seg->s_as->a_isize -= len;

	/*
	 * Check for beginning of segment
	 */
	if (addr == seg->s_base) {
		seg->s_base += len;
		seg->s_size -= len;
		return 0;
	}

	/*
	 * Check for end of segment
	 */
	if (addr + len == seg->s_base + seg->s_size) {
		seg->s_size -= len;
		return 0;
	}

	/*
	 * The section to go is in the middle of the segment,
	 * have to make it into two segments.  nseg is made for
	 * the high end while seg is cut down at the low end.
	 */
	nbase = addr + len;				/* new seg base */
	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
	seg->s_size = addr - seg->s_base;		/* shrink old seg */
	nseg = seg_alloc(seg->s_as, nbase, nsize);
	ASSERT(nseg != NULL);

	return segdummy_create(nseg, NULL);
}

/*
 * STATIC faultcode_t
 * segdummy_fault(struct seg *seg, vaddr_t addr, size_t len,
 *		  enum fault_type type, enum seg_rw rw)
 *	Fault handler; called for both hardware faults and softlock requests.
 *
 * Calling/Exit State:
 *	Called with the AS lock held (in read mode) and returns the same.
 *
 *	Addr and len arguments have been properly aligned and rounded
 *	with respect to page boundaries by the caller (this is true of
 *	all SOP interfaces).
 *
 *	On success, 0 is returned and the requested fault processing has
 *	taken place. On error, non-zero is returned in the form of a
 *	fault error code.
 *
 * Description:
 *	Faults on seg_dummy always fail.
 */
/*ARGSUSED*/
STATIC faultcode_t
segdummy_fault(struct seg *seg, vaddr_t addr, size_t len, enum fault_type type,
	       enum seg_rw rw)
{
	return FC_HWERR;
}

/*
 * STATIC int
 * segdummy_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
 *	Return the protections on pages starting at addr for len.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	This function, which cannot fail, returns the permissions of the
 *	indicated pages in the protv array.
 */
/*ARGSUSED*/
STATIC int
segdummy_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
{
	*protv = PROT_NONE;
	return 0;
}

/*
 * STATIC off_t
 * segdummy_getoffset(struct seg *seg, vaddr_t addr)
 *	Return the vnode offset mapped at the given address within the segment.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
/* ARGSUSED */
STATIC off_t
segdummy_getoffset(struct seg *seg, vaddr_t addr)
{
	return (off_t)0;
}

/*
 * STATIC int
 * segdummy_gettype(struct seg *seg, vaddr_t addr)
 *	Return the segment type (MAP_SHARED||MAP_PRIVATE) to the caller.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
/* ARGSUSED */
STATIC int
segdummy_gettype(struct seg *seg, vaddr_t addr)
{
	return MAP_PRIVATE;
}

/*
 * STATIC int
 * segdummy_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
 *	Return the vnode associated with the segment.
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	The AS needs to be locked to prevent an unmap from occuring
 *	in parallel and is usually already held for other reasons by
 *	the caller.
 */
/* ARGSUSED */
STATIC int
segdummy_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
{
	*vpp = NULL;
	return -1;
}

/*
 * STATIC int
 * segdummy_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
 *	Return an indication, in the array, vec, of whether each page
 *	in the given range is "in core".
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	"Pages" for segdummy are never "in core", so set all to false.
 */
/*ARGSUSED*/
STATIC int
segdummy_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
{
	bzero(vec, len = btop(len));
	return ptob(len);
}
