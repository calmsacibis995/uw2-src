/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/xnamfs/xsd.c	1.16"
#ident	"$Header: $"

/*
 * Xenix shared data, xsd, is a shared memory facility similar to System V
 * shared memory, shm.  Like shm, xsd is implemented using as_map() to grant
 * accesses to the object from an address space.  Unlike shm, xsd uses the
 * file system name space, instead of an object id, to designate a shared
 * memory objects. In addition, it does not have separate interfaces to cause
 * the shared memory to be locked in memory, but it has separate user-level
 * interfaces, sdenter(2), sdleave(2), sdgetv(2), and sdwaitv(2), for
 * synchronizing user activity within the same object.
 *
 * Xsd uses the xnamfs filesystem utility routines to cause the shared memory
 * to appear in the filesystem name space in a fashion similar to the way
 * fifofs uses specfs to cause named pipes to appear in the filesystem name
 * space.  In fact, specvp() invokes xnamvp() to allocate a xnamnode and set
 * v_op to point to xnam_vnodeops such that all future accesses to the vnode
 * is switched to appropriate xnamfs VOP routines.
 *
 * Xsd is different from other Xenix IPC in the following areas:
 *	- The object is created in two stages: xnamenode creation and address
 *	  space manipulation.
 *
 *	- Each object must be created or attached to the current address space
 *	  before it can be used.  Its footprints can be found in both address
 *	  space and p_sdp link list, which records the address, size, and
 *	  permission of each object, of the current process.
 *
 * Xsd design hightlights:
 *
 *	- Xsd has two tunable parameters XSDSEGS and XSDSLOTS. 
 *	  XSDSEGS specifies the number of xsd objects configured in the system.
 *	  (XSDSLOTS x XSDSEGS) speficies the maximum number of xsd attachments
 *	  allowed at any instant in the system.
 *
 * 	- Shared data objects are vnodes of the type VXNAM.  Thus, they live
 *	  in the same name space as XENIX semaphores with different v_rdev
 *	  type (XNAM_SD).
 *
 * 	- Each XENIX shared data object has a xsd structure associated
 *	  with it. A maximum total of XSDSEGS objects is allowed in
 *	  the system.
 *
 * 	- The per object data, when it is in use, lives in the xnamnode
 *	  associated with the object.  When a object is not used, its xsd 
 *	  structure lives in a free list, xsd_freelist.
 *
 *	- System allows a maximum total of (XSDSEGS * XSDSLOTS) xsd
 *	  simultaneous accesses at any time.  This is managed through
 *	  sdtab[XSDSEGS * XSDSLOTS], which is allocated at boot time.
 *
 *	- All XENIX shared data objects currently attached to a process can be
 *	  found from the link list pointed to by its p_sdp, which is an sdtab[]
 *	  entry link list.
 *
 *	- All free sdtab[] entry is linked to sdtab_freep.
 *
 */

/*
 * Locking:
 *
 *	- one spin lock xsd_global_lock is used to protect
 *	  xsd_freelist and sdtab_freep manipulations.
 *
 *	- p_sdp link list and all fields of the sd structures on the list
 *	  are protected by p_mutex lock.
 *
 *	- one synchronization variable, x_sv, per object is used to
 *	  synchronize Version Control of an object.  The synchronization
 *	  variable is protected by its x_mutex lock.
 *
 *	- as_wrlock is used to serialize mutiple sdget(create) opeartions
 *	  on the same object.  See comments in xsd_create for details.
 * 
 *	- Each object, that is currently in use, is protected by its
 *	  associated xnamnode spin lock, x_mutex.  See xnamnode.h for more
 *	  details.
 */

/* XENIX Support */

#include <acc/mac/mac.h>
#include <fs/fstyp.h>
#include <fs/memfs/memfs.h>
#include <fs/pathname.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <fs/xnamfs/xnamhier.h>
#include <fs/xnamfs/xnamnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/seg_vn.h>
#include <mem/tuneable.h>
#include <proc/cred.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/sd.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

extern void map_addr(vaddr_t *, uint_t, off_t, int);
extern void xnammark(xnamnode_t *, int );

/*
 * Out of xsd resources report time in HZ.
 */
#define XSD_REPORTTIME	(30 * 60 * HZ)

/*
 * head of free sdtab list
 */
STATIC struct sd	*sdtab_freep;

/*
 * head of xsd free list
 */
STATIC xsd_t    	*xsd_freelist;

/*
 * spin lock protecting:
 *	- xsd_freelist
 *	- sdtab_freep
 */

STATIC lock_t    	xsd_global_lock;
STATIC LKINFO_DECL(xsd_global_lkinfo, "FS:XNAMFS:xsd_global_lock", 0);

#define XSDGLOBAL_LOCK()	LOCK(&xsd_global_lock, FS_XFREEPL);
#define XSDGLOBAL_UNLOCK(pl)	UNLOCK(&xsd_global_lock, (pl));

/*
 * marcos to handle sd_keepcnt
 */

/*
 * XSD_HOLD(struct sd *sdp)
 *	Bump the sd_keepcnt of sdp.
 *
 * Calling/Exit State:
 *	The caller should hold p_mutex lock before calling this macro.
 *	The lock is returned in the same state.
 */
#define	XSD_HOLD(sdp)	{	\
	(sdp)->sd_keepcnt++;	\
}

/*
 * XSD_RELE(struct sd *sdp)
 *	decrement the sd_keepcnt of sdp.
 *
 * Calling/Exit State:
 *	None.  The caller should hold p_mutex lock before calling this routine.
 *	The lock is returned in the same state.
 */
