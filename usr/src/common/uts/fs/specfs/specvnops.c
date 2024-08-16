/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/specfs/specvnops.c	1.110"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <fs/buf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/flock.h>
#include <fs/fs_subr.h>
#include <fs/specfs/snode.h>
#include <fs/specfs/spechier.h>
#include <fs/specfs/specdata.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/poll.h>
#include <io/strsubr.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_dev.h>
#include <mem/seg_kvn.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/swap.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <proc/user.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/mod/ksym.h>
#include <util/mod/mod_hier.h>
#include <util/mod/mod_k.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

/*
 * Added to support enhanced security
 */
#include <acc/dac/acl.h>		/* for acl support */
#include <acc/mac/mac.h>		/* for MAC access checks */
#include <acc/priv/privilege.h>		/* for privilege checks */
#include <fs/specfs/devmac.h>		/* for device security structure */
#include <io/termios.h>			/* added to analyze ioctl cmds */
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

/*
 * Some platforms require special handling of errors for driver compatibility.
 */
#ifndef CHECK_COMPAT_ERRNO
#define CHECK_COMPAT_ERRNO()	/**/
#endif

STATIC	int spec_open(vnode_t **, int, cred_t *);
STATIC	int spec_close(vnode_t *, int, boolean_t , off_t, cred_t *);
STATIC	int device_close(vnode_t *, int, cred_t *);
STATIC	int spec_read(vnode_t *, struct uio *, int, cred_t *);
STATIC	int spec_write(vnode_t *, struct uio *, int, cred_t *);
STATIC	int spec_ioctl(vnode_t *, int, int, int, cred_t *, int *);
STATIC	int spec_getattr(vnode_t *, vattr_t *, int, cred_t *);
STATIC	int spec_setattr(vnode_t *, vattr_t *, int, int, cred_t *);
STATIC	int spec_access(vnode_t *, int, int, cred_t *);
STATIC  int spec_fsync(vnode_t *, cred_t *);
STATIC  void spec_inactive(vnode_t *, cred_t *);
STATIC	void spec_release(vnode_t *);
STATIC	int spec_fid(vnode_t *, struct fid **);
STATIC	int spec_rwlock(vnode_t *, off_t, int, int, int);
STATIC	void spec_rwunlock(vnode_t *, off_t, int);
STATIC	int spec_seek(vnode_t *, off_t, off_t *);
STATIC  int spec_poll(vnode_t *, int, int, short *, struct pollhead **);
STATIC	int spec_frlock(vnode_t *, int, struct flock *, int, off_t, cred_t *);
STATIC	int spec_realvp(vnode_t *, vnode_t **);
STATIC	int spec_getpage(vnode_t *, uint_t, uint_t, uint_t *, page_t **,
		uint_t, struct seg *, vaddr_t, enum seg_rw, cred_t *);
