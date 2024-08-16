/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/specfs/specsubr.c	1.43"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <acc/priv/privilege.h>
#include <fs/buf.h>
#include <fs/fcntl.h>
#include <fs/file.h>
#include <fs/specfs/snode.h>
#include <fs/specfs/specdata.h>
#include <fs/specfs/spechier.h>
#include <fs/specfs/devmac.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/open.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/termios.h>
#include <io/uio.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>

#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#include <acc/mac/mac.h>
#endif

STATIC snode_t *sfind(dev_t, vtype_t, vnode_t *);
STATIC vnode_t *commonvp(dev_t, vtype_t);
int spec_saccess(snode_t *, int, int, cred_t *);
STATIC snode_t *sinsert(snode_t *);
int specpreval(vtype_t, dev_t, cred_t *);

extern vnode_t *fifovp(vnode_t *, cred_t *);
extern vnode_t *xnamvp(vnode_t *, cred_t *);
extern int fifopreval(vtype_t, dev_t, cred_t *);
extern int xnampreval(vtype_t, dev_t, cred_t *);
extern struct vfsops spec_vfsops;
extern int strclose(vnode_t *, int , cred_t *);
extern int strioctl(vnode_t *, int, int, int, int, cred_t *, int *);

/*
 * vnode_t *
 * specvp(vnode_t *vp, dev_t dev, vtype_t type, cred_t *cr)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Return a shadow special vnode for the given dev.
 *	If no snode exists for this dev create one and put it
 *	in a table hashed by <dev,realvp>.  If the snode for
 *	this dev is already in the table return it (ref count is
 *	incremented by sfind).  The snode will be flushed from the
 *	table when spec_inactive calls sdelete.
 *	The fsid is inherited from the real vnode so that clones
 *	can be found.
 *
 */
vnode_t *
specvp(vnode_t *vp, dev_t dev, vtype_t type, cred_t *cr)
{
	snode_t *sp;
	vnode_t *svp;
	snode_t *nsp;
	struct vattr va;
	pl_t pl;

	if (vp == NULL)
		return NULL;
	if (vp->v_type == VFIFO)
		return fifovp(vp, cr);
	if (vp->v_type == VXNAM)
		return xnamvp(vp, cr);

	ASSERT(vp->v_type == type);
	ASSERT(vp->v_rdev == dev);

	if ((sp = sfind(dev, type, vp)) != NULL)
		return STOV(sp);

#ifdef CC_PARTIAL
	/*
	 * Strictly speaking, this MAC_ASSUME is incorrect.
	 * No MAC check has been done on this vnode.  However,
	 * this ensures that the CCA tool understands that
	 * *vp and *sp are at the same level, and the later
	 * MAC_UNKLEV(sp) ensures that the level relationship
	 * to *sp is correct.
	 */
	MAC_ASSUME(vp, MAC_SAME);
#endif

	sp = (snode_t *)kmem_zalloc(sizeof(*sp), KM_SLEEP);
	STOV(sp)->v_op = &spec_vnodeops;

	/*
	 * Init the times in the snode to those in the vnode.
	 */
	va.va_mask = AT_TIMES|AT_FSID;
	if (VOP_GETATTR(vp, &va, 0, cr) == 0) {
		sp->s_atime = va.va_atime.tv_sec;
		sp->s_mtime = va.va_mtime.tv_sec;
		sp->s_ctime = va.va_ctime.tv_sec;
		sp->s_fsid = va.va_fsid;
	} else
		sp->s_fsid = specdev;
	sp->s_realvp = vp;
	sp->s_dev = dev;
	svp = STOV(sp);
	SPEC_SLOCK_INIT(sp);
	SPEC_RWLOCK_INIT(sp);
	VN_INIT(svp, vp->v_vfsp, type, dev, 0, KM_SLEEP);
	svp->v_data = sp;

	/* 
	 * Set the level on the vnode.  Do this whether or not
	 * mac_installed is set, since you should be able to see the
	 * level on a file even if the kernel is not enforcing MAC
	 * access restritions.
	 * If the file system does not support labeling,
	 * vnode will be labelled with the level floor of
	 * of the mounted file system if vfsp is NON_NULL.
	 */
	if (sp->s_realvp->v_macflag & VMAC_SUPPORT)
		svp->v_lid = sp->s_realvp->v_lid;
	else {
		if  (sp->s_realvp->v_vfsp)
			svp->v_lid = sp->s_realvp->v_vfsp->vfs_macfloor;
	}
			
	/* initialization for security */
	if (mac_installed) {
		switch (svp->v_type) {

		case VCHR:
			if ((cdevcnt > getmajor(dev)) &&
				(cdevsw[getmajor(dev)].d_flag))
					sp->s_secflag = 
					        (*cdevsw[getmajor(dev)].d_flag) 
						& SECMASK;			
			else 
				sp->s_secflag = 0;
			break;	
		case VBLK:
			if ((bdevcnt > getmajor(dev)) &&
				(bdevsw[getmajor(dev)].d_flag))
					sp->s_secflag = 
						(*bdevsw[getmajor(dev)].d_flag)
						& SECMASK;			
			else 
				sp->s_secflag = 0;
			break;
		}
		sp->s_dstate = ((sp->s_secflag & D_INITPUB) ? 
				 DEV_PUBLIC : DEV_PRIVATE);
		sp->s_dmode = DEV_STATIC;
		sp->s_dsecp = NULL;
	}

	/* Assign node id. */
	pl = LOCK(&snode_id_mutex, FS_SNOIDPL);
	sp->s_nodeid = spec_lastnodeid++;
	UNLOCK(&snode_id_mutex, pl);

	if (type == VBLK || type == VCHR) {
		sp->s_commonvp = commonvp(dev, type);
		if (mac_installed && sp->s_commonvp->v_lid == 0) {
			/* it means that the common vnode was not initialized */
			sp->s_commonvp->v_lid = svp->v_lid;
		}
		svp->v_stream = sp->s_commonvp->v_stream;
		sp->s_devsize = UNKNOWN_SIZE;
	}

	svp->v_macflag |= (VMAC_DOPEN | VMAC_SUPPORT);

	nsp = sinsert(sp);
	if (nsp != (snode_t *)NULL) {
		/*
		 * Lost the race in creating an snode to
		 * some other LWP. We need to destroy ours.
		 */
		if (type == VBLK || type == VCHR)
			VN_RELE(sp->s_commonvp);
		VN_DEINIT(svp);
		SPEC_RWLOCK_DEINIT(sp);
		SPEC_SLOCK_DEINIT(sp);
		kmem_free(sp, sizeof (*sp));
		sp = nsp;
	} else {
		/* Need another hold on the realvp. */
		VN_HOLD(vp);
	}

	return STOV(sp);
}

