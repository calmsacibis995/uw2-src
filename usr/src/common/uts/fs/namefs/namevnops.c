/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/namefs/namevnops.c	1.27"
#ident	"$Header: $"

/*
 * This file defines the vnode operations for mounted file descriptors.
 * The routines in this file act as a layer between the NAMEFS file 
 * system and SPECFS/FIFOFS.  With the exception of nm_open(), nm_setattr(),
 * nm_getattr(), nm_setacl(), nm_getacl() and nm_access(), the routines simply
 * apply the VOP operation to the vnode representing the file descriptor.  This
 * switches control to the underlying file system to which the file descriptor 
 * belongs.
 */

#include <util/types.h>
#include <acc/priv/privilege.h>
#include <util/param.h>
#include <util/plocal.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <fs/file.h>
#include <mem/kmem.h>
#include <io/uio.h>
#include <fs/buf.h>
#include <fs/fs_hier.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <util/debug.h>
#include <fs/namefs/namehier.h>
#include <fs/namefs/namenode.h>
#include <acc/dac/acl.h>
#include <io/poll.h>
#include <fs/fs_subr.h>
#include <acc/mac/mac.h>
/*
 * Define the routines within this file.
 */
STATIC int	nm_open(vnode_t **, int, cred_t *);
STATIC int	nm_close(vnode_t *, int, boolean_t, off_t, cred_t *);
STATIC int	nm_read(vnode_t *, uio_t *, int, cred_t *);
STATIC int	nm_write(vnode_t *, uio_t *, int, cred_t *);
STATIC int	nm_ioctl(vnode_t *, int, int, int, cred_t *, int *);
STATIC int	nm_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC int	nm_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC int	nm_access(vnode_t *, int, int, cred_t *);
STATIC int	nm_create(vnode_t *, char *, vattr_t *, enum vcexcl, 
			  int, vnode_t **, cred_t *);
STATIC int	nm_link(vnode_t *, vnode_t *, char *, cred_t *);
STATIC int	nm_fsync(vnode_t *, cred_t *);
STATIC void	nm_inactive(vnode_t *, cred_t *);
STATIC int	nm_fid(vnode_t *, struct fid **);
STATIC int	nm_rwlock(vnode_t *, off_t, int, int, int);
STATIC void	nm_rwunlock(vnode_t *, off_t, int);
STATIC int	nm_seek(vnode_t *, off_t, off_t *);
STATIC int	nm_realvp(vnode_t *, vnode_t **);
STATIC int	nm_poll(vnode_t *, int, int, short *, struct pollhead **);
STATIC int	nm_getacl(vnode_t *, long, long *, struct acl *,
			    cred_t *, int *);
STATIC int	nm_setacl(vnode_t *, long, long, struct acl *, cred_t *);
STATIC int	nm_msgio(vnode_t *, struct strbuf *, struct strbuf *, 
			int, unsigned char *, int, int *, rval_t *, cred_t *);
STATIC int	nm_frlock(vnode_t *, int, flock_t *, int, off_t, cred_t *);

int	nm_aclstore(struct namenode *, struct acl *, long);
STATIC mode_t	nm_daccess(struct namenode *, mode_t, cred_t *);

extern	lock_t namelist_mutex;
extern	lkinfo_t nmlock_lkinfo;

/*
 * Define external routines.
 */
extern	ino_t	nmgetid(void);
extern	void	nmclearid(ino_t);
extern	void	nameinsert(struct namenode *);
extern	int	nameremove(struct namenode *, int);
extern	struct	namenode *namefind(vnode_t *, vnode_t *);
extern	int	cleanlocks();

