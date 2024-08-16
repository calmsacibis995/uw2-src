/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/sfs/sfs_dir.c	1.69"
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
 * Directory manipulation routines.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/debug.h>
#include <util/sysmacros.h>
#include <util/cmn_err.h>
#include <util/inline.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/disp.h>
#include <io/uio.h>
#include <fs/dow.h>
#include <fs/vnode.h>
#include <fs/vfs.h>
#include <fs/fbuf.h>
#include <fs/stat.h>
#include <fs/mode.h>
#include <fs/dnlc.h>
#include <mem/seg_map.h>
#include <mem/kmem.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <fs/sfs/sfs_dow.h>
#include <fs/sfs/sfs_hier.h>
#include <fs/sfs/sfs_inode.h>
#include <fs/sfs/sfs_fs.h>
#include <fs/sfs/sfs_fsdir.h>
#include <fs/sfs/sfs_quota.h>
#include <fs/fs_subr.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <acc/dac/acl.h>
#ifdef CC_PARTIAL
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif

extern short maxlink;

/*
 * If "dircheckforname" fails to find an entry with the given name, this
 * structure holds state for "sfs_direnter" as to where there is space to put
 * an entry with that name.
 *
 * If "dircheckforname" finds an entry with the given name, this structure
 * holds state for "dirrename" and "sfs_dirremove" as to where the entry is.
 *
 * "status" indicates what "dircheckforname" found:
 *	NONE		name not found, large enough free slot not found,
 *			can't make large enough free slot by compacting entries
 *	COMPACT		name not found, large enough free slot not found,
 *			can make large enough free slot by compacting entries
 *	FOUND		name not found, large enough free slot found
 *	EXIST		name found
 *
 * If "dircheckforname" fails due to an error, this structure is not filled in.
 *
 * After "dircheckforname" succeeds the values are:
 *	status	offset		size		fbp, ep
 *	------	------		----		-------
 *	NONE	end of dir	needed		not valid
 *	COMPACT	start of area	of area		not valid
 *	FOUND	start of entry	of ent		not valid
 *	EXIST	start if entry	of prev ent	valid
 *
 * "endoff" is set to 0 if the an entry with the given name is found, or if no
 * free slot could be found or made; this means that the directory should not
 * be truncated.  If the entry was found, the search terminates so
 * "dircheckforname" didn't find out where the last valid entry in the
 * directory was, so it doesn't know where to cut the directory off; if no free
 * slot could be found or made, the directory has to be extended to make room
 * for the new entry, so there's nothing to cut off.
 * Otherwise, "endoff" is set to the larger of the offset of the last
 * non-empty entry in the directory, or the offset at which the new entry will
 * be placed, whichever is larger.  This is used by "diraddentry"; if a new
 * entry is to be added to the directory, any complete directory blocks at the
 * end of the directory that contain no non-empty entries are lopped off the
 * end, thus shrinking the directory dynamically.
 *
 * On success, "sfs_dirprepareentry" makes "fbp" and "ep" valid.
 */
struct slot {
	enum	{NONE, COMPACT, FOUND, EXIST} status;
	off_t	offset;		/* offset of area with free space */
	int	size;		/* size of area at slotoffset */
	struct	fbuf *fbp;	/* dir buf where slot is */
	struct direct *ep;	/* pointer to slot */
	off_t	endoff;		/* last useful location found in search */
};

/*
 * A virgin directory.
 */
STATIC struct dirtemplate sfs_mastertemplate = {
	0, 12, 1, ".",
	0, DIRBLKSIZ - 12, 2, ".."
};

#define DOT	0x01
#define DOTDOT	0x02

/*
 * The SFS_DIRSIZ macro gives the minimum record length which will hold
 * a directory entry given the name length.  This requires the amount
 * of space for the name with a terminating null byte, rounded up to a 4
 * byte boundary.
 */
#define SFS_DIRSIZ(len) \
    ((sizeof (struct direct) - (SFS_MAXNAMLEN + 1)) + ((len + 1 + 3) &~ 3))

void sfs_dirbad();

extern void	sfs_iupdat(inode_t *, enum iupmode);
extern void	sfs_iput(inode_t *, cred_t *);
extern int	sfs_iaccess(inode_t *, mode_t, cred_t *);
extern int	sfs_rdwri(enum uio_rw, inode_t *, caddr_t, int, off_t,
			enum uio_seg, int *, cred_t *);
extern void	sfs_fsinvalid(vfs_t *);
extern int	sfs_iget(vfs_t *, fs_t *, ino_t, inode_t **, int,
			 cred_t *);
extern int	sfs_itrunc(inode_t *, uint_t, cred_t *);
extern int	sfs_ialloc(inode_t *, ino_t, mode_t, inode_t **, cred_t *,
			int *);
extern dquot_t	*sfs_getinoquota(inode_t *, cred_t *);
#ifdef _SFS_SOFT_DNLC
extern boolean_t sfs_tryhold(vnode_t *);
#endif
void sfs_unlink_pending(inode_t *, off_t);
void sfs_unlink_order(inode_t *, dowid_t, cred_t *);

/*
 * void
 * sfs_dirbad(inode_t *ip, char *how, off_t offset)
 *	Report a bad directory.
 *
 * Calling/Exit State:
 *	The caller has found a bad directory entry while
 *	searching <ip>.
 */
void
sfs_dirbad(inode_t *ip, char *how, off_t offset)
{
	/*
	 *+ While searching a directory, the system discovered that
	 *+ the directory had a corrupted format.
	 *+ Corrective action:  run fsck on that filesystem.
	 */
	cmn_err(CE_NOTE, "%s: bad dir ino %d at offset %d: %s\n",
	        ip->i_fs->fs_fsmnt, ip->i_number, offset, how);
	return;
}

#ifdef CC_PARTIAL
/*
 * find_msb(int n).  Returns bit number (numbering from right) of most
 * significant bit in n.  find_msb(0)==-1.
 *
 * Calling/Exit State:
 */
static int
find_msb(int n)
{
	unsigned int i;
	int ret;

	ret = -1;
	i = n;
	while (i) {
		++ret;
		i >>= 1;
	}
	return (ret);
}
#endif /* CC_PARTIAL */

/*
 * STATIC int
 * sfs_blkatoff(inode_t *ip, off_t offset, char **res, fbuf_t **fbpp,
 *		cred_t *cr)
 *	Return buffer with contents of block "offset"
 *	from the beginning of directory "ip".
 *
 * Calling/Exit State:
 *	The caller holds ip's rwlock at least *shared*.
 *
 * Description:
 *	If "res" is non-zero, fill it in with a pointer to the
 *	remaining space in the directory.
 */
/* ARGSUSED */
STATIC int
sfs_blkatoff(inode_t *ip, off_t offset, char **res, fbuf_t **fbpp, cred_t *cr)
{
	struct fs	*fs;
	struct fbuf	*fbp;
	u_int		bsize;
	int		error;

	fs = ip->i_fs;
	bsize = MIN(fragroundup(fs, ip->i_size) - (offset & fs->fs_bmask),
								fs->fs_bsize);
 	error = fbread(ITOV(ip), (long)(offset&fs->fs_bmask), bsize,
							S_OTHER, &fbp);
	if (error) {
		*fbpp = (struct fbuf *)NULL;
	} else {
		MET_DIRBLK(MET_SFS);
		if (res) {
			*res = fbp->fb_addr + blkoff(fs, offset);
		}
		*fbpp = fbp;
	}
	return (error);
}

/*
 * STATIC int
 * sfs_dircheckpath(inode_t *source, inode_t *target, cred_t *cr)
 *	Check if source directory is in the path of the target directory.
 *
 * Calling/Exit State:
 *	The caller holds target's rwlock *exclusive* on entry; the
 *	lock is dropped and re-acquired before returning.
 *
 *	<source> is unlocked on entry; remains unlocked on exit.
 *
 *	Return value is 0 if source directory is *not* on the path
 *	of the target, EINVAL if it is, or possibly another error
 *	code if a different error is encountered.
 *
 * Description:
 */
STATIC int
sfs_dircheckpath(inode_t *source, inode_t *target, cred_t *cr)
{
	struct fbuf *fbp;
	struct dirtemplate	*dirp;
	inode_t		*ip;
	inode_t		*tip;
	ino_t		dotdotino;
	int		error = 0;
	vfs_t		*vfsp;
	fs_t		*fs;

	/*
	 * If two renames of directories were in progress at once, the partially
	 * completed work of one dircheckpath could be invalidated by the other
	 * rename.  To avoid this, all directory renames within a given file
	 * system are serialized in sfs_direnter.
	 */
	ip = target;
	if (ip->i_number == source->i_number) {
		error = EINVAL;
		goto out;
	}
	if (ip->i_number == SFSROOTINO) {
		goto out;
	}
	/*
	 * Search back through the directory tree, using the ".." entries.
	 * Fail any attempt to move a directory into an ancestor directory.
	 */
	fbp = NULL;
	for (;;) {
		if (((ip->i_mode & IFMT) != IFDIR) || ip->i_nlink == 0
		    || ip->i_size < sizeof (struct dirtemplate)) {
			sfs_dirbad(ip,
			           "bad size, unlinked or not dir", (off_t)0);
			error = ENOTDIR;
			break;
		}
		error = sfs_blkatoff(ip, (off_t)0, (char **)&dirp, &fbp, cr);
		if (error) {
			break;
		}
		if (dirp->dotdot_namlen != 2 ||
		    dirp->dotdot_name[0] != '.' ||
		    dirp->dotdot_name[1] != '.') {
			sfs_dirbad(ip, "mangled .. entry", (off_t)0);
			error = ENOTDIR;		/* Sanity check */
			break;
		}
		dotdotino = dirp->dotdot_ino;
		if (dotdotino == source->i_number) {
			error = EINVAL;
			break;
		}
		if (dotdotino == SFSROOTINO) {
			break;
		}
		if (fbp) {
			fbrelse(fbp, 0);
			fbp = NULL;
		}
		/*
		 * Save v_vfsp, i_fs since we can't reference
		 * ip after sfs_iput.
		 */
		fs = ip->i_fs;
		vfsp = ITOV(ip)->v_vfsp;
		/*
		 * Get the parent's inode, but in order to avoid
		 * deadlock, do not acquire the IRWLOCK on it
		 * until we drop the IRWLOCK we presently own.
		 */
		error = sfs_iget(vfsp, fs, dotdotino, &tip,
						IG_NCREATE|IG_PR_WARN, cr);
		if (ip != target) {
			sfs_iput(ip, cr);
		} else {
			SFS_IRWLOCK_UNLOCK(ip);
		}
		if (error) {
			ip = NULL;
			break;
		}
		ip = tip;
		SFS_IRWLOCK_WRLOCK(ip);
	}
	if (fbp) {
		fbrelse(fbp, 0);
	}
out:

	if (ip && (ip != target)) {
		sfs_iput(ip, cr);
		/*
		 * Relock target and make sure it has not gone away
		 * while it was unlocked.
		 */
		SFS_IRWLOCK_WRLOCK(target);
		if ((error == 0) && (target->i_nlink == 0)) {
			error = ENOENT;
		}
	}
	return (error);
}


/*
 * int
 * sfs_dircheckforname(inode_t *tdp, char *namep, int namelen,
 *		       struct slot *slotp, inode_t **ipp, cred_t *cr)
 *	Check for the existence of a name in a directory,
 *	or else of an empty slot in which an entry may be made.
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
 *	If the requested name is found, then on return *ipp points at the
 *	(locked) inode and *offp contains its offset in the directory.
 *	If the name is not found, then *ipp will be NULL and *slotp will
 *	contain information about a directory slot in which an entry may be
 *	made (either an empty slot, or the first position past the end of
 *	the directory).
 *
 *	This may not be used on "." or "..", but aliases of "." are ok.
 */
