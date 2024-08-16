/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/vm_swap.c	1.26"
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
 * Generic swap space interfaces:
 *
 * 	Run-time addition of swap devices.
 *	Run-time deletion of swap devices.
 *	Run-time configuration queries (device configuration).
 *	Allocation and deallocation of physical swap space.
 *
 * NOTE: swap space mangement has been reorganized for ES/MP. Previously,
 *       all anonymous pages required physical backing storage when created
 *  	 because the location of the page on a swap device was used to
 *	 lend identity (vp and offset) to the anonymous page. This made the
 *	 total number of bytes of physical memory available for anonymous
 *	 pages (e.g. all user malloc()ed data) limited to the total number
 *	 of bytes of configured swap space. Bytes of memory in excess of
 *	 configured swap space was simply unavailable for use as anonymous
 *	 memory.
 *
 *	 Under the new organization, the management of swap space and the
 *	 management of page identity have been have been separated into to
 *	 sub-functions. Swap devices are now only used to provide backing
 *	 storage, if and when needed. The vnodes which underly swap devices
 *	 are no longer used to provide identity to anonymous pages. Instead,
 *	 identity is lent by anonfs or memfs provided vnodes (called anodes
 *	 and mnodes, respectively) created specifically to represent
 *	 anonymous pages. Each anode can represent up to
 *	 ANON_MAX_OFFSET+PAGESIZE bytes of anonymous memory. Thus, a small
 *	 system will typically have only a single anode, even when multiple
 *	 swap devices have been added. On the other hand, each mnode
 *	 gives identity to the pages of a single memfs file.
 *
 *	 Chunks of anon structures are now allocated as needed by
 *	 anon_alloc().
 * 
 * 	 These changes allow ES/MP to restore the more flexible policies of
 * 	 SVR3 under the 'availsmem' management scheme. Swappable pages may
 * 	 consume 'all' of available physical memory *plus* any configured
 * 	 physical swap space. One immediate benefit of this is that the
 * 	 system no longer requires a swap device to boot.
 * 
 * 	 For more complete details, consult mem:vm_anon.c and the design
 * 	 specification "Multiprocessing Virtual Memory Design for UNIX
 * 	 System V Release 4ES MP."
 */

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/memfs/memfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/anon.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/memresv.h>
#include <mem/swap.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>


/*
 * External declarations:
 */
extern vnode_t *common_specvp(vnode_t *);
extern int swap_maxdev;

/*
 * Global data:
 */

/*
 * The following fields are mutexed by swap_lck.
 */
swapinfo_t	**swaptab;		/* the swap table */
uint_t		nswapfiles;		/* number of swap files */
ulong_t		nswappg;		/* total swap pages */
ulong_t		nswappgfree;		/* free swap pages */
ulong_t		swapdoubles;		/* see anon_pageout for details */

/*
 * Mutex for swap global data.
 */
lock_t		swap_lck;

/*
 * static data
 *
 *	Also mutexed by swap_lck.
 */
STATIC swapinfo_t	**eswaptab;	/* current swap table end (exclusive) */
STATIC swapinfo_t	**swapcursor;	/* allocation cursor into swaptab */

/*
 *+ global lock used to protect swapinfo_t list and fields.
 */
STATIC LKINFO_DECL(swap_lck_info, "MP:swap:swap_lck", 0);

/*
 * Internal STATIC functions.
 */
STATIC vnode_t *swapdel_byname(char *name, off_t lowblk);

/*
 * Calculate number of bytes needed for swap space bitmap.
 */

#define PGMAP_BYTES(npages) \
	((BITMASK_NWORDS(npages)) * sizeof(uint_t))

/*
 * void
 * swap_init(void)
 *	Initialize the swap subsystem.
 *
 * Calling/Exit State:
 *	Called from kvm_init() after anon_conf() is called.
 */
void
swap_init(void)
{
	LOCK_INIT(&swap_lck, VM_SWAP_HIER, VM_SWAP_IPL, &swap_lck_info,
		  KM_NOSLEEP);
	eswaptab = swapcursor = swaptab =
		kmem_alloc(sizeof(swapinfo_t *) * swap_maxdev, KM_NOSLEEP);
	if (swaptab == NULL) {
		/*
		 *+ The operating system was unable to allocate the
		 *+ memory required to initialize the swap file management
		 *+ subsystem. This probably indicates a configuration
		 *+ error elsewhere in the operating system, or possibly
		 *+ the underlying hardware.
		 */
		cmn_err(CE_PANIC, "swap_init: no memory for swaptab");
	}
}

