/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/namefs/namesubr.c	1.12"

/*
 * This file contains the supporting routines
 * for NAMEFS file system.
 */

#include <util/types.h>
#include <util/engine.h>
#include <acc/dac/acl.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <fs/file.h>
#include <fs/fs_subr.h>
#include <fs/statvfs.h>
#include <fs/buf.h>
#include <fs/fs_hier.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/namefs/namehier.h>
#include <fs/namefs/namenode.h>
#include <io/strsubr.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/plocal.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

/*
 * The next set of lines define the bit map for obtaining 
 * unique node ids. The value chosen for the maximum node id
 * is arbitrary and should be altered to better fit the system
 * on which this file system is defined.  The maximum value can
 * not exceed the maximum value for a long, since the node id
 * must fit into a long.  (In fact, currently the routine
 * that assigns node ids restricts it to fit in a ino_t.)
 * The maximum number of mounted file descriptors at a given
 * time will be limited to NMMAXID-1 (node id 0 isn't used).
 *
 * nmap    --> bitmap with one bit per node id
 * testid  --> is this node already in use?
 * setid   --> mark this node id as used
 * clearid --> mark this node id as unused
 */

#define NMMAXID		8191
#define testid(i)	((nmmap[(i)/NBBY] & (1 << ((i)%NBBY))))
#define setid(i)	((nmmap[(i)/NBBY] |= (1 << ((i)%NBBY))), (nmids++))
#define clearid(i)	((nmmap[(i)/NBBY] &= ~(1 << ((i)%NBBY))), (nmids--))

STATIC char nmmap[NMMAXID/NBBY + 1];
STATIC ino_t nmids;
STATIC lock_t nmmap_mutex; /* protects nmmap and nmids */

/*
 * Define variables.
 */
STATIC struct namenode	*namealloc;
lock_t		namelist_mutex;	/* protects namealloc list */

/*
 *+ Lock protecting the namenode hash list, namealloc.
 */
STATIC	LKINFO_DECL(nmlist_lkinfo, "NM::namelist_mutex", 0);

/*
 *+ Lock protecting nmmap and nmids.
 */
STATIC	LKINFO_DECL(nmmap_lkinfo, "NM::nmmap_mutex", 0);

/*
 * Define the routines in this file.
 */
struct namenode *namefind(vnode_t *, vnode_t *);
void	nameinsert(struct namenode *);
int	nameremove(struct namenode *, int);
int	nm_unmountall(vnode_t *, cred_t *);
ino_t	nmgetid(void);
void	nmclearid(ino_t);

/*
 * Define external variables and routines .
 */
extern dev_t		namedev;
extern struct vfs	*namevfsp;
extern struct vfsops 	nmvfsops;
extern struct vfsops 	dummyvfsops;

extern int nm_aclstore(struct namenode *, struct acl *, long);
extern struct vnodeops nm_vnodeops;

/*
 * void nminit(register struct vfssw *vswp, int fstype)
 *	File system initialization routine.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Save the file system type, establish a file system device number 
 *	and initialize namealloc, namelist_mutex and nmmap_mutex.
 */
void
nameinit(register struct vfssw *vswp, int fstype)
{
	vswp->vsw_vfsops = &nmvfsops;
	if ((namedev = getudev()) == NODEV) {
		/*
		 *+ A unique device number cannot be established.
		 */
		cmn_err(CE_WARN, "nameinit: can't get unique device");
		namedev = 0;
	}
	namealloc = NULL;
	LOCK_INIT(&namelist_mutex, NM_HIER, PLNM, 
		&nmlist_lkinfo, KM_NOSLEEP);
	LOCK_INIT(&nmmap_mutex, NM_HIER, PLNM, 
		&nmmap_lkinfo, KM_NOSLEEP);
	namevfsp = (struct vfs *)kmem_zalloc(sizeof(struct vfs), KM_SLEEP);
	VFS_INIT(namevfsp, &dummyvfsops, (caddr_t)NULL);
	namevfsp->vfs_next = NULL;
	namevfsp->vfs_vnodecovered = NULL;
	namevfsp->vfs_bsize = 1024;
	namevfsp->vfs_fstype = fstype;
	namevfsp->vfs_fsid.val[0] = namedev;
	namevfsp->vfs_fsid.val[1] = fstype;
	namevfsp->vfs_dev = namedev;
	namevfsp->vfs_bcount = 0;
}