STATIC int spec_putpage(vnode_t *, off_t, uint_t, int, cred_t *);
STATIC	int spec_map(vnode_t *, off_t, struct as *, vaddr_t *, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int spec_addmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int spec_delmap(vnode_t *, uint_t, struct as *, vaddr_t, uint_t,
		uint_t, uint_t, uint_t, cred_t *);
STATIC	int spec_stablestore(vnode_t **, off_t *, size_t *, void **, cred_t *);
STATIC	int spec_relstore(vnode_t *, off_t, size_t, void *, cred_t *);
STATIC	int spec_getpagelist(vnode_t *, off_t, uint_t, page_t *,
		  void *, int, cred_t *);
STATIC	int spec_putpagelist(vnode_t *, off_t, page_t *, void *, int, cred_t *);
STATIC	int spec_getacl(vnode_t *, long, long *, struct acl *, cred_t *, int *);
STATIC	int spec_setacl(vnode_t *, long, long, struct acl *, cred_t *);
STATIC	int spec_setlevel(vnode_t *, lid_t, cred_t *);
STATIC	int spec_msgio(vnode_t *, struct strbuf *, struct strbuf *, int,
			unsigned char *, int, int *, rval_t *, cred_t *);
STATIC  int spec_getdvstat(vnode_t *, struct devstat *, cred_t *);
STATIC	int spec_setdvstat(vnode_t *, struct devstat *, cred_t *);

extern int strgetmsg(vnode_t *, struct strbuf *, struct strbuf *,
			unsigned char *, int *, int, int, rval_t *);
extern int strputmsg(vnode_t *, struct strbuf *, struct strbuf *,
			unsigned char, int, int, int, cred_t *);
extern int stropen(vnode_t *, dev_t *, vnode_t **, int, cred_t *);
extern int strclose(vnode_t *, int, cred_t *);
extern int strclean(vnode_t *, boolean_t);
extern int strread(vnode_t *, struct uio *, cred_t *);
extern int strwrite(vnode_t *, struct uio *, cred_t *);
extern int strioctl(vnode_t *, int, int, int, int, cred_t *, int *);
extern int strpoll(stdata_t *, short, int, short *, struct pollhead **);
extern void map_addr(vaddr_t *, uint_t, off_t, int);
extern void devsec_dcicreat(snode_t *);
extern void devsec_dcifree(snode_t *);
extern void devsec_close(snode_t *);

extern int mod_bdev_open(dev_t *, int, int, struct cred *);
extern int mod_cdev_open(dev_t *, int, int, struct cred *);
extern int mod_sdev_open(queue_t *, dev_t *, int, int, cred_t *);

extern rwlock_t mod_bdevsw_lock, mod_cdevsw_lock;

vnodeops_t spec_vnodeops = {
	spec_open,
	spec_close,
	spec_read,
	spec_write,
	spec_ioctl,
	(int (*)())fs_setfl,
	spec_getattr,
	spec_setattr,
	spec_access,
	(int (*)())fs_nosys,	/* lookup */
	(int (*)())fs_nosys,	/* create */
	(int (*)())fs_nosys,	/* remove */
	(int (*)())fs_nosys,	/* link */
	(int (*)())fs_nosys,	/* rename */
	(int (*)())fs_nosys,	/* mkdir */
	(int (*)())fs_nosys,	/* rmdir */
	(int (*)())fs_nosys,	/* readdir */
	(int (*)())fs_nosys,	/* symlink */
	(int (*)())fs_nosys,	/* readlink */
	spec_fsync,
	spec_inactive,
	spec_release,
	spec_fid,
	spec_rwlock,
	spec_rwunlock,
	spec_seek,
	fs_cmp,
	spec_frlock,
	spec_realvp,
	spec_getpage,	/* getpage */
	spec_putpage,	/* putpage */
	spec_map,	/* map */
	spec_addmap,	/* addmap */
	spec_delmap,	/* delmap */
	spec_poll,
	fs_pathconf,
	spec_getacl,	
	spec_setacl,
	spec_setlevel,
	spec_getdvstat,		/* spec_getdvstat, */
	spec_setdvstat,		/* spec_setdvstat, */
	(int (*)())fs_nosys,	/* makemld */
	(int (*)())fs_nosys,	/* testmld */
	spec_stablestore,	/* stablestore */
	spec_relstore,		/* relstore */
	spec_getpagelist,	/* getpagelist */
	spec_putpagelist,	/* putpagelist */
	spec_msgio,		/* msgio */
	(int (*)())fs_nosys,	/* filler[4] */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

/*
 * int
 * spec_open(vnode_t **vpp, int flag, cred_t *cr)
 *
 * Calling/Exit State:
 *	Vnode v_lock is locked shared on entry and remains locked at exit.
 *
 *	This routine may block, so no spinlocks may be held on entry,
 *	and none will be held at exit.
 *
 * Description:
 *	We update the snode's open count while holding the common snode
 *	s_rwlock shared to synchronize with spec_close. The vnode
 *	lock synchronizes spec_open with spec_setlevel.
 */
STATIC int
spec_open(vnode_t **vpp, int flag, struct cred *cr)
{
	major_t maj;
	dev_t dev;
	dev_t newdev;
	vnode_t *nvp;
	vnode_t *vp = *vpp;
	vnode_t *cvp = VTOS(vp)->s_commonvp;
	snode_t *sp = (snode_t *)vp->v_data;
	snode_t *csp = (snode_t *)cvp->v_data;
	int error = 0;
	int macmode = 0;
	struct module *modp = NULL;
	int (*open)();

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * Don't allow users to access devices if the file system
	 * of this device is mounted with nosuid on.
	 */
	if (vp->v_vfsp && (vp->v_vfsp->vfs_flag & VFS_NOSUID))
		return EPERM;

	flag &= ~FCREAT;		/* paranoia */

	dev = vp->v_rdev;
	newdev = dev;

	ASSERT(vp->v_type == VCHR || vp->v_type == VBLK);

retry:
	/* Lock the common snode exclusive. */
	if (flag & (FNDELAY | FNONBLOCK)) {
		/* Don't block if opened with O_NDELAY or O_NONBLOCK */
		if (!SPEC_RWLOCK_TRYWRLOCK(csp))
			return EAGAIN;
	} else {
		SPEC_RWLOCK_WRLOCK(csp);
	}

	sp->s_count++;	/* one more open reference */
	csp->s_count++;	/* one more open reference */

	/*
	 * If mac is installed, check for mandatory access control for devices.
  	 */
	if (mac_installed) {
		if (flag & FREAD)
			macmode |= VREAD;
		if (flag & (FWRITE|FTRUNC))
			macmode |= VWRITE;
		error = spec_saccess(sp, macmode, MAC_ACC, cr);
		if (error != 0)
			goto done;
	}

	switch (vp->v_type) {
	case VCHR:
		if ((maj = getmajor(dev)) >= cdevswsz) {
			error = ENXIO;
			break;
		}
		(void) RW_WRLOCK(&mod_cdevsw_lock, PLDLM);
		if (cdevsw[maj].d_str) {
			RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
			if ((error =
				stropen(cvp, &newdev, vpp, flag, cr)) == 0) {
				if (dev != newdev) {
					/*
					 * Clone open:
					 * Stropen would have set up the
					 * snodes for the new device.
					 */

					/*
					 * Decrement open count of odev.
					 */
					ASSERT(sp->s_count != 0);
					ASSERT(csp->s_count != 0);
					sp->s_count--;
					csp->s_count--;
					/*
					 * Unlock csp of master clone
					 */
					SPEC_RWLOCK_UNLOCK(csp);
					(*vpp)->v_vfsp = vp->v_vfsp;
					VN_RELE(vp);
					/*
					 * New clone common snode is locked
					 * exclusive from stropen.
					 */
					sp = VTOS(*vpp);
					cvp = sp->s_commonvp;
					csp = VTOS(cvp);
					sp->s_count++;
					csp->s_count++;
				}
				/*
				 * Normal open.
				 */
			}
		} else {
			if ((modp = cdevsw[maj].d_modp) != NULL) {
				if (MOD_IS_UNLOADING(modp)) {
					RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
					open = mod_cdev_open;
				} else {
					RW_UNLOCK(&mod_cdevsw_lock, PLDLM);
					MOD_HOLD_L(modp, PLBASE);
					open = cdevsw[maj].d_open;
				}
			} else {
				open = cdevsw[maj].d_open;
				RW_UNLOCK(&mod_cdevsw_lock, PLBASE);
			}
			if ((error = (*open)(&newdev, flag, OTYP_CHR, cr)) == 0
			    && dev != newdev) {
				/*
				 * Clone open:
				 * Allocate new snode for clone and copy
				 * security attributes from master clone.
				 */
				nvp = devsec_cloneopen(vp, newdev, VCHR);
				/*
				 * Decrement open count of odev.
				 */
				ASSERT(sp->s_count != 0);
				ASSERT(csp->s_count != 0);
				sp->s_count--;
				csp->s_count--;
				/*
				 * Unlock csp
				 */
				SPEC_RWLOCK_UNLOCK(csp);
				sp = VTOS(nvp);
				cvp = sp->s_commonvp;
				csp = VTOS(cvp);
				/*
				 * Lock new common snode exclusive
				 */
				SPEC_RWLOCK_WRLOCK(csp);
				/*
				 * Increment open count of new dev
				 */
				sp->s_count++;
				csp->s_count++;
				/*
				 * Character clones inherit fsid.
				 */
				sp->s_fsid = VTOS(vp)->s_fsid;
				nvp->v_vfsp = vp->v_vfsp;
				VN_RELE(vp);
				*vpp = nvp;
			}
		}
		break;
	case VBLK:
		if ((maj = (major_t)getmajor(dev)) >= bdevcnt) {
			error = ENXIO;
			break;
		}
		(void) RW_WRLOCK(&mod_bdevsw_lock, PLDLM);
		if ((modp = bdevsw[maj].d_modp) != NULL) {
			if (MOD_IS_UNLOADING(modp)) {
				RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
				open = mod_bdev_open;
			} else {
				RW_UNLOCK(&mod_bdevsw_lock, PLDLM);
				MOD_HOLD_L(modp, PLBASE);
				open = bdevsw[maj].d_open;
			}
		} else {
			open = bdevsw[maj].d_open;
			RW_UNLOCK(&mod_bdevsw_lock, PLBASE);
		}
		if ((error = (*open)(&newdev, flag, OTYP_BLK, cr)) == 0 &&
		    dev != newdev) {
			maj = getmajor(newdev);
			/*
			 * Clone open:
			 * allocating new snode for clone and copying
			 * security attributes from master clone
			 */
			nvp = devsec_cloneopen(vp, newdev, VBLK);
			ASSERT(sp->s_count != 0);
			ASSERT(csp->s_count != 0);
			sp->s_count--;
			csp->s_count--;
			/*
			 * Unlock csp
			 */
			SPEC_RWLOCK_UNLOCK(csp);
			sp = VTOS(nvp);
			cvp = sp->s_commonvp;
			csp = VTOS(cvp);
			/*
			 * Lock new common snode exclusive
			 */
			SPEC_RWLOCK_WRLOCK(csp);
			/*
			 * Block clones inherit fsid.
			 */
			sp->s_fsid = VTOS(vp)->s_fsid;
			nvp->v_vfsp = vp->v_vfsp;
			VN_RELE(vp);
			sp->s_count++;
			csp->s_count++;
			*vpp = nvp;
		}

		/*
		 * If this is first open, call the driver's size routine
		 * to see if the device size can be determined.
		 * Note that the driver is allowed to change its size between
		 * last close and another open, but not while it is open.
		 */
		if (sp->s_count == 1) {
			if (csp->s_count == 1) {
				int (*sizef)() = bdevsw[maj].d_size;
				int size;

				if (sizef != nulldev &&
				    (size = (*sizef)(newdev)) != -1)
					csp->s_devsize = dtob(size);
				else
					ASSERT(csp->s_devsize == UNKNOWN_SIZE);
			}
			sp->s_devsize = csp->s_devsize;
		}
		break;

	default:
		break;
	}
done:
	CHECK_COMPAT_ERRNO(error);
	if (error != 0) {
		sp->s_count--;	/* one less open reference */
		csp->s_count--;	/* one less open reference */
		/*
		 * If the driver is loadable, decrement its
		 * count on failure.
		 */
		if (modp)
			MOD_RELE(modp);
	}
	SPEC_RWLOCK_UNLOCK(csp);
	/*
	 * clone open raced detected by driver. Retry open on device
	 */
	if (error == ECLNRACE) {
		error = 0;
		goto retry;
	}
	return error;
}

/*
 * int
 * spec_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset,
 *	      cred_t *cr)
 *
 * Calling/Exit State:
 *	This routine may block, so no spinlocks may be held on entry,
 *	and none will be held at exit.
 *
 * Description:
 *	We update the snode's open count while holding the 
 *	common snode rwlock exclusive to synchronize with
 *	spec_open.
 */
/* ARGSUSED */
STATIC int
spec_close(vnode_t *vp, int flag, boolean_t lastclose, off_t offset, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	snode_t *csp;
	int error = 0;
	struct module *modp = NULL;
	rwlock_t *swlockp;
	dev_t dev;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);
	ASSERT(vp->v_type == VCHR || vp->v_type == VBLK);

	csp = VTOS(sp->s_commonvp);
	SPEC_RWLOCK_WRLOCK(csp);

	if (vp->v_filocks)
		cleanlocks(vp, u.u_procp->p_epid, u.u_procp->p_sysid);

	if (vp->v_stream)
		strclean(vp, B_FALSE);

	if (!lastclose)
		goto out;

	ASSERT(sp->s_count != 0 && csp->s_count != 0);
	sp->s_count--;	/* one fewer open reference */
	csp->s_count--;	/* one fewer open reference */

	/*
	 * Only call the close routine when the last open reference through
	 * any [s,v]node goes away.
	 */
	if (csp->s_count == 0)
		error = device_close(vp, flag, cr);

	dev = sp->s_dev;

	if (vp->v_type == VBLK) {
		if (sp->s_count == 0) {
			sp->s_devsize = UNKNOWN_SIZE;
			if (csp->s_count == 0)
				csp->s_devsize = UNKNOWN_SIZE;
		}
		(void) RW_RDLOCK((swlockp = &mod_bdevsw_lock), PLDLM);
		modp = bdevsw[getmajor(dev)].d_modp;
	} else {
		(void) RW_RDLOCK((swlockp = &mod_cdevsw_lock), PLDLM);
		modp = cdevsw[getmajor(dev)].d_modp;
	}

	/*
	 * If the driver is loadable, and is not streams driver,
	 * decrement its hold count. The hold count of streams
	 * driver is handled in strclose().
	 */
	if (modp && (vp->v_type == VBLK ||
		     !cdevsw[getmajor(sp->s_dev)].d_str)) {
		(void) LOCK(&modp->mod_lock, PLDLM);
		RW_UNLOCK(swlockp, PLDLM);
		MOD_RELE_L(modp, PLBASE);
	} else
		RW_UNLOCK(swlockp, PLBASE);
		
out:
	SPEC_RWLOCK_UNLOCK(csp);
	return error;
}

/*
 * STATIC int
 * device_close(vnode_t *vp, int flag, cred_t *cr)
 *
 * Calling/Exit State:
 *	The common snode rwlock is locked exclusive on entry and
 *	remains locked at exit.
 *
 */
STATIC int
device_close(vnode_t *vp, int flag, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	dev_t dev = sp->s_dev;
	enum vtype type = vp->v_type;
	int error;

	/* added for enchanced security */
	if (mac_installed)
		devsec_close(sp);

	switch (type) {
	case VCHR:
		if (cdevsw[getmajor(dev)].d_str) {
			error = strclose(sp->s_commonvp, flag, cr);
			vp->v_stream = NULL;
		} else {
			error = (*cdevsw[getmajor(dev)].d_close)
			  (dev, flag, OTYP_CHR, cr);
		}
		break;

	case VBLK:
		/*
		 * On last close a block device we must
		 * invalidate any in-core blocks so that we
		 * can, for example, change floppy disks.
		 */
		(void) spec_putpage(sp->s_commonvp, 0, 0, B_INVAL,
		  (struct cred *)NULL);

		error = (*bdevsw[getmajor(dev)].d_close)
		  (dev, flag, OTYP_BLK, cr);
		break;
	}

	CHECK_COMPAT_ERRNO(error);
	return error;
}

/*
 * int
 * spec_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcr)
 *
 * Calling/Exit State:
 *	If the vp is associated with a block device, caller holds
 *	the  snode s_rwlock in shared mode via VOP_RWRDLOCK().
 *	The lock will be released by the caller via VOP_RWUNLOCK().
 *	For a character device, no lock is held on entry or at exit.
 *
 * Description:
 *	Handle read from char or block device.
 *
 */
/* ARGSUSED */
STATIC int
spec_read(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcr)
{
	cred_t *cr = VCURRENTCRED(fcr);
	int error;
	snode_t *sp = VTOS(vp);
	dev_t dev = sp->s_dev;
	unsigned on, n;
	vnode_t *blkvp;
	unsigned long bdevsize;
	int oresid;

	ASSERT(vp->v_type == VCHR || vp->v_type == VBLK);

	/* OPEN ISSUE - Is it ok not to do any access check for
	 * zero-length?
	 */
	if (uiop->uio_resid == 0)
		return 0;

	/*
	 * security check if device is in private state or if it is dynamic 
	 *
	 */
	if ((mac_installed) &&
	    (error = spec_access(vp, VREAD, MAC_ACC | MAC_RW, cr)))
		return error;

	if (!(vp->v_flag & VNOMAP) &&
		vp->v_vfsp && WRITEALLOWED(vp, cr)) {
#ifdef CC_PARTIAL
		MAC_ASSERT (sp, MAC_SAME);
#endif
		smark(sp, SACC);
	}

	if (vp->v_type == VCHR) {
		if (cdevsw[getmajor(dev)].d_str)
			error = strread(vp, uiop, cr);
		else 
			error = (*cdevsw[getmajor(dev)].d_read)(dev, uiop, cr);
		CHECK_COMPAT_ERRNO(error);
		return error;
	}

	/*
	 * Block device.
	 */

	error = 0;
	blkvp = sp->s_commonvp;
	bdevsize = sp->s_devsize;
	oresid = uiop->uio_resid;

	do {

		int diff;
		caddr_t base;

		on = uiop->uio_offset & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uiop->uio_resid);
		diff = bdevsize - uiop->uio_offset;

		if (diff <= 0) 
			break;
		if (diff < n)
			n = diff;

		base = segmap_getmap(segkmap, blkvp, uiop->uio_offset, n,
				     S_READ, B_FALSE, NULL);

		if ((error = uiomove(base + on, n, UIO_READ, uiop)) == 0) {
			int flags;
			/*
			 * If we read a whole block, we won't need this
			 * buffer again soon.
			 */
			if (n + on == MAXBSIZE)
				flags = SM_DONTNEED;
			else
				flags = 0;
			error = segmap_release(segkmap, base, flags);
		} else {
			(void) segmap_release(segkmap, base, 0);
			/*
			 * If driver does not have a size routine, the size
			 * of the driver is assumed to be infinite.
			 */
			if (bdevsize == UNKNOWN_SIZE) {
				/* OPEN ISSUE - Is it appropriate not to
				 * return error to caller? */
				error = 0;
				break;
			}
		}
		CHECK_COMPAT_ERRNO(error);
	} while (error == 0 && uiop->uio_resid > 0);

	if (oresid != uiop->uio_resid) {          /* partial read */
                error = 0;
	}
	return error;
}