#define XSD_RELE(sdp) {					\
	ASSERT(((sdp)->sd_keepcnt) >= 2);		\
	if ((--(sdp)->sd_keepcnt) == 2)	 {		\
		if (SV_BLKD(&(sdp)->sd_sv)) {		\
			SV_SIGNAL(&(sdp)->sd_sv, 0);	\
		}					\
	}						\
} 

/*
 * Local routines
 */

STATIC int xsd_create(struct xnamnode *xp, uint_t len, int flags);
STATIC int xsd_srch(char *addr, struct sd **sdpp, struct xnamnode **xp);
STATIC int xsd_detach(struct sd *sdp);

/*
 * void xsd_init(void)
 *	initialize XENIX shared data resources.
 *
 * Calling/Exit State:
 *	The routine is called during boot time and require no locking.
 */
void
xsdinit(void)
{
	struct sd *sdp;
	xsd_t *xsdp;
	int i, size, max_attach;

	if (v.v_xsdsegs <= 0 || v.v_xsdslots <= 0) {
		/* 
		 *+ XENIX shared data is not configured in because at least
		 *+ one tunable parameter is less than or equal to zero.  Check
		 *+ the parameters XSDSEGS and XSGSLOTS, and rebuild the kernel
		 *+ to configure XENIX shared data.
		 */
		cmn_err(CE_WARN, "xsdinit: XENIX shared data not configured");
		LOCK_INIT(&xsd_global_lock, FS_XFREEHIER, FS_XFREEPL,
	                  &xsd_global_lkinfo, KM_NOSLEEP);
	
		return;
	}

	LOCK_INIT(&xsd_global_lock, FS_XFREEHIER, FS_XFREEPL,
	          &xsd_global_lkinfo, KM_NOSLEEP);
	
	/*
	 * allocate xsd[v.v_xsdsegs] and sdtab[v.v_xsdsegs * v.v_xsdslots]
	 * and link them in their free list.
	 */
	
	max_attach = v.v_xsdsegs * v.v_xsdslots;
	size = sizeof(xsd_t) * v.v_xsdsegs + sizeof(struct sd) * max_attach;

	xsd_freelist = (struct xsd *)kmem_zalloc(size, KM_NOSLEEP);

	if (xsd_freelist == NULL) {
		/*
		 *+ Could not allocate xsd entries at boot time.  Check XENIX
		 *+ shared data tunable parameters XSDSEGS and XSGSLOTS.
		 */
		cmn_err(CE_NOTE,
			"xsdinit: Can't allocate XENIX shared data directory");
		return;
	} else {
		sdtab_freep = (struct sd *)(xsd_freelist + v.v_xsdsegs);
	}

	/*
	 * link all xsd entries in xsd_freelist, null terminated
	 */

	for (xsdp = xsd_freelist, i = 0; i < v.v_xsdsegs; xsdp++, i++) {
		xsdp->xsd_nextsd = xsdp + 1;
	}
	xsdp->xsd_nextsd = NULL;

	/*
 	 * Set up the free list of sdtab entries. One entry is
	 * maintained per attachment per shared data object.  The entries
 	 * for one proc's shared data objects are linked together  
         * through the table.
 	 */
	for (sdp = sdtab_freep, i = 0; i < max_attach; sdp++, i++)
		sdp->sd_link = sdp + 1;

	sdp->sd_link = NULL;

	return;
}

/*
 * STATIC int
 * xsd_create(struct xnamnode *xp, uint_t len, int flags)
 * 	create a new XENIX shared data object and associate it with xp
 *
 * Calling/Exit State:
 *	The routine acquires x_mutex  and drops it before returning to it's
 *	callers.  The callers are prepared to block.
 *	Return 0, if a new object is successfully created. Return a non-zero
 *	value, if a new object cannot be created for reasons listed below:
 *		- ENFILE, if there is no free entry available
 *		- ENOMEM, if there is no enough virtual memroy to create the
 *		  object.
 *		- EEXIST, if the object to be created already exists.
 */