vnodeops_t nm_vnodeops = {
	nm_open,
	nm_close,
	nm_read,
	nm_write,
	nm_ioctl,
	fs_setfl,
	nm_getattr,
	nm_setattr,
	nm_access,
	(int (*)())fs_nosys,	/* lookup */
	nm_create,
	(int (*)())fs_nosys,	/* remove */
	nm_link,
	(int (*)())fs_nosys,	/* rename */
	(int (*)())fs_nosys,	/* mkdir */
	(int (*)())fs_nosys,	/* rmdir */
	(int (*)())fs_nosys,	/* readdir */
	(int (*)())fs_nosys,	/* symlink */
	(int (*)())fs_nosys,	/* readlink */
	nm_fsync,
	nm_inactive,
	(void (*)())fs_nosys,	/* release */
	nm_fid,
	nm_rwlock,
	nm_rwunlock,
	nm_seek,
	fs_cmp,
	nm_frlock,
	nm_realvp,
	(int (*)())fs_nosys,	/* getpage */
	(int (*)())fs_nosys,	/* putpage */
	(int (*)())fs_nosys,	/* map */
	(int (*)())fs_nosys,	/* addmap */
	(int (*)())fs_nosys,	/* delmap */
	nm_poll,
	fs_pathconf,
	nm_getacl,
	nm_setacl,
	(int (*)())fs_nosys,	/* setlevel */
	(int (*)())fs_nosys,	/* getdvstat */
	(int (*)())fs_nosys,	/* setdvstat */
	(int (*)())fs_nosys,	/* makemld */
	(int (*)())fs_nosys,	/* testmld */
	(int (*)())fs_nosys,	/* stablestore */
	(int (*)())fs_nosys,	/* relstore */
	(int (*)())fs_nosys,	/* getpagelist */
	(int (*)())fs_nosys,	/* putpagelist */
	nm_msgio,
	(int (*)())fs_nosys,	/* filler[4]... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};
 
/*
 * STATIC int nm_open(vnode_t **vpp, int flag, cred_t *crp)
 * 	Create a reference to the vnode representing the file descriptor.
 * 	Then, apply the VOP_OPEN operation to that vnode.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	The vnode for the file descriptor may be switched under you.
 * 	If it is, search the hash list for an nodep - nodep->nm_filevp
 * 	pair. If it exists, return that nodep to the user.
 * 	If it does not exist, create a new namenode to attach
 * 	to the nodep->nm_filevp then place the pair on the hash list.
 *
 * 	Newly created objects are like children/nodes in the mounted
 * 	file system, with the parent being the initial mount.
 */
STATIC int
nm_open(vnode_t **vpp, int flag, cred_t *crp)
{
	register struct namenode *nodep = VTONM(*vpp);
	register int error = 0;
	register struct namenode *newnamep, *namep;
	vnode_t *newvp;
	vnode_t *infilevp;
	vnode_t *outfilevp;
	pl_t pl;

	ASSERT(nodep->nm_filevp);
	/*
	 * If the vnode is switched under us, the corresponding
	 * VN_RELE for this VN_HOLD will be done by the file system
	 * performing the switch. Otherwise, the corresponding
	 * VN_RELE will be done by nm_close().
	 */
	VN_HOLD(nodep->nm_filevp);
	infilevp = outfilevp = nodep->nm_filevp;

	if ((error = VOP_OPEN(&outfilevp, flag | FNMFS, crp)) != 0) {
		VN_RELE(nodep->nm_filevp);
		return (error);
	}
	if (infilevp != outfilevp) {
		/*
		 * See if the new filevp (outfilevp) is already associated
		 * with the mount point. If it is, then it already has a
		 * namenode associated with it.
		 */
		pl = LOCK(&namelist_mutex, PLNM);
		if ((newnamep = namefind(outfilevp, nodep->nm_mountpt)) != NULL)
			goto gotit;

		UNLOCK(&namelist_mutex, pl);
		/*
		 * Create a new namenode. 
		 */
		newnamep =
			(struct namenode *)kmem_zalloc (sizeof(struct namenode),
				KM_SLEEP);

		/*
		 * Initialize the fields of the new vnode/namenode
		 * then overwrite the fields that should not be copied.
		 */
		*newnamep = *nodep;

		newvp = NMTOV(newnamep);
		VN_INIT(newvp, NULL, outfilevp->v_type, outfilevp->v_rdev, 0,
								KM_SLEEP);
		newvp->v_flag &= ~VROOT;  	/* new objects are not roots */
		newvp->v_flag |= VNOMAP|VNOSWAP;
		newvp->v_count = 0;        	/* bumped down below */
		newvp->v_vfsmountedhere = NULL;
		newvp->v_stream = outfilevp->v_stream;
		newvp->v_pages = NULL;
		newvp->v_lid = outfilevp->v_lid;
		/* set VMAC_DOPEN if it is set in outfilevp */
		if(outfilevp->v_macflag & VMAC_DOPEN)
			newvp->v_macflag |= VMAC_DOPEN;
		newvp->v_macflag |= VMAC_SUPPORT;
		newvp->v_data = (caddr_t) newnamep;
		newvp->v_filocks = NULL;
		newnamep->nm_vattr.va_type = outfilevp->v_type;
		newnamep->nm_vattr.va_nodeid = nmgetid();
		newnamep->nm_vattr.va_size = 0;
		newnamep->nm_vattr.va_rdev = outfilevp->v_rdev;
		newnamep->nm_flag = 0;
		newnamep->nm_filevp = outfilevp;
		/*
		 * Hold the new vnode. 
		 * The VN_RELE for this VN_HOLD is done in nm_close.
		 */
		VN_HOLD(outfilevp);
		newnamep->nm_mountpt = nodep->nm_mountpt;
		newnamep->nm_backp = newnamep->nm_nextp = NULL;
		RWSLEEP_INIT(&newnamep->nm_lock, NM_HIER,
			&nmlock_lkinfo, KM_SLEEP);

		(void)LOCK(&namelist_mutex, PLNM);
		if ((namep = namefind(outfilevp, nodep->nm_mountpt)) != NULL) {
			VN_RELE(outfilevp);
			RWSLEEP_DEINIT(&newnamep->nm_lock);
			VN_DEINIT(newvp);
			kmem_free((caddr_t)newnamep, sizeof(struct namenode));
			newnamep = namep;
			goto gotit;
		}
		/*
		 * Insert the new namenode into the hash list.
		 */
		nameinsert(newnamep);
gotit:		
		/*
		 * Release the above reference to the infilevp, the reference 
		 * to the NAMEFS vnode, create a reference to the new vnode
		 * and return the new vnode to the user.
		 */
		RWSLEEP_WRLOCK_RELLOCK(&newnamep->nm_lock, PRIMED, 
			&namelist_mutex);
		VN_HOLD(NMTOV(newnamep));
		RWSLEEP_UNLOCK(&newnamep->nm_lock);
		VN_RELE(*vpp);
		*vpp = NMTOV(newnamep);
	}
	return (0);
}

/*
 * STATIC int nm_close(vnode_t *vp, int flag, boolean_t lastclose, 
 *		       off_t offset, cred_t *crp)
 * 	Close a mounted file descriptor.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Remove any locks and apply the VOP_CLOSE operation to the vnode for
 * 	the file descriptor.
 */
STATIC int
nm_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *crp)
{
	register struct namenode *nodep = VTONM(vp);
	register int error = 0;

	ASSERT (nodep->nm_filevp);

	error = VOP_CLOSE(nodep->nm_filevp, flag, lastclose, offset, crp);
	if (lastclose) {
		(void) nm_fsync(vp, crp);
		VN_RELE(nodep->nm_filevp);
	}
	return (error);
}

