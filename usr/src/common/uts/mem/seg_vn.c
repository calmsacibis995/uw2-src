/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/seg_vn.c	1.94"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * VM - shared or copy-on-write from a vnode/anonymous memory.
 */

#include <fs/buf.h>
#include <fs/memfs/memfs.h>
#include <fs/vnode.h>
#include <mem/amp.h>
#include <mem/anon.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_kmem.h>
#include <mem/seg_vn.h>
#include <mem/tuneable.h>
#include <mem/vmparam.h>
#include <mem/vpage.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Private seg op routines.
 */
STATIC	int segvn_dup(struct seg *seg, struct seg *newsegp);
STATIC	int segvn_unmap(struct seg *seg, vaddr_t addr, uint_t len);
STATIC	void segvn_free(struct seg *seg);
STATIC	faultcode_t segvn_fault(struct seg *seg, vaddr_t addr, uint_t len,
	                        enum fault_type type, enum seg_rw rw);
STATIC	int segvn_setprot(struct seg *seg, vaddr_t addr, uint_t len, uint_t prot);
STATIC	int segvn_checkprot(struct seg *seg, vaddr_t addr, uint_t prot);
STATIC	int segvn_getprot(struct seg *seg, vaddr_t addr, uint_t *prot);
STATIC	off_t segvn_getoffset(struct seg *seg, vaddr_t addr);
STATIC	int segvn_gettype(struct seg *seg, vaddr_t addr);
STATIC	int segvn_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp);
STATIC	int segvn_kluster(struct seg *seg, vaddr_t addr, int delta);
STATIC	int segvn_sync(struct seg *seg, vaddr_t addr, uint_t len,
		       int attr, uint_t flags);
STATIC	int segvn_incore(struct seg *seg, vaddr_t addr, uint_t len, char *vec);
STATIC	int segvn_lockop(struct seg *seg, vaddr_t addr, uint_t len,
	                 int attr, int op);
STATIC	void segvn_childload(struct seg *pseg, struct seg *cseg);
STATIC	void segvn_swr_scan(struct seg *seg, vnode_t **vp, off_t *offset,
			    int *idx);
STATIC	int segvn_memory(struct seg *seg, vaddr_t *basep, u_int *lenp);

struct seg_ops segvn_ops = {
	segvn_unmap,
	segvn_free,
	segvn_fault,
	segvn_setprot,
	segvn_checkprot,
	segvn_kluster,
	segvn_sync,
	segvn_incore,
	segvn_lockop,
	segvn_dup,
	segvn_childload,
	segvn_getprot,
	segvn_getoffset,
	segvn_gettype,
	segvn_getvp,
	(void (*)())NULL,			/* age: MUST BE NULL!!! */
	(boolean_t (*)())NULL,			/* lazy_shootdown */
	segvn_memory
};

/*
 * Common zfod structure, provided as a shorthand for others to use.
 */
STATIC
struct	segvn_crargs zfod_segvn_crargs = {
	(vnode_t *)NULL,
	0,
	(struct cred *)NULL,
	MAP_PRIVATE,
	PROT_ALL,
	PROT_ALL,
};

void *zfod_argsp = &zfod_segvn_crargs;	/* user zfod argsp */

STATIC	boolean_t segvn_concat(struct seg *seg1, struct seg *seg2);
STATIC	void segvn_vpage_info_alloc(struct seg *seg);
STATIC	int segvn_swap_resv(struct seg *seg);
STATIC	void segvn_private_dup(struct seg *pseg, struct seg *cseg,
			       anon_t **capp);

/*
 * array sizes in segvn_swap_resv [holding off_t(s)]
 */
#define	SEGVN_SWRV_FIXED	32		/* fixed automatic size */
#define	SEGVN_SWRV_ALLOC	(PAGESIZE / sizeof(off_t))
						/* first kmem_alloc size */

/*
 * STATIC boolean_t
 * SW_RESERVED(struct segvn_data *svp)
 *	Returns B_TRUE if M_SWAP has been reserved for the segment given
 *	by ``svp''.
 *
 * Calling/Exit State:
 *	The SEGLOCK is held in READer, WRITEr, or INTENT state.
 */
#define SW_RESERVED(svp)	((svp)->svd_swresv > 0 || \
				 ((svp)->svd_vp->v_flag & VSWAPBACK))

/*
 * STATIC boolean_t
 * MEM_LOCKING(struct segvn_data *svp)
 *	Returns B_TRUE if a MC_LOCK operation is in progress for the segment
 *	given by ``svp''.
 *
 * Calling/Exit State:
 *	The AS lock is held in READer or WRITEr mode.
 */
#define MEM_LOCKING(svp)	((svp)->svd_flags & SEGVN_MLCKIP)

/*
 * STATIC mresvtyp_t
 * M_SWAPBACK(struct segvn_data *svp)
 *	Returns the appropriate mresvtyp_t for softlocking a page when an
 *	M_SWAP reservation has already been obtained for the segment.
 *
 * Calling/Exit State:
 *	The AS lock is held in READer or WRITEr state.
 *
 * Description:
 *	If a segvn_lockop(MC_LOCK) operation is in progress, then an
 *	M_REAL_USER reservation is needed. Otherwise, a true SOFTLOCK is in
 *	progress, and an M_REAL reservation is needed.
 */
#define M_SWAPBACK(svp)		(MEM_LOCKING(svp) ? M_REAL_USER : M_REAL)

/*
 * STATIC mresvtyp_t
 * M_NSWAPBACK(struct segvn_data *svp)
 *	Returns the appropriate mresvtyp_t for softlocking a page when an
 *	M_SWAP reservation has not been obtained for the segment.
 *
 * Calling/Exit State:
 *	The AS lock is held in READer or WRITEr state.
 *
 * Description:
 *	If a segvn_lockop(MC_LOCK) operation is in progress, then an
 *	M_BOTH_USER reservation is needed. Otherwise, a true SOFTLOCK is in
 *	progress, and an M_BOTH reservation is needed.
 */
#define M_NSWAPBACK(svp) 	(MEM_LOCKING(svp) ? M_BOTH_USER : M_BOTH)

/*
 * STATIC mresvtyp_t
 * M_SOFTLOCK(struct segvn_data *svp)
 *	Returns the appropriate mresvtyp_t for softlocking a page.
 *
 * Calling/Exit State:
 *	The SEGLOCK is held in READer, WRITEr, or INTENT state, and the
 *	AS lock is held in READer or WRITEr state.
 *
 * Description:
 *	This macro is used to determine the type of reservation to take
 *	when obtaining or releasing pvn_memresv(s) for softlock pages.
 *	The reservation type needed depends (1) upon whether M_SWAP has
 *	already been acquired for the entire segment, and (2) whether
 *	the softlock operation is being performed on behalf of a
 *	segvn_lockop() operation.
 */
#define M_SOFTLOCK(svp)	(SW_RESERVED(svp) ? M_SWAPBACK(svp) : M_NSWAPBACK(svp))

/*
 * STATIC mresvtyp_t
 * M_MEMLOCK(struct segvn_data *svp)
 *	Returns the appropriate mresvtyp_t for memory locking a page (on
 *	behalf of a segvn_lockop() operation).
 *
 * Calling/Exit State:
 *	AS lock is held in READer or WRITEr state.
 *
 * Description:
 *	This macro is used to determine the type of reservation to take
 *	when obtaining or releasing pvn_memresv(s) for memory locked pages.
 *	The reservation type needed depends upon whether M_SWAP has
 *	already been acquired for the entire segment.
 */
#define M_MEMLOCK(svp)	(SW_RESERVED(svp) ? M_REAL_USER : M_BOTH_USER)

/*
 * STATIC mresvtyp_t
 * M_SEGVN((struct segvn_data *svp)
 *	Returns M_REAL if M_SWAP has been reserved for the segment
 *	specified by svp, and M_BOTH otherwise.
 *
 * Calling/Exit State:
 *	The SEGLOCK is held in READer, WRITEr, or INTENT state.
 *
 * Description:
 *	This macro is used to determine the type of reservation to take
 *	when obtaining mem_resv(s) for pages internally locked down by the
 *	segment driver. A locked page requires an M_BOTH reservation.
 *	However, if M_SWAP has already been reserved for the entire
 *	segment, then an incremental reservation of M_REAL will suffice.
 */
#define M_SEGVN(svp)	(SW_RESERVED(svp) ? M_REAL : M_BOTH)

/*
 * STATIC boolean_t
 * XLAT_LOADED(struct seg *seg, vaddr_t addr)
 *	Determine if a translation is loaded for the specified address
 *	in the specified segment.
 *
 * Calling/Exit State:
 *	Returns true if the translation for the specified virtual
 *	page in the specified segment is loaded, and B_FALSE otherwise.
 */
#define XLAT_LOADED(seg, addr)	\
		(hat_xlat_info((seg)->s_as, (addr)) & HAT_XLAT_EXISTS)

/*
 *+ Per-segment lock for segvn. Protects all state variables in the segment.
 */
LKINFO_DECL(segvn_seglockinfo, "MS:segvn:svd_seglock", 0);

/*
 * void
 * segvn_kmadv(void)
 *	Call kmem_advise() for segvn data structures.
 *
 * Calling/Exit State:
 *	Called at sysinit time while still single threaded.
 */
void
segvn_kmadv(void)
{
	kmem_advise(sizeof(struct segvn_data));
	kmem_advise(sizeof(struct anon_map));
}

/*
 * int
 * segvn_create(struct seg *seg, const void * const argsp)
 *	Public function to instantiate new segment in an AS
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusively locked before calling this
 *	function; the AS is returned locked. This is required because
 *	nominatively a new portion of address space is being added to
 *      the AS and in many cases, will be concatenated with previously
 *	existing segments of ``like type'' (see below).
 *
 *	No LOCKs are held on entry to this function. It returns the same
 *	way. This function may block.
 *
 *	On success, 0 is returned and the segment structure opaque data field
 *	(seg_data) points to the newly created segment. On failure, the
 *	return value is non-zero and indicates an appropriate errno.
 *
 */
int
segvn_create(struct seg *seg, const void * const argsp)
{
	const struct segvn_crargs *a = (const struct segvn_crargs *)argsp;
	struct segvn_data *svp;
	int error;
	struct segvn_data proto_sv;
	vnode_t *dzvp = NULL;
	struct seg *pseg, *nseg;

	ASSERT(seg->s_as != &kas);
	ASSERT(getpl() == PLBASE);
	ASSERT((seg->s_base & PAGEOFFSET) == 0);
	ASSERT((seg->s_size & PAGEOFFSET) == 0);

	if (a->type != MAP_PRIVATE && a->type != MAP_SHARED) {
		/*
		 *+ An attempt was made to instantiate a new segvn
		 *+ segment using an illegal/unrecognized segment
		 *+ type. This is an unrecoverable situation and
		 *+ probably indicates a more profound software or
		 *+ hardware problem elsewhere in the system.
		 */
		cmn_err(CE_PANIC, "segvn_create: illegal segment type");
	}

	/*
	 * If segment needs a swap reservation and/or an anon_map, get
	 * them now.
	 */
	proto_sv.svd_swresv = 0;
	proto_sv.svd_amp = NULL;
	proto_sv.svd_vp = a->vp;
	if (a->type == MAP_SHARED) {
		if (a->vp == NULL) {
			/*
			 * This is a SHARED /dev/zero mapping. We need to
			 * create a memfs vp. If this fails, it must be
			 * because we didn't get the swap reservation.
			 */
			dzvp = memfs_create_unnamed(seg->s_size,
						    MEMFS_FIXEDSIZE);
			if (dzvp == NULL)
				return (ENOMEM);
			proto_sv.svd_vp = dzvp;
		}
	} else { /* a->type == MAP_PRIVATE */
		/*
		 * MAP_PRIVATE segment. Reserve swap now if segment is
		 * writable, or if there is zfod potential. However,
		 * let the anon_map be instantiated lazily.
		 */
		if (a->prot & PROT_WRITE || a->vp == NULL) {
			proto_sv.svd_swresv = seg->s_size;
			if (!mem_resv(btop(proto_sv.svd_swresv), M_SWAP))
				return (ENOMEM);
		}
	}

	if (a->cred)
		proto_sv.svd_cred = a->cred;
	else
		proto_sv.svd_cred = CRED();

	crhold(proto_sv.svd_cred);

	/*
	 * If this is a vnode backed segment, take care of business.
	 */

	if (proto_sv.svd_vp) {

		/* Inform the vnode of the new mapping */

		error = VOP_ADDMAP(proto_sv.svd_vp, a->offset & PAGEMASK,
				   seg->s_as, seg->s_base, seg->s_size,
				   a->prot, a->maxprot, a->type,
				   proto_sv.svd_cred);
		if (error)
			goto error_backout;
	}

	/*
	 * Inform the HAT of the new mapping, so it may allocate any
	 * necessary structures.  It may also choose to preload translations.
	 * We disallow writeable translations in all cases, even
	 * for shared mappings, to force a protection fault, so
	 * filesystems can allocate blocks for holes if they need to.
	 *
	 * We must setup the ops vector so that as_ageswap can age the
	 * translations for the address range of this segment. If we didn't
	 * do this, the pages would in effect be temporarily locked into real
	 * memory, without the required reservation. Note that such aging is
	 * required to avoid memory resource deadlock, even if this
	 * proto-segment never becomes a segment (i.e. it get concatenated
	 * with adjacent segments).
	 */
	seg->s_ops = &segvn_ops;
	error = hat_map(seg, proto_sv.svd_vp, a->offset & PAGEMASK,
			a->prot & ~PROT_WRITE, HAT_PRELOAD);
	if (error) {
		if (proto_sv.svd_vp) {
			(void) VOP_DELMAP(proto_sv.svd_vp,
					  a->offset & PAGEMASK, seg->s_as,
					  seg->s_base, seg->s_size,
					  a->prot, a->maxprot, a->type,
					  proto_sv.svd_cred);
		}
		goto error_backout;
	}

	/*
	 * NOTE at this point we may have loaded valid translations
	 * for the new segment. On some machines this will make
	 * them visible immediately to other LWPs running in usermode.
	 * Access by user programs to the associated pages is probably a
	 * user level bug (because mapping has not yet completed).
	 * Nonetheless, it is no problem for us, because access to the
	 * segment data structures is serialized by the AS lock.
	 */

	/*
	 * Finish initializing the prototype segvn_data structure for possible
	 * concatenation purposes.
	 */
	proto_sv.svd_maxprot = a->maxprot;
	proto_sv.svd_prot = a->prot;
	proto_sv.svd_type = a->type;
	proto_sv.svd_flags = SEGVN_PROTO;
	proto_sv.svd_offset = a->offset;
	proto_sv.svd_lock = NULL;
	proto_sv.svd_info = NULL;
	seg->s_data = &proto_sv;
	seg->s_as->a_isize += seg->s_size;

	/*
	 * First, try to concatenate with the following segment.
	 * Note that there will be two segments in the address space
	 * if the first test passes. If the first concatenation
	 * succeeds, only one segment might be left in the address
	 * space.
	 *
	 * If segvn_concat can concatenate seg with nseg, it will
	 * expand nseg and free seg. This is because it never chooses
	 * a SEGVN_PROTO segment.
	 */
	nseg = seg->s_next;
	if (seg->s_base + seg->s_size == nseg->s_base && IS_SEGVN(nseg) &&
	    segvn_concat(seg, nseg)) {
		/* success! now try to concatenate with previous seg */
		pseg = nseg->s_prev;
		if (pseg->s_base + pseg->s_size == nseg->s_base &&
		    IS_SEGVN(pseg))
			(void) segvn_concat(pseg, nseg);
		return (0);
	}
	/* failed, so try to concatenate with the previous seg */
	pseg = seg->s_prev;
	if (pseg->s_base + pseg->s_size == seg->s_base && IS_SEGVN(pseg) &&
	    segvn_concat(pseg, seg)) {
		return (0);
	}

	/*
	 * Concatenation optimization failed. We are now committed to
	 * actually creating a segment.
	 */

	if (a->vp != NULL) {
		VN_HOLD(a->vp);
	}
	svp = kmem_alloc(sizeof(struct segvn_data), KM_SLEEP);
	seg->s_data = svp;

	/*
	 * Initialize per-segment locking goo.
	 */

	*svp = proto_sv; /* structure copy */
	LOCK_INIT(&svp->svd_seglock, VM_SEGVN_HIER, VM_SEGVN_IPL,
	          &segvn_seglockinfo, KM_SLEEP);
	SV_INIT(&svp->svd_segsv);
	SV_INIT(&svp->svd_vpagesv);
	svp->svd_lckflag = 0;
	svp->svd_anonlck = 0;
	svp->svd_lckcnt = 0;
	svp->svd_flags = 0;
	svp->svd_anon_index = 0;
	return (0);

error_backout:
	if (proto_sv.svd_swresv)
		mem_unresv(btop(proto_sv.svd_swresv), M_SWAP);
	else if (dzvp)
		VN_RELE(dzvp);
	crfree(proto_sv.svd_cred);
	return (error);
}

/*
 * STATIC boolean_t
 * segvn_concat(struct seg *seg1, struct seg *seg2)
 * 	Concatenate two segments, if possible.
 *
 * Calling/Exit State:
 *	Only called from segvn_create with AS write locked. The
 *	argument segments are contiguous in the virtual address space
 *	(seg2 immediately follows seg1).
 *
 *	Returns B_TRUE on success with the two supplied segments merged
 *	into one. All resources allocated for the other segment will have
 *	been freed (including the segment itself). Returns B_FALSE on
 *	failure and both segments are unaffected.
 */