/*
 * int
 * spec_write(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcr)
 *
 * Calling/Exit State:
 *	If the vp is associated with a block device, caller holds
 *	the common snode s_rwlock in excl. mode via VOP_RWWRLOCK().
 *	The lock will be released by the caller via VOP_RWUNLOCK().
 *	For a character device, no lock is held on entry or at exit.
 *
 * Description:
 *	Handles write to char or block device.
 */
/* ARGSUSED */
STATIC int
spec_write(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *fcr)
{
	cred_t *cr = VCURRENTCRED(fcr);
	int error;
	snode_t *sp = VTOS(vp);
	unsigned n, on;
	unsigned long bdevsize;
	vnode_t *blkvp;
	int diff;
	off_t off;
	caddr_t base;
	int flags;
	int oresid;

	ASSERT(vp->v_type == VCHR || vp->v_type == VBLK);

	/*
	 * security checks if device is private or dynamic
	 */
	if ((mac_installed) &&
	    (error = spec_access(vp, VWRITE, MAC_ACC|MAC_RW, cr)))
		return error;
								
	if (vp->v_type == VCHR) {

		dev_t dev = sp->s_dev;

		/* 
		 * there is no MAC_VACCESS around smark because the access
		 * check is covered where needed in spec_access above.
		 * If the device is DYNAMIC spec_access does the MAC_VACCESS
		 * with VWRITE. if spec_access we know we have VWRITE, allowing
		 * us to do the smark. If the device is STATIC, no MAC_VACCESS
		 * is performed. This is still O.K. since on a STATIC device 
		 * the attributes can't change while a device is open
		 * and we had to pass write access on the open, so
		 * we would never come here if we didn't have write access.
		 */
		smark(sp, SUPD|SCHG);
		if (cdevsw[getmajor(dev)].d_str)
			error = strwrite(vp, uiop, cr);
		else
			error = (*cdevsw[getmajor(dev)].d_write)(dev, uiop, cr);
		CHECK_COMPAT_ERRNO(error);
		return error;
	}

	if (uiop->uio_resid == 0)
		return 0;

	/*
	 * Initialize error to ENXIO so that if it's first
	 * time in the loop and io fails, ENXIO is returned.
	 * For subsequent times thorough the loop, if nothing
	 * is written because we are going off the end of the
	 * device, no error should be returned.
	 */
	error = ENXIO;

	blkvp = sp->s_commonvp;
	bdevsize = sp->s_devsize;
	oresid = uiop->uio_resid;

	do {

		off = uiop->uio_offset;
		on = off & MAXBOFFSET;
		n = MIN(MAXBSIZE - on, uiop->uio_resid);
		diff = bdevsize - off;

		/* Going off the end of the device. */
		if (diff <= 0)
			break;

		if (diff < n)
			n = diff;

		base = segmap_getmap(segkmap, blkvp, off, n, S_WRITE,
				     B_FALSE, NULL);

		error = uiomove(base + on, n, UIO_WRITE, uiop);

		if (error == 0) {

			/*
			 * Force write back for synchronous write cases.
			 */
			if (ioflag & IO_SYNC)
				flags = SM_WRITE;
			else if (n + on == MAXBSIZE) {
				/*
				 * Have written a whole block.
				 * Start an asynchronous write and
				 * mark the buffer to indicate that
				 * it won't be needed again soon.
				 */
				flags = SM_WRITE | SM_ASYNC | SM_DONTNEED;
			} else
				flags = 0;
			smark(sp, SUPD|SCHG);
			/* OPEN ISSUE - What if putpage only writes out
			 * part of the data? How should the partial write
			 * info propogate back?
			 */
			error = segmap_release(segkmap, base, flags);
		} else {
			off_t noff;
			ASSERT(uiop->uio_offset < off + n);

			/*
			 * If we had some sort of error during uiomove,
			 * call segmap_abort_create to have the pages
			 * aborted if we created them.
			 */
			noff = segmap_abort_create(segkmap,
					base, uiop->uio_offset,
					(off + n - uiop->uio_offset));

			if (noff != -1 && noff < uiop->uio_offset) {
				/*
				 * Some pages aborted, need to fix resid.
				 */
				uiop->uio_resid += uiop->uio_offset - noff;
			}

			/*
			 * For synchronous writes, if any data was
			 * written, force the data to be flushed out.
			 */
			if (ioflag & IO_SYNC && uiop->uio_offset != off)
				flags = SM_WRITE;
			else
				flags = 0;
			(void) segmap_release(segkmap, base, flags);
		}
		CHECK_COMPAT_ERRNO(error);
	} while (error == 0 && uiop->uio_resid > 0);

	if (oresid != uiop->uio_resid)  {         /* partial write */
                error = 0;
	}

	return error;
}


/*
 * int
 * spec_ioctl(vnode_t *vp, int cmd, int arg, int mode, cred_t *cr, int *rvalp)
 *	Perform an ioctl command on a device file.	
 *
 * Calling/Exit State:
 *	No locking on entry or at exit;
 *
 */
/* ARGSUSED */
STATIC int
spec_ioctl(vnode_t *vp, int cmd, int arg, int mode, cred_t *cr, int *rvalp)
{
	dev_t dev;
	int error = 0;

	if (vp->v_type != VCHR)
		return ENOTTY;
	/*
	 * security checks if device is private or dynamic
	 */

	if (mac_installed) {
		switch(cmd) {

		/*
                 * we know that MIOC_READKSYM, MIOC_IREADKSYM, TCGETS and
                 * TCGETA are ioctls that only do reads so we can use the
                 * less restrictive VREAD access check on them. All other
                 * ioctls must pass the VWRITE test
                 */
		case MIOC_READKSYM:
                case MIOC_IREADKSYM:
		case TCGETS:
		case TCGETA:
		/* only check state of device and MAC acc on dynamic devices */
			if (error = spec_access(vp, VREAD, MAC_ACC| MAC_IO, cr))
				return error;
			break;
		default:
			if (error = spec_access(vp, VWRITE, MAC_ACC, cr))
				return error;
			break;
		}
	}

	dev = VTOS(vp)->s_dev;
	if (cdevsw[getmajor(dev)].d_str)
		error = strioctl(vp, cmd, arg, mode, U_TO_K, cr, rvalp);
	else
		error = (*cdevsw[getmajor(dev)].d_ioctl)(dev, cmd, arg, 
			mode, cr, rvalp);
	CHECK_COMPAT_ERRNO(error);
	return error;
}

/*
 * int
 * spec_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 *	Since there is a one-to-one correspondense of the snode
 *	with its realvp, VOP_GETATTR of realvp guarantees
 *	consistency without snode locking in the case of a
 *	non-NULL realvp.
 *
 */
/* ARGSUSED */
STATIC int
spec_getattr(vnode_t *vp, vattr_t *vap, int flags, cred_t *cr)
{
	int error;
	snode_t *sp;
	vnode_t *realvp;
	pl_t pl;

	if (flags & ATTR_COMM && vp->v_type == VBLK)
		vp = VTOS(vp)->s_commonvp;
	
	sp = VTOS(vp);
	if ((realvp = sp->s_realvp) == NULL) {
		/*
		 * No real vnode behind this one.  Fill in the fields
		 * from the snode.
		 *
		 * This code should be refined to return only the
		 * attributes asked for instead of all of them.
		 */
		vap->va_type = vp->v_type;
		vap->va_mode = 0;
		vap->va_uid = vap->va_gid = 0;
		vap->va_fsid = sp->s_fsid;
		vap->va_nodeid = sp->s_nodeid;
		vap->va_nlink = 0;
		vap->va_rdev = sp->s_dev;
		vap->va_blksize = MAXBSIZE;
		if (vap->va_mask & AT_ACLCNT) {
			vap->va_aclcnt = NACLBASE;
		}
	} else if (error = VOP_GETATTR(realvp, vap, flags, cr))
		return error;
	vap->va_size = (sp->s_devsize == UNKNOWN_SIZE ? 0 : sp->s_devsize);
	vap->va_nblocks = btod(vap->va_size);
	pl = SPEC_SLOCK(sp);
	vap->va_atime.tv_sec = sp->s_atime;
	vap->va_atime.tv_nsec = 0;
	vap->va_mtime.tv_sec = sp->s_mtime;
	vap->va_mtime.tv_nsec = 0;
	vap->va_ctime.tv_sec = sp->s_ctime;
	vap->va_ctime.tv_nsec = 0;
	vap->va_vcode = 0;
	SPEC_SUNLOCK(sp, pl);

	return 0;
}

/*
 * int
 * spec_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 *	Since there is a one-to-one correspondense of the snode
 *	with its realvp, VOP_SETATTR of realvp guarantees
 *	consistency without snode locking in the case of a
 *	non-NULL realvp.
 *
 */
