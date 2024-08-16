/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/seg_dz.c	1.15"
#ident	"$Header: $"

/*
 * Things to do:
 * 	- prefaulting pages
 *	- pre-breaking cow in fork
 * 	- CE_WARN message if future lock fails at fault time.
 *	- protchk manipulation at fault time.
 */

/*
 * The main characteristics of the Deferred Zero driver are:
 *	- management of large, sparse active pages in the segment
 *	- swap reservations are delayed until instatiation time
 *	- manages only anon pages
 *	- hole is filled between the low and high end range of
 *	  active mappings when the whole range is locked down
 *	- holes are filled when a fault extends this range and 
 *	  future locking is enabled.
 * This driver is used for the autogrow stack management.
 * The last 2 properties are motivated by the POSIX requirements for the
 * locking of autogrow stack.
 */

#include <mem/anon.h>
#include <mem/as.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_dz.h>
#include <mem/tuneable.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/mman.h>
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
STATIC faultcode_t segdz_fault(struct seg *seg, vaddr_t addr, uint_t len,
				enum fault_type type, enum seg_rw rw);
STATIC void segdz_badop(void);
STATIC int segdz_dup(struct seg *pseg, struct seg *cseg);
STATIC void segdz_childload(struct seg *pseg, struct seg *cseg);
STATIC int segdz_unmap(struct seg *seg, vaddr_t addr, uint_t len);
STATIC void segdz_free(struct seg *seg);
STATIC int segdz_getprot(struct seg *seg, vaddr_t addr, uint_t *prot);
STATIC int segdz_lockop(struct seg *seg, vaddr_t addr, uint_t len,
			int attr, int op);
STATIC int segdz_incore(struct seg *seg, vaddr_t addr, uint_t len, char *vec);
STATIC int segdz_sync(struct seg *seg, vaddr_t addr, uint_t len,
			int attr, uint_t flags);
STATIC int segdz_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp);
STATIC int segdz_gettype(struct seg *seg, vaddr_t addr);
STATIC off_t segdz_getoffset(struct seg *seg, vaddr_t addr);
STATIC int segdz_setprot(struct seg *seg, vaddr_t addr, uint_t len,
			uint_t prot);
STATIC int segdz_checkprot(struct seg *seg, vaddr_t addr, uint_t prot);
STATIC int segdz_memory(struct seg *seg, vaddr_t *addr, u_int *lenp);

struct seg_ops segdz_ops = {
	segdz_unmap,
	segdz_free,
	segdz_fault,
	segdz_setprot,			/* setprot */
	segdz_checkprot,		/* checkprot */
	(int (*)())segdz_badop,		/* no klustering for anon pages yet */
	segdz_sync,
	segdz_incore,
	segdz_lockop,
	segdz_dup,
	segdz_childload,
	segdz_getprot,
	segdz_getoffset,
	segdz_gettype,
	segdz_getvp,
	(void (*)())NULL,	/* age: MUST BE NULL!!! */
	(boolean_t (*)())NULL,	/* lazy_shootdown */
	segdz_memory
};

#define IS_SEGDZ(seg)	((seg)->s_ops == &segdz_ops)

/*
 *+ Per-segment lock for segvn. Protects all state variables in the segment.
 */
LKINFO_DECL(segdz_seglockinfo, "MS:segdz:sdz_seglock", 0);

/*
 * int
 * segdz_create(struct seg *seg, const void * const argsp)
 *	Public function to instantiate new segment in an AS
 *
 * Calling/Exit State:
 *
 *	No spin locks should be held by the caller since this function can
 *	sleep.
 *
 *	Caller must hold the AS exclusively locked before calling this
 *	function; the AS is returned locked. This is required because
 *	nominatively a new portion of address space is being added to
 *      the AS.
 *
 *	Flags argument passed via the segdz_crargs struct can contain
 *	SEGDZ_CONCAT or SEGDZ_NOCONCAT indicationg if the caller wants
 *	concatenation performed.
 *	The protection for this segment is also specified through the
 *	segdz_crargs struct.
 *	No spin locks are held on entry to this function. It returns the same
 *	way. This function may block.
 *
 * 	This function cannot fail. 0 is returned and the segment structure
 *	opaque data field (seg_data) points to the newly created segment.
 *
 */
int
segdz_create(struct seg *seg, const void * const argsp)
{
	const struct segdz_crargs *a = (const struct segdz_crargs *)argsp;
	segdz_data_t *sdp;
	struct seg *adj_seg;

	ASSERT(seg->s_as != &kas);
	ASSERT((seg->s_base & PAGEOFFSET) == 0);
	ASSERT((seg->s_size & PAGEOFFSET) == 0);

	/*
	 * simplified concatenation. We concatenate only if the SEGDZ_CONCAT
	 * flag is passed. Try to concatenate only if amp->base
	 * will remain unchanged. Otherwise the anon map tree needs to be
	 * reorganized and this is not an easy job. If if the segment
	 * being created is concatenable with the segment pointed to by
	 * its s_next field, then we concat only if the STK_GROWTH_DIR
	 * is HI_TO_LOW. Same way, if the segment being created
	 * is concatenable with the segment pointed to by its s_prev field,
	 * then we concat only if the STK_GROWTH_DIR flag is set LOW_TO_HIGH.
	 */

	if (!(a->flags & SEGDZ_CONCAT))
		goto skip_cncat;

#if (STK_GROWTH_DIR == HI_TO_LOW)
	adj_seg = seg->s_next;
	sdp = adj_seg->s_data; 
	if (seg->s_base + seg->s_size == adj_seg->s_base && 
		IS_SEGDZ(adj_seg) && !(sdp->sdz_flags & SEGDZ_PGPROT) && 
		a->prot == sdp->sdz_prot) {

		adj_seg->s_base = seg->s_base;
		adj_seg->s_size += seg->s_size;
		seg_free(seg);
		return 0;
	}
#else
	adj_seg = seg->s_prev;
	sdp = adj_seg->s_data;

	if (adj_seg->s_base + adj_seg->s_size == seg->s_base && 
		IS_SEGDZ(adj_seg) && !(sdp->sdz_flags & SEGDZ_PGPROT) && 
		a->prot == sdp->sdz_prot) {

		adj_seg->s_size += seg->s_base;
		seg_free(seg);
		return 0;
	}
#endif

skip_cncat:

	/*
	 * no concatenation possible with existing segment. Create segment
	 * private structure.
	 */
	sdp = kmem_zalloc(sizeof(segdz_data_t), KM_SLEEP);
	sdp->sdz_amp.am_root = kmem_zalloc(sizeof(segdz_leaf_t), KM_SLEEP);

	seg->s_data = sdp;
	seg->s_ops = &segdz_ops;

	RWSLEEP_INIT(&sdp->sdz_seglock, 0, &segdz_seglockinfo, KM_SLEEP);
	sdp->sdz_prot = a->prot;

#if (STK_GROWTH_DIR == HI_TO_LOW)
	sdp->sdz_flags = SEGDZ_HI_TO_LOW;
#else
	sdp->sdz_flags = SEGDZ_LOW_TO_HI;
#endif
	sdp->sdz_amp.am_off_range = LEAF_PARTITION;
	return 0;
}

/*
 * STATIC anon_t **
 * segdz_findnode(segdz_data_t *sdz, off_t off, vpginfo_t **vinfo,
 *		segdz_leaf_t **nleaf)
 *	Find the leaf node in the anon map for a given addr.
 *
 * Calling/Exit State:
 *	Caller must hold the segment sleep lock at least in shared mode
 *	to stabilize the anon map.
 *
 *	NULL is returned if the anon_t has not been instantiated.
 *	Else, anon pointer for the vpage is returned.
 * 
 *	If no leaf has been instantiated, then NULL is returned in the
 *	outarg *vinfo. Otherwise, the vpage info pointer is returned in
 *	*vinfo.
 *
 *	If a leaf has been instantiated, and nleaf is non-NULL, then
 *	the leaf pointer is returned in the outarg *nleaf. Else, *nleaf
 *	is unchanged.
 */
STATIC anon_t **
segdz_findnode(segdz_data_t *sdp, off_t off, vpginfo_t **vinfo,
		segdz_leaf_t **nleaf)
{
	segdz_amp_t *amp = &sdp->sdz_amp;
	segdz_node_t *node;
	segdz_leaf_t *leaf;
	int shift, index;
	off_t offset;
	anon_t **app;

	offset = off >> PAGESHIFT;
	/*
	 * offset beyond the range of the tree
	 */
	if (offset >= amp->am_off_range) {
		*vinfo = NULL;

		return NULL;
	}

	node = amp->am_root;
	ASSERT(node != NULL);

	offset >>= LEAF_PARTITION_SHIFT;

	if (amp->am_level > 0) {
		shift = (amp->am_level - 1) * SEGDZ_LVL_SHIFT;
		do {
			node += (offset >> shift) & SEGDZ_LVL_MASK;
			ASSERT(node != NULL);
			if (node->next == (segdz_node_t *)NULL) {
				if (nleaf != NULL)
					*nleaf = NULL;
				return NULL;
			}
			node = node->next;
			shift -= SEGDZ_LVL_SHIFT;
		} while (shift >= 0);
	}

	leaf = (segdz_leaf_t *)node;
	ASSERT(leaf->am_anon != NULL);

	if (nleaf)
		*nleaf = leaf;

	index = (off >> PAGESHIFT) & LEAF_PARTITION_MASK;

	app = leaf->am_anon + index;

	*vinfo = leaf->vpi_info + index;

	if (*app) {
		return app;
	} else
		return NULL;
}

/*
 * STATIC anon_t **
 * segdz_insnode(segdz_data_t *sdp, off_t off, page_t **pp,
 *		vpginfo_t **vpginfo, segdz_leaf_t **nleaf, mresvtyp_t mtype,
 *		int flags)
 *
 *	Install this address in the anon map.
 *
 * Calling/Exit State:
 *	Caller must hold the segment sleep lock in exclusive mode
 *	since the anon map will be changed.
 *
 *	The anon struct created for this address is returned.
 *
 *	If the flags argument is SEGDZ_NONAON, then no anon page is allocated.
 * 	Only the leaf node is inserted into the tree. In this case, the caller
 *	does not check the return value and only looks at the nleaf outarg and
 *	the vpginfo outarg.
 *
 * Otherwise the anon page allocated is returned read locked.
 *	No spin locks may be held by this function since it could block.
 */
STATIC anon_t **
segdz_insnode(segdz_data_t *sdp, off_t off, page_t **ppp,
		vpginfo_t **vpginfo, segdz_leaf_t **nleaf, mresvtyp_t mtype,
		int flags)
{
	segdz_amp_t *amp = &sdp->sdz_amp;
	segdz_node_t *node;
	segdz_leaf_t *leaf;
	int index, level;
	off_t offset;
	anon_t **app;

	node = amp->am_root;
	ASSERT(node != NULL);

	offset = off >> PAGESHIFT;
	/*
	 * need to grow vertically?
	 */
	while (offset >= amp->am_off_range) {
		ASSERT(amp->am_level < SEGDZ_LEVELS);
		node = kmem_zalloc(SEGDZ_NUMNODES * sizeof(segdz_node_t), 
				KM_SLEEP);
		node->next = amp->am_root;
		amp->am_root = node;
		amp->am_level++;
		if (amp->am_level == SEGDZ_LEVELS)
			amp->am_off_range = OFF_MAX;
		else
			amp->am_off_range = 1 << ((SEGDZ_LVL_SHIFT * amp->am_level) +
				LEAF_PARTITION_SHIFT);
	}

	/* we should be set with the right level of the tree now. */

	offset >>= LEAF_PARTITION_SHIFT;

	node = amp->am_root;
	level = amp->am_level;

	/*
	 * Allocate neccessary levels for the tree.
	 */
	while (level > 0) {
		index = (offset >> (level - 1) * SEGDZ_LVL_SHIFT) & SEGDZ_LVL_MASK;
		if ((node + index)->next == (segdz_node_t *)NULL) {
			if (level != 1) {
				(node + index)->next = kmem_zalloc(SEGDZ_NUMNODES * 
				sizeof(segdz_node_t), KM_SLEEP);
			} else {
				if ((node + index)->leaf == NULL) {
					(node + index)->leaf = 
					      kmem_zalloc(sizeof(segdz_leaf_t),
								KM_SLEEP);
				}
			}
		}
		node = (node + index)->next;
		level--;
	}

	index = (off >> PAGESHIFT) & LEAF_PARTITION_MASK;

	leaf = (segdz_leaf_t *)node;
	app = leaf->am_anon + index;

	ASSERT(nleaf != NULL);

	if (flags & SEGDZ_NOANON)
		*nleaf = leaf;
	else {
		ASSERT(ppp != NULL);
		ASSERT(vpginfo != NULL);

		*ppp = anon_zero(app, mtype);

		ASSERT(*ppp != NULL);
		ASSERT(PAGE_IS_RDLOCKED(*ppp));
		*vpginfo = leaf->vpi_info + index;
		if (nleaf)
			*nleaf = leaf;
	}
	return app;
}