/*
 * STATIC int
 * swapadd(vnode_t *vp, off_t soff, uint_t len, char *swapname)
 *	Add [additional] swap space to the system from the backing
 *	storage entity represented by vp, starting at offset soff
 *	and extending for len bytes.
 *
 * Calling/Exit State:
 *	No LOCKs are held on entry and this function returns that way.
 *
 *	The vp has has been checked to insure it can be used as a swap device.
 *	Both soff and len have been rounded appropriately to PAGESIZE
 *	boundaries.
 *
 *	On success, len bytes of swap space backed by the entity represented
 *	by vp will have been added to the system. The underlying filesystem
 *	will have been informed of operation via VOP_STABLESTORE(). Global
 *	counters will have been appropriately adjusted and an entry added
 *	to the swap table. The character string passed to us via
 *	swapname will have been copied into a newly kmem_alloced array and
 *	pointed to by the newly created swapinfo_t.  A zero is returned in the
 *	success case.
 *
 *	On failure, non-zero is returned in the form of an errno to indicate
 *	the failure mode.
 *
 * Remarks:
 * 	Our caller has `stabilized' the vp passed (by doing a lookupname)
 *	to us but expects any further manipulation to take place here,
 *	including VN_RELE in error cases. This is necessary because VOP_OPEN
 *	and/or VOP_ALLOCSTORE may choose to substitute different vps for the
 *	one we passed in and performed implicit VN_RELEs on the original
 *	object.

 *      In previous versions of SVR4 a swap device was required to boot the
 *      system and it was possible to swapadd this (first) device even if
 *	it was smaller than specifed by the len parameter. Under the anonfs
 *	paradigm, there is no boot swap device. Consequently, all swap
 *	devices must match the specified len parameter.
 */
