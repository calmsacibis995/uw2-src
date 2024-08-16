/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/fifofs/fifosubr.c	1.21"
#ident	"$Header: $"

/*
 * This file contains the supporting routines
 * for FIFOFS file system.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/plocal.h>
#include <util/engine.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <svc/time.h>
#include <mem/kmem.h>
#include <util/inline.h>
#include <fs/file.h>
#include <proc/cred.h>
#include <util/sysmacros.h>
#include <fs/buf.h>
#include <fs/fs_hier.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/user.h>
#include <fs/fifofs/fifohier.h>
#include <fs/fifofs/fifonode.h>
#include <io/conf.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <fs/fs_subr.h>
#include <svc/errno.h>
#include <acc/mac/mac.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#include <acc/mac/covert.h>
#include <acc/mac/cc_count.h>
#endif


/*
 * The next set of lines define the bit map used to assign
 * unique pipe inode numbers.  The value chosen for FIFOMAXID
 * is arbitrary, but must fit in a short.
 *
 * fifomap   --> bitmap with one bit per pipe ino
 * testid(x) --> is ino x already in use?
 * setid(x)  --> mark ino x as used
 * clearid(x)--> mark ino x as unused
 */

#define FIFOMAXID	SHRT_MAX
#define testid(i)	((fifomap[(i)/NBBY] & (1 << ((i)%NBBY))))
#define setid(i)	((fifomap[(i)/NBBY] |= (1 << ((i)%NBBY))), (fifoids++))
#define clearid(i)	((fifomap[(i)/NBBY] &= ~(1 << ((i)%NBBY))), (fifoids--))

STATIC char fifomap[FIFOMAXID/NBBY + 1];
STATIC ino_t fifoids;
STATIC lock_t fifomap_mutex; /* protects fifomap and fifoids */

/*
 * Define routines/data structures within this file.
 */
STATIC struct fifonode	*fifoalloc;
lock_t			fifolist_mutex;	/* protects fifoalloc list */
STATIC struct vfs	*fifovfsp;
dev_t			fifodev;

/*
 *+ Lock protecting the fifonode hash list, fifoalloc.
 */
STATIC	LKINFO_DECL(flist_lkinfo, "FF::fifolist_mutex", 0);

/*
 *+ Lock protecting fifomap and fifoids.
 */
STATIC	LKINFO_DECL(fmap_lkinfo, "FF::fifomap_mutex", 0);

/*
 *+ Lock protecting fifofnode data.
 */
STATIC	LKINFO_DECL(node_lkinfo, "FF::n_lock", 0);

/*
 *+ FIFO read/write lock.
 */
STATIC	LKINFO_DECL(fiolock_lkinfo, "FF::fn_iolock", 0);

STATIC	int	fifo_stropen(vnode_t *, int, cred_t *);
STATIC	void	fifoinsert(struct fifonode *);
STATIC	struct fifonode *fifofind(vnode_t *);
STATIC	vnode_t	*makepipe(cred_t *);

void		fifoinit(struct vfssw *, int);
ino_t		fifogetid(void);
void		fifoclearid(ino_t);
vnode_t		*fifovp(struct vnode *, cred_t *);
int 		fifo_mkpipe(vnode_t **, vnode_t **, cred_t *);
void 		fifo_rmpipe(vnode_t *, vnode_t *, cred_t *);
int		setup_pipe(vnode_t *, vnode_t *, cred_t *);
int		fiforemove(struct fifonode *);
int             fifopreval(vtype_t, dev_t, cred_t *);

/*
 * Declare external variables/routines.
 */
extern struct	vnodeops	fifo_vnodeops;
extern int			fifoblksize;

extern int stropen(vnode_t *, dev_t *, vnode_t **, int,  cred_t *);
extern int strclose(vnode_t *, int, cred_t *);

STATIC struct vfsops fifovfsops = {
	(int (*)())fs_nosys,	/* mount */
	(int (*)())fs_nosys,	/* umount */
	(int (*)())fs_nosys,	/* root */
	(int (*)())fs_nosys,	/* statvfs */
	fs_sync,
	(int (*)())fs_nosys,	/* vget */
	(int (*)())fs_nosys,	/* mountroot */
	(int (*)())fs_nosys,	/* setceiling */
	(int (*)())fs_nosys,	/* filler[8] */
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
	(int (*)())fs_nosys,
};

