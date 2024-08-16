/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/s5fs/s5dir.c	1.29"
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
 * Directory manipulation routines. From outside this file, only
 * s5_dirlook(), s5_direnter(), and s5_dirremove() should be called.
 */

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/dnlc.h>
#include <fs/fbuf.h>
#include <fs/file.h>
#include <fs/mode.h>
#include <fs/pathname.h>
#include <fs/s5fs/s5dir.h>
#include <fs/s5fs/s5filsys.h>
#include <fs/s5fs/s5hier.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5macros.h>
#include <fs/s5fs/s5param.h>
#include <fs/stat.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/seg.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/sysmacros.h>
#include <util/types.h>

#define	DOT	0x01
#define	DOTDOT	0x02

extern	void	s5_iput(inode_t *);
extern	void	s5_iupdat(inode_t *);
extern	int	s5_iaccess(inode_t *, mode_t, cred_t *);
extern	int	s5_itrunc(inode_t *, uint_t);
extern	int	s5_rdwri(enum uio_rw, inode_t *, caddr_t, int, off_t,
			 enum uio_seg, int, int *);
extern	int	s5_ialloc(vfs_t *, o_mode_t, int, dev_t, int, int,
			  inode_t **);
extern short	maxlink;

/*
 * int
 * s5_dirsearch(inode_t *dip, char *comp, inode_t **ipp, off_t offp, int excl)
 *	Search for an entry.
 *
 * Calling/Exit State:
 *	dip is locked on entry and remain locked on exit.
 *	if found ipp is locked on exit.
 */
int
s5_dirsearch(inode_t *dip, char *comp, inode_t **ipp, off_t *offp, int excl)
	/* dip  - Directory to search. */
	/* comp - Component to search for. */
	/* ipp  - Ptr-to-ptr to result inode, if found. */
	/* offp - Offset of entry or empty entry. */
	/* excl - exclusive or shared mode. */
{
	char	*cp;
	int	off;
	vnode_t	*dvp;
	direct_t dir, *dp;
	int	error, count, n, len;
	off_t	offset;
	off_t	eo;
	int	bsize;
	int	bmask;
	fbuf_t	*fbp;
	struct inode	*ip;

	dvp = ITOV(dip);
	bsize = VBSIZE(dvp);
	*ipp = NULL;
	dir.d_ino = 0;
	len = strlen(comp);
	if (len < DIRSIZ)
		++len;
	else
		len = DIRSIZ;
	bcopy(comp, dir.d_name, len);
	bmask = ((s5_fs_t *)S5FS(dvp->v_vfsp))->fs_bmask;
	fbp = NULL;

	offset = 0;
	eo = -1;
	count = dip->i_size;

	while (count) {
		/*
		 * Read the next directory block.
		 */
		MET_DIRBLK(MET_S5);
		if (error = fbread(dvp, offset & bmask, bsize, S_OTHER, &fbp))
			goto out;
		/*
		 * Search directory block for entry with the specified name
		 */
		n = MIN(bsize, count);
		ASSERT((n >= 0) && ((n % SDSIZ) == 0));
		dp = (direct_t *)fbp->fb_addr;
		for (off = 0 ; off < n ; off += SDSIZ) {
			if (dp->d_ino != 0) {
				if ((dp->d_name[0] == dir.d_name[0]) &&
						(bcmp(dp->d_name, dir.d_name,
						len) == 0)) {
					offset += off;
					dir.d_ino = dp->d_ino;
					fbrelse(fbp, 0);
					break;
				}
			} else if (eo < 0)
				eo = offset + off;
			++dp;
		}
		if (dir.d_ino != 0)
			break;
		offset += n;
		count -= n;
		fbrelse(fbp, 0);
	}
	if (dir.d_ino != 0) {
		if (strcmp(dir.d_name, "..") == 0) {
			error = s5_iget(dvp->v_vfsp, getfs(dvp->v_vfsp),
				 dir.d_ino, IG_NONE, ipp);
			if (error) {
				goto out;
			}
                        ip = *ipp;
                        if (excl) {
				S5_IRWLOCK_WRLOCK(ip);
                        } else {
				S5_IRWLOCK_RDLOCK(ip);
                        }
		} else if (dir.d_ino == dip->i_number) {
			/* want outself, i.e., "." */
			VN_HOLD(dvp);
			*ipp = dip;
                        if (excl) {
                                S5_IRWLOCK_UNLOCK(dip);
                                S5_IRWLOCK_WRLOCK(dip);
                        }
		} else {
			/* Not looking for ".." - Just get inode. */ 
			error = s5_iget(dvp->v_vfsp, getfs(dvp->v_vfsp),
				 dir.d_ino, excl ? IG_EXCL : IG_SHARE, ipp);
	 		if (error) {
				goto out;
			}
		}
	} else {
		/*
		 * If an empty slot was found, return it.  Otherwise leave the
		 * offset unchanged (pointing at the end of directory).
		 */
		if (eo != -1)
			offset = eo;
		error = ENOENT;
	}
out:
	if (offp)
		*offp = offset;
	return (error);
}

/*
 * int
 * s5_dirlook(inode_t *dp, char *namep, inode_t **ipp, int excl, cred_t *cr)
 *	Look for a given name in a directory.
 *
 * Calling/Exit State:
 *	<dp> is unlocked on entry; remains unlocked on return.
 *
 *	On successful returns, *ipp's rwlock is held *exclusive* if
 *	<excl> is non-zero, otherwise, *shared*.
 *
 *	A return value of 0 indicated success; otherwise, a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		ENOTDIR		<dp> is not a directory
 */