/*
 * STATIC vaddr_t
 * segdz_amp_findrange(segdz_amp_t *amp, off_t *lowoff, off_t *hioff)
 *	Find the lowest and highest instantiated offset in the anon map.
 *
 * Calling/Exit State:
 *	The caller stabilizes the anon map through some means.
 *
 *	The lowest instantiated offset is placed in outarg lowoff and the 
 *	highest instantiated offset in the segment is placed in outarg hioff.
 */
STATIC void
segdz_amp_findrange(segdz_amp_t *amp, off_t *lowoff, off_t *hioff)
{
	segdz_node_t *node;
	int i = 0, tval, k;
	segdz_leaf_t *leaf;
	segdz_stack_t stack;
	boolean_t found = B_FALSE, search_low = B_TRUE;
	int level, numnodes;

	ASSERT(amp->am_root != NULL);
start:
	node = amp->am_root;
	stack.tos = stack.amp_index = 0;

	level = amp->am_level;
	ASSERT(level <= SEGDZ_LEVELS);
	if (search_low) {
		i = 0;
		tval = SEGDZ_NUMNODES;
	} else {
		i = (amp->am_level == 0) ? 0 : SEGDZ_NUMNODES - 1;
		tval = -1;
	}

	numnodes = (amp->am_level == 0) ? 1 : SEGDZ_NUMNODES;

	while ((i < numnodes) && (i >= 0)) {

		if (amp->am_level > 0) {
			if ((node + i)->next == (segdz_node_t *)NULL)
				goto next_node;
		}

		if (level > 1) {
			SEGDZ_PUSH(stack, node);
			ASSERT(node != NULL);
			level--;
			stack.amp_index |= (i << (level * SEGDZ_LVL_SHIFT));
			node = (node + i)->next;
			ASSERT(node != NULL);
			i = (search_low ? 0 : SEGDZ_NUMNODES - 1);
			continue;
		}

		if (level == 0)
			leaf = (segdz_leaf_t *)node;
		else {
			ASSERT((node + i)->leaf);
			leaf = (node + i)->leaf;
			stack.amp_index |= i;
		}
		for (k = 0; k < LEAF_PARTITION; k++) {
			if (search_low) {
				if (leaf->am_anon[k] != NULL) {
					found = B_TRUE;
					stack.amp_index = (stack.amp_index << 
						      LEAF_PARTITION_SHIFT) | k;
					break;
				}
			} else {
				if ((leaf->am_anon[LEAF_PARTITION - 1 - k]) !=
					NULL) {
					found = B_TRUE;
					stack.amp_index = (stack.amp_index << 
						LEAF_PARTITION_SHIFT) | 
						(LEAF_PARTITION - 1 - k);
					break;
				}
			}
		}
		if (found || amp->am_level == 0)
			break;
next_node:
		if (search_low)
			i++;
		else
			i--;
		stack.amp_index &= ~SEGDZ_LVL_MASK;
		while ((i == tval) && (level != amp->am_level)) {
			ASSERT(level > 0);
			SEGDZ_POP(stack, node);
			ASSERT(node != NULL);
			i = ((stack.amp_index >> (level * SEGDZ_LVL_SHIFT)) & 
				SEGDZ_LVL_MASK);
			if (search_low)
				i++;
			else
				i--;
			ASSERT(i <= SEGDZ_NUMNODES);
			stack.amp_index &= ~(SEGDZ_LVL_MASK << ((level - 1) * 
					SEGDZ_LVL_SHIFT));
			level++;
		}
	}	/* while (i < SEGDZ_NUMNODES) */

	if (search_low) {
		*lowoff = stack.amp_index << PAGESHIFT;
		search_low = B_FALSE;
		found = B_FALSE;
		goto start;
	} else 
		*hioff = stack.amp_index << PAGESHIFT;
	return;
}

/*
 * boolean_t
 * segdz_setprot_range(struct seg *seg, off_t offset, vaddr_t *start_addr,
 *			boolean_t doshoot)
 *	Change translation protections in the range from start_addr upto
 *	but NOT including the address obtained from the offset.
 *
 * Calling/Exit State:
 *	Appropriate mutexes for having the anon map of the segment 
 *	stable should be held.
 */
STATIC boolean_t
segdz_setprot_range(struct seg *seg, off_t offset, vaddr_t *start_addr,
			boolean_t doshoot)
{
	vaddr_t curaddr, baddr;
	ulong_t len;
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;

	curaddr = SEGDZ_OFF_TO_ADDR(seg, offset);
	baddr = (sdp->sdz_flags & SEGDZ_HI_TO_LOW) ? 
		curaddr + PAGESIZE : *start_addr;

	len = (sdp->sdz_flags & SEGDZ_HI_TO_LOW) ? 
			*start_addr - curaddr : curaddr - baddr;

	*start_addr = (sdp->sdz_flags & SEGDZ_HI_TO_LOW) ? 
			curaddr - PAGESIZE : curaddr + PAGESIZE;

	if (len != 0)
		return(hat_chgprot(seg, baddr, len, ~PROT_WRITE, doshoot));
	else
		return B_FALSE;
}

/*
 * boolean_t
 * segdz_break_cow(struct seg *seg, anon_t *old_ap, page_t *opp, 
 *		anon_t **new_ap, vaddr_t addr, u_char vprot,
 *		boolean_t opage_lock, boolean_t npage_lock)
 *
 *	Break cow for the page pointed to by new_ap.
 *
 * Calling/Exit State:
 *	opp points to the anon page pointed to by old_ap. It is
 *	expected to be read locked.
 *	If npage_lock is B_TRUE, then the page's translation is locked.
 *	If opage_lock is B_TRUE, then the existing translation is unloaded
 *	with the HAT_UNLOCK flag.
 *	The translations are loaded with vprot protection.
 */
STATIC int
segdz_break_cow(struct seg *seg, anon_t *old_ap, page_t *opp, anon_t **new_ap,
		vaddr_t addr, u_char vprot, boolean_t opage_lock,
		boolean_t npage_lock)
{
	page_t *pp;
	vnode_t *vp;
	off_t off;


	ASSERT(PAGE_IS_RDLOCKED(opp));

	if (npage_lock && !mem_resv(1, M_REAL_USER)) {
		page_unlock(opp);
		return EAGAIN;
	}

	if (old_ap == *new_ap) {
		if (opage_lock)
			hat_unload(seg, addr, PAGESIZE, HAT_UNLOCK);
		else
			hat_unload(seg, addr, PAGESIZE, HAT_NOFLAGS);
	}

	if (opage_lock) {
		anon_antovp(old_ap, &vp, &off);
		pvn_memunresv(vp, off, M_REAL_USER);
	}

	pp = anon_private(new_ap, opp, npage_lock ? M_REAL_USER : M_NONE);

	ASSERT(pp != opp);
	ASSERT(PAGE_IS_RDLOCKED(pp));

	page_unlock(opp);
	anon_decref(old_ap);

	if (npage_lock)
		hat_memload(seg, addr, pp, vprot, HAT_LOCK);

	page_unlock(pp);

	return 0;
}

/*
 * int
 * segdz_amp_dup(struct seg *pseg, segdz_amp_t *ch_amp, struct as *cas)
 *
 *	Dup the anon map of the parent for the child.
 *
 * Calling/Exit State:
 *	The parent segment's sleep lock is held in shared mode to
 *	stabilize it anon map. 
 *
 *	Returns 0 on seccess.
 *	Returns error on failure (only if anon_getpage fails).
 *
 * Description:
 * 	We break COW on all pages that are memory locked by the parent.
 *	All the other parent segment's pages are changes to read only.
 *	One hat_chgprot() call is issued for a range of pages between
 *	the memory locked pages.
 */
STATIC int
segdz_amp_dup(struct seg *pseg, segdz_amp_t *ch_amp, struct as *cas)
{
	segdz_data_t *sdp = (segdz_data_t *)pseg->s_data;
	segdz_amp_t *p_amp =&sdp->sdz_amp;
	segdz_node_t *pnode, *cnode = 0, *node = 0;
	int i = 0, level, err, index, j, numnodes;
	segdz_stack_t pstack, cstack;
	vaddr_t start_addr;
	boolean_t doshoot= B_FALSE;
	segdz_leaf_t *p_leaf, *c_leaf;
	off_t offset;
	anon_t *old_ap;
	uint_t prot;
	page_t *anon_pl[1+1], *opp;

	pstack.tos = cstack.tos = pstack.amp_index = 0;
	pnode = p_amp->am_root;
	level = p_amp->am_level;
	start_addr = SEGDZ_OFF_TO_ADDR(pseg, 0);

	ASSERT(level <= SEGDZ_LEVELS);
	ASSERT(ch_amp != NULL);
	ASSERT(ch_amp->am_root == (segdz_node_t *)NULL);
	ASSERT(ch_amp->am_swresv == 0);

	numnodes = (p_amp->am_level == 0) ? 1 : SEGDZ_NUMNODES;

	while (i < numnodes) {
		ASSERT(level >= 0);

		/*
		 * if we are not processing a leaf node.
		 */
		if (level > 0) {
			if ((pnode + i)->next == (segdz_node_t *)NULL)
				goto next_node;
		}

		if (cnode == NULL) {
			if (p_amp->am_level == 0) {
				cnode = kmem_zalloc(sizeof(segdz_leaf_t),
							KM_SLEEP);
				ASSERT(level == p_amp->am_level);
				ch_amp->am_root = cnode;
			} else {
				cnode = kmem_zalloc(SEGDZ_NUMNODES * 
						sizeof(segdz_node_t), KM_SLEEP);
				if (level == p_amp->am_level)
					ch_amp->am_root = cnode;
			}
		}
		if (level != p_amp->am_level) {
			SEGDZ_STACK_TOP(cstack, node);
			ASSERT(node);
			index = (pstack.amp_index >> (level * SEGDZ_LVL_SHIFT)) & 
				SEGDZ_LVL_MASK;
			(node + index)->next = cnode;
		}

		if (level > 1) {
			SEGDZ_PUSH(pstack, pnode);
			SEGDZ_PUSH(cstack, cnode);
			ASSERT(pnode != NULL);
			ASSERT(cnode != NULL);
			level--;
			pstack.amp_index |= (i << (level * SEGDZ_LVL_SHIFT));
			pnode = (pnode + i)->next;
			cnode = (cnode + i)->next;
			ASSERT(pnode != NULL);
			i = 0;
			continue;
		}

		if (p_amp->am_level == 0) {
			p_leaf = (segdz_leaf_t *)pnode;
			c_leaf = (segdz_leaf_t *)ch_amp->am_root;
		} else {
			ASSERT((pnode + i)->leaf != NULL);
			p_leaf = (pnode + i)->leaf;
			pstack.amp_index |= i;
			c_leaf = (cnode + i)->leaf = 
				kmem_zalloc(sizeof(segdz_leaf_t), KM_SLEEP);
		}

		anon_dup(p_leaf->am_anon, c_leaf->am_anon, 
			LEAF_PARTITION * PAGESIZE);

		offset = pstack.amp_index << LEAF_PARTITION_SHIFT + 
						PAGESHIFT;

		for (j = 0; j < LEAF_PARTITION; j++) {
			if (p_leaf->am_anon[j]) {
				ch_amp->am_swresv++;
				cas->a_isize += PAGESIZE;

				/* If the parent's page is memory locked,
				 * then we need to break COW. Change prot to
				 * read-only on pages collected so far. We
				 * don't break COW however, if the segment or 
				 * page has read-only permission, since COW 
				 * will be broken when and if setprot write- 
				 * enable is done on this page.
				 */
				if (p_leaf->vpi_info[j].vpi_nmemlck && 
					VPAGE_WRITABLE(sdp,
					      p_leaf->vpi_info[j].vpi_flags)) {

					doshoot |= segdz_setprot_range(pseg,
						offset,	&start_addr, B_FALSE);

					old_ap = p_leaf->am_anon[j];
					err = anon_getpage(&old_ap, &prot,
						anon_pl, PAGESIZE, S_WRITE,
						NULL);

					if (err) {
						ASSERT(anon_pl[0] == NULL);
						return err;
					}
					opp = anon_pl[0];
					ASSERT(PAGE_IS_RDLOCKED(opp));

					(void)segdz_break_cow(pseg,
						p_leaf->am_anon[j], opp,
						&c_leaf->am_anon[j], 0, 0,
						B_FALSE, B_FALSE);
				}  /* need to break COW? */
			} /* parent's anon exists? */
			offset += PAGESIZE;
		}  /* for loop */

		if (sdp->sdz_flags & SEGDZ_PGPROT) {
			for (j = 0; j < LEAF_PARTITION; j++)
				c_leaf->vpi_info[j].vpi_flags |=
				    p_leaf->vpi_info[j].vpi_flags & PROT_ALL;
		}
		if (p_amp->am_level == 0)
			break;
next_node:
		pstack.amp_index &= ~SEGDZ_LVL_MASK;
		i++;
		while ((i == SEGDZ_NUMNODES) && (level != p_amp->am_level)) {
			ASSERT(level > 0);
			SEGDZ_POP(pstack, pnode);
			SEGDZ_POP(cstack, cnode);
			ASSERT(pnode != NULL);
			ASSERT(cnode != NULL);
			i = ((pstack.amp_index >> (level * SEGDZ_LVL_SHIFT)) &
				SEGDZ_LVL_MASK);
			i++;
			ASSERT(i <= SEGDZ_NUMNODES);
			pstack.amp_index &= ~(SEGDZ_LVL_MASK << 
						(level * SEGDZ_LVL_SHIFT));
			level++;
		}
	}	/* while (i < SEGDZ_NUMNODES) */

	ASSERT(start_addr <= (pseg->s_base + pseg->s_size));

	/* 
	 * Call hat_chgprot() for the rest of the pages.
	 */
	if ((start_addr < (pseg->s_base + pseg->s_size)) &&
		(start_addr >= (pseg->s_base))) {

		offset = ((sdp->sdz_flags & SEGDZ_HI_TO_LOW) ? 
			 SEGDZ_ADDR_TO_OFF(pseg, pseg->s_base - PAGESIZE) :
			 SEGDZ_ADDR_TO_OFF(pseg, pseg->s_base + pseg->s_size));

		doshoot |= segdz_setprot_range(pseg, offset, &start_addr,
						B_FALSE);
	}

	if (doshoot)
		hat_uas_shootdown(pseg->s_as);

	return 0;
}