/*
 * void
 * fifoinit(struct vfssw *vswp, int fstype)
 *	Initialize fifofs file system.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Save file system type/index, initialize vfs operations vector, get
 * 	unique device number for FIFOFS and initialize the FIFOFS hash.
 * 	Create and initialize a "generic" vfs pointer that will be placed
 * 	in the v_vfsp field of each pipes vnode.
 * 	Initialize fifolist_mutex, fifomap_mutex and vfs_mutex and vfs_lock.
 */
void
fifoinit(struct vfssw *vswp, int fstype)
{

	vswp->vsw_vfsops = &fifovfsops;
	if ((fifodev = getudev()) == NODEV) {
		/*
		 *+ A unique device number cannot be established.
		 */
		cmn_err(CE_WARN, "fifoinit: can't get unique device number");
		fifodev = 0;
	}
	fifoalloc = NULL;
	LOCK_INIT(&fifolist_mutex, FIFO_HIER, PLFIFO, 
		&flist_lkinfo, KM_SLEEP);
	LOCK_INIT(&fifomap_mutex, FIFO_HIER, PLFIFO, 
		&fmap_lkinfo, KM_SLEEP);

	fifovfsp = (vfs_t *)kmem_zalloc(sizeof(vfs_t), KM_SLEEP);
	VFS_INIT(fifovfsp, &fifovfsops, (caddr_t)NULL);
	fifovfsp->vfs_next = NULL;
	fifovfsp->vfs_vnodecovered = NULL;
	fifovfsp->vfs_bsize = FIFOBSIZE;
	fifovfsp->vfs_fstype = fstype;
	fifovfsp->vfs_fsid.val[0] = fifodev;
	fifovfsp->vfs_fsid.val[1] = fstype;
	fifovfsp->vfs_dev = fifodev;
	fifovfsp->vfs_bcount = 0;
}

/*
 * vnode_t * fifovp(vnode_t *vp, cred_t *crp)
 * 	Provide a shadow for a vnode. 
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	If vp already has a shadow in the hash list,
 * 	return its shadow. Otherwise, create a vnode to shadow vp, hash the 
 * 	new vnode and return its pointer to the caller.
 */