STATIC int
sfs_dircheckforname(inode_t *tdp, char *namep, int namelen,
		    struct slot *slotp, inode_t **ipp, cred_t *cr)
	/* tdp - inode of directory being checked */
	/* namep - name we're checking for */
	/* namelen - length of name */
	/* slotp - slot structure */
	/* ipp - return inode if we find one */
	/* cr - user credentials */
{
	int		dirsize;	/* size of the directory */
	struct fbuf	*fbp;		/* pointer to directory block */
	int		entryoffsetinblk;/* offset of ep in fbp's buffer */
	int		slotfreespace;	/* free space in block */
	struct direct	*ep;		/* directory entry */
	off_t		offset;		/* offset in the directory */
	off_t		last_offset;	/* last offset */
	off_t		enduseful;	/* pointer past last used dir slot */
	int		remaining;	/* length remaining in dir block */
	int		needed;
	int		error;
	pl_t		opl;

	fbp = NULL;
	entryoffsetinblk = 0;
	needed = SFS_DIRSIZ(namelen);
	/*
	 * No point in using i_diroff since we must search whole directory
	 */
	dirsize = roundup(tdp->i_size, DIRBLKSIZ);
	enduseful = 0;
	offset = last_offset = 0;
	while (offset < dirsize) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 */
		if (blkoff(tdp->i_fs, offset) == 0) {
			if (fbp != NULL) {
				fbrelse(fbp, 0);
			}
			error = sfs_blkatoff(tdp, offset, (char **)0, &fbp, cr);
			if (error) {
				return (error);
			}
			entryoffsetinblk = 0;
		}
		/*
		 * If still looking for a slot, and at a DIRBLKSIZ
		 * boundary, have to start looking for free space
		 * again.
		 */
		if (slotp->status == NONE &&
		    (entryoffsetinblk & (DIRBLKSIZ-1)) == 0) {
			slotp->offset = -1;
			slotfreespace = 0;
		}
		/*
		 * Get pointer to next entry.
		 * Since we are going to do some entry manipulation
		 * we do some checks.
		 */
		/* LINTED pointer alignment*/
		ep = (struct direct *)(fbp->fb_addr + entryoffsetinblk);
		remaining = SFS_DIR_REMAINING(entryoffsetinblk);
		if (SFS_DIR_MANGLED(remaining, ep)) {
			sfs_dirbad(tdp, "mangled entry", offset);
			offset += remaining;
			entryoffsetinblk += remaining;
			continue;
		}
		/*
		 * If an appropriate sized slot has not yet been found,
		 * check to see if one is available. Also accumulate space
		 * in the current block so that we can determine if
		 * compaction is viable.
		 */
		if (slotp->status != FOUND) {
			int size = ep->d_reclen;

			if (ep->d_ino != 0) {
				size -= DIRSIZ(ep);
			}
			if (size > 0) {
				if (size >= needed) {
					slotp->status = FOUND;
					slotp->offset = offset;
					slotp->size = ep->d_reclen;
				} else if (slotp->status == NONE) {
					slotfreespace += size;
					if (slotp->offset == -1)
						slotp->offset = offset;
					if (slotfreespace >= needed) {
						slotp->status = COMPACT;
						slotp->size =
						    offset + ep->d_reclen -
						    slotp->offset;
					}
				}
			}
		}
		/*
		 * Check for a name match.
		 */
		if (ep->d_ino && ep->d_namlen == namelen &&
		    *namep == *ep->d_name &&	/* fast chk 1st char */
		    bcmp(namep, ep->d_name, namelen) == 0) {
			opl = SFS_ILOCK(tdp);
			tdp->i_diroff = offset;
			SFS_IUNLOCK(tdp, opl);
			if (tdp->i_number == ep->d_ino) {
				*ipp = tdp;	/* we want ourself, ie "." */
				VN_HOLD(ITOV(tdp));
			} else {
				error = sfs_iget((ITOV(tdp))->v_vfsp, tdp->i_fs,
				            ep->d_ino, ipp, 
					    IG_EXCL|IG_NCREATE|IG_PR_WARN, cr);
				if (error) {
					fbrelse(fbp, 0);
					return (error);
				}
			}
			slotp->status = EXIST;
			slotp->offset = offset;
			slotp->size = offset - last_offset;
			slotp->fbp = fbp;
			slotp->ep = ep;
			slotp->endoff = 0;
			return (0);
		}
		last_offset = offset;
		offset += ep->d_reclen;
		entryoffsetinblk += ep->d_reclen;
		if (ep->d_ino) {
			enduseful = offset;
		}
	}
	if (fbp) {
		fbrelse(fbp, 0);
	}
	if (slotp->status == NONE) {
		/*
		 * We didn't find a slot; the new directory entry should be put
		 * at the end of the directory.  Return an indication of where
		 * this is, and set "endoff" to zero; since we're going to have
		 * to extend the directory, we're certainly not going to
		 * trucate it.
		 */
		slotp->offset = dirsize;
		slotp->size = DIRBLKSIZ;
		slotp->endoff = 0;
	} else {
		/*
		 * We found a slot, and will return an indication of where that
		 * slot is, as any new directory entry will be put there.
		 * Since that slot will become a useful entry, if the last
		 * useful entry we found was before this one, update the offset
		 * of the last useful entry.
		 */
		if (enduseful < slotp->offset + slotp->size) {
			enduseful = slotp->offset + slotp->size;
		}
		slotp->endoff = roundup(enduseful, DIRBLKSIZ);
	}
	*ipp = (inode_t *)NULL;
	return (0);
}


/*
 * sfs_dirmakedirect(inode_t *ip, inode_t *dp, cred_t *cr)
 *	Write a prototype directory into the empty inode <ip>,
 *	whose parent is dp.
 *
 * Calling/Exit State:
 *	The caller holds both ip's and dp's rwlock *exclusive* on
 *	entry and exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EMLINK	<dp> already has a link count of maxlink.
 *
 * Description:
 *	This routine modifies file system structural data to create a
 *	directory and specifies the order in which the data must be
 *	written to disk.  The modifications are:
 *
 *		(1) increment dp's link count (parent directory)
 *		(2) create pages for ip (using directory template)
 *		(3) increment ip's link count and update its size and
 *			data block pointers (done by caller after returning
 *			from here)
 *
 *	The ordering is:
 *                     (1) ----->(2)------>(3)
 *		
 *	In words, the incremented link count for dp must be written
 *	to disk before the pages for ip, and the pages for ip must be
 *	written to disk before the size, data blocks, and incremented
 *	link count for ip are written to disk.
 *
 * Remarks:
 *	Note that the write (3) of the inode buffer to disk must
 *	precede the writing of the directory entry to disk; the
 *	creation of the directory entry and the ordering specification
 *	is handled by the caller of this routine.
 */
/* ARGSUSED */
STATIC int
sfs_dirmakedirect(inode_t *ip, inode_t *dp, cred_t *cr)
{
	int		error;
	struct fbuf	*fbp;
	struct dirtemplate *dirp;
	dowid_t		ipdow;	/* dow for buffer containing ip's inode */
	dowid_t		dpdow;	/* dow for buffer containing dp's inode */
	dowid_t		ippgdow;/* dow for first page/block of ip */
	pl_t opl;

	if (dp->i_nlink >= maxlink) {
		return (EMLINK);
	}
	ASSERT(DIRBLKSIZ <= ip->i_fs->fs_fsize);

	/*
	 * Update the dp link count and write out the change.
	 * This reflects the ".." entry we'll soon write.
	 */
	dp->i_nlink++;
	opl = SFS_ILOCK(dp);
	IMARK(dp, ICHG);
	SFS_IUNLOCK(dp, opl);
	dpdow = sfs_dow_iupdat(dp);

	/*
	 * Initialize directory with "."
	 * and ".." from static template.
	 */

	error = fbread(ITOV(ip), 0, (u_int)ip->i_fs->fs_fsize, S_WRITE, &fbp);
	
	ip->i_size = DIRBLKSIZ;
	if (!error) {
		ippgdow = sfs_dow_create_page(ip, 0, DIRBLKSIZ);
		/*
		 * Write ip's pages to disk after dp
		 */
		sfs_dow_order(ippgdow, dpdow);
		dow_startmod(ippgdow);
		/* LINTED pointer alignment*/
		dirp = (struct dirtemplate *)fbp->fb_addr;
		/*
		 * Now initialize the directory we're creating
		 * with the "." and ".." entries.
		 */
		*dirp = sfs_mastertemplate;	/* structure copy */
		dirp->dot_ino = ip->i_number;
		dirp->dotdot_ino = dp->i_number;
		if (ippgdow == DOW_NONE) {
			error = fbrelse(fbp,
			SFS_VFS_PRIV(ITOV(ip)->v_vfsp)->vfs_fbrel_flags);
		} else {
			error = fbrelse(fbp, 0);
			dow_setmod(ippgdow, FLUSHDELTAMAX);
		}
		/*
		 * Write out ip after writing out its pages.
		 */
		ipdow = sfs_dow_create_inode(ip);
		sfs_dow_order(ipdow, ippgdow);
		dow_rele(ippgdow);
		dow_rele(ipdow);
	}
	dow_rele(dpdow);

	opl = SFS_ILOCK(ip);
	IMARK(ip, IUPD|ICHG);
	SFS_IUNLOCK(ip, opl);
	return (error);
}

/*
 * int
 * sfs_dirempty(inode_t *ip, cred_t *cr, int *dotflagp)
 *	Check if a directory is empty or not.
 *
 * Calling/Exit State:
 *	Caller holds ip's rwlock exclusive on entry and exit.
 *
 * Description:
 *	Using a struct dirtemplate here is not precisely
 *	what we want, but better than using a struct direct.
 *
 *	N.B.: does not handle corrupted directories.
 */
STATIC int
sfs_dirempty(inode_t *ip, cred_t *cr, int *dotflagp)
{
	off_t			off;
	struct dirtemplate	dbuf;
	struct direct 		*dp = (struct direct *)&dbuf;
	int			error;
	int			count;
#define	MINDIRSIZ (sizeof (struct dirtemplate) / 2)

	*dotflagp = 0;	
	for (off = 0; off < ip->i_size; off += dp->d_reclen) {
		error = sfs_rdwri(UIO_READ, ip, (caddr_t)dp, (int)MINDIRSIZ,
		                  off, UIO_SYSSPACE, &count, cr);
		/*
		 * Since we read MINDIRSIZ, residual must
		 * be 0 unless we're at end of file.
		 */
		if (error || count != 0 || dp->d_reclen == 0) {
			return (0);
		}
		/* skip empty entries */
		if (dp->d_ino == 0) {
			continue;
		}
		/* accept only "." and ".." */
		if (dp->d_namlen > 2) {
			return (0);
		}
		if (dp->d_name[0] != '.') {
			return (0);
		}
		/*
		 * At this point d_namlen must be 1 or 2.
		 * 1 implies ".", 2 implies ".." if second
		 * char is also "."
		 */
		if (dp->d_namlen == 1) {
			*dotflagp |= DOT;
			continue;
		}	
		/* don't check parentino, link will change it */
		if (dp->d_name[1] == '.') {
			*dotflagp |= DOTDOT;
			continue;
		}	
		return (0);
	}
	return (1);
}

/*
 * sfs_dirdefacl(tdp, ip, cr)
 *	Add an ACL to a newly created file based on the default
 *	ACL of the parent directory.
 *
 * Calling/Exit State:
 *	<tdp> is locked exclusive on entry; remains so locked on exit.
 *	<ip> is locked exclusive on entry; remains so locked on exit.
 *
 * Description:
 *	If the new file is a directory, the default entries are
 *	propagated as defaults in addition to the non-defaults that
 *	are applied.
 */
/* ARGSUSED */
STATIC int
sfs_dirdefacl(inode_t *tdp, inode_t *ip, cred_t *cr)
	/* tdp - ptr to parent directory inode */
	/* ip - ptr to new inode */
	/* cr - caller's credentials */
{
        struct acl     *srcaclp;             /* source ACL */
        struct acl     *tgtaclp;             /* target ACL */
        long   		dentries;            /* # parent default entries */
        long   		i;
        vnode_t    *vp = ITOV(ip);      /* new vnode */
        struct acl      *tmpaclp;            /* parent default ACL buffer*/
        struct acl      *newaclp;            /* new file ACL buffer */
        struct acl      *user_1p = NULL;     /* 1st USER entry */
        struct acl      *group_1p = NULL;    /* 1st GROUP entry */
        struct acl      base_grp = {GROUP_OBJ, (uid_t)0, (ushort)0};
        long            entries;             /* # entries in ACL buffer */
        long            bufsize;             /* size of newaclp */
	long		tmpsize;	     /* size of tmpaclp */
	long            groups = 0;
        long            group_obj = 0;
        long            users = 0;
	mode_t		type = ip->i_mode & IFMT;	/* file type */
        mode_t          ownclass = ip->i_mode & 0700;   /* owner class bits */
        mode_t          grpclass = ip->i_mode & 070;    /* group class bits */
        mode_t          othclass = ip->i_mode & 07;     /* other class bits */
        int             error;

	ASSERT(!UFSIP(tdp));
        dentries = tdp->i_daclcnt;	
        ASSERT(dentries > 0);
        tmpsize = dentries + 1;         /* room for GROUP_OBJ if necessary */
        entries = dentries;             /* entries for new file's ACL */
        tmpaclp = (struct acl *)kmem_alloc(tmpsize * sizeof(struct acl),
                                                KM_SLEEP);
        /* get default ACL from parent directory inode */
        error = sfs_aclget(tdp, tmpaclp, 1);	/* get defaults only */
        if (error) {
                kmem_free(tmpaclp, tmpsize * sizeof(struct acl));
                return (error);
        }
        /* propagate defaults if this is a new directory */
        if (vp->v_type == VDIR) {
                bufsize = tmpsize + dentries;
                entries += dentries;
                newaclp = (struct acl *)kmem_alloc(bufsize * sizeof(struct acl),                                                KM_SLEEP);
                tgtaclp = newaclp + dentries + 1;
                bcopy((caddr_t)tmpaclp, (caddr_t)tgtaclp, 
			dentries * sizeof(struct acl));
        } else {
		bufsize = tmpsize;
		newaclp = (struct acl *)kmem_alloc(bufsize * sizeof(struct acl),
						   KM_SLEEP);
                tgtaclp = newaclp + dentries + 1;
        }
        /*
         * scan defaults, looking for base entries,
         * and use them to set permission bits.
         * set defaults to access-related entries (turn off DEFAULT flag)
         */
        srcaclp = tmpaclp;
        for (i = 0; i < dentries; i++, srcaclp++) {
                srcaclp->a_type &= ~ACL_DEFAULT;
                switch (srcaclp->a_type) {
                case USER_OBJ:
                        ownclass &= ((long)srcaclp->a_perm & 07) << 6;
                        entries--;      /* don't store USER_OBJ in ACL */
                        break;
                case USER:
                        if (user_1p == NULL)
                                user_1p = srcaclp;
                        users++;
                        break;
                case GROUP_OBJ:
                        group_obj++;
                        /*
                         * if additional USER or GROUP entries we're gonna
			 * store GROUP_OBJ in the ACL.  Note that DEF_GROUP
			 * must be used, not GROUP, because the masking of
			 * ACL_DEFAULT done above was for srcaclp, but we're
			 * checking (srcaclp + 1)
                         */
                        if ((user_1p != NULL) ||
			    ((srcaclp + 1)->a_type == DEF_GROUP)) {
                                group_1p = srcaclp;
                                groups++;
                        } else
                                entries--;
                        break;
                case GROUP:
                        if (group_1p == NULL)
                                group_1p = srcaclp;
                        groups++;
                        break;
                case CLASS_OBJ:
                        grpclass &= ((long)srcaclp->a_perm & 07) << 3;
			entries--;	/* don't store CLASS_OBJ in ACL */
			break;
                case OTHER_OBJ:
                        othclass &= (long)srcaclp->a_perm & 07;
                        entries--;      /* don't store OTHER_OBJ in ACL */
                        break;
                }       /* end switch */
        }       /* end for */
        if (groups > 0) {
                /* if default had additional groups, copy them first */
                tgtaclp -= groups;
                bcopy((caddr_t)group_1p, (caddr_t)tgtaclp,
		      groups * sizeof(struct acl));
        }
        /*
         * if no default GROUP_OBJ were specified, and
         * there were default additional USER or GROUP entries,
	 * we've gotta build a GROUP_OBJ entry
         */
        if (((groups > 0) || (users > 0)) && (group_obj == 0)) {
                base_grp.a_perm = (ushort)grpclass >> 3;
                tgtaclp--;
                bcopy((caddr_t)&base_grp, (caddr_t)tgtaclp, sizeof(struct acl));
                entries++;
        }
        if (users > 0) {
                /* if default had additional users, copy them next */
                tgtaclp -= users;
                bcopy((caddr_t)user_1p, (caddr_t)tgtaclp,
		      users * sizeof (struct acl));
        }

        if ((vp->v_type == VDIR) && (entries > acl_getmax()) && 
		pm_denied(cr, P_FILESYS))
		error = ENOSPC;
	else {
        	/* now, set the ACL on the disk inode & disk blocks */
        	error = sfs_aclstore(ip, tgtaclp, entries,
                                (vp->v_type == VDIR) ? dentries : 0, cr);
        	/* set the permission bits */
        	ip->i_mode = type | ownclass | grpclass | othclass;
	}
        kmem_free(tmpaclp, tmpsize * sizeof(struct acl));
        kmem_free(newaclp, bufsize * sizeof(struct acl));
        return (error);
}

