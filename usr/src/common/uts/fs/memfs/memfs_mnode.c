/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/memfs/memfs_mnode.c	1.19"
#ident	"$Header: $"

#include <fs/memfs/memfs.h>
#include <fs/memfs/memfs_hier.h>
#include <fs/memfs/memfs_mnode.h>
#include <fs/memfs/memfs_mkroot.h>
#include <fs/mode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/anon.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/swap.h>
#include <proc/disp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/bootinfo.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern int memfs_files;
extern int memfsfstype;

extern void memfs_mapfree(mem_vfs_t *, ino_t);
extern long memfs_mapalloc(mem_vfs_t *);
extern void memfs_dirtrunc(mem_vfs_t *, mnode_t *, boolean_t);
extern void memfs_expand_bs(mnode_t *, size_t);
extern void memfs_delabort_cleanup(void);
extern swaploc_t *memfs_lookup_bs(mnode_t *, off_t);
extern	paddr_t	bootinfo_loc;

#define pbootinfo	(*((struct bootinfo *)bootinfo_loc))

/*
 * All active mnodes are linked onto a doubly linked list.
 * This list contains a permanent list marker, called the ``mnode_anchor''.
 *
 * Similarly, all inactive mnodes are linked onto another doubly linked list.
 * This list contains a permanent list marker, called the ``mnode_ianchor''.
 * Some of the inactive mnodes are marked as MEMFS_DELABORT. A count of
 * such nodes is maintained in memfs_ndelabort.
 *
 * Both mnode lists and memfs_ndelabort are mutexed by the memfs_list_lck.
 */
mnode_header_t	mnode_anchor;
mnode_header_t	mnode_ianchor;
int		memfs_ndelabort;
lock_t		memfs_list_lck;
sv_t		memfs_sv;
sleep_t		memfs_renamelock;
struct seg	memfs_seg;
memfs_bs_t	memfs_empty_node[MEMFS_KLUSTER_NUM];
size_t		memfs_maxbsize[MEMFS_LEVELS];

size_t		memfsmeta_size;	/* size meta raw data in bytes */
vaddr_t		memfsroot_mp;	/* vaddr memfs root meta data */
vaddr_t		memfsroot_fsp;	/* vaddr memfs root filesystem data */
page_t		*memfsmeta_plist; /* page struct list for meta data */
size_t		memfsroot_fssize; /* size filesystem raw data */
memfs_image_t	*memfsroot_wmp = NULL;	/* working meta data pointer */
/*
 * mnode rwlock 
 */
STATIC LKINFO_DECL(memfs_rw_lkinfo, "MP:memfs:mno_rwlock", 0);

/*
 * mnode getpage lock
 */
STATIC LKINFO_DECL(memfs_getpage_lkinfo, "MP:memfs:mno_getpagelock", 0);

/*
 * mnode list lock
 */
STATIC LKINFO_DECL(memfs_list_lkinfo, "MP:memfs:memfs_list_lck", 0);

/*
 * memfs rename lock
 */
STATIC LKINFO_DECL(memfs_rename_lkinfo, "MP:memfs:memfs_renamelock", 0);


/*
 * The following are patchable variables limiting the amount of system
 * resources memfs can use.
 *
 * memfs_maxkmem limits the amount of kernel kmem_alloc memory
 * memfs can use for it's data structures (e.g. memfs node, directory entries).
 * It is not determined by setting a hard limit but rather as a percentage of
 * physical memory which is determined when memfs is first used in the system.
 *
 * memfs_minfree is the minimum amount of swap space that memfs leaves for
 * the rest of the system.  In other words, if the amount of free swap space
 * in the system (i.e. anoninfo.ani_free) drops below memfs_minfree, memfs
 * anon allocations will fail.
 *
 * There is also a per mount limit on the amount of swap space
 * (mem_vfs.mem_anonmax) settable via a mount option.
 */
u_int memfs_minfree = 0;

/*
 * Mutex to protect memfs global data (e.g. mount list, statistics)
 */
fspin_t memfs_mutex;

/*
 * void
 * memfs_init(struct vfssw *vswp, int fstype)
 *	Early initialization for the memfs file system.
 *
 * Calling/Exit State:
 *	Called from fs_init() during kernel initialization.
 */