vnode_t *
fifovp(vnode_t *vp, cred_t *crp)
{
	struct fifonode *fnp, *nfnp;
	vnode_t *newvp;
	pl_t	pl;
	struct	 vattr va;

	ASSERT(vp != NULL);
	pl = LOCK(&fifolist_mutex, PLFIFO);
	if ((fnp = fifofind(vp)) == NULL) {
		UNLOCK(&fifolist_mutex, pl);
		fnp = (struct fifonode *)kmem_zalloc(sizeof(struct fifonode),
			KM_SLEEP);
		fnp->fn_nodelp = 
			(struct nodelock *)kmem_zalloc(sizeof(struct nodelock),
			KM_SLEEP);
		       
		/*
		 * initialize the times from vp.
		 */
		va.va_mask = AT_TIMES;
		if (VOP_GETATTR(vp, &va, 0, crp) == 0) {
			fnp->fn_atime = va.va_atime.tv_sec;
			fnp->fn_mtime = va.va_mtime.tv_sec;
			fnp->fn_ctime = va.va_ctime.tv_sec;
		}

#ifdef CC_PARTIAL
                /*
                 * Strictly speaking, this MAC_ASSUME is incorrect.
                 * No MAC check has been done on this vnode.  However,
                 * this ensures that the CCA tool understands that
                 * *vp and *sp are at the same level, and the later
                 * MAC_UNKLEV(fnp) ensures that the level relationship
                 * to *sp is correct.
                 */
                MAC_ASSUME(vp, MAC_SAME);
#endif

		fnp->fn_realvp = vp;
		newvp = FTOV(fnp);

		/* initialize vnode */
		VN_INIT(newvp, vp->v_vfsp, VFIFO, vp->v_rdev, 0, KM_SLEEP);
		newvp->v_op = &fifo_vnodeops;
		newvp->v_macflag |= (VMAC_DOPEN | VMAC_SUPPORT);
		/*
		 * Clear the fifonode's level.  Let the fs independent
		 * code take care of setting the vnode's level.
		 */
		if (vp->v_macflag & VMAC_SUPPORT)
			newvp->v_lid = vp->v_lid;
		else
			newvp->v_lid = vp->v_vfsp->vfs_macfloor;
		newvp->v_data = (caddr_t)fnp;
		VN_HOLD(vp);

		/* initialize locks and sync. variables */
		LOCK_INIT(&fnp->fn_nodelp->n_lock, FIFO_HIER, PLFIFO,
			&node_lkinfo, KM_SLEEP);
		fnp->fn_nodelp->n_count = 1;
		SLEEP_INIT(&fnp->fn_iolock, FIFO_HIER,
			&fiolock_lkinfo, KM_SLEEP);
		SV_INIT(&fnp->fn_rwait);
		SV_INIT(&fnp->fn_wwait);
		SV_INIT(&fnp->fn_fdwait);
		SV_INIT(&fnp->fn_openwait);

		pl = LOCK(&fifolist_mutex, PLFIFO);
		if ((nfnp = fifofind(vp)) != NULL) {
			/*
			 * Someone already created one. 
			 * Hold that one and destroy ours.
			 */
			FIFO_LOCK(nfnp, &fifolist_mutex);
			VN_HOLD(FTOV(nfnp));
			FIFO_UNLOCK(nfnp);

			VN_RELE(vp);
			VN_DEINIT(newvp);
			LOCK_DEINIT(&fnp->fn_nodelp->n_lock);
			SLEEP_DEINIT(&fnp->fn_iolock);
			kmem_free((caddr_t)fnp->fn_nodelp, 
				sizeof(struct nodelock));
			kmem_free((caddr_t)fnp, sizeof(struct fifonode));
			return FTOV(nfnp);
		}

		fifoinsert(fnp);
#ifdef CC_PARTIAL
                MAC_UNKLEV (fnp);
#endif
		UNLOCK(&fifolist_mutex, pl);

	}
	else {
		FIFO_LOCK(fnp, &fifolist_mutex);
		VN_HOLD(FTOV(fnp));
		FIFO_UNLOCK(fnp);
	}
	return FTOV(fnp);
}

/*
 * int
 * fifopreval(vtype_t, dev_t, cred_t *)
 *      Pre-validate attributes that will be passed to a
 *       subsequent fifovp() call.
 *
 * Calling/Exit State:
 *      No locks are held on entry or exit.
 *
 */
/*ARGSUSED*/
int
fifopreval(vtype_t type, dev_t dev, cred_t *cr)
{
	/*
         * fifovp() always succeeds, so return success here.
         */
        return 0;
}
/*
 * STATIC vnode_t * makepipe(cred_t *crp)
 *	Create one end of a pipe.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *	Called by pipe() and connld open routine.
 *	Since a new pipe is created and nobody can get hold of it yet,
 *	no locking is needed on the fifonode.
 *
 * Description:
 * 	Create a pipe end by...
 * 	allocating a vnode-fifonode pair, intializing the fifonode,
 * 	setting the ISPIPE flag in the fifonode and assigning a unique
 * 	ino to the fifonode.
 */