STATIC int
swapadd(vnode_t *vp, off_t soff, uint_t len, char *swapname)
{
	swapinfo_t *osip, *nsip;
	swapinfo_t **sipp, **nsipp;
	void *storeobj;
	ulong_t npages, diff;
	off_t eoff;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Perform an 'open' on the swap device. It is possible 
	 * that the underlying filesystem will choose to replace
	 * the vp passed in with another vp. This is not a problem and we
	 * expect that in this case the underlying filesystem will perform
	 * the necessary VN_HOLD/VN_RELE adjustments.
	 *
	 * Note the VNOSWAP test has already been performed by our
	 * caller.
	 */
	if ((error = VOP_OPEN(&vp, FREAD|FWRITE, CRED())) != 0) {
		goto nfree_out;
	}

	/*
	 * Allocate the swapinfo_t structure now before anything else.
	 * The reason for this is to let us put a placeholder into the
	 * list of active swap devices and prevent any duplications after
	 * we drop swap_lck and potentially block for other allocations.
	 */

	nsip = kmem_alloc(sizeof(swapinfo_t), KM_SLEEP);

	/*
	 * Get the common_specvp. Remember both vnodes in the swapinfo_t.
	 */
	nsip->si_ovp = vp;
	nsip->si_cvp = common_specvp(vp);

	/*
	 * Since the file may have dirty pages from its use before the
	 * swapadd, make sure these pages are synced to backing store
	 * before using it as a swap device.
	 */
	if ((error = VOP_FSYNC(nsip->si_cvp, CRED())) != 0) {
		goto nlock_out;
	}

	/*
	 * Remember the user specified starting offset in order to recognize
	 * future SDC_REMOVE requests using the SC_ADD specified value.
	 *
	 * We round the the starting value (up) and the length (down) to
	 * page aligned values. Note that the VOP_STABLESTORE() [below]
	 * may perform an additional block alignment. However, we do the
	 * page alignment here in order to minimize the size of the
	 * anon_del_swap(... , ANON_ADD_BACKOUT), or possibly eliminate it.
	 */
	eoff = soff + len;
	nsip->si_osoff = soff;
	nsip->si_soff = (soff + PAGEOFFSET) & PAGEMASK;
	nsip->si_eoff = eoff & PAGEMASK;
	npages = btop(len);

	/*
	 * No pages after rounding?
	 */
	if (nsip->si_soff >= nsip->si_eoff) {
		error = EINVAL;
		goto nlock_out;
	}

	/*
	 * Lock the swapinfo list and search for duplicate or overlapping
	 * entries.
	 */

	(void) LOCK(&swap_lck, VM_SWAP_IPL);

	nsipp = NULL;
	for (sipp = swaptab; sipp < eswaptab; ++sipp) {
		osip = *sipp;
		if (osip != NULL) {
			/*
			 * See if this swap device aleady exists in one form
			 * or another. It can be in many stages of existence:
			 * being added by another process, in use, or being
			 * deleted. Any such stage of existence is sufficient
			 * to inhibit us from another swappadd().
			 *
			 * Check for overlapping range. For ease
			 * of user understanding, we are checking the
			 * the intended new usage (not rounded) against the
			 * actual current usage (which is reported by the
			 * "swap -l" command).
			 */
			if (osip->si_cvp == nsip->si_cvp &&
			    soff < osip->si_eoff && eoff > osip->si_soff) {
				error = EEXIST;
				goto out;
			}
		} else if (nsipp == NULL)
			nsipp = sipp;
	}

	/*
	 * Check to see if the swap table is full.
	 */
	if (nsipp == NULL) {
		if (eswaptab < swaptab + swap_maxdev) {
			nsipp = eswaptab;
		} else {
			error = ENOMEM;
			goto out;
		}
	}

	/*
	 * No duplication found. We need to insert this new swapinfo_t into
	 * swaptab before releasing swap_lck to prevent duplications.
	 * We need to mark it ``ST_NOTREADY'' so that it is not actually
	 * used. It will be found by other swapadd()s (to prevent
	 * duplication) but swapdel()s, swapalloc()s, and swapcntl(SC_LIST)
	 * requests will skip it.
	 */

	nsip->si_flags = ST_NOTREADY;

	*nsipp = nsip;
	if (nsipp == eswaptab)
		++eswaptab;

	UNLOCK(&swap_lck, PLBASE);

	/*
	 * Check with anon to see if it is okay to add the swap space.
	 */
	if (!anon_add_swap(npages)) {
		error = ENOMEM;
		goto total_out;
	}

	/*
	 * Make our call to VOP_STABLESTORE() to give the filesystem a
	 * chance to assign physical space on the backing device to the
	 * entity we are using for swap storage. This is also when we find
	 * out if the entity represented by vp is large enough to provide the
	 * requested space and if sufficient storage exists to back it up.
	 *
	 * We had to wait until now because it is our responsibility to insure
	 * no overlapping swapadds are performed for the same vnode. This
	 * spares implementing filesystems the burden of tracking this them-
	 * selves and the corresponding increase in complexity.
	 *
	 * We pass in the address of a void * which the filesystem may choose
	 * to set to point to an object it can use later when we start pushing
	 * pages out. We also pass the address of our vnode since the
	 * filesystem is free to pass us back something else if convienent.
	 * filesystem will perform the necessary VN_HOLD/VN_RELE adjustments.
	 *
	 * Note that (1) si_ovp will still be needed to to perform
	 * VOP_RELSTORE/VOP_CLOSE in the event of a deletion, and (2) si_stvp
	 * is an outarg of VOP_STABLESTORE() - which is free to perform a
	 * vnode substitution on us.
	 */

	soff = nsip->si_soff;
	len =  nsip->si_eoff - soff;
	nsip->si_stvp = vp;
	error = VOP_STABLESTORE(&nsip->si_stvp, &soff, &len, &storeobj, CRED());

	if (error) {

		if (error == ENOSYS) {
			/*
			 *+ During a swapctl(2) SC_ADD attempt, a file was
			 *+ opened for swapping, but the filesystem reported
			 *+ that it does not support VOP_STABLESTORE(). This
			 *+ indicates an inconsistency in the file system
			 *+ implementation, because the VNOSWAP flag in the
			 *+ vnode is clear. An investigation into the
			 *+ filesystem is indicated.
			 */
			cmn_err(CE_WARN,
				"swapadd: VOP_STABLESTORE() returned ENOSYS");
		}

		/*
		 * Back out the anon_add_swap().
		 */
		anon_del_swap(npages, 0, ANON_ADD_BACKOUT);
		goto total_out;
	}

	/*
	 * We are now committed to the swap add.
	 */

	/*
	 * The file system, at its discretion, may have adjusted soff
	 * and len to achieve block alignment. If so, we need to adjust our
	 * view of the swap device.
	 */
	ASSERT((soff & PAGEOFFSET) == 0);
	ASSERT((len & PAGEOFFSET) == 0);
	ASSERT(soff >= nsip->si_soff);
	ASSERT(soff + len <= nsip->si_eoff);
	diff = npages - btop(len);
	if (diff != 0) {
		/*
		 * Inform anon of the change in intentions.
		 */
		anon_del_swap(diff, 0, ANON_ADD_BACKOUT);

		/*
		 * Adjust our view
		 */
		nsip->si_soff = soff;
		nsip->si_eoff = soff + len;
		npages -= diff;
	}

	/*
	 * Allocate and initialize the bitmap used to disburse swap space.
	 * One bit is used per page (PAGESIZE) of swap. The array is zalloced
	 * to initially set each page of space (bit) free.
	 */

	nsip->si_pgmap = kmem_zalloc(PGMAP_BYTES(npages), KM_SLEEP);

	/*
	 * Complete initialization of the swapinfo_t
	 */

	nsip->si_obj = storeobj;
	nsip->si_npgs = npages;
	nsip->si_nfpgs = npages;
	nsip->si_pname = kmem_alloc(strlen(swapname) + 1, KM_SLEEP);
	strcpy(nsip->si_pname, swapname);
	nsip->si_cursor = 0;

	/*
	 * Make the swap device available for use.
	 * Significantly, we need to zap the ST_NOTREADY flag.
	 * Add to the pool of reservations in anoninfo.
	 */

	(void) LOCK(&swap_lck, VM_SWAP_IPL);

	nsip->si_flags = 0;
	nswapfiles++;
	nswappgfree += npages;
	nswappg += npages;

	/*
	 * It is important that we do the anon_add_swap_resv() with the
	 * swap_lck held, so that a competing swapdel() does not try to
	 * remove what has not been added yet.
	 */
	anon_add_swap_resv(npages);

	UNLOCK(&swap_lck, PLBASE);

	return (0);

	/*
	 * Four error exit points:
	 *
	 *	nfree_out:
	 *		Just release the vnode passed in.
	 *
	 *	nlock_out:
	 *		Takes care of errors which occur before the new
	 *		swapinfo_t is in the swap table. The swap_lck
	 *		is not held.
	 *
	 *	out:
	 *		Takes care of errors which occur before the new
	 *		swapinfo_t is in the swap table.
	 *
	 *	total_out:
	 * 		Takes care of errors which occur after the new
	 *		swapinfo_t is already in the swap table.
	 */
total_out:
	(void) LOCK(&swap_lck, VM_SWAP_IPL);
	*nsipp = NULL;
	if (nsipp == eswaptab - 1)
		--eswaptab;
out:
	UNLOCK(&swap_lck, PLBASE);
nlock_out:
	kmem_free(nsip, sizeof(swapinfo_t));
	(void)VOP_CLOSE(vp, FREAD|FWRITE, 1, (off_t)0, CRED());
nfree_out:
	VN_RELE(vp);

	return (error);
}