/*
 * STATIC int nm_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *crp)
 *
 * Calling/Exit State:
 * 	Read from a mounted file descriptor.
 */
STATIC int
nm_read(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *crp)
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_READ (VTONM(vp)->nm_filevp, uiop, ioflag, crp));
}

/*
 * STATIC int nm_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *crp)
 *
 * Calling/Exit State:
 * 	Apply the VOP_WRITE operation on the file descriptor's vnode.
 */
STATIC int
nm_write(vnode_t *vp, struct uio *uiop, int ioflag, cred_t *crp)
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_WRITE(VTONM(vp)->nm_filevp, uiop, ioflag, crp));
}

/*
 * STATIC int nm_ioctl(vnode_t *vp, int cmd, int arg, int mode, 
 *		       cred_t *cr, int *rvalp)
 *
 * Calling/Exit State:
 * 	Apply the VOP_IOCTL operation on the file descriptor's vnode.
 */
STATIC int
nm_ioctl(vnode_t *vp, int cmd, int arg, int mode, cred_t *cr, int *rvalp)
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_IOCTL(VTONM(vp)->nm_filevp, cmd, arg, mode, cr, rvalp));
}

/*
 * STATIC int nm_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *crp)
 *
 * Calling/Exit State:
 * 	Return in vap the attributes that are stored in the namenode
 * 	structure. In addition, overwrite the va_mask field with 0;
 */
/* ARGSUSED */
STATIC int
nm_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *crp)
{
	register struct namenode *nodep = VTONM(vp);
	vattr_t va;
	register int error;
	long mask = nodep->nm_vattr.va_mask;

        /*
         * Must have MAC read access to the real vnode since some
         * of its information is returned as well.
         * Of course, this check need only be done if the namenode's
         * level does not match the real vnode's level.
         * Since the namenode's level can never change, the real
         * vnode's level must have changed.  This can only happen
         * for file system types which allow level changes while
         * the file is open (e.g. device special files).
         */
        if (vp->v_lid != nodep->nm_filevp->v_lid
        &&  (error = MAC_VACCESS(nodep->nm_filevp, VREAD, crp)))
                return(error);

	va.va_mask = AT_SIZE;
	if (error = VOP_GETATTR(nodep->nm_filevp, &va, flags, crp))
		return (error);

	RWSLEEP_RDLOCK(&nodep->nm_lock, PRIMED);
	/*
	 * Copy the whole struct, even stuff that wasn't asked for in va_flags.
	 * It's easier than doing a field by field copy.  Note however that
	 * the va_aclcnt field is incorrect, and is only corrected if asked
	 * for (see comment below).
	 */
	*vap = nodep->nm_vattr;
	RWSLEEP_UNLOCK(&nodep->nm_lock);
	vap->va_mask = 0;
	vap->va_size = va.va_size;
	if (mask & AT_ACLCNT) {
		/*
		 * Only the *extra* ACL entries are counted in va_aclcnt.  Need
		 * to adjust for mode entries before returning to user.
		 */
		vap->va_aclcnt = vap->va_aclcnt ? vap->va_aclcnt + NACLBASE - 1 : NACLBASE;
	}
	return (0);
}