STATIC vnode_t *
makepipe(cred_t *crp)
{
	struct fifonode *fnp;
	vnode_t *newvp;

	fnp = (struct fifonode *)kmem_zalloc(sizeof(struct fifonode), KM_SLEEP);

	fnp->fn_rcnt = fnp->fn_wcnt = 1;
	fnp->fn_atime = fnp->fn_mtime = fnp->fn_ctime = hrestime.tv_sec;
	fnp->fn_flag |= ISPIPE;

	/* initialize locks and sync. variables */
	SLEEP_INIT(&fnp->fn_iolock, FIFO_HIER, &fiolock_lkinfo, KM_SLEEP);
	SV_INIT(&fnp->fn_rwait);
	SV_INIT(&fnp->fn_wwait);
	SV_INIT(&fnp->fn_fdwait);
	SV_INIT(&fnp->fn_openwait);

	newvp = FTOV(fnp);
	VN_INIT(newvp, fifovfsp, VFIFO, fifodev, 0, KM_SLEEP);

	/*
	 * The fifonode level is assigned the level of the calling process.
	 * The proper coding would be to pass in a credentials structure
	 * pointer as an argument to makepipe().  However, makepipe()
	 * is a well-defined standard interface.  Therefore, the
	 * credentials structure is retrieved directly from the u.
	 */
	newvp->v_lid = crp->cr_lid;
	newvp->v_op = &fifo_vnodeops;
	newvp->v_data = (caddr_t) fnp;
	newvp->v_macflag |= (VMAC_DOPEN | VMAC_SUPPORT);
	return newvp;
}

/*
 * int fifo_mkpipe(vnode_t **vpp1, vnode_t **vpp2, cred_t *crp)
 * 	create and setup a pipe.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *	Called by pipe() and connld open routine.
 *	Since a new pipe is created and nobody can get hold of it yet,
 *	no locking is needed on the fifonode.
 *
 * Description:
 * 	setup a pipe by connecting two streams together. Associate
 * 	each end of the pipe with a vnode and one of the streams.
 *	It is the responsibility of the caller to associate 
 * 	file pointers with the returned vnodes.
 */
int
fifo_mkpipe(vnode_t **vpp1, vnode_t **vpp2, cred_t *crp)
{
	vnode_t *vp1, *vp2;
	struct fifonode *fnp1, *fnp2;
	int error = 0;

	/*
	 * Allocate and initialize two vnodes. 
	 */
	vp1 = makepipe(crp);
#ifdef CC_PARTIAL
        MAC_ASSERT(vp1, MAC_SAME);  /* New pipes have same level as process. */
#endif
	vp2 = makepipe(crp);
#ifdef CC_PARTIAL
        MAC_ASSERT(vp2, MAC_SAME);  /* New pipes have same level as process. */
#endif

	/*
	 * Create two stream heads and attach to each vnode.
	 */
	if (error = fifo_stropen(vp1, 0, crp)) {
		VN_RELE(vp1);
		VN_RELE(vp2);
		return (error);
	}

	if (error = fifo_stropen(vp2, 0, crp)) {
		(void)strclose(vp1, 0, crp);
		VN_RELE(vp1);
		VN_RELE(vp2);
		return (error);
	}

	/*
	 * destroy one stream lock and make both lock pointers
	 * point to the same mutex.
	 */
	LOCK_DEALLOC(vp1->v_stream->sd_mutex);
	vp1->v_stream->sd_mutex = vp2->v_stream->sd_mutex;

	/*
	 * Twist the stream head queues so that the write queue
	 * points to the other stream's read queue.
	 */
	vp1->v_stream->sd_wrq->q_next = RD(vp2->v_stream->sd_wrq);
	vp2->v_stream->sd_wrq->q_next = RD(vp1->v_stream->sd_wrq);

	fnp1 = VTOF(vp1);
	fnp2 = VTOF(vp2);

	/*
	 * initialize shared pipe lock structure.
	 */
	fnp1->fn_nodelp =
		(struct nodelock *)kmem_zalloc(sizeof(struct nodelock),
		KM_SLEEP);
	fnp2->fn_nodelp = fnp1->fn_nodelp;
	LOCK_INIT(&fnp1->fn_nodelp->n_lock, FIFO_HIER, PLFIFO,
		&node_lkinfo, KM_SLEEP);
	fnp1->fn_nodelp->n_count = 2;

	/*
	 * Tell each pipe about its other half.
	 */
	fnp1->fn_mate = vp2;
	fnp2->fn_mate = vp1;
	fnp1->fn_ino = fnp2->fn_ino = fifogetid();
	*vpp1 = vp1;
	*vpp2 = vp2;
	return (0);
}

/*
 * void fifo_rmpipe(vnode_t *vp1, vnode_t *vp2, cred_t *crp)
 * 	External interface to dismantle the pipe ends.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *	Called by pipe() and connld open routine.
 *	Since a new pipe is created and nobody can get hold of it yet,
 *	no locking is needed on the fifonode.
 */