STATIC boolean_t
segvn_concat(struct seg *seg1, struct seg *seg2)
{
	struct segvn_data *svp1 = (struct segvn_data *)seg1->s_data;
	struct segvn_data *svp2 = (struct segvn_data *)seg2->s_data;
	struct segvn_data *chosen_svp, *victim_svp, *svp;
	struct anon_map *amp1, *amp2, *amp;
	uint_t osize1, osize2, chosen_size, victim_size, chosen_index;
	uint_t victim_index, index, asize;
	struct seg *chosen_seg, *victim_seg;
	vpage_info_t *vpinfop, *vpip, *evpip;
	vpage_info_t *cvpinfop, *vvpinfop; /* for chosen/victim segs */
	anon_t **aa;

	/*
	 * One of the two segments may not have yet been born.
	 * In that case, seg->s_data will be allocated in automatic
	 * storage, and the SEGVN_PROTO flag will be set in svp->svd_flags.
	 * Thus, both segments have some sort of segvn_data allocated.
	 */
	ASSERT(svp1 && svp2);

	/*
	 * Compare vp, offset, maxprot, type, and cred for compatibility.
	 */
	if (svp1->svd_vp != svp2->svd_vp ||
	    svp1->svd_maxprot != svp2->svd_maxprot ||
	    svp1->svd_type != svp2->svd_type ||
	    svp1->svd_cred != svp2->svd_cred)
		return (B_FALSE);
	/* vp == NULL implies zfod, offset doesn't matter */
	if (svp1->svd_vp != NULL &&
	    svp1->svd_offset + seg1->s_size != svp2->svd_offset)
		return (B_FALSE);

	/*
	 * Make sure the swap space reservations match (either both have or
	 * don't have a reservation). If they don't match, then we would need
	 * to reserve some more M_SWAP to merge these segments. If we just
	 * went ahead with the merge and (and reserved swap), the result
	 * would be that we would reserve swap space for all text segments, a
	 * huge waste of swap resource.
	 */
	if ((svp1->svd_swresv != 0) != (svp2->svd_swresv != 0))
		return (B_FALSE);

	/*
	 * From this point on, we are committed to the merge.
	 */

	/*
	 * Merge anon maps if either segment has one.
	 */
	osize1 = seg_pages(seg1);
	osize2 = seg_pages(seg2);
	amp1 = svp1->svd_amp;
	amp2 = svp2->svd_amp;
	if (amp1 || amp2) {
		/*
		 * Only MAP_PRIVATE segments have anon_maps.
		 */
		ASSERT(svp1->svd_type == MAP_PRIVATE);

		/*
		 * If the two segments share the same anon_map, then the
		 * anon_maps are already merged.
		 *
		 * Then check if seg1 has enough slop to absorb seg2's
		 * anon requirement, or vica versa. This is an important
		 * optimization, as it allows stack auto-grow to proceed
		 * without constantly reallocating the anon_maps.
		 */
		if (amp1 == amp2) {
			chosen_seg = seg1;
			victim_seg = seg2;
			/*
			 * Protect the victim's portion of the anon_map from
			 * the ravages of segvn_free.
			 */
			--amp2->am_refcnt;
			svp2->svd_amp = NULL;
		} else if (amp2 && svp2->svd_anon_index >= osize1) {
			/*
			 * Good, seg1 fits in seg2's anon map. seg2 will
			 * be chosen to live, seg1 will die. This is a
			 * good optimization for machines which grow the
			 * stack from high to low addresses.
			 */
			chosen_seg = seg2;
			victim_seg = seg1;
			svp2->svd_anon_index -= osize1;
			if (amp1) {
				/*
				 * Copy the anon_map from the victim to the
				 * chosen one.
				 */
				bcopy((amp1->am_anon + svp1->svd_anon_index),
				      (amp2->am_anon + svp2->svd_anon_index),
				      osize1 * sizeof(anon_t *));
				/*
				  Clean the victim's anon_map.
				 */
				bzero((amp1->am_anon + svp1->svd_anon_index),
				      osize1 * sizeof(anon_t *));
			} else {
				bzero((amp2->am_anon + svp2->svd_anon_index),
				      osize1 * sizeof(anon_t *));
			}
		} else if (amp1 && btop(amp1->am_size) -
				   svp1->svd_anon_index >= osize1 + osize2) {
			/*
			 * Also good, seg2 fits in seg1's anon map.  seg1 will
			 * be chosen to live, seg2 will die. This is a good
			 * optimization for machines which grow the stack
			 * from low to high addresses.
			 */
			chosen_seg = seg1;
			victim_seg = seg2;
			if (amp2) {
				/*
				 * Copy the anon_map from the victim to the
				 * chosen one.
				 */
				bcopy((amp2->am_anon + svp2->svd_anon_index),
				      (amp1->am_anon +
						svp1->svd_anon_index + osize1),
				      osize2 * sizeof(anon_t *));
				/*
				 * Clean the victim's anon_map.
				 */
				bzero((amp2->am_anon + svp2->svd_anon_index),
				      osize2 * sizeof(anon_t *));
			} else {
				bzero((amp1->am_anon +
						svp1->svd_anon_index + osize1),
				      osize2 * sizeof(anon_t *));
			}
		} else {
			/*
			 * Well, we lose. Neither anon map can absorb the
			 * other segment. So, allocate a new, larger anon map,
			 * with enough extra space (slop) to cover possible
			 * future absorptions.
			 */
			if (amp2) {
				/*
				 * Place the slop toward the lower order
				 * addresses.  This is a good optimization
				 * for machines which grow the stack from
				 * high to low addresses.
				 */
				chosen_seg = seg2;
				victim_seg = seg1;
				amp = amp2;
				svp = svp2;
				index = btop(ANON_SLOP);
			} else {
				/*
				 * Place the slop toward the higher order
				 * addresses.  This is a good optimization
				 * for machines which grow the stack from
				 * low to high addresses.
				 */
				ASSERT(amp1);
				chosen_seg = seg1;
				victim_seg = seg2;
				amp = amp1;
				svp = svp1;
				index = 0;
			}
			asize = seg1->s_size + seg2->s_size + ANON_SLOP;
			/*
			 * We will need an entire anon_map structure if
			 * the anon_map of the chosen one is shared with
			 * other segment(s). Note that the other segment(s)
			 * also live in our AS, for which we hold the write
			 * lock. Therefore, am_refcnt is stabilized.
			 *
			 * Otherwise, a new anon_array will do.
			 */
			if (amp->am_refcnt > 1) {
				svp->svd_amp = amp_alloc(asize);
				aa = svp->svd_amp->am_anon;
			} else {
				aa = kmem_zalloc(btop(asize) * sizeof(anon_t *),
						 KM_SLEEP);
			}
			/*
			 * Copy in or zero out the sections of the
			 * anon_maps for each of the two segments.
			 */
			if (amp1) {
				bcopy((amp1->am_anon + svp1->svd_anon_index),
				      (aa + index), osize1 * sizeof(anon_t *));
				/*
				 * Clean the anon_map.
				 */
				bzero((amp1->am_anon + svp1->svd_anon_index),
				      osize1 * sizeof(anon_t *));
			} else {
				bzero((aa + index), osize1 * sizeof(anon_t *));
			}
			if (amp2) {
				bcopy((amp2->am_anon + svp2->svd_anon_index),
				      (aa + osize1 + index),
				      osize2 * sizeof(anon_t *));
				/*
				 * Clean the anon_map.
				 */
				bzero((amp2->am_anon + svp2->svd_anon_index),
				      osize2 * sizeof(anon_t *));
			} else {
				bzero((aa + osize1 + index),
				      osize2 * sizeof(anon_t *));
			}
			if (amp->am_refcnt > 1) {
				/*
				 * Just void our claim on the old anon_map.
				 * At least one other segment still uses it.
				 */
				--amp->am_refcnt;
			} else {
				/*
				 * Replace the old anon array of the chosen
				 * segment with the new one.
				 */
				kmem_free(amp->am_anon, btop(amp->am_size) *
					  sizeof(anon_t *));
				amp->am_anon = aa;
				amp->am_size = asize;
			}
			svp->svd_anon_index = index;
			/*
			 * The victim's anon_map will be disposed of by
			 * segvn_free().
			 */
		}
	} /* amp1 || amp2 */ else {
		/*
		 * If one of the segments is a prototype, it must be chosen
		 * for destruction.
		 */
		if (svp2->svd_flags & SEGVN_PROTO) {
			chosen_seg = seg1;
			victim_seg = seg2;
		} else {
			chosen_seg = seg2;
			victim_seg = seg1;
		}
	}

	/*
	 * Beginning of non-swappable (NO-SWAPOUT) Region.
	 *
	 *	If we blocked while we were transferring the address range from
	 *	the victim to the chosen one, we could be swapped, in which
	 *	case as_ageswap() might observe an inconsistent segment
	 *	chain for the address space.
	 */

	/*
	 * The chosen will live and the victim will die!
	 */

	/*
	 * We need to disable swapout.
	 */
	DISABLE_PRMPT();
	chosen_svp = (struct segvn_data *)chosen_seg->s_data;
	victim_svp = (struct segvn_data *)victim_seg->s_data;
	ASSERT(!(chosen_svp->svd_flags & SEGVN_PROTO));
	if (chosen_seg == seg1) {
		chosen_size = osize1;
		chosen_index = 0;
		victim_size = osize2;
		victim_index = osize1;
	} else {
		chosen_size = osize2;
		chosen_index = osize1;
		victim_size = osize1;
		victim_index = 0;
		chosen_seg->s_base = victim_seg->s_base;
		chosen_svp->svd_offset = victim_svp->svd_offset;
	}
	chosen_seg->s_size += victim_seg->s_size;
	chosen_svp->svd_swresv += victim_svp->svd_swresv;

	/*
	 * We NULL out the ops vector in the victim segment so that
	 * as_ageswap (running in the swapper) will ignore it.
	 */
	victim_seg->s_ops = (struct seg_ops *)NULL;

	/*
	 * End of non-swappable (NO_SWAPOUT) Region.
	 */
	ENABLE_PRMPT();

	/*
	 * We need to merge vpage_info arrays if they exist in either of
	 * the old segments.  Also, if segment level protections mismatch,
	 * then we need to create a vpage_info array, and go to page
	 * granular protections.
	 */
	if (svp1->svd_info != NULL || svp2->svd_info != NULL ||
	    svp1->svd_prot != svp2->svd_prot) {
		/*
		 * get a new array
		 */
		cvpinfop = chosen_svp->svd_info;
		vvpinfop = victim_svp->svd_info;
		chosen_svp->svd_info = NULL;
		segvn_vpage_info_alloc(chosen_seg);
		vpinfop = chosen_svp->svd_info;

		/*
		 * Copy vpage_info from the chosen segment's old array.
		 */
		if (cvpinfop) {
			bcopy(cvpinfop, (&vpinfop[chosen_index]),
			      sizeof(vpage_info_t) * chosen_size);
			kmem_free(cvpinfop, sizeof(vpage_info_t) * chosen_size);
		}

		/*
		 * Copy vpage_info from the victim segment's array.
		 * segvn_free() will free the old array.
		 */
		if (vvpinfop) {
			bcopy(vvpinfop, (&vpinfop[victim_index]),
			      sizeof(vpage_info_t) * victim_size);
		}

		/*
		 * See if we need to establish page granular protections
		 * for the chosen segment.
		 *
		 * This will occur if the chosen segment didn't have page
		 * granular protections, and either the victim did, or the
		 * protections do not match.
		 */
		if (!(chosen_svp->svd_flags & SEGVN_PGPROT) &&
		    ((victim_svp->svd_flags & SEGVN_PGPROT) ||
		    chosen_svp->svd_prot != victim_svp->svd_prot)) {
			vpip = vpinfop + chosen_index;
			evpip = vpip + chosen_size;
			while (vpip < evpip) {
				vpip->vpi_flags |= chosen_svp->svd_prot;
				++vpip;
			}
			chosen_svp->svd_flags |= SEGVN_PGPROT;
			chosen_svp->svd_prot = 0;
		}

		/*
		 * See if we need to establish page granular protections
		 * for the victim part of the segment.
		 *
		 * This will occur if the victim didn't have page granular
		 * protections, and the chosen one now does.
		 */
		if (!(victim_svp->svd_flags & SEGVN_PGPROT) &&
		    (chosen_svp->svd_flags & SEGVN_PGPROT)) {
			vpip = vpinfop + victim_index;
			evpip = vpip + victim_size;
			while (vpip < evpip) {
				vpip->vpi_flags |= victim_svp->svd_prot;
				++vpip;
			}
		}

		/*
		 * Make sure we remember if the victim had memory locked
		 * pages.
		 */
		chosen_svp->svd_flags |= (victim_svp->svd_flags & SEGVN_MEMLCK);
	}

	/*
	 * If the chosen segment had a page granular locking array, then just
	 * discard it. We can safely do this because:
	 *
	 *	1) We hold the AS WRITE lock, so that no faults are in
	 *	   progress and no softlocks are held, and
	 *
	 *	2) The array is just an optimization. A new array will be
	 *	   allocated if and when required.
	 */
	ASSERT(chosen_svp->svd_lckflag == 0);
	if (chosen_svp->svd_lock) {
		kmem_free(chosen_svp->svd_lock,
			  chosen_size * sizeof(vpage_lock_t));
		chosen_svp->svd_lock = NULL;
	}

	/*
	 * Protect victim's swap reservation from the ravages of segvn_free.
	 */
	victim_svp->svd_swresv = 0;

	/*
	 * Beginning of non-swappable (NO-SWAPOUT)  Region.
	 *
	 *	If we blocked after restoring the victim's s_ops vector, we
	 *	could be swapped, in which case as_ageswap() would observe
	 *	an as_chain with two segments claiming the same address
	 *	range.
	 */
	DISABLE_PRMPT();
	victim_seg->s_ops = &segvn_ops;
	seg_free(victim_seg);

	/*
	 * End of non-swappable (NO-SWAPOUT) Region.
	 */
	ENABLE_PRMPT();
	return (B_TRUE);
}

/*
 * STATIC int
 * segvn_dup(struct seg pseg, struct seg cseg)
 *	Called from as_dup to begin the segment driver specific portion
 *	of a fork. The segvn_data is copied here, and an M_SWAP reservation
 *	is obtained for the child.
 *
 * Calling/Exit State:
 *	The parent's address space is read locked on entry to the call and
 *	remains so on return. The child's address space is not visible to
 *	the swapper.
 *
 *	The child's address space is not locked on entry to the call since
 *	there can be no active LWPs in it at this point in time. The
 *	segment is not locked (read or write) for the same reason.
 *
 * Description:
 * 	Note that the AS lock is held in reader mode, effectively stabilizing
 * 	the protections and memory locked state of each virtual page.
 *
 *	Because the child's address space is not visible to the swapper, we
 *	cannot load translations (e.g. via hat_dup()). Instead,
 *	segvn_childload() calls hat_dup(). Also, we cannot dup the anon_map
 *	here - because once we drop the segment lock, a COW fault can
 *	change the parent's anon_map *and* install a read-only translation
 *	in the parent's address space. The latter can occur via either
 *	F_MAXPROT_SOFTLOCK or by a subsequent segvn_dup. Such a translation
 *	would be mistakenly copied to the child, leaving the child with an
 *	inconsistent anon_amp.
 */
STATIC int
segvn_dup(struct seg *pseg, struct seg *cseg)
{
        struct segvn_data *psvp = (struct segvn_data *)pseg->s_data;
        struct segvn_data *csvp;
	uint_t npages = seg_pages(pseg);
	vpage_info_t *pvpinfop, *cvpinfop, *ecvpinfop;
	size_t swresv = 0;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

        /*
	 * First, get a swap resrvation if we need one.
	 *
	 * We do this first, because obtaining the swap reservation is the
	 * only part of the dup which can fail.
	 */
	if (psvp->svd_type == MAP_PRIVATE) {
		/*
		 * We need to get a swap reservation for the child if we
		 * are going to need one. However, we do not need to hold
		 * the parent's segment lock in order to test psvp->svd_swresv
		 * but can instead make use of STORE-ORDERING of the
		 * underlying hardware because:
		 *
		 * 1) Our caller holds an address space reader lock,
		 *    prohibiting the psvp->svd_swresv from changing for
		 *    any reason other than F_MAXPROT_SOFTLOCK fault.
		 *
		 * 2) An F_MAXPROT_SOFTLOCK can cause the psvp->svd_swresv
		 *    to transition from 0 to non-zero, but never in the
		 *    other direction.
		 *
		 * 3) We must guarantee: if an F_MAXPROT_SOFTLOCK has 
		 *    obtained a swap reservation and then COWed a page,
		 *    and the user in this LWP has observed that page,
		 *    then we will be able to dup the anon_map. We rely
		 *    upon the STORE-ORDERING property to guarantee that
		 *    we will see the swap reservation in that case.
		 *
		 * Also note that if the parent does not have a swap
		 * reservation at this time, it might obtain a reservation,
		 * together with a anon_map, before this LWP reaches
		 * segvn_childload(). Should that eventuality arise,
		 * segvn_childload() will attempt to obtain the swap
		 * resrvation. But, should it fail, the penalty will be that
		 * the hat_dup() optimization will have to be skipped
		 * [because the pages created by COW might potentially have
		 * read-only translations installed - e.g. by
		 * F_MAXPROT_SOFTLOCK or by a concurrent fork].
		 */
		if (psvp->svd_swresv != 0) {
			if (!mem_resv(npages, M_SWAP))
				return (ENOMEM);
			swresv = pseg->s_size;
		}
	} else {
		/*
		 * MAP_SHARED case.
		 */
		ASSERT(psvp->svd_amp == NULL);
		ASSERT(psvp->svd_vp != NULL);
	}

	/*
	 * Now, allocate the child's segvn_data structure, and
	 * initialize it.
	 */
	csvp = kmem_alloc(sizeof(struct segvn_data), KM_SLEEP);

	/*
	 * Now finish up the initialization of the segvn_data structure,
	 * and miscellaneous other information.
	 */
	LOCK_INIT(&csvp->svd_seglock, VM_SEGVN_HIER, VM_SEGVN_IPL,
		  &segvn_seglockinfo, KM_SLEEP);
	SV_INIT(&csvp->svd_segsv);
	SV_INIT(&csvp->svd_vpagesv);
	csvp->svd_lckflag = 0;
	csvp->svd_anonlck = 0;
	csvp->svd_lckcnt = 0;
	if ((csvp->svd_vp = psvp->svd_vp) != NULL) {
		VN_HOLD(psvp->svd_vp);
	}

	csvp->svd_offset = psvp->svd_offset;
	csvp->svd_prot = psvp->svd_prot;
	csvp->svd_maxprot = psvp->svd_maxprot;
	csvp->svd_flags = psvp->svd_flags & SEGVN_PGPROT;
	csvp->svd_type = psvp->svd_type;
	csvp->svd_cred = psvp->svd_cred;
	crhold(csvp->svd_cred);
	csvp->svd_amp = NULL;
	csvp->svd_anon_index = 0;
	csvp->svd_swresv = swresv;

	cseg->s_ops = &segvn_ops;
	cseg->s_data = csvp;
	cseg->s_as->a_isize += cseg->s_size;

	/*
	 * If the parent segment has page granular protections set, then
	 * the child must inherit these.
	 *
	 * Note that the AS read lock is held, stabilizing the protections
	 * of the parent.
	 */

	if (psvp->svd_flags & SEGVN_PGPROT) {

		/*
		 * Make a vpage_info array to hold the protections for the
		 * child.
		 */
		csvp->svd_info = NULL;
		segvn_vpage_info_alloc(cseg);

		/*
		 * copy over protections
		 */
		pvpinfop = psvp->svd_info;
		cvpinfop = csvp->svd_info;
		ecvpinfop = cvpinfop + npages;
		while (cvpinfop < ecvpinfop)
			(cvpinfop++)->vpi_flags =
				(pvpinfop++)->vpi_flags & PROT_ALL;
	} else {
		csvp->svd_info = NULL;
	}

	/*
	 * Child is not born with a vpage_lock array.
	 * The child will lazily bring it into existence upon
	 * escalated lock contention.
	 */
	csvp->svd_lock = NULL;

	/*
	 * Let the vnode count up this mapping.
	 */
	if (csvp->svd_vp) {
#ifdef DEBUG
		int error;

		error =
#else
		(void)
#endif
		VOP_ADDMAP(csvp->svd_vp, csvp->svd_offset, cseg->s_as,
			   cseg->s_base, cseg->s_size, csvp->svd_prot,
			   csvp->svd_maxprot, csvp->svd_type, csvp->svd_cred);

		/*
		 * How can this fail? The vnode has already been mapped
		 * once in the parent segment?
		 */
		ASSERT(error == 0);
	}

	return (0);
}