STATIC int
xsd_create(struct xnamnode *xp, uint_t len, int flags)
{
	xsd_t *xsdp;
	vnode_t *mvp;
	static clock_t last_overflow = 0;
	pl_t opl;

	(void)XNODE_LOCK(xp);

	/*
	 * If the object exists now, it is an error.
	 */

	if (xp->x_sd) {
		ASSERT(xp->x_sd->xsd_mvp);
		XNODE_UNLOCK(xp, PLBASE);
		return (EEXIST);	/* one exists now */
	}

	opl = XSDGLOBAL_LOCK();
	if ((xsdp = xsd_freelist) == NULL) {
		XSDGLOBAL_UNLOCK(opl);
		if (last_overflow) {	/* not the first time */
			if ((lbolt - last_overflow) >= XSD_REPORTTIME) {  
				/*
				 *+ Out of XENIX shared data objects. Increase XENIX shared data
				 *+ tunable parameters XSDSEGS and XSGSLOTS, and rebuild kernel.
				 */
				cmn_err(CE_NOTE, "XENIX shared data table overflow\n");
			}
			last_overflow = lbolt;
		} else {	/* it is the first time */
			/*
			 *+ Out of XENIX shared data objects. Increase XENIX shared data
			 *+ tunable parameters XSDSEGS and XSGSLOTS, and rebuild kernel.
			 */
			cmn_err(CE_NOTE, "XENIX shared data table overflow\n");
			last_overflow = lbolt;
		}

		if (SV_BLKD(&xp->x_sv)) {
			XNODE_UNLOCK(xp, PLBASE);
			SV_BROADCAST(&xp->x_sv, 0);
		} else
			XNODE_UNLOCK(xp, PLBASE);

		return (ENFILE);
	}	/* out of objects */

	xsd_freelist = (struct xsd *)xsdp->xsd_nextsd;
	XSDGLOBAL_UNLOCK(opl);

	xp->x_sd = xsdp;
	XNODE_UNLOCK(xp, PLBASE);

	if ((mvp = memfs_create_unnamed(len + 1, MEMFS_FIXEDSIZE)) ==
	    (vnode_t *)NULL) {
		opl = XSDGLOBAL_LOCK();
		xsdp->xsd_nextsd = xsd_freelist;
		xsd_freelist = xsdp;
		XSDGLOBAL_UNLOCK(opl);

		XNODE_LOCK(xp);
		xp->x_sd = NULL;

		if (SV_BLKD(&xp->x_sv)) {
			XNODE_UNLOCK(xp, PLBASE);
			SV_BROADCAST(&xp->x_sv, 0);
		} else
			XNODE_UNLOCK(xp, PLBASE);

		return (ENOMEM);
	}

	XNODE_LOCK(xp);

	SV_INIT(&xsdp->xsd_sv);
	xsdp->xsd_flags = (short)flags;
	xsdp->xsd_len = len;
	xsdp->xsd_snum = 0;
	xsdp->xsd_mvp = mvp;

	xp->x_flag &= ~XNAMEINVAL;

	/* 
	 * wake up all sleepers
	 */

	if (SV_BLKD(&xp->x_sv)) {
		XNODE_UNLOCK(xp, PLBASE);
		SV_BROADCAST(&xp->x_sv, 0);
	 } else
		XNODE_UNLOCK(xp, PLBASE);

	return (0);
}

/*
 * void 
 * xsd_destroy(struct xnamnode *xp)
 * 	destroy the XENIX shared data object currently associated with xp.
 * Calling/Exit State:
 *	This rotuine does not block.  The caller should make sure that nobody
 *	is blocked for xp.  It acquires xnamnode x_mutex and xsd_global_lock
 *	locks and release them before returning to it caller.
 *	
 * Remarks:
 *	The routine is called from xnam_inactive() and cannot be static
 */
void
xsd_destroy(struct xnamnode *xp)
{
	pl_t opl;
	ASSERT(!SLEEP_LOCKBLKD(&xp->x_rwlock));
	ASSERT(xp->x_sd->xsd_mvp);

	VN_RELE(xp->x_sd->xsd_mvp);

	opl = XSDGLOBAL_LOCK();
	xp->x_sd->xsd_nextsd = xsd_freelist;
	xsd_freelist = xp->x_sd;
	xp->x_sd = NULL;
	XSDGLOBAL_UNLOCK(opl);

	return;
}

/*
 * STATIC int
 * xsd_attach(struct xnamnode *xp, int flags, rval_t *rvp)
 *	Attach the object identified in xp to the current address space
 *	and return the attached address in rvp
 *
 * Calling/Exit State:
 *	The caller is prepared to block in this routine.  This routine acquires
 *	address space write lock and p_mutex locks and drops them before
 *	returning to its callers.
 */
STATIC int
xsd_attach(struct xnamnode *xp, int flags, rval_t *rvp)
{
	vnode_t *vp;
	struct	sd *sdp, *sdq;
	proc_t *pp = u.u_procp;
	struct as *asp = pp->p_as;
	uint_t size;
	struct segvn_crargs	crargs; /* segvn create arguments */
	vaddr_t addr;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	vp = XNAMTOV(xp);

	ASSERT(xp->x_sd);

	/*
	 * allocate a free slot from the head of the sdtab[]
	 */
	(void)XSDGLOBAL_LOCK();
	if ((sdp = sdtab_freep) == NULL) {
		XSDGLOBAL_UNLOCK(PLBASE);
		return (EMFILE);
	}

	sdtab_freep = sdtab_freep->sd_link;
	XSDGLOBAL_UNLOCK(PLBASE);

	/*
	 * ensure this sd object not already attached to this proc.
	 * After dropping p_mutex lock below, another sdget(create) can
	 * race to create the same object.  To solve this problem, 
	 * as_wrlock is acquired and held through the following operation
	 * until sdp is successfully created and linked into the p_sdp list.
	 * No separate synchronization means is deployed to save locking
	 * trip since as_wrlock is required for map_addr and as_map later.
	 */

	as_wrlock(asp);
	(void)LOCK(&pp->p_mutex, PLHI);
	for (sdq = pp->p_sdp; sdq != NULL; sdq = sdq->sd_link) {
		ASSERT(sdq->sd_addr);
		if (sdq->sd_xnamnode == xp)
		{
			UNLOCK(&pp->p_mutex, PLBASE);
			error = EINVAL;
			goto err1;
		}
	}
	UNLOCK(&pp->p_mutex, PLBASE);

	/*
	 * attach the object to current address space.
 	 * using as layer interface to do this.
	 */

	size = memfs_map_size(((xsd_t *)xp->x_sd)->xsd_mvp);

	map_addr(&addr, size, (off_t)0, 1);
	if (addr == NULL) {
		error = ENOMEM;
		goto err1;
	}

	crargs = *(struct segvn_crargs *)zfod_argsp;    /* structure copy */
	crargs.vp = xp->x_sd->xsd_mvp;
	crargs.offset = 0;
	crargs.type = MAP_SHARED;
	crargs.prot = (flags & SD_WRITE ) ? PROT_ALL :
	    (PROT_ALL & ~PROT_WRITE);
	crargs.maxprot = crargs.prot;

	error = as_map(pp->p_as, addr, size, segvn_create, (caddr_t)&crargs);
	if (error) {
		goto err1;
	}

	/*
	 * fill the per process data with correct information
	 */

	sdp->sd_addr = (char *)addr;
	sdp->sd_xnamnode = xp;
	sdp->sd_cpaddr = NULL;
	sdp->sd_flags = flags & (SD_WRITE);
	sdp->sd_keepcnt = 1;
	SV_INIT(&sdp->sd_sv);

	(void)LOCK(&pp->p_mutex, PLHI);
	sdp->sd_link = pp->p_sdp;
	pp->p_sdp = sdp;
	UNLOCK(&pp->p_mutex, PLBASE);
	as_unlock(asp);

	/* set return value (start address of object) */
	/* convert linear address to logical address */

	rvp->r_val1 = (int)addr;

	return (0);

err1:
	(void)XSDGLOBAL_LOCK();
	sdp->sd_link = sdtab_freep;		/* free the sdtab entry */
	sdtab_freep = sdp;
	XSDGLOBAL_UNLOCK(PLBASE);

	as_unlock(asp);

	return (error);
}