/*
 * void nameinsert(struct namenode *nodep)
 * 	Insert a namenode into the namealloc hash list.
 *
 * Calling/Exit State:
 *	namelist_mutex must be locked on entry and remains locked on exit.
 *
 * Description:
 *
 * 	namealloc is a doubly linked list that contains namenode
 * 	as links. Each link has a unique namenode with a unique
 * 	nm_mountvp field. The nm_filevp field of the namenode need not
 * 	be unique, since a file descriptor may be mounted to multiple
 * 	nodes at the same time.
 * 	This routine inserts a namenode link onto the front of the
 * 	linked list.
 */
void
nameinsert(struct namenode *nodep)
{
	nodep->nm_backp = NULL;
	nodep->nm_nextp = namealloc;
	namealloc = nodep;
	if (nodep->nm_nextp)
		nodep->nm_nextp->nm_backp = nodep;
}

/*
 * struct namenode * namefind(vnode_t *vp, vnode_t *mnt)
 *
 * Calling/Exit State:
 *	namelist_mutex must be locked on entry and remains locked on exit.
 *
 * Description:
 * 	Search the doubly linked list, namealloc, for a namenode that
 * 	has a nm_filevp field of vp and a nm_mountpt of mnt.
 * 	If the namenode/link is found, return the address. Otherwise,
 * 	return NULL.
 * 	If mnt is NULL, return the first link with a nm_filevp of vp.
 * 	Assume namelist_mutex is locked.
 */
struct namenode *
namefind(vnode_t *vp, vnode_t *mnt)
{
	register struct namenode *tnode;

	for (tnode = namealloc; tnode; tnode = tnode->nm_nextp)
		if (tnode->nm_filevp == vp && 
			(!mnt || (mnt && tnode->nm_mountpt == mnt)))
				break;
	return (tnode);
}

/*
 * int nameremove(struct namenode *node, int unmount)
 * 	Remove a namenode from the hash table.
 *
 * Calling/Exit State:
 *	namelist_mutex must be locked on entry and remains locked on exit.
 *
 * Description:
 * 	If nodep is the only node on the list, set namealloc to NULL.
 * 	Return 0 if nodep is remove, 
 *	      -1 if someone is blocked on nm_lock.
 */
int
nameremove(struct namenode *nodep, int inactive)
{
	if (inactive) {
		if (RWSLEEP_LOCKBLKD(&nodep->nm_lock)) {
			/* someone finds this node */
			return(-1);
		}
	}
	if (nodep == namealloc)      /* delete first link */
		namealloc = nodep->nm_nextp;
	if (nodep->nm_nextp)
		nodep->nm_nextp->nm_backp = nodep->nm_backp;
	if (nodep->nm_backp)
		nodep->nm_backp->nm_nextp = nodep->nm_nextp;
	return(0);
}

/*
 * void nmclearid(ino_t ino)
 *
 * Calling/Exit State:
 *	nmmap_mutex must not be locked on entry.
 * 	Clear the bit in the bit map corresponding to the
 * 	nodeid in the namenode.
 */
void
nmclearid(ino_t ino)
{
	register pl_t	pl;

	/*
	 * Safeguard against decrementing nmids when bit has
	 * already been cleared.
	 */
	pl = LOCK(&nmmap_mutex, PLNM);
	if (testid(ino))
		clearid(ino);
	UNLOCK(&nmmap_mutex, pl);
}

/*
 * ino_t nmgetid(void)
 *
 * Calling/Exit State:
 *	nmmap_mutex must not be locked on entry.
 *
 * Description:
 * 	Attempt to establish a unique node id. Start searching
 * 	the bit map where the previous search stopped. If a
 * 	free bit is located, set the bit and keep track of
 * 	it because it will become the new node id.
 */