int
s5_dirlook(inode_t *dp, char *namep, inode_t **ipp, int excl, cred_t *cr)
{
	int	error;
	int	namelen;
	vnode_t	*vp;
	inode_t	*ip;
	void	*cookie;
	boolean_t softhold;

	*ipp = NULL;
	/*
	 * Check accessibility of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR) {
		return (ENOTDIR);
	}
	/*
	 * Prevent concurrent directory modifications by
	 * grabbing the directory's rwlock. *Shared*
	 * is sufficient to prevent modifications; the
	 * caller, may, however, specify *exclusive*
	 * via <excl>.
	 */
	namelen = strlen(namep);
	if (excl && (*namep == '\0' || (namelen == 1 && *namep == '.'))) {
		S5_IRWLOCK_WRLOCK(dp);
	} else {
		S5_IRWLOCK_RDLOCK(dp);
	}
	if (error = s5_iaccess(dp, IEXEC, cr)) {
		S5_IRWLOCK_UNLOCK(dp);
		return (error);
	}
	/*
	 * Null component name is synonym for directory being searched.
	 */
	if (*namep == '\0' || (namelen == 1 && *namep == '.')) {
		VN_HOLD(ITOV(dp));
		*ipp = dp;
		return (0);
	}

	/*
	 * Check the directory name lookup cache.
	 */
	vp = dnlc_lookup(ITOV(dp), namep, &cookie, &softhold, NOCRED);
	if (vp) {
		ASSERT(softhold == B_FALSE);
		ip = VTOI(vp);
		/*
		 * It's ok to release the dir lock at this
		 * point since the calling LWP already has
		 * a reference to the inode.
		 */
		S5_IRWLOCK_UNLOCK(dp);
		if (excl) {
			S5_IRWLOCK_WRLOCK(ip);
		} else {
			S5_IRWLOCK_RDLOCK(ip);
		}
		*ipp = ip;
		return (0);
	}

	error = s5_dirsearch(dp, namep, ipp, (off_t *) 0, excl);
	if (dp != *ipp || error) {
		S5_IRWLOCK_UNLOCK(dp);
	}
	if (error == 0) {
		vp = ITOV(*ipp);
		dnlc_enter(ITOV(dp), namep, vp, NULL, NOCRED);
	}
	return (error);
}

/*
 * int
 * s5_dircheckpath(inode_t *source, inode_t *target)
 *	 Check if source directory is in the path of the target directory.
 *
 * Calling/Exit State:
 *	The caller holds target's rwlock *exclusive* on entry; the
 *	lock is dropped and re-acquired before returning.
 *
 *	<source> is unlocked on entry; remains unlocked on exit.
 *
 * Description:
 */
int
s5_dircheckpath(inode_t *source, inode_t *target)
{
	inode_t	 *ip;
	int	 error;
	direct_t dir;

	error = 0;
	/*
	 * If two renames of directories were in progress at once, the partially
	 * completed work of one dircheckpath could be invalidated by the other
	 * rename. To avoid this, all directory renames in the system are
	 * serialized in s5_direnter.
	 */
	ip = target;
	if (ip->i_number == source->i_number) {
		error = EINVAL;
		return error;
	}
	if (ip->i_number == S5ROOTINO)
		return error;
	/*
	 * Search back through the directory tree, using the ".." entries.
	 * Fail any attempt to move a directory into an ancestor directory.
	 */
	for (;;) {

		if (((ip->i_mode & IFMT) != IFDIR) || ip->i_nlink == 0
		  || ip->i_size < 2*SDSIZ) {
			error = ENOTDIR;
			break;
		}
		error = s5_rdwri(UIO_READ, ip, (caddr_t) &dir, SDSIZ,
				(off_t) SDSIZ, UIO_SYSSPACE, 0, (int *) 0);
		if (error) {
			break;
		}
		if (strcmp(dir.d_name, "..") != 0) {
			error = ENOTDIR;	/* Sanity check */
			break;
		}
		if (dir.d_ino == source->i_number) {
			error = EINVAL;
			break;
		}
		if (dir.d_ino == S5ROOTINO)
			break;
		if (ip != target) {
			s5_iput(ip);
		} else {
			S5_IRWLOCK_UNLOCK(ip);
		}
		error = s5_iget(ITOV(ip)->v_vfsp, getfs(ITOV(ip)->v_vfsp),
					 dir.d_ino, IG_EXCL, &ip);
		if (error)
			break;
	}
	if (ip) {
		if (ip != target) {
			s5_iput(ip);
			/*
			 * Relock target and make sure it has not gone away
			 * while it was unlocked.
			 */
			S5_IRWLOCK_WRLOCK(target);
			if (error == 0 && target->i_nlink == 0)
				error = ENOENT;
		}
	}
	return (error);
}

/*
 * int s5_dircheckforname(inode_t *, char *, off_t, inode_t **)
 * 	Check for the existence of a name in a directory, or else of an empty
 * 	slot in which an entry may be made.
 *
 * Calling/Exit State:
 *	<tdp> is locked exclusive on entry; remains so locked on exit.
 *
 *	On successful returns, <(*ipp)->i_rwlock> is held exclusive.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. There are no errnos returned directly
 *	by this routine.
 *
 * Description:
 *	If the requested name is found,
 * 	then on return *tipp points at the (locked) inode and *offp contains
 * 	its offset in the directory.  If the name is not found, then *tipp
 * 	will be NULL and *offp will contain the offset of a directory slot in
 * 	which an entry may be made (either an empty slot, or the first offset
 * 	past the end of the directory).
 * 
 * 	This may not be used on "." or "..", but aliases of "." are okay.
 */