/*
 * STATIC int
 * swapdel(vnode_t *vp, off_t soff)
 *      Delete swap space from the system.
 *
 * Calling/Exit State:
 *	The vp passed in is expected to already be in use as a swap device
 *	and soff has been rounded down to a page boundary.
 *
 *	On success, a zero is returned and the swap space added to the system
 *	via the device indicated by vp has been deleted. The swapinfo_t data
 *	structure is gone.
 *
 *	On failure, a non-zero errno is returned to indicate the failure mode.
 *	Deletions can fail if a race occured and another request to delete the
 *	the same device was received first or if the system would have less
 *	than tune.t_minasmem pages of free swap space.
 *
 *      No SPIN LOCKs are held on entry to or exit from this function.
 *
 * Remarks:
 *	In previous versions of SVR4 a swap device was required to boot the
 *	system and it was not possible to delete the last swap device. Under
 *	the anonfs paradigm, there is no boot swap device, and all swap
 *	devices may be deleted.
 */
/* ARGSUSED */
STATIC int
swapdel(vnode_t *vp, off_t soff)
{
	swapinfo_t **sipp;
	swapinfo_t *sip;
	ulong_t npages;
	vnode_t *cvp;
	int error;

	ASSERT((soff & PAGEOFFSET) == 0);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Perform an 'open' on the swap device since it is possible 
	 * that the underlying filesystem will choose to replace
	 * the vp passed in with another vp. This is not a problem and we
	 * expect that in this case the underlying filesystem will perform
	 * the necessary VN_HOLD/VN_RELE adjustments. We immediately close
	 * the opened vnode since all we are interested in getting the real 
	 * vnode.
	 */
	if ((error = VOP_OPEN(&vp, FREAD|FWRITE, CRED())) != 0) {
		VN_RELE(vp);
		return error;
	}
	(void)VOP_CLOSE(vp, FREAD|FWRITE, 1, (off_t)0, CRED());

	/*
         * Find the swap file entry for the file to be deleted.
         */

	cvp = common_specvp(vp);

	/*
	 * We can do the VN_RELE of the vp now. This is because
	 * we don't do any VOP operations on the vnode if it is not
	 * found on the swaptab list. If it is a swap device, then we still
	 * have a hold on the vnode from swapadd.
	 */
	VN_RELE(vp);

	(void) LOCK(&swap_lck, VM_SWAP_IPL);

	for (sipp = swaptab; sipp < eswaptab; ++sipp) {
		/*
		 * See if this swap device is fully formed. It can be in
		 * many stages of existence: being added by another process,
		 * in use, or being deleted. However, we will only delete a
		 * device which is fully formed (si_flags == 0).
		 *
		 * Because we performed page and/or file block rounding
		 * when we added the swap file, we are a bit tolerant of
		 * start offset specification here, accepting any value
		 * between the start offset specified to the SC_ADD
		 * (sip->si_osoff) and the actual start offset
		 * (sip->si_soff).
		 */
		sip = *sipp;
		if (sip && sip->si_cvp == cvp &&
		    sip->si_osoff <= soff && soff <= sip->si_soff &&
		    sip->si_flags == 0)
				break;
	}

	if (sipp == eswaptab) {
		UNLOCK(&swap_lck, PLBASE);
		return (EINVAL);
	}

	/*
	 * Do the anon reservation accounting.
	 */
	npages = sip->si_npgs;
	if(!anon_del_swap_resv(npages)) {
		UNLOCK(&swap_lck, PLBASE);
		return (ENOMEM);
	}

	/*
	 * We are now committed to the swap delete.
	 *
	 * Starting now, the free space on the swap file being deleted can no
	 * longer be allocated from. So ding nswappgfree and nswappg to
	 * reflect this. Also note that the ding of nswappgfree is required
	 * to get the accounting right for the breaking of doubly associated
	 * pages.
	 */
	nswappgfree -= sip->si_nfpgs;
	nswappg -= sip->si_npgs;
	--nswapfiles;

	/*
	 * In addition to inhibiting additional concurrent delete attempts,
	 * this flag also suppresses further adjustments to nswappgfree
	 * based upon swap_free(s) from this swap file.
	 */
	sip->si_flags |= ST_INDEL;

	UNLOCK(&swap_lck, PLBASE);

	/*
	 * Ask anonfs to gets its claws out of the swap file.
	 * Note that once anon_del_swap_resv() agrees to the delete,
	 * anon_del_swap() is sure to succeed.
	 */
	anon_del_swap(npages, (uchar_t)(sipp - swaptab), ANON_SWAP_DELETE);

	/*
	 * Ask memfs to get its claws out of the swap file.
	 */
	memfs_swapdel((uchar_t)(sipp - swaptab));

	/*
	 * Remove the swapinfo structure from the swaptab.
	 */
	(void) LOCK(&swap_lck, VM_SWAP_IPL);
	ASSERT(sip->si_nfpgs == sip->si_npgs);
	*sipp = NULL;
	if (sipp == eswaptab - 1)
		--eswaptab;
	UNLOCK(&swap_lck, PLBASE);

	/*
	 * Now, let the file system free up its STABLESTORE object.
	 */
	error = VOP_RELSTORE(sip->si_ovp, sip->si_soff, ptob(sip->si_npgs),
			     sip->si_obj, CRED());
	if (error) {
		/*
		 *+ A swap delete operation successfully removed the swap
		 *+ device, but failed to reclaim the space associated with an
		 *+ in memory data structure called the ``store object''. A
		 *+ single occurence of this problem is probably not serious.
		 *+ However, repeated occurences may cause the system to lose
		 *+ substantial memory resources. At that point, a system
		 *+ reboot may be advisable.
		 */
		cmn_err(CE_WARN, "swap delete failed to delete store object");
	}

	/*
	 * Close the vnode and release it. Note we release the hold
	 * remaining from the SC_ADD operation here. Our called,
	 * swapctl(), we release the hold it has placed.
	 */
	(void)VOP_CLOSE(sip->si_ovp, FREAD|FWRITE, 1, (off_t)0, CRED());
	VN_RELE(sip->si_ovp);

	/*
	 * Finally, free up the swapinfo structure plus its
	 * various attached pieces.
	 */
	kmem_free(sip->si_pname, strlen(sip->si_pname) + 1);
	kmem_free(sip->si_pgmap, PGMAP_BYTES(npages));
	kmem_free(sip, sizeof(swapinfo_t));

	return (0);
}