/* ARGSUSED */
void
memfs_init(struct vfssw *vswp, int fstype)
{
	size_t size;
	memfs_bs_t *bsnp;
	int i;

	memfsfstype = fstype;

	/*
	 * Initialize the table of backing store coverage sizes (for a node
	 * at each level of a backing store tree).
	 */
	size = PAGESIZE;
	for (i = 0; i < MEMFS_MAXLEVEL; ++i) {
		size *= MEMFS_BNODE_SIZE;
		memfs_maxbsize[i] = size;
	}
	memfs_maxbsize[MEMFS_MAXLEVEL] = ULONG_MAX;

	/*
	 * Initialize the global lock and synchronization variable for memfs.
	 */
	LOCK_INIT(&memfs_list_lck, FS_MEMFS_HIER, FS_MEMFS_PL,
		  &memfs_list_lkinfo, KM_NOSLEEP);
	SV_INIT(&memfs_sv);
	SLEEP_INIT(&memfs_renamelock,
		  (uchar_t) 0, &memfs_rename_lkinfo, KM_SLEEP);

	/*
	 * Initialize the active and inactive mnode lists.
	 */
	mnode_anchor.mnh_nextp = mnode_anchor.mnh_lastp = &mnode_anchor;
	mnode_anchor.mnh_flags = MEMFS_ANCHOR;
	mnode_ianchor.mnh_nextp = mnode_ianchor.mnh_lastp = &mnode_ianchor;
	mnode_ianchor.mnh_flags = MEMFS_ANCHOR;

	/*
	 * Initialize the dummy segment driver.
	 */
	memfs_seg.s_ops = &segmemfs_ops;

	/*
	 * Initialize the prototype for empty backing store tree nodes.
	 */
	for (bsnp = memfs_empty_node;
	    bsnp < &memfs_empty_node[MEMFS_KLUSTER_NUM];
	    ++bsnp) {
		SWAPLOC_MAKE_EMPTY(&bsnp->mbs_swaploc);
	}

	/*
	 * Associate vfs opererations.
	 */
	vswp->vsw_vfsops = &memfs_vfsops;

}

/*
 * memfs_kmadv()
 *      Call kmem_advise() for allocation sizes frequently used by memfs.
 *
 * Calling/Exit State:
 *      Called at sysinit time while still single threaded.
 */

void
memfs_kmadv()
{
        kmem_advise(sizeof(mnode_unnamed_t) +
		    MEMFS_BNODE_SIZE * sizeof(memfs_bs_t));
	kmem_advise(MEMFS_BNODE_BYTES);
}

/*
 * vnode_t *
 * memfs_create_unnamed(size_t size, uint_t flags)
 *	Create an mnode for an unnamed memfs (regular) file, and return
 *	the corresponding vnode.
 *
 * Calling/Exit State:
 *	``size'' specifies the initial length of the file in bytes.
 *
 *	The following flags are supported:
 *
 *		MEMFS_FIXEDSIZE		The file size will be fixed for
 *					the entire life of the file.
 *
 *		MEMFS_NPGZERO		New pages of the file will not
 *					be zeroed. The caller does not
 *					care what initial contents might
 *					be in the file.
 *
 *		MEMFS_NPASTEOF		The caller will never access pages
 *					past the end of file.
 *
 *		MEMFS_DMA		Pages are to be P_DMA.
 *
 *	On success, a pointer to a newly created vnode (with a VN_HOLD
 *	count of 1) is returned. On failure, NULL is returned.
 *
 *	This function acquires btopr(size) M_SWAP reservations.
 *	This function can fail in only one way: failure to acquire the
 *	M_SWAP reservations. Thus, if size == 0, this function cannot fail.
 *
 *	Called at PLBASE with no spin LOCKs held and returns that way.
 */

vnode_t *
memfs_create_unnamed(size_t size, uint_t flags)
{
	mnode_t *mp;
	uint_t pgs, lvl0pgs;
	swaploc_t *bsp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Do we need a memory reservation?
	 */
	pgs = btopr(size);
	if ( flags & MEMFS_NORESV )
		flags = 0;
	else
		if (size != 0 && !mem_resv(pgs, M_SWAP))
			return (NULL);

	/*
	 * Allocate and initialize the mnode structure.
	 *
	 * We pre-allocate a single backing store tree node. This node is
	 * allocated together with the mnode in a single kmem_alloc(). If
	 * this is for a fixed size file, then we size this first node to
	 * exactly provide for the entire file - so that the backing store
	 * tree need never grow.
	 *
	 * ALIGNMENT NOTE: this joint allocation of two data structures
	 *		   is acceptable because the first data structure
	 *		   contains a long member.
	 */
	if (flags & MEMFS_FIXEDSIZE) {
		lvl0pgs = pgs;
		mp = kmem_alloc(sizeof(mnode_unnamed_t) +
					lvl0pgs * sizeof(memfs_bs_t),
				KM_SLEEP);
		mp->mno_bsnp = (memfs_bs_t *)((vaddr_t)mp +
					      sizeof(mnode_unnamed_t));
		mp->mno_bsize = ptob(lvl0pgs);
		bsp = &mp->mno_bsnp->mbs_swaploc;
		while (lvl0pgs-- != 0) {
			SWAPLOC_MAKE_EMPTY(bsp);
			++bsp;
		}
	} else {
		mp = kmem_alloc(sizeof(mnode_unnamed_t) +
					MEMFS_BNODE_SIZE * sizeof(memfs_bs_t),
				KM_SLEEP);
		mp->mno_bsnp = (memfs_bs_t *)((vaddr_t)mp +
					      sizeof(mnode_unnamed_t));
		mp->mno_bsize = MEMFS_KLUSTER_SIZE;
		bcopy(memfs_empty_node, mp->mno_bsnp, MEMFS_BNODE_BYTES);
	}
	mp->mno_maxlevel = 0;


	/*
	 * Initialize the rest of the mnode.
	 */
	MNODE_INIT_UNNAMED(mp, flags, size);
	MNODE_TO_VP(mp)->v_softcnt = 1; /* one soft hold for the mnode list */

#ifndef NO_RDMA
	/*
	 * In the RESTRICTED DMA case, we may be asked to place a file
	 * in DMAable memory. In this case, change the page flags
	 * accordingly.
	 */
	if (flags & MEMFS_DMA)
		mp->mno_page_flags = P_DMA;
#endif /* NO_RDMA */

	/*
	 * If we need to do so, extend the backing store tree to the
	 * required size.
	 */
	if (size > mp->mno_bsize)
		memfs_expand_bs(mp, size);

	/*
	 * Add this mnode to the mnode list.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	MNODE_LIST_INSERT(MNODE_TO_MNODEH(mp), &mnode_anchor);
	UNLOCK(&memfs_list_lck, PLBASE);

	/*
	 * Cleanup delayed abort mnodes.
	 *
	 *	Note that we need not hold the memfs_list_lck while
	 *	sampling memfs_ndelabort because (i) our notion of its
	 *	value cannot be too stale since we just acquired the
	 *	memfs_list_lck moments ago, (ii) memfs_delabort_cleanup()
	 *	can tolerant a false call, and (iii) if we miss a
	 *	MEMFS_DELABORT mnode here, we will just pick it up later.
	 */
	if (memfs_ndelabort > 0)
		memfs_delabort_cleanup();

	return (MNODE_TO_VP(mp));
}