struct sdgeta {
	char *path;
	int flags;
	unsigned limit;
	int mode;
};

/*
 * int
 * sdget(struct sdgeta *uap, rval_t *rvp)
 *	XENIX system call sdget(2).
 *
 * Calling/Exit State:
 *	The routine is called at PLBASE without holding any lock.
 *	The routine is called in context.
 *
 *	The routine returns 0 on success; otherwise returns -1 for the
 *	following conditions:
 *		-EEXIST: SD_CREATE and the object already exists
 *		-EINVAL: trying to attach to a already attached object or
 *		         trying to SD_CREAT an existing non-xnamfs file
 *		-ENOTNAM: trying to attached a non-existent object and
 *	  	          SD_CREAT not specified. 
 */
/* ARGSUSED */
int
sdget(struct sdgeta *uap, rval_t *rvp)
{
	vnode_t *vp;
	struct xnamnode *xp;
	struct vattr vattr;
	proc_t *pp = u.u_procp;
	int error;
	cred_t *credp = CRED();

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (uap->flags & SD_CREAT) {	/* we want to create a object */

		/*
		 * Check whether the file exists now
		 */

		if (error = lookupname(uap->path, UIO_USERSPACE, FOLLOW, NULLVPP, &vp)) {
			/*
			 * the file either does not exist or lookupname failed
			 * for the reasons below:
			 * 	- copyinstr(called from pn_get) returned
			 *	  EFAULT or ENAMETOOLONG, or
			 *	- lookuppn returned ELOOP, ENOENT, ENOTDIR, or
			 *	  EINVAL.
			 */

			if (error != ENOENT) {
				goto sdget_err2;
			}

			/*
			 * the file does not exist, create it.
			 */

			vattr.va_type = VXNAM;
			/*
			 * no locking required for p_cmask, see Process
			 * subsystem feature design document 
			 */
			vattr.va_mode = (uap->mode & MODEMASK) & ~(pp->p_cmask);
			vattr.va_mask = AT_TYPE|AT_MODE;
			vattr.va_rdev = XNAM_SD;
			vattr.va_mask |= AT_RDEV;

			if (error = vn_create(uap->path, UIO_USERSPACE,
					&vattr, EXCL, 0, &vp, CRMKNOD)) { 
				/*
				 * vn_create failed for the reasons below:
				 *	- EACCESS: cannot access the parent directory
				 *	- EEXIST: the named file already exists.
				 *	- EROFS: it is read-only file system
				 */

				goto sdget_err2;
			}

			if (vp->v_op != &xnam_vnodeops) {
				error = EINVAL;
				goto sdget_err1;
			}

			xp = VTOXNAM(vp);

			xnammark(xp, XNAMCHG);

		} else	{	/* the file exists */

			/*
			 * if file is not correct type or is active,
			 * exit with ENOTNAM or EEXIST.
			 */

			/*
			 * no locking required since these fields never changed
			 * after their instantiations.
			 */
			if ((vp->v_type != VXNAM) || 
			    (vp->v_rdev != XNAM_SD)) {
				error = ENOTNAM;
				goto sdget_err1;
			}

			/*
		 	 * Don't allow sdget unless vp is associated with
			 * correct v_op.
		 	 */

			if (vp->v_op != &xnam_vnodeops) {
				error = EINVAL;
				goto sdget_err1;
			}

			/* check permissions */
			if (error = VOP_ACCESS(vp, VREAD, 0, credp)) {
				goto sdget_err1;
			}

			if (uap->flags & SD_WRITE)
				if (error = VOP_ACCESS(vp, VWRITE, 0, credp)) {
					goto sdget_err1;
			}

			if (vp->v_count != 1) { 
				if (BADVISE_PRE_SV) {
					/* ignore extraneous CREAT flag */
					uap->flags &= ~SD_CREAT;
					goto attach_only;
				}
				error = EEXIST;
				goto sdget_err1;
			}

			/*
			 * object file exists, but not in use now.
			 */
			xp = VTOXNAM(vp);

		}	/* lookupname */

		/*
		 * no locking since these two fields never changed
		 * after their instantiation.
		 */ 
		ASSERT(vp->v_type == VXNAM);
		ASSERT(vp->v_rdev == XNAM_SD);

		/*
		 * Racing between multiple sdget's(SD_CREAT):
		 *	No explicit synchronization is required! We are taking
		 *	advantage of serialization done in file system.
		 *	Only the one reach here first actually creates the
		 *	object.  For others, xsd_create() returns EEXIST.
		 *
		 * Racing between sdget(SD_CREAT) and sdget(attach):
		 *	To prevent another sdget(attach) from using the object
		 *	before it is ready, set x_sd and hold XNAMEINVAL until
		 *	the object is ready for use. In case of creation error,
		 *	x_sd is set to NULL and all blocked attachers are
		 *	waken up.
		 */

		if (error = xsd_create(xp, uap->limit,
		                       uap->flags & (SD_UNLOCK|SD_NOWAIT))) {
			goto sdget_err1;
		}
	} else {	/* !SD_CREAT: just want to attach, not create */

		int vmode;

		if (uap->flags & SD_WRITE)
			vmode = VWRITE;
		else
			vmode = VREAD;

		if (error = lookupname(uap->path, UIO_USERSPACE, FOLLOW, NULLVPP, &vp)) { 
			goto sdget_err2;
		}

		/* check MAC access */
		if ((error = MAC_VACCESS(vp, vmode, credp)) != 0) {
			goto sdget_err1;
		}
		
		if ((vp->v_type != VXNAM) ||
		    (vp->v_rdev != XNAM_SD)) {
			error = ENOTNAM;
			goto sdget_err1;
		}

		/*
		 * Don't allow sdget unless
		 * v_op is associated with an xnamnode.
		 */
		if (vp->v_op != &xnam_vnodeops) {
			error = EINVAL;
			goto sdget_err1;
		}

		/* check permissions */
		if (error = VOP_ACCESS(vp, VREAD, 0, credp)) {
			goto sdget_err1;
		}

		if (uap->flags & SD_WRITE)
			if (error = VOP_ACCESS(vp, VWRITE, 0, credp)) {
				goto sdget_err1;
		}

		xp = VTOXNAM(vp);

		/* 
		 * Easy case first: the object does not exist at all. 
		 */

		XNODE_LOCK(xp);
		if (xp->x_sd == NULL) {
			XNODE_UNLOCK(xp, PLBASE);
			error = ENOENT;
			goto sdget_err1;
		}

		/*
		 * If the creation of the object is undergoing.  Wait for it
		 */

		while (xp->x_flag & XNAMEINVAL && xp->x_sd != NULL) {
			SV_WAIT(&xp->x_sv, FS_XSDPRI, &xp->x_mutex);
			XNODE_LOCK(xp);
		}

		/*
		 * Check whether we are racing with another sdget, which
		 * is in the middle of creating the object.
		 * If the object was not created successfully, return ENOENT.
		 */

		if (xp->x_sd == NULL) {
			XNODE_UNLOCK(xp, PLBASE);
			error = ENOENT;
			goto sdget_err1;
		}
		XNODE_UNLOCK(xp, PLBASE);

	}	/* !SD_CREAT */

attach_only:
	/*
	 * Now attach it to my address space
	 */
	if (error = xsd_attach(xp, uap->flags, rvp))
		goto sdget_err1;

	return (0);

sdget_err1:
	VN_RELE(vp);

sdget_err2:
	return (error);
}