int
s5_dircheckforname(inode_t *tdp, char *namep, off_t *offp, inode_t **tipp)
	/* tdp   - inode of directory being checked */
	/* namep - name we're checking for */
	/* offp  - return offset of old or new entry */
	/* tipp  - return inode if we find one */
{
	int	error;

	/*
	 * Search for entry.  The caller doesn't require that it exist, so
	 * don't return ENOENT.  The non-existence of the entry will be
	 * indicated by *tipp == NULL.
	 */
	if ((error = s5_dirsearch(tdp, namep, tipp, offp, 1)) == ENOENT)
		error = 0;
	return (error);
}

/*
 * int
 * s5_dirempty(inode_t *ip, int *dotflagp)
 * 	Check whether a directory is empty (i.e. whether it contains
 * 	any entries apart from "." and "..").
 *
 * Calling/Exit State:
 *	Caller holds ip's rwlock exclusive on entry and exit.
 *
 * Description:
 *	Using a struct dirtemplate here is not precisely
 *	what we want, but better than using a struct direct.
 * 	The value returned in *dotflagp encodes whether
 * 	"." and ".." are actually present.
 *
 *	N.B.: does not handle corrupted directories.
 */
int
s5_dirempty(inode_t *ip, int *dotflagp)
{
	off_t	 off;
	direct_t dir;
	direct_t *dp = &dir;

	*dotflagp = 0;
	for (off = 0; off < ip->i_size; off += SDSIZ) {
		if (s5_rdwri(UIO_READ, ip, (caddr_t) dp, SDSIZ, off,
			     UIO_SYSSPACE, 0, (int *) 0))
			break;
		if (dp->d_ino != 0) {
			if (strcmp(dp->d_name, ".") == 0)
				*dotflagp |= DOT;
			else if (strcmp(dp->d_name, "..") == 0)
				*dotflagp |= DOTDOT;
			else
				return (0);
		}
	}
	return (1);
}

/*
 * int
 * s5_dirfixdotdot(inode_t *dp, inode_t *opdp, inode_t *npdp)
 * 	Fix the ".." entry of the child directory so that it points
 *	to the new parent directory instead of the old one.  Routine
 *	assumes that dp is a directory and that all the inodes are on
 *	the same file system.
 *
 * Calling/Exit State:
 *	<dp> is assumed to be a directory and all the inodes are
 *	assumed to be on the same file system.
 *
 *	<dp> and <opdp> are unlocked on entry; remain unlocked on exit.
 *	<npdp> is locked exclusive on entry; remains so locked on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENOTDIR <dp> contains a mangled ".." entry
 *		EMLINK  <npdp> already has a link count of maxlink.
 *
 * Description:
 *	Obtain <dp->i_rwlock> in exclusive mode. If <dp> doesn't
 *	exist any more (nlink == 0) or <dp> is smaller than the
 *	smallest allowable directory, than return 0.
 *
 */
int
s5_dirfixdotdot(inode_t *dp, inode_t *opdp, inode_t *npdp)
	/* dp   - child directory */
	/* opdp - old parent directory */
	/* npdp - new parent directory */
{
	int	 error;
	direct_t dir;
	pl_t s;

	S5_IRWLOCK_WRLOCK(dp);
	/*
	 * Check whether this is an ex-directory.
	 */
	if (dp->i_nlink == 0 || (dp->i_size < 2*SDSIZ)) {
		goto bad;
	}
	error = s5_rdwri(UIO_READ, dp, (caddr_t) &dir, SDSIZ,
			(off_t) SDSIZ, UIO_SYSSPACE, 0, (int *) 0);
	if (error) {
		goto bad;
	}
	if (dir.d_ino == npdp->i_number) {	/* Just a no-op. */
		goto bad;
	}
	if (strcmp(dir.d_name, "..") != 0) {	/* Sanity check. */
		error = ENOTDIR;
		goto bad;
	}
	/*
	 * Increment the link count in the new parent inode and force it out.
	 */
	if (npdp->i_nlink >= maxlink) {
                error = EMLINK;
                goto bad;
        }

	npdp->i_nlink++;
	s = S5_ILOCK(npdp);
	IMARK(npdp, ICHG|ISYN);
	S5_IUNLOCK(npdp, s);
	s5_iupdat(npdp);

	/*
	 * Rewrite the child ".." entry and force it out.
	 */
	dir.d_ino = npdp->i_number;
	error = s5_rdwri(UIO_WRITE, dp, (caddr_t) &dir, SDSIZ, (off_t) SDSIZ,
			 UIO_SYSSPACE, IO_SYNC, (int *) 0);
	if (error) {
		goto bad;
	}

	dnlc_remove(ITOV(dp), "..");
	dnlc_enter(ITOV(dp), "..", ITOV(npdp), NULL, NOCRED);
	S5_IRWLOCK_UNLOCK(dp);

	/*
	 * Decrement the link count of the old parent inode and force
	 * it out.  If opdp is NULL, then this is a new directory link;
	 * it has no parent, so we need not do anything.
	 */
	if (opdp != NULL) {
		S5_IRWLOCK_WRLOCK(opdp);
		if (opdp->i_nlink != 0) {
			opdp->i_nlink--;
			s = S5_ILOCK(opdp);
			IMARK(opdp, ICHG|ISYN);
			S5_IUNLOCK(opdp, s);
			s5_iupdat(opdp);
		}
		S5_IRWLOCK_UNLOCK(opdp);
	}
	return (0);
bad:
	S5_IRWLOCK_UNLOCK(dp);
	return (error);
}