/*
 * void
 * segdz_xfer_init(segdz_amp_t *old_amp, segdz_args_t *args)
 *
 *	Setup routine used for setting up the root node for the new
 *	anon map.
 *
 * Calling/Exit State:
 *	None. Only this context has a hold on this new anon map and thus
 *	there is no need for any mutexes to be held.
 */
STATIC void
segdz_xfer_init(segdz_amp_t *old_amp, segdz_args_t *args)
{
	segdz_amp_t *new_amp = &((segdz_data_t *)(args->xferargs.nseg->s_data))->sdz_amp;

	ASSERT(old_amp->am_root != NULL);

	ASSERT(new_amp != NULL);
	ASSERT(new_amp->am_root == (segdz_node_t *)NULL);

	*(args->xferargs.swresv) = 0;

	if (old_amp->am_level == 0) {
		new_amp->am_root = kmem_zalloc(sizeof(segdz_leaf_t), KM_SLEEP);
		new_amp->am_level = 0;
		new_amp->am_off_range = LEAF_PARTITION;
	} else {
		new_amp->am_root = kmem_zalloc(SEGDZ_NUMNODES * sizeof(segdz_node_t),
						KM_SLEEP);
		new_amp->am_level = 1;
		new_amp->am_off_range = 1 << (SEGDZ_LVL_SHIFT + 
						LEAF_PARTITION_SHIFT);
	}
}

/*
 * void
 * segdz_xfer_leaf(segdz_args_t *args, off_t curoff, off_t soff, off_t eoff,
 *		int leaf_index, segdz_leaf_t *leaf)
 *
 *	Dup all the anon nodes in the leaf and set up the new anon map to
 *	point to these anons.
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map that is being
 *	copied by either holding the segment lock or by holding the AS lock.
 *
 * Description:
 *	The number of nodes copied from the old node to the new node
 *	(which is same as the number of swap reservations transferred from the
 *	old anon map to the new one) is recorde in parameter swresv.
 */
STATIC void
segdz_xfer_leaf(segdz_args_t *args, off_t curoff, off_t soff, off_t eoff,
		int leaf_index, segdz_leaf_t *leaf)
{
	segdz_data_t *nsdp = (segdz_data_t *)args->xferargs.nseg->s_data;
	segdz_leaf_t *new_leaf;
	int nleaf_index, j = leaf_index;
	ulong_t len;
	boolean_t loop_again;
	vpginfo_t *vinfo, *new_vinfo;
	struct as *as = args->xferargs.as;

	/* 
	 * The following loop should be executed maximum 2 times
	 * in one go around.
	 */
	do {
		segdz_insnode(nsdp, curoff - soff, NULL, NULL, &new_leaf,
				0, SEGDZ_NOANON);

		nleaf_index = ((curoff - soff) >> PAGESHIFT) & 
				LEAF_PARTITION_MASK;

		len = (min((LEAF_PARTITION - j),
				(LEAF_PARTITION - nleaf_index))) << PAGESHIFT;


		loop_again = B_FALSE;
		if (len < ((LEAF_PARTITION - j) << PAGESHIFT))
			loop_again = B_TRUE;

		if ((curoff + len) >= eoff)
			len = eoff - curoff;

		*args->xferargs.swresv += anon_dup(leaf->am_anon + j,
				    new_leaf->am_anon + nleaf_index, len);

		as->a_isize += ptob(len);

		vinfo = leaf->vpi_info + j;
		new_vinfo = new_leaf->vpi_info + nleaf_index;
		bcopy(vinfo, new_vinfo, btop(len) * sizeof(vpginfo_t));

		if (loop_again) {
			curoff += len;
			j = (curoff >> PAGESHIFT) & LEAF_PARTITION_MASK;
		}
	} while (loop_again);
}

/*
 * void
 * segdz_unlock_leaf(struct seg *seg, segdz_leaf_t *leaf, int leaf_index,
 *		off_t curoff, off_t eoff)
 *
 * 	Unlock the pages in this leaf node in the given range.
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map that is being
 *	unlocked by either holding the segment lock or by holding the AS lock.
 *
 */
STATIC void
segdz_unlock_leaf(struct seg *seg, segdz_leaf_t *leaf, int leaf_index,
		off_t curoff, off_t eoff)
{
	vpginfo_t *vinfo;
	vnode_t *vp;
	off_t offset;
	int j = leaf_index;

	vinfo = &leaf->vpi_info[j];
	for (; j < LEAF_PARTITION; j++, vinfo++) {
		if (curoff >= eoff)
			return;

		if (!(vinfo->vpi_flags & VPI_MEMLOCK)) {
			curoff += PAGESIZE;
			continue;
		}

		ASSERT(leaf->am_anon[j] != NULL);

		anon_antovp(leaf->am_anon[j], &vp, &offset);
		ASSERT(vp);

		vinfo->vpi_flags &= ~VPI_MEMLOCK;
		ASSERT(vinfo->vpi_nmemlck);

		if ((--vinfo->vpi_nmemlck) == 0) {
			hat_unlock(seg, SEGDZ_OFF_TO_ADDR(seg, curoff));
			pvn_memunresv(vp, offset, M_REAL_USER);
		}
		curoff += PAGESIZE;
	}
}

/*
 * int
 * segdz_lock_leaf(struct seg *seg, off_t *nextoffset, off_t curoff, 
 *	off_t eoff,s egdz_leaf_t *leaf, int leaf_index, segdz_args_t *args)
 *
 * 	Lock the pages in this leaf node in the given range.
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map that is being
 *	locked by either holding the segment lock or by holding the AS lock.
 *
 *	On success, 0 is returned.
 *	On failure, error is returned. Failure cases are when mem_resv for
 *	M_REAL or M_SWAP fails.
 *
 * Remarks:
 *      For POSIX compliance, we make sure there are no holes in the range we
 *      are locking down. We do this by calling segdz_fault for the holes.
 *      If op is MC_LOCKAS, then all the holes between the highest and the
 *      lowest  instantiated addresses are filled.
 */
STATIC int
segdz_lock_leaf(struct seg *seg, off_t *nextoffset, off_t curoff, off_t eoff,
		segdz_leaf_t *leaf, int leaf_index, segdz_args_t *args)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	vaddr_t curaddr, nextaddr, baddr;
	ulong_t len;
	off_t nextoff = *nextoffset, offset;
	int err = 0, j = leaf_index;
	page_t *opp, *anon_pl[1 + 1];
 	u_char vprot, mask = args->lockargs.mask, check = args->lockargs.check;
 	anon_t *ap;
	vnode_t *vp;
	u_int prot;

	/* 
	 * If there are holes in the middle, call
	 * segdz_fault() for the range and lock those
	 * pages down.
	 */
	ASSERT(nextoff <= curoff);

	if (nextoff != curoff) {
		curaddr = SEGDZ_OFF_TO_ADDR(seg, curoff);
		nextaddr = SEGDZ_OFF_TO_ADDR(seg, nextoff);
		baddr = (sdp->sdz_flags & SEGDZ_HI_TO_LOW) ?
			curaddr + PAGESIZE : nextaddr;

		len = (sdp->sdz_flags & SEGDZ_HI_TO_LOW) ?
			(nextaddr - curaddr) : (curaddr - baddr);

		vprot = sdp->sdz_flags & SEGDZ_PGPROT ? 
			leaf->vpi_info[j].vpi_flags & PROT_ALL : sdp->sdz_prot;

		err = segdz_fault(seg, baddr, len, F_INVAL, S_OTHER);
		if (err != 0)
			return err;
	}

	nextoff = curoff;

	for (; j < LEAF_PARTITION; j++) {

		if (curoff >= eoff) {
			*nextoffset = curoff;
			return 0;
		}

		if (leaf->vpi_info[j].vpi_flags & VPI_MEMLOCK) {
			curoff += PAGESIZE;
			continue;
		}

		/* 
		 * if page level protection is set, need to 
		 * check if the caller's attributes match. If
		 * not, skip.
		 */
		vprot = sdp->sdz_flags & SEGDZ_PGPROT ? 
			leaf->vpi_info[j].vpi_flags & PROT_ALL : sdp->sdz_prot;

		if (sdp->sdz_flags & SEGDZ_PGPROT) {
			if ((vprot & mask) != 0 && (vprot != check)) {
				curoff += PAGESIZE;
				continue;
			}
		}

		if ((ap = leaf->am_anon[j]) == NULL) {
			segdz_fault(seg, SEGDZ_OFF_TO_ADDR(seg, curoff),
					PAGESIZE, F_INVAL, S_OTHER);
			curoff += PAGESIZE;
			continue;
		}


		curaddr = SEGDZ_OFF_TO_ADDR(seg, curoff);

		err = anon_getpage(leaf->am_anon + j, &prot, anon_pl, PAGESIZE,
				S_WRITE, NULL);
		if (err) {
			ASSERT(anon_pl[0] == NULL);
			return err;
		}
                opp = anon_pl[0];
                ASSERT(PAGE_IS_RDLOCKED(opp));

		/*
		 * We need to break COW if this anon is shared
		 * and the vpage is writable.
		 */
		if (((prot & PROT_WRITE) == 0) && VPAGE_WRITABLE(sdp, 
						leaf->vpi_info[j].vpi_flags)) {

			err = segdz_break_cow(seg, leaf->am_anon[j], opp,
					leaf->am_anon + j, curaddr,
					vprot, B_FALSE, B_TRUE);

		} else {	/* non cow breaking case */
			anon_antovp(ap, &vp, &offset);
			ASSERT(vp);

			/* if not already reserved */
			if (leaf->vpi_info[j].vpi_nmemlck == 0) {
				if (!pvn_memresv(vp, offset, M_REAL_USER,
						SLEEP)) {
					page_unlock(opp);
					return EAGAIN;
				}
			}
			hat_memload(seg, curaddr, opp, vprot, HAT_LOCK);
			page_unlock(opp);
		}

		leaf->vpi_info[j].vpi_flags |= VPI_MEMLOCK;
		leaf->vpi_info[j].vpi_nmemlck++;
		sdp->sdz_flags |= SEGDZ_MEMLCK;
		curoff += PAGESIZE;
	}

	*nextoffset = curoff;
	return 0;
}

/*
 * int
 * segdz_lock_endrange(struct seg *seg, off_t nextoff, off_t eoff)
 *
 * 	Lock down the non-instantiated pages if any that could be at the
 *	end of the range (only in the case of MC_LOCK).
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map that is being
 *	locked by either holding the segment lock or by holding the AS lock.
 *
 *	Returns 0 if successful.
 *	Returns error if M_REAL or M_SWAP reservations fail in segdz_fault().
 *
 */
STATIC int
segdz_lock_endrange(struct seg *seg, off_t nextoff, off_t eoff)
{
	segdz_data_t	*sdp = (segdz_data_t *)seg->s_data;
	
	if (nextoff < eoff) {
		vaddr_t baddr;

		baddr = (sdp->sdz_flags & SEGDZ_HI_TO_LOW) ?
			SEGDZ_OFF_TO_ADDR(seg, eoff - PAGESIZE) :
			SEGDZ_OFF_TO_ADDR(seg, nextoff + PAGESIZE);

		return segdz_fault(seg, baddr, eoff - nextoff, F_INVAL,
					S_OTHER);
	}
	return 0;
}

/*
 * void
 * segdz_setprot_init(struct seg *seg, u_long len, off_t *soff, off_t *eoff,
 *		segdz_setprot_args_t *protargs)
 *
 *	Setup routine for setprot. 
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map 
 *	byeither holding the segment lock or by holding the AS lock.
 *
 * Description:
 *	If the page level protections are enabled for the first time
 *	in this segment, then the starting offset and the ending offset
 *	is changed to cover the entire anon map in order to install
 *	vpage protections for instantiated pages in this segment.
 *
 */