/*
 * int
 * specpreval(vtype_t, dev_t, cred_t *cr)
 *      Pre_validate attributes.
 * Calling/Exit State:
 *       No locks are held on entry or exit.
 *
 * Description:
 *      Try to pre-validate attributes that will be passed to a subsequent
 *      specvp() call.  This function is not required to catch all possible
 *      error cases for specvp(), but we catch as many as we can.
 */
int
specpreval(vtype_t type, dev_t dev, cred_t *cr)
{
	if (type == VFIFO)
		return fifopreval(type, dev, cr);

	if (type == VXNAM)
		return xnampreval(type, dev, cr);

	return 0;
}

/*
 * vnode_t *
 * makespecvp(dev_t dev, vtype_t type)
 *
 * Calling/Exit State:
 *	This routine may block, so no locks may be held on entry,
 *	and none are held at exit.
 *
 * Description:
 *	Return a special vnode for the given dev; no vnode is supplied
 *	for it to shadow.  Usually callers of makespecvp expects a
 *	new snode created. However, there is one exception. Clone open
 *	could race with normal open. When clone open tries to establish
 *	an snode for the new device, a racing normal open may have
 *	already established one via lookup/specvp. sinsert() detects
 *	this case and returns the existing vnode so we can throw away
 *	the one we created.
 */
