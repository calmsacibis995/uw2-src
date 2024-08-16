/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/profs/profs_vnops.c	1.21"
#ident	"$Header: $"

#include <util/types.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/engine.h>
#include <io/uio.h>
#include <fs/vnode.h>
#include <fs/dirent.h>
#include <fs/fs_subr.h>
#include <fs/pathname.h>
#include <fs/profs/prosrfs.h>
#include <fs/profs/profs_data.h>
#include <proc/cred.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <svc/time.h>
#include <util/processor.h>
#include <proc/disp.h>
#include <acc/dac/acl.h>
#include <fs/fs_hier.h>

STATIC int	proopen(vnode_t **, int, cred_t *);
STATIC int	proclose(vnode_t *, int, boolean_t, off_t, cred_t *);
STATIC int	proread(vnode_t *, struct uio *, int, cred_t *);
STATIC int 	prowrite(vnode_t *, struct uio *, int, cred_t *);
STATIC int	progetattr(vnode_t *, struct vattr *, int, cred_t *);
STATIC int	proaccess(vnode_t *, int, int, cred_t *);
STATIC int	prolookup(vnode_t *, char *, vnode_t **, pathname_t *, int,
			  vnode_t *, cred_t *);
STATIC int	proreaddir(vnode_t *, struct uio *, cred_t *, int *);
STATIC void	proinactive(vnode_t *, cred_t *);
STATIC int	prorwlock(vnode_t *, off_t, int, int, int);
STATIC void	prorwunlock(vnode_t *, off_t, int);
STATIC int	proseek(vnode_t *, off_t, off_t *);

extern void	pro_get_chip_fpu(struct plocal *plp, procfile_t *procfilep);

extern struct ctlr_desc ctlr_desc;
extern struct pronode *processorp;
extern struct pronode prorootnode;
extern struct pronode *ctlp;
extern struct pronode *processoridp[32];
extern struct prorefcnt prorefcnt;
extern lkinfo_t prorootnode_lock_buf;

extern lock_t processorfs_ptr_lck;

extern lid_t 	proctllid;
extern lid_t 	prorootlid;


/*
 * The following is the operations vector.
 */

vnodeops_t provnodeops = {
	proopen,
	proclose,
	proread,
	prowrite,
	(int (*)())fs_nosys,	/* ioctl */
	fs_setfl,
	progetattr,
	(int (*)())fs_nosys,	/* setattr */
	proaccess,
	prolookup,
	(int (*)())fs_nosys,	/* create */
	(int (*)())fs_nosys,	/* remove */
	(int (*)())fs_nosys,	/* link */
	(int (*)())fs_nosys,	/* rename */
	(int (*)())fs_nosys,	/* mkdir */
	(int (*)())fs_nosys,	/* rmdir */
	proreaddir,
	(int (*)())fs_nosys,	/* symlink */
	(int (*)())fs_nosys,	/* readlink */
	(int (*)())fs_nosys,	/* fsync */
	proinactive,
	(void (*)())fs_nosys,	/* release */
	(int (*)())fs_nosys,	/* fid */
	prorwlock,
	prorwunlock,
	proseek,
	fs_cmp,
	(int (*)())fs_nosys,	/* frlock */
	(int (*)())fs_nosys,	/* realvp */
	(int (*)())fs_nosys,	/* getpage */
	(int (*)())fs_nosys,	/* putpage */
	(int (*)())fs_nosys,	/* map */
	(int (*)())fs_nosys,	/* addmap */
	(int (*)())fs_nosys,	/* delmap */
	fs_poll,
	(int (*)())fs_nosys,	/* pathconf */
	(int (*)())fs_nosys,	/* getacl */
	(int (*)())fs_nosys,	/* setacl */
	(int (*)())fs_nosys,	/* setlevel */
	(int (*)())fs_nosys,	/* getdvstat */
	(int (*)())fs_nosys,	/* setdvstat */
	(int (*)())fs_nosys,	/* makemld */
	(int (*)())fs_nosys,	/* testmld */
	(int (*)())fs_nosys,	/* stablestore */
	(int (*)())fs_nosys,	/* relstore */
	(int (*)())fs_nosys,	/* getpagelist */
	(int (*)())fs_nosys,	/* putpagelist */
	(int (*)())fs_nosys,	/* msgio */
	(int (*)())fs_nosys,	/* filler[4]... */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys
};