/*
 * STATIC void
 * segvn_childload(struct seg *pseg, struct seg *cseg)
 *      Called from as_childload to complete the fork operation after the
 *	child becomes visible to the swapper. This routine dups the
 *	anon_map of PRIVATE segments, plus calls hat_dup() to load
 *	translations into the child [as an optimization].
 *
 * Calling/Exit State:
 *      The parent's address space is read locked on entry to the call and
 *      remains so on return.
 *
 *      The child's address space is not locked on entry to the call since
 *      there can be no active LWPs in it at this point in time. The segment
 *      is not locked (read or write) for the same reason.
 *
 *	The child's AS is visible to the swapper. Therefore, it is
 *	possible to load up translations for the child in this routine
 *	without running the risk of memory deadlock.
 *
 *	This function can block. No spin LOCKs are held at entry to or
 *	exit from this function.
 *
 * Description:
 *
 *      The tricky cases all deal with a MAP_PRIVATE segment with writable
 *	pages (where COW might become necessary). There are two approaches
 *	to dealing with such pages.
 *
 *	1. COW them now
 *
 *		This is required for memory locked pages, as faults of any
 *		type are inconsistent with the semantic guarantees afforded
 *		to such pages. Therefore, locked pages are COWed at this
 *		time.
 *
 *		In addition, a few other pages might be COWed as sort of a
 *		``pre-COW'' optimization.
 *
 *	2. write protect the translations and permit a fault to COW them later
 *
 *		This is what happens to all other writable pages in a
 *		MAP_PRIVATE segment.
 */
STATIC void
segvn_childload(struct seg *pseg, struct seg *cseg)
{
        struct segvn_data *psvp = (struct segvn_data *)pseg->s_data;
        struct segvn_data *csvp = (struct segvn_data *)cseg->s_data;
	struct anon_map *amp;
	anon_t **papp, **capp;
	boolean_t need_private_dup;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * For SHARED segments, we can simply call hat_dup().
	 */
	if (psvp->svd_type == MAP_SHARED) {
		(void) hat_dup(pseg, cseg, HAT_NOFLAGS);
		return;
	}

	/*
	 * For PRIVATE segments, we hold the segment lock to:
	 *
	 * => Get a consistent picture of the parent's swap resrvation,
	 *    anon_map, and translations.
	 *
	 *    Requires a segment READ lock.
	 *
	 * => Inhibit COW faults in other LWPs of this process.
	 *
	 *    Note that hat_dup(..., HAT_NOPROTW) will only copy read-only
	 *    translations. However, that does not fully inhibit the
	 *    copying of translations for COWed pages (since an
	 *    F_MAXPROT_SOFTLOCK COW fault, or a COW fault followed by a
	 *    concurrent fork, can install a read-only translation for a
	 *    COWed page).
	 *
	 *    Requires a segment READ lock.
	 *
	 * => Inhibit SOFTLOCKs in other LWPs, if hat_chgprot()s are required.
	 *    The hat_chgprot(... ~PROT_WRITE...) operation is in conflict
	 *    with the semantic guarantees afforded to holders of SOFTLOCKs.
	 *
	 *    Requires a segment WRITE lock.
	 *
	 * Consequently, the type of lock we need depends upon volatile
	 * conditions.
	 *
	 * Also note that we cannot copy write enabled translations
	 * to the child for a MAP_PRIVATE segment. We remember this
	 * in hatflag, which will later serve at the flags argument to
	 * hat_dup().
	 */

	for (;;) {
		(void) LOCK_PLMIN(&psvp->svd_seglock);
		amp = psvp->svd_amp;
		if (amp && ((psvp->svd_flags & SEGVN_PGPROT) ||
			      (psvp->svd_prot & PROT_WRITE))) {
			/*
			 * Segment might contain writable translations.
			 */
			if (SEGTRYLOCK_WRITE(psvp)) {
				need_private_dup = B_TRUE;
				break;
			}
		} else {
			/*
			 * Segment does not contain writable translations.
			 */
			if (SEGTRYLOCK_READ(psvp)) {
				need_private_dup = B_FALSE;
				break;
			}
		}
		SV_WAIT(&psvp->svd_segsv, VM_VPAGE_PRI, &psvp->svd_seglock);
	}
	UNLOCK_PLMIN(&psvp->svd_seglock, PLBASE);

	/*
	 * If the parent has a swap reservation, but the child does not,
	 * then the parent must have obtained it since segvn_dup().
	 * If this has occured, then try to get a swap reservation for the
	 * child. But if this fails, then it is acceptable to skip the
	 * the anon_map dup below, provided that we also skip the hat_dup().
	 */
	if (psvp->svd_swresv != csvp->svd_swresv) {
		ASSERT(psvp->svd_swresv == cseg->s_size);
		ASSERT(csvp->svd_swresv == 0);
		if (!mem_resv(seg_pages(cseg), M_SWAP))
			goto out;
		csvp->svd_swresv = cseg->s_size;
	}

	/*
	 * If the parent has an anon_map, then duplicate it for the child.
	 *
	 * Note that we must use the cmap_index of the parent since it may
	 * be sharing an anon_map, but that the child segment, created fresh,
	 * uses a completely private anon_map and array. The index is
	 * therefore not needed.
	 */
	if (amp != NULL) {
		ASSERT(csvp->svd_swresv == cseg->s_size);
		csvp->svd_amp = amp_alloc(cseg->s_size);
		papp = &amp->am_anon[psvp->svd_anon_index];
		capp = csvp->svd_amp->am_anon;
		anon_dup(papp, capp, pseg->s_size);
	}

	/*
	 * We need to:
	 *
	 * => Find all writable and locked pages in the parent, and COW the
	 *    corresponding pages in the child.
	 *
	 * => Write protect the translations for other writable pages
	 */
	if (need_private_dup)
		segvn_private_dup(pseg, cseg, capp);

	/*
	 * OK. Now for the hat_dup() optimization - copy translations
	 *     from the parent to the child.
	 */
	(void) hat_dup(pseg, cseg, HAT_NOPROTW);

out:
	(void) LOCK_PLMIN(&psvp->svd_seglock);
	SEGLOCK_UNLOCK(psvp);
	UNLOCK_PLMIN(&psvp->svd_seglock, PLBASE);
}

/*
 * STATIC void
 * segvn_private_dup(struct seg *pseg, struct seg *cseg, anon_t **capp)
 *	Implement special handling for a dup of a MAP_PRIVATE segment
 *	which might contain write enabled translations.
 *
 * Calling/Exit State:
 *	Processing proceeds from the first virtual page of the segment to
 *	the end.
 *
 *	If the segment contains memory locked pages, then this function
 *	searches for memory locked writable pages in the parent and COWs
 *	the corresponding page in the child's anon_map.
 *
 * 	This function may block. It is called with no LOCKs held (at PLBASE)
 * 	and returns that way.
 *
 * Description:
 *	This routine can issue a single large hat_chgprot() for a large
 *	groups of pages, or even for the entire segment, even when none of
 *	the pages need to be write protected. The theory behind this: for
 *	most families, it is less expensive for hat_chgprot() to loop
 *	throught the pages than to do it here.
 */
STATIC void
segvn_private_dup(struct seg *pseg, struct seg *cseg, anon_t **capp)
{
	struct segvn_data *psvp = (struct segvn_data *)pseg->s_data;
#ifdef DEBUG
	struct segvn_data *csvp = (struct segvn_data *)cseg->s_data;
#endif
	vpage_info_t *vpinfop;
	vaddr_t addr, end_addr, prot_addr;
	uchar_t gprot;
	uchar_t chld_prot;
	size_t prot_size;
	anon_t *old_ap;
	page_t *pp, *cpp;
	uchar_t wmflg;
	boolean_t do_shootdown, pgprot;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(psvp->svd_amp);
	ASSERT(csvp->svd_amp);
	ASSERT(csvp->svd_amp->am_anon == capp);
	ASSERT(psvp->svd_type == MAP_PRIVATE);
	ASSERT((psvp->svd_flags & SEGVN_PGPROT) ||
	       (psvp->svd_prot & PROT_WRITE));

	addr = pseg->s_base;
	end_addr = addr + pseg->s_size;
	vpinfop = psvp->svd_info;

	/*
	 * If no pages are memory locked, then just deny write permission
	 * to the entire segment.
	 */
	if (!(psvp->svd_flags & SEGVN_MEMLCK)) {
		hat_chgprot(pseg, addr, pseg->s_size, ~PROT_WRITE, B_TRUE);
		return;
	}

	/*
	 * Locked pages exists in the parent's address space. Some of
	 * these might be write enabled.
	 */
	ASSERT(vpinfop);
	do_shootdown = B_FALSE;
	pgprot = (psvp->svd_flags & SEGVN_PGPROT);
	gprot = psvp->svd_prot;

	while (addr < end_addr) {
		/*
		 * COW locked + writable pages
		 */
		wmflg = (vpinfop->vpi_flags | gprot) & (VPI_MEMLOCK|PROT_WRITE);
		if (capp && wmflg == (VPI_MEMLOCK|PROT_WRITE)) {
			/*
			 * Writeable and memory locked:
			 *
			 * COW the child's anon page.
			 *
			 * Note: the locked writable pages are anon,
			 * so they cannot be aborted.
			 */
			old_ap = *capp;
			ASSERT(old_ap);
			ASSERT(old_ap->an_page);
			cpp = anon_private(capp, old_ap->an_page, M_NONE);
			ASSERT(cpp);
			anon_decref(old_ap);
			chld_prot = (vpinfop->vpi_flags | pgprot) & PROT_ALL;
			hat_memload(cseg, addr, cpp, chld_prot, HAT_NOFLAGS);
			page_unlock(cpp);
			goto next_page;
		}

		/*
		 * We need only protect pages which are writable and not
		 * memory locked. Skip otherwise.
		 */
		if (wmflg != PROT_WRITE)
			goto next_page;

		/*
		 * Now gather up as many pages as we can to chgprot at once.
		 */
		prot_addr = addr;
		prot_size = 0;
		do {
			++vpinfop;
			addr += PAGESIZE;
			prot_size += PAGESIZE;
			++capp;
			if (addr == end_addr)
				break;
			wmflg = (vpinfop->vpi_flags | gprot) &
				(VPI_MEMLOCK|PROT_WRITE);
		} while (wmflg == PROT_WRITE);

		/*
		 * Write enabled, but not locked.
		 * Just write protect the translation.
		 */
		do_shootdown |= hat_chgprot(pseg, prot_addr, prot_size,
					    ~PROT_WRITE, B_FALSE);
		continue;

next_page:
		++vpinfop;
		++capp;
		addr += PAGESIZE;
	}
	if (do_shootdown)
		hat_uas_shootdown(pseg->s_as);
}

/*
 * STATIC int
 * segvn_unmap(struct seg *seg, vaddr_t addr, uint_t len)
 *	Unmap a portion (up to and including all) of the specified
 *	segment.
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
segvn_unmap(struct seg *seg, vaddr_t addr, uint_t len)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	struct segvn_data *nsvp;
	struct seg *nseg;
	uint_t	opages,		/* old segment size in pages */
		npages,		/* new segment size in pages */
		dpages;		/* pages being deleted (unmapped)*/
	anon_t **app;
	vaddr_t nbase;
	uint_t nsize;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(svp->svd_lckcnt == 0);
	ASSERT(svp->svd_lckflag == 0);
	ASSERT(!SV_BLKD(&svp->svd_segsv));
	ASSERT(!SV_BLKD(&svp->svd_vpagesv));

	ASSERT(addr >= seg->s_base && addr + len <= seg->s_base + seg->s_size);
	ASSERT((len & PAGEOFFSET) == 0);
	ASSERT((addr & PAGEOFFSET) == 0);

	/* Inform the vnode of the unmapping. */
	if (svp->svd_vp) {
#ifdef DEBUG
		int err;

		err =
#else
		(void)
#endif
		VOP_DELMAP(svp->svd_vp, svp->svd_offset, seg->s_as, addr, len,
			   svp->svd_prot, svp->svd_maxprot, svp->svd_type,
			   svp->svd_cred);

		ASSERT(err == 0);
	}

	/*
	 * Remove any page locks set through this mapping.
	 */
	(void) segvn_lockop(seg, addr, len, 0, MC_UNLOCK);

	/*
	 * Unload any hardware translations in the range to be taken out.
	 */
	hat_unload(seg, addr, len, HAT_NOFLAGS);

	seg->s_as->a_isize -= seg->s_size;

	/*
	 * Check for entire segment
	 */
	if (addr == seg->s_base && len == seg->s_size) {
		seg_free(seg);
		return (0);
	}

	opages = seg_pages(seg);
	dpages = btop(len);
	npages = opages - dpages;

	/*
	 * If the segment has a page granular locking array, then just
	 * discard it.  We can safely do this, as the array
	 * is just an optimization. The newly sized segment, or new
	 * segments, will grow new arrays, as required.
	 */
	ASSERT(svp->svd_lckflag == 0);
	if (svp->svd_lock) {
		kmem_free(svp->svd_lock, opages * sizeof(vpage_lock_t));
		svp->svd_lock = NULL;
	}

	/*
	 * Check for beginning of segment
	 */
	if (addr == seg->s_base) {
		if (svp->svd_info != NULL) {
			uint nbytes;
			vpage_info_t *ovpinfop;

			/* keep pointer to vpage */

			ovpinfop = svp->svd_info;

			nbytes = npages * sizeof(vpage_info_t);
			svp->svd_info = kmem_alloc(nbytes,KM_SLEEP);

			bcopy(&ovpinfop[dpages], svp->svd_info, nbytes);

			/* free up old vpage */
			kmem_free(ovpinfop, opages * sizeof(vpage_info_t));
		}
		if (svp->svd_amp != NULL) {
			/*
			 * Only MAP_PRIVATE segments have anon_maps.
			 */
			ASSERT(svp->svd_type == MAP_PRIVATE);

			/*
			 * Free up now unused parts of anon_map array.
			 */
			app = &svp->svd_amp->am_anon[svp->svd_anon_index];
			anon_free(app, len);
			svp->svd_anon_index += dpages;
		}
		if (svp->svd_vp != NULL) {
			svp->svd_offset += len;
		}

		if (svp->svd_swresv) {
			mem_unresv(btop(len), M_SWAP);
			svp->svd_swresv -= len;
		}

		seg->s_base += len;
		seg->s_size -= len;
		return (0);
	}

	/*
	 * Check for end of segment
	 */
	if (addr + len == seg->s_base + seg->s_size) {
		if (svp->svd_info != NULL) {
			uint nbytes;
			vpage_info_t *ovpinfop;

			/* keep pointer to vpage */

			ovpinfop = svp->svd_info;

			nbytes = npages * sizeof(vpage_info_t);
			svp->svd_info = kmem_alloc(nbytes, KM_SLEEP);
			bcopy(ovpinfop, svp->svd_info, nbytes);

			/* free up old vpage */
			kmem_free(ovpinfop, opages * sizeof(vpage_info_t));
		}
		if (svp->svd_amp != NULL) {
			/*
			 * Only MAP_PRIVATE segments have anon_maps.
			 */
			ASSERT(svp->svd_type == MAP_PRIVATE);

			/*
			 * Free up now unused parts of anon_map array
			 */
			app = &svp->svd_amp->am_anon[svp->svd_anon_index +
								npages];
			anon_free(app, len);
		}

		if (svp->svd_swresv) {
			mem_unresv(btop(len), M_SWAP);
			svp->svd_swresv -= len;
		}

		seg->s_size -= len;

		return (0);
	}

	/*
	 * The section to go is in the middle of the segment,
	 * have to make it into two segments.  nseg is made for
	 * the high end while seg is cut down at the low end.
	 */
	nbase = addr + len;				/* new seg base */
	nsize = (seg->s_base + seg->s_size) - nbase;	/* new seg size */
	nseg = kmem_zalloc(sizeof(struct seg), KM_SLEEP);

	/*
	 * Beginning of non-swappable (NO-SWAPOUT) Region.
	 *
	 *	If we blocked while we were transferring the address range from
	 *	the old segment to the new one, we could be swapped, in which
	 *	case as_ageswap() might observe an inconsistent segment
	 *	chain for the address space.
	 */
	DISABLE_PRMPT();
	seg->s_size = addr - seg->s_base;
	(void) seg_attach(seg->s_as, nbase, nsize, nseg);
	nseg->s_ops = seg->s_ops;

	/*
	 * End of non-swappable (NO-SWAPOUT) Region.
	 */
	ENABLE_PRMPT();
	nsvp = kmem_alloc(sizeof(struct segvn_data), KM_SLEEP);
	nseg->s_data = nsvp;
	LOCK_INIT(&nsvp->svd_seglock, VM_SEGVN_HIER, VM_SEGVN_IPL,
		  &segvn_seglockinfo, KM_SLEEP);
	SV_INIT(&nsvp->svd_segsv);
	SV_INIT(&nsvp->svd_vpagesv);
	nsvp->svd_lckflag = 0;
	nsvp->svd_anonlck = 0;
	nsvp->svd_lckcnt = 0;

	nsvp->svd_flags = svp->svd_flags;
	nsvp->svd_prot = svp->svd_prot;
	nsvp->svd_maxprot = svp->svd_maxprot;
	nsvp->svd_type = svp->svd_type;
	nsvp->svd_vp = svp->svd_vp;
	nsvp->svd_cred = svp->svd_cred;
	nsvp->svd_offset = svp->svd_offset + nseg->s_base - seg->s_base;
	nsvp->svd_swresv = 0;
	if (svp->svd_vp != NULL) {
		VN_HOLD(nsvp->svd_vp);
	}
	crhold(svp->svd_cred);

	if (svp->svd_info == NULL)
		nsvp->svd_info = NULL;
	else {
		/* need to split the vpage info array into two arrays */
		uint nbytes;
		vpage_info_t *ovpinfop;

		ovpinfop = svp->svd_info;	/* old info */

		npages = seg_pages(seg);	/* seg has shrunk */
		nbytes = npages * sizeof(vpage_info_t);
		svp->svd_info = kmem_alloc(nbytes, KM_SLEEP);

		bcopy(ovpinfop, svp->svd_info, nbytes);

		npages = seg_pages(nseg);
		nbytes = npages * sizeof(vpage_info_t);
		nsvp->svd_info = kmem_alloc(nbytes, KM_SLEEP);

		bcopy(&ovpinfop[opages - npages], nsvp->svd_info, nbytes);

		/* free up old vpage */
		kmem_free(ovpinfop, opages * sizeof(vpage_info_t));
	}

	if (svp->svd_amp == NULL) {
		nsvp->svd_amp = NULL;
		nsvp->svd_anon_index = 0;
	} else {
		/*
		 * Only MAP_PRIVATE segments have anon_maps.
		 */
		ASSERT(svp->svd_type == MAP_PRIVATE);

		/*
		 * Share the same anon_map structure.
		 */
		opages = btop(addr - seg->s_base);
		npages = btop(nseg->s_base - seg->s_base);

		/*
		 * Free up now unused parts of anon_map array
		 */
		app = &svp->svd_amp->am_anon[svp->svd_anon_index + opages];
		anon_free(app, len);

		/*
		 * Give away upper section of the anon map to the new
		 * segment.
		 */
		nsvp->svd_amp = svp->svd_amp;
		nsvp->svd_anon_index = svp->svd_anon_index + npages;
		nsvp->svd_amp->am_refcnt++;
	}
	nsvp->svd_lock = NULL;
	if (svp->svd_swresv) {
		ASSERT((seg->s_size + nseg->s_size + len) == svp->svd_swresv);
		mem_unresv(btop(len), M_SWAP);
		svp->svd_swresv = seg->s_size;
		nsvp->svd_swresv = nseg->s_size;
	}

	return (0);			/* I'm glad that's all over with! */
}

/*
 * STATIC void
 * segvn_free(struct seg *seg)
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusivley locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 *
 *	No useful value is returned. The operations cannot fail.
 */