vnode_t *
makespecvp(dev_t dev, vtype_t type)
{
	snode_t *sp;
	snode_t *nsp;
	vnode_t *svp;
	time_t t;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	sp = (snode_t *)kmem_zalloc(sizeof(*sp), KM_SLEEP);
	svp = STOV(sp);
	SPEC_SLOCK_INIT(sp);
	SPEC_RWLOCK_INIT(sp);
	VN_INIT(svp, NULL, type, dev, 0, KM_SLEEP);
	svp->v_op = &spec_vnodeops;
	svp->v_data = sp;
	t = hrestime.tv_sec;
	sp->s_atime = t;
	sp->s_mtime = t;
	sp->s_ctime = t;
	sp->s_fsid = specdev;
	sp->s_realvp = NULL;
	sp->s_dev = dev;

	sp->s_commonvp = commonvp(dev, type);
	svp->v_stream = sp->s_commonvp->v_stream;
	sp->s_devsize = UNKNOWN_SIZE;
	(void) LOCK(&snode_id_mutex, FS_SNOIDPL);
	sp->s_nodeid = spec_lastnodeid++;
	UNLOCK(&snode_id_mutex, PLBASE);

	/* initialization for security */
	if (mac_installed) {
		switch (svp->v_type) {
		case VCHR:
			if ((cdevcnt > getmajor(dev)) &&
				(cdevsw[getmajor(dev)].d_flag))
					sp->s_secflag = 
						(*cdevsw[getmajor(dev)].d_flag) 
						& SECMASK;			
			else 
				sp->s_secflag = 0;
			break;	
		case VBLK:
			if ((bdevcnt > getmajor(dev)) &&
				(bdevsw[getmajor(dev)].d_flag))
					sp->s_secflag = 
						(*bdevsw[getmajor(dev)].d_flag) 
						& SECMASK;			
			else 
				sp->s_secflag = 0;
			break;
		}
		/*
		 * state is initialized to DEV_PUBLIC 
		 * needed for bfs support for unprivileged I/O
		 */
		sp->s_dstate = DEV_PUBLIC; 
		sp->s_dmode = DEV_STATIC;
		sp->s_dsecp = NULL;
		svp->v_lid = CRED()->cr_lid;
		svp->v_macflag |= (VMAC_DOPEN | VMAC_SUPPORT);
		/* 
		 * The LID in the commonvp is populated because /dev/tty
		 * performs access check on the commonvp.
		 */
		if (sp->s_commonvp->v_lid == 0)
			sp->s_commonvp->v_lid = CRED()->cr_lid;  

	}
	nsp = sinsert(sp);
	if (nsp != (snode_t *)NULL) {
		/*
		 * Lost the race to some other LWP.
		 * Destroy our snode.
		 */
		VN_RELE(sp->s_commonvp);
		VN_DEINIT(svp);
		SPEC_RWLOCK_DEINIT(sp);
		SPEC_SLOCK_DEINIT(sp);
		kmem_free(sp, sizeof (*sp));
		sp = nsp;
	}

	return STOV(sp);
}

/*
 * vnode_t *
 * specfind(dev_t dev, vtype_t type)
 *
 * Calling/Exit State:
 *	This routine may block, so no locks may be held on entry,
 *	and none are held at exit.
 *
 * Description:
 *	Find a special vnode that refers to the given device
 *	of the given type.  Never return a "common" vnode.
 *	Return NULL if a special vnode does not exist.
 *	HOLD the vnode before returning it.
 */
vnode_t *
specfind(dev_t dev, vtype_t type)
{
	snode_t *st;
	vnode_t *nvp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	(void) LOCK(&spec_table_mutex, FS_SPTBLPL);
	st = spectable[SPECTBHASH(dev)];
	while (st != NULL) {
		if (st->s_dev == dev) {
			nvp = STOV(st);
			if (nvp->v_type == type && st->s_commonvp != nvp) {
				/*
				 * Queue for the snode rwlock while
				 * dropping the table lock atomically.
				 * This prevents the snode from being
				 * deleted.
				 */
				VN_HOLD(nvp);
				UNLOCK(&spec_table_mutex, PLBASE);
				return nvp;
			}
		}
		st = st->s_next;
	}
	UNLOCK(&spec_table_mutex, PLBASE);
	return NULL;
}

/*
 * vnode_t *
 * common_specvp(vnode_t *vp)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Given a device vnode, return the common vnode associated with it.
 *	Since the common vnode associated with an snode is invariant,
 *	no locking is necessary.
 *
 */
vnode_t *
common_specvp(vnode_t *vp)
{
	snode_t *sp;

	if ((vp->v_type != VBLK) && (vp->v_type != VCHR) || 
	  vp->v_op != &spec_vnodeops)
		return vp;
	sp = VTOS(vp);
	return sp->s_commonvp;
}