ino_t
nmgetid(void)
{
	register ino_t i;
	register ino_t j;
	register pl_t	pl;
	static ino_t prev = 0;

	pl = LOCK(&nmmap_mutex, PLNM);
#ifdef CC_PARTIAL
	/*
	 * If we're concerned about covert channels, start the id search
	 * at a random place rather than where the previous search stopped.
	 * If the random() routine is stubbed out, it will return 0,
	 * in which case we want to revert to the sequential method.
	 */
	if ((i = random((u_long)NMMAXID)) == 0)
#endif
		i = prev;

	for (j = NMMAXID; j--; ) {
		if (i++ >= (ino_t)NMMAXID)
			i = 1;

		if (!testid(i)) {
			setid(i);
			prev = i;
#ifdef CC_PARTIAL
			if (nmids > (ino_t)(NMMAXID - RANDMINFREE))
				CC_COUNT(CC_RE_NAMEFS, CCBITS_RE_NAMEFS);
#endif /* CC_PARTIAL */
			UNLOCK(&nmmap_mutex, pl);
			return i;
		}
	}

#ifdef CC_PARTIAL
	CC_COUNT(CC_RE_NAMEFS, CCBITS_RE_NAMEFS);
#endif /* CC_PARTIAL */
	UNLOCK(&nmmap_mutex, pl);
	/*
	 *+ A unique namenode id cannot be established.
	 *+ This indicates the limit on number of mounted stream
	 *+ on the system has been reached.
	 */
	cmn_err(CE_WARN, "nmgetid: could not establish a unique node id\n");
	return 0;
}

/*
 * int nm_unmountall(vnode_t *vp, cred_t *crp)
 * 	Force the unmouting of a file descriptor from ALL of the nodes
 * 	that it was mounted to.
 *
 * Calling/Exit State:
 *	No locking assumption.
 *
 * Description:
 * 	At the present time, the only usage for this routine is in the
 * 	event one end of a pipe was mounted. At the time the unmounted
 * 	end gets closed down, the mounted end is forced to be unmounted.
 *
 * 	This routine searches the namealloc hash list for all namenodes 
 * 	that have a nm_filevp field equal to vp. Each time one is found,
 * 	the dounmount() routine is called. This causes the nm_unmount()
 * 	routine to be called and thus, the file descriptor is unmounted 
 * 	from the node.
 *
 * 	At the start of this routine, the reference count for vp is
 * 	incremented to protect the vnode from being released in the 
 * 	event the mount was the only thing keeping the vnode active.
 * 	If that is the case, the VOP_CLOSE operation is applied to
 * 	the vnode, prior to it being released.
 */
int
nm_unmountall(vnode_t *vp, cred_t *crp)
{
	register vfs_t *vfsp;
	register struct namenode *nodep;
	register struct namenode *nextp;
	register pl_t pl;
	register error = 0;
	int realerr = 0;
	int done;

	/*
	 * For each namenode that is associated with the file:
	 * If the v_vfsp field is not namevfsp, dounmount it.  Otherwise,
	 * it was created in nm_open() and will be released in time.
	 * The following loop replicates some code from nm_find.  That
	 * routine can't be used as is since the list isn't strictly
	 * consumed as it is traversed.
	 */

	/* 
	 * first round, mark namenodes that needs to be unmounted 
	 */
	pl = LOCK(&namelist_mutex, PLNM);
	nodep = namealloc;
	while (nodep) {
		nextp = nodep->nm_nextp;
		if (nodep->nm_filevp == vp && 
		  (vfsp = NMTOV(nodep)->v_vfsp) != NULL && vfsp != namevfsp) {
			nodep->nm_flag |= NMUNMOUNT;
		}
		nodep = nextp;
	}
	if (vp->v_stream) {
		(void)STREAM_LOCK(vp->v_stream);
		vp->v_stream->sd_flag &= ~STRMOUNT;
		STREAM_UNLOCK(vp->v_stream, PLNM);
	}
	/* 
	 * second round, remove nodes with NMUNMOUNT flag set.
	 */
	done = 0;
	while (!done) {
		done = 1;
		nodep = namealloc;
		while (nodep) {
			nextp = nodep->nm_nextp;
			if ((nodep->nm_filevp == vp) && 
			    (nodep->nm_flag & NMUNMOUNT) &&
			    (vfsp = NMTOV(nodep)->v_vfsp) != NULL && 
				vfsp != namevfsp) {
				VN_HOLD(NMTOV(nodep));
				UNLOCK(&namelist_mutex, pl);
				if ((error = dounmount(vfsp, crp)) != 0) {
					VN_RELE(NMTOV(nodep));
					realerr = error;
					if (realerr == EIO) {
			    			NMTOV(nodep)->v_vfsp = namevfsp;
						cmn_err(CE_WARN,"Umount failed due to hardware errors\n");
					}
				}
				(void)LOCK(&namelist_mutex, PLNM);
				done = 0;
				break;
			}
			nodep = nextp;
		}
	}
	UNLOCK(&namelist_mutex, pl);
	return (realerr);
}