/*
 * STATIC int nm_setattr(vnode_t *vp, register vattr_t *vap, 
 *			 int flags, int ioflags, cred_t *crp)
 *
 * Calling/Exit State:
 * 	Set the attributes of the namenode from the attributes in vap.
 */
/* ARGSUSED */
STATIC int
nm_setattr(vnode_t *vp, register vattr_t *vap, int flags,
	int ioflags, cred_t *crp)
{
	register struct namenode *nodep = VTONM(vp);
	register struct vattr *nmvap = &nodep->nm_vattr;
	register long mask = vap->va_mask;
	int error = 0;

	/*
	 * Cannot set these attributes.
	 */
	if (mask & (AT_NOSET|AT_SIZE))
		return (EINVAL);

	RWSLEEP_WRLOCK(&nodep->nm_lock, PRIMED);

	/*
	 * Change ownership/group/time/access mode of mounted file
	 * descriptor.  Must be owner or privileged.
	 */
	if (crp->cr_uid != nmvap->va_uid && pm_denied(crp, P_OWNER)) {
		error = EPERM;
		goto out;
	}
	/*
	 * If request to change mode, copy new
	 * mode into existing attribute structure.
	 */
	if (mask & AT_MODE) {
		nmvap->va_mode = vap->va_mode & ~VSVTX;
		if (!groupmember(nmvap->va_gid, crp) && pm_denied(crp, P_OWNER))
			nmvap->va_mode &= ~VSGID;
	}
	/*
	 * If request was to change user or group, turn off suid and sgid
	 * bits.
	 * If the system was configured with the "rstchown" option, the 
	 * owner is not permitted to give away the file, and can change 
	 * the group id only to a group of which he or she is a member.
	 */
	if (mask & (AT_UID|AT_GID)) {
		int checksu = 0;

		if (rstchown) {
			if (((mask & AT_UID) && vap->va_uid != nmvap->va_uid)
			  || ((mask & AT_GID)
			    && !groupmember(vap->va_gid, crp)))
				checksu = 1;
		} else if (crp->cr_uid != nmvap->va_uid)
			checksu = 1;

		if (checksu && pm_denied(crp, P_OWNER)) {
			error = EPERM;
			goto out;
		}
		if ((nmvap->va_mode & (VSUID|VSGID)) && pm_denied(crp, P_OWNER))
			nmvap->va_mode &= ~(VSUID|VSGID);
		if (mask & AT_UID)
			nmvap->va_uid = vap->va_uid;
		if (mask & AT_GID)
			nmvap->va_gid = vap->va_gid;
	}
	/*
	 * If request is to modify times, make sure user has write 
	 * permissions on the file.
	 */
	if (mask & (AT_ATIME|AT_MTIME)) {
		if (!(nmvap->va_mode & VWRITE) && pm_denied(crp, P_DACWRITE)) {
			error = EACCES;
			goto out;
		}
		if (mask & AT_ATIME)
			nmvap->va_atime = vap->va_atime;
		if (mask & AT_MTIME) {
			nmvap->va_mtime = vap->va_mtime;
			GET_HRESTIME(&nmvap->va_ctime);
		}
	}
out:
	RWSLEEP_UNLOCK(&nodep->nm_lock);
	return (error);
}

/*
 * STATIC int nm_access(vnode_t *vp, int mode, int flags, cred_t *crp)
 *
 * Calling/Exit State:
 * 	Check mode permission on the namenode.  If the namenode bits deny the
 * 	privilege is checked.  In addition an access check is performed
 * 	on the mounted file. Finally, if the file was opened without the
 * 	requested access at mount time, deny the access.
 */