STATIC void
segvn_free(struct seg *seg)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	uint_t npages = seg_pages(seg);

	ASSERT(getpl() == PLBASE);
	ASSERT((seg->s_base & PAGEOFFSET) == 0);
	ASSERT((seg->s_size & PAGEOFFSET) == 0);

	/*
	 * release credentials
	 */
	crfree(svp->svd_cred);

	/*
	 * There is no more work for us to do in the case where a
	 * prototype segment never really came into existence, but instead
	 * was immediately concatenated into an existing segment.
	 */
	if (svp->svd_flags & SEGVN_PROTO) {
		return;
	}

	/*
	 * Would really like to assert no one spinning on svd_seglock
	 * but how?
	 */
	ASSERT(svp->svd_lckcnt == 0);
	ASSERT(svp->svd_lckflag == 0);
	ASSERT(!SV_BLKD(&svp->svd_segsv));
	ASSERT(!SV_BLKD(&svp->svd_vpagesv));

	/*
	 * Discard the page granular locking array, if necessary.
	 */
	if (svp->svd_lock != NULL)
		kmem_free(svp->svd_lock, npages * sizeof(vpage_lock_t));

	/*
	 * Deallocate the vpage info array.
	 */
	if (svp->svd_info != NULL)
		kmem_free(svp->svd_info, npages * sizeof(vpage_info_t));

	/*
	 * Free anon array and anon structures, as necessary
	 */
	if (svp->svd_amp != NULL)
		amp_release(svp->svd_amp, svp->svd_anon_index, seg->s_size);

	/*
	 * Release swap reservation.
	 *
	 * We also skip this for a segment concatenated into an existing
	 * segment.
	 */
	if (svp->svd_swresv)
		mem_unresv(btop(svp->svd_swresv), M_SWAP);
	/*
	 * Release claim on vnode, and finally free the private data.
	 */
	if (svp->svd_vp != NULL) {
		VN_RELE(svp->svd_vp);
	}
	LOCK_DEINIT(&svp->svd_seglock);
	kmem_free(svp, sizeof(struct segvn_data));
}

/*
 * STATIC void
 * segvn_softunlock(struct seg *seg, vaddr_t addr, uint_t len, uint_t lcklen,
 *		    enum seg_rw rw)
 *  	Do a F_SOFTUNLOCK over the range requested.  The range must have
 *	already been subjected to an F_SOFTLOCKed attempt - though it
 *	could be one that failed.
 *
 * Calling/Exit State:
 *	Invoked from segvn_fault, the AS must be locked, the segment must
 *	be read locked and the vpage_info array already allocated.
 *	No LOCKs are held at entry to, or at exit from this function.
 *
 *	Virtual page READ or WRITE locks have been taken for the
 *	<addr, addr + len>. The range <addr, addr + lcklen> has been
 *	SOFTLOCKed.
 *
 * Remarks:
 *	For any given page in a given segment there will be no more than
 *	one call to pvn_memresv || pvn_cache_memresv made before calling
 *	pvn_memunresv. The call is made when the first SOFTLOCK is
 *	established.
 */
STATIC void
segvn_softunlock(struct seg *seg, vaddr_t addr, uint_t len, uint_t lcklen,
		 enum seg_rw rw)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	anon_t **app;
	vnode_t *vp, *refvp;
	off_t offset, refoffset;
	vaddr_t adr;
	vpage_info_t *vpinfop;
	int page = seg_page(seg, addr);
	mresvtyp_t mtype;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * grap a handle on the anon map
	 */
	if (svp->svd_amp != NULL)
		app = &svp->svd_amp->am_anon[svp->svd_anon_index + page];
	else
		app = NULL;

	/*
	 * 'Cache' the segment's vp, offset and base for non-anon pages
	 * to save dereferencing costs in the loop below.
	 *
	 * NOTE that these may be NULL/invalid but they are not used unless
	 * the page in question is vnode-backed, in which case they had
	 * better be valid.
	 */

	refvp = svp->svd_vp;
	refoffset = svp->svd_offset + (addr - seg->s_base);

	ASSERT(svp->svd_info);

	vpinfop = &svp->svd_info[page];

	ASSERT(vpinfop != NULL);

	/*
	 * Compute type of memory reservation to release. The type needed
	 * depends upon whether swap has been reserved for the segment.
	 * Note that segvn_lockop(MC_UNLOCK) releases its own reservations,
	 * so that we need only consider genuine softlocks here.
	 */
	mtype = M_SEGVN(svp);

	/*
	 * Unlock pages, unlock translations, and release memory reservations.
	 *
	 * In the for case of unraveling F_SOFTLOCK requests, some
	 * pages in the request may not have been locked.  Therefore,
	 * this loop terminates at "lcklen", not "len".
	 */
	for (adr = addr; adr < addr + lcklen; adr += PAGESIZE) {
		/*
		 * The pages we are dealing with were really locked down.
		 */
		ASSERT(vpinfop->vpi_nmemlck > 0);

		/*
		 * Get the ID of this page.
		 */
		if (app != NULL && *app != NULL) {
			anon_antovp(*app, &vp, &offset);
		} else {
			ASSERT(refvp != NULL);
			vp = refvp;
			offset = refoffset;
		}

		/*
		 * Step 1: unlock the pages
		 *
		 * Call handy-dandy page utility function that finds
		 * the page by identity, removes the read lock we placed
		 * on it, and sets the software mod bit. Wonder what the
		 * poor people are doing today, eh, Smedley? Be a good
		 * chap and pass the sherry!
		 */

		page_find_unlock(vp, offset, ((rw == S_WRITE) ? P_SETMOD : 0));

		/*
		 * softlocks and memory locks both share the hat lock
		 * and memory reservation. The number of reasons for
		 * memory lock (of both types) are recorded in the
		 * vpinfop array.
		 *
		 * Note that vpinfop->vpi_nmemlck is mutexed by the
		 * segment spin lock.
		 */
		(void) LOCK_PLMIN(&svp->svd_seglock);
		if (--vpinfop->vpi_nmemlck == 0) {
			/*
			 * Release translation lock
			 *
			 * Note that we must guard the hat_unlock() operation
			 * with the svd_seglock spinlock in order to
			 * guarantee that the translation is not
			 * relocked by a racing softlock operation.
			 *
			 * It is not necessary to protect the pvn_memunresv()
			 * with the svd_seglock spinlock because it is okay
			 * for the counts to be temporarily inaccurate. It
			 * is not okay for the translation lock to ever be
			 * inaccurate.
			 */
			hat_unlock(seg, adr);
			UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

			/*
			 * Release memory reservation.
			 */
			pvn_memunresv(vp, offset, mtype);
		} else {
			UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
		}

		if (app != NULL)
			app++;
		vpinfop++;
		refoffset += PAGESIZE;
	}

	/*
	 * Now release the locks we hold on the virtual page identities.
	 */
	vpage_unlock_range(svp, page, btop(len));
}

/*
 * STATIC int
 * segvn_swap_resv(struct seg *seg)
 * 	Reserve swap space for the segment.
 *
 * Calling/Exit State:
 *	No spin LOCKS are held. The SEGLOCK pseudo lock is not held.
 *	The segment has no anon_map allocated (unless somebody beat us
 *	in reserving swap space), is mapped PRIVATE, and is backed by a vnode.
 *
 *	Return 0 on success or ENOMEM on failure.
 *
 * Description:
 *	In order to prevent double booking of swap space, any swap space
 *	which had been allocated to locked pages is released from the
 *	pvn cache and transferred to the segment's swap reservation.
 */

STATIC int
segvn_swap_resv(struct seg *seg)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	uint_t npages;
	int ret;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(svp);
	ASSERT(svp->svd_type == MAP_PRIVATE);
	ASSERT(svp->svd_vp != NULL);

	/*
	 * Obtain the segment write lock.
	 *
	 * We need to get the segment write locked before reserving swap
	 * space, as this operation is incompatible with SOFTLOCK processing.
	 * That is because the type of pvn_memresv done by F_SOFTLOCK depends
	 * upon whether or not the segment has reserved swap space.
	 */
	(void) LOCK_PLMIN(&svp->svd_seglock);
	SEGLOCK_WRITE(svp);

	/*
	 * Already reserved?
	 */
	if (svp->svd_swresv != 0) {
		SEGLOCK_UNLOCK(svp);
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
		return (0);
	}
	UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

	ASSERT(svp->svd_amp == NULL);

	/*
	 * Reserve the required M_SWAP reservations, exchanging reservation
	 * as possible with pvn_memresv obtained reservations.  If successful,
	 * this operation will effectively do a pvn_memunresv(...M_SWAP...)
	 * for each locked page.
	 *
	 * If the underlying vnode is swap backed, then we don't actually
	 * hold the M_SWAP reservation for the locked pages (the file
	 * system manager holds them instead).
	 */

	npages = seg_pages(seg);
	ret = 0;
	if ((svp->svd_flags & SEGVN_MEMLCK) &&
	    !(svp->svd_vp->v_flag & VSWAPBACK)) {
		if (!pvn_memresv_swap(npages, segvn_swr_scan, seg)) {
			ret = ENOMEM;
		} else {
			svp->svd_swresv = seg->s_size;
		}
	} else {
		/*
		 * Nothing is locked or the underlying vnode is swap backed.
		 * So, a simple mem_resv() operation suffices.
		 */
		if (!mem_resv(npages, M_SWAP)) {
			ret = ENOMEM;
		} else {
			svp->svd_swresv = seg->s_size;
		}
	}

	/*
	 * release segment write lock.
	 */
	(void) LOCK_PLMIN(&svp->svd_seglock);
	SEGLOCK_UNLOCK(svp);
	UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

	return (ret);
}

/*
 * STATIC void
 * segvn_swr_scan(struct seg *seg, vnode_t *vp, off_t *offset, int *idx)
 *	When iteratively called, this routine scans a segment for locked
 *	pages.
 *
 * Calling/Exit State:
 *	Arguments:
 *		seg (input argument)
 *			segment beging scanned for locked pages
 *		vp, offset (out arguments)
 *			page identity of a locked page
 *		index (bidirectional argument)
 *			The first time this routine is called, the (input)
 *			value of index should be 0. If a locked page is found,
 *			index is updated to reflect the portion of the segment
 *			which has been scanned. If no locked page is found,
 *			then -1 is returned in index, indicating to the caller
 *			that the scan is over.
 *
 *	This routine acquires no locks. It does not block.
 *	The caller owns the AS write lock or the SEGLOCK WRITE lock.
 *
 * Description:
 *	A reference to this routine is passed into pvn_memresv_swap() by
 *	segvn_swap_resv(). pvn_memresv_swap() calls us back in order to
 *	locate the locked pages in the segment.
 */

STATIC void
segvn_swr_scan(struct seg *seg, vnode_t **vp, off_t *offset, int *idx)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	int npages = seg_pages(seg);
	struct vpage_info *vpinfop;
	int index = *idx;

	ASSERT(index <= npages);
	ASSERT(index >= 0);
	ASSERT(svp->svd_info);

	vpinfop = svp->svd_info + index;

	for (;;) {
		if (index == npages) {
			*idx = -1;
			return;
		}
		if (vpinfop->vpi_nmemlck != 0) {
			*vp = svp->svd_vp;
			*offset = svp->svd_offset + ptob(index);
			*idx = index + 1;
			return;
		}
		++vpinfop;
		++index;
	}
}

/*
 * STATIC void
 * segvn_pre_page(struct seg *seg, page_t **ppp, uint_t npages, uint_t vpprot)
 *	Utility function to handle pages obtained from VOP_GETPAGE, but
 *	not on an address being faulted upon.  Called from segvn_fault().
 *
 * Calling/Exit State:
 *	Called with the AS read or write locked, and returns that way.
 *	The segment is either READ, WRITE, or INTENT locked.
 *	Called with no LOCKs held, and returns that way. This function
 *	may block.
 *
 *	The "ppp" array contains "npages" pages to process, each one READ
 *	locked. When this functions returns, all of these READ locks have
 *	been dropped.
 *
 *	"vpprot" is the permissions (as granted by the file system, and
 *	perhaps adjusted by segvn_fault() for the MAP_PRIVATE case).
 *
 * Description:
 *	This routine is basically part of a performance OPTIMIZATION,
 *	albeit a largely cooperative one between VM and the file systems. We
 *	try to load up translations to as many of these pages as possible.
 *
 *	We cannot load up a translation under the following conditions:
 *
 *	=> The page is not within the range covered by our segment.
 *	=> The page is already anonymous in our segment.
 *	=> The page is memory locked.
 *	=> We are unable to get the lock on the virtual page without blocking.
 *
 *	We cannot wait for the virtual page lock here, because we are
 *	not operating in increasing virtual page address order, and thus
 *	could create deadlock. However, as this is only a performance
 *	optimization, and since any other LWP locking the page is going to
 *	take care of the translation, there is no reason to wait.
 */

STATIC void
segvn_pre_page(struct seg *seg, page_t **ppp, uint_t npages, uint_t vpprot)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	anon_t **app;
	vpage_lock_t *vplp;
	vpage_info_t *vpinfop;
	boolean_t must_wakeup = B_FALSE;
	vaddr_t addr;
	int diff, page, segment_pages;
	uint_t prot;
	page_t *pp;
#ifdef DEBUG
	vaddr_t offset = (*ppp)->p_offset;
#endif

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(npages != 0);
	ASSERT(*ppp != NULL);
	ASSERT(svp->svd_vp != NULL);
	ASSERT(svp->svd_lckflag != 0);

	/*
	 * Find the page number (in the segment) of the first page in the list.
	 * Note that this page number might be negative (i.e. not lie in
	 * the segment). This code takes advantage of the fact that the pages
	 * are returned by VOP_GETPAGE in order of increasing offset.
	 *
	 * Note: diff may be negative. The division operator gives the right
	 *	 answer for this case.
	 */
	pp = *ppp;
	diff = (int)(pp->p_offset - svp->svd_offset);
	page = diff / PAGESIZE;
	addr = seg->s_base + diff;
	segment_pages = seg_pages(seg);

	/*
	 * Pre-compute pointers to the anon, page lock, and page info arrays.
	 */
        if (svp->svd_amp == NULL) {
                app = NULL;
        } else {
                app = &svp->svd_amp->am_anon[svp->svd_anon_index + page];
	}
	if (svp->svd_lckflag == SEGVN_IXLCK) {
		ASSERT(svp->svd_lock);
		vplp = &svp->svd_lock[page];
	} else {
		vplp = NULL;
	}
	if (!(svp->svd_flags & (SEGVN_MEMLCK|SEGVN_PGPROT))) {
		vpinfop = NULL;
	} else {
		ASSERT(svp->svd_info);
		vpinfop = &svp->svd_info[page];
	}

	/*
	 * Get the segment spin lock before the loop begins as an
	 * optimization to reduce the number of lock aquisition/release
	 * round trips. Note that the number of times we can traverse the
	 * loop is limited by the file system klustering size.
	 */
	(void) LOCK_PLMIN(&svp->svd_seglock);

	for (;;) {
		ASSERT(PAGE_IS_RDLOCKED(pp));
		ASSERT(pp->p_offset == offset);
		ASSERT(pp->p_vnode == svp->svd_vp);

		/*
		 * skip this page if outside our mapping
		 */
		if (page < 0 || page >= segment_pages)
			goto skip_page;

		/*
		 * skip if already anon
		 */
		if (app && *app)
			goto skip_page;

		/*
		 * skip if memory locked
		 *
		 * Note that the NON-LOCKed access to vpi_nmemlck is
		 * harmless under all memory models:
		 *
		 * 	For a false negative on this test to do harm, we
		 *	would need to load a translation for a page which
		 *	had been locked, and was then aborted. This
		 *	scenario is impossible, because the action of locking
		 *	the page, and then aborting it, will acquire and
		 *	release a PAGE_USELOCK, thus synchronizing vpi_nmemlck
		 *	to memory before we acquired the page READER lock.
		 *
		 * 	A false positive will skip the optimization, and so
		 * 	is harmless.
		 */
		if (vpinfop && vpinfop->vpi_nmemlck)
			goto skip_page;

		/*
		 * We are currently processing a fault. Therefore, the
		 * segment is locked.
		 *
		 * If the segment is WRITE locked, then no other fault is in
		 * progress - so it is safe to proceed. If the segment is
		 * READ locked, then no COW or ZFOD faults are in progress,
		 * so it is safe to proceed. If the segment is INTENT locked,
		 * then just skip the page if any faults at all are in
		 * progress on this page.
		 */
		if (svp->svd_lckflag == SEGVN_IXLCK) {
			if (vplp->vpl_lcktcnt)
				goto skip_page;

			/*
			 * Anon map may have instantiated since we last looked!
			 */
			if (app == NULL && svp->svd_amp != NULL) {
				app = &svp->svd_amp->am_anon[
						svp->svd_anon_index + page];
				if (*app)
					goto skip_page;
			}

			/*
			 * Because hat_memload can block, it is necessary
			 * to take out a true READ lock on the virtual
			 * page.
			 */
			vplp->vpl_lcktcnt = 1;
		}

		/*
		 * compute protections, using per-segment or per-page
		 * information, as appropriate
		 */
		if (!(svp->svd_flags & SEGVN_PGPROT)) {
			prot = svp->svd_prot & vpprot;
		} else {
			ASSERT(vpinfop);
			prot = vpinfop->vpi_flags & vpprot;
		}

		/*
		 * hat_memload might block.  Therefore, release the
		 * spin lock for it.
		 */
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
		hat_memload(seg, addr, pp, prot, HAT_NOFLAGS);
		(void) LOCK_PLMIN(&svp->svd_seglock);

		/*
		 * If we took a virtual page lock, we must now release it,
		 * remembering to wake up any waiters.
		 */
		if (svp->svd_lckflag == SEGVN_IXLCK) {
			ASSERT(vplp->vpl_lcktcnt != 0);
			ASSERT(vplp->vpl_lcktcnt != VPAGE_WR_LOCKED);
			if ((vplp->vpl_lcktcnt == 1 ||
			    vplp->vpl_lcktcnt == VPAGE_MAX_SHARE) &&
			    SV_BLKD(&svp->svd_vpagesv)) {
				must_wakeup = B_TRUE;
			}
			--vplp->vpl_lcktcnt;
		}
skip_page:
		page_unlock(pp);

		/*
		 * Optimized loop exit
		 */
		if ((pp = *++ppp) == NULL || --npages == 0)
			break;

		/*
		 * Onto the next page
		 */
		++page;
		addr += PAGESIZE;
		if (app)
			++app;
		if (vplp)
			++vplp;
		if (vpinfop)
			++vpinfop;
#ifdef DEBUG
		offset += PAGESIZE;
#endif
	}

	UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

	if (must_wakeup) {
		SV_BROADCAST(&svp->svd_vpagesv, 0);
	}
}