/*
 * int
 * s5_dirrename(inode_t *sdp, inode_t *sip, inode_t *tdp, char *namep,
 * 	Rename the entry in the directory tdp so that it points to
 * 	sip instead of tip.
 *
 * Calling/Exit State:
 *	The calling LWP holds tdp's rwlock *exclusive* on entry
 *	and exit; tip's rwlock *exclusive* on entry and exit.
 *
 *	<sdp> is unlocked on entry; remains unlocked on exit.
 *	<sip> is unlocked on entry; remains unlocked on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EXDEV   Either <tip> and <tdp> or <tip> and <sip> are
 *			on different file systems
 *		EISDIR  <tip> refers to a directory, but <sip> is not
 *			a directory.
 *		EBUSY   <tip> is a mounted on directory.
 *		EEXIST  <tip> is a non-empty directory.
 *		ENOTDIR <sip> is a directory but <tip> is not.
 *		EIO     <tip> is a directory that contained more than 2 links.
 *
 * Description:
 *	Insure that the calling LWP has IWRITE access to <tdp> and
 *	that <tip>, <tdp>, and <sip> are all on the same file system.
 *
 *	If the parent directory is sticky, than insure that the
 *	calling LWP has appropriate permission to perform the
 *	rename.
 *
 *	Only allow renames if the file are compatible; do not allow
 *	renames of mounted file systems or to non-empty directories.
 */
/* ARGSUSED */
int
s5_dirrename(inode_t *sdp, inode_t *sip, inode_t *tdp, char *namep,
	     inode_t *tip, off_t offset, cred_t *cr)
	/* sdp    - parent directory of source */
	/* sip    - source inode */
	/* tdp    - parent directory of target */
	/* namep  - entry we are trying to change */
	/* tip    - locked target inode */
	/* offset - offset of new entry */
	/* cr     - credentials */
{
	int	 error;
	int	 doingdirectory;
	int	 dotflag;
	int	 vplocked;
	direct_t dir;
	vnode_t	 *vp;
	pl_t s;

	/*
	 * Check that everything is on the same filesystem.
	 */
	if (tip->i_dev != tdp->i_dev || tip->i_dev != sip->i_dev)
		return (EXDEV);
	/*
	 * Short circuit rename of something to itself.
	 */
	if (sip->i_number == tip->i_number)
		return (ESAME);		/* special KLUDGE error code */
	/*
	 * Must have write permission to rewrite target entry.
	 */
	if (error = s5_iaccess(tdp, IWRITE, cr))
		return (error);
	/*
	 * If the parent directory is "sticky", then the user must own
	 * either the parent directory or the destination of the rename,
	 * or else must have permission to write the destination.
	 * Otherwise the destination may not be changed (except by the
	 * super-user).  This implements append-only directories.
	 */
	if ((tdp->i_mode & ISVTX) && pm_denied(cr, P_OWNER)
	  && cr->cr_uid != tdp->i_uid && cr->cr_uid != tip->i_uid
	  && (error = s5_iaccess(tip, IWRITE, cr)))
		return (error);
	vplocked = 0;
	/*
	 * Ensure source and target are compatible (both directories
	 * or both not directories).  If target is a directory it must
	 * be empty and have no links to it; in addition it must not
	 * be a mount point.
	 */
	doingdirectory = ((sip->i_mode & IFMT) == IFDIR);
	if ((tip->i_mode & IFMT) == IFDIR) {
		if (!doingdirectory)
			return (EISDIR);
		/*
		 * Racing with a concurrent mount?
		 * Drop tip's rwlock and get the vnode
		 * rwlock to synchronize with mount.
		 *
		 * It's okay to release the lock here
		 * because we already have our reference
		 * to the inode and we hold the containing
		 * directory locked.
		 */
		S5_IRWLOCK_UNLOCK(tip);

		vp = ITOV(tip);
		RWSLEEP_WRLOCK(&vp->v_lock, PRIVFS);

		vplocked++;
		VN_LOCK(vp);
		if (vp->v_flag & VMOUNTING) {
			VN_UNLOCK(vp);
			/*
			 * concurrent mount...let mount win
			 */
			RWSLEEP_UNLOCK(&vp->v_lock);
			S5_IRWLOCK_WRLOCK(tip);
			return (EBUSY);
		}
		/*
		 * Can't remove mount points
		 */
		if (vp->v_vfsmountedhere) {
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&vp->v_lock);
			S5_IRWLOCK_WRLOCK(tip);
			return (EBUSY);
		}

		vp->v_flag |= VGONE;
		VN_UNLOCK(vp);
		S5_IRWLOCK_WRLOCK(tip);

		if (!s5_dirempty(tip, &dotflag) || (tip->i_nlink > 2)) {
			VN_LOCK(vp);
			vp->v_flag &= ~VGONE;
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&vp->v_lock);
			return (EEXIST);
		}
	} else if (doingdirectory) {
		return (ENOTDIR);
	}
	/*
	 * Rewrite the inode number for the target name entry
	 * from the target inode (ip) to the source inode (sip).
	 * This prevents the target entry from disappearing
	 * during a crash.
	 */
	dir.d_ino = sip->i_number;
	(void) strncpy(dir.d_name, namep, DIRSIZ);
	error = s5_rdwri(UIO_WRITE, tdp, (caddr_t) &dir, SDSIZ, offset,
			 UIO_SYSSPACE, IO_SYNC, (int *) 0);
	if (error) {
		goto out;
	}
	dnlc_remove(ITOV(tdp), namep);
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), NULL, NOCRED);

	s = S5_ILOCK(tdp);
	IMARK(tdp, IUPD|ICHG);
	S5_IUNLOCK(tdp, s);

	/*
	 * Decrement the link count of the target inode.
	 * Fix the ".." entry in sip to point to dp.
	 * This is done after the new entry is on the disk.
	 */
	tip->i_nlink--;
	if (doingdirectory) {
		if (dotflag & DOT) {
			tip->i_nlink--;
			dnlc_remove(ITOV(tip), ".");
		}
		if (tip->i_nlink != 0) {
			/*
			 *+ After the kernel ensured that the directory was
			 *+ empty (via diremtpy()), and decremented the link
			 *+ count for the entry in the parent dir and the "."
			 *+ entry, it found that the link count was not zero.
			 *+ This indicates a possible filesystem corruption
			 *+ problem. Corrective Action: run fsck on the
			 *+ filesystem.
			 */
			cmn_err(CE_WARN,
				"s5_dirrename: target directory link count");
			error = EIO;
		}
		error = s5_itrunc(tip, (u_long)0);
		if (error) {
			goto out;
		}
		/*
		 * Renaming a directory with the parent different
		 * requires that ".." be rewritten.  The window is
		 * still there for ".." to be inconsistent, but this
		 * is unavoidable, and a lot shorter than when it was
		 * done in a user process.  We decrement the link
		 * count in the new parent as appropriate to reflect
		 * the just-removed target.  If the parent is the
		 * same, this is appropriate since the original
		 * directory is going away.  If the new parent is
		 * different, s5_dirfixdotdot() will bump the link count
		 * back.
		 */
		if (dotflag & DOTDOT) {
			tdp->i_nlink--;
			dnlc_remove(ITOV(tip), "..");
		}
		if (sdp != tdp) {
			error = s5_dirfixdotdot(sip, sdp, tdp);
			if (error)
				goto out;
		}
	}
	s = S5_ILOCK(tip);
	IMARK(tip, ICHG);
	S5_IUNLOCK(tip, s);