void
fifo_rmpipe(vnode_t *vp1, vnode_t *vp2, cred_t *crp)
{
        (void)strclose(vp1, 0, crp);
        (void)strclose(vp2, 0, crp);
        VN_RELE(vp1);
        VN_RELE(vp2);
}

/*
 * void fifoclearid(ino_t ino)
 * 	Release a pipe-ino.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 */
void
fifoclearid(ino_t ino)
{
	pl_t pl;

	pl = LOCK(&fifomap_mutex, PLFIFO);
	clearid(ino);
	UNLOCK(&fifomap_mutex, pl);
}

/*
 * ino_t fifogetid(void)
 * 	Establish a unique pipe id.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	Start searching the bit map where 
 * 	the previous search stopped. If a free bit is located,
 * 	set the bit and return the new position in the bit map.
 */
ino_t
fifogetid(void)
{
	ino_t i;
	ino_t j;
	pl_t pl;
	static ino_t prev = 0;

	pl = LOCK(&fifomap_mutex, PLFIFO);
#ifdef CC_PARTIAL
	/*
	 * If we're concerned about covert channels, start the id search
	 * at a random place rather than where the previous search stopped.
	 * If the random() routine is stubbed out, it will return 0,
	 * in which case we want to revert to the sequential method.
	 */
	if ((i = random((ulong)FIFOMAXID)) == 0)
#endif
		i = prev;

	for (j = FIFOMAXID; j--; ) {
		if (i++ >= (ino_t)FIFOMAXID)
			i = 1;

		if (!testid(i)) {
			setid(i);
			prev = i;
#ifdef CC_PARTIAL
			if (fifoids > (ino_t)(FIFOMAXID - RANDMINFREE))
				CC_COUNT(CC_RE_PIPE, CCBITS_RE_PIPE);
#endif /* CC_PARTIAL */
			UNLOCK(&fifomap_mutex, pl);
			return(i);
		}
	}

#ifdef CC_PARTIAL
	CC_COUNT(CC_RE_PIPE, CCBITS_RE_PIPE);
#endif /* CC_PARTIAL */
	UNLOCK(&fifomap_mutex, pl);
	/*
	 *+ A unique fifonode id cannot be established.
	 *+ This indicates the limit on number of pipes
	 *+ on the system has been reached.
	 */
	cmn_err(CE_WARN, "fifogetid: could not establish a unique node id\n");
	return(0);
}

/*
 * STATIC int fifo_stropen(vnode_t *vp, int flag, cred_t *crp)
 * 	Stream one end of a pipe,
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *	Called by setup_pipe() only.
 *	Since fifonode is  that of a newly created pipe 
 *	and nobody can get hold of it yet,
 *	no locking is needed on the fifonode.
 *
 * Description:
 *	Call stropen() to setup a stream.
 *
 */
STATIC int
fifo_stropen(vnode_t *vp, int flag, cred_t *crp)
{
	struct fifonode *fnp = VTOF(vp);
	struct stdata *stp;
	queue_t *wqp;
	dev_t pdev = 0;
	int error = 0;

	if ((error = stropen(vp, &pdev, NULLVPP, flag, crp)) != 0) {
		return error;
	}
	
	fnp->fn_open++;
	stp = vp->v_stream;
	stp->sd_flag |= OLDNDELAY;

	wqp = stp->sd_wrq;
	wqp->q_hiwat = 2 * fifoblksize;
	RD(wqp)->q_hiwat = 2 * fifoblksize;
	wqp->q_lowat = fifoblksize;
	RD(wqp)->q_lowat = fifoblksize;

	wqp->q_minpsz = 0;
	RD(wqp)->q_minpsz = 0;
	wqp->q_maxpsz = INFPSZ;
	RD(wqp)->q_maxpsz = INFPSZ;

	return(0);
}


/*
 * STATIC void fifoinsert(struct fifonode *fnp)
 *
 * Calling/Exit State:
 * 	fifolist_mutex must be locked on entry and remain locked on exit.
 *
 * Description:
 * 	Insert a fifonode-vnode pair onto the fifoalloc hash list.
 */