STATIC void
segdz_setprot_init(struct seg *seg, u_long len, off_t *soff, off_t *eoff,
			segdz_setprot_args_t *protargs)
{
	segdz_data_t	*sdp = (segdz_data_t *)seg->s_data;

	protargs->start_addr = SEGDZ_OFF_TO_ADDR(seg, *soff);

	/* need to install page protections for the first time? */ 
	if ((len != seg->s_size) && !(sdp->sdz_flags & SEGDZ_PGPROT)) {
		protargs->srange = *soff;
		protargs->erange = *eoff;
		protargs->install = B_TRUE;
		*eoff = SEGDZ_HI_OFFSET(seg, seg->s_base, seg->s_len) + PAGESIZE; 
		*soff = 0;
	} else
		protargs->install = B_FALSE;
}

/*
 * void
 * segdz_setprot_leaf(struct seg *seg, off_t *nextoff, off_t curoff, 
 *		off_t eoff, segdz_leaf_t *leaf, int leaf_index,
 *		segdz_setprot_args_t *protargs)
 *
 *	Set new protections for all nodes in this leaf that are in the
 *	requested range.
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map 
 *	by either holding the segment lock or by holding the AS lock.
 *
 *	Returns 0 on success.
 *	Returns error in case of M_REAL reservation failure or 
 *	anon_getpage() failure.
 *
 * Description:
 *      We break COW on pages that are shared but we are enabling write
 *      permissions for the page and the page is memory locked. If the page
 *      was originally memory locked and it is this context that memory locked
 *      it, then we transfer the memory locking to the new page.
 */
STATIC int
segdz_setprot_leaf(struct seg *seg, off_t *nextoff, off_t curoff, off_t eoff,
		segdz_leaf_t *leaf, int leaf_index, 
		segdz_setprot_args_t *protargs)
{
    segdz_data_t	*sdp = (segdz_data_t *)seg->s_data;
    segdz_leaf_t	*nleaf;
    int			j = leaf_index, k, err;
    page_t		*opp, *anon_pl[1+1];
    uint_t		curprot;
    uchar_t		vpi_flags;
    anon_t		*ap;
    struct as		*as = seg->s_as;

    /* 
     * need to instantiate vpages ?
     */
    while (curoff > *nextoff) {
	segdz_insnode(sdp, *nextoff, NULL, NULL, &nleaf, 0, SEGDZ_NOANON);

	ASSERT(nleaf != NULL);

	for (k = (*nextoff >> PAGESHIFT) & LEAF_PARTITION_MASK;
				k < LEAF_PARTITION; k++) {
		if (protargs->install && ((*nextoff < protargs->srange) || 
				(*nextoff >= protargs->erange))) 
			nleaf->vpi_info[k].vpi_flags |= sdp->sdz_prot;
		else
			nleaf->vpi_info[k].vpi_flags |= protargs->prot;
		*nextoff += PAGESIZE;
		as->a_isize += PAGESIZE;
	}
    }

    ASSERT(*nextoff == curoff);

    for (; j < LEAF_PARTITION; j++) {

	if (curoff >= eoff) {
	    *nextoff = curoff;
	    return 0;
	}

	/*
	 * need to instantiate the vpages if setting
	 * prot and the vpage is out of the range.
	 */
	if (protargs->install && ((curoff < protargs->srange) || 
				(curoff >= protargs->erange))) {

	    leaf->vpi_info[j].vpi_flags &= ~PROT_ALL;
	    leaf->vpi_info[j].vpi_flags |= sdp->sdz_prot;
	    curoff += PAGESIZE;
	    continue;
	}

	if (leaf->am_anon[j] == NULL) {
	    leaf->vpi_info[j].vpi_flags &= ~PROT_ALL;
	    leaf->vpi_info[j].vpi_flags |= protargs->prot;
	    curoff += PAGESIZE;
	    continue;
	}

	ap = leaf->am_anon[j];
	err = anon_getpage(&ap, &curprot, anon_pl, PAGESIZE, S_WRITE, NULL);
	if (err) {
	    ASSERT(anon_pl[0] == NULL);
	    hat_uas_shootdown(seg->s_as);
	    return err;
	}

	opp = anon_pl[0];
	ASSERT(PAGE_IS_RDLOCKED(opp));

	vpi_flags = leaf->vpi_info[j].vpi_flags;
	/* 
	 * Need to break COW? We need to break COW if we are enabling
	 * write permissions and if the page is memory locked.
	 */
	if ((protargs->prot & PROT_WRITE) && (curprot & PROT_WRITE) == 0) {

	    if (!(vpi_flags & VPI_MEMLOCK)) {
		/* 
		 * enable write permissions on all the page so
		 * far.
		 */
		(void)segdz_setprot_range(seg, curoff, &protargs->start_addr,
					B_TRUE);
		page_unlock(opp);
		leaf->vpi_info[j].vpi_flags = 
				(vpi_flags & ~PROT_ALL) | protargs->prot;

		curoff += PAGESIZE;
		continue;
	    }

	    /* 
	     * We need to transfer the memory lock to the new page
	     * if we are the one who has memory locked this page.
	     * We pvn_memresv for this page before pvn_memunresving
	     * for the old page temporarily holding this page doubly
	     * reserved.
	     */
	    ASSERT(VPAGE_RDONLY(sdp, vpi_flags));

	    /* Break COW */

	    err = segdz_break_cow(seg, leaf->am_anon[j], opp, 
				  leaf->am_anon + j, 
				  SEGDZ_OFF_TO_ADDR(seg, curoff), 
				  protargs->prot, B_TRUE, B_TRUE);

	    if (err) {
		hat_uas_shootdown(seg->s_as);
		return err;
	    }

	} else		/* write permission not turned on */
	    page_unlock(opp);

	leaf->vpi_info[j].vpi_flags &= ~PROT_ALL;
	leaf->vpi_info[j].vpi_flags |= protargs->prot;

	curoff += PAGESIZE;
    }   /* for (; j < LEAF_PARTITION; j++) */

    *nextoff = curoff;
    return 0;
}


/*
 * void
 * segdz_setprot_endrange(struct seg *seg, off_t eoff, off_t nextoff,
 *			segdz_setprot_args_t *protargs)
 *
 *	Instantiate vpage protections at the end of the requested range
 *	if neccessary and change protections at the translations layer
 *	if enabling write protections.
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map 
 *	by either holding the segment lock or by holding the AS lock.
 *
 *	Returns 0 on success.
 *	Returns error in case of M_REAL reservation failure or 
 *	anon_getpage() failure.
 *
 * Description:
 *	The nextoff argument denotes the next offset to be processed.
 *	If this is less than the end of the requested range, then this
 *	this mean that the pages between nextoff and eoff are not
 *	instantiated in the anon map.
 *	The startaddr field in the protargs arguments denotes the starting
 *	address in the range to have protection changed.
 *
 */
STATIC void
segdz_setprot_endrange(struct seg *seg, off_t eoff, off_t nextoff,
			segdz_setprot_args_t *protargs)
{
	segdz_data_t	*sdp = (segdz_data_t *)seg->s_data;
	segdz_leaf_t	*nleaf;
	vaddr_t		eaddr, baddr;
	ulong_t		len;
	int		k;
	struct as	*as = seg->s_as;

	/* 
	 * need to instantiate vpages at end of the range?
	 */
	while (eoff > nextoff) {
		segdz_insnode(sdp, nextoff, NULL, NULL, &nleaf, 0, 
				SEGDZ_NOANON);
		for (k = (nextoff >> PAGESHIFT) & LEAF_PARTITION_MASK;
			k < LEAF_PARTITION; k++) {

			if (protargs->install && 
			    ((nextoff < protargs->srange) ||
			     (nextoff >= protargs->erange))) {

				nleaf->vpi_info[k].vpi_flags &= ~PROT_ALL;
				nleaf->vpi_info[k].vpi_flags |= sdp->sdz_prot;
			} else
				nleaf->vpi_info[k].vpi_flags |= protargs->prot;

			ASSERT(nextoff < eoff);
			nextoff += PAGESIZE;
			as->a_isize += PAGESIZE;

			if (nextoff == eoff)
				break;
		}
	}

	/*
	 * chgprot any pages at the end of the range when enabling write
	 * permissions.
	 */
	if (protargs->prot & PROT_WRITE) {
		eaddr = SEGDZ_OFF_TO_ADDR(seg, eoff - PAGESIZE);
		baddr = (sdp->sdz_flags & SEGDZ_HI_TO_LOW) ? 
				eaddr : protargs->start_addr;

		len = ((sdp->sdz_flags & SEGDZ_HI_TO_LOW) ? 
			(protargs->start_addr - eaddr) : 
			(eaddr - baddr));

		len += PAGESIZE;

		if (len != 0)
			hat_chgprot(seg, baddr, len, protargs->prot, B_TRUE);
	}
}

/*
 * void
 * segdz_release_leaf(int leaf_index, off_t curoff, off_t eoff, 
 *		segdz_args_t *args, segdz_leaf_t *leaf, ulong_t *swfree_cnt,
 *		segdz_node_t *node, int index)
 *
 *	Free the anon nodess and swap reservations for all the nodes in 
 *	this leaf in the requested range.
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map 
 *	by either holding the segment lock or by holding the AS lock.
 *
 * Description:
 *      The caller sets skip_swfree to some non-zero value to skip freeing
 *      of swap space for that many pages. The only case where this
 *	count is non-zero is when part of this anon map is transferred
 *	to a new segment (segdz_xfer_leaf()) and so conceptually, the 
 *	swap reservations is just transferred to the new anon map.
 */
STATIC void
segdz_release_leaf(int leaf_index, off_t curoff, off_t eoff, 
		segdz_args_t *args, segdz_leaf_t *leaf, ulong_t *swfree_cnt,
		segdz_node_t *node, int index)
{
	segdz_amp_t *amp = args->relargs.amp;
	int j = leaf_index;
	struct as *as = args->relargs.as;

	for (; j < LEAF_PARTITION; j++) {
		if (curoff >= eoff)
			break;

		if (leaf->am_anon[j] != NULL) {
			*swfree_cnt += 1;
			/*
			 * skip unreserving swap space until we
			 * exceed skip_swfree
			 */
			if (*swfree_cnt > args->relargs.skip_swfree)
				mem_unresv(1, M_SWAP);
			ASSERT(amp->am_swresv);
			amp->am_swresv--;
			as->a_isize -= PAGESIZE;
			ASSERT((leaf->vpi_info[j].vpi_nmemlck == 0) ||
				args->relargs.skip_swfree);
			ASSERT((!(leaf->vpi_info[j].vpi_flags & 
				VPI_MEMLOCK)) || args->relargs.skip_swfree);
		}
		curoff += PAGESIZE;
	}

	anon_free(&leaf->am_anon[leaf_index], (j - leaf_index) * PAGESIZE);

	if (amp->am_level == 0)
		return;

	if (leaf_index == 0 && j == LEAF_PARTITION) {
		kmem_free(leaf, sizeof(segdz_leaf_t));
		(node + index)->leaf = 0;
	}
}

/*
 * void
 * segdz_release_root(struct seg *seg, ulong_t len, segdz_args_t *args)
 *
 *	Free the root of the anon map.
 *
 * Calling/Exit State:
 *	The caller gurantees the stability of the anon map 
 *	by either holding the segment lock or by holding the AS lock.
 *
 */
STATIC void
segdz_release_root(struct seg *seg, ulong_t len, segdz_args_t *args)
{
	segdz_amp_t *amp = args->relargs.amp;

	/*
	 * Delete am_root only if deleting the whole segment
	 */
	if (len == seg->s_size) {
		if (amp->am_level == 0)
			kmem_free(amp->am_root, sizeof(segdz_leaf_t));
		else
			kmem_free(amp->am_root,
				SEGDZ_NUMNODES * sizeof(segdz_node_t));
		amp->am_root = 0;
	} else
		ASSERT(amp->am_root != NULL);
}

#define SFU_IC_CORE  0x01 /* page exists */
#define SFU_IC_VLOCK 0x02 /* page memory locked */
#define SFU_IC_MLOCK 0x08 /* page has memory locks */
#define SFU_IC_ANON  0x20 /* page is anonymous */