STATIC int
spec_setattr(vnode_t *vp, vattr_t *vap, int flags, int ioflags, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	vnode_t *realvp;
	int error;
	pl_t pl;

	if ((realvp = sp->s_realvp) == NULL)
		error = 0;	/* no real vnode to update */
	else
		error = VOP_SETATTR(realvp, vap, flags, ioflags, cr);
	if (error == 0) {
		/*
		 * If times were changed, update snode.
		 */
		pl = SPEC_SLOCK(sp);
		if (vap->va_mask & AT_ATIME)
			sp->s_atime = vap->va_atime.tv_sec;
		if (vap->va_mask & AT_MTIME) {
			sp->s_mtime = vap->va_mtime.tv_sec;
			sp->s_ctime = hrestime.tv_sec;
		}
		SPEC_SUNLOCK(sp, pl);
	}
	return error;
}

/*
 * int
 * spec_access(vnode_t *vp, int mode, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	The common snode lock must not be held on entry. 
 *	The snode's rwlock is held *shared* while determining
 *	accessibility of the file to the caller.
 *
 *	A return value of 0 indicates success; otherwise, a valid
 *	errno is returned. Errnos returned directly by this routine
 *	are:
 *		EIO	The file system containing <vp> has been invalidated
 *			and is unaccessible.
 *
 * Description:
 *	If the file system containing <vp> has not been sealed then
 *	obtain the snode shared/exclusive lock in shared mode. Use
 *	spec_saccess() to determine accessibility. Return what it does
 *	after releasing the shared/exclusive lock.
 */
/* ARGSUSED */
STATIC int
spec_access(vnode_t *vp, int mode, int flags, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	struct snode *csp = VTOS(sp->s_commonvp);
	int error;

	SPEC_RWLOCK_RDLOCK(csp);
	error = spec_saccess(sp, mode, flags, cr);
	SPEC_RWLOCK_UNLOCK(csp);

	return error;
}

/*
 * int
 * spec_fsync(vnode_t *vp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	In order to sync out the snode times without multi-client problems,
 *	We need to make sure the times written out are never earlier than
 *	the times already set in the vnode. This is done via VOP_SETATTR
 *	with ATTR_UPDTIME. The underlying file system will compare the
 *	times in vattr with the times in the real file node and update
 *	the times in the real file node only if the times in vattr is
 *	more current.
 */
STATIC int
spec_fsync(vnode_t *vp, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	vnode_t *realvp;
	vnode_t *cvp;
	struct vattr va;
	pl_t pl;

	/*
	 * If times didn't change, don't flush anything.
	 */
	pl = SPEC_SLOCK(sp);
	if ((sp->s_flag & (SACC|SUPD|SCHG)) == 0 && vp->v_type != VBLK) {
		SPEC_SUNLOCK(sp, pl);
		return 0;
	}

	sp->s_flag &= ~(SACC|SUPD|SCHG);
	SPEC_SUNLOCK(sp, pl);

	cvp = sp->s_commonvp;
	if (vp->v_type == VBLK && cvp != vp
	  && cvp->v_pages != NULL && (VTOS(cvp))->s_swapcnt == 0)
		(void) VOP_PUTPAGE(cvp, 0, 0, 0, sys_cred);

	/*
	 * If no real vnode to update, don't flush anything.
	 */
	if ((realvp = sp->s_realvp) == NULL)
		return 0;

	va.va_mask = AT_ATIME|AT_MTIME;
	pl = SPEC_SLOCK(sp);
	va.va_atime.tv_sec = sp->s_atime;
	va.va_atime.tv_nsec = 0;
	va.va_mtime.tv_sec = sp->s_mtime;
	va.va_mtime.tv_nsec = 0;
	SPEC_SUNLOCK(sp, pl);
	(void) VOP_SETATTR(realvp, &va, ATTR_UPDTIME, 0, cr);

	(void) VOP_FSYNC(realvp, cr);
	return 0;
}

/*
 * void
 * spec_inactive(vnode_t *vp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Inactivate the snode. If someone raced and got a new
 *	reference before we are able to get the hash table
 *	lock, abort the inactivation.
 */
STATIC void
spec_inactive(vnode_t *vp, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	vnode_t *cvp;

	ASSERT(KS_HOLD0LOCKS());
        ASSERT(getpl() == PLBASE);

	/* lock hash table */
        (void) LOCK(&spec_table_mutex, FS_SPTBLPL);
	VN_LOCK(vp);
	if (vp->v_count != 1) {
		vp->v_count--;
		VN_UNLOCK(vp);
		UNLOCK(&spec_table_mutex, PLBASE);
		return;
	}
	/*
	 * The reference count is exactly 1. We hold the last hard
	 * reference to the vnode. We exchange the hard reference
	 * for a soft reference to suppress any new reference via
	 * sinsert.
	 */
	vp->v_count = 0;
	vp->v_softcnt++;
	sp->s_flag |= SINVALID;
	VN_UNLOCK(vp);

        UNLOCK(&spec_table_mutex, PLBASE);

	cvp = sp->s_commonvp;

	if (sp->s_realvp) {
		(void) spec_fsync(vp, cr);
		VN_RELE(sp->s_realvp);
		sp->s_realvp = NULL;
		ASSERT(vp->v_pages == NULL);
		if (cvp) {
			ASSERT(VN_CMP(vp, cvp) == 0);
			VN_RELE(cvp);
		}
	} else if (vp->v_pages != NULL) {
		ASSERT(vp->v_type == VBLK);
		ASSERT(vp == cvp);
		VOP_PUTPAGE(vp, 0, 0, B_INVAL, sys_cred);
	}

	VN_SOFTRELE(vp);

}

/*
 * STATIC void
 * spec_release(vnode_t *vp)
 *	Release the storage for a totally unreferenced vnode.
 *
 * Calling/Exit State:
 *	The user may hold locks.
 *
 *	This function does not block.
 */
STATIC void
spec_release(vnode_t *vp)
{
	snode_t   	*sp = VTOS(vp);

	/*
	 * The snode is privately held at this point.
	 * Therefore, no locking is necesary in order to inspect it.
	 */
	ASSERT(VN_IS_RELEASED(vp));
	ASSERT(vp->v_pages == NULL);

	/* Remove the snode from the hash. */
        (void) LOCK(&spec_table_mutex, FS_SPTBLPL);
	sdelete(sp);
	UNLOCK(&spec_table_mutex, PLBASE);

	VN_DEINIT(vp);
	SPEC_RWLOCK_DEINIT(sp);
	SPEC_SLOCK_DEINIT(sp);
	kmem_free(sp, sizeof (*sp));

	/* Wake up waiters, if any. */
	if (SV_BLKD(&snode_sv))
		SV_BROADCAST(&snode_sv, 0);

	return;
}

/*
 * int
 * spec_fid(vnode_t *vp, fid_t **fidpp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 *	Since there is a one-to-one correspondense of the snode
 *	with its realvp, VOP_FID of realvp guarantees
 *	consistency without snode locking in the case of a
 *	non-NULL realvp.
 *
 */
STATIC int
spec_fid(vnode_t *vp, fid_t **fidpp)
{
	vnode_t *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return VOP_FID(realvp, fidpp);
	else
		return EINVAL;
}

/*
 * int
 * spec_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *
 * Calling/Exit State:
 *	No lock is held on entry. If vnode is VBLK, at exit the snode rwlock is
 * 	held in shared or exclusive mode depending on "mode".
 *
 */
/* ARGSUSED */
STATIC int
spec_rwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	snode_t *sp;

	if (vp->v_type == VCHR)
		return 0;

	sp = VTOS(vp);

	if (mode == LOCK_EXCL) {
		SPEC_RWLOCK_WRLOCK(sp);
	} else if (mode == LOCK_SHARED) {
		SPEC_RWLOCK_RDLOCK(sp);
	} else {
		/*
		 *+ An invalid mode was passed as a parameter to
		 *+ this routine. This indicates a kernel software
		 *+ problem
		 */
		cmn_err(CE_PANIC,"spec_rwlock: invalid lock mode requested");
	}

	return 0;
}

/*
 * int
 * spec_rwunlock(vnode_t *vp, off_t off, int len)
 *
 * Calling/Exit State:
 *	If vnode is VBLK, the snode rwlock is held on entry and
 *	unlocked upon exit.
 *
 */
/* ARGSUSED */
STATIC void
spec_rwunlock(vnode_t *vp, off_t off, int len)
{
	snode_t *sp;

	if (vp->v_type == VCHR)
		return;

	sp = VTOS(vp);
	SPEC_RWLOCK_UNLOCK(sp);

	return;
}

/*
 * int
 * spec_seek(vnode_t *vp, off_t ooff, off_t *noffp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 */
/* ARGSUSED */
STATIC int
spec_seek(vnode_t *vp, off_t ooff, off_t *noffp)
{
	return 0;
}

/*
 * int
 * spec_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag,
 *	       off_t offset, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Obtain the common snode rwlock before checking whether
 *	the file is mapped. If so, fail the frlock.
 */
/* ARGSUSED */
STATIC int
spec_frlock(vnode_t *vp, int cmd, flock_t *bfp, int flag, off_t offset,
	    cred_t *cr)
{

	snode_t *csp = VTOS(VTOS(vp)->s_commonvp);
	int	 error;
	off_t	 size;

	if (vp->v_type != VCHR)
		SPEC_RWLOCK_RDLOCK(csp);
	size = (csp->s_devsize == UNKNOWN_SIZE ? 0 : csp->s_devsize);
	error = fs_frlock(vp, cmd, bfp, flag, offset, cr, size);
	if (vp->v_type != VCHR)
		SPEC_RWLOCK_UNLOCK(csp);

	return error;
}


/*
 * int
 * spec_realvp(vnode_t *vp, vnode_t **vpp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 */
/* ARGSUSED */
STATIC int
spec_realvp(vnode_t *vp, vnode_t **vpp)
{
	snode_t *sp = VTOS(vp);
	vnode_t *rvp;

	vp = sp->s_realvp;
	if (vp && VOP_REALVP(vp, &rvp) == 0)
		vp = rvp;
	*vpp = vp;
	return 0;
}

/*
 * int
 * spec_getpageio(vnode_t *vp, off_t off, uint_t len, page_t *pp, int flag)
 *
 * Calling/Exit State:
 *	The snode rwlock may or may not be held locked.
 *
 * Description:
 * 	Set up for page io and call driver strategy routine to
 *	fill the pages. Pages are linked by p_next. If flag is
 *	B_ASYNC, don't wait for io to complete.
 */