/*
 * int
 * sfs_dirmakeinode(inode_t *tdp, inode_t **ipp,
 *		    vattr_t *vap, enum de_op op, cred_t *cr, dowid_t *ipdowp)
 *	Allocate and initialize a new inode that will go into directory tdp.
 *
 * Calling/Exit State:
 *	The  caller holds tdp's rwlock exclusive on entry; remains locked
 *	on exit.
 *
 *	On success, <*ipp> is returned referenced, <*ipp->i_rwlock> is
 *	unlocked, and <*ipdowp> contains a dow corresponding to the buffer
 *	containing the on-disk inode data for ip.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. There are no errnos returned directly
 *	by this routine.
 */
STATIC int
sfs_dirmakeinode(inode_t *tdp, inode_t **ipp,
		 vattr_t *vap, enum de_op op, cred_t *cr, dowid_t *ipdowp)
{
	inode_t		*ip;
	enum vtype	type;
	int		imode;		/* mode and format as in inode */
	ino_t		ipref;
	int		error;
	vnode_t		*tvp = ITOV(tdp);
	lid_t		nlid;
	int		eff_dir = 0;
	int		iskipped;
	pl_t		pl;
	sfs_vfs_t	*sfs_vfsp;

	ASSERT(vap != NULL);
	ASSERT(op == DE_CREATE || op == DE_MKDIR || op == DE_MKMLD);
	ASSERT((vap->va_mask & (AT_TYPE|AT_MODE)) == (AT_TYPE|AT_MODE));

	if (vap->va_type == VLNK) {
		nlid = tvp->v_lid;
	} else {
		nlid = cr->cr_lid;
	}

	/*
	 * Allocate a new inode.
	 */
	type = vap->va_type;
	if (type == VDIR) {
		ipref = sfs_dirpref(tdp->i_fs);
	} else {
		ipref = tdp->i_number;
	}

	imode = MAKEIMODE(type, vap->va_mode);

	error = sfs_ialloc(tdp, ipref, imode, &ip, cr, &iskipped);
	if (error) {
		return (error);
	}

	ASSERT(ip->i_dquot == NULL);
	ip->i_mode = imode;
	ip->i_ic.ic_eftflag = (ulong)EFT_MAGIC;
	if (type == VBLK || type == VCHR || type == VXNAM) {
		(ITOV(ip))->v_rdev = ip->i_rdev = vap->va_rdev;
	}

	if (!UFSIP(tdp)) {
#ifdef CC_PARTIAL
		/*
		 * Covert channel check.  If inode was previously used at a
		 * different level, record as potential covert channel
		 */
		if (ip->i_lid != cr->cr_lid) {
			if (iskipped) {
				iskipped = find_msb(iskipped);
			}
			CC_COUNT(CC_ALLOC_INODE, (long)iskipped + 2);
		}
#endif /* CC_PARTIAL */
		(ITOV(ip))->v_lid = ip->i_lid = nlid; /* MAC */
		(ITOV(ip))->v_macflag = VMAC_SUPPORT;
	}
	(ITOV(ip))->v_type = type;
	if (type == VDIR && (op == DE_MKDIR || op == DE_MKMLD)) {
		ip->i_nlink = 2;      /*anticipating a call to dirmakedirect */
		if (op == DE_MKMLD && !(UFSIP(tdp))) {
			ip->i_sflags |= ISD_MLD; /*indicate MLD*/
			(ITOV(ip))->v_macflag |= VMAC_ISMLD;
		}
	} else {
		ip->i_nlink = 1;
	}
	ip->i_uid = cr->cr_uid;
	/*
         * To determine the group-id of the created file:
	 *  1) If this is a creation of an effective directory by
	 *     virtue of an automatic creation, i.e., process is
	 *     in virtual MLD mode, just take the DAC information
	 *     from the parent directory (MLD).
	 *  2) If the gid is set in the attribute list (non-Sun & pre-4.0
	 *     clients are not likely to set the gid), then use it if
	 *     the process is privileged, belongs to the target group,
	 *     or the group is the same as the parent directory.
	 *  3) If the filesystem was not mounted with the Old-BSD-compatible
	 *     GRPID option, and the directory's set-gid bit is clear,
	 *     then use the process's gid.
	 *  4) Otherwise, set the group-id to the gid of the parent directory.
	 */
	if (op == DE_MKDIR && !UFSIP(tdp) &&
	    !(cr->cr_flags & CR_MLDREAL) && tdp->i_sflags & ISD_MLD) {
		/*
		 * If there are stored ACL entries on parent dir,
		 * allocate space to store the ACL entries,
		 * store ACL entries on inode, and free the space.
		 */
		if (tdp->i_aclcnt && dac_installed) {
			struct acl *tmpaclp;	/* tmp storage for ACL */
			int tmpsize;		/* size of tmp storage */

			tmpsize = tdp->i_aclcnt * sizeof(struct acl);
			tmpaclp = (struct acl *)kmem_alloc(tmpsize, KM_SLEEP);
			if (((error = sfs_aclget(tdp, tmpaclp, 0)) != 0) ||
		  	    ((error = sfs_aclstore(ip, tmpaclp, tdp->i_aclcnt,
					tdp->i_daclcnt, cr)) != 0)) {
				/* Throw away inode just allocated */
				kmem_free(tmpaclp, tmpsize);
				ip->i_nlink = 0;
				pl = SFS_ILOCK(ip);
				IMARK(ip, ICHG);
				SFS_IUNLOCK(ip, pl);
				sfs_iput(ip, cr);
				return (error);
			}
			kmem_free(tmpaclp, tmpsize);
		} else {
			ip->i_aclcnt = 0;
			ip->i_daclcnt = 0;
			ip->i_aclblk = (daddr_t)0;
		}

		ip->i_uid = tdp->i_uid;
		ip->i_gid = tdp->i_gid;
		ip->i_mode = tdp->i_mode;

		eff_dir = 1;

	} else if ((vap->va_mask & AT_GID) &&
	    ((vap->va_gid == tdp->i_gid) || groupmember(vap->va_gid, cr) ||
	    (!pm_denied(cr, P_OWNER)))) {
		ip->i_gid = vap->va_gid;
	} else {
		if (tdp->i_mode & ISGID) {
			ip->i_gid = tdp->i_gid;
		} else {
			ip->i_gid = cr->cr_gid;
		}
	}
	/*
	 * If we're creating a directory, and the parent directory has the
	 * set-GID bit set, set it on the new directory.
	 * Otherwise, if the user is neither privileged nor a member of the
	 * file's new group, clear the file's set-GID bit.
	 */
	if (tdp->i_mode & ISGID && type == VDIR) {
		ip->i_mode |= ISGID;
	} else {
		if ((ip->i_mode & ISGID) && !groupmember((uid_t)ip->i_gid, cr)
		    && pm_denied(cr, P_OWNER))
			ip->i_mode &= ~ISGID;
	}
	sfs_vfsp = (sfs_vfs_t *)(ITOV(ip))->v_vfsp->vfs_data;
	if (UFSIP(ip)) {
		pl = QLIST_LOCK();
		if (sfs_vfsp->vfs_qinod != NULL) {
			QLIST_UNLOCK(pl);
			ip->i_dquot = sfs_getinoquota(ip, cr);
		} else {
			QLIST_UNLOCK(pl);
		}
	}

	/* 
	 * If the parent directory has default ACL entries, call
	 * sfs_dirdefacl to apply the defaults to the new file
	 */
	if (!UFSIP(tdp) && !eff_dir) {
		if (tdp->i_daclcnt && dac_installed) {
		   	error = sfs_dirdefacl(tdp, ip, cr);
		   	if (error) {
				/* Throw away inode just allocated */
				ip->i_nlink = 0;
				ip->i_flag |= ICHG;
				sfs_iput(ip, cr);
				return (error);
			}
		} else {
			/* otherwise clear all inode ACL info */
			ip->i_aclcnt = 0;
			ip->i_daclcnt = 0;
			ip->i_aclblk = (daddr_t)0;
		}
	}

	pl = SFS_ILOCK(ip);
	IMARK(ip, IACC|IUPD|ICHG);
	SFS_IUNLOCK(ip, pl);

	/*
	 * If this is a directory, then go make it
	 */
	if (op == DE_MKDIR || op == DE_MKMLD)
		error = sfs_dirmakedirect(ip, tdp, cr);
	/*
	 * If an error, throw away the inode we just allocated; otherwise
	 * make sure inode goes to disk before directory data and entries
	 * pointing to it. Then unlock it, since nothing points to it yet.
	 */
	if (error) {
		/* Throw away inode we just allocated. */
		ip->i_nlink = 0;
		ip->i_flag |= ICHG;
		sfs_iput(ip, cr);
	} else {
		*ipdowp = sfs_dow_iupdat(ip);
		SFS_IRWLOCK_UNLOCK(ip);
		*ipp = ip;
	}
	return (error);
}

/*
 * int
 * sfs_dirfixdotdot(inode_t *dp, inode_t *opdp, inode_t *npdp, cred_t *cr)
 *	Rewrite the ".." entry of the child directory so that it
 *	points to the new parent directory instead of the old one.
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
 *		EMLINK	<npdp> already has a link count of maxlink.
 *
 * Description:
 *	This routine modifies file system structural data to move a
 *	directory from one directory to another, and specifies the order
 *	in which the data must be written to disk.   In the following
 *	description:
 *
 *		dp:	is the inode of the directory which is being moved
 *		opdp:	is the inode of the directory where dp used to live
 *		npdp:	is the inode of the directory where dp is going to live
 *
 *	(opdp and npdp are "old" and "new" parent directory pointer,
 *	respectively)
 *
 *	The modifications are:
 *
 *		(1) Increment the new parent directory's inode link count
 *		(2) Change the inode number for the ".." entry in dp's
 *			first page to the new parent directory's inode number
 *		(3) If there is an old parent directory, decrement the link
 *			count in its inode
 *
 *	The ordering is:
 *			(1) ----> (2) ----> (3)
 *		
 *	In words, the incremented link count for npdp must be written to disk
 *	first; the change to the inode number for the ".." in dp's pages
 *	is written after that; and the decremented link count for the old
 *	parent directory (opdp) is written last.
 *
 * Remarks:
 *	Obtain <dp->i_rwlock> in exclusive mode. If <dp> doesn't
 *	exist any more (nlink == 0) or <dp> is smaller than the
 *	smallest allowable directory, than return 0.
 *
 */