/* ARGSUSED */
STATIC int
nm_access(vnode_t *vp, int mode, int flags, cred_t *crp)
{
	register struct namenode *nodep = VTONM(vp);
	register mode_t denied_mode;
	int error, fmode;

	RWSLEEP_WRLOCK(&nodep->nm_lock, PRIMED);
	if ((denied_mode = nm_daccess(nodep, mode, crp)) != 0) {
                if ((denied_mode & (VREAD | VEXEC)) 
				&& pm_denied(crp, P_DACREAD)) {
			RWSLEEP_UNLOCK(&nodep->nm_lock);
                        return (EACCES);
		}
                if ((denied_mode & VWRITE) && pm_denied(crp, P_DACWRITE)) {
			RWSLEEP_UNLOCK(&nodep->nm_lock);
                        return (EACCES);
		}
        }
	RWSLEEP_UNLOCK(&nodep->nm_lock);

	if (error = VOP_ACCESS(nodep->nm_filevp, mode, flags, crp))
		return (error);
	/*
	 * Last stand.  Regardless of the requestor's credentials, don't
	 * grant a permission that wasn't granted at the time the mounted
	 * file was originally opened.
	 */
	fmode = nodep->nm_flag;
	if (((mode & VWRITE) && (fmode & NMWRITE) == 0)
	  || ((mode & VREAD) && (fmode & NMREAD) == 0))
		return (EACCES);
	return (0);
}

/*
 * STATIC mode_t nm_daccess(struct namenode *nodep, mode_t mode, cred_t *crp)
 *
 * Calling/Exit State:
 *	nm_lock is locked in share mode on entry and remains locked on exit.
 *
 * Description:
 * 	Do discretionary access check on the name node.  This includes
 * 	checking the mode bits, and any ACL entries.
 * 	Any denied mode returned must be shifted to the leftmost octal
 * 	permissions digit, so that nm_access (above) will be able to do
 * 	the correct privilege check based on VREAD, VEXEC, or VWRITE.
 */
/* ARGSUSED */
STATIC mode_t
nm_daccess(struct namenode *nodep, mode_t mode, cred_t *crp)
{
        struct acl *aclp;
        long    idmatch = 0;
        mode_t  workmode = 0;
        mode_t  reqmode;
        mode_t  basemode;
	int	i;

        ASSERT(nodep->nm_vattr.va_aclcnt >= 0);

        /*
         *      check if effective uid == owner of name node
         */
        if (crp->cr_uid == nodep->nm_vattr.va_uid) {
                if ((workmode = (nodep->nm_vattr.va_mode & mode)) == mode)
                        return (0);
                else
                        return (mode & ~workmode);
        }
        mode >>= 3;
        /*
         *      If there's no ACL, check only the group &
         *      other permissions
         */
        if (nodep->nm_vattr.va_aclcnt == 0) {
                if (groupmember(nodep->nm_vattr.va_gid, crp)) {
                        if ((workmode = (nodep->nm_vattr.va_mode & mode)) 
				== mode)
                                return (0);
                        else
                                return ((mode & ~workmode) << 3);
                } else
                        goto other_ret;
        }

        /*      set up requested & base permissions     */
        reqmode = (mode >>= 3) & 07;
        basemode = (nodep->nm_vattr.va_mode >> 3) & 07;
        for (i = nodep->nm_vattr.va_aclcnt, aclp = nodep->nm_aclp; i > 0; i--, aclp++) {
                switch (aclp->a_type) {
                case USER:
                        if (crp->cr_uid == aclp->a_id) {
                                if ((workmode = ((aclp->a_perm & reqmode) & 
					basemode)) == reqmode)
                                        /*
                                         * matching USER found,
                                         *  access granted
                                         */
                                        return (0);
                                else
                                        /*
                                         * matching USER found,
                                         * access denied
                                         */
                                        return ((reqmode & ~workmode) << 6);
                        }
                        break;

                case GROUP_OBJ:
                        if (groupmember(nodep->nm_vattr.va_gid, crp)) {
                                if ((workmode |= (aclp->a_perm & reqmode)) ==
                                        reqmode)
                                        goto match;
                                else
                                        idmatch++;
                        }
                        break;

                case GROUP:
                        if (groupmember(aclp->a_id, crp)) {
                                if ((workmode |= (aclp->a_perm & reqmode)) ==
                                        reqmode)
                                        goto match;
                                else
                                        idmatch++;
                        }
                        break;
                }       /* end switch statement */
        }       /* end for statement */
        if (idmatch)
                /*
                 *      Matching GROUP or GROUP_OBJ entries
                 *      were found, but did not grant the access
                 */
                return ((reqmode & ~workmode) << 6);

other_ret:
        /*
         *      Not the file owner, and either
         *      no ACL, or no match in ACL.
         *      Now, check the file other permissions.
         */
        mode >>= 3;
        if ((workmode = (nodep->nm_vattr.va_mode & mode)) == mode) 
		return (0);
        else
                return ((mode & ~workmode) << 6);

match:
        /*
         *      Access granted by ACL entry or entries
         *      File Group Class bits mask access, so
         *      determine whether matched entries
         *      should really grant access.
         */
        if ((workmode & basemode) == reqmode)
                return (0);
        else
                return ((reqmode & ~(workmode & basemode)) << 6);
}