STATIC int
spec_getpageio(vnode_t *vp, off_t off, uint_t len, page_t *pp, int flag)
{
	struct buf *bp;
	int err = 0;

	ASSERT((off & DEV_BOFFSET) == 0);
	ASSERT((len & DEV_BOFFSET) == 0);

	bp = pageio_setup(pp, off & PAGEOFFSET, len, flag | B_READ);

	bp->b_edev = vp->v_rdev;
	bp->b_blkno = btodt(off);

	(*bdevsw[getmajor(vp->v_rdev)].d_strategy)(bp);

	/* Update the number of dev sized blocks read by this LWP */
	ldladd(&u.u_ior, btodb(len));
#ifdef PERF
	mets_fsinfo[MET_OTHER].pgin++;
	mets_fsinfo[MET_OTHER].pgpgin++;
	mets_fsinfo[MET_OTHER].sectin += btodb(len);
#endif /* PERF */

	/*
	 * Mark the level of the process actually
	 * faulting in the page.  Anonymous pages
	 * are skipped.
	 */
	if (mac_installed && (VTOS(vp)->s_swapcnt == 0)) {
		uint_t i; 
		page_t *pp2 = pp;

		for (i = btop(len); i-- != 0; pp2 = pp2->p_next)
			pp2->p_lid = CRED()->cr_lid;
	}

	if (!(flag & B_ASYNC)) {
		err = biowait(bp);
		pageio_done(bp);
	}
#ifdef PERF
	else {
		mets_fsinfo[MET_OTHER].rasectin += btodb(len);
		mets_fsinfo[MET_OTHER].rapgpgin++;
	}
#endif

	return err;
}

/*
 * klustsize should be a multiple of PAGESIZE. It is arbitrarily
 * defined to be 8K.
 */
#define	KLUSTSIZE	(8 * 1024)

STATIC int klustsize = KLUSTSIZE;
STATIC int spec_ra = 0;

/*
 * int
 * spec_getapage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp,
 *	page_t *pl[], uint_t plsz, struct seg *seg,
 *	vaddr_t addr, enum seg_rw rw, cred_t *cr)
 *
 * Calling/Exit State:
 *	The snode rwlock may or may not be held locked.
 *
 * Description:
 * 	Return all the pages from [off..off+len) in block device.
 */
/* ARGSUSED */
STATIC int
spec_getapage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp,
	page_t *pl[], uint_t plsz, struct seg *seg,
	vaddr_t addr, enum seg_rw rw, cred_t *cr)
{
	snode_t *sp;
	page_t *pp, *pp2, **ppp;
	off_t roff, io_off;
	page_t *io_pl[KLUSTSIZE/PAGESIZE];
	uint_t io_len, sz;
	int xlen;
	uint_t adj_klustsize;
	int i, j, err;

	sp = VTOS(vp);

	err = 0;

	if (sp->s_devsize == UNKNOWN_SIZE) {
		adj_klustsize = PAGESIZE;
	} else
		adj_klustsize = klustsize;

	roff = off & PAGEMASK;

	/*
	 * If we are not doing read-ahead, we call page_lookup_or_create
	 * to look up the page in the page cache or have it created. If
	 * the page is in the page cache, the page is returned read-locked.
	 * If the page is created, it is write-locked.
	 */
	if (pl != NULL) {
		pp = page_lookup_or_create(vp, roff);
		ASSERT(pp != NULL);

		/*
		 * If we find the page in the page cache
		 * or if overwriting the whole page, no
		 * need to do any io.
		 */
		if (PAGE_IS_RDLOCKED(pp) ||
		    (rw == S_OVERWRITE && (off & PAGEOFFSET) == 0 &&
			len == PAGESIZE)) {

			if (pl != NULL) {
				*pl++ = pp;
				*pl = NULL;
			} else
				page_unlock(pp);

			return 0;
		}
	} else {
		/*
		 * This is a read-ahead, we don't supply the center
		 * page. Instead, we let pvn_kluster decide if and
		 * how much io is appropriate to do. If pvn_kluster
		 * finds the center page or if memory is not readily
		 * available, it will return a null list and we will
		 * NOT do any io.
		 */
		pp = NULL;
		ASSERT(rw != S_OVERWRITE);
	}

	/*
	 * If page is newly created, we need
	 * to do disk I/O to fill in the data.
	 */

	/*
	 * If this is an overwrite case, we should see if we can trim
	 * the io size for the portion of the page that will be
	 * overwritten anyway. If io_off starts on a page boundary,
	 * we only need to do io on [io_off+io_len-1, end-of-page].
	 * If io_off doesn't start on a page bounday but io_off+io_len-1
	 * ends on a page boundary, then we only need to do io on
	 * [page-boundary, io_off]. We don't try to trim io size because
	 * if either end is not on a pageboundary, we would have to issue
	 * two io to fill in the page which is more costly than filling
	 * the whole page with one io.
	 *
	 * However, for files that are currently mapped, we cannot
	 * take advantage of this optimization to avoid the potential
	 * deadlock if the page on the "from" side of the copy in
	 * VOP_WRITE() happens to be a page that was exclusively locked
	 * on the "to" side.
	 */
	if (rw == S_OVERWRITE && VTOS(sp->s_commonvp)->s_mapcnt == 0) {
		if ((off & PAGEOFFSET) == 0) {
			io_off = ((off + len) & DEV_BMASK);
			io_len = PAGESIZE - (io_off & PAGEOFFSET);
		} else if (((off + len) & PAGEOFFSET) == 0) {
			io_off = roff;
			io_len = (((off & PAGEOFFSET) + DEV_BSIZE - 1)
								& DEV_BMASK);
		} else {
			io_off = roff;
			io_len = PAGESIZE;
		}
		io_pl[0] = pp;
	} else {
		uint_t blksz, blkoff;

		blkoff = (off / adj_klustsize) * adj_klustsize;
		if (blkoff + adj_klustsize <= sp->s_devsize)
			blksz = adj_klustsize;
		else
			blksz = pageroundup(sp->s_devsize) - blkoff;

		pp2 = pp = pvn_kluster(vp, off, seg, addr, &io_off, &io_len,
			 			blkoff, blksz, pp);
		/*
		 * If there is not a page list for us to do I/O, this must be
		 * a read-ahead. Either the page is already in the page cache
		 * or memory is not available. We simply return.
		 */
		if (pp == NULL) {
			ASSERT(pl == NULL);
			return 0;
		}

		ppp = io_pl;
		do {
			*ppp++ = pp2;
			pp2->p_nio = 1;
		} while ((pp2 = pp2->p_next) != pp);
	}
	ASSERT(pp != NULL);

	/*
	 * If device size is known, zero part of page beyond end of device.
	 */
	if (sp->s_devsize != UNKNOWN_SIZE &&
	    (xlen = io_off + io_len - sp->s_devsize) > 0) {
		ASSERT(xlen < PAGESIZE);
		ASSERT(((io_off + io_len) & PAGEOFFSET) == 0);
		ASSERT(pp->p_prev->p_offset == ptob(btop(io_off + io_len - 1)));
		pagezero(pp->p_prev, PAGESIZE - xlen, xlen);
		io_len -= xlen;
	} else
		xlen = PAGESIZE - (io_len & PAGEOFFSET);

	if (io_len != 0) {
		pp->p_nio = 1;

		err = spec_getpageio(vp, io_off, io_len, pp,
				     (pl == NULL ? B_ASYNC : 0));

		/*
		 * If we encountered any I/O error, the pages should have been
		 * aborted by pvn_done() and we need not downgrade the page
		 * lock, nor should we reference any of the pages in the
		 * page list.
		 */
		if (pl == NULL || err)
			return err;
	} else {
		/*
		 * The whole page was covered by S_OVERWRITE and/or zeroing
		 * past the end of the device.
		 */
		ASSERT(rw == S_OVERWRITE);
		ASSERT(pp == io_pl[0]);

		if (pl != NULL) {
			*pl++ = pp;
			*pl = NULL;
		} else {
			page_unlock(pp);
		}
		return 0;
	}

	/*
	 * At this point, we've already returned if async.
	 * Otherwise, load the pages in the page list to return to caller.
	 */
	if (plsz >= io_len) {
		/*
		 * Everything fits, set up to load up all the pages.
		 */
		i = 0;
		sz = pageroundup(io_len);
	} else {
		/*
		 * Not everything fits. Set up to load plsz worth
		 * starting at the needed page.
		 */
		for (i = 0; io_pl[i]->p_offset != off; i++) {
			ASSERT(i < btopr(io_len) - 1);
		}
		sz = plsz;
		if (ptob(i) + sz > io_len)
                        i = btopr(io_len - sz);
	}

	j = i;
	ppp = pl;
	do {
		*ppp = io_pl[j++];
		ASSERT(PAGE_IS_WRLOCKED(*ppp));
		/*
		 * If page is not completed initialized, which is only
		 * possible for the S_OVERWRITE case, the page is
		 * returned to the caller with a write-lock; otherwise,
		 * downgrade to a read-lock.
		 */
		if (io_len + xlen >= PAGESIZE)
			page_downgrade_lock(*ppp);
#ifdef DEBUG
		else
			ASSERT(rw == S_OVERWRITE);
#endif
		ppp++;
	} while ((sz -= PAGESIZE) != 0);

	*ppp = NULL;		/* terminate list */

	/* Unlock pages we're not returning to our caller. */
	if (j >= btopr(io_len))
		j = 0;
	while (j != i) {
		if (io_pl[j] != NULL) {
			ASSERT(PAGE_IS_WRLOCKED(io_pl[j]));
			page_unlock(io_pl[j]);
		}
		j++;
		if (j >= btopr(io_len))
			j = 0;
	}

	return 0;
}

/*
 * int
 * spec_getpage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp,
 *		page_t *pl[], uint_t plsz, struct seg *seg,
 *		vaddr_t addr, enum seg_rw rw, cred_t *cr)
 *
 * Calling/Exit State:
 *	The snode rwlock may or may not be held locked.
 *
 * Description:
 * 	Return all the pages from [off..off+len) in block device.
 */