STATIC struct vnode *proget();

/*
 * Directory characteristics (patterned after the s5 file system).
 */

struct prodirect {
	u_short	d_ino;
	char	d_name[PRONSIZE];
};

#define	PROSDSIZE	(sizeof(struct prodirect))


/*
 * int
 * proopen(vnode_t **vpp, int flag, cred_t *cr)
 *
 *	File system specific open routine.
 *
 * Calling/Exit State:
 *
 *      Calling LWP holds vnode's r/w sleep lock (v_lock) in *shared* mode
 *      for duration of call. The lock was obtained before calling sfs_open.
 *
 * Description:
 *
 *	There is no file system specific action required for this
 *	file system. It simply returns success.
 */
/* ARGSUSED */
STATIC int
proopen(vnode_t **vpp, int flag, cred_t *cr)
{
	return(0);
}


/*
 * int
 * proclose(vnode_t *vp, int flag, boolean_t count, off_t offset, cred_t *cr)
 *
 *	File system specific close routine.
 *
 * Calling/Exit State:
 *
 *	The caller doesn't hold any locks on the vnode in question.
 *
 * Description:
 *
 *	There is no file system specific action required for this
 *	file system. It simply returns success.
 */
/* ARGSUSED */
STATIC int
proclose(vnode_t *vp, int flag, boolean_t count, off_t offset, cred_t *cr)
{
	return(0);
}

 
/*
 * int
 * proread(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
 *
 *      Transfer data from <vp> to the calling process's address
 *      space.
 *
 * Calling/Exit State:
 *
 *      The calling LWP must hold the inode's rwlock in at least
 *      *shared* mode on entry; rwlock remains held on exit. This
 *      lock is usually obtained by a call to VOP_RWRDLOCK().
 *
 * Description:
 *
 *	For this file system, any readable file (processorid file)
 *	has a fixed size. All the items of interest can be
 *	read from the engine structure after acquiring the
 *	eng_tbl_mutex lock. After reading this information
 *	into a local buffer, only the required information
 *	is copied back to the user space via uiomove.
 */
/* ARGSUSED */
STATIC int
proread(vnode_t *vp, uio_t *uiop, int ioflag, cred_t *cr)
{
	struct pronode *pnp = VTOPRO(vp);
	engine_t *eng;
	struct plocal *plp;
	pl_t pl;
	procfile_t procfile1; /* local buffer */
	procfile_t *procfilep;
	int	error = 0;
	int	size;
	char *tmpp;

	if (vp->v_type == VDIR)  {
		return (EISDIR); /* can't read directory */
	}

	if (uiop->uio_offset < 0)
		return EINVAL;

	if (uiop->uio_offset > sizeof(struct procfile))
		return 0;
 
	size = min(uiop->uio_resid, sizeof(struct procfile) - uiop->uio_offset);
 
	/* Check what type of file is being read */
	/* i.e., whether ctl or processorid.     */

	procfilep = &procfile1;
 
	switch (pnp->pro_filetype) {
 
	case CTL :  /* ctl file, should return error */
		error = EACCES;
		break;
 
	case PROCESSORID :	/* processorid file is being read */

		/*
		 * processorid file is being read.
		 * Get the info into the procfile structure.
		 * Most of the required info is in the
		 * engine table of the processor. 
		 * eng_tbl_mutex needs to be held while
		 * examining the engine table.
		 * Only the cache size is not available in
		 * the engine structure, but can be obtained
		 * by reading the ctlr_desc array mapped
		 * into memory.
		 */
		eng = &engine[pnp->pro_id];
		plp = ENGINE_PLOCAL_PTR(pnp->pro_id);

		pl = LOCK(&eng_tbl_mutex, PLHI);

		procfilep->clockspeed = eng->e_cpu_speed;

		if ((eng->e_flags & E_OFFLINE) == 0)
			procfilep->status = P_ONLINE; /* Online */
		else if ((eng->e_flags & (E_SHUTDOWN | E_OFFLINE)) == 1)
			procfilep->status = P_OFFLINE; /* Offline */
		else
			procfilep->status = P_BAD; /* Bad */

		/*
		 * Now, get the chip type and FPU type
		 * by calling pro_get_chip_fpu() that is
		 * instruction set dependent.
		 */
		pro_get_chip_fpu(plp, procfilep);

		procfilep->cachesize = (eng->e_nsets) * (eng->e_setsize);
		if (eng->e_flags & E_DRIVERBOUND)
			procfilep->bdrivers = 1;
		else
			procfilep->bdrivers = 0;
		procfilep->modtime = eng->e_smodtime;

		UNLOCK(&eng_tbl_mutex, pl);

		/*
		 * Now, move data to user space.
		 */
		tmpp = (char *)procfilep + uiop->uio_offset;
 
		error = uiomove((caddr_t)tmpp, size, UIO_READ, uiop);
		break;
	default :	/* should not happen */
		break;
	}
 
 	return error;
 
}