STATIC int
sfs_dirfixdotdot(inode_t *dp, inode_t *opdp, inode_t *npdp, cred_t *cr)
		/* dp - child directory */
		/* opdp - old parent directory */
		/* npdp -new parent directory */
{
	struct fbuf		*fbp = NULL;
	struct dirtemplate	*dirp;
	int			error = 0;
	dowid_t			npdpdow;	/* dow for new parent inode */
	dowid_t			dpgdow;		/* dow for child's page */
	pl_t			opl;

	SFS_IRWLOCK_WRLOCK(dp);
	/*
	 * Check whether this is an ex-directory.
	 */
	if (dp->i_nlink == 0 || dp->i_size < sizeof (struct dirtemplate)) {
		goto out;
	}

	error = sfs_blkatoff(dp, (off_t)0, (char **)&dirp, &fbp, cr);
	if (error) {
		goto bad;
	}
	if (dirp->dotdot_ino == npdp->i_number)	{ /* Just a no-op. */
		goto bad;
	}
	if (dirp->dotdot_namlen != 2 ||
	    dirp->dotdot_name[0] != '.' ||
	    dirp->dotdot_name[1] != '.') {	/* Sanity check. */
		sfs_dirbad(dp, "mangled .. entry", (off_t)0);
		error = ENOTDIR;
		goto bad;
	}

	/*
	 * Increment the link count in the new parent inode and update
	 *	the inode buffer via a delayed ordered write.
	 */
	if (npdp->i_nlink >= maxlink) {
		error = EMLINK;
		goto out;
	}
	npdp->i_nlink++;
	opl = SFS_ILOCK(npdp);
	IMARK(npdp, ICHG);
	SFS_IUNLOCK(npdp, opl);
	npdpdow = sfs_dow_iupdat(npdp);

	/*
	 * Rewrite the child ".." entry and arrange for it to
	 *	be written after the new parent's inode.
	 */
	dpgdow = sfs_dow_create_page(dp, 0, DIRBLKSIZ);
	sfs_dow_order(dpgdow, npdpdow);
	dow_rele(npdpdow);
	dnlc_remove(ITOV(dp), "..");
	dow_startmod(dpgdow);
	dirp->dotdot_ino = npdp->i_number;
	if (dpgdow != DOW_NONE) {
		error = fbrelse(fbp, 0);
		dow_setmod(dpgdow, sfs_hardening.sfs_dir);
	} else {
		error = fbrelse(fbp,
			SFS_VFS_PRIV(ITOV(npdp)->v_vfsp)->vfs_fbrel_flags);
	}
	fbp = NULL;
	if (error) {
		dow_rele(dpgdow);
		goto bad;
	}

	dnlc_enter(ITOV(dp), "..", ITOV(npdp), (void *) dp->i_agen, NOCRED);
	SFS_IRWLOCK_UNLOCK(dp);

	/*
	 * Decrement the link count of the old parent inode and arrange for
	 * it to be written after dp's revised ".." entry.  If opdp is NULL,
	 * then this is a new directory link; it has no parent, so we need
	 * not do anything.
	 */
	if (opdp != NULL) {
		SFS_IRWLOCK_WRLOCK(opdp);
		if (opdp->i_nlink != 0) {
			dowid_t opdow;
			opdow = sfs_dow_create_inode(opdp);
			sfs_dow_order(opdow, dpgdow);
			dow_rele(opdow);
			opdp->i_nlink--;
			opl = SFS_ILOCK(opdp);
			IMARK(opdp, ICHG);
			SFS_IUNLOCK(opdp, opl);
			dow_rele(sfs_dow_iupdat(opdp));
		}
		SFS_IRWLOCK_UNLOCK(opdp);
	}
	dow_rele(dpgdow);
	return (0);

bad:
	if (fbp) {
		fbrelse(fbp, 0);
	}

out:
	SFS_IRWLOCK_UNLOCK(dp);
	return (error);
}