/*
 * STATIC faultcode_t
 * segvn_faultpage(struct seg *seg, vaddr_t addr, anon_t **app,
 *	vpage_info_t *vpinfop, page_t *pp, uint_t vpprot, enum seg_rw rw,
 *	enum fault_type type)
 *
 * 	Handles all the dirty work of getting the right anonymous pages and
 *	loading up the translations.
 *
 * Calling/Exit State:
 *	The AS is READ or WRITE locked. If the AS is READ locked, then the
 *	segment is READ, WRITE, or INTENT locked. If the segment is INTENT
 *	locked, then a virtual page lock is held for this page.
 *
 * 	On success 0 is returned and the request has been satisfied. On
 *	failure a non-zero fault code is returned.
 *
 *
 * Description:
 * 	This routine is called from segvn_fault() when looping over the
 *	range of addresses requested, and from segvn_setprot() to COW
 *	locked pages when enabling write access.
 *
 * 	The basic algorithm here is:
 * 		If this is an anon_zero case
 *			If this page is being locked, call memresv
 *			Call anon_zero to allocate page
 *			Load up translation
 *			Return
 *		endif
 *		If this is an anon page
 *			Use anon_getpage to get the page
 *		else
 *			Use pp - passed in from segvn_fault()
 *		endif
 *		If not a COW
 *			If this page is being locked, call pvn_memresv
 *			Load up the translation to the page
 *			Return
 *		endif
 *		If this page is being locked, call pvn_memresv
 *		Call anon_private to handle COW
 *		Manage lock counts
 *
 * Remarks:
 *	One tricky part of this code deals with F_MAXPROT_SOFTLOCKs, which
 *	are the only faults which can COW locked pages. We need to work
 *	to get the pvn_memresv accounting straight, as well as the
 *	HAT_UNLOCK flag to hat_unload.
 *
 *	Because of locking done in segvn_fault this code is essentially
 *	lock free, with one main exception:
 *
 *		=> We need to get the segment spin-lock (svd_seglock)
 *		   to manipulate vpinfop->vpi_nmemlck.
 *
 *	The calls to hat_memload for pages which have RO translation and
 *	are being reloaded with writable translations assume that the HAT
 *	upgrades the protections. We rely upon this behavior in the cases
 *	of COW (MAP_PRIVATE) and file backing store allocation
 *	(MAP_SHARED).
 *
 *	However, the calls to hat_memload for pages which have RW
 *	translations and are being downgraded to RO assume that the HAT
 *	ignores the request. This ``downgrade request'' phenomena can only
 *	occur in the case of a MAP_SHARED segment - when a validity and
 *	protection fault race, and the validity fault gets to install
 *	its translation second. However, if the HAT actually were to
 *	downgrade protections, this would violate the guarantees given
 *	to holders of SOFTLOCKs.
 */
STATIC faultcode_t
segvn_faultpage(struct seg *seg, vaddr_t addr, anon_t **app,
	vpage_info_t *vpinfop, page_t *pp, uint_t vpprot, enum seg_rw rw,
	enum fault_type type)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	int hatflags;
	page_t *anon_pl[1 + 1];
	page_t *opp = NULL;		/* original object page */
	uint_t prot;
	int err;
	enum anaction { ANON_ZERO, ANON_GETPAGE, ANON_NONE };
	enum anaction action;
	boolean_t countup;
	mresvtyp_t mtype;
	anon_t *old_ap = NULL;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Initialize protection value for this page.
	 * Note that protection checks have all been
	 * made already.
	 */
	if (svp->svd_flags & SEGVN_PGPROT) {
		ASSERT(vpinfop);
		prot = vpinfop->vpi_flags & PROT_ALL;
	} else {
		prot = svp->svd_prot;
	}

	action = ANON_NONE;
	if (app != (anon_t **)NULL) {
		/*
		 * Since an anon_map is allocated, it follows that the
		 * segment is mapped PRIVATE.
		 */
		ASSERT(svp->svd_type == MAP_PRIVATE);
		if (*app != (anon_t *)NULL) {
			action = ANON_GETPAGE;
			old_ap = *app;
		} else if (svp->svd_vp == (vnode_t *) NULL) {
			action = ANON_ZERO;
		}
	}

	switch (action) {
	case ANON_ZERO:
		/*
		 * ZFOD can occur only on a segment not backed by a vnode
		 */
		ASSERT(pp == NULL);
		ASSERT(svp->svd_vp == NULL);

		/*
		 * Get a memory reservation for this page if it's being
		 * locked. anon_zero will cache it for us once it
		 * instantiates the page. We can perform the memory
		 * reservation directly in this case because we know the page
		 * is about to be created from scratch and can't have been
		 * locked before.
		 */
		mtype = M_NONE;
		if (type == F_SOFTLOCK) {
			mtype = M_SWAPBACK(svp);
  			if (!(mem_resv(1, mtype))) {
                        	/*
                         	 * Yes, we have no memory
                         	 */
                        	return (FC_MAKE_ERR(EAGAIN));
			}
                }

		/*
		 * Allocate a (normally) writable
		 * anonymous page of zeroes.
		 */
		pp = anon_zero(app, mtype);

		ASSERT(pp);
		ASSERT(PAGE_IS_RDLOCKED(pp));


		if (type == F_SOFTLOCK) {
			/*
			 * Simultaneously locking down and performing ZFOD.
			 *
			 * It is not necessary to protect
			 * vpinfop->vpi_nmemlck with the segment spin
			 * lock because we know that a write lock is held for
			 * the virtual page (at either segment or page
			 * level).
			 */
			ASSERT(vpinfop);
			ASSERT(vpinfop->vpi_nmemlck == 0);
			vpinfop->vpi_nmemlck = 1;
			hat_memload(seg, addr, pp, prot, HAT_LOCK);
		} else {
			hat_memload(seg, addr, pp, prot, HAT_NOFLAGS);
			page_unlock(pp);
		}

		return (0);

	case ANON_GETPAGE:
		/*
		 * Thow away the VOP_GETPAGE gotten page, if one was passed
		 * in. We won't be needing it.
		 */
		if (pp) {
			page_unlock(pp);
		}

#ifdef DEBUG
		(void) LOCK_PLMIN(&svp->svd_amp->am_lock);
		++svp->svd_amp->am_npageget;
		UNLOCK_PLMIN(&svp->svd_amp->am_lock, PLBASE);
#endif /* DEBUG */

		/*
		 * Obtain the page structure via anon_getpage() if it is
		 * a private copy of an object (the result of a previous
		 * copy-on-write).
		 */
		err = anon_getpage(app, &vpprot, anon_pl, PAGESIZE, rw,
				   svp->svd_cred);
		if (err)
			return (FC_MAKE_ERR(err));
		/*
		 * If this is a shared mapping to an anon_map, then an_refcnt == 1,
		 * so that anon_getpage() has already granted write permission.
		 */
		ASSERT((svp->svd_type == MAP_SHARED) ? vpprot & PROT_WRITE : 1);
		opp = anon_pl[0];
		break;

	default:
		/*
		 * case ANON_NONE:
		 *
		 * Use original file page.
		 */
		if (pp == NULL)
			/*
			 *+ In the process of handling a fault on a vnode
			 *+ backed segment we were unable to find a page
			 *+ structure for one of the pages still backed by
			 *+ vnode contents. This probably represents a failure
			 *+ in the backing filesystem code (VOP_GETPAGE) which
			 *+ was to have filled this information out in the page
			 *+ array supplied by segvn_fault. The plsz outarg
			 *+ indicated that this page was filled into the array
			 *+ by the filesystem and should have been present.
			 */
			cmn_err(CE_PANIC,"segvn_faultpage not found");
		ASSERT(pp->p_offset == svp->svd_offset + (addr - seg->s_base));
		opp = pp;
		break;
	}

	/*
	 * All pages processed at this point should just be PAGE_RDLOCKED().
	 * That is to say, their contents are valid.
	 */

	ASSERT(opp != NULL);
	ASSERT(PAGE_IS_RDLOCKED(opp));

	/*
	 * The fault is treated as a copy-on-write fault if a
	 * write occurs on a private segment and the object
	 * page is write protected.  We assume that fatal
	 * protection checks have already been made.
	 *
	 * If not a copy-on-write case load the translation
	 * and return.
	 */

	if (rw != S_WRITE || svp->svd_type != MAP_PRIVATE ||
	    (vpprot & PROT_WRITE) != 0) {

		/*
		 * If we are locking this page under its current identity,
		 * attempt to get a memory reservation now. It may or may
		 * not have been locked before by another AS and only
	 	 * pvn knows for sure.
		 */

		if (type == F_SOFTLOCK) {
			ASSERT(vpinfop);

			/*
			 * Count up the number of reasons for memory locking
			 * a page in the vpage_info structure.
			 *
			 * M_SOFTLOCK returns the type of memory reservation
			 * we will need.
			 *
			 * Each segment only takes a single pvn_memresv
			 * count on each page, no matter how many SOFTLOCKs
			 * are applied. This both conserves on the
			 * pvn_memresv counters, and makes it easy for us to
			 * unreserve the swap component (e.g. when we reserve
			 * M_SWAP for the segment).
			 *
			 * Since we are not changing the page identity,
			 * concurrent SOFTLOCKs may be in progress.
			 * Thus, we mutex vpinfop->vpi_nmemlck with the
			 * segment spin lock.  However, pvn_memresv may block,
			 * so that we cannot call it with a lock held.
			 * Therefore, we sample vpinfop->vpi_nmemlck
			 * before the lock is taken, guessing on whether a
			 * reservation will be needed. If were too optimistic,
			 * then we drop the lock and get a reservation.
			 * If we were too pessimistic, then we drop the
			 * uneeded reservation.
			 */
			mtype = M_SOFTLOCK(svp);
			countup = B_FALSE;
			if (vpinfop->vpi_nmemlck != 0) {
				(void) LOCK_PLMIN(&svp->svd_seglock);
				if (vpinfop->vpi_nmemlck != 0) {
					++vpinfop->vpi_nmemlck;
					countup = B_TRUE;
				}
				UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
			}
			if (!countup) {
				if (!pvn_memresv(opp->p_vnode, opp->p_offset,
						 mtype, SLEEP)) {
					page_unlock(opp);
					return (FC_MAKE_ERR(EAGAIN));
				}
				(void) LOCK_PLMIN(&svp->svd_seglock);
				if (++vpinfop->vpi_nmemlck != 1) {
					UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
					pvn_memunresv(opp->p_vnode,
						      opp->p_offset, mtype);
				} else {
					UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
				}
			}
			ASSERT(vpinfop->vpi_nmemlck != 0);
			hat_memload(seg, addr, opp, prot & vpprot, HAT_LOCK);
		} else /* type != F_SOFTLOCK */ {
			if (vpinfop && (vpinfop->vpi_flags & VPI_MEMLOCK)) {
				/*
				 * If page is already memory locked, then
				 * there is nothing to do here.
				 */
				if (XLAT_LOADED(seg, addr)) {
					page_unlock(opp);
					return (0);
				}

				/*
				 * A memory locked page has been aborted.
				 * We need to mark it as not locked, and
				 * if necessary, release its memory
				 * reservation. The check of vpi_nmemlck is
				 * necessary because a concurrent F_SOFTLOCK
				 * might be in progress.
				 *
				 * svd_seglock protects both vpi_flags
				 * and vpi_nmemlck here.
				 */
				mtype = M_MEMLOCK(svp);
				(void) LOCK_PLMIN(&svp->svd_seglock);
				ASSERT(vpinfop->vpi_nmemlck);
				vpinfop->vpi_flags &= ~VPI_MEMLOCK;
				if (--vpinfop->vpi_nmemlck == 0) {
					UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
					pvn_memunresv(opp->p_vnode,
						      opp->p_offset, mtype);
				} else {
					UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
				}
			}

			hat_memload(seg, addr, opp, prot & vpprot, HAT_NOFLAGS);
			page_unlock(opp);
		}

		return (0);
	}

	ASSERT(app != NULL);

	/*
	 * Copy-on-write case: anon_private() will copy the contents
	 * of the original page into a new page.
	 *
	 * If we are doing memory locking we need to get a memory reservation
	 * for the new page now.
	 */

	ASSERT(svp->svd_swresv != 0);
	hatflags = HAT_NOFLAGS;

	mtype = M_NONE;
	if (type == F_SOFTLOCK) {
		/*
		 * anon_private will ``cache'' the reservation via
		 * pvn_cache_memresv when it sees the AN_LOCK_PAGE flag
		 * passed in.
		 */
		mtype = M_SWAPBACK(svp);
		if (!(mem_resv(1, mtype))) {
			/*
			 * Yes, we have no memory.
			 */
			page_unlock(opp);
			return (FC_MAKE_ERR(EAGAIN));
		}

		/*
		 * Special handling for the case of F_MAXPROT_SOFTLOCK/S_WRITE
		 * with an existing memory locked page. Note that we need to
		 * instruct the HAT to unlock the previously locked page. We
		 * also need to remember to give up the memory reservation
		 * previously taken on this page. Both actions are indicated
		 * by the HAT_UNLOCK flag. However, we know that the
		 * previously locked page was not SOFTLOCKed (only memory
		 * locked) because COWs and SOFTLOCKs are serialized.
		 */
		ASSERT(vpinfop);
		if (vpinfop->vpi_nmemlck) {
			ASSERT(vpinfop->vpi_nmemlck == 1);
			hatflags = HAT_UNLOCK;
		}
	}

	/*
	 * Locked pages are not COWed except by one of:
	 *	 (i) F_MAXPROT_SOFTLOCK/S_WRITE faults (type has already been
	 *	     changed to F_SOFTLOCK), or
	 *	(ii) enabling write protection in segvn_setprot() (also seen
	 *	     as F_SOFTLOCK here).
	 */
	ASSERT(type == F_SOFTLOCK || vpinfop == NULL ||
	       vpinfop->vpi_nmemlck == 0);

	/*
	 * anon_private() will make a new copy of the contents of this
	 * page (opp). The page is currently PAGE_RDLOCK()ed and may or
	 * may not be currently loaded into our address space.
	 */

	pp = anon_private(app, opp, mtype);

	/*
	 * Must have made a new copy.
	 */
	ASSERT(pp != opp);
	ASSERT(PAGE_IS_RDLOCKED(pp));

	/*
	 * Special handling for F_MAXPROT_SOFTLOCK/S_WRITE.
	 * Release the reservation for the formerly locked page.
	 */
	if (hatflags == HAT_UNLOCK) {
		pvn_memunresv(opp->p_vnode, opp->p_offset, mtype);
	}

	/*
	 * Unload the old translation
	 * (if it exists) and unlock the original as we are done
	 * with it.
	 */
	hat_unload(seg, addr, PAGESIZE, hatflags);
	page_unlock(opp);
	if (old_ap)
		anon_decref(old_ap);

	if (type == F_SOFTLOCK) {
		/*
		 * Simultaneously locking down and breaking COW.
		 *
		 * It is not necessary to protect vpinfop->vpi_nmemlck with
		 * the segment spin lock because we know that a write
		 * lock is held for the virtual page (at either segment or
		 * page level).
		 */
		ASSERT(vpinfop);
		++vpinfop->vpi_nmemlck;
		hat_memload(seg, addr, pp, prot, HAT_LOCK);
	} else {
		hat_memload(seg, addr, pp, prot, HAT_NOFLAGS);
		page_unlock(pp);
	}

	return (0);
}

#ifdef DEBUG
int	segvn_faultcnt = 0;
#endif

/*
 * STATIC faultcode_t
 * segvn_fault(struct seg *seg, vaddr_t addr, uint_t len,
 *	       enum fault_type type, enum seg_rw rw)
 *	Fault handler for segvn based segments. Here protection, COW,
 *	inval, and softlock operations are all focused.
 *
 * Calling/Exit State:
 *	Called with the AS lock held (in read mode) and returns the
 *	same.
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
 *	This function is divided into a number of logical operations as
 *	follows:
 *
 *
 *		- take care of special processing for F_SOFTLOCK,
 *		  F_MAXPROT_SOFTLOCK, and F_SOFTUNLOCK cases.
 *
 * 		- determine if this fault resulted from an unrecoverable
 *		  protection violation and return an appropriate error.
 *
 *		- allocate anon_map and make a swap reservation,
 *		  as necessary
 *
 *		- establish locking for the segment based on fault type
 *		  and segment state. The net result is that the segment is
 *		  either left READ, WRITE, or INTENT locked. If INTENT
 *		  locked, then page granular locks are also obtained for
 *		  the virtual pages (this work is done in routine
 *		  vpage_lock_range()). See seg_vn.h for a more complete
 *		  picture.
 *
 *		- loop over the fault range calling VOP_GETPAGE,
 *		  segvn_faultpage, and segvn_pre_page to do actual work:
 *		  LOCKing, COW, or INVAL fault processing. segvn_pre_page
 *		  deals with pages obtained by VOP_GETPAGE, but not in the
 *		  fault range.
 *
 *		- Call vpage_unlock_range() or vpage_downgrade_range()
 *		  [SOFTLOCK case] to downgrade/release segment/page
 *		  level locks as appropriate.
 */
STATIC faultcode_t
segvn_fault(struct seg *seg, vaddr_t addr, uint_t len,
	    enum fault_type type, enum seg_rw rw)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	page_t **ppp;
	anon_t **app, **sapp, **sapp_end;
	vaddr_t a;
	vpage_info_t *vpinfop, *tinfop;
	uint_t vpprot, get_len;
	int err;
	faultcode_t fc;
	page_t *pl[PVN_KLUSTER_NUM + 1];
	int i, remaining, pre_paged;
	uint_t protchk;
	vp_lock_t vplock;
	off_t vp_off;
	size_t vp_len;
	int mem_reserved;
	mresvtyp_t mtype;
	enum seg_rw arw;
	int npage = btop(len);
	int page = seg_page(seg, addr);
	boolean_t check_max = B_FALSE;
	boolean_t escalated;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);
#ifdef DEBUG
	segvn_faultcnt++;