/*
 * int
 * prowrite(struct vnode *vp, struct uio *uiop, int ioflag, cred_t *cr)
 *
 *      Transfer data from the calling process's address space
 *      to <vp>.
 *
 * Calling/Exit State:
 *
 *      The calling LWP holds the inode's rwlock in *exclusive* mode on
 *      entry; it remains held on exit. The rwlock was acquired by calling
 *      VOP_RWWRLOCK.
 *
 * Description:
 *
 *	This is the file system specific write routine. Data to the
 *	'ctl' file (only writable file in the file system) is expected
 *	in the form of one or more message structures. Each message
 *	structure consists of a command (online or offline) and a
 *	processor-id argument. Each message is parsed and depending
 *	on the command, the online_engine() or offline_engine()
 *	is called with the appropriate processorid argument.
 *	There is no need to serialize simultaneous onlines and
 *	offlines, as such serialization is provided inside the
 *	called routines that perform the operation.
 */
/* ARGSUSED */
STATIC int
prowrite(struct vnode *vp, struct uio *uiop, int ioflag, cred_t *cr)
{
	struct pronode *pnp = VTOPRO(vp);
	int error;
	int error2;
	ctlmessage_t message1;  /* local buffer */
	ctlmessage_t *msgp;

	/*
	 * Check for permissions. Only privileged users
	 * can write into this file.
	 */
	if (pm_denied (cr, P_SYSOPS))
		return EPERM;

	if (vp->v_type == VDIR) {
		return EISDIR;
	}
 
	if (pnp->pro_filetype != CTL) { /* only ctl file is writable */
		return EACCES;
	}

	if (uiop->uio_offset < 0)
		return EINVAL;

	if (uiop->uio_resid == 0)
		return 0;

	if (((uiop->uio_resid) % sizeof(struct message)) != 0)
		return EINVAL;	  /* message not in correct format */

	msgp = &message1;
	error2 = 0;
 
 	while (uiop->uio_resid > 0) {	/* while more messages to process */
		error = uiomove((caddr_t)&message1, sizeof(struct message), UIO_WRITE, uiop);
		if (error)
			return error;

		switch (msgp->m_cmd) {
		case P_ONLINE :
			error = online_engine((int)msgp->m_argument);
			break;
		case P_OFFLINE :
			error = offline_engine((int)msgp->m_argument);
			break;
		default :
			error = EINVAL;
			break;
		}

		if (error2 == 0)
			error2 = error;
 
	}

	return error2;	/* non-zero value => one or more messages failed */
}


