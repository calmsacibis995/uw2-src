/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

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

#ident	"@(#)kern:mem/seg_dev.c	1.21"
#ident	"$Header: $"

/*
 * VM - user segment driver for mapped devices.
 *
 * This segment driver is used when mapping character special devices.
 */

#include <fs/vnode.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/seg.h>
#include <mem/seg_dev.h>
#include <mem/vmparam.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Private seg op routines.
 */
STATIC int segdev_dup(struct seg *, struct seg *);
STATIC int segdev_unmap(struct seg *, vaddr_t, uint_t);
STATIC void segdev_free(struct seg *);
STATIC faultcode_t segdev_fault(struct seg *, vaddr_t, uint_t, enum fault_type,
				enum seg_rw);
STATIC int segdev_setprot(struct seg *, vaddr_t, uint_t, uint_t);
STATIC int segdev_checkprot(struct seg *, vaddr_t, uint_t);
STATIC int segdev_incore(struct seg *, vaddr_t, uint_t, char *);
STATIC int segdev_getprot(struct seg *, vaddr_t, uint_t *);
STATIC off_t segdev_getoffset(struct seg *, vaddr_t);
STATIC int segdev_gettype(struct seg *, vaddr_t);
STATIC int segdev_getvp(struct seg *, vaddr_t, vnode_t **);
STATIC void segdev_badop(void);
STATIC int segdev_nop(void);
STATIC void segdev_age(struct seg *, u_int);
STATIC int segdev_memory(struct seg *, vaddr_t *basep, u_int *lenp);

STATIC struct seg_ops segdev_ops = {
	segdev_unmap,
	segdev_free,
	segdev_fault,
	segdev_setprot,
	segdev_checkprot,
	(int (*)())segdev_badop,	/* kluster */
	(int (*)())segdev_nop,		/* sync */
	segdev_incore,
	(int (*)())segdev_nop,		/* lockop */
	segdev_dup,
	(void(*)())segdev_nop,		/* childload */
	segdev_getprot,
	segdev_getoffset,
	segdev_gettype,
	segdev_getvp,
	segdev_age,			/* age */
	(boolean_t (*)())segdev_nop,	/* lazy_shootdown */
	segdev_memory
};

/* Other private functions */
STATIC segdev_page_t *segdev_vpage_init(struct seg *seg,
					struct segdev_data *sdp);

	/*+ Per-segment lock to mutex F_SOFTLOCK and F_SOFTUNLOCK faults */
static LKINFO_DECL(segdev_lkinfo, "MS:segdev:mutex", 0);

/*
 * int
 * segdev_create(struct seg *seg, void *argsp)
 *	Create a user segment for mapping device memory.
 *
 * Calling/Exit State:
 *	Called with the AS write locked.
 *	Returns with the AS write locked.
 */
int
segdev_create(struct seg *seg, void *argsp)
{
	struct segdev_data *sdp;
	struct segdev_crargs *a = argsp;
	int error;
	extern vnode_t *specfind(dev_t, vtype_t);

	ASSERT(seg->s_as != &kas);

	sdp = kmem_alloc(sizeof(struct segdev_data), KM_SLEEP);
	sdp->mapfunc = a->mapfunc;
	sdp->offset = a->offset;
	sdp->prot = a->prot;
	sdp->maxprot = a->maxprot;
	sdp->vpage = NULL;

	/* Hold associated vnode -- segdev only deals with CHR devices */
	sdp->vp = specfind(a->dev, VCHR);
	ASSERT(sdp->vp != NULL);

	/* Inform the vnode of the new mapping */
	error = VOP_ADDMAP(sdp->vp, sdp->offset, seg->s_as, seg->s_base,
			   seg->s_size, sdp->prot, sdp->maxprot, MAP_SHARED,
			   u.u_lwpp->l_cred);
	if (error != 0) {
		VN_RELE(sdp->vp);
		kmem_free(sdp, sizeof(*sdp));
		return error;
	}

	/*
	 * Inform the HAT of the new mapping, so it may allocate any
	 * necessary structures.  It may also choose to preload translations.
	 */
	if ((error = hat_map(seg, NULL, 0, a->prot, HAT_NOFLAGS)) != 0) {
		(void) VOP_DELMAP(sdp->vp, sdp->offset, seg->s_as, seg->s_base,
			          seg->s_size, sdp->prot, sdp->maxprot,
				  MAP_SHARED, u.u_lwpp->l_cred);
		VN_RELE(sdp->vp);
		kmem_free(sdp, sizeof(*sdp));
		return error;
	}

	LOCK_INIT(&sdp->mutex, VM_SEGDEV_HIER, PLMIN, &segdev_lkinfo,
		  KM_SLEEP);
	SV_INIT(&sdp->softsv);

	seg->s_ops = &segdev_ops;
	seg->s_data = sdp;
	seg->s_as->a_isize += seg->s_size;

	return 0;
}