/*
 * memfs_bind(vnode_t *vp, size_t size, page_t *pp)
 *	Binding in a list of pages to form file contents.
 *
 * Calling/Exit State:
 *	vp specifies the vnode (i.e. mnode) to be used.
 *
 *	``size'' specifies the initial length of the file in bytes.
 *
 *	pp specifies a list of exactly btopr(size) pages, each of which is
 *	PAGE_WRITELOCKed, but has no identity. The caller holds an M_BOTH
 *	reservation for each of these pages. After this function returns,
 *	the pages are page_unlock()ed, and have identities <vp, 0>, <vp,
 *	PAGESIZE>, ..., <vp, ptob(btopr(PAGESIZE)) - PAGESIZE>, where vp is
 *	the vnode returned by this function. For each page on the pp list,
 *	the M_REAL component of this reservation is released. Control
 *	of the M_SWAP component passes to the memfs file system.
 *
 *	The caller is responsible for ensuring that the last
 *	(ptob(btopr(size)) - size) bytes of the last page on the pp list
 *	are zero.
 *
 * 	This function cannot fail.
 *
 * 	Called at PLBASE with no spin LOCKs held and returns that way.
 *
 *
 */

vnode_t *
memfs_bind(vnode_t *vp, size_t size, page_t *pp)
{
	mnode_t *mp = VP_TO_MNODE(vp);
	page_t *epp;
	off_t offset;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * If we need to do so, extend the backing store tree to the
	 * required size.
	 */
	if (size > mp->mno_bsize)
		memfs_expand_bs(mp, size);
	
	/*
	 * Process each page on the pp list.
	 */
	offset = 0;
	while (pp) {
		/*
		 * Pick the first page off the list.
		 */
		epp = pp->p_prev;
		ASSERT(epp);
		page_sub(&pp, epp);
		ASSERT(PAGE_IS_WRLOCKED(epp));
		ASSERT(epp->p_vnode == NULL);
		ASSERT(epp->p_offset == 0);
		ASSERT(btopr(offset + PAGESIZE) <= btopr(size));

		/*
		 * Give the page <vp, offset> identity.
		 */
		page_assign_identity(epp, vp, offset);

		/*
		 * Mark the page as dirty and then free it. This sends the
		 * page to the dirtyflist.
		 */
		page_setmod(epp);
		page_unlock(epp);

		/*
		 * All done with this page. Onto the next page.
		 */
		offset += PAGESIZE;
	}

	/*
	 * Release the M_REAL reservations associated with the pages.
	 */
	mem_unresv(btop(offset), M_REAL);

}

void
mnode_rele(mnode_t *mp)
{
	vnode_t *vp = MNODE_TO_VP(mp);

	/*
	 * This thread shouldn't be holding the contents lock
	 * on this mnode because inactive could be called
	 * via vn_rele
	 */
	VN_RELE(vp);
}

/*
 * mnode_t *
 * mnode_alloc(mem_vfs_t *mem_vfsp, struct vattr *vap, struct cred *cred)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Allocate a mnode and add it to mnode list under mount point.
 *	Returns initialized, excl locked and held mnode on success.
 */