/*
 * int
 * progetattr(struct vnode *vp, struct vattr *vap, int flags, cred_t *cr)
 *
 *      Return attributes for a vnode.
 *
 * Calling/Exit State:
 *
 *      No locks are held on entry or exit.
 *
 * Description:
 *
 *	This function provides all the attributes of a vnode
 *	such as the uid, gid, number of links, the file size,
 *	etc.
 */
/* ARGSUSED */
STATIC int
progetattr(struct vnode *vp, struct vattr *vap, int flags, cred_t *cr)
{
	struct pronode *pnp = VTOPRO(vp);
	struct engine *eng;

	/*
	 * Return all the attributes. 
	 *
	 */
	vap->va_type = vp->v_type;
	vap->va_mode = pnp->pro_mode;
	vap->va_uid = 0;
	vap->va_gid = 0;
	vap->va_fsid = processordev;
	vap->va_nlink = 1;
	/*
	 * node id's : root node - 2
	 *             ctl file  - 3
	 *             proc id 0 - 4
	 *             proc id 1 - 5
	 *                -
	 *             proc id i - i+4
	 */
	if (pnp->pro_filetype == CTL)
		vap->va_nodeid = 3;
	else if (pnp->pro_filetype == PROCESSORID)
		vap->va_nodeid = pnp->pro_id + 4;
	else /* root node */
		vap->va_nodeid = 2;
	if (pnp->pro_filetype == CTL)
		vap->va_size = 0;
	else if (pnp->pro_filetype == PROCESSORID)
		vap->va_size = sizeof(procfile_t);
	else {
		vap->va_size = (Nengine + 3) * PROSDSIZE;
	}

	vap->va_rdev = 0;

	vap->va_atime = hrestime;
	if (pnp->pro_filetype == PROCESSORID) {
		eng = &engine[pnp->pro_id];
		RUNQUE_LOCK();
		vap->va_mtime = eng->e_smodtime;
		RUNQUE_UNLOCK();
	} else
		vap->va_mtime = hrestime;
	vap->va_ctime = hrestime;
	vap->va_blksize = MAXBSIZE;
	vap->va_nblocks = (vap->va_size)/MAXBSIZE;
	vap->va_vcode = 0;
	if (vap->va_mask & AT_ACLCNT) {
		vap->va_aclcnt = NACLBASE;
	}
	return 0;
}


/*
 * int
 * proaccess(struct vnode *vp, int mode, int flags, cred_t *cr)
 *
 *      Determine the accessibility of a file to the calling
 *      lwp.
 *
 * Calling/Exit State:
 *
 *      No locks held on entry; no locks held on exit.
 *
 * Description:
 *
 *	The mode of the file system node (stored in the node)
 *	is compared with the access mode. Next, DAC access is
 *	checked. EACCES is returned if the modes do not match
 *	and DAC access failed. Otherwise, success is returned.
 */
/* ARGSUSED */
STATIC int
proaccess(struct vnode *vp, int mode, int flags, cred_t *cr)
{
	struct pronode *pnp = VTOPRO(vp);
	int cmode = 0;
	int error = 0;
	int denied_mode;

	if ((cmode = (pnp->pro_mode & mode)) != mode)
		error = EACCES;

	if ((mode == VEXEC) && (pnp != &prorootnode))
		return EACCES;

	/*
	 * Check if the caller has privilege.
	 */
	denied_mode = (mode & ~cmode);

	if ((((denied_mode & VREAD) && pm_denied(cr, P_DACREAD))
		|| ((denied_mode & VWRITE) && pm_denied(cr, P_DACWRITE))) == 0)
		return 0;
	return error;
}

/*
 * int
 * prolookup(struct vnode *dp, char *comp, struct vnode **vpp, pathname_t *pnp,
 *	     int lookup_flags, vnode_t *rootvp, cred_t *cr)
 *
 *      Check whether a given directory contains a file named <nm>.
 *
 * Calling/Exit State:
 *
 *      No locks on entry or exit.
 *
 * Description:
 *
 *	The pathname supplied is compared with predefined file/
 *	directory names such as "processor", "ctl". If it does
 *	not match these names, it must be a processor-id file
 *	and the processor-id is extracted from the supplied pathname.
 *	If there is a match with "processor", the global pointer
 *	to the root vnode is returned. If the pathname corresponds
 *	to either "ctl" or a processor-id file, a check needs to
 *	be made to see if a vnode already exists or if one needs
 *	to be allocated. This is done by a utility function proget()
 *	that is called by this function.
 */
