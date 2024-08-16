/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/memfs/memfs_dir.c	1.7"

#include <acc/priv/privilege.h>
#include <fs/dnlc.h>
#include <fs/memfs/memfs.h>
#include <fs/memfs/memfs_hier.h>
#include <fs/memfs/memfs_mnode.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

extern int memfs_taccess();

/*
 * int
 * memfs_dirlook(mnode_t *dp, char *name, mnode_t **mpp, struct cred *cred)
 *
 * Calling/Exit State:
 *	<dp> is locked on entry; remains locked on return.
 *
 * Description:
 * 	Search directory 'dp' for entry 'name'.
 *
 * 	0 is returned on success and *mpp points
 * 	to the found mnode with its vnode held.
 */
int
memfs_dirlook(mnode_t *dp, char *name, mnode_t **mpp, struct cred *cred)
{
	int error;
	struct memfs_dirent *tdp;
	vnode_t *vp;
	void *cookie;
	boolean_t softhold;
	int namelen;

	*mpp = NULL;

	if ((dp->mno_mode & S_IFMT) != S_IFDIR)
		return (ENOTDIR);

	if (error = memfs_taccess(dp, VEXEC, cred))
		return (error);

	if (*name == '\0') {
		VN_HOLD(MNODE_TO_VP(dp));
		*mpp = dp;
		return (0);
	}

	/*
	 * Lookup in dnlc
	 */
	vp = dnlc_lookup(MNODE_TO_VP(dp), name, &cookie, &softhold, NOCRED);
	if (vp) {
		*mpp = VP_TO_MNODE(vp);
		ASSERT(!softhold);
		return (0);
	}

	/*
	 * Search the directory for the matching name
	 */
	namelen = strlen(name);
	for (tdp = dp->mno_dir; tdp; tdp = tdp->td_next) {
		if (tdp->td_namelen == namelen &&
		    *name == *tdp->td_name && /* fast chk 1st chr */
		    bcmp(tdp->td_name, name, namelen) == 0) {
			ASSERT(tdp->td_mnode);
			*mpp = tdp->td_mnode;

			/*
			 * The memfs_list_lck is required so the memfs_unmount
			 * code can reliably detect when files are open.
			 */
			vp = MNODE_TO_VP(*mpp);
			(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
			VN_HOLD(vp);
			UNLOCK(&memfs_list_lck, PLBASE);

			/*
			 * Make entry in dnlc
			 */
			dnlc_enter(MNODE_TO_VP(dp), name, vp,
				   (void *)0, NOCRED);

			return (0);
		}
	}
	return (ENOENT);
}

/*
 * This is the fake size of a memfs_dirent structure that is used in
 * assigning offsets to directory entries
 */
#define	MEM_DIRSIZ 1

/*
 * int
 * memfs_direnter(struct mem_vfs *mem_vfsp, mnode_t *dir, char *name,
 *	mnode_t *mp, struct cred *cred)
 *
 * Calling/Exit State:
 *	<dir> is locked in 'excl' mode on entry and remains locked
 *	at exit.
 *
 * Description:
 * 	Wite a new directory entry for 'name' and 'mp' into directory 'dir'
 *
 * 	Returns 0 on success.
 */
int
memfs_direnter(struct mem_vfs *mem_vfsp, mnode_t *dir, char *name, mnode_t *mp,
	struct cred *cred)
{
	struct memfs_dirent *tdp, *tpdp;
	int error, namelen;
	pl_t pl;

	/*
	 * mno_rwlock is held to serialize direnter and dirdeletes
	 */
	ASSERT(!RWSLEEP_RDAVAIL(&dir->mno_rwlock));
	ASSERT((dir->mno_mode & S_IFMT) == S_IFDIR);

	if (error = memfs_taccess(dir, VWRITE|VEXEC, cred))
		return (error);

	/*
	 * We don't need to grab the memfs_list_lck because we know that
	 * all those who modify mno_rwlock also hold the RW lock (in
	 * addition to the memfs_list_lck).
	 */
	if (dir->mno_nlink == 0)
		return (ENOENT);

	/*
	 * allocate and initialize directory entry
	 */
	namelen = strlen(name);
	tdp = memfs_kmemalloc(mem_vfsp, MEMFS_DIRENT_SIZE(namelen), KM_SLEEP);
	if (tdp == NULL)
		return (ENOSPC);

	dir->mno_size += MEMFS_DIRENT_SIZE(namelen);

	tdp->td_mnode = mp;
	tdp->td_namelen = namelen;
	bcopy(name, tdp->td_name, namelen);

	tpdp = dir->mno_dir;
	/*
	 * Install at first empty "slot" in directory list.
	 */
	while (tpdp->td_next != NULL && (tpdp->td_next->td_offset -
	    tpdp->td_offset) <= MEM_DIRSIZ) {
		ASSERT(tpdp->td_next->td_offset > tpdp->td_offset);
		tpdp = tpdp->td_next;
	}
	tdp->td_offset = tpdp->td_offset + MEM_DIRSIZ;
	tdp->td_next = tpdp->td_next;
	tpdp->td_next = tdp;

	/*
	 * Update child's link count.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	++mp->mno_nlink;
	UNLOCK(&memfs_list_lck, PLBASE);

	pl = LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
	mem_vfsp->mem_direntries++;
	UNLOCK(&mem_vfsp->mem_contents, pl);

	/*
	 * Make entry in dnlc
	 */
	dnlc_enter(MNODE_TO_VP(dir), name, MNODE_TO_VP(mp), (void *)0, NOCRED);

	return (0);
}

/*
 * int
 * memfs_dirremove(mem_vfs_t *mem_vfsp, mnode_t *dir, mnode_t *mp, char *nm,
 *	 cred_t *cred)
 *
 * Calling/Exit State:
 *	<dir> is locked in 'excl' mode on entry and remains locked
 *	at exit.
 *
 * Description:
 * 	Delete entry mp of name "nm" from dir.
 *
 * 	Return 0 on success.
 */
/*ARGSUSED*/
int
memfs_dirremove(mem_vfs_t *mem_vfsp, mnode_t *dir, mnode_t *mp, char *nm, cred_t *cred)
{
	struct memfs_dirent *tpdp, **tpdpp;
	int error, namelen;
	pl_t pl;

	ASSERT(!RWSLEEP_RDAVAIL(&dir->mno_rwlock));
	ASSERT((dir->mno_mode & S_IFMT) == S_IFDIR);
	ASSERT(strcmp(nm, ".") != 0);

	namelen = strlen(nm);
	if (namelen == 0) {
		/*
                 *+ A zero-length name was specified to be removed from
                 *+ a directory. Such condition indicates possible
                 *+ file system corruption and/or a kernel programming
                 *+ error. Corrective action: reboot.
                 */
		cmn_err(CE_PANIC, "memfs_dirremove: NULL strlen for 0x%x", mp);
	}

	if (error = memfs_taccess(dir, VWRITE|VEXEC, cred))
		return (error);

	/*
	 * Can't delete from directory with sticky-bit set unless
	 * you own the directory or the file.
	 */
	if ((dir->mno_mode & S_ISVTX) && pm_denied(cred, P_OWNER) &&
	    cred->cr_uid != dir->mno_uid && mp->mno_uid != cred->cr_uid &&
	    (memfs_taccess(mp, VWRITE, cred) != 0))
		return (EPERM);

	tpdpp = &dir->mno_dir;
	for (;;) {
		tpdp = *tpdpp;
		if (mp == tpdp->td_mnode && tpdp->td_namelen == namelen &&
		    *nm == *tpdp->td_name && /* fast chk 1st chr */
		    bcmp(tpdp->td_name, nm, namelen) == 0)
			break;
		tpdpp = &tpdp->td_next;
	}

	/*
	 * tpdp points to the correct directory entry
	 */
	ASSERT(tpdp->td_next != tpdp);
	*tpdpp = tpdp->td_next;

	memfs_kmemfree(mem_vfsp, tpdp, MEMFS_DIRENT_SIZE(namelen));
	dir->mno_size -= MEMFS_DIRENT_SIZE(namelen);

        /*
         * Remove the cached entry, if any.
         */
        dnlc_remove(MNODE_TO_VP(dir), nm);

	/*
	 * Update child link count.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	ASSERT(mp->mno_nlink != 0);
	--mp->mno_nlink;
	UNLOCK(&memfs_list_lck, PLBASE);

	pl = LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
	mem_vfsp->mem_direntries--;
	UNLOCK(&mem_vfsp->mem_contents, pl);

	return (0);
}

/*
 * int
 * memfs_dirinit(mem_vfs_t *mem_vfsp, mnode_t *dp, mnode_t *dir,cred_t *cred)
 *
 * Calling/Exit State:
 *	The RW locks for both dp and dir are held in exclusive mode.
 *
 * Description:
 * 	Initialize a directory (dir) with '.' and '..' entries.
 *
 * 	Return 0 on success.
 */
/*ARGSUSED*/
int
memfs_dirinit(mem_vfs_t *mem_vfsp, mnode_t *dp, mnode_t *dir,cred_t *cred)
{
	struct memfs_dirent *dot, *dotdot;
	static char onedot[] = ".";
	static char twodots[] = "..";

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * allocate "." entry
	 */
	dot = memfs_kmemalloc(mem_vfsp, MEMFS_DIRENT_SIZE(sizeof(onedot) - 1),
			      KM_SLEEP);
	if (dot == NULL)
		return (ENOSPC);
	/*
	 * allocate ".." entry
	 */
	dotdot = memfs_kmemalloc(mem_vfsp,
				 MEMFS_DIRENT_SIZE(sizeof(twodots) - 1),
				 KM_SLEEP);
	if (dotdot == NULL) {
		memfs_kmemfree(mem_vfsp, dot,
			       MEMFS_DIRENT_SIZE(sizeof(onedot) - 1));
		return (ENOSPC);
	}

	/*
	 * initialize the entries
	 */
	dot->td_mnode = dir;
	dot->td_offset = 0;
	dot->td_namelen = sizeof(onedot) - 1;
	dot->td_name[0] = onedot[0];

	dotdot->td_mnode = dp;
	dotdot->td_offset = 1;
	dotdot->td_namelen = sizeof(twodots) - 1;
	dotdot->td_name[0] = twodots[0];
	dotdot->td_name[1] = twodots[1];

	/*
	 * Enter "." in dnlc
	 */
	dnlc_enter(MNODE_TO_VP(dir), onedot, MNODE_TO_VP(dir), (void *)0,
		   NOCRED);

	/*
	 * and ".." in dp.
	 */
	dnlc_enter(MNODE_TO_VP(dir), twodots, MNODE_TO_VP(dp), (void *)0,
		   NOCRED);

	/*
	 * Update link counts. Note that the VN_HOLDs must come before the
	 * link count increments for the unmount code to work properly.
	 */
	(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
	++dp->mno_nlink;
	++dir->mno_nlink;
	UNLOCK(&memfs_list_lck, PLBASE);

	/*
	 * Initialize directory entry list.
	 */
	dir->mno_dir = dot;
	dot->td_next = dotdot;
	dotdot->td_next = NULL;

	(void) LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
	mem_vfsp->mem_direntries += 2;
	UNLOCK(&mem_vfsp->mem_contents, PLBASE);

	dir->mno_size = MEMFS_DIRENT_SIZE(sizeof(onedot) - 1) +
			MEMFS_DIRENT_SIZE(sizeof(twodots) - 1);

	return (0);
}

/*
 * void
 * memfs_dirtrunc(mem_vfs_t *mem_vfsp, mnode_t *dir, struct cred *cred)
 *
 * Calling/Exit State:
 *	<dir> is locked in 'excl' mode on entry. This routine releases
 *	the lock.
 *
 *	The caller owns a VN_HOLD on <dir>. This routine drops the hold.
 *
 * Description:
 * 	memfs_dirtrunc is called to remove all directory entries under
 *	this directory.
 */
void
memfs_dirtrunc(mem_vfs_t *mem_vfsp, mnode_t *dir, boolean_t is_unmount)
{
	struct memfs_dirent *tdp;
	mnode_t *mp;
	vnode_t *vp;
	int namelen;

	ASSERT(MNODE_TO_VP(dir)->v_count != 0);
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	for (tdp = dir->mno_dir; tdp; tdp = dir->mno_dir) {
		ASSERT(tdp->td_next != tdp);
		dir->mno_dir = tdp->td_next;
		namelen = tdp->td_namelen;
		ASSERT(tdp->td_mnode);

		/*
		 * Count down link count.
		 */
		mp = tdp->td_mnode;
		(void) LOCK(&memfs_list_lck, FS_MEMFS_PL);
		ASSERT(mp->mno_nlink != 0);
		--mp->mno_nlink;
		vp = MNODE_TO_VP(mp);
		if (is_unmount)
			VN_HOLD(vp);
		UNLOCK(&memfs_list_lck, PLBASE);

		/*
		 * Free the entry
		 */
		memfs_kmemfree(mem_vfsp, tdp, MEMFS_DIRENT_SIZE(namelen));
		dir->mno_size -= MEMFS_DIRENT_SIZE(namelen);

		/*
		 * Purge "." and ".." from dnlc. When called from
		 * memfs_unmount, dnlc has already been purged.
		 * However, in the unmount case we might actually be
		 * deleting "..".
		 */
		if (is_unmount) {
			VN_RELE(vp);
		} else if (tdp->td_namelen == 1) {
			dnlc_remove(MNODE_TO_VP(dir), ".");
		} else {
			dnlc_remove(MNODE_TO_VP(dir), "..");
		}

		(void) LOCK(&mem_vfsp->mem_contents, FS_MEMFS_PL);
		mem_vfsp->mem_direntries--;
		UNLOCK(&mem_vfsp->mem_contents, PLBASE);
	}
	ASSERT(dir->mno_dir == NULL);
	ASSERT(dir->mno_size == 0);
	RWSLEEP_UNLOCK(&dir->mno_rwlock);
	VN_RELE(MNODE_TO_VP(dir));
}