/*
 * STATIC vnode_t *
 * commonvp(dev_t dev, vtype_t type)
 *
 * Calling/Exit State:
 *    No lock is held on entry or at exit.
 *
 * Description:
 *    Returns a special vnode for the given dev.  The vnode is the
 *    one which is "common" to all the snodes which represent the
 *    same device.  For use ONLY by SPECFS.
 *
 */
STATIC vnode_t *
commonvp(dev_t dev, vtype_t type)
{
	snode_t *sp;
	vnode_t *svp;
	snode_t *nsp;

	if ((sp = sfind(dev, type, NULL)) != NULL)
		return STOV(sp);

	sp = (snode_t *)kmem_zalloc(sizeof(*sp), KM_SLEEP);
	STOV(sp)->v_op = &spec_vnodeops;
	sp->s_realvp = NULL;
	sp->s_dev = dev;
	sp->s_fsid = specdev;
	svp = STOV(sp);
	VN_INIT(svp, NULL, type, dev, 0, KM_SLEEP);
	SPEC_SLOCK_INIT(sp);
	SPEC_RWLOCK_INIT(sp);
	svp->v_data = sp;
	sp->s_commonvp = STOV(sp); /* points to itself */
	sp->s_devsize = UNKNOWN_SIZE;

	/* initialization for security */
	if (mac_installed) {
		switch (svp->v_type) {
		case VCHR:
			if ((cdevcnt > getmajor(dev)) &&
				(cdevsw[getmajor(dev)].d_flag))
					sp->s_secflag = 
						(*cdevsw[getmajor(dev)].d_flag) 
						& SECMASK;			
			else 
				sp->s_secflag = 0;
			break;	
		case VBLK:
			if ((bdevcnt > getmajor(dev)) &&
				(bdevsw[getmajor(dev)].d_flag))
					sp->s_secflag = 
						(*bdevsw[getmajor(dev)].d_flag) 
						& SECMASK;			
			else 
				sp->s_secflag = 0;
			break;
		}
		sp->s_dstate = ((sp->s_secflag & D_INITPUB) ? 
				DEV_PUBLIC : DEV_PRIVATE);
		sp->s_dmode = DEV_STATIC;
		sp->s_dsecp = NULL;
		svp->v_macflag |= (VMAC_DOPEN | VMAC_SUPPORT);
	}
	nsp = sinsert(sp);
	if (nsp != (snode_t *)NULL) {
		/*
		 * Lost the race to some other LWP.
		 * Destroy our snode.
		 */
		VN_DEINIT(svp);
		SPEC_RWLOCK_DEINIT(sp);
		SPEC_SLOCK_DEINIT(sp);
		kmem_free(sp, sizeof (*sp));
		sp = nsp;
	}

	return STOV(sp);
}

/*
 * Snode lookup stuff.
 * These routines maintain a table of snodes hashed by dev so
 * that the snode for an dev can be found if it already exists.
 */

/*
 * STATIC snode_t *
 * sinsert(snode_t *sp)
 *	Enter an snode into the snode table by <dev, type, vp>,
 *	where dev, type, and vp are derived from the snode, sp.
 *
 * Calling/Exit State:
 *	While it is not the typical case, a race might have occurred,
 *	and an snode of the same identity might already exist in the table.
 *	If an existing snode is found, an additional VN_HOLD is placed
 *	on it, and it is returned; otherwise NULL is returned.
 *
 *	This routine may block, so no locks may be held on entry,
 *	and none are held at exit.
 *
 * Description:
 *	Obtain the hash table lock before traversing down the hash chain.
 *	If we find one that matches the one we are adding to the chain,
 *	someone raced and we lost. Atomically queue for the snode rwlock
 *	while dropping the hash table lock. This prevents the snode from
 *	being removed in sdelete. While holding the snode rwlock,
 *	establish a reference to the vnode.
 */