/* ARGSUSED */
STATIC int
prolookup(struct vnode *dp, char *comp, struct vnode **vpp, pathname_t *pnp,
	  int lookup_flags, vnode_t *rootvp, cred_t *cr)
{
	int n = 0;

	if (comp[0] == 0 || strcmp(comp, ".") == 0 || strcmp(comp, "..") == 0) {
		VN_HOLD(dp);
		*vpp = dp;
		return 0;
	}
 
	/*
	 * If 'comp' is either 'ctl' or a processorid file
	 * create vnode if one does not exist for it 
	 */
 
	if (strcmp(comp, "ctl") == 0) {
		*vpp = proget(CTL, 0);
		return (*vpp == NULL) ? ENOENT: 0;
	}
 
	/* It is one of the "id-files" */
	/* Collect the file name (id) in 'n'.*/
 
	while (*comp) {
		if (*comp < '0' || *comp > '9')
			return ENOENT;
		n = 10 * n + *comp++ - '0';
	}
	if (n > Nengine)
		return ENOENT;

	*vpp = proget(PROCESSORID, n);
	return (*vpp == NULL) ? ENOENT : 0;
}

/*
 * struct vnode *
 * proget(int type, int id)
 *
 *	Find or construct a vnode for the file.
 *
 * Calling/Exit State:
 *
 *	No locks held on entry.
 *
 * Description:
 *
 *	A check is made to see if the global pointer to the
 *	named vnode already exists. If one exists, simply
 *	perform VN_HOLD on it and return its pointer.
 *	If a vnode does not exist, one is kmem_alloc'd with
 *	KMSLEEP option. It is possible that we sleep here
 *	and by the time we wake up with the requested memory,
 *	someone else might have created this vnode. To
 *	avoid having multiple vnodes for a single file, we
 *	go back to see if a vnode exists now. If it exists,
 *	we simply release the memory just acquired and return
 *	the pointer to the vnode. If one does not exist still,
 *	we hold on to the acquired memory and initialize the
 *	vnode appropriately. 
 */