/* ARGSUSED */
STATIC int
spec_getpage(vnode_t *vp, uint_t off, uint_t len, uint_t *protp,
      page_t *pl[], uint_t plsz, struct seg *seg,
              vaddr_t addr, enum seg_rw rw, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	uint_t nextoff;
	int err;

#ifdef PERF
	mets_fsinfo[MET_OTHER].getpage++;
#endif
	ASSERT(vp->v_type == VBLK && sp->s_commonvp == vp);

	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	if (off + len > ((sp->s_devsize + PAGEOFFSET) & PAGEMASK))
		return EFAULT;	/* beyond EOF */

	if (protp != NULL)
		*protp = PROT_ALL;

        if (btop(off) == btop(off + len - 1))
		err = spec_getapage(vp, off, len, protp, pl, plsz, seg, addr,
		  rw, cr);
	else
		err = pvn_getpages(spec_getapage, vp, off, len, protp, pl,
		  plsz, seg, addr, rw, cr);

	nextoff = ptob(btopr(off + len) - 1);
	if (!err && spec_ra && sp->s_nextr == off && rw != S_OVERWRITE) {
		/*
		 * For read-ahead, pass a NULL pl so that if it's
		 * not convenient, io will not be done. And also,
		 * if io is done, we don't wait for it.
		 */
#ifdef PERF
		mets_fsinfo[MET_OTHER].ra++;
#endif
		err = spec_getapage(vp, nextoff, PAGESIZE, NULL, NULL, 0,
				seg, addr, rw, cr);
	}
	sp->s_nextr = nextoff;

	return err;

}

/*
 * int
 * spec_getpagelist(vnode_t *vp, off_t off, page_t *pp, uint_t plsz,
 * 		void *bmapp, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Fills in data for each of the pages in the pp list from
 *	consecutive logical offsets starting from off. Note that these
 *	offsets are logical offsets relative to the base offset passed
 *	into a prior call to VOP_STABLESTORE(). The logical offsets
 *	are translated into the backing store offsets.
 *
 */
/*ARGSUSED*/
STATIC int
spec_getpagelist(vnode_t *vp, off_t off, uint_t len, page_t *pp,
		  void *bmapp, int flags, cred_t *cr)
{
	off_t offset;
	int err;

	ASSERT(vp->v_type == VBLK);

#ifdef DEBUG
	{ uint_t n;
	  page_t *pp2 = pp;

		for (n = btop(len); n-- != 0;  pp2 = pp2->p_next) {
			ASSERT(PAGE_IS_WRLOCKED(pp2));
		}
		ASSERT(pp2 == pp);
	}
#endif

	offset = off + (off_t)bmapp;

	ASSERT((offset & PAGEOFFSET) == 0);
	ASSERT((len >= PAGESIZE) && ((len & PAGEOFFSET) == 0));
	ASSERT(offset + len <= VTOS(vp)->s_devsize);

	err = spec_getpageio(vp, offset, len, pp, 0);

	return err;
}

/*
 * int
 * spec_putpageio(vnode_t *vp, off_t off, uint_t len, page_t *pp, int flags)
 *
 * Calling/Exit State:
 *	The snode rwlock may or may not be held locked.
 *
 * Description:
 * 	Flags are composed of {B_ASYNC, B_INVAL, B_DONTNEED}.
 */
STATIC int
spec_putpageio(vnode_t *vp, off_t off, uint_t len, page_t *pp, int flags)
{
	struct buf *bp;
	int error = 0;

	ASSERT((off & PAGEOFFSET) == 0);
	ASSERT(pp != NULL);
	ASSERT(!(flags & B_READ));

	bp = pageio_setup(pp, 0, len, B_WRITE | flags);

	bp->b_edev = vp->v_rdev;
	bp->b_blkno = btodt(off);

	(*bdevsw[getmajor(vp->v_rdev)].d_strategy)(bp);

	/* Update the number of dev sized blocks written by this LWP */
	ldladd(&u.u_iow, btodb(len));
#ifdef PERF
	mets_fsinfo[MET_OTHER].pgout++;
	mets_fsinfo[MET_OTHER].pgpgout += btopr(len);
	mets_fsinfo[MET_OTHER].sectout += btodb(len);
#endif

	/*
	 * If async, assume that pvn_done will handle the pages when
	 * I/O is done.
	 */
	if (flags & B_ASYNC)
		return 0;

	/*
	 * Wait for io to complete. biowait() calls pvn_done()
	 * to unlock/abort the pages.
	 */
	error = biowait(bp);

	/* Undo the buf header allocation. */
	pageio_done(bp);


	return error;

}

/*
 * STATIC int
 * spec_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	Snode may or may not be locked on entry.
 *
 * Description:
 *	Flags are composed of {B_ASYNC, B_INVAL, B_DIRTY, B_DONTNEED}.
 *	If len == 0, do from off to EOF.
 *
 *	The normal cases should be
 * 	a. len == 0 & off == 0 (entire vp list),
 *	b. len == MAXBSIZE (from segmap_release actions), and
 *	c. len == PAGESIZE (from pageout).
 */
/* ARGSUSED */
STATIC int
spec_putpage(vnode_t *vp, off_t off, uint_t len, int flags, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	int adj_klustsize;
	off_t kloff, eoff, fsize;
	uint_t kllen;
	STATIC int spec_doputpage(vnode_t *, page_t *, int, cred_t *);

	ASSERT(!(vp->v_flag & VNOMAP));
	ASSERT(vp->v_type == VBLK && sp->s_commonvp == vp);
	ASSERT(off < sp->s_devsize);

#ifdef PERF
	mets_fsinfo[MET_OTHER].putpage++;
#endif

	if (vp->v_pages == NULL)
		return 0;

	if (len == 0) {
		/*
		 * No klustering in the len == 0 case.
		 */
		kloff = off;
		kllen = 0;
	} else {
		if (sp->s_devsize == UNKNOWN_SIZE)
			adj_klustsize = PAGESIZE;
		else
			adj_klustsize = klustsize;

		/*
		 * We set limits so that we kluster to adj_klustsize boundaries.
		 */
		fsize = (sp->s_devsize + PAGEOFFSET) & PAGEMASK;
		eoff = MIN(off + len, fsize);
		kloff = (off / adj_klustsize) * adj_klustsize;
		kllen = roundup(eoff, adj_klustsize) - kloff;
	}

	return pvn_getdirty_range(spec_doputpage, vp, off, len, kloff, kllen,
				  sp->s_devsize, flags, cr);
}

/*
 * STATIC int
 * spec_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
 *	workhorse for spec_putpage
 *
 * Calling/Exit State:
 *	Snode may or may not be locked on entry.
 *
 *	A list of dirty pages, prepared for I/O (in pageout state),
 *	is passed in dirty.  Other parameters are passed through from
 *	spec_putpage.
 */
/* ARGSUSED */
STATIC int
spec_doputpage(vnode_t *vp, page_t *dirty, int flags, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	page_t *pp, *io_list;
	off_t io_off;
	uint_t io_len;
	int err;

	while ((pp = dirty) != NULL) {
		/*
		 * Pull off a contiguous chunk
		 */
		page_sub(&dirty, pp);
		io_list = pp;
		io_off = pp->p_offset;
		io_len = PAGESIZE;
		while (dirty != NULL && dirty->p_offset == io_off + io_len) {
			if (io_len >= klustsize)
				break;
			pp = dirty;
			page_sub(&dirty, pp);
			page_sortadd(&io_list, pp);
			io_len += PAGESIZE;
		}

		/*
                 * Clip down to device size if necessary
                 */
                if (io_off + io_len > sp->s_devsize) {
                        ASSERT((io_off + io_len) - sp->s_devsize < PAGESIZE);
                        io_len = sp->s_devsize - io_off;
                }

		err = spec_putpageio(vp, io_off, io_len, io_list, flags);
		if (err)
			break;
	}

	if (err && dirty != NULL) {
		/*
		 * We encountered an I/O error and not all pages are written.
		 * Call pvn_fail with B_WRITE so that pages that are still
		 * on the io list will be marked modified and that they
		 * will get written back again later.
		 */
		pvn_fail(dirty, B_WRITE | flags);
	}

	return err;
}

/*
 * int
 * spec_putpagelist(vnode_t *vp, off_t off, page_t *pp, void *bmapp,
	int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Write out the data from the circular linked list of pages
 *	pointed to by pp to consecutive logical offset 
 */
/*ARGSUSED*/
STATIC int
spec_putpagelist(vnode_t *vp, off_t off, page_t *pp, void *bmapp,
	int flags, cred_t *cr)
{
	size_t len;
	page_t *pplist;
	off_t offset;

	ASSERT(vp->v_type == VBLK);
	ASSERT(pp != NULL);
	ASSERT((off & PAGEOFFSET) == 0);

	for (len = PAGESIZE, pplist = pp; (pplist = pplist->p_next) != pp;)
		len += PAGESIZE;

	offset = off + (off_t)bmapp;

	return spec_putpageio(vp, offset, len, pp, flags);

}

/*
 * int
 * spec_stablestore(vnode_t **vpp, off_t *off, size_t *len, void **bmapp,
 *			cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Reserve storage associated with the vnode pointed to by *vpp
 *	starting at offset "off" for the length of "len" bytes. *bmapp
 *	points to a backing store mapping data structure which is
 *	filled in by this vnode operation and returned to the caller.
 *	For block special devices, there are no backing store reservation
 *	or mapping issues. This function simply sets the backing store
 * 	data structure to "off".
 *	
 */
/*ARGSUSED*/
STATIC int
spec_stablestore(vnode_t **vpp, off_t *off, size_t *len, void **bmapp,
		  cred_t *cr)
{
	snode_t *sp = VTOS(*vpp);

	ASSERT((*vpp)->v_type == VBLK);
	ASSERT((*off & PAGEOFFSET) == 0);
	ASSERT((*len & PAGEOFFSET) == 0);
	ASSERT(*len != 0);

	/*
	 * For compatibility reasons, we allow block devices with
	 * unknown sizes to bypass the range checks.  In effect,
	 * we assume they are infinite in size.
         */
       if ((sp->s_devsize != UNKNOWN_SIZE) && (*off + *len) > sp->s_devsize)
                return EINVAL;

	/*
	 * Bump up the swap count of common snode and mark
	 * the real snode as not mountable.
	 */
	(VTOS(sp->s_commonvp))->s_swapcnt++;
	(*vpp)->v_flag |= VNOMOUNT;
	*bmapp = (void *)*off;
	return 0;
}

/*
 * int
 * spec_relstore(vnode_t *vp, off_t off, size_t len, void *bmapp,
 *		cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Release storage associated with the vnode pointed to by *vpp
 *	starting at offset "off" for the length of "len" bytes. *bmapp
 *	points to a backing store mapping data structure which is
 *	filled in by this vnode operation and returned to the caller.
 *	
 */