/*
 * STATIC vnode_t *
 * swapdel_byname(char *name, off_t lowblk)
 *	 See if name is one of our swap files even though lookupname failed.
 *
 * Calling/Exit State:
 *	Called at PLBASE and returns that way.
 *
 *	On success returns the vp of the swap file matching the ``name''
 *	argument. On failure, returns NULL.
 *
 * Remarks:
 *	 This can be used by swapctl(2) in the SC_REMOVE case to delete
 *	 swap resources on remote machines where the link has gone down.
 */
STATIC vnode_t *
swapdel_byname(char *name, off_t lowblk)
{
	swapinfo_t **sipp;
	swapinfo_t *sip;
        off_t soff;
	vnode_t *vp;

	ASSERT(getpl() == PLBASE);


	/*
	 * round up to page boundary
	 */
	soff = ptob(btopr(lowblk));

	/*
	 * Find the swap file entry for the file to
	 * be deleted. Skip any entries that are in
	 * transition.
	 */

	(void) LOCK(&swap_lck, VM_SWAP_IPL);

	/*
	 * Because we performed page and/or file block rounding when we
	 * added the swap file, we are a bit tolerant of start offset
	 * specification here, accepting any value between the start offset
	 * specified to the SC_ADD (sip->si_osoff) and the actual start
	 * offset (sip->si_soff).
	 */
	sipp = swaptab;
	for (;;) {
		if (sipp == eswaptab) {
			UNLOCK(&swap_lck, PLBASE);
			return ((vnode_t *)NULL);
		}
		if ((sip = *sipp) != NULL &&
	            sip->si_osoff <= soff && soff <= sip->si_soff &&
	            !sip->si_flags && strcmp(sip->si_pname, name) == 0)
			break;
		++sipp;
	}

	/*
	 * Found the swap file. Place a hold on its vnode before releasing
	 * the swap_lck.
	 */
	vp = sip->si_ovp;
	VN_HOLD(vp);

	UNLOCK(&swap_lck, PLBASE);

	return (vp);
}