STATIC struct vnode *
proget(int type, int id)
{
 
	struct pronode *pnp = NULL;
	struct vnode *vp;

	/* First, check if a vnode already exists */
again:
	LOCK(&processorfs_ptr_lck, PLHI); /* acquire global pointer lock */

	switch (type) {
	case CTL :
		if (ctlp) { /* vnode exists */
			RWSLEEP_WRLOCK_RELLOCK(&ctlp->pro_lock, PRINOD,
					&processorfs_ptr_lck);
			VN_HOLD((vnode_t *)ctlp);
			vp = (vnode_t *)ctlp;
			RWSLEEP_UNLOCK(&ctlp->pro_lock);
			if (pnp != NULL) {
				RWSLEEP_DEINIT(&pnp->pro_lock);
				kmem_free(pnp, sizeof(struct pronode));
			}
			return vp;
		}
		break;
	case PROCESSORID :
		if (processoridp[id]) {  /* vnode exists */
			RWSLEEP_WRLOCK_RELLOCK(&processoridp[id]->pro_lock,
					PRINOD, &processorfs_ptr_lck);
			VN_HOLD((vnode_t *)processoridp[id]);
			vp = (vnode_t *)processoridp[id];
			RWSLEEP_UNLOCK(&processoridp[id]->pro_lock);
			if (pnp != NULL) {
				RWSLEEP_DEINIT(&pnp->pro_lock);
				kmem_free(pnp, sizeof(struct pronode));
			}
			return vp;
		}
		break;
	default : /* ??? */
		break;
	}
 
	/*
	 * New vnode required; allocate it and fill in all the fields.
	 */

	if (pnp == NULL) {	/* first time here */
		UNLOCK(&processorfs_ptr_lck, PL0);
		pnp = (pronode_t *)kmem_zalloc(sizeof(pronode_t), KM_SLEEP);

		/* Initialize R/W sleep lock in the pronode */

		RWSLEEP_INIT(&pnp->pro_lock, (uchar_t) 0, 
			&prorootnode_lock_buf, KM_SLEEP);

	/*
	 * There could be a race condition here.
	 * In case we slept in kmemalloc, perhaps someone else
	 * created this vnode while we were asleep.
	 * Go back and try again.
	 */
 
		goto again;
	}

	/*
	 * At this time, we have ensured that neither ctlp nor
	 * processoridp[id] exists. We must assign ctlp or processoridp[id]
	 * and prevent other lwp's from using the node before releasing
	 * processorfs_ptr_lck.
	 */
	
	switch (type) {
	case CTL :
		ctlp = pnp;
		break;
	case PROCESSORID :
		processoridp[id] = pnp;
		break;
	default :  /* ??? */
		break;
	}

	ASSERT(RWSLEEP_IDLE(&pnp->pro_lock));
	(void)RWSLEEP_TRYWRLOCK(&pnp->pro_lock);
	UNLOCK(&processorfs_ptr_lck, PL0);

	/*
	 * Now, we have exclusive access to the node, other lwp's
	 * trying to access the node will block on pro_lock which
	 * we hold. We complete the initialization outside of
	 * of processorfs_ptr_lck.
	 */

	vp = &pnp->pro_vnode;

	vp->v_vfsmountedhere = NULL;
	vp->v_op = &provnodeops;
	vp->v_count = 1;
	vp->v_data = (caddr_t) pnp;


	/*
	 * Initialize the other locks in generic vnode.
	 */
	VN_INIT(vp, (vfs_t *) 0, VREG, (dev_t) 0, 0, KM_SLEEP);

	vp->v_flag = VNOMAP|VNOMOUNT|VNOSWAP;
 
	switch (type) {
	case CTL :
		pnp->pro_mode = 0200; /* write-only ctl file */
		vp->v_lid = proctllid; /* this is a tunable */
		vp->v_vfsp = provfs;
		break;
	case PROCESSORID : 
		pnp->pro_mode = 0444; /* read-only file */
		pnp->pro_id = id;
		vp->v_lid = prorootlid; /* same lid as root node */
		vp->v_vfsp = provfs;
		break;
	default : /* ?? */
		break;
	}
	pnp->pro_filetype = type;
	RWSLEEP_UNLOCK(&pnp->pro_lock);
 
	/*
	 * Now, increment the global reference count of active
	 * files in the file system.
	 */

	LOCK(&prorefcnt.lock, PLHI);
	prorefcnt.count++;
	UNLOCK(&prorefcnt.lock, PL0);

	return vp;
}


/*
 * int
 * proreaddir(struct vnode *vp, struct uio *uiop, cred_t *cr, int *eofp)
 *
 *      Read from a directory.
 *
 * Calling/Exit State:
 *
 *      The calling LWP holds the inode's rwlock in *shared* mode. The
 *      rwlock was obtained by a call to VOP_RWRDLOCK.
 *
 * Description:
 *
 *	Reads the successive entries from the directory.
 *	It simply loops through the directory entries until either
 *	the user's request is satisfied or all entries are read.
 *	The requested information is passed on to user via uiomove.
 */