/*
 * int
 * sfs_dirrename(inode_t *sdp, inode_t *sip, inode_t *tdp,
 *               char *namep, inode_t *tip, struct slot *slotp, cred_t *cr,
 *		dowid_t sipdow)
 *	Rename the entry in the directory <tdp> so that it points to
 *	<sip> instead of <tip>.
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
 *		EXDEV	Either <tip> and <tdp> or <tip> and <sip> are
 *			on different file systems
 *		EISDIR	<tip> refers to a directory, but <sip> is not
 *			a directory.
 *		EBUSY	<tip> is a mounted on directory.
 *		EEXIST	<tip> is a non-empty directory.
 *		ENOTDIR <sip> is a directory but <tip> is not.
 *		EIO	<tip> is a directory that contained more than 2 links.
 *
 * Description:
 *	This routine modifies file system structural data to change the
 *	inode to which a directory entry refers, and specifies the order
 *	in which the data must be written to disk.   In the following
 *	description:
 *
 *		tip:	inode to which the directory entry currently refers
 *		sip:	inode to which the directory entry will refer
 *		tdp:	inode for directory containing entry
 *		sdp:	inode for directory containing original entry referring
 *				to sip
 *
 *	The structural modifications made by this routine are:
 *
 *		(1) Change the inode number for the directory entry, from
 *			tip's inode number to sip's inode number
 *		(2) Decrement link count for tip, and if it's a directory,
 *			call sfs_itrunc to remove it
 *		(3) If sip is a directory, then call sfs_dirfixdotdot to make
 *			sip's ".." entry point to tdp instead of sdp
 *
 *	The ordering requirements are that (1) be written before (2).  (3)
 *	can occur at any time.
 *
 *	In addition, prior to this routine, sip's link count was bumped,
 *	and that must be written before (1).  A dowid, sipdow, is passed
 *	in which corresponds to the buffer containing sip's data.
 *
 *	Thus, the order is:
 *			(sipdow)----->(1)------->(2)
 *	
 *	and, as noted (3) is independent of this ordering.
 *
 * Remarks:
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
STATIC int
sfs_dirrename(inode_t *sdp, inode_t *sip, inode_t *tdp,
		char *namep, inode_t *tip, struct slot *slotp, cred_t *cr,
		dowid_t sipdow)
	/* sdp - parent directory of source */
	/* sip - source inode */
	/* tdp - parent directory of target */
	/* namep - entry we are trying to change */
	/* tip	- locked target inode */
	/* slotp - slot for entry */
	/* cr -	credentials */
{
	int 		error;
	boolean_t	doingdirectory;
	int		dotflag;
	vnode_t		*vp;
	int		vplocked;
	dowid_t		tdppgdow, tipdow;
	pl_t		opl;

	ASSERT(slotp->fbp != NULL);

	/*
	 * Must have write permission to rewrite target entry
	 */
	error = sfs_iaccess(tdp, IWRITE, cr);
	if (error) {
		return (error);
	}

	/*
	 * Check that everything is on the same filesystem.
	 */
	if (((ITOV(tip))->v_vfsp != (ITOV(tdp))->v_vfsp) ||
	    ((ITOV(tip))->v_vfsp != (ITOV(sip))->v_vfsp))
		return (EXDEV);

	/*
	 * If the parent directory is "sticky", then the user must own
	 * either the parent directory or the destination of the rename,
	 * or else must have permission to write the destination.
	 * Otherwise the destination may not be changed (except with
	 * privilege).  This implements append-only directories.
	 */
	if ((tdp->i_mode & ISVTX) && cr->cr_uid != tdp->i_uid
	    && cr->cr_uid != tip->i_uid && pm_denied(cr, P_OWNER)
	    && (error = sfs_iaccess(tip, IWRITE, cr)))
		return (error);

	vplocked = 0;
	doingdirectory = ((sip->i_mode & IFMT) == IFDIR);

	/*
	 * Ensure source and target are compatible (both directories
	 * or both not directories).  If target is a directory it must
	 * be empty and have no links to it; in addition it must not
	 * be a mount point.
	 */
	if ((tip->i_mode & IFMT) == IFDIR) {
		/*
		 * Target is a directory. Source must be a directory
		 * and target must be empty.
		 */
		if (!doingdirectory) {
			return (EISDIR);
		}

		/*
		 * Racing with a concurrent mount?
		 * Drop tip's rwlock and get the vnode
		 * r/w lock to synchronize with mount.
		 *
		 * Ok to release lock here because we
		 * already have our reference to the
		 * inode and we hold the containing
		 * directory locked.
		 */
		SFS_IRWLOCK_UNLOCK(tip);

		vp = ITOV(tip);
		RWSLEEP_WRLOCK(&vp->v_lock, PRIVFS);

		vplocked++;

		VN_LOCK(vp);
		if (vp->v_flag & VMOUNTING){
			VN_UNLOCK(vp);
			/*
			 * concurrent mount -> let mount win
			 */
			RWSLEEP_UNLOCK(&(ITOV(tip))->v_lock);
			SFS_IRWLOCK_WRLOCK(tip);
			return (EBUSY);
		}

		/*
		 * Can't remove mount points
		 */
		if (vp->v_vfsmountedhere) {
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&vp->v_lock);
			SFS_IRWLOCK_WRLOCK(tip);
			return (EBUSY);
		}

		vp->v_flag |= VGONE;
		VN_UNLOCK(vp);

		SFS_IRWLOCK_WRLOCK(tip);

		/* Release fbp, since only one allowed at a time. */
		ASSERT(slotp->fbp != NULL);
		ASSERT(slotp->status == EXIST);
		fbrelse(slotp->fbp, SM_SETMOD);
		slotp->fbp = NULL;

		if (!sfs_dirempty(tip, cr, &dotflag) ||
		    (tip->i_nlink > 2)) {
			VN_LOCK(vp);
			vp->v_flag &= ~VGONE;
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&vp->v_lock);
			return (EEXIST);
		}

		/* Re-acquire fbp */
		error = sfs_blkatoff(tdp, slotp->offset, (char **)&slotp->ep,
				     &slotp->fbp, cr);
		if (error) {
			goto bad;
		}
	} else if (doingdirectory) {
		return (ENOTDIR);
	}

	tdppgdow = sfs_dow_create_page(tdp, slotp->offset & ~(DIRBLKSIZ - 1),
		DIRBLKSIZ);
	/*
	 * Rewrite the inode pointer for target name entry
	 * from the target inode (ip) to the source inode (sip).
	 * This prevents the target entry from disappearing
	 * during a crash. Mark the directory inode to reflect the changes.
	 * Specify that the modified directory entry should be written to
	 * disk after sip's link count is written to disk.
	 */
	dnlc_remove(ITOV(tdp), namep);
	ASSERT(slotp->fbp != NULL);
	sfs_dow_order(tdppgdow, sipdow);
	dow_startmod(tdppgdow);
	slotp->ep->d_ino = sip->i_number;
	if (tdppgdow != DOW_NONE) {
		error = fbrelse(slotp->fbp, 0);
		dow_setmod(tdppgdow, doingdirectory ? sfs_hardening.sfs_dir :
			sfs_hardening.sfs_file);
	} else {
		error = fbrelse(slotp->fbp, doingdirectory ?
			SFS_VFS_PRIV(ITOV(sip)->v_vfsp)->vfs_fbrel_flags : 0);
	}
	slotp->fbp = NULL;
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), (void *) tdp->i_agen, NOCRED);
	if (error) {
		dow_rele(tdppgdow);
		goto bad;
	}

	opl = SFS_ILOCK(tdp);
	IMARK(tdp, IUPD|ICHG);
	SFS_IUNLOCK(tdp, opl);
	/*
	 * Decrement the link count of the target inode, but first
	 * make sure that the modified link count will be written to
	 * disk *after* the modified target directory entry.
	 */
	tipdow = sfs_dow_create_inode(tip);
	sfs_dow_order(tipdow, tdppgdow);
	dow_rele(tdppgdow);
	dow_rele(tipdow);
	tip->i_nlink--;
	/*
	 * If sip and tip are directories, then:
	 *	(1) truncate tip
	 *	(2) fix the ".." entry in sip to point to tdp.
	 */
	if (doingdirectory) {
		/*
		 * Decrement target link count once more if it was a directory.
		 */
		if (dotflag & DOT) {
			--tip->i_nlink;
			dnlc_remove(ITOV(tip), ".");
		}
		if (tip->i_nlink != 0) {
			/*
			 *+ After the kernel ensured that the directory was
			 *+ empty (via diremtpy()), and decremented the link
			 *+ count for the entry in the parent dir and the "."
			 *+ entry, it found that the link count was not zero.
			 *+ This indicates a possible filesystem corruption
			 *+ problem.  Corrective action:  run fsck on the
			 *+ filesystem.
			 */
			cmn_err(CE_WARN,
			  "UFS/SFS sfs_dirrename: target directory link count");
			sfs_fsinvalid(ITOV(tip)->v_vfsp);
			error = EIO;
		}
		error = sfs_itrunc(tip, (u_long)0, cr);

		if (error) {
			goto bad;
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
		 * different, dirfixdotdot() will bump the link count
		 * back.
		 */
		if (dotflag & DOTDOT) {
			tdp->i_nlink--;
			dnlc_remove(ITOV(tip), "..");
		}
		if (sdp != tdp) {
			error = sfs_dirfixdotdot(sip, sdp, tdp, cr);
			if (error) {
				goto bad;
			}
		}
	}
	opl = SFS_ILOCK(tip);
	IMARK(tip, ICHG);
	SFS_IUNLOCK(tip, opl);

bad:
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
 * sfs_dirprepareentry(inode_t *dp, struct slot *slotp, cred_t *cr)
 *	Prepare a directory slot to receive an entry.
 *
 * Calling/Exit State:
 *	The caller holds dp's rwlock *exclusive* on entry; remains locked
 *	on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	<slotp->offset> is not on a directory block boundary
 *			and a new block must be allocated.
 *		EIO	<slotp->status> is invalid.
 *
 * Description:
 *	Allocate space (<slotp->status> == NONE) or adjust the size
 *	of the directory as required. Then, depending on the
 *	value of <slotp->status>, prepare for the new entry.
 *	NONE indicates that the block should be zero'd. COMPACT
 *	or FOUND indicates that free space is available but
 *	may require moving current entries forward such that
 *	all free space is at the end.
 *	
 */
STATIC int
sfs_dirprepareentry(inode_t *dp, struct slot *slotp, cred_t *cr)
	/* dp - directory we are working in */
	/* slotp - available slot info */
	/* cr - user credentials */
{
	u_short		slotfreespace;
	u_short		dsize;
	int		loc;
	struct direct	*ep;
	struct direct	*nep;
	char		*dirbuf;
	off_t		entryend;
	int		error;
	pl_t		opl;

	/*
	 * If we didn't find a slot, then indicate that the
	 * new slot belongs at the end of the directory.
	 * If we found a slot, then the new entry can be
	 * put at slotp->offset.
	 */
	entryend = slotp->offset + slotp->size;
	if (slotp->status == NONE) {
		if (slotp->offset & (DIRBLKSIZ - 1)) {
			/*
			 *+ When trying to create a new entry in a directory, 
			 *+ the kernel did not find a vacant slot. This
			 *+ indicates that a new block needs to be allocated;
			 *+ however, the offset was not on a directory block
			 *+ size boundary.
			 */
			cmn_err(CE_WARN, 
				"UFS/SFS sfs_dirprepareentry: new block");
			sfs_fsinvalid(ITOV(dp)->v_vfsp);
			return (EIO);
		}
		ASSERT(DIRBLKSIZ <= dp->i_fs->fs_fsize);
		/*
		 * Allocate the new block.
		 */
		error = pvn_trunczero(ITOV(dp), (uint_t)slotp->offset,
						 DIRBLKSIZ);

		if (error)
			return (error);

		opl = SFS_ILOCK(dp);
		IMARK(dp, IUPD|ICHG);
		SFS_IUNLOCK(dp, opl);
	} else if (entryend > dp->i_size) {
		/*
		 * Adjust directory size, if needed. This should never
		 * push the size past a new multiple of DIRBLKSIZ.
		 * This is an artifact of the old (4.2BSD) way of initializing
		 * directory sizes to be less than DIRBLKSIZ.
		 */
		dp->i_size = roundup(entryend, DIRBLKSIZ);
		opl = SFS_ILOCK(dp);
		IMARK(dp, IUPD|ICHG);
		SFS_IUNLOCK(dp, opl);
	}

	/*
	 * Get the block containing the space for the new directory entry.
	 */

	error = sfs_blkatoff(dp, slotp->offset,
			     (char **)&slotp->ep, &slotp->fbp, cr);
	if (error) {
		return (error);
	}

	ep = slotp->ep;
	switch (slotp->status) {
	case NONE:
		/*
		 * No space in the directory. slotp->offset will be on a
		 * directory block boundary and we will write the new entry
		 * into a fresh block.
		 */
		ep->d_reclen = DIRBLKSIZ;
		break;

	case FOUND:
	case COMPACT:
		/*
		 * Found space for the new entry in the range slotp->offset
		 * to slotp->offset + slotp->size in the directory.  To use
		 * this space, we have to compact the entries located there,
		 * by copying them together towards the beginning of the
		 * block, leaving the free space in one usable chunk at the end.
		 */
		dirbuf = (char *)ep;
		dsize = DIRSIZ(ep);
		slotfreespace = ep->d_reclen - dsize;
		for (loc = ep->d_reclen; loc < slotp->size; ) {
			/* LINTED pointer alignment*/
			nep = (struct direct *)(dirbuf + loc);
			if (ep->d_ino) {
				/* trim the existing slot */
				ep->d_reclen = dsize;
				/* LINTED pointer alignment*/
				ep = (struct direct *)((char *)ep + dsize);
				slotp->offset += dsize;
			} else {
				/* overwrite; nothing there; header is ours */
				slotfreespace += dsize;
			}
			dsize = DIRSIZ(nep);
			slotfreespace += nep->d_reclen - dsize;
			loc += nep->d_reclen;
			bcopy((caddr_t)nep, (caddr_t)ep, (unsigned)dsize);
		}
		/*
		 * Update the pointer fields in the previous entry (if any).
		 * At this point, ep is the last entry in the range
		 * slotp->offset to slotp->offset + slotp->size.
		 * Slotfreespace is the now unallocated space after the
		 * ep entry that resulted from copying entries above.
		 */
		if (ep->d_ino == 0) {
			ep->d_reclen = slotfreespace + dsize;
		} else {
			ep->d_reclen = dsize;
			/* LINTED pointer alignment*/
			ep = (struct direct *)((char *)ep + dsize);
			slotp->offset += dsize;
			ep->d_reclen = slotfreespace;
		}
		slotp->size = ep->d_reclen;
		break;

	default:
		/*
		 *+ When the kernel attempted to add a new entry to a
		 *+ directory, the structure describing the next available
		 *+ slot in the directory had an invalid status.
		 */
		cmn_err(CE_WARN, 
			"UFS/SFS sfs_dirprepareentry: invalid slot status");
		sfs_fsinvalid(ITOV(dp)->v_vfsp);
		return (EIO);
	}
	slotp->ep = ep;
	return (0);
}

/*
 * int
 * sfs_diraddentry(inode_t *tdp, char *namep, short namelen, struct slot *slotp,
 *		inode_t *sip, inode_t *sdp, cred_t *cr, enum de_op op,
 *		dowid_t sipdow)
 *	Enter the file sip in the directory tdp with name namep.
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
 *		EXDEV	<tip> and <sip> are on different file systems
 *
 * Description:
 *	Prepare <tdp> for the new entry by calling sfs_dirprepareentry() and
 *	verify that <tdp> and <sip> are on the same file systems. Fix
 *	the ".." of <sip> and write out the directory entry.
 *
 *	This routine modifies file system structural data to create a
 *	new directory entry for an inode, and specifies the order in
 *	which that data should be written to disk.  The following
 *	modifications are made:
 *
 *		(1) Create directory entry referencing the source inode (sip)
 *		(2) If the source inode is itself a directory, call
 *			sfs_dirfixdotdot so that the sip's ".." entry
 *			refers to the directory where the entry is (tdp)
 *		(3) If the directory tdp can be made more compact, then call
 *			sfs_itrunc to truncate it.
 *
 *	The caller should have incremented sip's link count prior to calling
 *	this routine, and provides a dowid (sipdow) which corresponds to the
 *	buffer containing the data for sip.  Thus, the ordering requirements
 *	are:
 *		(sipdow)------>(1)------>(3)
 *
 *	and (2) can be done independently.
 */
STATIC int
sfs_diraddentry(inode_t *tdp, char *namep, short namelen, struct slot *slotp,
		inode_t *sip, inode_t *sdp, cred_t *cr, enum de_op op,
		dowid_t sipdow)
{
	int		error, flags, flushdelta;
	dowid_t		tdppgdow, tdpdow;
	pl_t		opl;

	/*
	 * Check inode to be linked to see if it is in the
	 * same filesystem.
	 */
	if ((ITOV(tdp))->v_vfsp != (ITOV(sip))->v_vfsp) {
		return (EXDEV);
	}

	/*
	 * Prepare a new entry.  If the caller has not supplied a
	 * directory slot, make a new one.
	 */
	error = sfs_dirprepareentry(tdp, slotp, cr);
	if (error) {
		return (error);
	}
	ASSERT(slotp->fbp != NULL);
	ASSERT(((slotp->status == NONE)
			&& ((slotp->offset & (DIRBLKSIZ - 1)) == 0)
			&& (slotp->ep->d_reclen == DIRBLKSIZ))
		|| (((slotp->status == FOUND) || (slotp->status == COMPACT))
			&& (slotp->ep->d_reclen >= SFS_DIRSIZ(namelen)))
		|| (slotp->status == EXIST)) ;
	if ((sip->i_mode & IFMT) == IFDIR && op == DE_RENAME) {
		/* Release fbp, since only one allowed at a time. */
		slotp->ep->d_ino = 0; /* dirprepareentry may not have set it */
		fbrelse(slotp->fbp, SM_SETMOD);
		slotp->fbp = NULL;

		error = sfs_dirfixdotdot(sip, sdp, tdp, cr);

		if (error == 0) {
			/* Re-acquire fbp */
			error = sfs_blkatoff(tdp, slotp->offset,
					     (char **)&slotp->ep, &slotp->fbp,
					     cr);
		}
		if (error) {
			return (error);
		}
	}

	/*
	 * Fill in entry data, and arrange for it to be written out after
	 * the inode data (i.e., with its link count incremented).  Also
	 * call sfs_unlink_order to make sure that if any of the other pages
	 * of this directory have removed entries which haven't been written
	 * to disk yet get written before this page.  See the comment on
	 * sfs_unlink_order for details.
	 */
	tdppgdow = sfs_dow_create_page(tdp, slotp->offset & ~(DIRBLKSIZ - 1),
		DIRBLKSIZ);
	sfs_dow_order(tdppgdow, sipdow);
	/*
	 * This test is slightly stale, but thats OK; if the mask is 0, then
	 *	it's really 0, since the mask is only set if the inode
	 *	is write-locked, and we have the write-lock here, therefore
	 *	nothing is making this non-zero as we test it.  Something
	 *	could be clearing it, however, so it's checked again inside
	 *	of sfs_unlink_order to make sure it's still 0.
	 */
	if (tdp->i_unlinkmask != 0)	/* set order with pending unlinks */
		sfs_unlink_order(tdp, tdppgdow, cr);
	dow_startmod(tdppgdow);
	ASSERT(slotp->fbp != NULL);
	slotp->ep->d_namlen = namelen;
	(void) strncpy(slotp->ep->d_name, namep, (size_t)((namelen + 4) & ~3));
	slotp->ep->d_ino = sip->i_number;

	/*
	 * Release the fbp
	 */
	if (op == DE_MKDIR || op == DE_MKMLD) {
		flags = SFS_VFS_PRIV(ITOV(sip)->v_vfsp)->vfs_fbrel_flags;
		flushdelta = sfs_hardening.sfs_dir;
	} else {
		flags = SM_SETMOD;
		flushdelta = sfs_hardening.sfs_file;
	}
	if (tdppgdow != DOW_NONE) {
		error = fbrelse(slotp->fbp, 0);
		dow_setmod(tdppgdow, flushdelta);
	} else {
		error = fbrelse(slotp->fbp, flags);
	}
	slotp->fbp = NULL;
	dnlc_enter(ITOV(tdp), namep, ITOV(sip), (void *) tdp->i_agen, NOCRED);

	if (error) {
		dow_rele(tdppgdow);
		return (error);
	}

	/*
	 * Mark the directory inode to reflect the changes.
	 */
	opl = SFS_ILOCK(tdp);
	IMARK(tdp, IUPD|ICHG);
	tdp->i_diroff = 0;
	SFS_IUNLOCK(tdp, opl);
	/*
	 * Truncate the directory to chop off blocks of empty entries, and
	 * specify that the directory's inode data is to be written to disk
	 * after the new directory entry is written.
	 */
	if (slotp->endoff && slotp->endoff < tdp->i_size) {
		tdpdow = sfs_dow_create_inode(tdp);
		sfs_dow_order(tdpdow, tdppgdow);
		dow_rele(tdpdow);
		error = sfs_itrunc(tdp, (u_long)slotp->endoff, cr);
	}
	dow_rele(tdppgdow);
	return (error);
}

/*
 * int
 * sfs_dirlook(inode_t *dp, char *namep, inode_t **ipp,
 *	       int excl, cred_t *cr)
 *	Look for a given name in a directory.
 *
 * Calling/Exit State:
 *	<dp> is unlocked on entry; remains unlocked on return.
 *
 *	On successful returns, *ipp's rwlock is held *exclusive* if
 *	<excl> is non-zero, otherwise, *shared*.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		ENOTDIR	 <dp> is not a directory
 *		ENOENT	<namep> not found in <dp>
 *
 * Description:
 *	Verify that <dp> is a directory and that the calling process
 *	has search (IEXEC) permission on it.
 *
 *	Treat the null component as a synonym for <dp>. In this case,
 *	simply VN_HOLD it and obtain <dp->i_rwlock> according to
 *	<excl> and return.
 *
 *	Try to short-circuit the search by using the directory name
 *	lookup cache. If <namep> is in the cache, a vnode for
 *	it will be returned with an additional reference held. Thus,
 *	if we get a cache hit, simply obtain <ip->i_rwlock> according to
 *	<excl> and return.
 *
 *	If we get here than we have to search the directory data blocks.
 *	Obtain <dp->i_rwlock> in shared mode to prevent modifications
 *	to the directory. Start the search from the current value
 *	of the directory rotor (<dp->i_diroff>). If the search is not
 *	from the beginning of the directory, i.e., <dp->i_diroff> != 0,
 *	than 2 passes thru the directory - one for [rotor, end-of-directory]
 *	and another for [0, rotor).
 *
 *	Search for <namep> by iterating over the directory's data blocks
 *	never holding more than one block at a time.
 *
 *	For each directory data block, get a pointer to the next directory
 *	entry. If the entry is valid, then check for a name match.
 *	We must get the target inode before unlocking the directory to
 *	insure that the inode will not be removed before we get it.
 *	Deadlock is prevented by always fetching inodes from the root,
 *	moving down the directory tree. Thus when following backward
 *	pointers "..", unlock the parent directory before getting the
 *	requested directory. There is a potential race condition here if
 *	both the current and parent directories are removed before the
 *	`sfs_iget' for the inode associated with ".." returns. We hope that
 *	this occurs infrequently since we can't avoid this race condition
 *	without implementing a sophisticated deadlock detection algorithm.
 *	Note also that this simple deadlock detection scheme will not work
 *	if the file system has any hard links other than ".." that point
 *	backwards in the directory structure.
 */
int
sfs_dirlook(inode_t *dp, char *namep, inode_t **ipp, int excl, cred_t *cr)
{
	inode_t		*ip;
	vnode_t		*vp;
	struct fbuf	*fbp = NULL;	/* a buffer of directory entries */
	struct direct	*ep;		/* the current directory entry */
	int		entryoffsetinblock; /*offset of ep in addr's buffer */
	int		numdirpasses;	/* strategy for directory search */
	off_t		endsearch;	/* offset to end directory search */
	int		namelen;		/* length of name */
	off_t		offset;
	off_t		begin_offset;
	int		error;
	int		remaining;
	void		*agen;
	boolean_t	softhold;
	pl_t		opl;

	*ipp = NULL;
	/*
	 * Make sure dp is a directory...
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

	/* 
	 * If we're searching for '.' the algorithm is going to
	 * terminate early, Rather than acquire the lock in
	 * read mode and then having to drop and reacquire
	 * the lock in excl mode for '.'; grab the lock according
	 * to <excl> first.
	 */
	namelen = strlen(namep);
	if (excl && (*namep == '\0' || (namelen == 1 && *namep == '.'))) {
		SFS_IRWLOCK_WRLOCK(dp);
	} else {
		SFS_IRWLOCK_RDLOCK(dp);
	}

	error = sfs_iaccess(dp, IEXEC, cr);
	if (error) {
		SFS_IRWLOCK_UNLOCK(dp);
		return (error);
	}

	/*
	 * Null component name is synonym for directory being searched.
	 * Note: for this case, already acquired the dp lock in the
	 * correct mode.
	 */
	if (*namep == '\0' || (namelen == 1 && *namep == '.')) {
		VN_HOLD(ITOV(dp));
		*ipp = dp;
		return (0);
	}

	/*
	 * Check the directory name lookup cache.
	 * Assumes dnlc returns vp with an extra reference.
	 */
	vp = dnlc_lookup(ITOV(dp), namep, &agen, &softhold, NOCRED);
#ifdef _SFS_SOFT_DNLC
	if (vp && (!softhold || sfs_tryhold(vp))) {
#else	/* !_SFS_SOFT_DNLC */
	if (vp) {
		ASSERT(!softhold);
#endif	/* !_SFS_SOFT_DNLC */
		/*
		 * If dnlc's cached copy of the access generation number
		 * is out of date, then refresh it now.
		 */
		if ((int) agen != dp->i_agen)
			dnlc_enter(ITOV(dp), namep, vp,
				   (void *) dp->i_agen, NOCRED);
		ip = VTOI(vp);
		/*
		 * It's ok to release the dir lock at this
		 * point since the calling LWP already has
		 * a reference to the inode.
		 */
		SFS_IRWLOCK_UNLOCK(dp);
		if (excl) {
			SFS_IRWLOCK_WRLOCK(ip);
		} else {
			SFS_IRWLOCK_RDLOCK(ip);
		}
		*ipp = ip;
		return (0);
	}

	if (dp->i_diroff > dp->i_size) {
		begin_offset = 0;
	} else {
		begin_offset = dp->i_diroff;
	}

	if (begin_offset == 0) {
		/* just need to search [0, dirsize] */
		offset = 0;
		numdirpasses = 1;
	} else {
		/* need to search [dp->i_diroff, dirsize] & [0, dp->i_diroff] */
		offset = begin_offset;
		numdirpasses = 2;
		entryoffsetinblock = blkoff(dp->i_fs, offset);
		if (entryoffsetinblock != 0) {
			error = sfs_blkatoff(dp, offset, (char **)0, &fbp, cr);
			if (error) {
				goto bad;
			}
		}
	}
	endsearch = roundup(dp->i_size, DIRBLKSIZ);

searchloop:
	while (offset < endsearch) {
		/*
		 * If offset is on a block boundary,
		 * read the next directory block.
		 * Release previous if it exists.
		 * Optimization: if wrapped around end of
		 * directory and need same block, don't
		 * release it.
		 */
		if (blkoff(dp->i_fs, offset) == 0) {
			if (fbp != NULL)
				fbrelse(fbp, 0);
			error = sfs_blkatoff(dp, offset,
					    (char **)0, &fbp, cr);
			if (error) {
				goto bad;
			}
			entryoffsetinblock = 0;
		}

		/*
		 * Get pointer to next entry.
		 * Full validation checks are slow, so we only check
		 * enough to insure forward progress through the
		 * directory.
		 */
		/* LINTED pointer alignment*/
		ep = (struct direct *)(fbp->fb_addr + entryoffsetinblock);
		remaining = SFS_DIR_REMAINING(entryoffsetinblock);
		if (SFS_DIR_MANGLED(remaining, ep)) {
			sfs_dirbad(dp, "mangled entry", offset);
			offset += remaining;
			entryoffsetinblock += remaining;
			continue;
		}

		/*
		 * Check for a name match.
		 */
		if (ep->d_ino && ep->d_namlen == namelen &&
		    *namep == *ep->d_name &&	/* fast chk 1st chr */
		    bcmp(namep, ep->d_name, (int)ep->d_namlen) == 0) {
			u_long ep_ino;

			/*
			 * We have to release the fbp early here to avoid
			 * a possible deadlock situation where we have the
			 * fbp and want the directory inode and someone doing
			 * a sfs_direnter has the directory inode and wants the
			 * fbp.
			 */
			ep_ino = ep->d_ino;
			fbrelse(fbp, 0);
			fbp = NULL;
			opl = SFS_ILOCK(dp);
			dp->i_diroff = offset;
#ifdef CC_PARTIAL
			/* 
			 * Note that this check will not block because the
			 * check is an equality check.
			 * 
			 * The i_dirofflid is initially set to the caller's
			 * LID in sfs_iget().
			 */
			if (MAC_ACCESS(MACEQUAL, dp->i_dirofflid, cr->cr_lid)) {
				dp->i_dirofflid = cr->cr_lid;
				CC_COUNT(CC_SPEC_DIROFF, CCBITS_SPEC_DIROFF);
			}
#endif /* CC_PARTIAL */
			SFS_IUNLOCK(dp, opl);

			/*
			 * Are we looking for the parent directory? If
			 * so, get the inode, but in order to avoid
			 * deadlock, do not acquire the IRWLOCK on it
			 * until we drop the IRWLOCK we presently own.
			 */
			if (namelen == 2 && namep[0] == '.' && namep[1] == '.') {
				error = sfs_iget((ITOV(dp))->v_vfsp, dp->i_fs,
						ep_ino, ipp,
						IG_NCREATE|IG_PR_WARN, cr);
				if (error) {
					SFS_IRWLOCK_UNLOCK(dp);
					if (fbp) {
						fbrelse(fbp, 0);
					}
					return (error);
				}
				/*
				 * Enter ".." in dnlc
				 */
				ip = *ipp;
				dnlc_enter(ITOV(dp), namep, ITOV(ip),
					    (void *) dp->i_agen, NOCRED);
				SFS_IRWLOCK_UNLOCK(dp);
				if (excl) {
					SFS_IRWLOCK_WRLOCK(ip);
				} else {
					SFS_IRWLOCK_RDLOCK(ip);
				}
			} else if (dp->i_number == ep_ino) {
				/* want ourself, i.e., "." */
				VN_HOLD(ITOV(dp));
				*ipp = dp;
				dnlc_enter(ITOV(dp), namep, ITOV(dp),
					    (void *) dp->i_agen, NOCRED);
				if (excl) {
					SFS_IRWLOCK_UNLOCK(dp);
					SFS_IRWLOCK_WRLOCK(dp);
				}
			} else {
				/* Not looking for ".." - just get inode. */
				error = sfs_iget((ITOV(dp))->v_vfsp, dp->i_fs,
					        ep_ino, ipp,
						excl ? 
						IG_EXCL|IG_NCREATE|IG_PR_WARN :
						IG_SHARE|IG_NCREATE|IG_PR_WARN,
						cr);
				if (error) {
					goto bad;
				}
				dnlc_enter(ITOV(dp), namep, ITOV(*ipp),
					    (void *) dp->i_agen, NOCRED);
				SFS_IRWLOCK_UNLOCK(dp);
			}
			return (0);
		}
		offset += ep->d_reclen;
		entryoffsetinblock += ep->d_reclen;
	}
	/*
	 * If we started in the middle of the directory and failed
	 * to find our target, we must check the beginning as well.
	 */
	if (numdirpasses == 2) {
		numdirpasses--;
		offset = 0;
		endsearch = begin_offset;
		goto searchloop;
	}
	error = ENOENT;
bad:
	if (fbp) {
		fbrelse(fbp, 0);
	}
	SFS_IRWLOCK_UNLOCK(dp);
	return (error);
}


/*
 * int
 * sfs_direnter(inode_t *tdp, char *namep, enum de_op op,
 *             inode_t *sdp, inode_t *sip,
 *             vattr_t *vap, inode_t **ipp, cred_t *cr)
 *	Write a new directory entry.
 *
 * Calling/Exit State:
 *	<sdp>, <tdp>, and <sip> are unlocked on entry; remain unlocked on exit.
 *
 *	If <ipp> is non-NULL and errno is NULL or EEXIST,
 *	<*ipp->i_rwlock> is held *exclusive* on exit.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EACCES	<namep> contains an invalid filename character.
 *		EINVAL	<op> is DE_RENAME and <namep> is either "." or ".."
 *		EEXIST	<namep> is either "." or ".."
 *		EMLINK	<op> is DE_MKDIR or DE_MKMLD and <tdp> already
 *			has a link count of maxlink
 *		ENOENT	<op> is DE_LINK or DE_RENAME and <sip> has been
 *			removed
 *		EMLINK	<op> is DE_LINK or DE_RENAME and <sip> alread
 *			has a link count of maxlink
 *		ENOENT	<tdp> has been removed
 *		ENOTDIR	<tdp> is not a directory
 *		ESAME	(not returned to user) <op> is DE_RENAME and <sip>
 *			and <tip> refer to the same inode.
 *
 * Description:
 *
 *	The directory must not have been removed and must be writable.
 *	We distinguish five operations that build a new entry:  creating a file
 *	(DE_CREATE), making a directory (DE_MKDIR), making a Multi-Level
 *	directory (DE_MKMLD), renaming (DE_RENAME) or linking (DE_LINK). There
 *	are five possible cases to consider:
 *
 *		Name
 *		found	op			action
 *		-----	---------------------	------------------------------
 *		no	DE_CREATE, DE_MKDIR,	create file according to vap
 *			or DE_MKMLD		   and enter
 *		no	DE_LINK or DE_RENAME	enter the file sip
 *		yes	DE_CREATE, DE_MKDIR,	error EEXIST *ipp = found file
 *			or DE_MKMLD
 *		yes	DE_LINK			error EEXIST
 *		yes	DE_RENAME		remove existing file, enter new
 *						    file
 *
 *	Renames involving different directories require the rename lock to
 *	prevent concurrent directory renames from orphaning both (e.g., a
 *	cross rename) and to prevent AB/BA deadlocks.
 *
 *	<tdp> must not have been removed and must be a directory
 *	that the calling process has IEXEC access to.
 *
 *	The calling process must have IWRITE permission to the source directory
 *	if a rename involving two different directories is done.
 *	source directory.
 *
 *	Locate <namep> in <tdp> via sfs_dircheckforname().
 *	Perform the operation depending on <op>.
 *
 * Remarks:
 *	This routine creates the dow (sipdow) corresponding to the
 *	buffer containing the source inode (sip) data.  It passes
 *	sipdow to other routines in this file; those other routines
 *	modify file system structural data and specify the order in
 *	which those data should be written to disk, both with respect
 *	to sip (using sipdow) and with each other.
 */
int
sfs_direnter(inode_t *tdp, char *namep, enum de_op op,
	     inode_t *sdp, inode_t *sip,
	     vattr_t *vap, inode_t **ipp, cred_t *cr)
{
	inode_t		*tip;	/* inode of (existing) target file */
	struct slot	slot;	/* slot info to pass around */
	short		namelen;	/* length of name */
	int		error;	/* error number */
	int		hold_rename_lock;
	char		*c;
	vfs_t		*vfsp;
	sfs_vfs_t	*sfs_vfsp;
	int		have_write_error;
	dowid_t		sipdow = DOW_NONE;
	pl_t 		opl;

	ASSERT(!(UFSIP(tdp) && (op == DE_MKMLD)));

	/* don't allow '/' characters in pathname component */
	for (c = namep, namelen = 0; *c; c++, namelen++) {
		if (*c == '/') {
			return(EACCES);
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
			error = sfs_dirlook(tdp, namep, ipp, 1, cr);
			if (error) {
				return (error);
			}
		}
		return (EEXIST);
	}
	slot.status = NONE;
	slot.fbp = NULL;

	/*
	 * For mkdir and mkmld, ensure that we won't be exceeding the maximum 
	 * link count of the parent directory.
	 */
	if ((op == DE_MKDIR || op == DE_MKMLD) && tdp->i_nlink >= maxlink) {
		return (EMLINK);
	}

	/*
	 * For link and rename lock the source entry and check the link count
	 * to see if it has been removed while it was unlocked.  If not, we
	 * increment the link count and force the inode to disk to make sure
	 * that it is there before any directory entry that points to it.
	 */
	have_write_error = 0;
	if (op == DE_LINK || op == DE_RENAME) {
		SFS_IRWLOCK_WRLOCK(sip);
		if (sip->i_nlink == 0) {
			SFS_IRWLOCK_UNLOCK(sip);
			return (ENOENT);
		}

		if (sip->i_nlink >= maxlink) {
			SFS_IRWLOCK_UNLOCK(sip);
			return (EMLINK);
		}

		sip->i_nlink++;

		opl = SFS_ILOCK(sip);
		IMARK(sip, ICHG);
		SFS_IUNLOCK(sip, opl);

		sipdow = sfs_dow_iupdat(sip);

		have_write_error = sfs_iaccess(sip, IWRITE, cr);
		SFS_IRWLOCK_UNLOCK(sip);
	}


	vfsp = ITOV(tdp)->v_vfsp;
	sfs_vfsp = (sfs_vfs_t *)(vfsp->vfs_data);
	if (op == DE_RENAME && (sip->i_mode & IFMT) == IFDIR && sdp != tdp) {
		SLEEP_LOCK(&sfs_vfsp->vfs_renamelock, PRINOD);
		hold_rename_lock = 1;
	} else {
		hold_rename_lock = 0;
	}


	tip = NULL;
	/*
	 * Lock the directory in which we are trying to make the new entry.
	 * Check accessibility of directory.
	 */
	SFS_IRWLOCK_WRLOCK(tdp);
	if ((tdp->i_mode & IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}

	/*
	 * The target directory must still exist in order to create
	 * a file in it.
	 */
	if (tdp->i_nlink == 0) {
		error = ENOENT;
		goto out;
	}

	/*
	 * Execute access is required to search the directory.
	 */
	error = sfs_iaccess(tdp, IEXEC, cr);
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
		error = sfs_dircheckpath(sip, tdp, cr);
		if (error) {
			goto out;
		}
	}

	/*
	 * Search for the entry.
	 */
	error = sfs_dircheckforname(tdp, namep, namelen, &slot, &tip, cr);
	if (error) {
		goto out;
	}

	/*
	 * MAC write checks are necessary in sfs dependent code
	 * because the time between lookup and VOP_CREATE at
	 * independent level is rather long.  The vnode is released
	 * after lookup, allowing the file to be created or
	 * removed prior to getting to VOP_CREATE. The MAC check
	 * at fs independent level is, therefore, not sufficient.
	 */
	if (tip) {
		ASSERT(slot.fbp != NULL);
		ASSERT(slot.status == EXIST);
		switch (op) {
		case DE_MKDIR:
		case DE_MKMLD:
		{
			vnode_t *vp = ITOV(tip);
			if (vp->v_type != VCHR &&
			    vp->v_type != VBLK &&
			    (error = MAC_VACCESS(vp, VWRITE, cr))) {
				sfs_iput(tip, cr);
				goto out;
			}
		}
		         /* FALLTHRU */
                case DE_CREATE: /* MAC check is handled in sfs_create() */

			if (ipp) {
				*ipp = tip;
				error = EEXIST;
			} else {
				sfs_iput(tip, cr);
			}
			break;

		case DE_RENAME:
			if (sip == tip) {
				/*
				 * Short circuit rename (foo, foo)
				 */
				error = ESAME;
			} else {
				error = sfs_dirrename(sdp, sip, tdp, namep,
						      tip, &slot, cr, sipdow);
			}
			sfs_iput(tip, cr);
			break;

		case DE_LINK:
			/*
			 * Can't link to an existing file.
			 */
			sfs_iput(tip, cr);
			error = EEXIST;
			break;
		}
	} else {
		ASSERT(slot.fbp == NULL);
		/*
		 * The entry does not exist. Check write permission in
		 * directory to see if entry can be created.
		 */
		error = sfs_iaccess(tdp, IWRITE, cr);
		if (error) {
			goto out;
		}
		if (op == DE_CREATE || op == DE_MKDIR || op == DE_MKMLD) {
			/*
			 * Ideally, check MAC write permission in directory
			 * only if object existed at fs independent
			 * level prior to calling dependent create call.
			 * If object did not exist at independent level, then
			 * the MAC write check on the directory has already
			 * been done at fs independent level.  Same comments
			 * apply to fs range checks.
			 *
			 * Note that applying the MAC checks
			 * after the DAC check is not a problem
			 * in this case.
			 */
			ASSERT(vap != NULL);
			if (vap->va_type != VLNK) { 
				/* not through vn_create() */
				if ((error = MAC_CHECKS(ITOV(tdp), cr)) != 0)
					goto out;
			}
			/*
			 * Make new inode and directory entry as required.
			 */
			error = sfs_dirmakeinode(tdp, &sip, vap, op, cr,
				&sipdow);
			if (error) {
				goto out;
			}
		}
		error = sfs_diraddentry(tdp, namep, namelen, &slot,
					sip, sdp, cr, op, sipdow);
		if (error) {
			if (op == DE_CREATE ||op == DE_MKDIR ||op == DE_MKMLD) {
				/*
				 * Unmake the inode we just made.
				 */
				if ((sip->i_mode & IFMT) == IFDIR) {
					tdp->i_nlink--;
				}
				sip->i_nlink = 0;
				opl = SFS_ILOCK(sip);
				IMARK(sip, ICHG);
				SFS_IUNLOCK(sip, opl);
				VN_RELE_CRED(ITOV(sip), cr);
				sip = NULL;
			}
		} else if (ipp) {
			SFS_IRWLOCK_WRLOCK(sip);
			*ipp = sip;
		} else if (op == DE_CREATE || op == DE_MKDIR ||op == DE_MKMLD) {
			VN_RELE_CRED(ITOV(sip), cr);
		}
	}

out:
	dow_rele(sipdow);
	if (slot.fbp) {
		fbrelse(slot.fbp, 0);
	}

	if (tdp != tip) {
		SFS_IRWLOCK_UNLOCK(tdp);
	}

	if (error && (op == DE_LINK || op == DE_RENAME)) {
		/*
		 * Undo bumped link count.
		 */
		SFS_IRWLOCK_WRLOCK(sip);
		sip->i_nlink--;
		opl = SFS_ILOCK(sip);
		IMARK(sip, ICHG);
		SFS_IUNLOCK(sip, opl);
		SFS_IRWLOCK_UNLOCK(sip);
	}

	if (hold_rename_lock) {
		SLEEP_UNLOCK(&sfs_vfsp->vfs_renamelock);
	}

	return (error);
}


/*
 * int
 * sfs_dirremove(inode_t *dp, char *namep, inode_t *oip,
 *		 vnode_t *cdir, enum dr_op op, cred_t *cr)
 *	Delete a directory entry.
 *
 * Calling/Exit State:
 *	<dp> is unlocked on entry; remains unlocked on exit.
 *	If given, <oip> is unlocked on entry; remains unlocked on exit.
 *
 * Description:
 *	This routine modifies file system structural data to remove an
 *	entry from a directory, and specifies the order in which the
 *	data must be written to disk.  The structural modifications
 *	made by this routine are as follows:
 *
 *		(1) delete the directory entry
 *		(2) decrement the link count in the inode
 *		(3) If the inode is a directory, decrement the link count
 *			of the parent directory's inode
 *		(4) if the inode is a directory, overwrite its entries
 *			for "." and ".."
 *
 *	The ordering requirements are:
 *		Write the directory entry (1) before the child's inode (2)
 *		If the child is a directory, then:
 *			Write the directory entry in the parent (1) before
 *				the parent's inode (3) and the child's
 *				directory (4)
 *			Write the child's directory (4) before the child's
 *				inode (2) and the parent's inode (3)
 *
 *                        +------------>(2)
 *                        |             /|\
 *                        |              |
 *                        |              |
 *                       (1)----------->(4)
 *                        |              |
 *                        |              |
 *                        |             \|/
 *                        +------------>(3)
 *
 * Remarks:
 *	If oip is nonzero the entry is checked to make sure it
 *	still reflects oip.
 */
/* ARGSUSED */
int
sfs_dirremove(inode_t *dp, char *namep, inode_t *oip,
	      vnode_t *cdir, enum dr_op op, cred_t *cr)
{
	struct direct	*ep;
	struct direct	*pep;
	inode_t		*ip;
	int		namelen;
	struct slot	slot;
	int		error = 0;
	int		dotflag;
	int		vplocked = 0;
	vnode_t		*vp;
	dowid_t		dppgdow, ipdow, dpdow;
	pl_t		opl;
	
	dppgdow = DOW_NONE;
	namelen = strlen(namep);
	if (namelen == 0) {
		/*
		 *+ A zero-length name was specified to be removed from
		 *+ a directory. Such condition indicates possible
		 *+ file system corruption and/or a kernel programming
		 *+ error. Corrective action: run fsck.
		 */
		cmn_err(CE_WARN, 
			"UFS/SFS sfs_dirremove: directory name length zero");
		sfs_fsinvalid(ITOV(dp)->v_vfsp);
		return (EIO);
	}
	/*
	 * return error when removing . and ..
	 */
	if (namep[0] == '.') {
		if (namelen == 1)
			return (EINVAL);
		else if (namelen == 2 && namep[1] == '.') {
			return (EEXIST);
		}
	}

	ip = NULL;
	slot.fbp = NULL;

	/*
	 * Check accessibility of directory.
	 */
	if ((dp->i_mode & IFMT) != IFDIR) {
		error = ENOTDIR;
		goto out;
	}

	SFS_IRWLOCK_WRLOCK(dp);

	/*
	 * Execute access is required to search the directory.
	 * Access for write is interpreted as allowing
	 * deletion of files in the directory.
	 */
	if (error = sfs_iaccess(dp, IEXEC|IWRITE, cr))
		goto out;

	slot.status = FOUND;	/* don't need to look for empty slot */
	error = sfs_dircheckforname(dp, namep, namelen, &slot, &ip, cr);
	if (error)
		goto out;
	if (ip == NULL) {
		error = ENOENT;
		goto out;
	}
	if (oip && oip != ip) {
		error = ENOENT;
		goto out;
	}

	/*
	 * There used to be a check here to make sure you are not removing a
	 * mounted on dir.  This was no longer correct because sfs_iget() does
	 * not cross mount points anymore so the the i_dev fields in the inodes
	 * pointed to by ip and dp will never be different.  There does need
	 * to be a check here though, to eliminate the race between mount and
	 * rmdir.  (It can also be a race between mount and unlink, if your
	 * kernel allows you to unlink a directory.)
	 */
	vp = ITOV(ip);
	if ((ip->i_mode & IFMT) == IFDIR) {
		/*
		 * Since the target is a directory
		 * Racing with a concurrent mount?
		 * Drop ip's rwlock and get the vnode
		 * r/w lock to synchronize with mount.
		 *
		 * Ok to release lock here because we
		 * already have our reference to the
		 * inode and we hold the containing
		 * directory locked.
		 */
		SFS_IRWLOCK_UNLOCK(ip);

		RWSLEEP_WRLOCK(&vp->v_lock, PRIVFS);

		vplocked++;

		VN_LOCK(vp);
		if (vp->v_flag & VMOUNTING) {
			VN_UNLOCK(vp);
			/*
			 * concurrent mount -> let mount win
			 */
			SFS_IRWLOCK_WRLOCK(ip);
			error = EBUSY;
			goto out;
		}

		/*
		 * Can't remove mount points
		 */
		if (vp->v_vfsmountedhere) {
			VN_UNLOCK(vp);
			SFS_IRWLOCK_WRLOCK(ip);
			error = EBUSY;
			goto out;
		}

		vp->v_flag |= VGONE;
		VN_UNLOCK(vp);

		SFS_IRWLOCK_WRLOCK(ip);
	}

	/*
	 * If the parent directory is "sticky", then the user must
	 * own the parent directory or the file in it, or else must
	 * have permission to write the file.  Otherwise it may not
	 * be deleted (except by a privileged user).  This implements
	 * append-only directories.
	 */
	if ((dp->i_mode & ISVTX) && cr->cr_uid != dp->i_uid
	    && cr->cr_uid != ip->i_uid && pm_denied(cr, P_OWNER)
	    && (error = sfs_iaccess(ip, IWRITE, cr))) {
		goto out;
	}

	if (op == DR_RMDIR) {
		/*
		 * For rmdir(2), some special checks are required.
	 	 * (a) Don't remove any alias of the parent (e.g. ".").
	 	 * (b) Don't remove the current directory.
		 * (c) Make sure the entry is (still) a directory.
		 * (d) Make sure the directory is empty.
	 	 */
		if (dp == ip || vp == cdir) {
			error = EBUSY;
		} else if ((ip->i_mode & IFMT) != IFDIR) {
			error = ENOTDIR;
		} else {
			/* Release fbp, since only one allowed at a time. */
			ASSERT(slot.fbp != NULL);
			ASSERT(slot.status == EXIST);
			fbrelse(slot.fbp, SM_SETMOD);
			slot.fbp = NULL;

			if (!sfs_dirempty(ip, cr, &dotflag)) {
#ifdef CC_PARTIAL
				if (MAC_ACCESS(MACEQUAL, ip->i_lid, cr->cr_lid))
					CC_COUNT(CC_SPEC_DIRRM,
						 CCBITS_SPEC_DIRRM);
#endif /* CC_PARTIAL */
				error = EEXIST;
			} else {
				/* Re-acquire fbp */
				error = sfs_blkatoff(dp, slot.offset,
						     (char **)&slot.ep,
						     &slot.fbp, cr);
			}
		}
		if (error) {
			goto out;
		}
	} else if (op == DR_REMOVE) {
		/*
		 * unlink(2) requires a different check:
		 * Allow only a privileged user may unlink a directory,
		 * and it must be empty.
		 */
		if (vp->v_type == VDIR) {
			if (pm_denied(cr, P_FILESYS)) {
				error = EPERM;
				goto out;
			}

			/* Release fbp, since only one allowed at a time. */
			ASSERT(slot.fbp != NULL);
			ASSERT(slot.status == EXIST);
			fbrelse(slot.fbp, SM_SETMOD);
			slot.fbp = NULL;

			if (!sfs_dirempty(ip, cr, &dotflag)) {
				error = EEXIST;
				goto out;
			}

			/* Re-acquire fbp */
			error = sfs_blkatoff(dp, slot.offset,
					     (char **)&slot.ep, &slot.fbp, cr);
			if (error) {
				goto out;
			}
		}
	}

	/*
	 * Remove the cached entry, if any.
	 */
	dnlc_remove(ITOV(dp), namep);
	dppgdow = sfs_dow_create_page(dp, slot.offset & ~(DIRBLKSIZ - 1),
		DIRBLKSIZ);
	dow_startmod(dppgdow);

	/*
	 * If the entry isn't the first in the directory, we must reclaim
	 * the space of the now empty record by adding the record size
	 * to the size of the previous entry.
	 */
	ASSERT(slot.fbp != NULL);
	ep = slot.ep;
	if ((slot.offset & (DIRBLKSIZ - 1)) == 0) {
		/*
		 * First entry in block: set d_ino to zero.
		 */
		ep->d_ino = 0;
	} else {
		/*
		 * Collapse new free space into previous entry.
		 */
		/* LINTED pointer alignment*/
		pep = (struct direct *)((char *)ep - slot.size);
		pep->d_reclen += ep->d_reclen;
	}

	/*
	 * Clear out the entry's name.  (POSIX requirement.)
	 */
	bzero(ep->d_name, ep->d_namlen);

	if (dppgdow != DOW_NONE) {
		/*
		 * Set bit for mask indicating an unflushed remove.  This
		 *	bit will be cleared in the sfs_putpage path.
		 */  
		sfs_unlink_pending(dp, slot.offset);
		fbrelse(slot.fbp, 0);
		dow_setmod(dppgdow, FLUSHDELTAMAX);
	} else {
		fbrelse(slot.fbp,
			SFS_VFS_PRIV(vp->v_vfsp)->vfs_fbrel_flags);
	}
	slot.fbp = NULL;
	if (error) {
		dow_rele(dppgdow);
		goto out;
	}

	/*
	 * Now dispose of the inode, and make sure the inode gets
	 *	written to disk after the page containing the cleared
	 *	directory entry
	 */
	ipdow = sfs_dow_create_inode(ip);
	sfs_dow_order(ipdow, dppgdow);
	if (ip->i_nlink > 0) {
		if ((op == DR_RMDIR || op == DR_REMOVE) && vp->v_type == VDIR) {
			if (ip != dp) {
				/*
				 * Dp's link count is about to be decremented.
				 * Make sure that the change to dp gets
				 * written to disk after the cleared directory
				 * entry.
				 */
				dpdow = sfs_dow_create_inode(dp);
				sfs_dow_order(dpdow, dppgdow);
			}
			/*
			 * decrement by 2 because we're trashing the "."
			 * entry as well as removing the entry in dp.
			 * Clear the inode, but there may be other hard
			 * links so don't free the inode.
			 * Decrement the dp linkcount because we're
			 * trashing the ".." entry.
			 */
			if (dotflag & DOT) { 
				ip->i_nlink -=2;
				dnlc_remove(ITOV(ip), ".");
			} else {
				ip->i_nlink--;
			}		
			if (dotflag & DOTDOT) {
				dp->i_nlink--;
				dnlc_remove(ITOV(ip), "..");
			}
			if (vp->v_count > 1 && ip->i_nlink <= 0 &&
					ip->i_size > 0) {
				dowid_t ippgdow;
				/*
				 * overwrite the directory pages of the
				 * directory being removed, and make
				 * sure that it gets written out after
				 * the removed directory's cleared out entry
				 * in the parent directory.
				 */
				ippgdow = sfs_dow_create_page(ip, 0,
					min(sizeof(sfs_mastertemplate),
					ip->i_size));
				sfs_dow_order(ippgdow, dppgdow);
				dow_startmod(ippgdow);
				sfs_rdwri(UIO_WRITE, ip,
				          (caddr_t)&sfs_mastertemplate,
				          min(sizeof(sfs_mastertemplate),
				          ip->i_size), (off_t)0,
				          UIO_SYSSPACE, (int *)0, cr);
				dow_setmod(ippgdow, FLUSHDELTAMAX);
				/*
				 * the removed directory's inode should
				 * be written out after its pages are
				 * overwritten.
				 */
				sfs_dow_order(ipdow, ippgdow);
				if (ip != dp)
					sfs_dow_order(dpdow, ippgdow);
				dow_rele(ippgdow);
			}
			if (ip != dp)
				dow_rele(dpdow);
		} else {
			ip->i_nlink--;
		}
	}
	dow_rele(dppgdow);
	dow_rele(ipdow);
	if (ip->i_nlink < 0) {
		ip->i_nlink = 0;
	}
	opl = SFS_ILOCK(dp);
	IMARK(dp, IUPD|ICHG);
	SFS_IUNLOCK(dp, opl);

	if (ip != dp) {
		opl = SFS_ILOCK(ip);
		IMARK(ip, ICHG);
		SFS_IUNLOCK(ip, opl);
	}
out:

	if (slot.fbp)
		fbrelse(slot.fbp, 0);
	if (vplocked) {
		if (error) {
			VN_LOCK(vp);
			vp->v_flag &= ~VGONE;
			VN_UNLOCK(vp);
		}
		RWSLEEP_UNLOCK(&vp->v_lock);
	}

	if (ip)
		sfs_iput(ip, cr);
	if (ip != dp)
		SFS_IRWLOCK_UNLOCK(dp);
	return (error);
}

/*
 * void
 * sfs_unlink_pending(inode_t *dp, off_t offset)
 *	Note that there is an unlink pending in a directory.  dp is the
 *	directory inode, and offset specifies the offset of the entry which
 *	was just unlinked.
 *
 * Calling/Exit State:
 *	dp is held write-locked on entry, and kept that way.
 *
 * Description:
 *	The routine sets a bit in the i_unlinkmask of the inode corresponding
 *	to the specified offset.  Each bit in i_unlinkmask corresponds to
 *	a portion of the file; the size of each portion is the min of PAGESIZE
 *	and blocksize, which is the unit of dow creation for file "pages".
 *
 * Remarks:
 *	This routine is called from sfs_dirremove whenever an unlink from
 *	a directory occurs.
 *
 *	See the explanation under sfs_unlink_order as to why this is needed.
 */
void
sfs_unlink_pending(inode_t *dp, off_t offset)
{
	uint_t shift, bitno;
	pl_t opl;

	shift = MIN(dp->i_fs->fs_bshift, PAGESHIFT);
	bitno = (uint_t)offset >> shift;
	if (bitno >= NBITPW)
		bitno = NBITPW - 1;
	opl = SFS_ILOCK(dp);
	BITMASK1_SET1(&dp->i_unlinkmask, bitno);
	SFS_IUNLOCK(dp, opl);
}

/*
 * void
 * sfs_unlink_flushed(inode_t *dp, off_t offset)
 *	Note that a unit (page or block) of a directory is about to
 *	be written to disk, and thus contains no pending unlinks.
 *	dp is the directory inode, and offset specifies the offset
 *	of the unit which is about to be written to disk.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	The routine clears a bit in the i_unlinkmask of the inode corresponding
 *	to the specified offset.  Each bit in i_unlinkmask corresponds to
 *	a portion of the file; the size of each portion is the min of PAGESIZE
 *	and blocksize, which is the unit of dow creation for file "pages".
 *
 * Remarks:
 *	This routine is called from sfs_putpageio just prior to writing a
 *	block or page of a directory is written out.
 *
 *	See the explanation under sfs_unlink_order as to why this is needed.
 */
void
sfs_unlink_flushed(inode_t *dp, off_t offset)
{
	uint_t shift, bitno;
	pl_t opl;

	shift = MIN(dp->i_fs->fs_bshift, PAGESHIFT);
	bitno = (uint_t)offset >> shift;
	if (bitno >= NBITPW)
		bitno = NBITPW - 1;
	opl = SFS_ILOCK(dp);
	BITMASK1_CLR1(&dp->i_unlinkmask, bitno);
	SFS_IUNLOCK(dp, opl);
}

/*
 * void
 * sfs_unlink_order(inode_t *dp, dowid_t createdow, cred_t *cr)
 *	Specify that the page corresponding to "createdow" should
 *	be written after any pages of the specified directory from
 *	which unlinks (removes) have been done.
 *
 * Calling/Exit State:
 *	dp is held write-locked.
 *
 * Description:
 *	Each bit of dp->i_unlinkmask corresponds to a unit, of size
 *	min(blocksize, PAGESIZE), of the directory dp, in which an
 *	unlink has been done but which has not been written to disk
 *	since the unlink.  This routine sets up orderings so that
 *	each of these pages (or blocks) with pending unlinks will be
 *	written to disk before the page corresponding to createdow.
 *
 * Remarks:
 *	This is necessary to prevent a race introduced by delayed
 *	ordered writes.  This race can occur if a file is removed
 *	from a directory, and then another file with the same name
 *	is created.  If the entry for the file being created is in
 *	a different page or block of the file than the one being
 *	removed, then is is possible for the created file to be
 *	written to disk before the overwritten entry gets written
 *	to disk.  If the system were to crash between these two
 *	writes, then, on reboot, there would be two entries with
 *	the same name in the same directory.  By establishing
 *	orderings to ensure that each create is written to disk
 *	after any unlinks, this race is avoided.
 *
 *	Each of the bits, except the last, of the i_unlinkmask field
 *	correspond to units of the directory of size MIN(blocksize,
 *	PAGESIZE).  The last bit corresponds to "the rest of the directory."
 *	Directories are generally a few pages or blocks at most, so the
 *	"rest of the directory" bit is not expected to see much use.
 *
 *	The sfs_unlink_order routine creates a dow for each unit of
 *	the directory whose i_unlinkmask is set, and specifies an ordering
 *	of that dow with the createdow parameter.  If the last bit in
 *	i_unlinkmask happens to be set, the routine will not create a
 *	dow, since that bit corresponds to multiple pages; instead, the
 *	routine does a synchronous VOP_PUTPAGE to flush those pages out,
 *	which also guarantees the ordering.  This heavyhanded solution
 *	is used because it is simple, it allows us to keep the bitmask for
 *	the directory, and it is expected to be an extremely infrequent
 *	case, given that in practice directories are generally only a few
 *	blocks or pages.
 */
void
sfs_unlink_order(inode_t *dp, dowid_t createdow, cred_t *cr)
{
	uint_t shift, bitno, unlinkmask, size;
	off_t offset;
	dowid_t rmdow;
	pl_t opl;

	opl = SFS_ILOCK(dp);
	unlinkmask = dp->i_unlinkmask;
	SFS_IUNLOCK(dp, opl);
	if (dp->i_fs->fs_bsize < PAGESIZE) {
		shift = dp->i_fs->fs_bshift;
		size = dp->i_fs->fs_bsize;
	} else {
		shift = PAGESHIFT;
		size = PAGESIZE;
	}
	if (BITMASK1_TEST1(&unlinkmask, NBITPW - 1)) {
		VOP_PUTPAGE(ITOV(dp), ((NBITPW - 1) << shift) & PAGEMASK, 0,
			0, cr);
		opl = SFS_ILOCK(dp);
		BITMASK1_CLR1(&dp->i_unlinkmask, NBITPW - 1);
		unlinkmask = dp->i_unlinkmask;
		SFS_IUNLOCK(dp, opl);
	}
	while ((bitno = BITMASK1_FFSCLR(&unlinkmask)) != -1) {
		offset = bitno << shift;
		rmdow = sfs_dow_create_page(dp, offset, size);
		if (rmdow == DOW_NONE) {
			VOP_PUTPAGE(ITOV(dp), offset & PAGEMASK, PAGESIZE,
				0, cr);
			opl = SFS_ILOCK(dp);
			unlinkmask &= dp->i_unlinkmask;
			SFS_IUNLOCK(dp, opl);
		} else {
			if (rmdow != createdow)
				sfs_dow_order(createdow, rmdow);
			dow_rele(rmdow);
		}
	}
}