mnode_t *
mnode_alloc(mem_vfs_t *mem_vfsp, struct vattr *vap, struct cred *cred)
{
	mnode_t *mp;
	extern struct vnodeops memfs_vnodeops;
	vnode_t *vp;
	pl_t pl;

	ASSERT(vap != NULL);
	ASSERT(cred != NULL);
	/*
	 * No spin locks should be held by this thread.
	 */
	mp = kmem_zalloc(sizeof(mnode_t) +
				MEMFS_BNODE_SIZE * sizeof(memfs_bs_t),
			KM_SLEEP);
	mp->mno_bsnp = (memfs_bs_t *)((vaddr_t)mp +
				      sizeof(mnode_t));
	mp->mno_bsize = MEMFS_KLUSTER_SIZE;
	bcopy(memfs_empty_node, mp->mno_bsnp, MEMFS_BNODE_BYTES);
	mp->mno_maxlevel = 0;

	if (mp == NULL)
		return (NULL);

	MNODE_INIT(mp, vap, cred);
	if ((mp->mno_nodeid = memfs_mapalloc(mem_vfsp)) == -1) {
		MNODE_DEINIT_COMMON(mp);
		kmem_free(mp, sizeof(mnode_t) +
                                MEMFS_BNODE_SIZE * sizeof(memfs_bs_t));
		return (NULL);
	}
	mp->mno_flag = TACC|TUPD|TCHG;
	memfs_timestamp(mp);
	mp->mno_nlink = 0;
	mp->mno_fsid = mem_vfsp->mem_dev;
	mp->mno_rdev = 0;

	vp = MNODE_TO_VP(mp);
	VN_INIT(vp, mem_vfsp->mem_vfsp, vap->va_type, vap->va_rdev,
		VSWAPBACK | VNOSYNC, KM_SLEEP);
	vp->v_softcnt = 1; /* one soft hold for the mnode list */

	pl = LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
	/*
	 * Increment the pseudo generation number for this mnode.
	 * Since mnodes are allocated and freed, there really is no
	 * particular generation number for a new mnode.  Just fake it
	 * by using a counter in each file system.
	 */
	mp->mno_gen = mem_vfsp->mem_gen++;

	switch (vap->va_type) {
	case VDIR:
		mem_vfsp->mem_directories++;
		break;
	case VBLK:
	case VCHR:
	case VXNAM:
		mp->mno_rdev = vap->va_rdev;
		/* FALLTHROUGH */
	case VREG:
	case VLNK:
	case VFIFO:
		mem_vfsp->mem_files++;
		break;
	default:
		/*
		 *+ Unknown file type when trying to allocate a memfs.
		 *+ node. This indicates either a software bug or memory
		 *+ corruption. Corrective action - reboot.
		 */
		cmn_err(CE_PANIC, "mnode_alloc: unknown file type 0x%x\n",
								vap->va_type);
		break;
	}
	UNLOCK(&mem_vfsp->mem_contents, pl);

	ASSERT(!RWSLEEP_LOCKBLKD(&mp->mno_rwlock));
	RWSLEEP_TRYWRLOCK(&mp->mno_rwlock);
	/*
	 * Add this mnode to the mnode list.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	MNODE_LIST_INSERT(MNODE_TO_MNODEH(mp), &mnode_anchor);
	UNLOCK(&memfs_list_lck, PLBASE);

	/*
	 * Return it held, locked and referenced.
	 */
	return (mp);
}

/*
 * void
 * mnode_free(mem_vfs_t *mem_vfsp, mnode_t *mp)
 *
 * Calling/Exit State:
 *	The mnode rwlock is held excl. on entry and at exit.
 *
 * Description:
 * 	Free mnode and all its associated anonymous memory (if any).
 *
 */
/*ARGSUSED*/
void
mnode_free(struct mem_vfs *mem_vfsp, mnode_t *mp)
{
	pl_t pl;

	ASSERT(mp->mno_nlink == 0);
	ASSERT(MNODE_TO_VP(mp)->v_count == 0);
	switch (MNODE_TO_VP(mp)->v_type) {
	case VDIR:
		pl = LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
		mem_vfsp->mem_directories--;
		UNLOCK(&mem_vfsp->mem_contents, pl);
		break;
	case VLNK:
		mp->mno_size = 0;
		pl = LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
		mem_vfsp->mem_files--;
		UNLOCK(&mem_vfsp->mem_contents, pl);
		break;
	case VFIFO:
	case VBLK:
	case VCHR:
	case VREG:
	case VXNAM:
		pl = LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
		mem_vfsp->mem_files--;
		UNLOCK(&mem_vfsp->mem_contents, pl);
		break;
	default:
		/*
		 *+ Unknown file type when trying to free a memfs node.
		 *+ This indicates either software bug or memory
		 *+ corruption. Corrective action - reboot.
		 */
		cmn_err(CE_PANIC, "mnode_free: unknown file type 0x%x\n", mp);
		break;
	}

	memfs_mapfree(mem_vfsp, mp->mno_nodeid);
}

/*
 * int
 * mnode_get(vfs_t *vfsp, uint_t ino, uint_t gen, vnode_t **vpp)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Lookup for a mnode with number "ino".
 *	Returns held mnode if found.
 */