#endif
	/*
	 * Calling this with no pages is illegal.
	 */
	ASSERT(npage);

	/*
	 * special processing for F_SOFTLOCK, F_SOFTUNLOCK, and
	 * F_MAXPROT_SOFTLOCK
	 */
        switch (type) {
		/*
		 *  Setup for F_MAXPROT_SOFTLOCK faults.
		 */
                case F_MAXPROT_SOFTLOCK:
			/*
			 * Protection check must be done against svd_maxprot.
			 */
			check_max = B_TRUE;

			/*
			 * For simplicity, change type to F_SOFTLOCK.
			 */
			type = F_SOFTLOCK;
			/* FALLTHROUGH */

		case F_SOFTLOCK:
			/*
			 * segment must have a vpage_info array allocated
			 * to process this fault
			 */
			if (svp->svd_info == NULL) {
				/*
				 * obtain the segment write lock
				 */
				(void) LOCK_PLMIN(&svp->svd_seglock);
				SEGLOCK_WRITE(svp);
				UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

				/*
				 * segvn_vpage_info_alloc will allocate the
				 * vpage_info array, if it was not already
				 * allocated by someone else while we were
				 * waiting for the segment write lock
				 */
				segvn_vpage_info_alloc(seg);

				/*
				 * release segment write lock.
				 */
				(void) LOCK_PLMIN(&svp->svd_seglock);
				SEGLOCK_UNLOCK(svp);
				UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
			}
			ASSERT(svp->svd_info);
			vpinfop = &svp->svd_info[page];

			/*
			 * SOFTLOCK processing calls VOP_GETPAGEs, holding
			 * down memory.  This requires a reservation (to
			 * prevent deadlock).
			 *
			 * For each page being locked, the reservation is
			 * actually taken twice, once here and once in
			 * segvn_faultpage. We do this because it is simpler
			 * and faster to double book than to try to give
			 * away reservations to segvn_faultpage(), which
			 * may or may not need one, or may need a pvn_memresv
			 * instead. However, the amount of double booking
			 * is limited to PVN_KLUSTER_NUM pages.
			 */
			mem_reserved = MIN(npage, PVN_KLUSTER_NUM);
			mtype = M_SEGVN(svp);
			if (!mem_resv(mem_reserved, mtype)) {
                        	return (FC_MAKE_ERR(EAGAIN));
			}
			break;

		case F_SOFTUNLOCK:
			/*
			 * segvn_softunlock does everything needed
			 */
			segvn_softunlock(seg, addr, len, len, rw);
			return (0);

                default:
			/*
			 * miscellaneous initialization for non-SOFTLOCKs
			 */
			mem_reserved = 0;
			if ((vpinfop = svp->svd_info) != NULL)
				vpinfop += page;
                        break;
        }

	/*
	 * Set protection to be used for checking.
	 */
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
		case S_OTHER:
			goto prot_check_done;
		default:
			/*
			 *+ An undefined seg_rw enum was passed into
			 *+ segvn_fault. This indicates a problem in either
			 *+ the trap, AS, or segment-generic code paths. This
			 *+ is an unrecoverable situation.
			 */
			cmn_err(CE_PANIC, "segvn_fault/unknown rw type");
			/* NOTREACHED */
	}

	/*
	 * per-segment/per-vpage protection checks (as appropriate)
	 *
	 * Since segvn_setprot executes holding the AS write locked,
	 * and since we hold the AS read lock at this point, no additional
	 * locking is needed to perform protection checks.
	 *
	 * OPTIMIZATION: check against svd_prot first
	 *
	 *	This optimization is valid because: (i) if page granular
	 *	protections are in effect, then svd_prot == 0, and
	 *	(ii) svd_prot is always a subset of svd_maxprot.
	 */
	if (!(svp->svd_prot & protchk)) {
		if (check_max) {
			/*
			 * F_MAXPROT_SOFTLOCK case
			 */
			if (!(svp->svd_maxprot & protchk)) {
				if (mem_reserved) {
					mem_unresv(mem_reserved, mtype);
				}
				return (FC_PROT);/* illegal access */
			}
		} else {
			/*
			 * Non-F_MAXPROT_SOFTLOCK case
			 */
			if (!(svp->svd_flags & SEGVN_PGPROT)) {
				if (mem_reserved) {
					mem_unresv(mem_reserved, mtype);
				}
				return (FC_PROT);/* illegal access */
			}
			ASSERT(svp->svd_info);
			tinfop = &svp->svd_info[page];
			i = npage;
			do {
				if (!(tinfop->vpi_flags & protchk)) {
					if (mem_reserved) {
						mem_unresv(mem_reserved, mtype);
					}
					return (FC_PROT); /* illegal access */
				}
				tinfop++;
			} while (--i);
		}
	}
prot_check_done:

	/*
	 * Pre-check for what type of lock will be required. Basically,
	 * we need to determine if the virtual identity of any page can be
	 * changed (or instantiated) by the processing of this fault.
	 * Such identity transformation occurs in the cases of COW and ZFOD.
	 * The conditions determining such an event are:
	 *
	 *	- A write fault to a MAP_PRIVATE segment
	 *
	 *		has COW and ZFOD potential
	 *
	 *	- Any other fault to a segment with no vnode
	 *
	 *		has ZFOD potential
	 *
	 * This information is needed by vpage_lock_range() in determining
	 * what types of locks it needs to take.
	 */

	if (rw == S_WRITE && svp->svd_type == MAP_PRIVATE) {
		/*
		 * Fault has both COW and ZFOD Potential. ZFOD potential
		 * only occurs in the case that svp->svd_vp == NULL. However,
		 * at this point we don't really care if the fault is ZFOD
		 * or COW, as the conditions determining when the page
		 * identity will change are the same for both types. Later on,
		 * segvn_faultpage() will distinguish between these two
		 * cases.
		 */
		vplock = VP_PRIV_WR;
	} else if (svp->svd_vp == NULL) {
		/*
		 * Non-COW fault to anonymous backed memory.
		 * vpage_lock_range must check for ZFOD.
		 */
		vplock = VP_ZFOD;
	} else {
		/*
		 * page identity will not change
		 */
		vplock = VP_READ;
	}

	/*
	 * See if we need an anon map. Any fault which had the potential to
	 * induce a virtual page identity transformation can need an
	 * anon map. If an anon map is needed, then so is the pre-requisite
	 * swap reservation.
	 *
	 * Note that we may or may not finally be the LWP which
	 * instantiates the anon map (we could lose such a race) but
	 * one will exist before we continue on with the fault.
	 */

	if (vplock != VP_READ && svp->svd_amp == NULL) {
		/*
		 * This segment needs an M_SWAP reservation. Make sure it has
		 * one before allocating an anon_map. It might not have a
		 * reservation if this is an F_MAXPROT_SOFTLOCK.
		 *
		 * Note: we sample svd_swresv without holding any
		 * lock as a performance optimization. If a reservation
		 * is actually needed, segvn_swap_resv() will get the
		 * SEGLOCK in WRITE mode, resample svd_swresv, and then
		 * get the reservation.
		 */
		if (svp->svd_swresv == 0 && (err = segvn_swap_resv(seg)) != 0) {
			if (mem_reserved) {
				mem_unresv(mem_reserved, mtype);
			}
			return (FC_MAKE_ERR(err));
		}

		/*
		 * Take the segment AMP_LOCK.
		 */
		(void) LOCK_PLMIN(&svp->svd_seglock);
		SEG_AMP_LOCK(svp);
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

		/*
		 * We may have committed to the anon_map lock and
		 * still lost the race. Recheck the anon_map
		 * pointer and skip if this is so, otherwise
		 * do the allocation.
		 */

		if (svp->svd_amp == NULL) {
			svp->svd_amp = amp_alloc(seg->s_size);
		}

		/*
		 * Release the segment AMP_LOCK
		 */
		(void) LOCK_PLMIN(&svp->svd_seglock);
		SEG_AMP_UNLOCK(svp);
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
	}

	/*
	 * Now let vpage_lock_range get the lock type(s) appropriate for
	 * this fault.
	 *
	 * Optimization: just take the segment write lock if this process
	 *		 is single threaded.
	 */
	if (SINGLE_THREADED()) {
		(void) LOCK_PLMIN(&svp->svd_seglock);
		SEGLOCK_WRITE(svp);
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
		escalated = B_TRUE;
	} else {
		escalated = vpage_lock_range(seg, page, npage, vplock);
	}

	ASSERT(svp->svd_lckcnt && svp->svd_lckflag);

	/*
	 * Get a handle on the anon map (if present).
	 */
	if (svp->svd_amp == NULL) {
		app = NULL;
	} else {
		app = &svp->svd_amp->am_anon[svp->svd_anon_index + page];
	}
	if (svp->svd_vp) {
		vp_off = svp->svd_offset + (addr - seg->s_base);
		vp_len = len;
	}

	/*
	 * Ok, now loop over the address range and handle:
	 *
	 *	=> getting pages from the file system via VOP_GETPAGE
	 *	=> handling faults via segvn_faultpage
	 */
	a = addr;
	ppp = pl;
	*ppp = (page_t *)NULL;
	remaining = npage;
	for (;;) {
		/*
		 * See if we need to call VOP_GETPAGE for *any* of the range
		 * being faulted on.  We can skip all of this work if there
		 * was no original vnode, if the required page was fetched
		 * by a call to VOP_GETPAGE on a previous pass through the
		 * loop, or if the page at the current fault address is
		 * already anonymous.
		 */
		if (svp->svd_vp != NULL && *ppp == NULL &&
		    (app == NULL || *app == NULL)) {

			/*
			 * Try to get all remaining pages in the fault
			 * range at once. However, we cannot get more
			 * than PVN_KLUSTER_NUM pages at once, because
			 * we have neither the array storage nor the
			 * memory reservations to accommodate it.
			 *
			 * Trim back the request to the maximum non-anonymous
			 * range starting at the current fault address.
			 */
			if (app) {
				sapp = app;
				sapp_end = sapp +
					MIN(remaining, PVN_KLUSTER_NUM);
				while (++sapp < sapp_end && *sapp == NULL)
					;
				get_len = ptob(sapp - app);
			} else {
				get_len = MIN(vp_len, PVN_KLUSTER_SZ);
			}

			/*
			 * S_WRITE faults on MAP_PRIVATE segments will give
			 * us a private copy of the vnode-backed page (i.e we
			 * will COW it). However, we must be careful
			 * regarding which rw flag to pass in because for a
			 * private mapping, the underlying object is never
			 * allowed to be written.
			 */
			if (vplock == VP_PRIV_WR) {
				arw = S_READ;
			} else {
				arw = rw;
			}

			err = VOP_GETPAGE(svp->svd_vp, vp_off, get_len,
					  &vpprot, pl, PVN_KLUSTER_SZ,
					  seg, a, arw, svp->svd_cred);
			if (err) {
				fc = FC_MAKE_ERR(err);
				goto error_exit_2;
			}

			/*
			 * For MAP_PRIVATE segments, remove any write
			 * permissions which may have been granted by
			 * the backing filesystem. This makes the pages
			 * COW (see segvn_faultpage()).
			 */
			if (svp->svd_type == MAP_PRIVATE)
				vpprot &= ~PROT_WRITE;

			/*
			 * At this time the pl array has the needed non-anon
			 * page for the current fault address, in addition to
			 * (possibly) having some adjacent pages. Some of
			 * these might be used for subsequent pages in the
			 * fault range. Others are simply pre-paged
			 * (klustered) in.
			 *
			 * We scan the page list returned by VOP_GETPAGE
			 * in increasing address order. At this time we
			 * handle any pre-paged pages preceding the current
			 * fault address.
			 */
			ASSERT(vp_off >= pl[0]->p_offset);
			pre_paged = btop(vp_off - pl[0]->p_offset);
			if (pre_paged > 0) {
				segvn_pre_page(seg, pl, pre_paged, vpprot);
			}
			ppp = pl + pre_paged;

		} /* svp->svd_vp != NULL && *ppp == NULL && ... */

		/*
		 * Now handle the fault on one page.
		 *
		 *	Note that segvn_faultpage always unlocks *ppp,
		 *	even if an error is returned.
		 */
		fc = segvn_faultpage(seg, a, app, vpinfop, *ppp, vpprot,
				      rw, type);
		if (*ppp)
			++ppp;
		if (fc)
			goto error_exit_1;

		/*
		 * loop exit optimized for one page case
		 */
		if (--remaining == 0) {
			break;
		}

		/*
		 * onto the next page
		 */
		a += PAGESIZE;
		if (app)
			app++;
		if (vpinfop)
			vpinfop++;
		if (svp->svd_vp) {
			vp_off += PAGESIZE;
			vp_len -= PAGESIZE;
		}

	} /* per-page for loop */

	/*
	 * Now handle any left over pre-paged pages.
	 */
	if (*ppp) {
		segvn_pre_page(seg, ppp, PVN_KLUSTER_NUM, vpprot);
	}

	/*
	 * All VOP_GETPAGE gotten pages have been handled. So drop the
	 * memory reservation now.
	 */
	if (mem_reserved) {
		mem_unresv(mem_reserved, mtype);
	}

	/*
	 * We waited till we were done to drop all the virtual page locks
	 * at once. This batching is required by the auto-escalation
	 * optimization (because in the escalated case only a single lock
	 * is held, the SEGLOCK).
	 *
	 * In the case of SOFTLOCKing, we don't actually drop the lock(s),
	 * but downgrade them to reader mode.
	 */
	if (type == F_SOFTLOCK) {
		vpage_downgrade_range(svp, page, npage);
	} else if (escalated) {
		(void) LOCK_PLMIN(&svp->svd_seglock);
		SEGLOCK_UNLOCK(svp);
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
	} else {
		vpage_unlock_range(svp, page, npage);
	}

	return (0);

error_exit_1:
	/*
	 * Release any pages not yet handled, plus release the reservation.
	 */
	while (*ppp != NULL) {
		ASSERT(PAGE_IS_LOCKED(*ppp));
		page_unlock(*ppp++);
	}
error_exit_2:
	if (mem_reserved) {
		mem_unresv(mem_reserved, mtype);
	}

	/*
	 * Drop all the SOFTLOCKs and virtual page locks
	 * that we might have acquired.
	 */
	if (type == F_SOFTLOCK) {
		ASSERT(svp->svd_info);
		segvn_softunlock(seg, addr, len, a - addr, S_OTHER);
	} else {
		vpage_unlock_range(svp, page, npage);
	}

	return (fc);
}

/*
 * STATIC int
 * segvn_setprot(struct seg *seg, vaddr_t addr, uint_t len, uint_t prot)
 *      Change the protections on one or more virtually contiguous pages
 *      within a single segment (seg) beginning at `addr' for `len' bytes.
 *
 * Calling/Exit State:
 *      Called and exits with the address space WRITE locked.
 *
 *      Returns zero on success. If the target pages had been previously
 *      loaded they will now be unloaded.
 *
 *      Returns a non-zero errno on failure.
 *
 * Remarks:
 *      In SVR4 ``classic'' this function was complicated by the fact that
 *      we could memory lock pages in core and leave them COW. This required
 *      the memory locking code to maintain separate counts for existing and
 *      potential memory claims and for memory claims to be shuffled back and
 *	forth between the two during setprots.
 *
 *      In ES/MP, all MAP_PRIVATE/RO (COWable) pages are COWed as part of
 *      memory locking and so separate counts are not required, making this
 *      setprot function much simpler. However, if we grant write permission
 *      to memory locked pages in a vnode-backed MAP_PRIVATE segment they
 *	become COWable, a condition we tried to avoid when the memory locking
 *	was first done. Another consideration, the granting of write
 *	permission to MAP_SHARED, vnode backed, and memory locked pages
 *	must cause the allocation of backing store (by the file system).
 *
 *	This is handled by detecting these conditions within segvn_setprot
 *	and then calling VOP_GETPAGE() and segvn_faultpage(..., F_SOFTLOCK)
 *	as needed. The beauty of this scheme is that since
 *	segvn_faultpage() does not actually change the page protections, no
 *	backout is required should it fail!
 */