/*
 * STATIC void
 * segdz_incore_leaf(struct seg *seg, int leaf_index, off_t curoff, off_t eoff,
 *		segdz_leaf_t *leaf, vaddr_t addr, segdz_args_t *args)
 *
 *	Scans all the nodes in this leaf node for the specified region
 *	to return the memory residency info. of the virtual pages.
 *
 * Calling/Exit State:
 *	Locks to stabilize the anon map need to be held by the caller.
 *
 * Remarks:
 *	The only bit defined to be returned by this function (in the case
 *	of an incore page) is 0x01. The SVR4.0 `classic' segvn driver returns a
 *	whole bunch more bits, probably intended to be used for indirect
 *	testing of other features. These bits are defined as follows:
 *
 *		0x02	- page memory locked by this segment
 *		0x04	- page has `cowcnt' (future) memory locks
 *		0x08	- page has `lckcnt' (current) memory locks
 *		0x20	- page is anonymous
 *
 *	These flags should be consistent with the segvn return values.
 *	For compatibility with who knows what which depends on these bits,
 *	they are retained for the most part under ES/MP. Since memory locking
 *	accounting has shifted from using the page cache (page_t fields
 *	p_cowcnt and p_lckcnt) to using a separate vnode/offset identity hash
 * 	and since any pages locked down are COWed if COWable at the time of
 *	the initial request, 0x04 (cowcnt) is no longer a possible return 
 *	value. 
 *
 */
STATIC void
segdz_incore_leaf(struct seg *seg, int leaf_index, off_t curoff, off_t eoff,
		segdz_leaf_t *leaf, vaddr_t addr, segdz_args_t *args)
{
	int j = leaf_index;
	unsigned char flags;
	vnode_t *vp;
	off_t offset;
	vaddr_t curaddr;

	for (; j < LEAF_PARTITION; j++) {
		if (curoff >= eoff)
			return;

		if (leaf->am_anon[j] != NULL) {
			flags = SFU_IC_ANON;
			anon_antovp(leaf->am_anon[j], &vp, &offset);
			ASSERT(vp);
			if (page_cache_query(vp, offset)) {
				flags |= SFU_IC_CORE;

			if (pvn_memresv_query(vp, offset))
				flags |= SFU_IC_MLOCK;
				if (leaf->vpi_info[j].vpi_flags	 & VPI_MEMLOCK)
					flags |= SFU_IC_VLOCK;
			}

			curaddr = SEGDZ_OFF_TO_ADDR(seg, curoff);
			*(args->incore_vec + btop(curaddr - addr)) = flags;
		}
		curoff += PAGESIZE;
	}
}

/*
 * STATIC int
 * segdz_amp_traverse(struct seg *seg, vaddr_t addr, u_long len, u_char op,
 *			segdz_args_t *args)
 *
 * 	Common anon map traversal function that walks the anon map and
 *	calls the appropriate function depending on the operation
 *	requested.
 *
 * Calling/Exit State:
 *	Locks to stabilize the anon map need to be held by the caller.
 *
 * Description:
 *	The operations for which this functions is called for are:
 *	LOCKAS, UNLOCK, LOCK, INCORE, SETPROT, RELEASE and XFER.
 *	The inarg args is used by the callers to pass the needed information.
 * 	In addition, this function uses an argument structure in the
 *	case of SETPROT that is used to pass information between the 
 *	different functions used for the setprot operation..
 *
 */
STATIC int
segdz_amp_traverse(struct seg *seg, vaddr_t addr, u_long len, u_char op,
			segdz_args_t *args)
{
	segdz_data_t	*sdp = (segdz_data_t *)seg->s_data;
	segdz_amp_t	*amp = &sdp->sdz_amp;
	off_t		soff, eoff, curoff, nextoff;
	segdz_node_t	*node = amp->am_root;
	segdz_stack_t	stack;
	int		node_index = 0, level = amp->am_level, leaf_index;
	int		numnodes, err;
	segdz_setprot_args_t	protargs;
	boolean_t	first_pass = B_TRUE;
	ulong_t		swfree_cnt = 0;
	segdz_leaf_t 	*leaf;

	ASSERT(len != 0);
	ASSERT(amp->am_root != NULL);

	ASSERT(op == SEGDZ_LOCKOP || op == SEGDZ_SETPROT || 
		op == SEGDZ_INCORE || op == SEGDZ_LOCKASOP || 
		op == SEGDZ_RELEASE || op == SEGDZ_UNLOCKOP || 
		op == SEGDZ_XFER);

	soff = SEGDZ_LOW_OFFSET(seg, addr, len);
	eoff = SEGDZ_HI_OFFSET(seg, addr, len);

	ASSERT(eoff >= soff);

	eoff += PAGESIZE;

	switch (op)
	{
		case SEGDZ_SETPROT:
			protargs.prot = args->prot;
			segdz_setprot_init(seg, len, &soff, &eoff, &protargs);
			break;
		case SEGDZ_XFER:
			segdz_xfer_init(amp, args);
			break;
		default:
			break;
	}

	ASSERT(eoff > soff);
	nextoff = curoff = soff;

	if (soff >= (amp->am_off_range << PAGESHIFT))
		goto done;

	numnodes = (amp->am_level == 0) ? 1 : SEGDZ_NUMNODES;

	stack.tos = 0;
	stack.amp_index = 0;

	if (level > 0)
		node_index = SEGDZ_OFF_TO_INDEX(soff, level);

	leaf_index = SEGDZ_OFF_TO_INDEX(soff, 0);

	ASSERT(node_index < SEGDZ_NUMNODES);

	while (node_index < numnodes) {
		ASSERT(level <= SEGDZ_LEVELS);

		/* 
		 * if we are not processing a leaf node.
		 */
		if (amp->am_level > 0) {
			if ((node + node_index)->next == (segdz_node_t *)NULL) {
				if (level > 1)
					first_pass = B_FALSE;
				goto next_node;
			}
		}
		if (level > 1) {
			ASSERT(node != NULL);
			SEGDZ_PUSH(stack, node);
			node = (node + node_index)->next;
			ASSERT(node != NULL);
			level--;
			stack.amp_index |= (node_index << (level * SEGDZ_LVL_SHIFT));
			if (first_pass)
				node_index = SEGDZ_OFF_TO_INDEX(soff, level);
			else
				node_index = 0;
			continue;
		}

		ASSERT(node != NULL);
		if (level == 0)
			leaf = (segdz_leaf_t *)node;
		else {
			ASSERT(level == 1);
			ASSERT((node + node_index)->leaf);
			leaf = (node + node_index)->leaf;
		}

		stack.amp_index |= node_index;

		curoff = (((stack.amp_index << LEAF_PARTITION_SHIFT) | 
				leaf_index) << PAGESHIFT);

		if (curoff >= eoff)
			goto done;

		switch (op) {

		case SEGDZ_LOCKOP:
		case SEGDZ_LOCKASOP:
			err = segdz_lock_leaf(seg, &nextoff, curoff, eoff,
						leaf, leaf_index, args);
			if (err)
				return err;
			break;
		case SEGDZ_UNLOCKOP:
			segdz_unlock_leaf(seg, leaf, leaf_index, curoff, eoff);
			break;
		case SEGDZ_SETPROT:
			err = segdz_setprot_leaf(seg, &nextoff, curoff, eoff,
					leaf, leaf_index, &protargs);
			if (err)
				return err;
			break;
		case SEGDZ_RELEASE:
			segdz_release_leaf(leaf_index, curoff, eoff, args,
					leaf, &swfree_cnt, node, node_index);
			break;
		case SEGDZ_INCORE:
			segdz_incore_leaf(seg, leaf_index, curoff, eoff, leaf,
					addr, args);
			break;
		case SEGDZ_XFER:
			segdz_xfer_leaf(args, curoff, soff, eoff,
					leaf_index, leaf);
			break;
		}
		stack.amp_index &= ~SEGDZ_LVL_MASK;
next_node:
		leaf_index = 0;
		node_index++;
		while ((node_index == SEGDZ_NUMNODES) && 
				(level != amp->am_level)) {
			ASSERT(level > 0);
			if (op == SEGDZ_RELEASE)
				SEGDZ_RELEASE_NODE(seg, len, node);

			SEGDZ_POP(stack, node);
			ASSERT(node);
			node_index = 
			     ((stack.amp_index >> (level * SEGDZ_LVL_SHIFT)) &
				SEGDZ_LVL_MASK);
			if (op == SEGDZ_RELEASE && len == seg->s_size)
				(node + node_index)->next = 0;

			node_index++;
			ASSERT(node_index <= SEGDZ_NUMNODES);
			stack.amp_index &= ~(SEGDZ_LVL_MASK << (level * 
						SEGDZ_LVL_SHIFT));
			level++;
			first_pass = B_FALSE;
		}
	} /* while (node_index < numnodes) */
done:
	err = 0;

	switch (op)
	{
		case SEGDZ_LOCKOP:
			err = segdz_lock_endrange(seg, nextoff, eoff);
			break;
		case SEGDZ_SETPROT:
			segdz_setprot_endrange(seg, eoff, nextoff, &protargs);
			break;
		case SEGDZ_RELEASE:
			segdz_release_root(seg, len, args);
			break;
		default:
			break;
	}

	return err;
}


/*
 * STATIC faultcode_t
 * segdz_faultpage(struct seg *seg, vaddr_t addr, enum fault_type type,
 *		uint_t protchk, enum seg_rw rw)
 *
 * 	Handles all the dirty work of getting the right anonymous page and
 *	loading up the translation.
 *
 * Calling/Exit State:
 *	The AS is READ locked and the segment is WRITE locked. The Writer
 *	is held since we may change the anon map in this function.
 *
 * 	On success 0 is returned and the request has been satisfied. On
 *	failure a non-zero fault code is returned.
 *
 *
 * Description:
 *
 *	This functions takes care of special processing for F_SOFTLOCK,
 *		  F_MAXPROT_SOFTLOCK, and F_SOFTUNLOCK cases.
 *
 *	Sets sfd_hiaddr and sfd_lowaddr if this faults extends the range in
 *	either way.
 *
 * 	The basic algorithm here is:
 * 		If this is an anon_zero case
 *			allocate anon_map and make a swap reservation,
 *			as necessary
 *			If this page is being locked, call memresv
 *			Call anon_zero to allocate page
 *			Load up translation
 *			Return
 *		endif
 *		If this is an anon_getpage case 
 *			Use anon_getpage to get the page
 *			If not a COW
 *				If this page is being locked, call pvn_memresv
 *				Load up the translation to the page
 *				Return
 *			endif
 *			If this page is being locked, call pvn_memresv
 *			Call anon_private to handle COW
 *			Manage lock counts
 *		endif
 *
 *
 * Remarks:
 *	If future lock flag is set, we try to lock the page. If we could
 *	not get memory reservations the we load an unlocked page. This is
 *	the required behaviour under POSIX.
 *	If it is a F_MAXPROT_SOFTLOCK fault, then we can never fail due to
 *	protection since maxprot for this segment is PROT_ALL.
 */
STATIC faultcode_t
segdz_faultpage(struct seg *seg, vaddr_t addr, enum fault_type type,
		uint_t protchk, enum seg_rw rw)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	mresvtyp_t mtype = M_NONE;
	page_t *anon_pl[1 + 1], *pp, *opp = NULL;    /* original object page */
	uint_t prot;
	int err;
	enum anaction { ANON_ZERO, ANON_GETPAGE };
	enum anaction action;
	anon_t **app, *old_ap;
	vnode_t *vp;
	off_t offset = SEGDZ_ADDR_TO_OFF(seg, addr), vp_off;
	boolean_t resv_failed = B_FALSE;
	vpginfo_t *vinfo;
	segdz_amp_t *amp = &sdp->sdz_amp;
	segdz_leaf_t *leaf;
#ifdef DEBUG
	segdz_leaf_t *nleaf;
	vpginfo_t *dbg_vinfo;