int
mnode_get(vfs_t *vfsp, uint_t ino, uint_t gen, vnode_t **vpp)
{
	struct mem_vfs *mem_vfsp = (struct mem_vfs *)VFSTOTM(vfsp);
        mnode_header_t *mnhp;
        mnode_t *mp;

	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
        mnhp = mnode_anchor.mnh_nextp;
        for (;;) {
                /*
                 * Locate the next mnode to relocate. We need to skip
                 * over all markers on the mnode list.
                 *
                 * Note that any newly created mnodes will end up behind
                 * our marker on the active mnode list. Thus, we will ignore
                 * them. This is proper, since such mnodes cannot use swap
                 * space from a swap file which is being deleted.
                 */
                while (mnhp->mnh_flags & MEMFS_MARKER)
                        mnhp = mnhp->mnh_nextp;

                /*
                 * If we see the anchor, then we are done with the active
                 * mnode list.
                 */
                if (mnhp->mnh_flags & MEMFS_ANCHOR)
                        break;

		if ((mnhp->mnh_flags & MEMFS_UNNAMED) == 0) {
			mp = (mnode_t *)mnhp;
			if (mp->mno_fsid == mem_vfsp->mem_dev &&
			    mp->mno_nodeid == ino) {
				RWSLEEP_RDLOCK_RELLOCK(&mp->mno_rwlock,
						 PRINOD, &memfs_list_lck);
				/*
				 * If the gen numbers don't match we know the
				 * file won't be found since only one memfs
				 * node can have this number at a time.
				 */
				if (mp->mno_gen != gen) {
					RWSLEEP_UNLOCK(&mp->mno_rwlock);
					return -1;
				}
				*vpp = (struct vnode *)MNODE_TO_VP(mp);
				VN_HOLD(*vpp);
				RWSLEEP_UNLOCK(&mp->mno_rwlock);
				return 0;
			}
		}
		mnhp = mnhp->mnh_nextp;
	}
	UNLOCK(&memfs_list_lck, PLBASE);
	return -1;
}

/*
 * int
 * int memfs_mflush(vfs_t *vfsp)
 *	Flush out all mnodes associated with a memfs file system as part
 *	of an unmount.
 *
 * Calling/Exit State:
 *	The vfslist_lock (sleep_t) is held.
 *
 *	No spin LOCKs are held on entry to or exit from this function.
 *
 * Description:
 *	The effect of this function is to delete all files in the affected
 *	file system
 */