/* ARGSUSED */
STATIC int
proreaddir(struct vnode *vp, struct uio *uiop, cred_t *cr, int *eofp)
{
	/* bp holds one dirent structure */
	char bp[round(sizeof(struct dirent)-1+PRONSIZE+1)];
	struct dirent *dirent = (void *)bp;
	int reclen;
	register int i, j, n;
	int oresid, dsize;
	off_t off;
	int newoff;

	if (uiop->uio_offset < 0 || uiop->uio_resid < PROSDSIZE
	  || (uiop->uio_offset % PROSDSIZE) != 0)
		return EINVAL;
 
 
	dsize = (char *)dirent->d_name - (char *)dirent;
	oresid = uiop->uio_resid;
	/*
	 * Loop until user's request is satisfied or until all entries
	 * have been examined.
	 */
 
	off = 0;
	for (; uiop->uio_resid > 0; uiop->uio_offset = off + PROSDSIZE) {
		if ((off = uiop->uio_offset) == 0) {	/* "." */
			strcpy(dirent->d_name, ".");
			reclen = dsize+1+1;
			dirent->d_ino = 2; 
		} else if (off == PROSDSIZE) { /* ".." */
			strcpy(dirent->d_name, "..");
			reclen = dsize+2+1;
		} else if (off == 2 * PROSDSIZE) { /* "ctl" */
			strcpy(dirent->d_name, "ctl");
			reclen = dsize+3+1;
			dirent->d_ino = 3; 
		} else {
			/* see how far down the directory we are */
			newoff = (off - 2*PROSDSIZE)/PROSDSIZE;

				/*
				 * Stop when entire directory has been examined.
				 */
			if (newoff >= (Nengine + 1))
				break;
			/*
			 * 'newoff' now has engine number (id).
			 * But, numbering should begin from 0
			 * and not 1. Hence, subtract 1.
			 */
			n = newoff - 1;
			/*
			 * numbering of inode numbers :  root - 2
			 *                               ctl  - 3
			 *                               proc 0 - 4
			 *                               proc i - i+4
			 */
			dirent->d_ino = n + 4; 
			for (j = PRONSIZE-1; j >= 0; j--) {
				dirent->d_name[j] = n % 10 + '0';
				n /= 10;
			}
			dirent->d_name[PRONSIZE] = '\0';
			reclen = dsize+PRONSIZE+1;
		}
		dirent->d_off = uiop->uio_offset + PROSDSIZE;
		/*
		 * Pad to nearest word boundary (if necessary).
		 */
		for (i = reclen; i < round(reclen); i++)
			dirent->d_name[i-dsize] = '\0';
		dirent->d_reclen = reclen = round(reclen);
		if (reclen > uiop->uio_resid) {
			/*
			 * Error if no entries have been returned yet.
			 */
			if (uiop->uio_resid == oresid)
				return EINVAL;
			goto done;
		}
		/*
		 * uiomove() updates both resid and offset by the same
		 * amount.  But we want offset to change in increments
		 * of PROSDSIZE, which is different from the number of bytes
		 * being returned to the user.  So we set uio_offset
		 * separately (in the 'for' loop), ignoring what uiomove()
		 * does.
		 */
		if (uiomove((caddr_t) dirent, reclen, UIO_READ, uiop))
			return EFAULT;
	}
done:	if (eofp)
		*eofp = ((uiop->uio_offset-3*PROSDSIZE)/PROSDSIZE >=
				Nengine);
	return 0;
}


/*
 * void
 * proinactive(struct vnode *vp, cred_t *cr)
 *
 *      Perform cleanup on an unreferenced inode.
 *
 * Calling/Exit State:
 *
 *      No locks held on entry, no locks held on exit.
 *
 * Description:
 *
 *	It ensures that it is not racing with another lwp that is doing
 *	a lookup (in which case it returns after releasing appropriate
 *	locks). If there is no such race, it sets the global pointer to
 *	the vnode to NULL and in all cases except the root node (which
 *	is statically allocated), it deallocates the file node.
 */