#endif
	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());

	if ((addr < seg->s_base) || (addr >= seg->s_base + seg->s_size))
		return FC_NOMAP;

	/*
	 * If segdz_findnode() return NULL, then it is an ANON_ZERO case
	 * (we need to instantiate the page). Else it is ANON_GETPAGE.
	 */
	app = segdz_findnode(sdp, offset, &vinfo, NULL);

	/*
	 * check for protection
	 */
	if ((sdp->sdz_flags & SEGDZ_PGPROT) && type != F_MAXPROT_SOFTLOCK &&
	    rw != S_OTHER && !(sdp->sdz_flags & SEGDZ_MLCKFLT) &&
	    !(((vinfo != NULL) ? vinfo->vpi_flags : sdp->sdz_prot) & protchk))
		return FC_PROT;

	if (app != NULL) {
		/*
		 * page has already been instantiated by anon_zero()
		 */
		action = ANON_GETPAGE;
		old_ap = *app;
	} else {
		if (!mem_resv(1, M_SWAP))
			return ENOMEM;

		amp->am_swresv++;
		action = ANON_ZERO;
	}

	if (type == F_MAXPROT_SOFTLOCK)
		type = F_SOFTLOCK;

	ASSERT((action == ANON_ZERO) || (action == ANON_GETPAGE));

	switch (action) {
	case ANON_ZERO:
		ASSERT(app == NULL);
		if ((type == F_SOFTLOCK) || (sdp->sdz_flags & SEGDZ_MLCKFLT) ||
			seg->s_as->a_paglck) {
			if (!(mem_resv(1, 
				(type == F_SOFTLOCK) ? M_REAL : 
				M_REAL_USER))) {
				/* it's ok to fail if future lock only */
				if ((type == F_SOFTLOCK) || 
					(sdp->sdz_flags & SEGDZ_MLCKFLT)) {
					mem_unresv(1, M_SWAP);
					amp->am_swresv--;
					return FC_MAKE_ERR(EAGAIN);
				} else
					resv_failed = B_TRUE;
			} else {
				mtype = ((type == F_SOFTLOCK) ? M_REAL : 
						M_REAL_USER);
			}
		}

		seg->s_as->a_isize += PAGESIZE;

		/* Insert the virtual page in the anon map. */
		app = segdz_insnode(sdp, offset, &pp, &vinfo, &leaf, mtype,
					SEGDZ_NOFLAGS);
		ASSERT(*app != NULL);
		ASSERT(app == segdz_findnode(sdp, offset, &dbg_vinfo, &nleaf));
		ASSERT(vinfo == dbg_vinfo);
		ASSERT(leaf == nleaf);

		ASSERT(pp);
		ASSERT(PAGE_IS_RDLOCKED(pp));

		if (!(sdp->sdz_flags & SEGDZ_PGPROT))
			prot = sdp->sdz_prot;
		else {
			prot = vinfo->vpi_flags & PROT_ALL;
			if (!prot) {
				prot = PROT_ALL;
				vinfo->vpi_flags |= PROT_ALL;
			}
		}
		if (type == F_SOFTLOCK) {
			hat_memload(seg, addr, pp, prot, HAT_LOCK);
			vinfo->vpi_nmemlck++;
			if (seg->s_as->a_paglck && !resv_failed) {
				vinfo->vpi_flags |= VPI_MEMLOCK;
				vinfo->vpi_nmemlck++;
			}
		} else {
			if ((sdp->sdz_flags & SEGDZ_MLCKFLT) ||
				seg->s_as->a_paglck && !resv_failed) {

				sdp->sdz_flags |= SEGDZ_MEMLCK;
				vinfo->vpi_flags |= VPI_MEMLOCK;
				hat_memload(seg, addr, pp, prot, HAT_LOCK);
				vinfo->vpi_nmemlck++;
			} else
				hat_memload(seg, addr, pp, prot, HAT_NOFLAGS);

			page_unlock(pp);
		}
		return 0;

	case ANON_GETPAGE:
		/*
		 * Obtain the page structure via anon_getpage() if it is
		 * a private copy of an object (the result of a previous
		 * copy-on-write).
		 */
		ASSERT(*app != NULL);

		err = anon_getpage(app, &prot, anon_pl, PAGESIZE, rw, NULL);
		if (err) {
			ASSERT(anon_pl[0] == NULL);
			return FC_MAKE_ERR(err);
		}
		opp = anon_pl[0];
		ASSERT(PAGE_IS_RDLOCKED(opp));
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
	 * object page is write protected.  We assume that fatal
	 * protection checks have already been made.
	 *
	 * If not a copy-on-write case load the translation
	 * and return.
	 */
	if ((rw != S_WRITE) || (prot & PROT_WRITE) != 0) {

		if (!(sdp->sdz_flags & SEGDZ_PGPROT))
			prot = sdp->sdz_prot & prot;
		else
			prot = vinfo->vpi_flags & prot;

		if (type == F_SOFTLOCK || seg->s_as->a_paglck || 
			sdp->sdz_flags & SEGDZ_MLCKFLT) { 
			if (vinfo->vpi_nmemlck == 0) {
				anon_antovp(old_ap, &vp, &vp_off);
				if (!pvn_memresv(vp, vp_off, 
					(type == F_SOFTLOCK) ? M_REAL : 
					M_REAL_USER, SLEEP)) {
					if ((type == F_SOFTLOCK) ||
					    (sdp->sdz_flags & SEGDZ_MLCKFLT)) {
						page_unlock(opp);
						return FC_MAKE_ERR(EAGAIN);
					}
				} else {
					mtype = ((type == F_SOFTLOCK) ? 
						M_REAL : M_REAL_USER);
				}
			}

			hat_memload(seg, addr, opp, prot, 
			    ((mtype != M_NONE) ? HAT_LOCK : HAT_NOFLAGS));
			if (type == F_SOFTLOCK)
				vinfo->vpi_nmemlck++;
			else {
				if (mtype == M_REAL_USER) {
					vinfo->vpi_nmemlck++;
					vinfo->vpi_flags |= VPI_MEMLOCK;
					sdp->sdz_flags |= SEGDZ_MEMLCK;
				}
				page_unlock(opp);
			}
		} else {  /* no locks held at this point */
			hat_memload(seg, addr, opp, prot, HAT_NOFLAGS);
			page_unlock(opp);
		}
		return 0;
	}

	/* COW case */
	amp->am_cowcnt++;

	ASSERT(amp->am_swresv != 0);

	if (type == F_SOFTLOCK || seg->s_as->a_paglck || 
			(sdp->sdz_flags & SEGDZ_MLCKFLT)) {
		if (vinfo->vpi_nmemlck == 0) {
			if (!mem_resv(1, (type == F_SOFTLOCK) ? M_REAL : 
					M_REAL_USER)) {
				if (type == F_SOFTLOCK || 
					(sdp->sdz_flags & SEGDZ_MLCKFLT)) {
					page_unlock(opp);
					return FC_MAKE_ERR(EAGAIN);
				}
			} else {
				mtype = (type == F_SOFTLOCK) ? M_REAL : 
					M_REAL_USER;
			}
		}
	}

	pp = anon_private(app, opp, mtype);

	ASSERT(pp != opp);
	ASSERT(PAGE_IS_RDLOCKED(pp));

	hat_unload(seg, addr, PAGESIZE, HAT_NOFLAGS);
	page_unlock(opp);

	ASSERT(old_ap);
	anon_decref(old_ap);

	if (!(sdp->sdz_flags & SEGDZ_PGPROT))
		prot = sdp->sdz_prot;
	else
		prot = vinfo->vpi_flags & PROT_ALL;

	if (mtype == M_REAL_USER) {

		sdp->sdz_flags |= SEGDZ_MEMLCK;
		if (!(vinfo->vpi_flags & VPI_MEMLOCK)) {
			vinfo->vpi_nmemlck++;
			vinfo->vpi_flags |= VPI_MEMLOCK;
		}
		hat_memload(seg, addr, pp, prot, HAT_LOCK);
		if (type == F_SOFTLOCK) {
			vinfo->vpi_nmemlck++;
		} else
			page_unlock(pp);
	} else if (type == F_SOFTLOCK) {
		vinfo->vpi_nmemlck++;
		hat_memload(seg, addr, pp, prot, HAT_LOCK);
	} else {
		hat_memload(seg, addr, pp, prot, HAT_NOFLAGS);
		page_unlock(pp);
	}
	return 0;
}

/*
 * STATIC void
 * segdz_softunlock(struct seg *seg, vaddr_t addr, u_int len, enum seg_rw rw)
 *  	Do a F_SOFTUNLOCK over the range requested.  The range must have
 *	already been subjected to an F_SOFTLOCKed request.
 *
 * Calling/Exit State:
 *	Invoked from segdz_fault, the AS must be locked, the segment must
 *	be write locked.
 *
 *
 * Remarks:
 *	For any given page in a given segment there will be no more than
 *	one call to pvn_memresv || pvn_cache_memresv made before calling
 *	pvn_memunresv. The call is made when the first SOFTLOCK / MEMLOCK is
 *	established.
 */
STATIC void
segdz_softunlock(struct seg *seg, vaddr_t addr, u_int len, enum seg_rw rw)
{
	vnode_t *vp;
	off_t offset;
	anon_t **app;
	vpginfo_t *vinfo;
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;

	ASSERT(getpl() == PLBASE);
        ASSERT(KS_HOLD0LOCKS());

	while (len) {
		app = segdz_findnode(sdp, SEGDZ_ADDR_TO_OFF(seg, addr), 
				&vinfo, NULL);
		ASSERT(app != NULL);

		anon_antovp(*app, &vp, &offset);
		ASSERT(vp);
		page_find_unlock(vp, offset, ((rw == S_WRITE) ? P_SETMOD : 0));
		ASSERT(vinfo->vpi_nmemlck);
		if (--vinfo->vpi_nmemlck == 0) {
			hat_unlock(seg, addr);
			pvn_memunresv(vp, offset, M_REAL);
		}
		addr += PAGESIZE;
		len -= PAGESIZE;
	}
}

#ifdef DEBUG
int segdz_faultcnt = 0;
#endif

/*
 * STATIC faultcode_t
 * segdz_fault(struct seg *seg, vaddr_t addr, uint_t len,
 *	       enum fault_type type, enum seg_rw rw)
 *	Fault handler for segdz based segments. Here protection, COW,
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
 *
 *	- Acquires the segment WRITE lock since anon map could change.
 *	- calls segdz_softlock() in the F_SOFTUNLOCK case.
 *	- loop over the fault range calling segdz_faultpage
 *
 *	Protection check mask is computed here and passed to segdz_faultpage().
 *
 * Remarks:
 *	F_MEMLOCK case is ONLY possible from segdz_amp_lockop(). It is used by
 *	the caller to fill any holes in the region that is being locked down.
 *	If this fault causes  a hole in the range from sfd_lowaddr and
 *	sfd_hiaddr and the future lock is present, then  the fault range
 *	is extended to fill the hole.
 */
STATIC faultcode_t
segdz_fault(struct seg *seg, vaddr_t addr, uint_t len, enum fault_type type,
		enum seg_rw rw)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	faultcode_t fc =0;
	uint_t protchk;
	vaddr_t lowaddr, hiaddr, savaddr = addr, naddr;
	off_t lowoff, hioff;
	segdz_amp_t *amp = &sdp->sdz_amp;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);
#ifdef DEBUG
	segdz_faultcnt++;
#endif
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
			break;
		default:
			/*
			 *+ An undefined seg_rw enum was passed into
			 *+ segvn_fault. This indicates a problem in either
			 *+ the trap, AS, or segment-generic code paths. This
			 *+ is an unrecoverable situation.
			 */
			cmn_err(CE_PANIC, "segdz_fault/unknown rw type");
			/* NOTREACHED */
	}

	/*
	 * per-segment/per-vpage protection checks (as appropriate)
	 *
	 * Since SOP_setprot executes holding the AS write locked,
	 * and since we hold the AS read lock at this point, no additional
	 * locking is needed to perform protection checks.
	 *
	 */
	if (!(sdp->sdz_prot & protchk) && 
		!(sdp->sdz_flags & (SEGDZ_PGPROT | SEGDZ_MLCKFLT)) &&
		(type != F_MAXPROT_SOFTLOCK) && (rw != S_OTHER))

		return FC_PROT;	/* illegal access */

	if (type == F_SOFTUNLOCK) {
		segdz_softunlock(seg, addr, len, rw);
		goto done;
	}

	RWSLEEP_WRLOCK(&sdp->sdz_seglock, PRIMEM);


	/* 
	 * Extend the fault range if future lock is set and this fault
	 * creates a hole. It could be at either end of the range.
	 */

	if (seg->s_as->a_paglck && (amp->am_swresv != 0)) {
		segdz_amp_findrange(amp, &lowoff, &hioff); 
		SEGDZ_ADDR_RANGE(seg, lowoff, hioff, lowaddr, hiaddr);

		if ((naddr = (addr + len)) < lowaddr) {
			while (naddr < lowaddr) {
				segdz_faultpage(seg, naddr, F_INVAL, protchk,
						rw);
				naddr += PAGESIZE;
			}
		} else if (addr > (naddr = (hiaddr + PAGESIZE))) {
			while (naddr < addr) {
				segdz_faultpage(seg, naddr, F_INVAL, protchk,
						rw);
				naddr += PAGESIZE;
			}
		}
	}
	/*
	 * Now handle the fault on one page at a time.
	 *
	 */
	while (len != 0) {
		fc = segdz_faultpage(seg, addr, type, protchk, rw);
		if (fc) {
			if (type == F_SOFTLOCK) {
				segdz_softunlock(seg, savaddr, addr - savaddr,
						rw);
				RWSLEEP_UNLOCK(&sdp->sdz_seglock);
			}
			break;
		}
		len -= PAGESIZE;
		addr += PAGESIZE;
	}
done:
        if ((type != F_SOFTLOCK) && (type != F_MAXPROT_SOFTLOCK))
		RWSLEEP_UNLOCK(&sdp->sdz_seglock);

	return fc;
}