/*
 * STATIC int nm_create(vnode_t *dvp, char *name, vattr_t *vap, 
 *		enum vcexcl excl, int mode, vnode_t **vpp, cred_t *cr)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Dummy op so that creats and opens with O_CREAT
 * 	of mounted streams will work.
 */
/*ARGSUSED*/
STATIC int
nm_create(vnode_t *dvp, char *name, vattr_t *vap, enum vcexcl excl,
	  int mode, vnode_t **vpp, cred_t *cr)
{
	register int error = 0;

	if (*name == '\0') {
		/*
		 * Null component name refers to the root.
		 */
		if ((error = nm_access(dvp, mode, 0, cr)) == 0) {
			VN_HOLD(dvp);
			*vpp = dvp;
		}
	} else {
		error = ENOSYS;
	}
	return (error);
}

/*
 * STATIC int nm_link(register vnode_t *tdvp; vnode_t *vp; 
 *		      char *tnm; cred_t *crp)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Links are not allowed on mounted file descriptors.
 */
/*ARGSUSED*/
STATIC int
nm_link(register vnode_t *tdvp, vnode_t *vp, char *tnm, cred_t *crp)
{
	return (EXDEV);
}

/*
 * STATIC int nm_fsync(vnode_t *vp, cred_t *crp)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Apply the VOP_FSYNC operation on the file descriptor's vnode.
 */
STATIC int
nm_fsync(vnode_t *vp, cred_t *crp)
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_FSYNC (VTONM(vp)->nm_filevp, crp));
}

/*
 * STATIC void nm_inactive(vnode_t *vp, cred_t *crp)
 *
 * Calling/Exit State:
 *
 * Description:
 *
 * 	Inactivate a vnode/namenode by...
 * 	clearing its unique node id, removing it from the hash list
 * 	and freeing the memory allocated for it.
 * 	vp->v_mutex is locked on entry.
 */
/* ARGSUSED */
STATIC void
nm_inactive(vnode_t *vp, cred_t *crp)
{
	register struct namenode *nodep = VTONM(vp);
	register pl_t	pl;

	/*
	 * Note:  Maintain this ordering since VN_RELE may sleep.
	 */
	RWSLEEP_WRLOCK(&nodep->nm_lock, PRIMED);
	if (vp->v_count != 1) {
		VN_LOCK(vp);
		if (vp->v_count != 1) {
			vp->v_count--;
			VN_UNLOCK(vp);
			RWSLEEP_UNLOCK(&nodep->nm_lock);
			return;
		}
		VN_UNLOCK(vp);
	}
	ASSERT(vp->v_count == 1);
	vp->v_count = 0;

	pl = LOCK(&namelist_mutex, PLNM);
	if (((nodep->nm_flag & NMREMOVED) == 0) &&
	    (nameremove(nodep, 1) != 0)) {
		UNLOCK(&namelist_mutex, pl);
		RWSLEEP_UNLOCK(&nodep->nm_lock);
		return;
	}
	UNLOCK(&namelist_mutex, pl);
	RWSLEEP_UNLOCK(&nodep->nm_lock);
	nmclearid(nodep->nm_vattr.va_nodeid);
	VN_RELE(nodep->nm_filevp);

	VN_DEINIT(vp);
	RWSLEEP_DEINIT(&nodep->nm_lock);
	kmem_free((caddr_t) nodep, sizeof(struct namenode));
}

/*
 * STATIC int nm_fid(vnode_t *vp, struct fid **fidnodep)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Apply the VOP_FID operation on the file descriptor's vnode.
 */
STATIC int
nm_fid(vnode_t *vp, struct fid **fidnodep)
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_FID(VTONM(vp)->nm_filevp, fidnodep));
}

/*
 * STATIC int nm_rwlock(vnode_t *vp)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Lock the namenode associated with vp.
 */
/*ARGSUSED*/
STATIC int
nm_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	register struct namenode *nodep = VTONM(vp);
	register int	error;

	if (mode == LOCK_EXCL) {
		error = VOP_RWWRLOCK(nodep->nm_filevp, off, len, fmode);
	} else {
		error = VOP_RWRDLOCK(nodep->nm_filevp, off, len, fmode);
	}

	return (error);
}

/*
 * STATIC void nm_rwunlock(vnode_t *vp)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Unlock the namenode associated with vp.
 */