struct swapcmda {
	int     sc_dummy;	/* uadmin cmd argument */
	int     sc_cmd;		/* command code for swapctl */
	void    *sc_arg;	/* argument pointer for swaptcl */
};

/*
 * int
 * swapctl(struct swapcmda *uap, rval_t *rvp)
 *	System call to add, delete, list, and total swap devices.
 *
 * Calling/Exit State:
 *	No MP-specific state is required to make this call and no
 *	MP-specific state is held on return.
 *
 *	On success, zero is returned and the requested operation has
 *	been performed. If (sc_cmd == SC_GETNSWAP), then the number
 *	of swap devices will be returned in rvp->r_val1.
 *	If (sc_cmd == SC_LIST), then the requested information will have
 *	been copied out into the user's address space at the address
 *	indicated by ((swaptbl_t *)uap->sc_arg)->swt_ent and rvp->r_val1
 *	will be set to indicate how many entries were actually returned.
 *
 * 	On failure, a non-zero errno is returned to indicate the failure
 *	mode.
 *
 * Remarks:
 *	When performing SC_ADD and SC_REMOVE requests, offset and length
 *	values passed in sr_start and sr_length are provided in terms
 *	of UBSIZE (512-byte) units and are converted before being used.
 *	See comment in SC_ADD/SC_REMOVE cases below.
 */