out:
	if (vplocked) {
		if (error) {
			VN_LOCK(vp);
			vp->v_flag &= ~VGONE;
			VN_UNLOCK(vp);
		}
		RWSLEEP_UNLOCK(&vp->v_lock);
	}
	return (error);
}

/*
 * int
 * s5_dirmakedirect(inode_t *ip, inode_t *dp)
 * 	Write a prototype directory into the empty inode ip, whose parent is dp.
 *
 * Calling/Exit State:
 *	The caller holds both ip's and dp's rwlock *exclusive* on
 *	entry and exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EMLINK  <tdp> already has a link count of maxlink.
 */
int
s5_dirmakedirect(inode_t *ip, inode_t *dp)
	/* ip - new directory */
	/* dp - parent directory */
{
	int	 error;
	direct_t newdir[2];
	pl_t s;

	if (ip->i_nlink >= maxlink)
                return (EMLINK);

	(void) strncpy(newdir[0].d_name, ".", DIRSIZ);
	newdir[0].d_ino = ip->i_number;			/* dot */
	(void) strncpy(newdir[1].d_name, "..", DIRSIZ);	
	newdir[1].d_ino = dp->i_number;			/* dot-dot */

	error = s5_rdwri(UIO_WRITE, ip, (caddr_t) newdir, 2*SDSIZ, (off_t) 0,
			 UIO_SYSSPACE, IO_SYNC, (int *) 0);
	if (error == 0) {
		/*
		 * Synchronously update link count of parent.
		 */
		dp->i_nlink++;
		s = S5_ILOCK(dp);
		IMARK(dp, ICHG|ISYN);
		S5_IUNLOCK(dp, s);
		s5_iupdat(dp);
	}	
	s = S5_ILOCK(ip);
	IMARK(ip, IUPD|ICHG|ISYN);
	S5_IUNLOCK(ip, s);
	
	return (error);
}

/*
 * int
 * s5_dirmakeinode(inode_t *tdp, inode_t **ipp, vattr_t *vap,
 * 		enum de_op op, cred_t *cr)
 * 	Allocate and initialize a new inode that will go into directory tdp.
 *
 * Calling/Exit State:
 *	The  caller holds tdp's rwlock exclusive on entry; remains locked
 *	on exit.
 *	On success, <*ipp->i_rwlock> is returned referenced and unlocked.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. There are no errnos returned directly
 *	by this routine.
 */
int
s5_dirmakeinode(inode_t *tdp, inode_t **ipp, vattr_t *vap,
		enum de_op op, cred_t *cr)
{
	inode_t	*ip;
	int	imode;
	int	nlink;
	int	gid;
	int	error;
	vfs_t	*vfsp;
	pl_t s;

	ASSERT(vap != NULL);
	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));
	ASSERT(op == DE_CREATE || op == DE_MKDIR);
	/*
	 * Allocate a new inode.
	 */
	imode = MAKEIMODE(vap->va_type, vap->va_mode);
	nlink = (op == DE_MKDIR) ? 2 : 1;
	/*
	 * If ISGID is set on the containing directory, the new
	 * entry inherits the directory's gid; otherwise the gid
	 * is taken from the supplied credentials.
	 */
	if (tdp->i_mode & ISGID) {
		gid = tdp->i_gid;
		if ((imode & IFMT) == IFDIR)
			imode |= ISGID;
		else if ((imode & ISGID) && !groupmember(gid, cr)
		  && pm_denied(cr, P_OWNER))
			imode &= ~ISGID;
	} else
		gid = cr->cr_gid;

	vfsp = ITOV(tdp)->v_vfsp;
	error = s5_ialloc(vfsp, imode, nlink, vap->va_rdev,
			  cr->cr_uid, gid, &ip);
	if (error) {
		return (error);
	}
	if (op == DE_MKDIR)
		error = s5_dirmakedirect(ip, tdp);
	if (error) {
		/*
		 * Throw away the inode we just allocated.
		 */
		ip->i_nlink = 0;
		s = S5_ILOCK(ip);
		IMARK(ip, ICHG);
		S5_IUNLOCK(ip, s);
		s5_iput(ip);
	} else {
		S5_IRWLOCK_UNLOCK(ip);
		*ipp = ip;
	}
	return (error);
}