/*
 * STATIC void
 * segdev_badop(void)
 *	Illegal operation.
 *
 * Calling/Exit State:
 *	Always panics.
 */
STATIC void
segdev_badop(void)
{
	/*
	 *+ A segment operation was invoked which is not supported by the
	 *+ segdev segment driver.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "segdev_badop");
	/*NOTREACHED*/
}

/*
 * STATIC void
 * segdev_nop(void)
 *	Do-nothing operation.
 *
 * Calling/Exit State:
 *	Always returns success w/o doing anything.
 */
STATIC int
segdev_nop(void)
{
	return 0;
}

/*
 * STATIC int
 * segdev_dup(struct seg *pseg, struct seg *cseg)
 *	Called from as_dup to replicate segment specific data structures,
 *	inform filesystems of additional mappings for vnode-backed segments,
 *	and, as an optimization to fork, pre-load copies of translations
 *	currently established in the parent segment.
 *
 * Calling/Exit State:
 *	The parent's address space is read locked on entry to the call and
 *	remains so on return.
 *
 *	The child's address space is not locked on entry to the call since
 *	there can be no active LWPs in it at this point in time.
 *
 *	On success, 0 is returned to the caller and s_data in the child
 *	generic segment stucture points to the newly created segvn_data.
 *	On failure, non-zero is returned and indicates the appropriate
 *	errno.
 */
STATIC int
segdev_dup(struct seg *pseg, struct seg *cseg)
{
	struct segdev_data *psdp = pseg->s_data;
	struct segdev_data *csdp;
	struct segdev_crargs a;
	int error;

	a.mapfunc = psdp->mapfunc;
	a.dev = psdp->vp->v_rdev;
	a.offset = psdp->offset;
	a.prot = psdp->prot;
	a.maxprot = psdp->maxprot;

	if ((error = segdev_create(cseg, &a)) != 0)
		return error;

	csdp = cseg->s_data;
	if (psdp->vpage != NULL) {
		size_t nbytes = seg_pages(pseg) * sizeof(segdev_page_t);

		csdp->vpage = kmem_alloc(nbytes, KM_SLEEP);
		bcopy(psdp->vpage, csdp->vpage, nbytes);
	}

	(void) hat_dup(pseg, cseg, HAT_HIDDEN);

	cseg->s_as->a_isize += cseg->s_size;
	return 0;
}