int
swapctl(struct swapcmda *uap, rval_t *rvp)
{
	swapinfo_t **sipp;
	swapinfo_t *sip;
	int error;
	struct swapent st, *ust;
	struct swapres sr;
	swapusage_t usage;
	vnode_t *vp;
	register int cnt;
	int length;
	char *swapname;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	switch (uap->sc_cmd) {

	case SC_GETNSWP:

		/*
		 * This data may be stale if it is ever passed back in a
		 * subsequent SC_LIST request. However, we include both INDEL
		 * and NOTREADY swap files in order to error on the side of
		 * returning a larger than necessary value.
		 */
		
		cnt = 0;
		(void) LOCK(&swap_lck, VM_SWAP_IPL);
		for (sipp = swaptab; sipp < eswaptab; ++sipp)
			if (*sipp != NULL)
				++cnt;
		UNLOCK(&swap_lck, PLBASE);

		rvp->r_val1 = cnt;
		return (0);

	case SC_LIST:
#ifdef CC_PARTIAL
		/*
		 * privilege required on a system running MAC
		 * to avoid a potential covert channel.
		 */
		if (pm_denied(CRED(), P_SYSOPS))
			return (EPERM);
#endif /* CC_PARTIAL */

		if ((error = ucopyin(uap->sc_arg, &length, sizeof(int),0)) != 0)
			return (error);

		/*
		 * Locate address of the user's table (to receive the data).
		 */
		ust = (swapent_t *)((swaptbl_t *)uap->sc_arg)->swt_ent;

		/*
		 * Allocate some storage for a copy of the swap file name
		 * which might be too large to fit in automatic storage.
		 */
		swapname = kmem_alloc(MAXPATHLEN, KM_SLEEP);
		sipp = swaptab;
		cnt = 0;

		for (;;) {
			(void) LOCK(&swap_lck, VM_SWAP_IPL);
			if (sipp >= eswaptab) {
				UNLOCK(&swap_lck, PLBASE);
				break;
			}
			if ((sip = *sipp) == NULL ||
			     (sip->si_flags & ST_NOTREADY)) {
				UNLOCK(&swap_lck, PLBASE);
				++sipp;
				continue;
			}

			/*
			 * Convert soff/eoff back from bytes to 512-byte
			 * sectors.
			 */
			st.ste_length = (sip->si_eoff-sip->si_soff) >> SCTRSHFT;
			st.ste_start = sip->si_soff >> SCTRSHFT;
			st.ste_pages = sip->si_npgs;
			st.ste_free = sip->si_nfpgs;
			st.ste_flags = sip->si_flags;
			strcpy(swapname, sip->si_pname);

			UNLOCK(&swap_lck, PLBASE);

			/*
			 * Return an error if the user provided insufficient
			 * storage to hold the return info.
			 */
			if (length-- == 0) {
				error = ENOMEM;
				break;
			}

                        /*
                         * Get a hold of the ste_path pointer, which we need
                         * in order to copyout the swap file name.
                         */
                        error = ucopyin(&ust->ste_path, &st.ste_path,
                                             sizeof(st.ste_path), 0);
			if (error)
                                break;

			/*
			 * Copyout the swapent_t.
			 */
			ASSERT((st.ste_flags & ~ST_INDEL) == 0);
			error = ucopyout(&st, ust, sizeof(swapent_t), 0);
			if (error) {
				break;
			}

			/*
			 * Copyout swap file name.
			 */
			error = ucopyout(swapname, st.ste_path,
				         (strlen(swapname) + 1), 0);
			if (error)
				break;

			++ust;
			++cnt;
			++sipp;
		}

		rvp->r_val1 = cnt;
		kmem_free(swapname, MAXPATHLEN);

		return (error);

	case SC_ADD:
	case SC_REMOVE:
		break;
	
	case SC_GETUSAGE:
#ifdef CC_PARTIAL
		/*
		 * privilege required on a system running MAC
		 * to avoid a potential covert channel.
		 */
		if (pm_denied(CRED(), P_SYSOPS))
			return (EPERM);
#endif /* CC_PARTIAL */

		/*
		 * Pick up allocation/reservation statistics from
		 * the swap variables nswappg and nswappgfree and
		 * the anoninfo structure.
		 */
		(void) LOCK(&swap_lck, VM_SWAP_IPL);
		usage.stu_allocated = nswappg - nswappgfree;
		FSPIN_LOCK(&vm_memlock);
		usage.stu_used = anoninfo.ani_resv;
		usage.stu_max = anoninfo.ani_kma_max;
		FSPIN_UNLOCK(&vm_memlock);
		UNLOCK(&swap_lck, PLBASE);

		return (ucopyout(&usage, uap->sc_arg, sizeof(usage), 0));

	default:
		return (EINVAL);
	}

	if (pm_denied(CRED(), P_SYSOPS))
		return (EPERM);

	if ((error = ucopyin((caddr_t)uap->sc_arg, (caddr_t)&sr,
				sizeof(swapres_t), 0)))
		return (error);

	/*
	 * Read in the pathname.
	 */
	swapname = kmem_alloc(MAXPATHLEN, KM_SLEEP);
	error = copyinstr(sr.sr_name, swapname, MAXPATHLEN, 0);
	if (error)
		goto out;

	error = lookupname(swapname, UIO_SYSSPACE, FOLLOW, NULLVPP, &vp);
	if (error) {
		if (uap->sc_cmd == SC_ADD)
			goto out;
		/* see if we match by name */
		vp = swapdel_byname(swapname, sr.sr_start << SCTRSHFT);
		if (vp == NULL)
			goto out;
		/* reset */
		error = 0;
	}

	if (uap->sc_cmd == SC_REMOVE) {
		/* 
		 * swapdel takes care of VN_RELE
		 */
		error = swapdel(vp, sr.sr_start << SCTRSHFT);
		goto out;
	} else {
		/*
		 * Vnode checking prior to swapadd. This checking function
		 * should probably be performed by VOP_STABLESTORE().
		 */
		if (vp->v_flag & (VNOMAP | VNOSWAP)) {
			error = ENOSYS;
			goto vnrele_out;
		}

		switch (vp->v_type) {
			case VBLK:
				break;

			case VREG:
				if (vp->v_vfsp->vfs_flag & VFS_RDONLY) {
					error = EROFS;
				} else
					error = VOP_ACCESS(vp, VREAD|VWRITE, 0, CRED());
				break;

			case VDIR:
				error = EISDIR;
				goto vnrele_out;

			default:
				error = ENOSYS;
				goto vnrele_out;
		}

		/*
		 * swapadd() takes care of the VN_RELE.
		 */
		error = swapadd(vp, sr.sr_start << SCTRSHFT,
				sr.sr_length << SCTRSHFT, swapname);
		goto out;
	}

vnrele_out:
	VN_RELE(vp);
out:
	kmem_free(swapname, MAXPATHLEN);
	return (error);
}

/*
 * boolean_t
 * swap_alloc(int npages, swaploc_t *swaplocp)
 *	Allocate `npages' worth of logically contiguous swap space.
 *
 * Calling/Exit State:
 *	Called with swap_lck held and returns the same way. This protects the
 *	linkages in the swapinfo list (from deletes and adds) and the si_nfpgs
 *	and si_pgmap field in all the swapinfo_t's.
 *
 *	Our caller has already checked nswappgfree and made a `reservation'
 *	by decrementing it by the appropriate amount.
 *
 *	On success, B_TRUE is returned and PAGESIZE `npages' worth of swap
 *	space will have been allocated on the device indicated by the outargs
 *	sipp and offp. *sipp will be used by the caller to determine the vnode
 *	and opaque storage object to use for subsequent calls to VOP_PUTPAGELIST
 *	and VOP_GETPAGELIST. Note that *offp is in bytes relative to the
 *	beginning of the swap area represented by *sipp.
 *
 * 	On failure, B_FALSE is returned to indicate that `npages' worth of
 *	contiguous space could not be found on any device.
 *
 * Remarks:
 *	Ideally, on failure we would return the largest logically
 *	contiguous piece of swap space currently available without
 *	`ripping off' any currently in-core pages. This would be very
 *	expensive. Our caller needs to make an arbitrary decision on
 *	how to back off.
 */