/*
 * int
 * xsd_srch(char *addr, struct sd **sdpp, struct xnamnode **xp)
 * 	search for matching slot in sdtab. By convention, addr == NULL
 *	means the slot is empty.  The routine is called in context.
 *
 * Calling/Exit State:
 *	On success, the routine returns 0.
 *	EINVAL if the current process does not have the XENIX shared data
 *	object attached at addr.
 *	The callers should hold p_mutex lock before calling this routine.
 *	The lock is returned in the same state.
 *
 * Remarks:
 *	All XENIX shared data system calls, except sdget(2), first call
 *	xsd_srch first to translate addr to sd structure and XSD_HOLD.
 */
STATIC int
xsd_srch(char *addr, struct sd **sdpp, struct xnamnode **xpp)
{
	struct sd *sdp;
	proc_t *pp = u.u_procp;

	ASSERT(KS_HOLD1LOCK() && LOCK_OWNED(&pp->p_mutex));

	/*
	 * Find sd entry in the current process.
	 */
	for (sdp = pp->p_sdp; sdp != NULL; sdp = sdp->sd_link) { 
		if (sdp->sd_addr == NULL)
			continue;

		if (addr == sdp->sd_addr) {
			*sdpp = sdp;
			*xpp = sdp->sd_xnamnode;
			break;
		}
	}

	if (sdp == NULL)
		return (EINVAL);

	XSD_HOLD(sdp);
	return (0);
}

/*
 * int
 * xsd_detach(struct sd *sdp)
 *	Detach the Xenix shared data object associated with sdp.
 *
 * Calling/Exit State:
 *	The routine is called with p_mutex held and returns the lock dropped
 *	before returning to its callers.  This routine may block.
 */