/*
 * STATIC int
 * segdev_unmap(struct seg *seg, vaddr_t addr, size_t len)
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
STATIC int
segdev_unmap(struct seg *seg, vaddr_t addr, size_t len)
{
	struct segdev_data *sdp = seg->s_data;
	struct segdev_data *nsdp;
	struct seg *nseg;
	uint_t	opages,		/* old segment size in pages */
		npages,		/* new segment size in pages */
		dpages;		/* pages being deleted (unmapped)*/

	vaddr_t nbase;
	size_t nsize;

	/*
	 * Check for bad sizes
	 */
	if (addr < seg->s_base || addr + len > seg->s_base + seg->s_size ||
	    (len & PAGEOFFSET) || (addr & PAGEOFFSET)) {
		/*
		 *+ A request was made to unmap segdev segment addresses
		 *+ which are outside of the segment.  This indicates a
		 *+ kernel software problem.
		 */
		cmn_err(CE_PANIC, "segdev_unmap");
	}

	seg->s_as->a_isize -= len;

	/*
	 * Unload any hardware translations in the range to be taken out.
	 */
	hat_unload(seg, addr, len, HAT_NOFLAGS);

	/* Inform the vnode of the unmapping. */
	ASSERT(sdp->vp != NULL);
	(void)VOP_DELMAP(sdp->vp, sdp->offset, seg->s_as, addr, len, sdp->prot,
		       sdp->maxprot, MAP_SHARED, u.u_lwpp->l_cred);

	/*
	 * Check for entire segment
	 */
	if (addr == seg->s_base && len == seg->s_size) {
		seg_free(seg);
		return 0;
	}

	opages = seg_pages(seg);
	dpages = btop(len);
	npages = opages - dpages;

	/*
	 * Check for beginning of segment
	 */
	if (addr == seg->s_base) {
		if (sdp->vpage != NULL) {
			size_t nbytes;
			segdev_page_t *ovpage;

			ovpage = sdp->vpage;	/* keep pointer to vpage */

			nbytes = npages * sizeof(segdev_page_t);
			sdp->vpage = kmem_alloc(nbytes, KM_SLEEP);
			bcopy(&ovpage[dpages], sdp->vpage, nbytes);

			/* free up old vpage */
			kmem_free(ovpage, opages * sizeof(segdev_page_t));
		}
		sdp->offset += len;

		seg->s_base += len;
		seg->s_size -= len;
		return 0;
	}

	/*
	 * Check for end of segment
	 */
	if (addr + len == seg->s_base + seg->s_size) {
		if (sdp->vpage != NULL) {
			size_t nbytes;
			segdev_page_t *ovpage;

			ovpage = sdp->vpage;	/* keep pointer to vpage */

			nbytes = npages * sizeof(segdev_page_t);
			sdp->vpage = kmem_alloc(nbytes, KM_SLEEP);
			bcopy(ovpage, sdp->vpage, nbytes);

			/* free up old vpage */
			kmem_free(ovpage, opages * sizeof(segdev_page_t));

		}
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

	nseg->s_ops = seg->s_ops;
	nsdp = kmem_alloc(sizeof(struct segdev_data), KM_SLEEP);
	nseg->s_data = nsdp;
	nsdp->prot = sdp->prot;
	nsdp->maxprot = sdp->maxprot;
	nsdp->mapfunc = sdp->mapfunc;
	nsdp->offset = sdp->offset + nseg->s_base - seg->s_base;
	nsdp->vp = sdp->vp;
	VN_HOLD(nsdp->vp);	/* Hold vnode associated with the new seg */

	LOCK_INIT(&nsdp->mutex, VM_SEGDEV_HIER, PLMIN, &segdev_lkinfo,
		  KM_SLEEP);
	SV_INIT(&nsdp->softsv);

	if (sdp->vpage == NULL)
		nsdp->vpage = NULL;
	else {
		/* need to split vpage into two arrays */
		size_t nbytes;
		segdev_page_t *ovpage;

		ovpage = sdp->vpage;	/* keep pointer to vpage */

		npages = seg_pages(seg);	/* seg has shrunk */
		nbytes = npages * sizeof(segdev_page_t);
		sdp->vpage = kmem_alloc(nbytes, KM_SLEEP);

		bcopy(ovpage, sdp->vpage, nbytes);

		npages = seg_pages(nseg);
		nbytes = npages * sizeof(segdev_page_t);
		nsdp->vpage = kmem_alloc(nbytes, KM_SLEEP);

		bcopy(&ovpage[opages - npages], nsdp->vpage, nbytes);

		/* free up old vpage */
		kmem_free(ovpage, opages * sizeof(segdev_page_t));
	}

	return 0;
}