/*ARGSUSED*/
STATIC void
nm_rwunlock(vnode_t *vp, off_t off, int len)
{
	register struct namenode *nodep = VTONM(vp);

	VOP_RWUNLOCK(nodep->nm_filevp, off, len);
}

/*
 * STATIC int nm_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag,
 *			off_t offset, cred_t *cr)
 *
 * Calling/Exit State:
 *
 * Description:
 * 	Lock the namenode associated with vp.
 */
/*ARGSUSED*/
STATIC int
nm_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag, off_t offset,
	  cred_t *cr)
{
	return (VOP_FRLOCK(VTONM(vp)->nm_filevp, cmd, bfp, flag, offset, cr));
}

/*
 * STATIC int nm_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *
 * Calling/Exit State:
 * 	Apply the VOP_SEEK operation on the file descriptor's vnode.
 */
STATIC int
nm_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_SEEK(VTONM(vp)->nm_filevp, ooff, noffp));
}

/*
 * STATIC int nm_realvp(register vnode_t *vp, register vnode_t **vpp)
 *
 * Calling/Exit State:
 * 	Return the vnode representing the file descriptor in vpp.
 */
STATIC int
nm_realvp(register vnode_t *vp, register vnode_t **vpp)
{
	register struct vnode *fvp = VTONM(vp)->nm_filevp;
	struct vnode *rvp;

	ASSERT(fvp);
	vp = fvp;
	if (VOP_REALVP(vp, &rvp) == 0)
		vp = rvp;
	*vpp = vp;
	return (0);
}

/*
 * STATIC int nm_poll(vnode_t *vp, register int events, int anyyet,
 * 		      register short *reventsp, struct pollhead **phpp)
 *
 * Calling/Exit State:
 * 	Apply VOP_POLL to the vnode representing the mounted file descriptor.
 */
STATIC int
nm_poll(vnode_t *vp, register int events, int anyyet,
	register short *reventsp, struct pollhead **phpp)
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_POLL(VTONM(vp)->nm_filevp, events, anyyet, reventsp, phpp));
}

/*
 * STATIC int nm_getacl(register vnode_t *vp, register long nentries,
 * 		 register long *dentriesp, register struct acl *aclbufp, 
 *		 cred_t *cr, int *rvalp)
 *
 * Description:
 * 	VOP_GETACL on the namenode
 * 	Note that default ACLs are not supported for NAMEFS.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
/*ARGSUSED*/
STATIC int
nm_getacl(register vnode_t *vp, register long nentries,
          register long *dentriesp, register struct acl *aclbufp, 
	  cred_t *cr, int *rvalp)
{
        register struct namenode *nodep = VTONM(vp);
        struct acl base_user = {USER_OBJ, (uid_t) 0, (ushort) 0};
        struct acl base_group = {GROUP_OBJ, (uid_t) 0, (ushort) 0};
        struct acl base_class = {CLASS_OBJ, (uid_t) 0, (ushort) 0};
        struct acl base_other = {OTHER_OBJ, (uid_t) 0, (ushort) 0};
        struct acl *tgt_aclp;
	int rval;

	RWSLEEP_RDLOCK(&nodep->nm_lock, PRIMED);

	rval = nodep->nm_vattr.va_aclcnt ? nodep->nm_vattr.va_aclcnt + NACLBASE - 1 : NACLBASE;
	if (rval > nentries) {
		RWSLEEP_UNLOCK(&nodep->nm_lock);
		return (ENOSPC);
	}

        /*
         * get USER_OBJ, CLASS_OBJ, & OTHER_OBJ entry permissions from file
         * owner class, file group class, and file other permission bits
         */
        base_user.a_perm = (nodep->nm_vattr.va_mode >> 6) & 07;
        base_class.a_perm = (nodep->nm_vattr.va_mode >> 3) & 07;
        base_other.a_perm = nodep->nm_vattr.va_mode & 07;

	tgt_aclp = aclbufp;

        /* copy USER_OBJ entry into caller's buffer */
        bcopy((caddr_t)&base_user, (caddr_t)tgt_aclp, sizeof(struct acl));
        tgt_aclp++;

        if (nodep->nm_vattr.va_aclcnt == 0) {
        	/* copy GROUP_OBJ entry into caller's buffer */
                base_group.a_perm = base_class.a_perm;
                bcopy((caddr_t)&base_group, (caddr_t)tgt_aclp,
                        sizeof(struct acl));
                tgt_aclp++;
	} else {
        	/* copy USER, GROUP_OBJ, & GROUP entries into caller's buffer */
        	bcopy((caddr_t)nodep->nm_aclp, (caddr_t)tgt_aclp,
                        nodep->nm_vattr.va_aclcnt * sizeof(struct acl));
		tgt_aclp += nodep->nm_vattr.va_aclcnt;
	}

        /* copy CLASS_OBJ & OTHER_OBJ entries  into caller's buffer */
        bcopy((caddr_t)&base_class, (caddr_t)tgt_aclp, sizeof(struct acl));
        tgt_aclp++;
        bcopy((caddr_t)&base_other, (caddr_t)tgt_aclp, sizeof(struct acl));
	*rvalp = rval;
	*dentriesp = 0;	/* no defaults */
	RWSLEEP_UNLOCK(&nodep->nm_lock);
	return (0);
}