/*
 * STATIC int
 * segdz_dup(struct seg pseg, struct seg cseg)
 *      Called from as_dup to replicate segment specific data structures.
 *
 * Calling/Exit State:
 *      The parent's address space is read locked on entry to the call and
 *      remains so on return. The parent segment is read locked when duping 
 *	the anon map to stabilize the anon map.
 *
 *      The child's address space is not locked on entry to the call since
 *      there can be no active LWPs in it at this point in time. The segment
 *      is not locked (read or write) for the same reason.
 *
 *	The child's AS is not visible to the swapper. Therefore, it is not
 *	possible to load up translations for the child in this routine
 *	without running the risk of memory deadlock. To preserve the
 *	hat_dup() optimization, we have introduced the segvn_childload()
 *	routine (called by as_childload()).
 *
 *	This function can block. No locks are held at entry to/exit from
 *	this function.
 *
 *	On success, 0 is returned to the caller and s_data in the child
 *	generic segment stucture points to the newly created segvn_data.
 *	On failure, non-zero is returned and indicates the appropriate
 *	errno.
 *
 * Description:
 *
 *	Calls segdz_amp_dup() which does most of the dirty work such as
 *	breaking cow and turning off HAT write protection for the parent.
 *
 *	Note that the AS lock is held in reader mode, effectively stabilizing
 *	the protections and memory locked state of each virtual page.
 */
STATIC int
segdz_dup(struct seg *pseg, struct seg *cseg)
{
	segdz_data_t *psdp = (segdz_data_t *)pseg->s_data;
	segdz_data_t *csdp;
	segdz_amp_t *ch_amp, *p_amp = &psdp->sdz_amp;
	int err;
	segdz_args_t args;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Need to stabilize the segment to get a consistent
	 * picture of its anon map and swap reservation. The
	 * segment READ lock suffices for this purpose.
	 */
	RWSLEEP_RDLOCK(&psdp->sdz_seglock, PRIMEM);

	/*
	 * If the parent has a swap reservation,
	 * then reserve swap for the child.
	 */
	if (!mem_resv(p_amp->am_swresv, M_SWAP)) {
		RWSLEEP_UNLOCK(&psdp->sdz_seglock);
		return ENOMEM;
	}

	/*
	 * Now, allocate the child's segvn_data structure, and
	 * initialize it.
	 */
	csdp = kmem_zalloc(sizeof(segdz_data_t), KM_SLEEP);

	ch_amp = &csdp->sdz_amp;

	err = segdz_amp_dup(pseg, ch_amp, cseg->s_as);
	/*
	 * Release segment READ lock.
	 */

	RWSLEEP_UNLOCK(&psdp->sdz_seglock);
	if (err != 0) {
		args.relargs.amp = ch_amp;
		args.relargs.skip_swfree = 0;
		args.relargs.as = cseg->s_as;

		segdz_amp_traverse(pseg, cseg->s_base, cseg->s_size, 
					SEGDZ_RELEASE, &args);
		ASSERT(ch_amp->am_swresv == 0);
		kmem_free(csdp, sizeof(segdz_data_t));
		return err;
	}

	/*
	 * Now finish up the initialization of the segdz_data structure,
	 * and miscellaneous other information.
	 */
	csdp->sdz_prot = psdp->sdz_prot;

	csdp->sdz_flags = psdp->sdz_flags & (SEGDZ_PGPROT | SEGDZ_HI_TO_LOW | 
						SEGDZ_LOW_TO_HI);
	csdp->sdz_amp.am_off_range = psdp->sdz_amp.am_off_range;
	csdp->sdz_amp.am_level = psdp->sdz_amp.am_level;
	csdp->sdz_amp.am_cowcnt = psdp->sdz_amp.am_cowcnt;
	ASSERT(csdp->sdz_amp.am_swresv == psdp->sdz_amp.am_swresv);

	RWSLEEP_INIT(&csdp->sdz_seglock, 0, &segdz_seglockinfo, KM_SLEEP);

	cseg->s_ops = &segdz_ops;
	cseg->s_data = csdp;

	return 0;
}

/*
 * STATIC void
 * segdz_childload(struct seg *pseg, struct seg *cseg)
 *      Called from as_childload to load up translations for the child
 *	at the end of a fork operation. This routine calls
 *	hat_dup() to copy translations from the parent to the child,
 *	thus implementing an optimization.
 *
 * Calling/Exit State:
 *      The parent's address space is read locked on entry to the call and
 *      remains so on return.
 *
 *	The child's AS is visible to the swapper. Therefore, it is
 *	possible to load up translations for the child in this routine
 *	without running the risk of memory deadlock.
 *
 * Description:
 *	The parent may contain writable translations for pages not shared
 *	with the child. These can result from COWs done by other LWPs
 *	since segdz_dup() executed. These translations must not propogate
 *	forward to the child. The HAT_NOPROTW, flag passed in to hat_dup(),
 *	is for this purpose.
 *
 * 	Note that the AS lock is held in reader mode, effectively stabilizing
 * 	the protections and memory locked state of each virtual page.
 *
 *	It is not proper to copy the read only HAT xlations that may have
 *	come into existence in the parent segment since the fork operation
 *	returned from hat_dup. This can happen only in the following cases:
 *
 *  1)  When the segment had only read only protection before the AS
 *	read lock is acquired in as_dup(). In that case, any read faults
 *	in the parent segment would instantiate a read only xlation.
 *	It is very rare indeed that this segment setprot to be read only.
 *	No setprots can go through during the time interval between the dup
 *	and childload since setprot requires the AS write locked but we hold it
 *	read locked for the duration. All other faults would load the
 *	translation writable.
 *
 *  2)  There was a fault in the parent segment and another LWP in the
 *	process forked causing the faulted xlation to be hat_chgprot() to
 *	read only. 
 *
 *  3)  MAXPROT_SOFTLOCK can cause a COW and load a read only translation.
 *
 *  4)  A cow fault can occur and a dup can come later on. This would
 *	establish a read only xlation.
 *
 *	Conditions 1 & 2 can be verified by checking if the swap
 *	reserved for the child is the same as the parent. If the parent's
 *	swap has changed, then we could have run into either of the above
 *	conditions. 
 *
 *	For conditions 3 & 4, we create a counter of cow faults
 *	and verify the parent's counter against the child's.
 *	We skip hat_dup if they are not the same.
 */
STATIC void
segdz_childload(struct seg *pseg, struct seg *cseg)
{
	segdz_data_t *psdp = (segdz_data_t *)pseg->s_data;
	segdz_data_t *csdp = (segdz_data_t *)cseg->s_data;

	/*
	 * Need to get the read lock to stabilize am_swresv field as well
	 * as to guarantee that no faults take place between checking the
 	 * am_swresv field and calling hat_dup.
	 */
	RWSLEEP_RDLOCK(&psdp->sdz_seglock, PRIMEM);

	if ((psdp->sdz_amp.am_swresv != csdp->sdz_amp.am_swresv) ||
		(psdp->sdz_amp.am_cowcnt != csdp->sdz_amp.am_cowcnt)) {

		RWSLEEP_UNLOCK(&psdp->sdz_seglock);
		return;
	}

	hat_dup(pseg, cseg, HAT_NOPROTW);

	RWSLEEP_UNLOCK(&psdp->sdz_seglock);
}

/*
 * STATIC int
 * segdz_unmap(struct seg *seg, vaddr_t addr, uint_t len)
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
 * Description:
 *	If the range unmapped falls into the middle of a segment the
 *	result will be the creation of a hole in the address space and
 *	the creation of a new segment. In this case the anon map of the
 *	old segment is duped to the new segment for the new segment's
 *	region (through segdz_amp_xfer()) and later the old segment's
 *	anon map for the region addr to the end of the segment is released
 *	(through segdz_amp_release()).
 */
STATIC int
segdz_unmap(struct seg *seg, vaddr_t addr, uint_t len)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data, *nsdp;
	vaddr_t nbase;
	uint_t nsize;
	struct seg *nseg;
	segdz_args_t args;
	struct as *as = seg->s_as;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(addr >= seg->s_base && addr + len <= seg->s_base + seg->s_size);
	ASSERT((len & PAGEOFFSET) == 0);
	ASSERT((addr & PAGEOFFSET) == 0);

	(void)segdz_lockop(seg, addr, len, 0, MC_UNLOCK);
	hat_unload(seg, addr, len, HAT_NOFLAGS);	
	if (addr == seg->s_base && len == seg->s_size) {
		seg_free(seg);
		return 0;
	}

	args.relargs.as = seg->s_as;

	/*
	 * Check for beginning of segment
	 */
	if (addr == seg->s_base) {
		if (sdp->sdz_flags & SEGDZ_HI_TO_LOW) {
			args.relargs.amp = &sdp->sdz_amp;
			args.relargs.skip_swfree = 0;

			segdz_amp_traverse(seg, addr, len, SEGDZ_RELEASE,
						&args);
			seg->s_base += len;
			seg->s_size -= len;
			return 0;
		}
		nseg = kmem_zalloc(sizeof(struct seg), KM_SLEEP);
		nsdp = kmem_zalloc(sizeof(segdz_data_t), KM_SLEEP);

		nseg->s_data = nsdp;
		RWSLEEP_INIT(&nsdp->sdz_seglock, 0, &segdz_seglockinfo,
				KM_SLEEP);

		nsdp->sdz_prot = sdp->sdz_prot;
		nsdp->sdz_flags = sdp->sdz_flags;

		nsize = seg->s_size + len;

		args.xferargs.nseg = nseg;
		args.xferargs.swresv = &nsdp->sdz_amp.am_swresv;
		args.xferargs.as = nseg->s_as;

		segdz_amp_traverse(seg, addr + len, nsize, SEGDZ_XFER, &args);

		ASSERT(nsdp->sdz_amp.am_root != NULL);
		ASSERT(sdp->sdz_amp.am_root != NULL);

		args.relargs.amp = &sdp->sdz_amp;
		args.relargs.skip_swfree = nsdp->sdz_amp.am_swresv;
		segdz_amp_traverse(seg, seg->s_base, seg->s_size, SEGDZ_RELEASE,
				&args);
		seg_free(seg);

		DISABLE_PRMPT();
		nseg->s_ops = &segdz_ops;
		(void)seg_attach(as, addr + len, nsize, nseg);
		ENABLE_PRMPT();		
		return 0;
	}

        /*
         * Check for end of segment
         */
	if (addr + len == seg->s_base + seg->s_size) {
		if (sdp->sdz_flags & SEGDZ_LOW_TO_HI) {
			args.relargs.amp = &sdp->sdz_amp;
			args.relargs.skip_swfree = 0;

			segdz_amp_traverse(seg, addr, len, SEGDZ_RELEASE,
						&args);
			ASSERT(sdp->sdz_amp.am_swresv == 0);
			seg->s_size -= len;
			return 0;
		}
		nseg = kmem_zalloc(sizeof(struct seg), KM_SLEEP);
		nsdp = kmem_zalloc(sizeof(segdz_data_t), KM_SLEEP);

		nseg->s_data = nsdp;
		RWSLEEP_INIT(&nsdp->sdz_seglock, 0, &segdz_seglockinfo,
				KM_SLEEP);
		nsdp->sdz_prot = sdp->sdz_prot;
		nsdp->sdz_flags = sdp->sdz_flags;

		nbase = seg->s_base;
		nsize = seg->s_size - len;

		args.xferargs.nseg = nseg;
		args.xferargs.swresv = &nsdp->sdz_amp.am_swresv;

		segdz_amp_traverse(seg, seg->s_base, nsize, SEGDZ_XFER, &args);

		ASSERT(nsdp->sdz_amp.am_root != NULL);
		ASSERT(sdp->sdz_amp.am_root != NULL);

		args.relargs.amp = &sdp->sdz_amp;
		args.relargs.skip_swfree = nsdp->sdz_amp.am_swresv;

		ASSERT(sdp->sdz_amp.am_root != NULL);
		segdz_amp_traverse(seg, seg->s_base, seg->s_size, SEGDZ_RELEASE,
				&args);
		seg_free(seg);

		DISABLE_PRMPT();
		nseg->s_ops = &segdz_ops;
		(void)seg_attach(as, nbase, nsize, nseg);
		ENABLE_PRMPT();		
		return 0;
	}
	/*
	 * The section to go is in the middle of the segment,
	 * have to make it into two segments. 
	 */
	if (sdp->sdz_flags & SEGDZ_HI_TO_LOW) {
		nbase = seg->s_base;
		nsize = addr - seg->s_base;
	} else {
		nbase = addr + len;
		nsize = seg->s_base + seg->s_size - (addr + len);
	}

	nseg = kmem_zalloc(sizeof(struct seg), KM_SLEEP);
	nsdp = kmem_zalloc(sizeof(segdz_data_t), KM_SLEEP);

	nseg->s_data = nsdp;

	RWSLEEP_INIT(&nsdp->sdz_seglock, 0, &segdz_seglockinfo, KM_SLEEP);

	nsdp->sdz_prot = sdp->sdz_prot;
	nsdp->sdz_flags = sdp->sdz_flags;

	args.xferargs.nseg = nseg;
	args.xferargs.swresv = &nsdp->sdz_amp.am_swresv;

	segdz_amp_traverse(seg, nbase, nsize, SEGDZ_XFER, &args);

	ASSERT(nsdp->sdz_amp.am_root != NULL);
	ASSERT(sdp->sdz_amp.am_root != NULL);

	args.relargs.amp = &sdp->sdz_amp;
	args.relargs.skip_swfree = nsdp->sdz_amp.am_swresv;

	segdz_amp_traverse(seg, (sdp->sdz_flags & SEGDZ_HI_TO_LOW) ? 
		nbase : nbase - len, nsize + len, SEGDZ_RELEASE, &args);

	DISABLE_PRMPT();
	if (sdp->sdz_flags & SEGDZ_HI_TO_LOW) {
		seg->s_size = seg->s_base + seg->s_size - (addr + len);
		seg->s_base = addr + len;
	} else {
		seg->s_size = addr - seg->s_base;
	}
	nseg->s_ops = &segdz_ops;
	(void) seg_attach(as, nbase, nsize, nseg);
	ENABLE_PRMPT();
	return 0;
}