/*ARGSUSED*/
STATIC int
spec_relstore(vnode_t *vp, off_t off, size_t len, void *bmapp,
		  cred_t *cr)
{

	snode_t *sp, *csp;

	ASSERT(bmapp == (void *)off);
	sp = VTOS(vp);
	csp = VTOS(sp->s_commonvp);

	if(--csp->s_swapcnt == 0)
		vp->v_flag &= ~VNOMOUNT;
		
	return 0;
}

/*
 * int
 * spec_poll(vnode_t *vp, int events, int anyyet, short *reventsp,
 *           struct pollhead **phpp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Call the appropriate lower level poll routine.
 */
/* ARGSUSED */
STATIC int
spec_poll(vnode_t *vp, int events, int anyyet, short *reventsp,
	  struct pollhead **phpp)
{
	dev_t dev;
	int error;

	if (vp->v_type == VBLK)
		error = fs_poll(vp, events, anyyet, reventsp, phpp);
	else {
		ASSERT(vp->v_type == VCHR);
		dev = vp->v_rdev;
		if (cdevsw[getmajor(dev)].d_str) {
			ASSERT(vp->v_stream != NULL);
			error = strpoll(vp->v_stream, events, anyyet,
			  reventsp, phpp);
		} else if (cdevsw[getmajor(dev)].d_poll)
			error = (*cdevsw[getmajor(dev)].d_poll)
			  (dev, events, anyyet, reventsp, phpp);
		else
			error = fs_poll(vp, events, anyyet, reventsp, phpp);
	}
	return error;
}

/*
 * int
 * spec_segmap(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp,
 *	 uint_t len, int_t prot, uint_t maxprot, uint_t flags, cred_t *fcred)
 *	Establish a mapping to a device.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Remarks:
 *	Called by spec_map to setup mappings to character devices
 *	which do not provide a d_segmap entry point in cdevsw.
 *
 *	Global so that it can be called from pse driver stubs function.
 */