/*
 * int
 * s5_diraddentry(inode_t *tdp, char *namep, off_t offset,
 *		  inode_t *sip, inode_t *sdp, enum de_op op)
 * 	Enter the file sip in the directory tdp with name namep.
 *
 * Calling/Exit State:
 *	The caller holds tdp's rwlock *exclusive* on entry; remains locked
 *	on exit.
 *	<sip> is unlocked on entry; remains unlocked on exit.
 *	<sdp> is unlocked on entry; remains unlocked on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EXDEV   <tip> and <sip> are on different file systems
 *
 * Description:
 *	Fix the ".." of <sip> and write out the directory entry.
 *
 */
int
s5_diraddentry(inode_t *tdp, char *namep, off_t offset,
	       inode_t *sip, inode_t *sdp, enum de_op op)
{
	int	 error;
	direct_t dir;

	if ((sip->i_mode & IFMT) == IFDIR && op == DE_RENAME
	  && (error = s5_dirfixdotdot(sip, sdp, tdp)))
		return (error);
	/*
	 * Fill in entry data.
	 */
	dir.d_ino = sip->i_number;
	(void) strncpy(dir.d_name, namep, DIRSIZ);
	/*
	 * Write out the directory entry.
	 */
	error = s5_rdwri(UIO_WRITE, tdp, (caddr_t) &dir, SDSIZ, offset,
			 UIO_SYSSPACE, op == DE_MKDIR ? IO_SYNC : 0, (int *) 0);
	if (error == 0)
		dnlc_enter(ITOV(tdp), namep, ITOV(sip), NULL, NOCRED);
	return (error);
}

/*
 * int
 * s5_direnter(inode_t *tdp, char *namep, enum de_op op, inode_t *sdp,
 *	       inode_t *sip, vattr_t *vap, inode_t **ipp, cred_t *cr)
 *	Write a new directory entry.
 *
 * Calling/Exit State:
 *	<tdp>, <sdp>, and <sip> are unlocked on entry; remain unlocked on exit.
 *	If <ipp> is non-NULL and there's no error, <*ipp->i_rwlock> is held
 *	*exclusive* on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned.
 *
 * Description:
 *	The directory must not have been removed and must be writable.
 *	We distinguish four operations which build a new entry: creating
 *	a file (DE_CREATE), creating a directory (DE_MKDIR), renaming
 *	(DE_RENAME) or linking (DE_LINK).  There are five possible cases
 *	to consider:
 *
 *	Name
 *	found   op		      action
 *	-----   ---------------------   --------------------------------------
 *	no      DE_CREATE or DE_MKDIR   create file according to vap and enter
 *	no      DE_LINK or DE_RENAME    enter the file sip
 *	yes     DE_CREATE or DE_MKDIR   error EEXIST *ipp = found file
 *	yes     DE_LINK		 error EEXIST
 *	yes     DE_RENAME	       remove existing file, enter new file
 *
 *	Note that a directory can be created either by mknod(2) or by
 *	mkdir(2); the operation (DE_CREATE or DE_MKDIR) distinguishes
 *	the two cases, which differ because mkdir(2) creates the
 *	appropriate "." and ".." entries while mknod(2) doesn't.
 */