STATIC int
xsd_detach(struct sd *sdp)
{
	int error = 0, len;
	struct xnamnode	*xp;
	vnode_t *vp;
	proc_t *pp = u.u_procp;
	struct as *asp = pp->p_as;
	vaddr_t addr;

 	xp = sdp->sd_xnamnode;

	addr = (vaddr_t)sdp->sd_addr;
	len = xp->x_sd->xsd_len + 1;

	ASSERT(LOCK_OWNED(&pp->p_mutex));

	/*
	 * force an sdleave if necessary.
	 */

	if (sdp->sd_flags & SD_BTWN) {
		sdp->sd_flags &= ~(SD_BTWN);
		UNLOCK(&pp->p_mutex, PLBASE);

		(void)XNODE_LOCK(xp);
		xp->x_sd->xsd_flags &= ~(SDI_LOCKED);

		/*
		 * increment snum
		 */
		xp->x_sd->xsd_snum++;
		xp->x_sd->xsd_snum &= 0x7fff;	/* for 286 compatibility */

		if (SV_BLKD(&xp->x_sd->xsd_sv)) {
			XNODE_UNLOCK(xp, PLHI);
			SV_BROADCAST(&xp->x_sd->xsd_sv, 0);
		} else {
			XNODE_UNLOCK(xp, PLHI);
		}
		LOCK(&pp->p_mutex, PLHI);
	}

	/* 
	 * remove sdp from the process.
	 */

	if (pp->p_sdp == sdp) {
		pp->p_sdp = sdp->sd_link;
	} else {
		struct	sd *p;
		for (p = pp->p_sdp; p->sd_link != sdp; p = p->sd_link)
			;
		p->sd_link = sdp->sd_link;
	}

	if (sdp->sd_keepcnt > 2) {
		SV_WAIT(&sdp->sd_sv, PRIMED, &pp->p_mutex);
		LOCK(&pp->p_mutex, PLHI);
	}

	sdp->sd_keepcnt = 0;
	UNLOCK(&pp->p_mutex, PLBASE);

	/*
	 *  Find the object corresponding to a sd entry.
	 */
	
	sdp->sd_addr = NULL;

	/*
	 * put sdp at the head of sdtab_freep
	 */
	(void) XSDGLOBAL_LOCK()
	sdp->sd_link = sdtab_freep;
	sdtab_freep = sdp;
 	sdp->sd_xnamnode = NULL;
	XSDGLOBAL_UNLOCK(PLBASE);

	vp = XNAMTOV(xp);

	VN_LOCK(vp);
	if (vp->v_count == 1) {
		VN_UNLOCK(vp);
		xnammark(xp, XNAMACC | XNAMCHG);
	} else {
		VN_UNLOCK(vp);
	}

	/*
	 * unmap the object from the address space
	 */
	as_wrlock(asp);
	if ((error = as_unmap(asp, addr, len)) != 0) {
		error = EINVAL;
	}
	as_unlock(asp);

	/* copy XENIX 286 shared data to "real" shared data */
	if ((BADVISE_XSDSWTCH) && (pp->p_sdp != NULL)) {
		xsdswtch(0);
	}

	return (error);
}

struct sdfreea {
	char *address;
};

/*
 * int
 * xsdfree(struct sdfreea *uap, rval_t *rvp)
 *	Detach the XENIX shared data object, which was attached at addr
 *	in the current process.  sdfree(2) system call.
 *
 * Calling/Exit State:
 *	The routine is called at PLBASE without holding any lock.
 *
 * Remarks:
 *	sdfree will detach the object from the current process even when
 *	the process is between sdenter and sdleave.
 */	
int
xsdfree(struct sdfreea *uap, rval_t *rvp)
{
	struct sd *sdp;
	struct xnamnode *xp;
	vnode_t *vp;
	proc_t *pp = u.u_procp;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	xp = 0;
	sdp = 0;
	error = 0;

	/* find the slot in sdtab in the current process */

	(void)LOCK(&pp->p_mutex, PLHI);
	if (error = xsd_srch(uap->address, &sdp, &xp)) {
		/*
		 * nothing attached at uap->address now, return EINVAL
		 */
		UNLOCK(&pp->p_mutex, PLBASE);
		return (error);
	}

	ASSERT(sdp);
	ASSERT(xp);

	vp = XNAMTOV(xp);

	error = xsd_detach(sdp);

	/* p_mutex lock is dropped in xsd_detach */

	rvp->r_val1 = 0;

	/*
	 * if this is the last process, the object is
	 * freed in the next VN_RELE, which calls VOP_INACTIVE.
	 * VOP_INACTIVE in turn calls xsd_destroy to destroy the object.
	 */

	VN_RELE(vp);
	return (error);

}

struct sdentera {
	char *	addr;
	int	flags;
};

/*
 * int
 * sdenter(struct sdentera *uap, rval_t *rvp)
 *	sdenter(2) system call.
 *
 * Calling/Exit State:
 *	The routine is called in context at PLBASE.
 *	Return 0, if succeed and mark the object SDI_BTWN. If the object is
 *	lockable, mark SDI_LOCKED on return.
 *	Return ENAVAIL, if the SDI_NOWAIT and the the object is current locked.
 */