STATIC snode_t *
sinsert(snode_t *sp)
{
	snode_t *tst;
	vnode_t *vp, *tvp;
	dev_t dev;
	vtype_t type;
	boolean_t common;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	vp = sp->s_realvp;
	dev = sp->s_dev;
	type = STOV(sp)->v_type;
	common = (sp->s_commonvp == STOV(sp));

loop:
	/* lock hash table */
	(void) LOCK(&spec_table_mutex, FS_SPTBLPL);
	tst = spectable[SPECTBHASH(dev)];
	while (tst != NULL) {
		if (tst->s_dev == dev) {
			tvp = STOV(tst);
			if (tvp->v_type == type &&
			    VN_CMP(tst->s_realvp, vp) &&
			    (tst->s_commonvp == tvp) == common) {
				if (tst->s_flag & SINVALID) {
				    SV_WAIT(&snode_sv, PRINOD,
						&spec_table_mutex);
				    goto loop;
				}
				VN_HOLD(tvp);
				UNLOCK(&spec_table_mutex, PLBASE);
				return tst;
			}
		}
		tst = tst->s_next;
	}
	sp->s_next = spectable[SPECTBHASH(dev)];
	spectable[SPECTBHASH(dev)] = sp;
	UNLOCK(&spec_table_mutex, PLBASE);
	return (snode_t *)NULL;
}

/*
 * void
 * sdelete(snode_t *sp)
 *
 * Calling/Exit State:
 *	The snode hash table lock is locked on entry and at exit.
 *
 * Description:
 *	Remove an snode from the hash table.
 *	The realvp is not released here because spec_inactive() still
 *	needs it to do a spec_fsync().
 */
void
sdelete(snode_t *sp)
{
	snode_t *st;
	snode_t *stprev = NULL;

	st = spectable[SPECTBHASH(sp->s_dev)];
	while (st != NULL) {
		if (st == sp) {
			if (stprev == NULL)
				spectable[SPECTBHASH(sp->s_dev)] = st->s_next;
			else
				stprev->s_next = st->s_next;
			break;
		}
		stprev = st;
		st = st->s_next;
	}
	return;
}

/*
 * STATIC snode_t *
 * sfind(dev_t dev, vtype_t type, vnode_t *vp)
 *
 * Calling/Exit State:
 *	This routine may block, so no locks may be held on entry,
 *	and none are held at exit.
 *
 * Description:
 *	Lookup an snode by <dev, type, vp>.
 *	If (vp == NULL) matches only common snodes;
 *	else matches only non-common snodes with s_realvp equal to vp.
 */
STATIC snode_t *
sfind(dev_t dev, vtype_t type, vnode_t *vp)
{
	snode_t *st;
	vnode_t *svp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

loop:
	(void) LOCK(&spec_table_mutex, FS_SPTBLPL);
	st = spectable[SPECTBHASH(dev)];
	while (st != NULL) {
		svp = STOV(st);
		if (st->s_dev == dev && svp->v_type == type &&
		    VN_CMP(st->s_realvp, vp) &&
		    (vp != NULL || st->s_commonvp == svp)) {
			if (st->s_flag & SINVALID) {
			    SV_WAIT(&snode_sv, PRINOD, &spec_table_mutex);
			    goto loop;
			}
			VN_HOLD(svp);
			UNLOCK(&spec_table_mutex, PLBASE);
			return st;
		}
		st = st->s_next;
	}
	UNLOCK(&spec_table_mutex, PLBASE);
	return NULL;
}

/*
 * int
 * spec_saccess(snode_t *sp, int mode, int flags, cred_t *cr)
 *
 * Calling/Exit State:
 *	The snode rwlock is locked shared on entry and remains locked at exit.
 *
 * Description:
 * 	This is the central routine that will perform mandatory and
 * 	discretionary access checks on block or character special files.
 *	The additional field has been used to encode what kind of access
 *      is to be done:
 *
 * 	DAC_ACC - require discretionary access checks
 * 	MAC_ACC - require mandatory access checks
 * 	MAC_RW/MAC_IO | MAC_ACC  - require mandatory access checks for
 * 		read,write or ioctl type operations. 
 *
 * 	For read,write/ioctl type operations, mandatory access checks are
 * 	only performed for dynamic devices.
 *	The only differnce then between MAC_RW and MAC_IO is that a different
 *	error return is used when there is no access permission.
 */