int
s5_direnter(inode_t *tdp, char *namep, enum de_op op, inode_t *sdp,
	    inode_t *sip, vattr_t *vap, inode_t **ipp, cred_t *cr)
	/* tdp   - target directory to make entry in */
	/* namep - name of entry */
	/* op    - entry operation */
	/* sdp   - source inode parent if rename */
	/* sip   - source inode if link/rename */
	/* vap   - attributes if new inode needed */
	/* ipp   - return entered inode (locked) here */
	/* cr    - user credentials */
{
	inode_t	*tip;		/* inode of (existing) target file */
	int	error;		/* error number */
	off_t	offset;		/* offset of old or new dir entry */
	short	namelen;	/* length of name */
	int 	have_write_error;
	char	*c;
	s5_fs_t	*s5fsp;
	vfs_t	*vfsp;
	int	hold_rename_lock;
	pl_t s;

	/* don't allow '/' characters in pathname component */
	for (c = namep, namelen = 0; *c; c++, namelen++) {
		if (*c == '/') {
			return (EACCES);
		}
	}
	ASSERT(namelen != 0);
	/*
	 * If name is "." or ".." then if this is a create look it up
	 * and return EEXIST.  Rename or link TO "." or ".." is forbidden.
	 */
	if (namep[0] == '.' &&
	   (namelen == 1 || (namelen == 2 && namep[1] == '.'))) {
		if (op == DE_RENAME) {
			return (EINVAL);
		}
		if (ipp) {
			error = s5_dirlook(tdp, namep, ipp, 1, cr);
			if (error) {
				return (error);
			}
		}
		return (EEXIST);
	}

	/*
	 * For mkdir, ensure that we won't be exceeding the maximum
	 * link count of the parent directory.
	 */
	if (op == DE_MKDIR && tdp->i_nlink >= maxlink)
		return (EMLINK);

	/*
	 * For link and rename, ensure that the source has not been
	 * removed while it was unlocked, that the source and target
	 * are on the same file system, and that we won't be exceeding
	 * the maximum link count of the source.  If all is well,
	 * synchronously update the link count.
	 */
	have_write_error = 0;
	if (op == DE_LINK || op == DE_RENAME) {
		S5_IRWLOCK_WRLOCK(sip);
		if (sip->i_nlink == 0) {
			S5_IRWLOCK_UNLOCK(sip);
			return (ENOENT);
		}
		if (sip->i_dev != tdp->i_dev) {
			S5_IRWLOCK_UNLOCK(sip);
			return (EXDEV);
		}
		if (sip->i_nlink >= maxlink) {
			S5_IRWLOCK_UNLOCK(sip);
			return (EMLINK);
		}
		sip->i_nlink++;
		s = S5_ILOCK(sip);
		IMARK(sip, ICHG|ISYN);
		S5_IUNLOCK(sip, s);

		s5_iupdat(sip);

		have_write_error = s5_iaccess(sip, IWRITE, cr);
		S5_IRWLOCK_UNLOCK(sip);
	}

	tip = NULL;
	vfsp = ITOV(tdp)->v_vfsp;
	s5fsp = S5FS(vfsp);
	if (op == DE_RENAME && (sip->i_mode & IFMT) == IFDIR && sdp != tdp) {
		SLEEP_LOCK(&s5fsp->fs_renamelock, PRINOD);
		hold_rename_lock = 1;
	} else {
		hold_rename_lock = 0;
	}

	/*
	 * Lock the directory in which we are trying to make the new entry.
	 */
	S5_IRWLOCK_WRLOCK(tdp);
	/*
	 * If target directory has not been removed, then we can consider
	 * allowing file to be created.
	 */
	if (tdp->i_nlink == 0) {
		error = ENOENT;
		goto out;
	}
	/*
	 * Check accessibility of directory.
	 */
	if ((tdp->i_mode & IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}
	/*
	 * Execute access is required to search the directory.
	 */
	error = s5_iaccess(tdp, IEXEC, cr);
	if (error) {
		goto out;
	}
	/*
	 * If this is a rename of a directory and the parent is
	 * different (".." must be changed), then the source
	 * directory must not be in the directory hierarchy
	 * above the target, as this would orphan everything
	 * below the source directory.  Also the user must have
	 * write permission in the source so as to be able to
	 * change "..".
	 */
	if (hold_rename_lock) {
		if (have_write_error) {
			error = have_write_error;
			goto out;
		}
		error = s5_dircheckpath(sip, tdp);
		if (error) {
			goto out;
		}
	}
	/*
	 * Search for the entry.
	 */
	if (error = s5_dircheckforname(tdp, namep, &offset, &tip))
		goto out;

	if (tip) {
		switch (op) {

		case DE_CREATE:
		case DE_MKDIR:
			if (ipp) {
				*ipp = tip;
				error = EEXIST;
			} else
				s5_iput(tip);
			break;

		case DE_RENAME:
			error = s5_dirrename(sdp, sip, tdp, namep,
					     tip, offset, cr);
			s5_iput(tip);
			break;

		case DE_LINK:
			/*
			 * Can't link to an existing file.
			 */
			s5_iput(tip);
			error = EEXIST;
			break;
		}
	} else {
		/*
		 * The entry does not exist.  Check write permission in
		 * directory to see if entry can be created.
		 */
		if (error = s5_iaccess(tdp, IWRITE, cr))
			goto out;
		if (op == DE_CREATE || op == DE_MKDIR) {
			/*
			 * Make new inode and directory entry as required.
			 */
			if (error = s5_dirmakeinode(tdp, &sip, vap, op, cr))
				goto out;
		}
		if (error = s5_diraddentry(tdp, namep, offset, sip, sdp, op)) {
			if (op == DE_CREATE || op == DE_MKDIR) {
				/*
				 * Unmake the inode we just made.
				 */
				if (op == DE_MKDIR)
					tdp->i_nlink--;
				sip->i_nlink = 0;
				s = S5_ILOCK(sip);
				IMARK(sip, ICHG);
				S5_IUNLOCK(sip, s);
				VN_RELE_CRED(ITOV(sip), cr);
			}
		} else if (ipp) {
			S5_IRWLOCK_WRLOCK(sip);
			*ipp = sip;
		} else if (op == DE_CREATE || op == DE_MKDIR) {
			VN_RELE_CRED(ITOV(sip), cr);
		}
	}

out:
	if (tdp != tip) {
		S5_IRWLOCK_UNLOCK(tdp);
	}

	if (error && (op == DE_LINK || op == DE_RENAME)) {
		/*
		 * Undo bumped link count.
		 */
		S5_IRWLOCK_WRLOCK(sip);
		sip->i_nlink--;
		s = S5_ILOCK(sip);
		IMARK(sip, ICHG);
		S5_IUNLOCK(sip, s);
		S5_IRWLOCK_UNLOCK(sip);
	}

	if (hold_rename_lock) {
		SLEEP_UNLOCK(&s5fsp->fs_renamelock);
	}
	return (error);
}


/*
 * int
 * s5_dirremove(inode_t *dp, char *namep, inode_t *oip,
 * 		vnode_t *cdir, enum dr_op op, cred_t *cr)
 * 	Delete a directory entry.  If oip is nonzero the entry is checked
 * 	to make sure it still reflects oip.
 *
 * Calling/Exit State:
 *	<dp> is unlocked on entry; remains unlocked on exit.
 *	If given, <oip> is unlocked on entry; remains unlocked on exit.
 *
 * Description:
 *	If oip is nonzero the entry is checked to make sure it
 *	still reflects oip.
 */
int
s5_dirremove(inode_t *dp, char *namep, inode_t *oip,
	     vnode_t *cdir, enum dr_op op, cred_t *cr)
{
	inode_t	 *ip;
	int	 error;
	int	 dotflag;
	int	 vplocked;
	off_t	 offset;
	vnode_t	 *vp;
	pl_t s;
	direct_t dir;
	static	 direct_t emptydirect[] = {
		0, ".",
		0, "..",
	};

	vplocked = 0;
	ip = NULL;
	S5_IRWLOCK_WRLOCK(dp);
	/*
	 * Check accessibility of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}
	error = s5_iaccess(dp, IEXEC|IWRITE, cr);
	if (error) {
		goto out;
	}
	error = s5_dircheckforname(dp, namep, &offset, &ip);
	if (error) {
		goto out;
	}
	if (ip == NULL || (oip && oip != ip)) {
		error = ENOENT;
		goto out;
	}
	/*
	 * Don't remove a mounted-on directory (the possible result
	 * of a race between mount(2) and unlink(2) or rmdir(2)).
	 */
	vp = ITOV(ip);
	if ((ip->i_mode & IFMT) == IFDIR) {
		/*
		 * Since the target is a directory
		 * Racing with a concurrent mount?
		 * Drop ip's rwlock and get the vnode
		 * r/w lock to synchronize with mount.
		 * Inode rwlock is dropped to avoid the locking
		 * violation before grabing vnode lock.
		 *
		 * Ok to release lock here because we
		 * already have our reference to the
		 * inode and we hold the containing
		 * directory locked.
		 */
		S5_IRWLOCK_UNLOCK(ip);

		RWSLEEP_WRLOCK(&vp->v_lock, PRIVFS);

		vplocked++;

		VN_LOCK(vp);
		if (vp->v_flag & VMOUNTING){
			VN_UNLOCK(vp);
			/*
			 * concurrent mount -> let mount win
			 */
			S5_IRWLOCK_WRLOCK(ip);
			error = EBUSY;
			goto out;
		}

		/*
		 * Can't remove mount points
		 */
		if (vp->v_vfsmountedhere) {
			VN_UNLOCK(vp);
			S5_IRWLOCK_WRLOCK(ip);
			error = EBUSY;
			goto out;
		}

		vp->v_flag |= VGONE;
		VN_UNLOCK(vp);

		S5_IRWLOCK_WRLOCK(ip);
	}

	/*
	 * If the parent directory is "sticky", then the user must
	 * own the parent directory or the file in it, or else must
	 * have permission to write the file.  Otherwise it may not
	 * be deleted (except by the super-user).  This implements
	 * append-only directories.
	 */
	if ((dp->i_mode & ISVTX) && pm_denied(cr, P_OWNER)
	  && cr->cr_uid != dp->i_uid && cr->cr_uid != ip->i_uid
	  && (error = s5_iaccess(ip, IWRITE, cr)))
		goto out;

	if (op == DR_RMDIR) {
		/*
		 * For rmdir(2), some special checks are required.
		 * (a) Don't remove any alias of the parent (e.g. ".").
		 * (b) Don't remove the current directory.
		 * (c) Make sure the entry is (still) a directory.
		 * (d) Make sure the directory is empty.
		 */
		if (dp == ip || vp == cdir)
			error = EBUSY;
		else if ((ip->i_mode & IFMT) != IFDIR)
			error = ENOTDIR;
		else if (!s5_dirempty(ip, &dotflag))
			error = EEXIST;
		if (error)
			goto out;
	} else if (op == DR_REMOVE) {
		/*
		 * unlink(2) requires a different check: allow only
		 * a privileged user to unlink a directory.
		 */
		if (vp->v_type == VDIR) {
			if (pm_denied(cr, P_FILESYS)) {
				error = EPERM;
				goto out;
			}
			if (!s5_dirempty(ip, &dotflag)) {
				error = EEXIST;
				goto out;
			}
		}
	}
	/*
	 * Zero the i-number field of the directory entry.  Retain the
	 * file name in the empty slot, as UNIX has always done.
	 */
	dir.d_ino = 0;
	(void) strncpy(dir.d_name, namep, DIRSIZ);
	error = s5_rdwri(UIO_WRITE, dp, (caddr_t) &dir, SDSIZ, offset,
			 UIO_SYSSPACE, IO_SYNC, (int *) 0);
	if (error) {
		goto out;
	}
	dnlc_remove(ITOV(dp), namep);
	/*
	 * Now dispose of the inode and
   	 * update parent link count if remove a directory.
	 */
	if ((op == DR_RMDIR) || (op == DR_REMOVE && vp->v_type == VDIR)) {
		if (dotflag & DOT) {
			ip->i_nlink -= 2;
			dnlc_remove(ITOV(ip), ".");
		} else
			ip->i_nlink--;
		if (dotflag & DOTDOT) {
			dp->i_nlink--;
			dnlc_remove(ITOV(ip), "..");
		}
		/*
		 * If other references exist, zero the "." and "..
		 * entries so they're inaccessible (POSIX requirement).
		 * If the directory is going away we can avoid doing
		 * this work.
		 */
		if (vp->v_count > 1 && ip->i_nlink <= 0)
			(void) s5_rdwri(UIO_WRITE, ip, (caddr_t) emptydirect,
					min(sizeof(emptydirect), ip->i_size),
					(off_t) 0, UIO_SYSSPACE, 0, (int *) 0);
	} else
		ip->i_nlink--;
	

	if (ip->i_nlink < 0) {	/* Pathological */
		/*
		 *+ Somehow, ip->i_nlink was < 0; this should never
		 *+ occur. Set it to 0.
		 */
#ifdef DEBUG
		cmn_err(CE_WARN, "dirremove: ino %d, dev %x, nlink %d",
				  ip->i_number, ip->i_dev, ip->i_nlink);
#endif
		ip->i_nlink = 0;
	}
	s = S5_ILOCK(dp);
	IMARK(dp, IUPD | ICHG);
	S5_IUNLOCK(dp, s);

	if (ip != dp) {
		s = S5_ILOCK(ip);
		IMARK(ip, ICHG);
		S5_IUNLOCK(ip, s);
	}

out:
	if (vplocked) {
		if (error) {
			VN_LOCK(vp);
			vp->v_flag &= ~VGONE;
			VN_UNLOCK(vp);
		}
		RWSLEEP_UNLOCK(&vp->v_lock);
	}

	if (ip) {
		s5_iput(ip);
	}
	if (ip != dp) {
		S5_IRWLOCK_UNLOCK(dp);
	}
	return (error);
}