/*ARGSUSED*/
int
sdenter(struct sdentera *uap, rval_t *rvp)
{
	struct sd *sdp;
	struct xnamnode *xp;
	short 	*flag;
	proc_t *pp = u.u_procp;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * find the object:
	 */

	xp = 0;
	sdp = 0;
	error = 0;

	(void)LOCK(&pp->p_mutex, PLHI);

	if (error = xsd_srch(uap->addr, &sdp, &xp)) {
		UNLOCK(&pp->p_mutex, PLBASE);
		return (error);
	}

	ASSERT(xp);
	ASSERT(sdp);

	/*
	 * disallow entering to write if is sdget'ed readonly
	 */

	if ( (uap->flags & SD_WRITE) && !(sdp->sd_flags & SD_WRITE )) { 
		UNLOCK(&pp->p_mutex, PLBASE);
		error = EINVAL;
		goto ret;
	}
	UNLOCK(&pp->p_mutex, PLBASE);

	flag = &(xp->x_sd->xsd_flags);

	(void)XNODE_LOCK(xp);
	if (*flag & SDI_LOCKED) {
		if (uap->flags & SD_NOWAIT) {
			XNODE_UNLOCK(xp, PLBASE);
			error = ENAVAIL;
			goto ret;
		}

		while (*flag & SDI_LOCKED) {
			if (SV_WAIT_SIG(&xp->x_sd->xsd_sv ,FS_XSDPRI,
					&xp->x_mutex) == B_FALSE) {
				error = EINTR;
				goto ret;
			}
			(void)XNODE_LOCK(xp);
		}
	}


	if ((*flag & SD_UNLOCK) == 0 )
		*flag |= SDI_LOCKED;

	XNODE_UNLOCK(xp, PLBASE);

	(void)LOCK(&pp->p_mutex, PLHI);
	sdp->sd_flags |= SD_BTWN;
	XSD_RELE(sdp);
	UNLOCK(&pp->p_mutex, PLBASE);
	return (0);

ret:
	(void)LOCK(&pp->p_mutex, PLHI);
	XSD_RELE(sdp);
	UNLOCK(&pp->p_mutex, PLBASE);
	return (error);
}

struct sdleavea {
	char *addr;
};

/*
 * int
 * sdleave(struct sdleavea *uap, rval_t *rvp)
 *	sdleave(2) system call
 *
 * Calling/Exit State:
 *	The routine is called at PLBASE without holding any lock.
 */
/*ARGSUSED*/
int 
sdleave(struct sdleavea *uap, rval_t *rvp)
{
	struct sd *sdp;
	struct xnamnode *xp;
	proc_t *pp = u.u_procp;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	xp = 0;
	sdp = 0;
	error = 0;

	(void)LOCK(&pp->p_mutex, PLHI);
	if (error = xsd_srch(uap->addr, &sdp, &xp)) {
		UNLOCK(&pp->p_mutex, PLBASE);
		return (error);
	}

	ASSERT(sdp);
	ASSERT(xp);

	sdp->sd_flags &= ~(SD_BTWN);
	UNLOCK(&pp->p_mutex, PLBASE);

	(void) XNODE_LOCK(xp);
	if ((xp->x_sd->xsd_flags & SD_UNLOCK) == 0)
		xp->x_sd->xsd_flags &= ~(SDI_LOCKED);

	/* increment snum */
	xp->x_sd->xsd_snum++;
	xp->x_sd->xsd_snum &= 0x7fff;	/* for 286 compatibility */

	if (SV_BLKD(&xp->x_sd->xsd_sv)) {
		XNODE_UNLOCK(xp, PLBASE);
		SV_BROADCAST(&xp->x_sd->xsd_sv, 0);
	} else
		XNODE_UNLOCK(xp, PLBASE);

	(void)LOCK(&pp->p_mutex, PLHI);
	XSD_RELE(sdp);
	UNLOCK(&pp->p_mutex, PLBASE);

	return (0);
}

struct sdgetva {
	char *addr;
};

/*
 * int
 * sdgetv(struct sdgetva *uap, rval_t *rvp)
 *	sdget(2) system call.
 * 
 * Calling/Exit State:
 *	The routine is called at PLBASE without holding any locks.
 */
int
sdgetv(struct sdgetva *uap, rval_t *rvp)
{
	struct sd *sdp;
	struct xnamnode *xp;
	proc_t *pp = u.u_procp;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	xp = 0;
	sdp = 0;
	error = 0;

	(void)LOCK(&pp->p_mutex, PLHI);
	if (error = xsd_srch(uap->addr, &sdp, &xp)) {
		UNLOCK(&pp->p_mutex, PLBASE);
		return (error);
	}

	ASSERT(sdp);
	ASSERT(xp);

	rvp->r_val1 = xp->x_sd->xsd_snum;
	XSD_RELE(sdp);
	UNLOCK(&pp->p_mutex, PLBASE);

	return (0);
}

struct sdwaitva {
	char *addr;
	int	num;
};

/*
 * int
 * sdwaitv(struct sdwaitva *uap, rval_t *rvp)
 *	sdwaitv(2) system call.
 *
 * Calling/Exit State:
 *	The routine is called at PLBASE without holding any locks.
 */
int
sdwaitv(struct sdwaitva *uap, rval_t *rvp)
{
	struct sd *sdp;
	struct xnamnode *xp;
	proc_t *pp = u.u_procp;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	sdp = 0;
	xp = 0;
	error = 0;

	(void)LOCK(&pp->p_mutex, PLHI);
	if (error = xsd_srch(uap->addr, &sdp, &xp)) {
		UNLOCK(&pp->p_mutex, PLBASE);
		return (error);
	}
	UNLOCK(&pp->p_mutex, PLBASE);

	ASSERT(sdp);
	ASSERT(xp);

	(void)XNODE_LOCK(xp);
	while (xp->x_sd->xsd_snum == uap->num) {
		if (SV_WAIT_SIG(&xp->x_sd->xsd_sv ,FS_XSDPRI, &xp->x_mutex) ==
			B_FALSE) {
			error = EINTR;
			(void)XNODE_LOCK(xp);
			break;
		}
		(void)XNODE_LOCK(xp);
	}

	rvp->r_val1 = xp->x_sd->xsd_snum;
	XNODE_UNLOCK(xp, PLBASE);

	(void)LOCK(&pp->p_mutex, PLHI);
	XSD_RELE(sdp);
	UNLOCK(&pp->p_mutex, PLBASE);

	return (error);
}