/* ARGSUSED */
int
spec_saccess(snode_t *sp, int mode, int flags, cred_t *cr)
{
	int macmode = mode;

	if (flags == 0)
		flags = DAC_ACC;
	if (flags & MAC_ACC) {
		/* Device in PRIVATE state requires P_DEV privilege */
		if (STATE(sp) == DEV_PRIVATE) {
			if (pm_denied(cr, P_DEV)) {
				if (flags & MAC_RW)
					return EIO;
				else	
					return EPERM;
			}
		}
		/* 
		 * D_NOSPECMACDATA denotes that no MAC checks be perfromed 
		 * The flag is set for /dev/null and other public type devices
		 * such as /dev/tty and /dev/sad to treat them as 
		 * special devices and bypass MAC checks..
		 */
		if (sp->s_secflag & D_NOSPECMACDATA)
			goto out;
		if (sp->s_secflag & D_RDWEQ)
			macmode = VWRITE;
		/*
		 * read/write/io type access checks only need access checks
		 * on dynamic type devices. On static devices, an access 
		 * check was made on open and from that point the lid
		 * cannot change, making access checks in read/write/ioctl
		 * unneeded.
		 */
		if (flags & (MAC_RW | MAC_IO)) {
			if ((MODE(sp) == DEV_DYNAMIC) &&
			    (MAC_VACCESS(STOV(sp), macmode, cr))) {
				/*
				* ioctl needed a different error return 
				* than read/write
				*/
				if (flags & MAC_IO)
					return EACCES;
				else 
					return EBADF;
			}
		/*
		 * non read/write/ioctl (e.g. open) always get access
		 * checks.
		 */
		} else if (MAC_VACCESS(STOV(sp), macmode, cr))
			return EACCES;
	}
out:
	if ((flags & DAC_ACC) && sp->s_realvp)
		return VOP_ACCESS(sp->s_realvp, mode, flags, cr);
	return 0;
}
/*
 * void
 * smark(snode_t *sp, int flag)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Mark the accessed, updated, or changed times in an snode
 *	with the current time.
 */
void
smark(snode_t *sp, int flag)
{
	time_t t = hrestime.tv_sec;
	pl_t pl;

	/* added for enhanced security */
	if ((mac_installed) && (sp->s_secflag & D_NOSPECMACDATA))
		return;
	pl = SPEC_SLOCK(sp);
	sp->s_flag |= flag;
	if (flag & SACC)
		sp->s_atime = t;
	if (flag & SUPD)
		sp->s_mtime = t;
	if (flag & SCHG)
		sp->s_ctime = t;
	SPEC_SUNLOCK(sp, pl);
	return;
}

/*
 * void
 * specinit(vfssw_t *vswp, int fstype)
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Called by main at boot time to initialize specfs.
 */
void
specinit(vfssw_t *vswp, int fstype)
{
	int i;

	LOCK_INIT(&spec_table_mutex, FS_SPTBLHIER, FS_SPTBLPL,
					&spec_table_lkinfo, KM_SLEEP);
	LOCK_INIT(&snode_id_mutex, FS_SNOIDHIER, FS_SNOIDPL, &snode_id_lkinfo,
					KM_SLEEP);
	SLEEP_INIT(&spec_updlock, (uchar_t)40, &spec_updlock_lkinfo,
							KM_SLEEP);
	SV_INIT(&snode_sv);

	/*
	 * Associate vfs and vnode operations.
	 */
	vswp->vsw_vfsops = &spec_vfsops;
	specfstype = fstype;
	if ((specdev = getudev()) == NODEV)
		specdev = 0;
	for (i = 0; i < SPECTBSIZE; i++) {
                spectable[i] = NULL;
        }

	return;
}

/* XENIX Support */
/*
 * int
 * spec_rdchk(vnode_t *vp, cred_t *cr, int *rvalp)
 *	Support for XENIX rdchk() system call.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Check if any data to be read in the special file.
 */
/* ARGSUSED */
int
spec_rdchk(vnode_t *vp, cred_t *cr, int *rvalp)
{
	dev_t	dev;
	int	error;

	if (vp->v_type != VCHR || vp->v_op != &spec_vnodeops)
		return (ENOTTY);
	dev = VTOS(vp)->s_dev;
	if (cdevsw[getmajor(dev)].d_str) {
		error = strioctl(vp, FIORDCHK, 0, 0, K_TO_K, cr, rvalp);
	} else {
		error = (*cdevsw[getmajor(dev)].d_ioctl)(dev, FIORDCHK,
			 0, 0, cr, rvalp);
	}
	return (error);
}
/* End XENIX Support */

/*
 * void
 * spec_flush()
 *
 * Calling/Exit State:
 *	The specfs update synchronization lock (spec_updlock) is
 *	held locked on entry and remains locked at exit.
 *
 * Description:
 *	This routine is called from spec_sync, while traversing the hash list,
 *	the routine can go to sleep, it drops the hash table lock before
 *	going to sleep.
 */