/*
 * STATIC int
 * segdz_setprot(struct seg *seg, vaddr_t addr, uint_t len, uint_t prot)
 *      Change the protections on one or more virtually contiguous pages
 *      within a single segment (seg) beginning at `addr' for `len' bytes.
 *
 * Calling/Exit State:
 *      Called and exits with the address space WRITE locked.
 *
 *      Returns zero on success.
 *
 *      Returns a non-zero errno on failure.
 *
 * Description:
 *	Calls segdz_amp_setprot() if introducing or already existing
 *	page level protections which does most of the dirty work 
 *
 * Remarks:
 *      In SVR4.0 ``classic'' segvn driver this function was complicated by
 *	the fact that we could memory lock pages in core and leave them COW.
 *	This required the memory locking code to maintain separate counts for
 *	existing and potential memory claims and for memory claims to be
 *	shuffled back and forth between the two during setprots.
 *
 *      In ES/MP, all COWable pages are COWed as part of
 *      memory locking and so separate counts are not required, making this
 *      setprot function much simpler. However, if we grant write permission
 *      to memory locked pages in a vnode-backed MAP_PRIVATE segment they
 *	become COWable, a condition we tried to avoid when the memory locking
 *	was first done.
 */
STATIC int
segdz_setprot(struct seg *seg, vaddr_t addr, uint_t len, uint_t prot)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	int err = 0;
	segdz_args_t args;

	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);
	ASSERT((prot & ~PROT_ALL) == 0);

	if (!(sdp->sdz_flags & SEGDZ_PGPROT) && prot == sdp->sdz_prot)
		return 0;


	/* covering the whole segment ? */
	if (len == seg->s_size) {
		if (sdp->sdz_flags & SEGDZ_PGPROT)
			sdp->sdz_flags &= ~SEGDZ_PGPROT;

		args.prot = prot;
		err = segdz_amp_traverse(seg, addr, len, SEGDZ_SETPROT,
						&args);
		sdp->sdz_prot = prot & PROT_ALL;
	} else {
		args.prot = prot;
		err = segdz_amp_traverse(seg, addr, len, SEGDZ_SETPROT, &args);
		if (!err) {
			sdp->sdz_prot = 0;
			sdp->sdz_flags |= SEGDZ_PGPROT;
		}
	}

	if (!(prot & PROT_WRITE))
		hat_chgprot(seg, addr, len, prot, B_TRUE);

	return err;
}

/*
 * STATIC int
 * segdz_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
 *	Return the protections on pages starting at addr for len.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *	This function, which cannot fail, returns the permissions of the
 *	indicated page in the protv outarg.
 *
 */
STATIC int
segdz_getprot(struct seg *seg, vaddr_t addr, uint_t *protv)
{
 	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	vpginfo_t *vinfo;
	anon_t **app;

	ASSERT((addr & PAGEOFFSET) == 0);

	if (!(sdp->sdz_flags & SEGDZ_PGPROT)) {
		*protv = sdp->sdz_prot;
	} else {
		app = segdz_findnode(sdp, SEGDZ_ADDR_TO_OFF(seg, addr),
					&vinfo, NULL);
		if (app == NULL)
			*protv = sdp->sdz_prot;
		else 
			*protv = vinfo->vpi_flags & PROT_ALL;
	}
	return 0;
}

/*
 * STATIC int
 * segdz_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
 *	Determine that for len bytes starting at addr the protection
 *	is at least equal to prot.
 *
 * Calling/Exit State:
 *	Called with the AS lock held and returns the same.
 *
 *      On success, 0 is returned, indicating that the vpage
 *      allows accesses indicated by the specified protection.
 *	Actual protection may be greater. On failure, EACCES is returned,
 *	to indicate that the page (or the entire segment) does not allow the
 *	desired access.
 *
 */
STATIC int
segdz_checkprot(struct seg *seg, vaddr_t addr, uint_t prot)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	vpginfo_t *vinfo;
	anon_t **app;

	ASSERT((addr & PAGEOFFSET) == 0);

	/*
	 * If segment protection can be used, simply check against them.
	 */
	if (!(sdp->sdz_flags & SEGDZ_PGPROT))
		return (((sdp->sdz_prot & prot) != prot) ? EACCES : 0);
	else {
		app = segdz_findnode(sdp, SEGDZ_ADDR_TO_OFF(seg, addr),
					&vinfo, NULL);
		if (app != NULL) {
			if ((vinfo->vpi_flags & prot) != prot)
				return EACCES;
		}
	}
	return 0;
}

/*
 * STATIC void
 * segdz_free(struct seg *seg)
 *
 * Calling/Exit State:
 *	Caller must hold the AS exclusivley locked before calling this
 *	function; the AS is returned locked. This is required because
 *	the constitution of the entire address space is being affected.
 *
 *	No useful value is returned. The operations cannot fail.
 *
 */
STATIC void
segdz_free(struct seg *seg)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	segdz_args_t args;

	ASSERT(getpl() == PLBASE);
	ASSERT((seg->s_base & PAGEOFFSET) == 0);
	ASSERT((seg->s_size & PAGEOFFSET) == 0);

	if (sdp->sdz_amp.am_root != NULL) {

		/* swap unreserved by SEGDZ_RELEASE op */ 
		args.relargs.amp = &sdp->sdz_amp;
		args.relargs.skip_swfree = 0;
		args.relargs.as = seg->s_as;

		segdz_amp_traverse(seg, seg->s_base, seg->s_size,
				SEGDZ_RELEASE, &args); 
	}

	ASSERT(sdp->sdz_amp.am_swresv == 0);
	RWSLEEP_DEINIT(&sdp->sdz_seglock);
	kmem_free(sdp, sizeof(segdz_data_t));
}

/*
 * STATIC void
 * segdz_badop(void)
 *
 * Calling/Exit State:
 *	None.
 *
 */
STATIC void
segdz_badop(void)
{
	/*
	 *+ A segment operation was invoked which is not supported by the
	 *+ segdev segment driver.  This indicates a kernel software problem.
	 */
	cmn_err(CE_PANIC, "segdz_badop");
	/* NOTREACHED */
}

/*
 *
 * STATIC int
 * segdz_lockop(struct seg *seg, vaddr_t addr, uint_t len, int attr, int op)
 * 	Lock down (or unlock) pages mapped for the indicated segment (seg)
 *	starting at virtual address addr for len bytes.
 *
 * Calling/Exit State:
 *	Called with the AS write locked and returns in the same manner.
 *	Because the AS is write locked, we have exclusive access to all
 *	segment-specific data without recourse to any segment-specific
 *	locks.
 *
 *	On success, zero is returned and the requested operation (MC_LOCK ||
 *	MC_UNLOCK) will have been performed. Note that zero is also returned
 *	in the case where the attr variable is set but the current segment
 *	does not match the indicated criteria. Success is also indicated
 *	when (op == MC_UNLOCK) but the segment is not currently locked.
 *
 *	On failure, a non-zero errno is returned to indicate the failure
 *	mode. If the request was MC_LOCK, some subset of the pages may be
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
 *
 */
STATIC int
segdz_lockop(struct seg *seg, vaddr_t addr, uint_t len, int attr, int op)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	uchar_t gprot, pageprot, check, mask;
	int err;
	segdz_args_t args;

	if (attr & SHARED)
		return 0;

	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);
	
	gprot = sdp->sdz_prot;

	mask = check = 0; 
	if (attr) {
		pageprot = attr & ~(SHARED|PRIVATE);
		if (sdp->sdz_flags & SEGDZ_PGPROT) {
			mask = PROT_ALL;
			check = pageprot;
		} else {
			if (gprot != pageprot)
				return(0);
		}
	}

	if (op == MC_UNLOCK || op == MC_UNLOCKAS) {
		if (!(sdp->sdz_flags & SEGDZ_MEMLCK))
			return 0;

		args.lockargs.check = check;
		args.lockargs.mask = mask;

		err = segdz_amp_traverse(seg, addr, len, SEGDZ_UNLOCKOP, &args);
		ASSERT(err == 0);
		if (len == seg->s_size) {
			sdp->sdz_flags &= ~SEGDZ_MEMLCK;
		}
		return 0;
	}

	ASSERT(op == MC_LOCK || op == MC_LOCKAS);

	sdp->sdz_flags |= SEGDZ_MLCKFLT;

	args.lockargs.check = check;
	args.lockargs.mask = mask;

	/* convert to segdz op */
	op = (op == MC_LOCK) ? SEGDZ_LOCKOP: SEGDZ_LOCKASOP;

	err = segdz_amp_traverse(seg, addr, len, op, &args);

	sdp->sdz_flags &= ~SEGDZ_MLCKFLT;
	sdp->sdz_flags |= SEGDZ_MEMLCK;

	return err;
}

/*
 * STATIC int
 * segdz_sync(struct seg *seg, vaddr_t addr, uint_t len, int attr, uint_t flags)
 * 	This function is a no-op for this segment driver since
 *	it consists only of anon pages and as permitted by POSIX, syncing of
 *	such pages is not neccessary.
 *
 * Calling/Exit State:
 *	None.
 *
 */
/* ARGSUSED */
STATIC int
segdz_sync(struct seg *seg, vaddr_t addr, uint_t len, int attr, uint_t flags)
{
	return 0;
}

/*
 * STATIC int
 * segdz_incore(struct seg *seg, vaddr_t addr, uint_t len, char *vec)
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
 * Description:
 *	The actual work is done by segdz_amp_incore() which traverses the anon
 *	map and collects the info. about each virtual page.
 *
 * Remarks:
 *	Please check the Remarks section of segdz_amp_incore() function.
 */
STATIC int
segdz_incore(struct seg *seg, vaddr_t addr, uint_t len, char *vec)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	segdz_args_t args;

	ASSERT(getpl() == PLBASE);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT((addr & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);

	bzero(vec, btop(len));

	RWSLEEP_RDLOCK(&sdp->sdz_seglock, PRIMEM);

	args.incore_vec = vec;
	segdz_amp_traverse(seg, addr, len, SEGDZ_INCORE, &args);

	RWSLEEP_UNLOCK(&sdp->sdz_seglock);
	return len;
}

/*
 * STATIC int
 * segdz_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
 *	There is no vp associated with this segment.
 *
 * Calling/Exit State:
 *	Returns error.
 *
 */
/* ARGSUSED */
STATIC int
segdz_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp)
{
	return -1;
}

/*
 * STATIC off_t
 * segdz_gettype(struct seg *seg, vaddr_t addr)
 *	This segment driver type is always MAP_PRIVATE.
 *
 * Calling/Exit State:
 *	Returns MAP_PRIVATE.
 *
 */
/* ARGSUSED */
STATIC int
segdz_gettype(struct seg *seg, vaddr_t addr)
{
	return MAP_PRIVATE;
}

/*
 * STATIC off_t
 * segdz_getoffset(struct seg *seg, vaddr_t addr)
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
/* ARGSUSED */
STATIC off_t
segdz_getoffset(struct seg *seg, vaddr_t addr)
{
	return addr - seg->s_base;
}

/*
 * int
 * segdz_memory(struct seg *seg, vaddr_t *basep, u_int *lenp)
 *      Returns the instantiated range contained in this segment.
 *
 * Calling/Exit State:
 *      Caller gurantees that there is no changes in the AS layout.
 *
 *      basep and lenp both serve as an inarg and an outarg.
 *
 * Remarks:
 *	We use the segfu_findrange utility to find the real memory backed
 *	vpage range.
 */
int
segdz_memory(struct seg *seg, vaddr_t *basep, u_int *lenp)
{
	segdz_data_t *sdp = (segdz_data_t *)seg->s_data;
	segdz_amp_t *amp = &sdp->sdz_amp;
	vaddr_t eaddr = *basep + *lenp, lowaddr, hiaddr;
	off_t lowoff, hioff;
#ifdef DEBUG
	vaddr_t addr = *basep;
#endif

	ASSERT(seg->s_base < eaddr);
	ASSERT(addr < seg->s_base + seg->s_size);

	segdz_amp_findrange(amp, &lowoff, &hioff); 
	SEGDZ_ADDR_RANGE(seg, lowoff, hioff, lowaddr, hiaddr);

	if (*basep < lowaddr)
		*basep = lowaddr;
	if (hiaddr < eaddr)
		*lenp = (hiaddr - *basep) + PAGESIZE;
	else
		*lenp = eaddr - *basep;

	return 0;
}