/*
 * void
 * xsdexit(void)
 *	remove all references to XENIX shared data objects from the
 *	current process.
 *
 * Calling/Exit State:
 *	The routine is called from remove_proc or exit.  Since the caller
 *	will make sure that no other agents can alter my address space
 *	before calling this routine, no locking is required.
 */
void
xsdexit(void)
{
	struct xnamnode *xp;
	vnode_t *vp;
	proc_t *pp = u.u_procp;

	while (u.u_procp->p_sdp != NULL) { 
		xp = pp->p_sdp->sd_xnamnode;
		ASSERT(xp->x_sd);

		(void)LOCK(&pp->p_mutex, PLHI);
		(void)xsd_detach(pp->p_sdp);

		/* p_mutex is dropped in xsd_detach */
		vp = XNAMTOV(xp);
		VN_RELE(vp);
	}
	return;
}

/*
 * void
 * xsd_cleanup(proc_t *pp)
 *	remove all references to XENIX shared data objects from the
 *	specified process.
 *
 * Calling/Exit State:
 *	The routine is called from proc_cleanup.  
 *	No locking is necessary for the proc structure, since it
 *	is not yet visible to others.
 */
void
xsd_cleanup(proc_t *pp)
{
	struct sd *sdpp, *sdcp;
	struct xnamnode *xp;
	vnode_t *vp;

	/* 
	 * no locking required since prp is not visible to the
	 * outside world yet.
	 */

	for (sdcp = pp->p_sdp; sdcp != NULL; 
			sdpp = sdcp, sdcp = sdcp->sd_link){
		xp = sdcp->sd_xnamnode;
		vp = XNAMTOV(xp);
		(void)XNODE_LOCK(xp);
		sdcp->sd_addr = NULL;
		sdcp->sd_keepcnt = 0;
		VN_RELE(vp);
		XNODE_UNLOCK(xp, PLBASE);
	}

	(void)XSDGLOBAL_LOCK();
	sdpp->sd_link = sdtab_freep;
	sdtab_freep = pp->p_sdp;
	XSDGLOBAL_UNLOCK(PLBASE);

	pp->p_sdp = NULL;
}

/*
 * int
 * xsdfork(proc_t proc *cp, proc_t *pp)
 * 	called to adjust vnode reference counts during fork and to create
 *	duplicate sdtab entries for the child.
 *
 * Calling/Exit State:
 *	The caller should make sure that no external accesses to cpp address
 *	space are possible.  The routine is called at PLBASE and
 *	will not block.
 */
int
xsdfork(proc_t *cp, proc_t *pp)
{
	struct sd *sdpp, *sdcp;
	struct xnamnode *xp;
	vnode_t *vp;
	int error = 0;

	ASSERT(getpl() == PLBASE);

	(void)XSDGLOBAL_LOCK();
	(void)LOCK(&pp->p_mutex, PLHI);
	for (sdpp = pp->p_sdp; sdpp != NULL; sdpp = sdpp->sd_link) {

		xp = sdpp->sd_xnamnode;
		vp = XNAMTOV(xp);
		if (sdpp->sd_addr != NULL) {

			/* allocate free slot in sdtab. */
	
			if ((sdcp = sdtab_freep) == NULL) {
				error = EMFILE;
				UNLOCK(&pp->p_mutex, FS_XFREEPL);
				XSDGLOBAL_UNLOCK(PLBASE);
				goto failed;
			}
			sdtab_freep = sdtab_freep->sd_link;

			/* fill the per process data with right stuff. */
			sdcp->sd_xnamnode = sdpp->sd_xnamnode;
			sdcp->sd_addr = sdpp->sd_addr;
			sdcp->sd_cpaddr = sdpp->sd_cpaddr;
			sdcp->sd_flags = sdpp->sd_flags;
			sdcp->sd_keepcnt = 1;		
			sdcp->sd_link = cp->p_sdp;		
			cp->p_sdp = sdcp;
		
			/* Increment reference count of vnode */
			VN_HOLD(vp);
			ASSERT(xp->x_sd);
		}
	}
	UNLOCK(&pp->p_mutex, FS_XFREEPL);
	XSDGLOBAL_UNLOCK(PLBASE);
	
	return (0);

failed:
	if (cp->p_sdp != NULL) {

		/* 
		 * no locking required since cpp is not visible to the
		 * outside world yet.
		 */

		for (sdcp=cp->p_sdp; sdcp!=NULL; sdpp=sdcp, sdcp = sdcp->sd_link){
			xp = sdcp->sd_xnamnode;
			vp = XNAMTOV(xp);
			(void)XNODE_LOCK(xp);
			sdcp->sd_addr = NULL;
			sdcp->sd_keepcnt = 0;
			VN_RELE(vp);
			XNODE_UNLOCK(xp, PLBASE);
	    	}

		(void)XSDGLOBAL_LOCK();
		sdpp->sd_link = sdtab_freep;
		sdtab_freep = cp->p_sdp;
		XSDGLOBAL_UNLOCK(PLBASE);

		cp->p_sdp = NULL;
	}
	return (error);
}

/* End XENIX Support */