STATIC void
fifoinsert(struct fifonode *fnp)
{

	fnp->fn_backp = NULL;
	fnp->fn_nextp = fifoalloc;
	fifoalloc = fnp;
	if (fnp->fn_nextp)
		fnp->fn_nextp->fn_backp = fnp;
}

/*
 * STATIC struct fifonode * fifofind(vnode_t *vp)
 * 	Find a fifonode-vnode pair on the fifoalloc hash list. 
 *
 * Calling/Exit State:
 * 	fifolist_mutex must be locked on entry and remain locked on exit.
 *
 * Description:
 * 	vp is a vnode to be shadowed. If it's on the hash list,
 * 	it already has a shadow, therefore return its corresponding 
 * 	fifonode.
 */
STATIC struct fifonode *
fifofind(vnode_t *vp)
{
	struct fifonode *fnode;

	for (fnode = fifoalloc;  fnode;  fnode = fnode->fn_nextp)
		if (fnode->fn_realvp == vp) {
			return fnode;
		}
	return NULL;
}

/*
 * int fiforemove(struct fifonode *fnp)
 *
 * Calling/Exit State:
 * 	fifolist_mutex must be locked on entry and remain locked on exit.
 *
 * Description:
 * 	Remove a fifonode-vnode pair from the fifoalloc hash list.
 * 	This routine is called from the fifo_inactive() routine when a
 * 	FIFO is being released.
 * 	Return 0 if fnp is remove, -1 if fnp is not removed.
 */
int
fiforemove(struct fifonode *fnp)
{
	if (FIFO_LOCKBLKD(fnp)) {
		/* someone wants this node */
		return(-1);
	}
	if (fnp == fifoalloc)
		fifoalloc = fnp->fn_nextp;
	if (fnp->fn_nextp)
		fnp->fn_nextp->fn_backp = fnp->fn_backp;
	if (fnp->fn_backp)
		fnp->fn_backp->fn_nextp = fnp->fn_nextp;
	return(0);
}

/*
 * void
 * fifo_getsz(vnode_t *vp, vattr_t *vap)
 *
 * Calling/Exit State:
 *	vp->v_stream->sd_mutex cannot be held on entry.
 *	PIPE_LOCK must be held before calling this routine if it is
 *	possible for strclose to happen simutaneously, such as in
 *	stat() calls that are not done on file descriptors.
 *
 * Description:
 *	Size is the number of un-read bytes at the stream head and
 *	nblocks is the unread bytes expressed in blocks.
 */
void
fifo_getsz(vnode_t *vp, vattr_t *vap)
{
	pl_t	pl;
	queue_t	*qp;
	struct	qband	*bandp;
	
	if (vp->v_stream) {
		pl = STREAM_LOCK(vp->v_stream);
		qp = RD(vp->v_stream->sd_wrq);
		if (qp->q_nband == 0)
			vap->va_size = qp->q_count;
		else {
			for (vap->va_size = qp->q_count, bandp = qp->q_bandp; 
			     bandp; bandp = bandp->qb_next)
				vap->va_size += bandp->qb_count;
		}
		STREAM_UNLOCK(vp->v_stream, pl);
		vap->va_nblocks = btod(vap->va_size);
	} else {
		vap->va_size = 0;
		vap->va_nblocks = 0;
	}
}

/* XENIX Support */
/*
 * int fifo_rdchk(vnode_t *vp)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 * 	XENIX rdchk support.
 */
int
fifo_rdchk(vnode_t *vp)
{
	struct fifonode *fnp = VTOF(vp);

	if (vp->v_type != VFIFO || vp->v_op != &fifo_vnodeops)
		return(0);

	if (fnp->fn_flag & ISPIPE)
		/*
		 * If it's a pipe and the other end is still open,
		 * return 1. Otherwise, return 0.
		 */
		if (fnp->fn_mate)
			return(1);
		else
			return(0);
	else
		/*
		 * For non-pipe FIFO, return number of writers.
		 */
		return(fnp->fn_wcnt);
}
/* End XENIX Support */