/*
 * STATIC void
 * segdev_free(struct seg *seg)
 *	Free a segment.
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusivley locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 */
STATIC void
segdev_free(struct seg *seg)
{
	struct segdev_data *sdp = seg->s_data;

	VN_RELE(sdp->vp);
	if (sdp->vpage != NULL)
		kmem_free(sdp->vpage, seg_pages(seg) * sizeof(segdev_page_t));

	LOCK_DEINIT(&sdp->mutex);

	kmem_free(sdp, sizeof(*sdp));
}

/*
 * STATIC faultcode_t
 * segdev_fault(struct seg *seg, vaddr_t addr, size_t len, enum fault_type type,
 *	        enum seg_rw rw)
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
 */
STATIC faultcode_t
segdev_fault(struct seg *seg, vaddr_t addr, size_t len, enum fault_type type,
	     enum seg_rw rw)
{
	struct segdev_data *sdp = seg->s_data;
	vaddr_t adr;
	uint_t prot, protchk;
	ppid_t ppid;
	segdev_page_t *vpage;
	uint_t hat_flags;
	pl_t oldpri;

	if (type == F_PROT) {
		/*
		 * Since the seg_dev driver does not implement copy-on-write,
		 * this means that a valid translation is already loaded,
		 * but we got a fault trying to access the device.
		 * Return an error here to prevent going in an endless
		 * loop reloading the same translation...
		 */
		return FC_PROT;
	}

	if (type == F_SOFTUNLOCK) {
		ASSERT(sdp->vpage != NULL);
		vpage = &sdp->vpage[seg_page(seg, addr)];
		oldpri = LOCK_PLMIN(&sdp->mutex);
		for (adr = addr; adr < addr + len; adr += PAGESIZE, vpage++) {
			ASSERT(vpage->dvp_softcnt != 0);
			if (vpage->dvp_softcnt == DVP_SOFTMAX &&
			    SV_BLKD(&sdp->softsv)) {
				SV_BROADCAST(&sdp->softsv, 0);
			}
			if (--vpage->dvp_softcnt == 0)
				hat_unlock(seg, adr);
		}
		UNLOCK_PLMIN(&sdp->mutex, oldpri);
		return 0;
	}

	switch (rw) {
	case S_READ:
		protchk = PROT_READ;
		break;
	case S_WRITE:
		protchk = PROT_WRITE;
		break;
	case S_EXEC:
		protchk = PROT_EXEC;
		break;
	default:
		protchk = PROT_READ | PROT_WRITE | PROT_EXEC;
		break;
	}

	if (type == F_MAXPROT_SOFTLOCK) {
		/* Treat F_MAXPROT_SOFTLOCK just like F_SOFTLOCK */
		type = F_SOFTLOCK;
	}

	if (type == F_SOFTLOCK) {
		oldpri = LOCK_PLMIN(&sdp->mutex);
		/* Make sure there's a vpage array instantiated. */
		if (sdp->vpage == NULL) {
			/*
			 * We need to allocate a vpage array, but since we may
			 * block during the allocation we have to drop the lock.
			 */
			UNLOCK_PLMIN(&sdp->mutex, PLMIN);
			vpage = segdev_vpage_init(seg, sdp);
			(void) LOCK_PLMIN(&sdp->mutex);
			/*
			 * Since we dropped the lock, we must check for a race
			 * against another vpage allocator; if we lost, just
			 * discard the array we allocated.
			 */
			if (sdp->vpage == NULL)
				sdp->vpage = vpage;
			else
				kmem_free(vpage, seg_pages(seg) *
						 sizeof(segdev_page_t));
		}
		vpage = &sdp->vpage[seg_page(seg, addr)];
		/* Increment SOFTLOCK count, blocking if at max. */
		while (vpage->dvp_softcnt == DVP_SOFTMAX) {
			SV_WAIT(&sdp->softsv, PRIMEM, &sdp->mutex);
			(void) LOCK_PLMIN(&sdp->mutex);
		}
		vpage->dvp_softcnt++;
		UNLOCK_PLMIN(&sdp->mutex, oldpri);
		hat_flags = HAT_LOCK;
	} else {
		hat_flags = HAT_NOFLAGS;
		if (sdp->vpage == NULL) {
			prot = sdp->prot;
			if (!(prot & protchk))
				return FC_PROT;
			vpage = NULL;
		} else
			vpage = &sdp->vpage[seg_page(seg, addr)];
	}