/*
 * STATIC int nm_setacl(register vnode_t *vp, register long nentries,
 * 	register long dentries, register struct acl *aclbufp, cred_t *cr)
 *
 * Calling/Exit State:
 *
 * Description:
 *      nm_setacl - Set a mounted file descriptor's ACL
 *
 *      Input:   Pointer to the file's namenode
 *               Pointer to user's ACL entries
 *               Number of ACL entries to save
 *               Pointer to number of default ACL entries
 *
 *	Note that NAMEFS does not support the mounting of a
 *	directory fd; therefore NAMEFS does not support default
 *	ACLs.
 */
/* ARGSUSED */
STATIC int
nm_setacl(register vnode_t *vp, register long nentries,
          register long dentries, register struct acl *aclbufp, cred_t *cr)
{
	int error;
        register struct namenode *nodep = VTONM(vp);

	RWSLEEP_WRLOCK(&nodep->nm_lock, PRIMED);

        if (cr->cr_uid != nodep->nm_vattr.va_uid && pm_denied(cr, P_OWNER)) {
		RWSLEEP_UNLOCK(&nodep->nm_lock);
                return (EPERM);
	}

	ASSERT(dentries == 0);

        /* go store the entries on the file */
	error = nm_aclstore(nodep, aclbufp, nentries);
	RWSLEEP_UNLOCK(&nodep->nm_lock);
	return(error);
}
/*
 * int nm_aclstore(register struct namenode *nodep, 
 * 		   register struct acl *aclbufp, register long nentries)
 *
 * Calling/Exit State:
 *
 * Description:
 *
 */

int
nm_aclstore(register struct namenode *nodep, 
	    register struct acl *aclbufp, register long nentries)
{
        register struct acl     *src_aclp;
        struct acl              *tmpaclp = NULL;
	long			entries = 0;
        mode_t                  mode;

	ASSERT(nentries >= NACLBASE);

        mode = (aclbufp->a_perm & 07) << 6;     /* save owner perms */
        src_aclp = aclbufp + nentries - 1;      /* point at OTHER_OBJ */
        mode |= src_aclp->a_perm & 07;          /* save other perms */
        src_aclp--;                             /* point at CLASS_OBJ */
        mode |= (src_aclp->a_perm & 07) << 3;   /* save file group class perm */


	if (nentries > NACLBASE) {
		entries = nentries - (NACLBASE - 1);
        	tmpaclp = (struct acl *)
			kmem_alloc(entries * sizeof(struct acl), KM_NOSLEEP);
		if (tmpaclp == (struct acl *)NULL)
			return (ENOSPC);
                src_aclp = aclbufp + 1; /* point past USER_OBJ */
        	bcopy((caddr_t)src_aclp, (caddr_t)tmpaclp,
                                        entries * sizeof(struct acl));
	}

	if (nodep->nm_vattr.va_aclcnt)
		kmem_free((caddr_t)nodep->nm_aclp,
			nodep->nm_vattr.va_aclcnt * sizeof(struct acl));

	nodep->nm_vattr.va_aclcnt = entries;
	nodep->nm_aclp = tmpaclp;
        nodep->nm_vattr.va_mode &= ~(ushort)PERMMASK;
        nodep->nm_vattr.va_mode |= mode;
        return (0);
}

/*
 * int
 * nm_msgio(vnode_t *vp, struct strbuf *mctl, struct strbuf *mdata,
 *		int mode, unsigned char *prip, int fmode, int *flagsp,
 *		rval_t *rvp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	This routine will call strputmsg/strgetmsg.
 */
int
nm_msgio(vnode_t *vp, struct strbuf *mctl, struct strbuf *mdata,
		int mode, unsigned char *prip, int fmode, int *flagsp,
		rval_t *rvp, cred_t *cr)
{
	ASSERT (VTONM(vp)->nm_filevp);
	return (VOP_MSGIO(VTONM(vp)->nm_filevp, mctl, mdata, mode, 
		prip, fmode, flagsp, rvp, cr));
}