boolean_t
swap_alloc(int npages, swaploc_t *swaplocp)
{
	swapinfo_t *sip;
	uint_t bitno, first_bit, scan_bits, found_bit;
	int visits = 0;
	uint_t *map;

	ASSERT(LOCK_OWNED(&swap_lck));

	/*
	 * Start searching for npages worth of logically contiguous swap space
	 * on the first swap device we find that has it. We start at the one
	 * pointed to by `nextswap.' We look at all swap devices exactly once.
	 */

	for (;;)  {
		/*
		 * Advance to the next swap device.
		 */
		if (++swapcursor >= eswaptab)
			swapcursor = swaptab;

		/*
		 * Skip over holes in swaptab.
		 */
		if ((sip = *swapcursor) == NULL)
			continue;

		/*
		 * Skip devices not yet fully configured and devices
		 * in deletion.
		 */
		if (sip->si_flags & (ST_NOTREADY|ST_INDEL))
			continue;

		/*
		 * Every time we search a true swap device
		 * it counts as a ``visit''.
		 */
		++visits;

		/*
		 * Enough room?
		 */
		if (sip->si_nfpgs < npages) {
			if (visits == nswapfiles)
				return (B_FALSE);
			continue;
		}

		/*
		 * Is it contiguously available?
		 */
		first_bit = sip->si_cursor * NBITPW;
		map = &sip->si_pgmap[sip->si_cursor];
		scan_bits = sip->si_npgs - first_bit;
		if (npages <= scan_bits &&
		    (bitno = BITMASKN_ALLOCRANGE(map, scan_bits, npages)) != -1)
			break;

		/*
		 * If the first scan was from the middle of the map, then
		 * a second scan from the beginning is needed. In that case,
		 * scan the map from the beginning to the cursor.
		 */
		if (first_bit == 0) {
			if (visits == nswapfiles)
				return (B_FALSE);
			continue;
		}
		scan_bits = first_bit + npages - 1;
		if (scan_bits > sip->si_npgs)
			scan_bits = sip->si_npgs;
		first_bit = 0;
		map = &sip->si_pgmap[0];
		if ((bitno = BITMASKN_ALLOCRANGE(map, scan_bits, npages)) != -1)
			break;

		if (visits == nswapfiles)
			return (B_FALSE);
	}

	/*
	 * We found space in this device. Adjust nfpgs,
	 * fill out outarg pointers.
	 */
	sip->si_nfpgs -= npages;
	ASSERT(sip->si_nfpgs >= 0);

	found_bit = bitno + first_bit;
	SWAPLOC_MAKE(swaplocp, ptob(found_bit), swapcursor - swaptab);
	ASSERT(SWAPLOC_TO_OFF(swaplocp) < sip->si_eoff);

	/*
	 * Advance si_cursor.
	 */
	sip->si_cursor = (found_bit + npages) / NBITPW;

	return (B_TRUE);
}

/*
 * void
 * swap_free(int npages, swaploc_t *swaplocp, boolean_t breakdouble)
 *	Return npages worth of space back to the device
 *	indicated by the sip->si_rvp and starting at off.
 *	If breakdouble is B_TRUE we also adjust swapdoubles.
 *
 * Calling/Exit State:
 *	No MP-specific state is required on entry or held on exit.
 *	swap_lck is acquired to protect the bit map and nfpgs field
 *	of the swapinfo_t. swap_lck is also acquired to increment
 *	nswappgfree and decrement swapdoubles.
 */
void
swap_free(int npages, swaploc_t *swaplocp, boolean_t breakdouble)
{
	pl_t ospl;
	int bitno;
	swapinfo_t *sip = swaptab[SWAPLOC_TO_IDX(swaplocp)];

	ASSERT((SWAPLOC_TO_OFF(swaplocp) & PAGEOFFSET) == 0);
	ASSERT(sip != NULL);

	/*
	 * Calculate bitno adjusting for zero origin of map
	 */

	bitno = btop(SWAPLOC_TO_OFF(swaplocp));

	/*
 	 * Lock the bitmap, return the space, increment the counters
 	 * and then outta here.
 	 */

	ospl = LOCK(&swap_lck, VM_SWAP_IPL);

	BITMASKN_FREERANGE(sip->si_pgmap, bitno, npages);

	sip->si_nfpgs += npages;
	ASSERT(sip->si_nfpgs <= sip->si_npgs);

	/*
	 * If the swap file is being deleted, then we aren't actually freeing
	 * up space for reuse.
	 */
	if (!(sip->si_flags & ST_INDEL))
		nswappgfree += npages;

	/*
	 * If we are throwing away swap component of these pages but
	 * retaining memory or they are no longer doubly associated. Adjust
	 * counter. See anon_pageout and page_swapreclaim for more details.
	 */

	if (breakdouble)
		swapdoubles -= npages;

	UNLOCK(&swap_lck, ospl);

	return;
}