	for (adr = addr; adr < addr + len; adr += PAGESIZE) {
		if (vpage != NULL) {
			prot = (vpage++)->dvp_prot;
			if (!(prot & protchk)) {
				/*
				 * We failed the protection check.  If we're
				 * SOFTLOCKing, we have to undo any locks we
				 * already made, since the caller will consider
				 * the whole range failed.
				 */
				if (type == F_SOFTLOCK && adr != addr) {
					oldpri = LOCK_PLMIN(&sdp->mutex);
					while (adr != addr) {
						adr -= PAGESIZE;
						if (--vpage->dvp_softcnt == 0)
							hat_unlock(seg, adr);
					}
					UNLOCK_PLMIN(&sdp->mutex, oldpri);
				}
				return FC_PROT;
			}
		}

		ppid = (ppid_t)(*sdp->mapfunc)(sdp->vp->v_rdev,
				sdp->offset + (adr - seg->s_base), prot);
		if (ppid == NOPAGE)
			return FC_MAKE_ERR(EFAULT);

		hat_devload(seg, adr, ppid, prot, hat_flags);
	}

	return 0;
}

/*
 * STATIC int
 * segdev_setprot(struct seg *seg, vaddr_t addr, size_t len, uint_t prot)
 *	Change the protections on a range of pages in the segment.
 *
 * Calling/Exit State:
 *	Called and exits with the address space exclusively locked.
 *
 *	Returns zero on success, returns a non-zero errno on failure.
 */
STATIC int
segdev_setprot(struct seg *seg, vaddr_t addr, size_t len, uint_t prot)
{
	struct segdev_data *sdp = seg->s_data;
	segdev_page_t *vp, *evp;

	if ((sdp->maxprot & prot) != prot)
		return EACCES;		/* violated maxprot */

	if (addr == seg->s_base && len == seg->s_size && sdp->vpage == NULL) {
		if (sdp->prot == prot)
			return 0;			/* all done */
		sdp->prot = (uchar_t)prot;
	} else {
		if (sdp->vpage == NULL)
			sdp->vpage = segdev_vpage_init(seg, sdp);
		/*
		 * Now go change the needed vpages protections.
		 */
		evp = &sdp->vpage[seg_page(seg, addr + len)];
		for (vp = &sdp->vpage[seg_page(seg, addr)]; vp < evp; vp++)
			vp->dvp_prot = (uchar_t)prot;
	}

#ifdef DEBUG
	/*
	 * ASSERT no pending SOFTLOCKs since AS read lock is held across
	 * F_SOFTLOCK/F_SOFTUNLOCK pair.
	 */
	if (sdp->vpage) {
		evp = &sdp->vpage[seg_page(seg, addr + len)];
		for (vp = &sdp->vpage[seg_page(seg, addr)]; vp < evp; vp++)
			ASSERT(vp->dvp_softcnt == 0);
	}
#endif /* DEBUG */

	hat_chgprot(seg, addr, len, prot, B_TRUE);

	return 0;
}

/*
 * STATIC int
 * segdev_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
 *	Determine that the vpage protection for addr  
 *	is at least equal to prot.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	On success, 0 is returned, indicating that the addr
 *	allow accesses indicated by the specified protection.
 *	Actual protection may be greater.
 *	On failure, EACCES is returned, to indicate that 
 *	the page does not allow the desired access.
 */
STATIC int
segdev_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
{
	struct segdev_data *sdp = seg->s_data;
	segdev_page_t *vp;

	ASSERT((addr & PAGEOFFSET) == 0);
	/*
	 * If segment protections can be used, simply check against them.
	 */
	if (sdp->vpage == NULL)
		return (((sdp->prot & prot) != prot) ? EACCES : 0);

	/*
	 * Have to check down to the vpage level.
	 */
	vp = &sdp->vpage[seg_page(seg, addr)];
	if ((vp->dvp_prot & prot) != prot)
		return EACCES;

	return 0;
}