/* ARGSUSED */
STATIC void
proinactive(struct vnode *vp, cred_t *cr)
{
	struct pronode *pnp = VTOPRO(vp);
	pl_t pl1;

	ASSERT(vp->v_count >= 0);

	RWSLEEP_WRLOCK(&pnp->pro_lock, PRINOD);
	/* got the lock */
	VN_LOCK(vp);
	if (vp->v_count > 1) {
		VN_UNLOCK(vp);
		RWSLEEP_UNLOCK(&pnp->pro_lock);
		return;
	}
	VN_UNLOCK(vp);

	pl1 = LOCK(&processorfs_ptr_lck, PLHI);

	/*
	 * If there are lwp's blocked on the R/W sleep lock,
	 * we are racing with another lwp doing a lookup
	 * on this vnode. Release locks and return.
	 */
	if (RWSLEEP_LOCKBLKD(&pnp->pro_lock)) {
		UNLOCK(&processorfs_ptr_lck, PL0);
		RWSLEEP_UNLOCK(&pnp->pro_lock);
		return;
	}
	RWSLEEP_UNLOCK(&pnp->pro_lock);

	/*
	 * Reset the vnode pointer, deinitialize lock and
	 * deallocate vnode.
	 * Deallocation of root vnode is not done as this is 
	 * preallocated memory (i.e., not allocated in mount).
	 */
	
	switch(pnp->pro_filetype) {
	case CTL :
		ASSERT(ctlp != NULL);
		ctlp = NULL;
		RWSLEEP_DEINIT(&pnp->pro_lock);
		VN_DEINIT(vp);
		kmem_free(pnp, sizeof(struct pronode));
		break;
	case PROCESSORID :
		ASSERT(processoridp[pnp->pro_id] != NULL);
		processoridp[pnp->pro_id] = NULL;
		RWSLEEP_DEINIT(&pnp->pro_lock);
		VN_DEINIT(vp);
		kmem_free(pnp, sizeof(struct pronode));
		break;
	default :
		break;
	}
	 
	UNLOCK(&processorfs_ptr_lck, pl1);
	 
	/* Decrement global count of active files */
	
	if (pnp->pro_filetype != PROCESSOR) {
		LOCK(&prorefcnt.lock, PLHI);
		prorefcnt.count--;
		UNLOCK(&prorefcnt.lock, PL0);
	}
}


/*
 * int
 * proseek(struct vnode *vp, off_t ooff, off_t *noffp)
 *
 *      Validate a seek pointer.
 *
 * Calling/Exit State:
 *
 *	No locks held on entry.
 *
 * Description :
 *
 *	This is a no op and simply returns 0.
 */
/* ARGSUSED */
int
proseek(struct vnode *vp, off_t ooff, off_t *noffp)
{
	return 0;
}
 

/*
 * int
 * prorwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
 *
 *      Obtain, if possible, the file node's rwlock according to <mode>.
 *
 * Calling/Exit State:
 *
 *	No lock held on entry.
 *
 * Description:
 *
 *      Acquire the file node's rwlock in the requested mode.
 *	On success, the lock is held in the requested mode.
  *	On failure, the rwlock of the node is not held.
 */
/* ARGSUSED */
int
prorwlock(vnode_t *vp, off_t off, int len, int fmode, int mode)
{
	struct	pronode *pnp = VTOPRO(vp);

	if (mode == LOCK_EXCL) {	/* exclusive lock */
		RWSLEEP_WRLOCK(&pnp->pro_lock, PRINOD);
	} else {			/* mode ==  LOCK_SHARED: shared lock */
		RWSLEEP_RDLOCK(&pnp->pro_lock, PRINOD);
	}

	return (0);
}


/*
 * void
 * prorwunlock(vnode_t *vp, off_t off, int len)
 *
 *      Release the file node's rwlock.
 *
 * Calling/Exit State:
 *
 *      On entry, the calling LWP must hold the inode's rwlock
 *      in either *shared* or *exclusive* mode. On exit, the
 *      caller's hold on the lock is released.
 *
 * Description:
 *
 *	It unlocks the rwlock of the file node.
 */
/* ARGSUSED */
void
prorwunlock(vnode_t *vp, off_t off, int len)
{
	struct pronode *pnp = VTOPRO(vp);


	RWSLEEP_UNLOCK(&pnp->pro_lock);
	return;
}