memfs_mflush(vfs_t *vfsp)
{
	mem_vfs_t *mem_vfsp = (mem_vfs_t *)VFSTOTM(vfsp);
	mnode_header_t *markerp, *mnhp;
	mnode_t *mp;
	vnode_t *rvp, *vp;
	uint_t vcount;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Now scan the active list, looking for any file in this file system
	 * which is open (i.e. has an elevated v_count). Note that for
	 * anyone to establish a new VN_HOLD within a memfs file system,
	 * they must already be 
	 */
	rvp = MNODE_TO_VP(mem_vfsp->mem_rootnode);
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	mnhp = mnode_anchor.mnh_nextp;
	for (;;) {
		/*
		 * We need to skip over all markers on the mnode list.
		 */
		while (mnhp->mnh_flags & MEMFS_MARKER)
			mnhp = mnhp->mnh_nextp;

		/*
		 * If we see the anchor, then we are done with the active
		 * mnode list.
		 */
		if (mnhp->mnh_flags & MEMFS_ANCHOR)
			break;

		mp = MNODEH_TO_MNODE(mnhp);
		vp = MNODE_TO_VP(mp);

		/*
		 * skip over mnodes not in the demounting file system
		 */
		if ((mp->mno_flags & MEMFS_UNNAMED) || vp->v_vfsp != vfsp) {
			mnhp = mnhp->mnh_nextp;
			continue;
		}

		/*
		 * v_count should be two for the root vnode and 0 for all
		 * other vnodes.
		 */
		VN_LOCK(vp);
		vcount = vp->v_count;
		VN_UNLOCK(vp);
		if (vcount != ((vp == rvp) ? 2 : 0)) {
			UNLOCK(&memfs_list_lck, PLBASE);
			return EBUSY;
		}

		mnhp = mnhp->mnh_nextp;
	}
	UNLOCK(&memfs_list_lck, PLBASE);

	/*
	 * Quiescent verification has passed. We are now committed to the
	 * unmount.
	 */

	/*
	 * allocate a list marker.
	 */
	markerp = kmem_alloc(sizeof(mnode_header_t), KM_SLEEP);
	markerp->mnh_flags = MEMFS_MARKER;

	/*
	 * Rescan the active list, this time shredding all directories.
	 * This will have the effect of deleting all files and directories.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	mnhp = mnode_anchor.mnh_nextp;
	for (;;) {
		/*
		 * Locate the next mnode to relocate. We need to skip
		 * over all markers on the mnode list.
		 *
		 * Note that any newly created mnodes will end up behind
		 * our marker on the active mnode list. Thus, we will ignore
		 * them. This is proper, since such mnodes cannot use swap
		 * space from a swap file which is being deleted.
		 */
		while (mnhp->mnh_flags & MEMFS_MARKER)
			mnhp = mnhp->mnh_nextp;

		/*
		 * If we see the anchor, then we are done with the active
		 * mnode list.
		 */
		if (mnhp->mnh_flags & MEMFS_ANCHOR)
			break;

		mp = MNODEH_TO_MNODE(mnhp);
		vp = MNODE_TO_VP(mp);

		/*
		 * skip over mnodes not in the demounting file system,
		 * or which do not represent directories
		 */
		if ((mp->mno_flags & MEMFS_UNNAMED) || vp->v_vfsp != vfsp
		    || vp->v_type != VDIR) {
			mnhp = mnhp->mnh_nextp;
			continue;
		}

		/*
		 * Insert our marker into the list.
		 */
		MNODE_LIST_INSERT(markerp, mnhp);

		/*
		 * shred the directory
		 */
		VN_HOLD(MNODE_TO_VP(mp));
		UNLOCK(&memfs_list_lck, PLBASE);
		RWSLEEP_WRLOCK(&mp->mno_rwlock, PRINOD);
		memfs_dirtrunc(mem_vfsp, mp, B_TRUE);

		/*
		 * Delete our marker and continue.
		 */
		(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
		mnhp = markerp->mnh_nextp;
		MNODE_LIST_DELETE(markerp);
	}
	mnhp = mnode_ianchor.mnh_nextp;

	for (;;) {
		/*
		 * Ignore other markers.
		 */
		while (mnhp->mnh_flags & MEMFS_MARKER)
			mnhp = mnhp->mnh_nextp;

		/*
		 * If we see the anchor, then we are done with the inactive
		 * mnode list.
		 */
		if (mnhp->mnh_flags & MEMFS_ANCHOR)
			break;

		/*
		 * skip over mnodes not in the demounting file system
		 */
		if ((mp->mno_flags & MEMFS_UNNAMED) || vp->v_vfsp != vfsp) {
			mnhp = mnhp->mnh_nextp;
			continue;
		}

		/*
		 * Insert our marker into the list.
		 */
		MNODE_LIST_INSERT_PREV(markerp, mnhp);

		/*
		 * Wait for somebody to finish inactivating an mnode.
		 */
		SV_WAIT(&memfs_sv, PRINOD, &memfs_list_lck);

		/*
		 * Delete our marker.
		 */
		(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
		mnhp = markerp->mnh_nextp;
		MNODE_LIST_DELETE(markerp);
	}
	UNLOCK(&memfs_list_lck, PLBASE);

	/*
	 * Free up the marker.
	 */
	kmem_free(markerp, sizeof(mnode_header_t));

	return 0;
}

/*
 * page_t **
 * memfs_getparm( struct krdata *krdp, vaddr_t *addr, size_t *size));
 *	Scan the memfs root raw data setting vaddr and size.
 *
 * Calling/Exit State:
 *
 *	No spin LOCKs are held on entry to or exit from this function.
 *
 * Description:
 *	This routine is called from pagepool_init() during system 
 *	initialization to create associated page list.
 *
 * 	N.B. this routine must be first called with MEMFS_META.
 */

page_t ** 
memfs_getparm( struct krdata *krdp, vaddr_t *addr, size_t *size)
{
	page_t **plist = 0;
	int	j;

	switch ( krdp->type ){
	case MEMFSROOT_META:
		*addr = memfsroot_mp = krdp->vaddr;
		*size = memfsmeta_size = krdp->size;
		memfsroot_wmp = (memfs_image_t *)memfsroot_mp;
		plist = &memfsmeta_plist;
		for(j = 0; pbootinfo.kd[j].type != MEMFSROOT_FS; j++ );
		memfsroot_fsp = pbootinfo.kd[j].vaddr;
		memfsroot_fssize = 0;
		break;
	case MEMFSROOT_FS:
		while ( memfsroot_wmp->mi_type != VNON ){
			if ( memfsroot_wmp->mi_type == VREG ){
				plist = (page_t **)&(memfsroot_wmp->mi_plist);
				*addr = memfsroot_fsp + memfsroot_fssize;
				*size = (size_t)(ptob(btopr(
						memfsroot_wmp->mi_size)));
				memfsroot_fssize += *size;
				memfsroot_wmp++;
				break;
			}else
				memfsroot_wmp++;
		}
		break;
	};
	return (plist);
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_mnode_bs(memfs_bs_t bsnp, size_t size)
 *	Print out the backing store information from a given backing store
 *	tree node.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Thus function is intended for use from a kernel debugger.
 */

void
print_mnode_bs(memfs_bs_t *bsnp, size_t size)
{
	off_t off = 0;

	while (off < size) {
		if (!SWAPLOC_IS_EMPTY(&bsnp->mbs_swaploc)) {
			if (SWAPLOC_IS_IOERR(&bsnp->mbs_swaploc))
				debug_printf("       IOERR at offset %x\n",
					     off);
			else
				debug_printf(
					"       swap <%x, %x> at offset %x\n",
					SWAPLOC_TO_IDX(&bsnp->mbs_swaploc),
					SWAPLOC_TO_OFF(&bsnp->mbs_swaploc),
					off);
			if (debug_output_aborted())
				break;
		}
		off += PAGESIZE;
		++bsnp;
	}
}

/*
 * void
 * print_mnode(mnode_t *mp)
 *	Print out the contents of an mnode.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Thus function is intended for use from a kernel debugger.
 */

void
print_mnode(mnode_t *mp)
{
	vnode_t *vp = MNODE_TO_VP(mp);
	char buf[128];
	boolean_t early_ret = B_FALSE;
	int level, shift, minshift, maxshift;
	size_t inc;
	off_t off, off1;
	memfs_bs_t *bsnp;

	debug_printf("Contents of mnode at %x:\n", mp, vp);

	buf[0] = '\0';
	if (mp->mno_flags & MEMFS_FIXEDSIZE)
		strcat(buf, "FIXEDSIZE ");
	if (mp->mno_flags & MEMFS_UNNAMED)
		strcat(buf, "UNNAMED ");
	if (mp->mno_flags & MEMFS_NPGZERO)
		strcat(buf, "NPGZERO ");
	if (mp->mno_flags & MEMFS_MARKER) {
		strcat(buf, "MARKER ");
		early_ret = B_TRUE;
	}
	if (mp->mno_flags & MEMFS_ANCHOR) {
		strcat(buf, "ANCHOR ");
		early_ret = B_TRUE;
	}
	if (mp->mno_flags & MEMFS_NPASTEOF)
		strcat(buf, "NPASTEOF ");
	if (mp->mno_flags & MEMFS_NSINACT)
		strcat(buf, "NSINACT ");
	if (mp->mno_flags & MEMFS_DELABORT)
		strcat(buf, "DELABORT ");
#ifdef NO_RDMA
	if (mp->mno_flags & MEMFS_DMA)
		strcat(buf, "DMA ");
#endif /* NO_RDMA */
	if (mp->mno_flags & ~MEMFS_ALL_FLAGS)
		strcat(buf, "*** illegal flags *** ");
	debug_printf("    flags = %x ( %s)\n", mp->mno_flags, buf);
	if (early_ret)
		return;

	debug_printf("    vnode at %x\n", vp);

	if ((mnode_t *)vp->v_data != mp) {
		debug_printf("    *** bad v_data pointer (%x) ***\n",
			     vp->v_data);
		return;
	}

	if (!(vp->v_flag & VSWAPBACK)) {
		debug_printf("    *** NOT an mnode: VSWAPBACK not set ***\n");
		return;
	}

	if (!IS_MEMFSVP(vp)) {
		debug_printf("    *** NOT an mnode:"
			      " vp->v_op != memfs vops ***\n");
		return;
	}

	debug_printf("    next = %x, last = %x\n",
		     mp->mno_nextp, mp-> mno_lastp);

	debug_printf("    size = %x, bsize = %x, maxlevel = %d\n",
	             mp->mno_size, mp->mno_bsize, mp->mno_maxlevel);

	if (!(mp->mno_flags & MEMFS_UNNAMED))
		debug_printf("    nodeis = %d, fsid = %x, gen = %d\n",
		             mp->mno_nodeid, mp->mno_fsid, mp->mno_gen);


	if (mp->mno_flags & MEMFS_FIXEDSIZE) {
		if (!debug_output_aborted())
			print_mnode_bs(mp->mno_bsnp, mp->mno_bsize);
		return;
	}

	/*
	 * Print each level from the tree, starting from the root.
	 */
	maxshift = MEMFS_SHIFT(mp->mno_maxlevel);
	for (level = mp->mno_maxlevel; level >= 0; --level) {
		inc = memfs_maxbsize[level];
		minshift = MEMFS_SHIFT(level);

		/*
		 * Delete each node at this level
		 */
		for (off = 0; off < mp->mno_bsize; off += inc) {
			bsnp = mp->mno_bsnp;
			shift = maxshift;
			off1 = off >> PAGESHIFT;
			while (shift > minshift) {
				bsnp += (off1 >> shift) & MEMFS_LVL_MASK;
				bsnp = bsnp->mbs_ptr;
				ASSERT(bsnp != NULL);
				shift -= MEMFS_LVL_SHIFT;
			}
			debug_printf("        level %d node at %x\n",
				     level, bsnp);
			if (level == 0)
				print_mnode_bs(bsnp, MEMFS_KLUSTER_SIZE);
			if (debug_output_aborted())
				return;
		}
	}
}

/*
 * void
 * print_vnode_mnode(vnode_t *vp)
 *	Print out an mnode, give the corresponding vnode.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Thus function is intended for use from a kernel debugger.
 */

void
print_vnode_mnode(vnode_t *vp)
{
	print_mnode(VP_TO_MNODE(vp));
}

/*
 * void
 * print_mnodes(void)
 *	Search the active and inactive lists to print out all mnodes
 *	in the system.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Thus function is intended for use from a kernel debugger.
 */

void
print_mnodes(void)
{
	mnode_header_t *mhp;

	debug_printf("ACTIVE MNODE LIST\n");
	debug_printf("------ ----- ----\n");
	mhp = mnode_anchor.mnh_nextp;
	while (!(mhp->mnh_flags & MEMFS_ANCHOR)) {
		print_mnode(MNODEH_TO_MNODE(mhp));
		mhp = mhp->mnh_nextp;
	}

	debug_printf("INACTIVE MNODE LIST\n");
	debug_printf("-------- ----- ----\n");
	mhp = mnode_ianchor.mnh_nextp;
	while (!(mhp->mnh_flags & MEMFS_ANCHOR)) {
		print_mnode(MNODEH_TO_MNODE(mhp));
		mhp = mhp->mnh_nextp;
	}
}

#endif /* DEBUG || DEBUG_TOOLS */

#ifdef _MEMFS_HIST

/*
 * void
 * memfs_log(mnode_t *mp, off_t off, char *service, int line, char *file)
 *      Make an entry into the memfs history log for the specified mnode.
 *
 * Calling/Exit State:
 *	Caller owns a VN_HOLD on the mnode.
 */
void
memfs_log(mnode_t *mp, off_t off, char *service, int line, char *file)
{
	memfs_hist_record_t *mrp;
	swaploc_t swaploc;
	swaploc_t *bsp;

	if (off != -1) {
		ASSERT(off < mp->mno_bsize);
		bsp = memfs_lookup_bs(mp, off);
		swaploc = *bsp;
	} else {
		SWAPLOC_MAKE_EMPTY(&swaploc);
	}

	FSPIN_LOCK(&mp->mno_realloclck);
	mrp = &mp->mno_hist.mli_rec[mp->mno_hist.mli_cursor];
	if (++mp->mno_hist.mli_cursor == MEMFS_HIST_SIZE)
		mp->mno_hist.mli_cursor = 0;
	mrp->mhr_offset = off;
	mrp->mhr_swaploc = swaploc;
	mrp->mhr_service = service;
	mrp->mhr_count = MNODE_TO_VP(mp)->v_count;
	mrp->mhr_softcnt = MNODE_TO_VP(mp)->v_softcnt;
	mrp->mhr_line = line;
	mrp->mhr_file = file;
	mrp->mhr_lwp = CURRENT_LWP();
	GET_TIME(&mrp->mhr_stamp);
	FSPIN_UNLOCK(&mp->mno_realloclck);
}

/*
 * void
 * print_mnode_log(mnode_t *mp)
 *      Print the entries from the history log within the specified mnode.
 *
 * Calling/Exit State:
 *      Intended for use form a kernel debugger.
 */
void
print_mnode_log(mnode_t *mp)
{
	memfs_hist_record_t *mrp;
	ulong_t last_stamp, diff;
	char digit[9];
	int i, j;
	char c, *p;
	int last;
	int cursor = mp->mno_hist.mli_cursor;

	debug_printf("TIME    LWP      count   soft     off     bs\n"
		     "----    ---      ----    ----     ---     --\n");

	last = cursor - 1;
	if (last < 0)
		last += MEMFS_HIST_SIZE;
	last_stamp = mp->mno_hist.mli_rec[last].mhr_stamp;

	for (j = 0; j < MEMFS_HIST_SIZE; ++j) {
		mrp = &mp->mno_hist.mli_rec[cursor];
		if (++cursor == MEMFS_HIST_SIZE)
			cursor = 0;
		if (mrp->mhr_service == NULL)
			continue;
		diff = last_stamp - mrp->mhr_stamp;
		p = &digit[8];
		*p-- = '\0';
		diff /= 100;
		for (i = 1; i <= 6 || diff != 0; ++i) {
			if (i == 5)
				*p-- = '.';
			else {
				*p-- = (diff % 10) + '0';
				diff /= 10;
			}
		}
		debug_printf("-%s %lx %ld %ld ",
			     p + 1, (ulong_t)mrp->mhr_lwp,
			     (ulong_t)mrp->mhr_count,
			     (ulong_t)mrp->mhr_softcnt);
		if (mrp->mhr_offset == -1)
			debug_printf("                 ");
		else if (SWAPLOC_IS_EMPTY(&mrp->mhr_swaploc))
			debug_printf("%lx EMPTY", (ulong_t)mrp->mhr_offset);
		else if (SWAPLOC_IS_IOERR(&mrp->mhr_swaploc))
			debug_printf("%lx IOERR", (ulong_t)mrp->mhr_offset);
		else
			debug_printf("%lx <%x, %x>",
				     (ulong_t)mrp->mhr_offset,
				     SWAPLOC_TO_IDX(&mrp->mhr_swaploc),
				     SWAPLOC_TO_OFF(&mrp->mhr_swaploc));
		debug_printf(" %s from line %d of file %s\n",
			mrp->mhr_service, mrp->mhr_line, mrp->mhr_file);
		if (debug_output_aborted())
			break;
	}
}

#endif /* _MEMFS_HIST */