/* ARGSUSED */
int
spec_segmap(dev_t dev, uint_t off, struct as *as, vaddr_t *addrp, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cred)
{
	struct segdev_crargs dev_a;
	int (*mapfunc)();
	int i, error = 0;

	if ((mapfunc = cdevsw[getmajor(dev)].d_mmap) == nodev)
		return ENODEV;

	if (cdevsw[getmajor(dev)].d_cpu != -1)
		if ((error = bindproc(cdevsw[getmajor(dev)].d_cpu)) != 0)
		       return error;

	/*
	 * Character devices that support the d_mmap
	 * interface can only be mmap'ed shared.
	 */
	if ((flags & MAP_TYPE) != MAP_SHARED)
		return EINVAL;

	/*
	 * Check to ensure that the entire range is
	 * legal and we are not trying to map in
	 * more than the device will let us.
	 */
	for (i = 0; i < len; i += PAGESIZE) {
		if ((*mapfunc)(dev, off + i, maxprot) == (int)NOPAGE)
			return ENXIO;
	}

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * Pick an address w/o worrying about
		 * any vac alignment contraints.
		 */
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL)
			return ENOMEM;
	} else {
		/*
		 * User-specified address; blow away any previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	dev_a.mapfunc = mapfunc;
	dev_a.dev = dev;
	dev_a.offset = off;
	dev_a.prot = (uchar_t)prot;
	dev_a.maxprot = (uchar_t)maxprot;

	if (error = as_map(as, *addrp, len, segdev_create, &dev_a))
	        return error;
	return 0;
}

/*
 * int
 * spec_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, uint_t len,
 *	int_t prot, uint_t maxprot, uint_t flags, cred_t *fcred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Lock the common snode rwlock shared. If it's
 *	a char. device, let the device driver pick the
 *	appropriate segment driver. If it's a block device,
 *	get the as lock in exclusive mode and call as_map()
 *	to set up the mapping.
 *	
 */
/* ARGSUSED */
STATIC int
spec_map(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *fcred)
{
	struct cred *cred = u.u_lwpp->l_cred;
	vnode_t *cvp = VTOS(vp)->s_commonvp;
	snode_t *csp = VTOS(cvp);
	struct segvn_crargs vn_a;
	int error;
	int macmode = 0;

	ASSERT(cvp != NULL);
	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	if (vp->v_type != VCHR && vp->v_type != VBLK)
		return ENODEV;

	SPEC_RWLOCK_RDLOCK(csp);
	/* checks for enhanced security */
	if (mac_installed) {
		if ((maxprot & (PROT_READ|PROT_EXEC)))
			macmode |= VREAD;
		if (maxprot & PROT_WRITE)
			macmode |= VWRITE;
		if (error = spec_saccess(csp, macmode, MAC_ACC|MAC_RW, cred)) {
			SPEC_RWLOCK_UNLOCK(csp);
			return error;
		}
	}

	if (vp->v_type == VCHR) {
		int (*segmap)();
		dev_t dev = vp->v_rdev;

		SPEC_RWLOCK_UNLOCK(csp);

		/*
		 * Character device: let the device driver
		 * pick the appropriate segment driver.
		 */
		segmap = cdevsw[getmajor(dev)].d_segmap;
		if (segmap == nodev) {
			if (cdevsw[getmajor(dev)].d_mmap == nodev)
				return ENODEV;

			/*
			 * For cdevsw[] entries that specify a d_mmap
			 * function but don't have a d_segmap function,
			 * we default to spec_segmap for compatibility.
			 */
			segmap = spec_segmap;
		}

		as_wrlock(as);

		error = (*segmap)(dev, off, as, addrp, len, prot, maxprot,
		    flags, cred);

		as_unlock(as);
		return error;
	}

	/*
	 * Block device, use the underlying commonvp name for pages.
	 */
	if ((int)off < 0 || (int)(off + len) < 0) {
		SPEC_RWLOCK_UNLOCK(csp);
		return EINVAL;
	}

	/*
	 * Don't allow a mapping beyond the last page of the device.
	 */
	if (off + len > ((VTOS(cvp)->s_devsize + PAGEOFFSET) & PAGEMASK)) {
		SPEC_RWLOCK_UNLOCK(csp);
		return ENXIO;
	}

	as_wrlock(as);

	if ((flags & MAP_FIXED) == 0) {
		map_addr(addrp, len, (off_t)off, 0);
		if (*addrp == NULL) {
			as_unlock(as);
			SPEC_RWLOCK_UNLOCK(csp);
			return ENOMEM;
		}
	} else {
		/*
		 * User-specified address; blow away any
		 * previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	vn_a.vp = cvp;
	vn_a.offset = off;
	vn_a.type = flags & MAP_TYPE;
	vn_a.prot = (uchar_t)prot;
	vn_a.maxprot = (uchar_t)maxprot;
	vn_a.cred = cred;

	error = as_map(as, *addrp, len, segvn_create, &vn_a);

	as_unlock(as);
	SPEC_RWLOCK_UNLOCK(csp);
	return error;
}

/*
 * int
 * spec_addmap(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp,
 *	 uint_t len, int_t prot, uint_t maxprot, uint_t flags, cred_t *fcred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Lock the common snode spin lock. Increment the snode
 *	map count.
 *	
 */
/* ARGSUSED */
STATIC int
spec_addmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cred)
{
	snode_t *csp;
	pl_t pl;

	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	csp = VTOS(VTOS(vp)->s_commonvp);
	ASSERT(csp != NULL);

	pl = SPEC_SLOCK(csp);
	csp->s_mapcnt += btopr(len);
	csp->s_count += btopr(len);
	SPEC_SUNLOCK(csp, pl);

	return 0;
}

/*
 * int
 * spec_delmap(vnode_t *vp, off_t off, struct as *as, vaddr_t *addrp,
 *	 uint_t len, int_t prot, uint_t maxprot, uint_t flags, cred_t *fcred)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	Lock the common snode spin lock. Decrement the snode
 *	map count. If the vnode is no longer referenced, lock
 *	the common snode rwlock exclusive and then
 *	call device_close() to close down the device.
 *	
 */
/* ARGSUSED */
STATIC int
spec_delmap(vnode_t *vp, uint_t off, struct as *as, vaddr_t addr, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cred)
{
	snode_t *csp;
	pl_t pl;

	if (vp->v_flag & VNOMAP)
		return ENOSYS;

	csp = VTOS(VTOS(vp)->s_commonvp);
	ASSERT(csp != NULL);

	pl = SPEC_SLOCK(csp);

	ASSERT(csp->s_mapcnt != 0);
	ASSERT(csp->s_count != 0);
	csp->s_mapcnt -= btopr(len);
	csp->s_count -= btopr(len);

	/*
	 * Call the close routine when the last reference of any
	 * kind through any [s,v]node goes away.
	 */
	if (csp->s_count == 0) {
		SPEC_SUNLOCK(csp, pl);
		SPEC_RWLOCK_WRLOCK(csp);
		(void) device_close(vp, 0, cred);
		csp->s_devsize = UNKNOWN_SIZE;
		SPEC_RWLOCK_UNLOCK(csp);
	} else
		SPEC_SUNLOCK(csp, pl);

	return 0;
}

/*
 * Here are all the vnode  operations added for security 
 *
 */

/*
 * int
 * spec_getacl(vnode_t *vp, long nentries, long *dentriesp,
 *	       struct acl *aclbufp, cred_t *cr, int *rvalp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	This operation returns the acls for a block or character special file.
 */
/* ARGSUSED */
STATIC int
spec_getacl(vnode_t *vp, long nentries, long *dentriesp, struct acl *aclbufp,
	cred_t *cr, int *rvalp)
{
	vnode_t *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return VOP_GETACL(realvp, nentries, dentriesp, aclbufp, cr, rvalp);
	else
		return ENOSYS;		
}

/*
 * int
 * spec_setacl(vnode_t *vp, long nentries, long dentries, struct acl *aclbufp,
 * 		cred_t *cr)
 *
 * Calling/Exit State:
 * 	No lock is held on entry or at exit.
 *
 * Description:
 * 	This function sets acls on a block or character special file.
 */
/* ARGSUSED */
STATIC int
spec_setacl(vnode_t *vp, long nentries, long dentries, struct acl *aclbufp,
	cred_t *cr)
{

	vnode_t *realvp;

	if ((realvp = VTOS(vp)->s_realvp) != NULL)
		return VOP_SETACL(realvp, nentries, dentries, aclbufp, cr);
	else
		return ENOSYS;		
}

/*
 * int
 * spec_setlevel(vnode_t *vp, lid_t level, cred_t *cr)
 *	The function sets the level on the specified file.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 * 	This routine will set the level on a block or character special file.
 * 	After passing privilege and mac access checks based on the device
 * 	state, public or private, the level of the vnode is changed before
 * 	calling the file system dependent level. If the file system does not
 *	support labelling, then the operations will fail.
 *	Note that validitiy of the new level is performed in vnode level,
 *	ownership check is done in the depend layer of the real vp.
 */
/* ARGSUSED */
STATIC int
spec_setlevel(vnode_t *vp, lid_t level, cred_t *cr)
{
	snode_t *sp = VTOS(vp);
	int error = 0;
	vnode_t *realvp;
	vnode_t *cvp = VTOS(vp)->s_commonvp;
	snode_t *csp = VTOS(cvp);
	

	/*
	 * If MAC is not installed, set the level on the real
	 * (SFS) file.
	 */
	if (mac_installed == 0) {
		if ((realvp = (sp)->s_realvp) != NULL)
			error = VOP_SETLEVEL(realvp, level, cr);
		return error;
	}

	SPEC_RWLOCK_WRLOCK(csp);
	ASSERT((STATE(sp)== DEV_PUBLIC) || (STATE(sp) == DEV_PRIVATE));

	switch (STATE(sp)) {
	case DEV_PUBLIC: 
		if ((error = MAC_VACCESS(vp, VWRITE, cr)) != 0) 
			goto out;

                if (pm_denied(cr, P_SETFLEVEL)) {
                    if ((MAC_ACCESS(MACDOM, level, vp->v_lid) == 0) &&
		        (pm_denied(cr, P_MACUPGRADE))){
				error = EPERM;
				goto out;
			}
		}
		break;

	case DEV_PRIVATE:
		if (error = pm_denied(cr, P_DEV)){
			error = EPERM;
			goto out;
		}
		break;
	}

	/* device has range, new level must be enclosed by device range */
	if ((REL_FLAG(sp) != DEV_SYSTEM) &&
            (MAC_ACCESS(MACDOM, HI_LEVEL(sp), level) ||
             (MAC_ACCESS(MACDOM, level, LO_LEVEL(sp))))) {
		error = ERANGE;
		goto out;
	}
	realvp = sp->s_realvp;
	if (MODE(sp) != DEV_STATIC || STATE(sp) == DEV_PRIVATE) {
		if (!realvp || (!(error = VOP_SETLEVEL(realvp, level, cr)))) { 
			vp->v_lid = level;
			cvp->v_lid = level;
		} 
	} else {
		if (csp->s_mapcnt)
			error = EBUSY;
		else if (realvp == NULL) {
			/* clone device, no disk vnode */
			VN_LOCK(vp);
			if ((vp->v_count - vp->v_softcnt) == 1) {
				vp->v_lid = level;
				cvp->v_lid = level;
			} else
				error = EBUSY;
			VN_UNLOCK(vp);
		} else {
			lid_t	lid;
			/* There is real disk vnode. Check the count 
			 * without lock first for the performance reason.
			 * If count is greater than one fail; otherwise,
			 * call real file system to change the lid. If
			 * successful, check the count under the lock and
			 * if count is not one reset the old lid back on
			 * the real file system using sys_cred to avoid
			 * any failure. 
			 */
			if ((vp->v_count - vp->v_softcnt) == 1) {
				lid = vp->v_lid;
				if (error = VOP_SETLEVEL(realvp, level, cr))  
					goto out;
				VN_LOCK(vp);
				if ((vp->v_count - vp->v_softcnt) == 1) {
					vp->v_lid = level;
					cvp->v_lid = level;
					VN_UNLOCK(vp);
				} else { 
					VN_UNLOCK(vp);
					(void)VOP_SETLEVEL(realvp, lid, 
							   sys_cred);
					error = EBUSY;
				}
			} else
				error = EBUSY;
		}
	}
out:
	SPEC_RWLOCK_UNLOCK(csp);
	return error;
}

/*
 * int
 * spec_getdvstat(vnode_t *vp, struct devstat *bufp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	This routine will return the security attributes of a device 
 * 	the snode is locked to return consistent info and prevent 
 * 	race condition betwen an ongoing devstat set or level change
 */
/* ARGSUSED */
STATIC int
spec_getdvstat(vnode_t *vp, struct devstat *bufp, cred_t *cr)
{
	snode_t *sp= VTOS(vp);
	snode_t *csp = VTOS(VTOS(vp)->s_commonvp);

	SPEC_RWLOCK_RDLOCK(csp);
	bufp->dev_relflag = REL_FLAG(sp);
	bufp->dev_state = STATE(sp);
	bufp->dev_mode = MODE(sp);
	bufp->dev_hilevel = HI_LEVEL(sp);
	bufp->dev_lolevel = LO_LEVEL(sp);
	bufp->dev_usecount = (csp->s_count ? 1 : 0);
	SPEC_RWLOCK_UNLOCK(csp);
	return 0;
}

/*
 * int
 * spec_setdvstat(vnode_t *vp, struct devstat *bufp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 * 	This routine will set the security attributes of a device
 * 	there are three options for setting these attributes
 * 	DEV_SYSTEM: releases the current attr and resets them to system defaults
 * 	DEV_LASTCLOSE/DEV_PERSISTENT: sets the security attributes on a snode.
 * 	Since these attributes must survive across system calls, the snode and
 * 	its common are kept incore by increasing reference counts on the vnode 
 * 	and the vnode of the common snode.
 * 	When invoking this operation with dev_system flag, if a security
 * 	attribute structure was allocated, it is released
 * 	and the reference count on the snode and its common is decremented.
 *
 * 	The common snode s_rwlock is acquire in exclusive mode by this
 *	routine and release before exit.
 */
/* ARGSUSED */
STATIC int
spec_setdvstat(vnode_t *vp, struct devstat *bufp, cred_t *cr)
{
	int error = 0;
	snode_t *sp = VTOS(vp);
	snode_t *csp = VTOS(sp->s_commonvp);


	switch (bufp->dev_relflag) {
	case DEV_LASTCLOSE:
	case DEV_PERSISTENT:
		{
		/* 
		 * Need to check for valid lids being passed 
		 * Hi level must dominate lo level
		 * mode must be valid : static or dynamic
		 * state must be valid: static or dynamic
		 * current device level must be within new hi and new lo
		 * If all checks pass, if data structure exists;
		 * straight copy, otherwise new structure is allocated
		 * before copying info in kernel
		 * privilege check done at fs indep level
		 */
		if ((bufp->dev_mode != DEV_STATIC ) && 
	         	(bufp->dev_mode != DEV_DYNAMIC))
			return EINVAL;
		if ((bufp->dev_state != DEV_PUBLIC) && 
			(bufp->dev_state != DEV_PRIVATE))
			return EINVAL;
			
                /*
                 * The level range must be validated as well.
                 * Validating one level is sufficient.
                 */
		if (mac_valid(bufp->dev_hilevel) || 
                    MAC_ACCESS(MACDOM, bufp->dev_hilevel, bufp->dev_lolevel))
                        return EINVAL;

                if (MAC_ACCESS(MACDOM, bufp->dev_hilevel, vp->v_lid) ||
                    MAC_ACCESS(MACDOM, vp->v_lid, bufp->dev_lolevel)){
			return EINVAL;
		}
		SPEC_RWLOCK_WRLOCK(csp);
		if (sp->s_dsecp == NULL)
			devsec_dcicreat(sp);
		sp->s_dstate = bufp->dev_state;
		sp->s_dmode = bufp->dev_mode;
		sp->s_dsecp->d_hilid = bufp->dev_hilevel;
		sp->s_dsecp->d_lolid = bufp->dev_lolevel;
		sp->s_dsecp->d_relflag = bufp->dev_relflag;
		csp->s_dstate = bufp->dev_state;
		csp->s_dmode = bufp->dev_mode;
		SPEC_RWLOCK_UNLOCK(csp);
		break;
		}
	case DEV_SYSTEM: {
		/* release security attributes to system setting */
		SPEC_RWLOCK_WRLOCK(csp);
		if ((sp)->s_dsecp != NULL)
			devsec_dcifree(sp);
		SPEC_RWLOCK_UNLOCK(csp);
		break;
		}
	default:
		error = EINVAL;
		break;
	}
	return error;
}

/*
 * int
 * spec_msgio(vnode_t *vp, struct strbuf *mctl, struct strbuf *mdata,
 *		int mode, unsigned char *prip, int fmode, int *flagsp,
 *		rval_t *rvp, cred_t *cr)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	This routine will call strputmsg/strgetmsg if the
 *	device is streaming or the char device entry
 *	routine d_msgio() for a non-streams char device.
 */
/* ARGSUSED */
STATIC int
spec_msgio(vnode_t *vp, struct strbuf *mctl, struct strbuf *mdata,
		int mode, unsigned char *prip, int fmode, int *flagsp,
		rval_t *rvp, cred_t *fcr)
{
	cred_t *cr = VCURRENTCRED(fcr);
	dev_t dev;
	int error;

	ASSERT(vp->v_type == VCHR);

	/*
	 * security checks if device is private or dynamic
	 */
	if (mac_installed &&
	    (error = spec_access(vp, ((mode == FREAD)? VREAD : VWRITE), 
						MAC_ACC|MAC_RW, cr)))
		return error;
#ifdef CC_PARTIAL
	/*
	 * CCA assumes MAC read(write) access is assured by above code,
	 * this implies MAC dominates(equality).
	 */
	MAC_ASSERT (sp, ((mode == FREAD)? MAC_DOMINATES : MAC_SAME));
#endif

	/* 
	 * update access time depending on mode and whether write is allowed.
	 * for a FWRITE, the spec_access already perfomed the write check
	 * (see spec_write for more detail) so we don't have to do any
	 * checking. for FREAD we must see if write is allowed.
	 */

	if (mode == FWRITE) {
		smark(VTOS(vp), SUPD|SCHG);
	} else if (vp->v_vfsp && WRITEALLOWED(vp, cr)) {
		smark(VTOS(vp), SACC);
	}

	dev = vp->v_rdev;
	if (cdevsw[getmajor(dev)].d_str) {
		if (mode == FREAD)
			return strgetmsg(vp, mctl, mdata, prip, flagsp, 
					fmode, U_TO_K, rvp);
		return strputmsg(vp, mctl, mdata, *prip, *flagsp, fmode, 
				U_TO_K, cr);
	}
	/* msgio allowed on non-streams char devices */
	return (*cdevsw[getmajor(dev)].d_msgio)(dev, mctl, mdata, mode, 
					prip, fmode, flagsp, rvp, cr);

}