void
spec_flush()
{
	snode_t **spp, *sp;
	vnode_t *vp, *pvp = NULL;
	pl_t pl;
	
	pl = LOCK(&spec_table_mutex, FS_SPTBLPL);
	for (spp = spectable; spp < &spectable[SPECTBSIZE]; spp++) {
		pvp = NULL;
loop:
		for (sp = *spp; sp != NULL; sp = sp->s_next) {
			vp = STOV(sp);
			/*
			 * Don't bother sync'ing a vp if it's
			 * part of a virtual swap device.
			 */
			if (sp->s_swapcnt > 0)
				continue;
			if (vp->v_type == VBLK && vp->v_pages) {
				if (sp->s_flag & SINVALID) {
					SV_WAIT(&snode_sv, PRINOD,
							&spec_table_mutex);
					pl = LOCK(&spec_table_mutex, FS_SPTBLPL);
					goto loop;
				}
				VN_HOLD(vp);
				UNLOCK(&spec_table_mutex, pl);
				if (pvp)
					VN_RELE(pvp);
				pvp = vp;
				(void) VOP_PUTPAGE(vp, 0, 0, B_ASYNC, sys_cred);
				pl = LOCK(&spec_table_mutex, FS_SPTBLPL);
			}
		}
		if (pvp) {
			UNLOCK(&spec_table_mutex, pl);
			VN_RELE(pvp);
			pl = LOCK(&spec_table_mutex, FS_SPTBLPL);
		}
	}
	UNLOCK(&spec_table_mutex, pl);
}

/*
 * void
 * spec_makestrcln(vnode_t *vp, dev_t *devp, vnode_t **vpp)
 *	setup snodes for new streaming clone device (*devp).
 *
 * Calling/Exit State:
 *	enter with original common snode locked. leave with same.
 *	Creates new set of snodes using devsec_cloneopen().
 *	The new common snode is locked exclusive
 *	and pointer to the vnode contained within the new 
 *	non-common snode is returned in vpp.
 */
void
spec_makestrcln(vnode_t *vp, dev_t ndev, vnode_t **vpp)
{

	snode_t *csp;

	/*
	 *	vp passed is vnode pointer of Master Clone.
	 *	Create New snodes and return pointer to vnode in
	 *	the non-common snode after acquiring exclusive
	 *	sleep lock on the new common snode.
	 */

	*vpp = devsec_cloneopen(*vpp, ndev, VCHR);
	ASSERT(*vpp);

	csp = VTOS(VTOS(*vpp)->s_commonvp);
	/*
	 * Acquire exclusive lock on the common snode.
	 */
	SPEC_RWLOCK_WRLOCK(csp);
	/*
	 * STREAMS clones inherit fsid.
	 */
	VTOS(*vpp)->s_fsid = VTOS(vp)->s_fsid;
	return;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

#ifdef	DEBUG
extern void print_vnode(const vnode_t *);
#endif	/* DEBUG */

/*
 * void
 * print_snode(const snode_t *sp)
 *	Print an snode.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_snode(const snode_t *sp)
{
	debug_printf("snode struct = %x:\n", sp);
	debug_printf("\tflag = %8x, size = %8x, fsid = %8x, dev: %d,%d\n",
		 sp->s_flag, sp->s_devsize, sp->s_fsid,
		 getemajor(sp->s_dev), geteminor(sp->s_dev));

	debug_printf("\trealvp = %8x, commonvp = %8x, nextr = %8x, cnt = %8x\n",
		 sp->s_realvp, sp->s_commonvp, sp->s_nextr, sp->s_count);

	if (mac_installed) {
		debug_printf(
		 "\tdsecp = %8x, dstate = %8d, dmode = %8d, secflag: %8x\n",
			sp->s_dsecp, sp->s_dstate, sp->s_dmode, sp->s_secflag);
		debug_printf("\trelflag = %8d, hilid = %8d, lolid = %8d\n",
			REL_FLAG(sp), HI_LEVEL(sp), LO_LEVEL(sp));
	}
#ifdef	DEBUG
	print_vnode(STOV(sp));
#else
	debug_printf("\tvnode = %lx\n", STOV(sp));
#endif	/* DEBUG */
}

#endif	/* defined(DEBUG) || defined(DEBUG_TOOLS) */