/*ARGSUSED*/
STATIC int
segvn_setprot(struct seg *seg, vaddr_t addr, uint_t len, uint_t prot)
{
        struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	int page = seg_page(seg, addr);
	int npage = btop(len);
	vaddr_t saddr, caddr;
	faultcode_t fc;
	enum seg_rw rw;
	uint_t clen, vpprot;
	uchar_t gprot, cprot, lkflag;
	vpage_info_t *vpinfop, *evpinfop;
	int spage, err;
	mresvtyp_t mtype;
	struct anon_map *amp;
	anon_t **app;
	vnode_t *refvp;
	off_t refoffset;
	page_t *pl[2];
	boolean_t do_shootdown;

	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);

        if ((svp->svd_maxprot & prot) != prot)
                return (EACCES);                /* violated maxprot */

	/*
	 * Quick exit optimization:
	 *
	 *	If protections are not page granular, and we are not
	 *	changing them, then exit immediately.
	 */
	if (!(svp->svd_flags & SEGVN_PGPROT) && prot == svp->svd_prot) {
		return (0);
	}

        /*
         * Special handling for MAP_PRIVATE segments:
	 *
	 * We may need to reserve swap space, or we might be able to give
	 * some up (very unlikely).
	 */

        if (svp->svd_type == MAP_PRIVATE) {
		if ((prot & PROT_WRITE) != 0) {
			/*
			 * If it's a private mapping and we're making any
			 * part of it writable, then we must have swap
			 * space reserved.
			 */
			if (svp->svd_swresv == 0 &&
			    (err = segvn_swap_resv(seg)) != 0)
				return (err);
		} else {
			/*
			 * If it's a private mapping, and we're removing write
			 * permission for the whole mapping, and we haven't
			 * modified any pages, and we haven't locked any pages,
			 * and if the segment has no ZFOD potential, then we
			 * can release the swap.
			 */
			if (svp->svd_amp == NULL && svp->svd_vp != NULL &&
			    svp->svd_swresv != 0 &&
			    addr == seg->s_base && len == seg->s_size &&
			    !(svp->svd_flags & SEGVN_MEMLCK)) {
				mem_unresv(btop(svp->svd_swresv), M_SWAP);
				svp->svd_swresv = 0;
			}
		}
        }

	/*
	 * If we are enabling write access, and if the segment has memory
	 * locked pages, then we must break COW (MAP_PRIVATE) or fill file
	 * backing store (MAP_SHARED) on all these pages. Note that we hold
	 * tbe AS write lock, so that no new children can be created to share
	 * the COWed pages this routine is creating.
	 *
	 * However, it is possible for locked file backed pages to be
	 * aborted, perhaps even while this routine is executing. If a page
	 * has been aborted, then it is semantically INCORRECT for us to
	 * relock it. Consequently, this routine may not call segvn_fault()
	 * specifying F_MAXPROT_SOFTLOCK to do this work. Therefore, a piece
	 * of segvn_fault() is cloned here.
	 *
	 * It is okay to do a hat_chgprot(..., PROT_WRITE) for a virtual page
	 * which was previously locked down, and was then aborted. This is
	 * because the translation is now NULL, making the hat_chgprot() a
	 * nop.
	 */
	gprot = svp->svd_prot;	/* 0 when using page granular protections */

	if ((prot & PROT_WRITE) && !(gprot & PROT_WRITE) &&
	    (svp->svd_flags & SEGVN_MEMLCK)) {

		ASSERT(svp->svd_info);
		vpinfop = &svp->svd_info[page];
		evpinfop = vpinfop + npage;
		saddr = addr;

		/*
		 * The pvn_memresv type of the locked pages is based upon
		 * whether the segment has swap space reserved (on a segment
		 * wide basis).
		 */
		mtype = M_MEMLOCK(svp);

		/*
		 * Grab a handle on the anon map.
		 */
		if (svp->svd_amp != NULL)
			app = &svp->svd_amp->am_anon[svp->svd_anon_index +
						     page];
		else
			app = NULL;
		spage = page;

		/*
		 * Cache these values in local variables.
		 */
		refvp = svp->svd_vp;
		refoffset = svp->svd_offset + (saddr - seg->s_base);

		/*
		 * Loop over all pages in the range.
		 */
		while (vpinfop < evpinfop) {
			/*
			 * skip over pages not memory locked, or which are
			 * already writable
			 */
			if (((vpinfop->vpi_flags | gprot) &
			     (VPI_MEMLOCK|PROT_WRITE)) != VPI_MEMLOCK)
				goto next_page;
			ASSERT(vpinfop->vpi_nmemlck == 1);

			/*
			 * We need to fill backing store for MAP_SHARED file
			 * backed pages. For all file backed pages, we must
			 * get a read locked page (unless the page has been
			 * aborted).
			 *
			 * Note that anonymous pages cannot be aborted.
			 */
			if (app != NULL && *app != NULL)
				pl[0] = NULL;
			else {
				ASSERT(refvp);

				if (!XLAT_LOADED(seg, saddr)) {
					/*
					 * A locked page has been aborted.
					 * We need to "erase" the locked
					 * state in the segment driver and
					 * release the pvn_memresv.
					 */
					ASSERT(vpinfop->vpi_nmemlck == 1);
					vpinfop->vpi_flags &= ~VPI_MEMLOCK;
					vpinfop->vpi_nmemlck = 0;
					pvn_memunresv(refvp, refoffset, mtype);
					goto next_page;
				}

				/*
				 * Get the read locked page from the
				 * file system. If this is a PRIVATE
				 * then we must not allocate backing
				 * store.
				 */
				rw = (svp->svd_type == MAP_PRIVATE) ?
							S_READ : S_WRITE;
				err = VOP_GETPAGE(refvp, refoffset,
						  PAGESIZE, &vpprot, pl,
						  PAGESIZE, seg, saddr, rw,
						  svp->svd_cred);

				/*
				 * The VOP_GETPAGE() cannot generate a page
				 * abort on our virtual page. Thus, if it
				 * has now been aborted, it must have been
				 * aborted by some other action of the user.
				 * Thus, if the page was aborted, we can
				 * then ignore any errors returned by
				 * VOP_GETPAGE().
				 *
				 * VOP_GETPAGE() may have allocated backing
				 * store after the page was aborted. This
				 * is an unpleasant consequence of the
				 * implementation, but not a bug. The
				 * alternative (locking the page before
				 * calling VOP_GETPAGE()) is worse, since
				 * it would recursively read lock the page.
				 */
				if (!XLAT_LOADED(seg, saddr)) {
					/*
					 * A locked page has been aborted.
					 * We need to "erase" the locked
					 * state in the segment driver and
					 * release the pvn_memresv.
					 */
					ASSERT(vpinfop->vpi_nmemlck == 1);
					vpinfop->vpi_flags &= ~VPI_MEMLOCK;
					vpinfop->vpi_nmemlck = 0;
					pvn_memunresv(refvp, refoffset, mtype);
					if (err == 0)
						page_unlock(pl[0]);
					goto next_page;
				}

				/*
				 * If we could not allocate backing store,
				 * then we cannot continue.
				 */
				if (err)
					return (err);
			}

			/*
			 * MAP_PRIVATE case: we need to COW the page, locking
			 * down the new page and unlocking the old.
			 * F_SOFTLOCK does both of these things.
			 */
			if (svp->svd_type == MAP_PRIVATE) {
				ASSERT(svp->svd_swresv);

				/*
				 * Do we need an anon_map?
				 */
				if (app == NULL) {
					amp = amp_alloc(seg->s_size);
					svp->svd_amp = amp;
					app = &amp->am_anon[spage];
				}

				/*
				 * We are holding the AS write lock. Thus,
				 * there is no need to obtain a
				 * vpage_lock_range() lock.
				 *
				 * 
				 */
				fc = segvn_faultpage(seg, saddr, app,
						     vpinfop, pl[0],
						     vpprot & ~PROT_WRITE,
						     S_WRITE, F_SOFTLOCK);
				if (fc) {
					ASSERT(vpinfop->vpi_nmemlck == 1);
					ASSERT(FC_CODE(fc) == FC_OBJERR);
					return (FC_ERRNO(fc));
				}

				/*
				 * The COWed page is now both SOFTLOCKed
				 * and memory locked (with one exception: we
				 * are not holding a vpage_lock_range()
				 * lock). We need to release the SOFTLOCK
				 * component, leaving just the memory lock.
				 */
				ASSERT(*app);
				ASSERT((*app)->an_page);
				ASSERT(vpinfop->vpi_flags & VPI_MEMLOCK);
				ASSERT(vpinfop->vpi_nmemlck == 2);
				pl[0] = (*app)->an_page;
				vpinfop->vpi_nmemlck = 1;
			}
			page_unlock(pl[0]);
			ASSERT(vpinfop->vpi_nmemlck == 1);

next_page:
			++vpinfop;
			++spage;
			saddr += PAGESIZE;
			refoffset += PAGESIZE;
			if (app)
				++app;

		} /* COW/backing store fill per-page loop */

	} /* enabling write access with locked pages */

	/*
	 * we are now commmited to the protection change
	 */

	/*
	 * Okay, onto updating the protections in the segment private data.
	 * We have two basic cases to take care of:
	 *	=> page granular protections
	 *	=> segment wide protections
	 * Page granular protections will be necessary if we are changing
	 * protections for less than the entire segment.
	 */

	if (len != seg->s_size) {

		/*
		 * If page granular protections are not already in place,
		 * then we must establish them now.
		 */
		if (!(svp->svd_flags & SEGVN_PGPROT)) {
			/*
			 * if no virtual page info array,
			 * then allocate one now.
			 */
			if (svp->svd_info == NULL) {
				segvn_vpage_info_alloc(seg);
			}

			/*
			 * Copy protections into the virtual page
			 * info array.
			 */
			vpinfop = svp->svd_info;
			evpinfop = vpinfop + seg_pages(seg);
			while (vpinfop < evpinfop)
				(vpinfop++)->vpi_flags |= gprot;

			/*
			 * Now clear out segment wide protections, and
			 * record the transfer of protections to the
			 * page granular level.
			 */
			svp->svd_prot = 0;
			svp->svd_flags |= SEGVN_PGPROT;
		}

		/*
		 * Now set new protections
		 */
		ASSERT(svp->svd_info);
		vpinfop = &svp->svd_info[page];
		evpinfop = vpinfop + npage;
		while (vpinfop < evpinfop) {
			vpinfop->vpi_flags =
				(vpinfop->vpi_flags & ~PROT_ALL) | prot;
			++vpinfop;
		}
	} else {
		ASSERT(page == 0);

		/*
		 * If page granular protections are already in place, we must
		 * destroy them.
		 */
		switch (svp->svd_flags & (SEGVN_PGPROT|SEGVN_MEMLCK)) {
		/*
		 * Nothing is locked and page granular protections are set.
		 * So just discard the entire vpage_info array.
		 */
		case SEGVN_PGPROT:
			ASSERT(svp->svd_info);
			kmem_free(svp->svd_info,
				  sizeof(vpage_info_t) * npage);
			svp->svd_info = NULL;
			svp->svd_flags &= ~SEGVN_PGPROT;
			break;

		/*
		 * Page granular protections are set, but some pages may
		 * be locked. Clear the protection values in the
		 * vpage_info array.
		 */
		case SEGVN_PGPROT|SEGVN_MEMLCK:
			vpinfop = svp->svd_info;
			ASSERT(vpinfop);
			evpinfop = vpinfop + npage;
			while (vpinfop < evpinfop) {
				vpinfop->vpi_flags |=
					vpinfop->vpi_flags & ~PROT_ALL;
				++vpinfop;
			}
			svp->svd_flags &= ~SEGVN_PGPROT;
			break;

		default:
			break;
		}

		/*
		 * setting global protections
		 */
		svp->svd_prot = (uchar_t)prot;
	}

	/*
	 * Finally, we need to change the protections in the PTEs.
	 *
	 * We cannot enable write permission in any of the following cases:
	 *
	 *	=> Any vnode backed page for which no backing store has
	 *	   been allocated by the file system.
	 *
	 *	   The only vnode backed pages for which we can be sure
	 *	   that backing store is allocated are the memory locked
	 *	   pages.
	 *
	 *	=> Any vnode backed page in a MAP_PRIVATE segment.
	 *
	 *	=> Any anonymous pages in a MAP_PRIVATE segment shared with
	 *	   another process.
	 *
	 * Conversely, if asked to do so, we must enable write permission
	 * for memory locked pages (since the semantic guarantees afforded
	 * to such pages make them immune to blocking faults).
	 *
	 * PERF: this routine uselessly denies write permission to the
	 *	 translations in either of the following cases:
	 *
	 *	=> when changing READ or EXECUTE permission on page whose
	 *	   translation is already WRITE enabled.
	 *
	 *	   In this case, the hat layer already knows if the page
	 *	   has write permission. But, the interface doesn't
	 *	   permit us to use this knowledge.
	 *
	 *	=> when enabling WRITE permission on a MAP_PRIVATE segment
	 *	   with anonymous pages
	 *
	 *	   In this case, we should first check if the anon page is
	 *	   shared with another process.
	 */
	if ((prot & PROT_WRITE) && (svp->svd_flags & SEGVN_MEMLCK)) {
		/*
		 * Enabling write permission on a segment which contains
		 * locked pages.
		 */
		ASSERT(svp->svd_info);
		vpinfop = &svp->svd_info[page];
		evpinfop = vpinfop + npage;
		do_shootdown = B_FALSE;
		while (vpinfop < evpinfop) {
			/*
			 * Gather up groups of pages, all of which are memory
			 * locked, or all of which are not memory locked.
			 */
			lkflag = (vpinfop->vpi_flags & VPI_MEMLOCK);
			caddr = addr;
			addr += PAGESIZE;
			++vpinfop;
			clen = PAGESIZE;
			while (vpinfop < evpinfop &&
			       (vpinfop->vpi_flags & VPI_MEMLOCK) == lkflag) {
				addr += PAGESIZE;
				clen += PAGESIZE;
				++vpinfop;
			}
			/*
			 * If the group of pages is memory locked, then
			 * just do what the caller asks for. For non-locked
			 * pages, deny write permission.
			 */
			cprot = (uchar_t)prot;
			if (!lkflag)
				cprot &= ~PROT_WRITE;
			do_shootdown |= hat_chgprot(seg, caddr, clen, cprot,
						    B_FALSE);
		}
		if (do_shootdown)
			hat_uas_shootdown(seg->s_as);
	} else {
		/*
		 * Either no locked pages, or not enabling write permission.
		 * A single hat_chgprot denying write permission will do
		 * the job.
		 */
		hat_chgprot(seg, addr, len, prot & ~PROT_WRITE, B_TRUE);
	}

        return (0);
}

/*
 * STATIC int
 * segvn_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
 *	Determine that for len bytes starting at addr the protection
 *	is at least equal to prot.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *      On success, 0 is returned, indicating that all of the pages
 *      in the range [addr, addr + len) allow accesses indicated by
 *      the specified protection. Actual protection may be greater.
 *      On failure, EACCES is returned, to indicate that at least one
 *	of the pages (or the entire segment) does not allow the
 *	desired access.
 *
 * Remarks:
 *	The AS lock stabilizes the page protection information.
 */
STATIC int
segvn_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	vpage_info_t *vpinfop;

	ASSERT((addr & PAGEOFFSET) == 0);

	/*
	 * If segment protection can be used, simply check against them.
	 */
	if (!(svp->svd_flags & SEGVN_PGPROT))  {
		return (((svp->svd_prot & prot) != prot) ? EACCES : 0);
	}

	/*
	 * Have to check down to the vpage level.
	 */
	ASSERT(svp->svd_info);
	vpinfop = &svp->svd_info[seg_page(seg, addr)];
	if ((vpinfop->vpi_flags & prot) != prot) {
		return (EACCES);
	}
	return (0);
}

/*
 * STATIC int
 * segvn_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
 *	Return the protections on pages starting at addr for len.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	This function, which cannot fail, returns the permissions of the
 *	indicated pages in the protv array.
 */
STATIC int
segvn_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
{
 	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	
	ASSERT((addr & PAGEOFFSET) == 0);

	if (!(svp->svd_flags & SEGVN_PGPROT)) {
		*protv =  svp->svd_prot;
	} else {
		int pgoff = seg_page(seg, addr);

		ASSERT(svp->svd_info);
		*protv = svp->svd_info[pgoff].vpi_flags & PROT_ALL;
	}

	return 0;
}

/*
 * STATIC off_t
 * segvn_getoffset(struct seg *seg, vaddr_t addr)
 *	Return the logical offset of the virtual address within
 *	the segment.
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
segvn_getoffset(struct seg *seg, vaddr_t addr)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;

	return (addr - seg->s_base + svp->svd_offset);
}

/*
 * STATIC off_t
 * segvn_gettype(struct seg *seg, vaddr_t addr)
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
segvn_gettype(struct seg *seg, vaddr_t addr)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;

	return svp->svd_type;
}

/*
 * STATIC int
 * segvn_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
 *	Return the vnode for this segment (if one exists).
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
segvn_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;

	if ((*vpp = svp->svd_vp) == (vnode_t *) NULL)
		return -1;

	return 0;
}

/*
 * STATIC int
 * segvn_kluster(struct seg *seg, vaddr_t addr, int delta)
 * 	Check to see if it makes sense to do klustering (read ahead) to
 * 	addr + delta relative to the mapping at addr.  We assume here
 * 	that delta is a signed PAGESIZE'd multiple (which can be negative).
 *
 * Calling/Exit State:
 *	Called indirectly via a VOP from segvn_fault. The AS is locked and
 *	the segment is READ, WRITE, or INTENT locked. If the segment is
 *	INTENT locked, then the base virtual page is read or write locked
 *	as well.
 *
 *	On success (klustering approved) 0 is returned; on failure -1.
 * 	We currently `approve' of the action if we are still in the segment
 *	and both pages are still backed by the original vnode or if the
 *	segment is shared. It is assumed that our caller tests one page at
 *	a time, before or after the base page at addr, gradually increasing
 *	the range of delta until either a segment boundary is reached or an
 *	anon page is encountered. The logic of the function depends on this
 *	behavour.
 *
 *	If either of the two pages are anonymous (and currently we do not
 *	kluster in from anonfs, so the base page should never be anon) then
 *	we return.
 *
 * Remarks:
 *	Currently, we never kluster in anon pages. This is ASSERTed by the
 *	code below. A non-anon page CANNOT become anon out from underneath
 *	us because that would require getting a write lock on the segment
 *	or virtual page, and we hold at LEAST a read lock already.
 *
 *	Since we don't necessarily have virtual page locks on the
 *	surrounding pages, nothing prevents them from becomming annonymous
 *	as we execute. In fact, the anon array itself can instantiate while
 *	we are executing. However, segvn_pre_page() does follow the correct
 *	locking protocol before loading translations for such pages, so
 *	that any inaccuracies here will do no structural harm.
 */
STATIC int
segvn_kluster(struct seg *seg, vaddr_t addr, int delta)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	int pd;
	uint_t page;
	anon_t *ap, *dap;

	ASSERT(svp->svd_lckcnt);
	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((delta & PAGEOFFSET) == 0);

	if (addr + delta < seg->s_base ||
	    addr + delta >= (seg->s_base + seg->s_size))
		return (-1);		/* exceeded segment bounds */

	/*
	 * delta may be negative or positive. Divide by PAGESIZE to retain
	 * sign bit.
	 */

	pd = delta / PAGESIZE;

	page = seg_page(seg, addr);

	if (svp->svd_type == MAP_SHARED)
		return (0);		/* shared mapping - all ok */

	if (svp->svd_amp == NULL)
		return (0);		/* off original vnode */

	page += svp->svd_anon_index;
	ap = svp->svd_amp->am_anon[page];
	dap = svp->svd_amp->am_anon[page + pd];

	if (ap) {
		/*
		 *+ A call was made to segvn_kluster for advice during a
		 *+ pagefault and the base page of the request was found
	 	 *+ to be anonymous. Since we are currently NOT calling
		 *+ SOP_KLUSTER() functions via anon_getpage/VOP_GETPAGELIST
		 *+ we should NOT be here now. This is not a recoverable
		 *+ error and probably indicates an implementation error
		 *+ in the SOP_FAULT function of a segment manager since
		 *+ it should NOT be calling VOP_GETPAGE on any anon-backed
		 *+ range of pages.
		 *+
		 *+ If we decide to kluster in from swap devices, other
		 *+ functionality will need to be provided and probably not
		 *+ via the SOP_KLUSTER mechanism.
		 */
		cmn_err(CE_PANIC, "segvn_kluster on anon page");
	}

	/*
	 * If delta page is anon, return failure.
	 */

	if (dap)
		return (-1);

	/*
	 * Both pages non-anon in same segment.
	 */

	return (0);
}

/*
 * STATIC int
 * segvn_sync(struct seg *seg, vaddr_t addr, uint_t len, int attr, uint_t flags)
 *    Push any pages mapped by this segment (starting at `addr' for `len'
 *    bytes) to backing storage. If (flags & MS_ASYNC) then we do not wait
 *    for any I/Os to complete before returning. If (flags & MS_INVALIDATE)
 *    then pages are purged from the cache after being cleaned. The segment
 *    must match the specifications in the argument `attr' for any pushes to
 *    be done. Note that `attr' may be zero.
 *
 * Calling/Exit State:
 *    Called with the AS WRITE locked and returns the same.
 *
 * Description:
 *    Since the AS is WRITE locked, no additional synchronization is
 *    necessary to serialize against faults or F_SOFTLOCKs (in the
 *    MS_INVALIDATE case).
 *
 *    On success zero is returned and all pages currently mapped by this
 *    segment starting at `addr' for btop(`len') pages will have been
 *    pushed to backing storage. The pages may or may not actually exist in
 *    mainstore at the time of the request and any existing pages may or may
 *    not be dirty. Only existing modifed pages are pushed with the underlying
 *    filesystem handling the details. Note that if pages are anon and no
 *    swap files have been added yet or no swap space is currently available,
 *    the pages will not be pushed out. See anon_putpage for details.
 *
 *    Note that pages need not be currently loaded (no translation) in this
 *    address space, merely mapped (contained within a segment) by it.
 *
 *    If the pages were synchronously pushed and (flags & MS_INVALIDATE),
 *    then any translations (i.e. visible mappings established by this or
 *    any other AS) for the pages will have also been unloaded by the time
 *    we return. If we were directed to asynchronously push the pages,
 *    translations will be unloaded when the I/O completes later.
 *
 *    Note that zero (success) is also returned in the case where the
 *    this segment does not match the attributes passed in the argument
 *    `attr.'
 *
 *    On failure, a non-zero errno is returned to the caller. Only a
 *    error generated by the underlying filesystem can cause this to
 *    occur.
 *
 * Remarks:
 *   In order to maintain conformance with SVID and POSIX, this
 *   function returns EBUSY if it encounters any memory locked pages (via
 *   plock(2), memcntl(2), or shmctl(2)) in the segment and the caller
 *   requests an invalidation after the I/O.
 *
 *   As permitted by POSIX, no operation is performed for MAP_PRIVATE and
 *   shared memory segments.
 */