/*
 * STATIC int
 * segdev_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
 *	Return the protections on pages starting at addr for len.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	This function, which cannot fail, returns the permissions of the
 *	indicated pages in the protv array.
 */
STATIC int
segdev_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
{
 	struct segdev_data *sdp = seg->s_data;
	uint_t pgoff;

	ASSERT((addr & PAGEOFFSET) == 0);

	if (sdp->vpage == NULL) {
 		*protv = sdp->prot;
	} else {
		pgoff = seg_page(seg, addr);
		*protv = sdp->vpage[pgoff].dvp_prot;
	}
	return 0;
}

/*
 * STATIC off_t
 * segdev_getoffset(struct seg *seg, vaddr_t addr)
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
STATIC off_t
segdev_getoffset(struct seg *seg, vaddr_t addr)
{
	struct segdev_data *sdp = seg->s_data;

	return (addr - seg->s_base) + sdp->offset;
}

/*
 * STATIC int
 * segdev_gettype(struct seg *seg, vaddr_t addr)
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
segdev_gettype(struct seg *seg, vaddr_t addr)
{
	return MAP_SHARED;
}

/*
 * STATIC int
 * segdev_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
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
segdev_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
{
	struct segdev_data *sdp = seg->s_data;

	ASSERT(sdp->vp != NULL);
	*vpp = sdp->vp;
	return 0;
}

/*
 * STATIC int
 * segdev_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
 *	Return an indication, in the array, vec, of whether each page
 *	in the given range is "in core".
 *
 * Calling/Exit State:
 *	Called with the AS locked and returns the same.
 *
 * Remarks:
 *	"Pages" for segdev are always "in core", so set all to true.
 */
/*ARGSUSED*/
STATIC int
segdev_incore(struct seg *seg, vaddr_t addr, size_t len, char *vec)
{
	size_t v = 0;

	for (len = (len + PAGEOFFSET) & PAGEMASK; len; len -= PAGESIZE) {
		*vec++ = 1;
		v += PAGESIZE;
	}
	return (int)v;
}

/*
 * STATIC segdev_page_t *
 * segdev_vpage_init(struct seg *seg, struct segdev_data *sdp)
 *	Allocate and initialize a vpage array for the segment.
 *
 * Calling/Exit State:
 *	Called with exclusive access to the segment, either by holding
 *	the AS lock exclusively, or by holding sdp->mutex.
 */
STATIC segdev_page_t *
segdev_vpage_init(struct seg *seg, struct segdev_data *sdp)
{
	segdev_page_t *vpage, *vp;

	/* Allocate an array of per-page structures. */
	vpage = kmem_alloc(seg_pages(seg) * sizeof(segdev_page_t), KM_SLEEP);

	/* Initialize all pages to the current segment-wide protections. */
	for (vp = &vpage[seg_pages(seg)]; vp-- != vpage;) {
		vp->dvp_prot = sdp->prot;
		vp->dvp_softcnt = 0;
	}

	return vpage;
}

/*
 * STATIC void
 * segdev_age(struct seg *, u_int type)
 *	Age the translations for a segdev segment. As an optimization,
 *	aging is limited to the swapout case.
 *
 * Calling/Exit State:
 *	The process owning the AS which owns the argument segment has
 *	been seized.
 *
 *	This function does not block.
 */
STATIC void
segdev_age(struct seg * seg, u_int type)
{
	if (type == AS_SWAP) {
		(void) hat_agerange(seg->s_as, seg->s_base,
				    seg->s_base + seg->s_size, AS_SWAP);
	}
}

/*
 * STATIC int
 * segdev_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
 *	This is a no-op for segdev.
 *
 * Calling/Exit State:
 *	returns ENOMEM.
 */
/*ARGSUSED*/
STATIC int
segdev_memory(struct seg *seg, vaddr_t *basep, uint *lenp)
{
	return ENOMEM;
}