STATIC int
segvn_sync(struct seg *seg, vaddr_t addr, uint_t len, int attr, uint_t flags)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	vpage_info_t *vpinfop;
	vnode_t *vp;
	uint_t off;
	vaddr_t eaddr;
	int bflags;
	int err = 0;
	int pageprot;
	cred_t *credp;
	boolean_t check_prot = B_FALSE;
	boolean_t check_locked = B_FALSE;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);

	/*
	 * There is nothing for us to do for a MAP_PRIVATE segment, or for
	 * a segment mapping a vp with a no sync policy.
	 */
	if (svp->svd_type == MAP_PRIVATE || 
	    (svp->svd_vp->v_flag & VNOSYNC)) {
		return (0);
	}

	/*
	 * Check the attributes. Also, remember if we need to perform
 	 * per-page protection checking (in the loop below).
	 */

	if (attr) {
		int segtype;

		pageprot = attr & ~(SHARED|PRIVATE);
		segtype = attr & SHARED ? MAP_SHARED : MAP_PRIVATE;

		/*
		 * If the segment types don't match, then we return
		 * `success' without attempting to sync any pages
		 * in the segment.
		 */

		if (svp->svd_type != segtype) {
			return (0);
		}

		/*
		 * Check to see if page protection attributes match. If
		 * no setprot was ever done on the segment, then we can
		 * pass/fail the request here. If setprots were done, we
		 * need to select/reject on a per-vpage basis in the loop
		 * below.
 		 */

		if (!(svp->svd_flags & SEGVN_PGPROT)) {
			if (svp->svd_prot != pageprot) {
				return (0);
			}
		} else {
			check_prot = B_TRUE;
		}
	}

	/*
	 * If doing MS_INVALIDATE, we must return EBUSY for locked pages.
	 * So, check for that condition now.
	 */
	if ((flags & MS_INVALIDATE) && (svp->svd_flags & SEGVN_MEMLCK)) {
		check_locked = B_TRUE;
	}

	/*
	 * Set up for request.
	 *
	 * Note the B_FORCE causes mod times in nodes of filesystem-backed
	 * pages to be updated.
	 */

	off = svp->svd_offset + (addr - seg->s_base);
	bflags = B_FORCE | ((flags & MS_ASYNC) ? B_ASYNC : 0) |
	         ((flags & MS_INVALIDATE) ? B_INVAL : 0);
	vp = svp->svd_vp;
	ASSERT(vp);
	credp = svp->svd_cred;

	/*
	 * If this segment has no locked pages, and we don't need to do any
	 * further protection checking - push the requested pages `en masse.'
	 */

	if (check_prot || check_locked) {
		ASSERT(svp->svd_info);
		vpinfop = &svp->svd_info[seg_page(seg, addr)];
	} else {
		return (VOP_PUTPAGE(vp, off, len, bflags, credp));
	}

	for (eaddr = addr + len; addr < eaddr; addr += PAGESIZE,
					       off += PAGESIZE,
					       ++vpinfop) {
		/*
		 * If protection attribute specified: Does this page meet
		 * the protection criteria specifed?
		 */
		if (check_prot && (vpinfop->vpi_flags & PROT_ALL) != pageprot)
			continue;

		/*
		 * Check for invalidating a locked page. If the page is
		 * still locked, then return EBUSY.
		 */
		if (check_locked && (vpinfop->vpi_flags & VPI_MEMLOCK) &&
		    XLAT_LOADED(seg, addr)) {
			err = EBUSY;
			break;
		}

		/*
		 * Initiate a page write.
		 */
		err = VOP_PUTPAGE(vp, off, PAGESIZE, bflags, credp);
		if (err)
			break;
	}

	return (err);
}

/*
 * #defines for segvn_incore bits
 */

#define SVN_IC_CORE  0x01 /* page exists */
#define SVN_IC_VLOCK 0x02 /* page memory locked */
#define SVN_IC_MLOCK 0x08 /* page has memory locks */
#define SVN_IC_VNODE 0x10 /* segment is vnode (file) backed */
#define SVN_IC_ANON  0x20 /* page is anonymous */

/*
 * STATIC int
 * segvn_incore(struct seg *seg, vaddr_t addr, uint_t len, char *vec)
 * 	Determine if we have pages in the primary storage virtual
 *	memory cache (i.e., "in core") as mapped by this segment starting
 *	at `addr' for `len' bytes. The results are returned in the byte
 *	array `vec.'
 *
 * Calling/Exit State:
 *	Called with AS read locked and returns the same way. Acquires segment
 *	write lock for duration of operation.
 *
 *	Returns its own argument `len' and does not have a notion of success
 *	or failure.
 *
 * Remarks:
 *	The only bit defined to be returned by this function (in the case
 *	of an incore page) is 0x01. The SVR4 `classic' version returns a
 *	whole bunch more bits, probably intended to be used for indirect
 *	testing of other features. These bits are defined as follows:
 *
 *		0x02	- page memory locked by this segment
 *		0x04	- page has `cowcnt' (future) memory locks
 *		0x08	- page has `lckcnt' (current) memory locks
 *		0x10	- segment is vnode (file) backed
 *		0x20	- page is anonymous
 *
 *	For compatibility with who knows what which depends on these bits,
 *	they are retained for the most part under ES/MP. Since memory locking
 *	accounting has shifted from using the page cache (page_t fields
 *	p_cowcnt and p_lckcnt) to using a separate vnode/offset identity hash
 * 	and since any pages locked down are COWed if COWable at the time of
 *	the initial request, 0x04 (cowcnt) is no longer a possible return value.
 *
 *	Also note it is quite possible for vnode-backed page to be memory
 *	locked but NOT incore since it may have been aborted asynchronously
 *	via the filesystem.
 */
STATIC int
segvn_incore(struct seg *seg, vaddr_t addr, uint_t len, char *vec)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	anon_t **app;
	vnode_t *vp;
	off_t offset;
	uint_t p, ep;
	int ret;
	vpage_info_t *vpinfop;
	boolean_t incore;
	uint_t vnodebacked;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);

        /*
         * Just get an effective writer lock on the segment and do our
         * thing knowing no one else can alter state on it.
         */

        (void) LOCK_PLMIN(&svp->svd_seglock);

	if (svp->svd_amp == NULL && svp->svd_vp == NULL) {

		/*
		 * No anon map AND no vnode - can't be anything loaded
		 * in this segment.
	  	 *
		 * Note we count on the fact that our caller passed us a
		 * kernel array and NOT the user's final copy of the incore
		 * summary and that the bzero CANNOT cause a page fault on
		 * anything in this segment.
		 */

		bzero(vec, btop(len));
		UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);
		return (len);
	}

	/*
	 * Convert to write lock and drop spinlock
	 */

	SEGLOCK_WRITE(svp);
	UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);


	p = seg_page(seg, addr);
	ep = seg_page(seg, addr + len);
	vpinfop = (svp->svd_flags & SEGVN_MEMLCK) ? &svp->svd_info[p] : NULL;

	/*
	 * Is this a vnode-backed segment?
	 */

	vnodebacked = svp->svd_vp ? SVN_IC_VNODE : 0;

	for ( ; p < ep; p++, addr += PAGESIZE) {
		ret = vnodebacked;
		if ((svp->svd_amp != NULL) &&
		    (*(app = &svp->svd_amp->am_anon[svp->svd_anon_index
							+ p]) != NULL)) {
			/*
			 * This is an anon page.
			 */

			ret |= SVN_IC_ANON;
			anon_antovp(*app, &vp, &offset);
			incore = page_cache_query(vp, offset);
		} else if (svp->svd_vp) {

			/*
			 * This is a vnode-backed file page.
			 */

			vp = svp->svd_vp;
			offset = svp->svd_offset + (addr - seg->s_base);
			incore = page_cache_query(vp, offset);
		}
		else {
			/*
			 * Non-vnode segment, no anon ID yet.
			 */
			incore = 0;
		}

		/*
		 * Is page in core?
		 */

		if (incore) {

			ret |= SVN_IC_CORE;

			/*
			 * Is this page locked in core by anyone?
			 *
		 	 * Note there is no longer a notion of a COW
			 * (future) lock count as there was in SVR4 `classic'
			 * and that locking information is no longer stashed
			 * in the page struct but in a separate cache
			 * (see: mem/pvn.c).
		 	 */

			if (pvn_memresv_query(vp, offset))
				ret |= SVN_IC_MLOCK;
		}

		/*
		 * Is this page locked by THIS segment?
		 */

		if (vpinfop && ((vpinfop++)->vpi_flags & VPI_MEMLOCK))
			ret |= SVN_IC_VLOCK;
		*vec++ = (char) ret;
	}

	(void) LOCK_PLMIN(&svp->svd_seglock);
	SEGLOCK_UNLOCK(svp);
	UNLOCK_PLMIN(&svp->svd_seglock, PLBASE);

	return (len);
}

/*
 *
 * STATIC int
 * segvn_lockop(struct seg *seg, vaddr_t addr, uint_t len, int attr, int op)
 * 	Lock down (or unlock) pages mapped for the indicated segment (seg)
 *	starting at virtual address addr for len bytes.
 *
 * Calling/Exit State:
 *	Called with the AS write locked and returns in the same manner.
 *	Because the AS is write locked, we have exclusive access to all
 *	segment-specific data without recourse to any segment-specific
 *	locks.
 *
 *	On success, zero is returned and the requested operation (MC_LOCK,
 *	MC_LOCKAS, MC_UNLOCKAS or MC_UNLOCK) will have been performed. Note
 *	that zero is also returned in the case where the attr variable is set
 *	but the current segment does not match the indicated criteria. Success
 *	is also indicated when (op == MC_UNLOCK(AS)) but the
 *	segment is not currently locked.
 *
 *	On failure, a non-zero errno is returned to indicate the failure
 *	mode. If the request was MC_LOCK(AS), some subset of the pages may be
 *	returned in a locked state. COW may have been broken, or backing
 *	store allocated, for still more pages.
 *
 *	If an MC_LOCK request succeeds, then the pages are rendered
 *	immune to all F_PROT and F_INVAL faults, including COWs and backing
 *	store fills (though in some architectures, hat layer faults can
 *	still be taken). In addition, this immunity will not be lost due to
 *	a fork.
 *
 * Remarks:
 *	In the case of error, we do not unlock pages we have locked. This
 *	mimics the behavior of SRV4. However, because we cluster our calls
 *	to segvn_fault(..., F_SOFTLOCK, ...), we may fail a few pages
 *	sooner than was the case in SRV4.
 */
STATIC int
segvn_lockop(struct seg *seg, vaddr_t addr, uint_t len, int attr, int op)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;
	int segtype;
	uchar_t mask, check, gprot, pageprot;
	faultcode_t fc;
	vpage_info_t *vpinfop, *evpinfop, *fvpinfop;
	enum seg_rw frw, rw;
	mresvtyp_t mtype;
	int page;
	vaddr_t faddr;
	size_t fsize;
	anon_t **app;
	vnode_t *vp, *refvp;
	off_t refoffset, offset;

	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);

	/*
	 * Remember segment wide protections, if any.
	 */
	gprot = svp->svd_prot;

	/*
	 * By default, there is no protection check to do.
	 * If an attr is passed in, this may change.
	 */
	mask = check = 0;

	/*
	 * segvn does not need to differentiate betweeen MC_LOCKAS and
	 * MC_LOCk unlike some other drivers.
	 */
	if (op == MC_LOCKAS)
		op = MC_LOCK;
	else if (op == MC_UNLOCKAS)
		op = MC_UNLOCK;
	/*
	 * If set, attr indicates a generic type of segment we have been
 	 * requested to lock down. If the current specimen does not meet
	 * the selection criteria, return, indicating success to our caller.
	 */

	if (attr) {
		pageprot = attr & ~(SHARED|PRIVATE);
		segtype = attr & SHARED ? MAP_SHARED : MAP_PRIVATE;

		/*
		 * We are done if the segment types don't match
		 * or if we have segment level protections and
		 * they don't match.
		 */
		if (svp->svd_type != segtype)
			return(0);
		if (svp->svd_flags & SEGVN_PGPROT) {
			/*
			 * We need to check protections at the page granular
			 * level.
			 */
			mask = PROT_ALL;
			check = pageprot;
		} else {
			if (gprot != pageprot)
				return(0);
			/* protection checking all done */
		}
	}

	/*
	 * First, take care of the easy case (unlocking).
	 */
	if (op == MC_UNLOCK) {
		/*
		 * OPTIMIZATION: if nothing is locked, then there is
		 *		 nothing to do.
		 */
		if (!(svp->svd_flags & SEGVN_MEMLCK)) {
			return (0);
		}

		page = seg_page(seg, addr);
		vpinfop = &svp->svd_info[page];
		evpinfop = vpinfop + btop(len);

		/*
		 * The pvn_memresv type of the locked pages is based upon
		 * whether the segment has swap space reserved (on a segment
		 * wide basis).
		 */
		mtype = M_MEMLOCK(svp);

		/*
		 * Grab a handle on the anon map.
		 */
		if (svp->svd_amp != NULL)
			app = &svp->svd_amp->am_anon[svp->svd_anon_index +
						     page];
		else
			app = NULL;

		/*
		 * Cache these values in local variables.
		 */
		refvp = svp->svd_vp;
		refoffset = svp->svd_offset + (addr - seg->s_base);

		/*
		 * Check vpi_flags for locked pages.
		 */
		check |= VPI_MEMLOCK;
		mask |= VPI_MEMLOCK;

		/*
		 * Now loop over the address range.
		 */
		while (vpinfop < evpinfop) {
			/*
			 * Skip if not memory locked, or if attributes
			 * do not match. This single test determines both
			 * at once.
			 */
			if ((vpinfop->vpi_flags & mask) != check)
				goto next_vpage;

			/*
			 * Clear the indications of memory locking in the
			 * vpage_info structure.  We know that vpi_nmemlck == 1
			 * because the AS write lock inhibits concurrent
			 * SOFTLOCK requests.
			 */
			ASSERT(vpinfop->vpi_nmemlck == 1);
			vpinfop->vpi_nmemlck = 0;
			vpinfop->vpi_flags &= ~VPI_MEMLOCK;

			/*
			 * Get the ID of this page.
			 */
			if (app != NULL && *app != NULL) {
				anon_antovp(*app, &vp, &offset);
			} else {
				ASSERT(refvp != NULL);
				vp = refvp;
				offset = refoffset;
			}

			/*
			 * The page might already have been aborted.
			 * But, in that case, hat_unlock will just pass
			 * over this request without complaint.
			 */
			hat_unlock(seg, addr);

			/*
			 * We need to release the pvn_memresv.  Once again,
			 * no special synchronization is needed, as the
			 * AS write lock guards all.
			 */
			pvn_memunresv(vp, offset, mtype);

next_vpage:
			++vpinfop;
			addr += PAGESIZE;
			refoffset += PAGESIZE;
			if (app)
				++app;
		} /* while */
		return 0;
	} /* MC_UNLOCK */

	/*
	 * Easy stuff behind us. Now take care of the LOCKing case.
	 */
	ASSERT(op == MC_LOCK);

	/*
	 * First, a couple of quick little optimizations, both of which
	 * save time because we hold the AS write lock, and thus do not
	 * need to get the segment write locked.
	 *
	 * OPTIMIZATION 1: if the vpage_info array does not yet exist then
	 *		   make one.
	 *
	 * OPTIMIZATION 2: instantiate the anon array if we are sure
	 *		   that we will need one.
	 */
	if (svp->svd_info == NULL) {
		segvn_vpage_info_alloc(seg);
	}
	if (svp->svd_type == MAP_PRIVATE && svp->svd_amp == NULL &&
	    (svp->svd_vp == NULL || (gprot & PROT_WRITE))) {
		ASSERT(svp->svd_swresv);
		svp->svd_amp = amp_alloc(seg->s_size);
	}
	page = seg_page(seg, addr);
	vpinfop = &svp->svd_info[page];
	evpinfop = vpinfop + btop(len);

	/*
	 * Loop over all pages in the range.
	 *
	 * We attempt to batch the SOFTLOCKing operations together, in order
	 * to optimize segvn_fault processing, especially in an attempt to
	 * take advantage of the klustering benefits of VOP_GETPAGE.
	 */
	while (vpinfop < evpinfop) {
		/*
		 * Skip if already memory locked, or if attributes
		 * do not match. Note that we must call XLAT_LOADED() to
		 * determine if the page has been aborted, and thus is no
		 * longer memory locked.
		 */
		if (((vpinfop->vpi_flags & mask) != check) ||
		    ((vpinfop->vpi_flags & VPI_MEMLOCK) &&
		     XLAT_LOADED(seg, addr))) {
			++vpinfop;
			addr += PAGESIZE;
			continue;
		}

		/*
		 * The scan has now reached the first page to be faulted
		 * in a group. If protections are stored on a per-page basis,
		 * then we must also adjust the SOFTLOCKing operation on a
		 * per-page basis.
		 */

		frw = ((vpinfop->vpi_flags | gprot) & PROT_WRITE) ?
						S_WRITE : S_OTHER;
		faddr = addr;
		fsize = 0;
		fvpinfop = vpinfop;

		/*
		 * Extend the fault group as far as it can go.
		 * Keep extending, until we reach the end of the lock down
		 * range, or until a page does not need to be locked,
		 * or until rw mismatches the rest of the group.
		 */
		do {
			fsize += PAGESIZE;
			addr += PAGESIZE;
			++vpinfop;
			if (vpinfop == evpinfop ||
			    (vpinfop->vpi_flags & mask) != check ||
			    ((vpinfop->vpi_flags & VPI_MEMLOCK) &&
			     XLAT_LOADED(seg, addr)))
				break;
			rw = ((vpinfop->vpi_flags | gprot) & PROT_WRITE) ?
					S_WRITE : S_OTHER;
		} while (frw == rw);

		/*
		 * Now, SOFTLOCK the group of pages into memory.
		 */
		svp->svd_flags |= SEGVN_MLCKIP;
		fc = segvn_fault(seg, faddr, fsize, F_SOFTLOCK, frw);
		svp->svd_flags &= ~SEGVN_MLCKIP;
		if (fc) {
			ASSERT(FC_CODE(fc) == FC_OBJERR);
			return (FC_ERRNO(fc));
		}

		/*
		 * For each SOFTLOCKed page, we need to:
		 *	=> count up vpi_nmemlck (reasons for memory lock)
		 *	=> set the VPI_MEMLOCK flag
		 * in the vpage info structure. At this point, we know
		 * that no other concurrent SOFTLOCKs are held (because
		 * the AS write lock is held). We set vpi_nmemlck to 2
		 * so that it will revert to 1 following the F_SOFTUNLOCK
		 * below.
		 */
		while (fvpinfop < vpinfop) {
			fvpinfop->vpi_nmemlck = 2;
			fvpinfop->vpi_flags |= VPI_MEMLOCK;
			++fvpinfop;
		}

		/*
		 * Remember that the segment now has pages locked.
		 */
		svp->svd_flags |= SEGVN_MEMLCK;

		/*
		 * Now release the SOFTLOCK.
		 */
		(void) segvn_fault(seg, faddr, fsize, F_SOFTUNLOCK, S_OTHER);
	}

	return (0);
}

/*
 * int
 * segvn_memory(struct seg *seg, vaddr_t *basep, u_int *lenp)
 * 	Returns the range contained in this segment.
 *
 * Calling/Exit State:
 *	Caller gurantees that there is no changes in the AS layout.
 *
 *	basep and lenp both serve as an inarg and an outarg.
 *	The caller guarantees that the *basep and *lenp is within 
 *	the range of the segment.
 *
 *	This function does nothing for now. It just checks the validity
 *	of the incoming arguments.
 *
 * Remarks:
 *	This function really need only return the range backed by real
 *	memory. 
 */
/* ARGSUSED */
int
segvn_memory(struct seg *seg, vaddr_t *basep, u_int *lenp)
{
#ifdef DEBUG
	vaddr_t eaddr = *basep + *lenp;
	vaddr_t addr = *basep;
#endif

	ASSERT(seg->s_base < eaddr);
	ASSERT(addr < seg->s_base + seg->s_size);
	ASSERT(addr >= seg->s_base);
	ASSERT((addr + *lenp) <= seg->s_base + seg->s_size);

	return 0;
}

/*
 * STATIC void
 * segvn_vpage_info_alloc(struct seg *seg)
 * 	Create a vpage_info array for this seg if one is not already in place
 *
 * Calling/Exit State:
 *	Called with either the AS write locked, the AS read locked and
 *	the segment write locked, or the segment privately held (i.e. being
 *	created). Returns the same way.
 *
 * Remarks:
 *	We have the segment effectively write locked but we may have lost a
 *	race to instantiate vpages. This is tolerated.
 *
 */
STATIC void
segvn_vpage_info_alloc(struct seg *seg)
{
	struct segvn_data *svp = (struct segvn_data *)seg->s_data;

	/*
	 * If no vpage_info array exists, allocate one.
	 */
	if (svp->svd_info == NULL) {
		svp->svd_info = kmem_zalloc(seg_pages(seg) *
					    sizeof (vpage_info_t), KM_SLEEP);
	}
}

