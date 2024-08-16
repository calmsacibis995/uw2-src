/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/streamio.c	1.99"
#ident	"$Header: $"

#include <util/types.h>
#include <acc/audit/audit.h>
#include <acc/audit/auditrec.h>
#include <acc/priv/privilege.h>
#include <util/sysmacros.h>
#include <util/param.h>
#include <util/ksynch.h>
#include <util/plocal.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <proc/bind.h>
#include <proc/signal.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <io/conf.h>
#include <util/debug.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/strsubr_f.h>
#include <io/strsubr_p.h>
#include <io/stropts.h>
#include <io/poll.h>
#include <io/termios.h>
#include <io/termio.h>
#include <io/ttold.h>
#include <util/inline.h>
#include <io/uio.h>
#include <util/cmn_err.h>
#include <io/sad/sad.h>
#include <proc/session.h>
#include <fs/ioccom.h>
#include <fs/filio.h>
#include <svc/clock.h>
#include <acc/mac/mac.h>
#include <acc/dac/acl.h>
#include <svc/secsys.h>
#include <mem/kmem.h>
#include <proc/proc_hier.h>
#include <util/mod/mod_hier.h>
#include <mem/memresv.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif


/* Following defines are used to optimize SV_BROADCASTs */
#define RBLOCK	0x1	/* reader is blocked */
#define WBLOCK	0x2	/* writer is blocked */
#define IBLOCK	0x4	/* ioctller is blocked */
#define OBLOCK	0x8	/* opener is blocked */
#define MBLOCK	0x10	/* ioctl mechanism is blocked */
#define TBLOCK	0x20	/* close/drainer is blocked */

/*
 * MACRO
 * STRKBIND(stp, engp, unbind)
 *	Bind the stream <stp> to engine <engp>.
 *
 * Calling/Exit State:
 *	<stp> is the stream that needs to be bound.
 *	<engp> is the engine to which the stream is to be bound.
 *	<unbind> is a flag which indicates a stream is either bound/unbound.
 */
#define STRKBIND(stp, engp, unbind) { \
		if ((stp)->sd_cpu) { \
			(engp) = kbind((stp)->sd_cpu); \
			DISABLE_PRMPT(); \
			u.u_lwpp->l_notrt++; \
			(unbind) = 1; \
		} \
}

/*
 * MACRO
 * STRKUNBIND(engp, unbind)
 *	Unbind the stream bound to engine <engp>.
 *
 * Calling/Exit State:
 *	<engp> is the engine to which the stream is to be bound.
 *	<unbind> is a flag which indicates a stream is either bound/unbound.
 */
#define STRKUNBIND(engp, unbind) { \
		if ((unbind)) { \
			ASSERT(u.u_lwpp->l_notrt != 0); \
			u.u_lwpp->l_notrt--; \
			ENABLE_PRMPT(); \
			kunbind((engp)); \
		} \
}

/*
 * MACRO
 * STRBIND(stp, engp, devp, unbind)
 *	Bind the stream <stp> to engine <engp>. Also save the
 *	device cdevsw table entry pointer for future-lwp-binding.
 *
 * Calling/Exit State:
 *	<stp> is the stream than needs to be bound.
 *	<engp> is the engine to which the stream is to be bound.
 *	<devp> is a pointer to the device number.
 *	<unbind> is a flag which indicates a stream is either bound/unbound.
 */
#define STRBIND(stp, engp, devp, unbind) { \
		if ((stp)->sd_cpu) { \
			u.u_lwpp->l_cdevswp = &cdevsw[getmajor((*devp))]; \
			(engp) = kbind((stp)->sd_cpu); \
			DISABLE_PRMPT(); \
			u.u_lwpp->l_notrt++; \
			(unbind) = 1; \
		} \
}

/*
 * MACRO
 * STRUNBIND(engp, unbind)
 *	Unbind the stream bound to engine <engp> and reset l_cdevswp.
 *
 * Calling/Exit State:
 *	<engp> is the engine to which the stream is to be bound.
 *	<unbind> is a flag which indicates a stream is either bound/unbound.
 */
#define STRUNBIND(engp, unbind) { \
		if ((unbind)) { \
			ASSERT(u.u_lwpp->l_notrt != 0); \
			u.u_lwpp->l_notrt--; \
			ENABLE_PRMPT(); \
			kunbind((engp)); \
			u.u_lwpp->l_cdevswp = NULL; \
		} \
}


/*
 * id value used to distinguish between different ioctl messages
 */
STATIC long ioc_id;

#ifdef DEBUG
STATIC long clnopen_restartcnt = 0;  /* number of non-cloning open restarts */
#endif /*DEBUG*/

STATIC int strrput(queue_t *, mblk_t *);
STATIC void strpipeput(queue_t *, mblk_t *);
STATIC int strsink(queue_t *, mblk_t *);
STATIC int strwsrv(queue_t *);
int strclose(struct vnode *, int, cred_t *);
int strmodpushed(stdata_t *, char *);
STATIC int strrecvfd(stdata_t *, cred_t *, int, int, int, int, pl_t);
STATIC int strfdinsert(stdata_t *, int, int, int);
STATIC int strpeek(stdata_t *, int, int, int *);
int strdoioctl(stdata_t *, struct strioctl *, mblk_t *, int, char *, cred_t *, int *);
extern int drv_priv(void *);
extern void spec_makestrcln(vnode_t *, dev_t, vnode_t **);
extern vnode_t *common_specvp(vnode_t *);

/*
 *  Qinit structure and Module_info structures
 *        for stream head read and write queues
 */

STATIC struct module_info strm_info = { 0, "strrhead", 0, INFPSZ, STRHIGH, STRLOW };
STATIC struct module_info stwm_info = { 0, "strwhead", 0, 0, 0, 0 };
struct	qinit strdata = { strrput, NULL, NULL, NULL, NULL, &strm_info, NULL };
struct	qinit stwdata = { NULL, strwsrv, NULL, NULL, NULL, &stwm_info, NULL };
STATIC struct qinit deadrend = {
	strsink, NULL, NULL, NULL, NULL, &strm_info, NULL
};
STATIC struct qinit deadwend = {
	NULL, NULL, NULL, NULL, NULL, &stwm_info, NULL
};

extern struct streamtab fifoinfo;
extern lock_t autoh_mutex;
extern lock_t strsig_mutex;

/*
 * Tunable parameters set in kernel.cf and used only in this file
 */
extern long strthresh;	/* Strcount threshold above which some streams */
			/* operations will be stopped */
extern int strmsgsz;	/* maximum stream message size */
extern int strctlsz;	/* maximum size of ctl part of message */
extern int Nengine;	/* number of physical processors */

extern rwlock_t mod_fmodsw_lock;

/*
 * int
 * stropen(struct vnode *vp, dev_t *devp, struct vnode **vpp,
 *					int flag, cred_t *crp)
 *	Open a stream device.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.  Assumes vstr_mutex unlocked.  Assumes
 *	autoh_mutex unlocked.  To close a race involving cloning opens, this
 *	routine must detect that a clone has happened and tell specfs.  This
 *	is done by setting/unsetting v_stream.  If v_stream is set, a clone
 *	open did not take place.  If v_stream is NULL,
 *	then a clone open happened and *vpp points at the new
 *	vnode created for the new cloned device.
 */
int
stropen(struct vnode *vp, dev_t *devp, struct vnode **vpp, int flag, cred_t *crp)
{
	struct stdata *stp;
	queue_t *qp;
	struct autopush *ap;
	queue_t *tqp;
	qband_t *qbp;
	struct engine *engp;
	pl_t pl;
	dev_t dummydev;
	dev_t olddev;
	struct autopush autostr;
	int error;
	int unbind;
	int idx;
	int freed;
	int s;
	uchar_t *rqbf;
	uchar_t *wqbf;
	int renab;
	int wenab;
	int nrband;
	int nwband;
	int i;
	int broadcast;

	error = 0;
	unbind = 0;
	broadcast = 0;
	freed = 0;
	olddev = *devp;

	/*
	 * If the stream already exists, wait for any open in progress
	 * to complete, then call the open function of each module and
	 * driver in the stream.  Otherwise create the stream.  Note: by
	 * virtue of being here, the vnode can not disappear out from under
	 * us.
	 */
retry:
	pl = LOCK(&vstr_mutex, PLSTR);
	if ((stp = vp->v_stream) != NULL) {
		UNLOCK(&vstr_mutex, pl);
		/* Check if an open is in progress */
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & STWOPEN) {
			if (flag & (FNDELAY|FNONBLOCK)) {
				UNLOCK(stp->sd_mutex, pl);
				return(EAGAIN);
			}
			/* note: SV_WAIT_SIG returns at pl0 with no lock held */
			if (SV_WAIT_SIG(stp->sd_open, PRIMED, stp->sd_mutex) == B_FALSE) {
				return(EINTR);
			}
			goto retry;  /* could be clone! */
		}

		if (stp->sd_flag & (STRDERR|STWRERR)) {
			UNLOCK(stp->sd_mutex, pl);
			return(EIO);
		}

		stp->sd_flag |= STWOPEN;

		/*
		 * Since this is a plumbing operation, it must serialize with
		 * other plumbers.
		 */

		UNLOCK(stp->sd_mutex, pl);
		if (SLEEP_LOCK_SIG(stp->sd_plumb, PRIMED) == B_FALSE) {
			pl = LOCK(stp->sd_mutex, PLSTR);
			stp->sd_flag &= ~STWOPEN;
			/* wake up anybody waiting to open */
			if (SV_BLKD(stp->sd_open)) {
				UNLOCK(stp->sd_mutex, pl);
				SV_BROADCAST(stp->sd_open, 0);
			} else {
				UNLOCK(stp->sd_mutex, pl);
			}
			return(EINTR);
		}

		pl = LOCK(stp->sd_mutex, PLSTR);

		/* check for errors while we were asleep */
		if (stp->sd_flag & (STRDERR|STWRERR)) {
			stp->sd_flag &= ~STWOPEN;
			/* wake anyone who might have blocked */
			SLEEP_UNLOCK(stp->sd_plumb);
			if (SV_BLKD(stp->sd_open))
				broadcast |= OBLOCK;
			if (SV_BLKD(stp->sd_timer2))
				broadcast |= MBLOCK;

			UNLOCK(stp->sd_mutex, pl);
			if (broadcast) {
				if (broadcast & OBLOCK)
					SV_BROADCAST(stp->sd_open, 0);
				if (broadcast & MBLOCK)
					SV_BROADCAST(stp->sd_timer2, 0);
			}

			return(EIO);
		}

		/*
		 * Open all modules and devices down stream to notify
		 * that another user is streaming.  For modules, set the
		 * last argument to MODOPEN and do not pass any open flags.
		 * Ignore dummydev since this is not the first open.
		 */
		qp = stp->sd_wrq;
		UNLOCK(stp->sd_mutex, pl);
		/*
		 * At this point, we still hold sd_plumb, so the stream can
		 * not change configuration, thus sd_cpu is invariant.  Also,
		 * since we hold sd_plumb, we don't have to bother setting
		 * UPBLOCK. 
		 */

		STRBIND(stp, engp, devp, unbind);

		pl = splstr();	/* for compatibility, open expects this */
		while (SAMESTR(qp)) {
			if (qp->q_next->q_qinfo->qi_minfo->mi_idnum == UNI_ID)
				qp = qp->q_next->q_ptr;
			else
				qp = qp->q_next;
			if (qp->q_flag & QREADR)
				break;
			dummydev = *devp;
			if (error=((*RD(qp)->q_qinfo->qi_qopen)(RD(qp),
			    &dummydev, flag, (qp->q_next ? MODOPEN : 0), crp)))
				break;
		}
		splx(pl);

		STRUNBIND(engp, unbind);

		pl = LOCK(stp->sd_mutex, PLSTR);
		stp->sd_flag &= ~(STRHUP|STWOPEN);
		stp->sd_rerror = 0;
		stp->sd_werror = 0;
		LOCK(&vstr_mutex, PLSTR);
		if (vpp != NULLVPP)
			(*vpp)->v_stream = stp;
		UNLOCK(&vstr_mutex, PLSTR);
		/*
		 * Controlling tty allocation handled here, SVR4 allowed
		 * reopens to set up a controlling tty in specfs, so we
		 * still allow this behavior.
		 */
		if (stp->sd_flag & STRISTTY) {
			/*
			 * this is a tty driver - try to allocate it as a
			 * controlling terminal
			 */
			if (!(flag & FNOCTTY))
				stralloctty(stp);
		}
		SLEEP_UNLOCK(stp->sd_plumb);
		if (SV_BLKD(stp->sd_open))
			broadcast |= OBLOCK;
		if (SV_BLKD(stp->sd_timer2))
			broadcast |= MBLOCK;
		UNLOCK(stp->sd_mutex, pl);

		if (broadcast) {
			if (broadcast & OBLOCK)
				SV_BROADCAST(stp->sd_open, 0);
			if (broadcast & MBLOCK)
				SV_BROADCAST(stp->sd_timer2, 0);
		}
		return(error);
	}
	UNLOCK(&vstr_mutex, pl);

	/*
	 * This is a first open.
	 * firewall - don't use too much memory
	 */
	if (strthresh && (Strcount > strthresh) && (drv_priv(crp) == EPERM)) {
		return(ENOSR);
	}

	if (!mem_resv_check())
		return(ENOSR);
	qp = allocq();
	stp = shalloc(qp);
	pl = LOCK(&vstr_mutex, PLSTR);

	/*
	 * We could have slept in the above allocations.
	 * Check vp->v_stream again to make sure nobody
	 * came in and out and opened the stream before us.
	 * There is no way to get to the just-allocated stream
	 * head unless v_stream is set.
	 */
	if (vp->v_stream) {
		UNLOCK(&vstr_mutex, pl);
		shfree(stp, 1);
		freeq(qp);
		goto retry;
	}

#ifdef CC_PARTIAL
        /*
         * We don't know that the vnode is really at the level of
         * the process, but it should be at the same level as the
         * stream being created, and this MAC_ASSUME() ensures
         * that the CCA tool figures this out.
         */
        MAC_ASSUME (vp, MAC_SAME);
#endif

	stp->sd_vnode = vp;
	setq(qp, &strdata, &stwdata);
	qp->q_ptr = (caddr_t)stp;
	WR(qp)->q_ptr = (caddr_t)stp;
	/* no write side put procedure, but this NULLs it instead of putnext */
	qp->q_putp = qp->q_qinfo->qi_putp;
	WR(qp)->q_putp = WR(qp)->q_qinfo->qi_putp;
	qp->q_flag |= QPROCSON;
	WR(qp)->q_flag |= QPROCSON;
	vp->v_stream = stp;
	UNLOCK(&vstr_mutex, pl);
	/*
	 * At this point, the linkage between the vnode and the stream is
	 * complete and STWOPEN is set, which will block any opens on this
	 * device.  If 2 or more opens raced to the above check, only 1 will
	 * win the race to set v_stream and the others will retry.
	 *
	 * This is a first open so no one else can be referencing the
	 * stream head just allocated, except for a subsequent open, which
	 * only looks at sd_flag and sd_open so no locking is necessary here.
	 * Note that the driver hasn't been attached yet so we can't get
	 * interrupts either.
	 */
	if (vp->v_type == VFIFO) {
		stp->sd_strtab = &fifoinfo;
		if (vpp != NULLVPP)
			(*vpp)->v_stream = stp;
		goto opendone;
	}

	/*
	 * Open driver and create stream to it (via qattach).  Device
	 * opens may sleep, but must set PCATCH if they do so that
	 * signals will not cause a longjmp.  Failure to do this may
	 * result in the queues and stream head not being freed.
	 */
	idx = getmajor(*devp);
	dummydev = *devp;
	if (error = qattach(qp, devp, flag, CDEVSW, idx, crp, 0))
		goto opendone;
	/* point to the correct table, could have been loaded */
	stp->sd_strtab = cdevsw[getmajor(*devp)].d_str;
	if (vpp != NULLVPP)
		(*vpp)->v_stream = stp;
	/*
	 * check for autopush, at this point, we have a "live" stream head
	 * and must start locking.
	 */
	pl = LOCK(&autoh_mutex, PLSTR);
	ap = strphash(getemajor(*devp));
	while (ap) {
		if (ap->ap_major == getemajor(*devp)) {
			if (ap->ap_type == SAP_ALL)
				break;
			else if ((ap->ap_type == SAP_ONE) &&
				 (ap->ap_minor == geteminor(*devp)))
					break;
			else if ((ap->ap_type == SAP_RANGE) &&
				 (geteminor(*devp) >= ap->ap_minor) &&
				 (geteminor(*devp) <= ap->ap_lastminor))
					break;
		}
		ap = ap->ap_nextp;
	}
	if (!ap) {
		UNLOCK(&autoh_mutex, pl);
		goto opendone;
	}
	/*
	 * copy the autopush info we just found into a local spot so we
	 * can drop the lock.  Things may change after this - we use a
	 * snapshot of what we found.
	 */
	autostr = *ap;	/* structure copy */
	UNLOCK(&autoh_mutex, pl);
	ap = &autostr;

	for (s = 0; s < ap->ap_npush; s++) {
		rqbf = kmem_zalloc(NBAND, KM_SLEEP);
		wqbf = kmem_zalloc(NBAND, KM_SLEEP);
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & (STRHUP|STRDERR|STWRERR)) {
			error = (stp->sd_flag & STRHUP) ? ENXIO : EIO;
			UNLOCK(stp->sd_mutex, pl);
			(void) strclose(vp, flag, crp);
			freed++;
			kmem_free((caddr_t)rqbf, NBAND);
			kmem_free((caddr_t)wqbf, NBAND);
			goto opendone;
		}
		if (stp->sd_pushcnt >= nstrpush) {
			UNLOCK(stp->sd_mutex, pl);
			(void) strclose(vp, flag, crp);
			freed++;
			error = EINVAL;
			kmem_free((caddr_t)rqbf, NBAND);
			kmem_free((caddr_t)wqbf, NBAND);
			goto opendone;
		}

		/*
 		 * push new module and call its open routine via qattach
		 * First, remember the flow control state for later
 		 */
		tqp = RD(stp->sd_wrq);
		renab = 0;
		if (tqp->q_flag & QWANTW) {
			renab = 1;
			rqbf[0] = 1;
		}
		nrband = (int)tqp->q_nband;
		for (i = 1, qbp = tqp->q_bandp; i <= nrband; i++) {
			if (qbp->qb_flag & QB_WANTW) {
				renab = 1;
				rqbf[i] = 1;
			}
			qbp = qbp->qb_next;
		}
		tqp = stp->sd_wrq->q_next;
		for ( ; tqp && !tqp->q_qinfo->qi_srvp; tqp = tqp->q_next)
			;
		wenab = 0;
		nwband = 0;
		if (tqp) {
			if (tqp->q_flag & QWANTW) {
				wenab = 1;
				wqbf[0] = 1;
			}
			nwband = (int)tqp->q_nband;
			for (i = 1, qbp = tqp->q_bandp; i <= nwband; i++) {
				if (qbp->qb_flag & QB_WANTW) {
					wenab = 1;
					wqbf[i] = 1;
				}
				qbp = qbp->qb_next;
			}
		}
		UNLOCK(stp->sd_mutex, pl);
		if (error = qattach(qp, &dummydev, 0, FMODSW, ap->ap_list[s], crp, 0)) {
			(void) strclose(vp, flag, crp);
			freed++;
			kmem_free((caddr_t)rqbf, NBAND);
			kmem_free((caddr_t)wqbf, NBAND);
			goto opendone;
		}
		pl = LOCK(stp->sd_mutex, PLSTR);
		stp->sd_pushcnt++;
		/*
		 * If flow control is on, don't break it - enable
		 * first back queue with svc procedure.
		 */
		tqp = RD(stp->sd_wrq->q_next);
		if (tqp->q_qinfo->qi_srvp && renab) {
			tqp = backq_l(tqp);
			for (;tqp && !tqp->q_qinfo->qi_srvp; tqp = backq_l(tqp))
				;
			if (tqp) {
				qenable_l(tqp);
				for (i = 0; i <= nrband; i++) {
					if (rqbf[i])
						setqback(tqp, i);
				}
			}
		}
		tqp = stp->sd_wrq->q_next;
		if (tqp->q_qinfo->qi_srvp) {
			if (wenab) {
				qenable_l(stp->sd_wrq);
				for (i = 0; i <= nwband; i++) {
					if (wqbf[i])
					    setqback(stp->sd_wrq, i);
				}
			}
		}
		UNLOCK(stp->sd_mutex, pl);
		kmem_free((caddr_t)rqbf, NBAND);
		kmem_free((caddr_t)wqbf, NBAND);
	} /* for */

opendone:

	/*
	 * Wake up others that are waiting for stream to be created.
	 * At this point, data could be coming up into the stream head
	 * from below.  Handle the error case carefully.  Need to null
	 * out v_stream before waking to force waiting openers onto the
	 * "correct" path, i.e. such that the to-be-freed stream head isn't
	 * referenced.
	 */

	if (error) {
		if (!freed) {
			pl = LOCK(&vstr_mutex, PLSTR);
			vp->v_stream = NULL;
			if (vpp != NULLVPP)
				(*vpp)->v_stream = NULL;
			UNLOCK(&vstr_mutex, pl);
			/* clean up any debris */
			qprocsoff(qp);
			/*
			 * probably don't need to muck with sd_vnode and
			 * sd_flag, since clearing v_stream will cause all
			 * references to this stream head to disappear and
			 * we're about to free it anyhow
			 */
			pl = LOCK(stp->sd_mutex, PLSTR);
			stp->sd_vnode = NULL;
			stp->sd_flag &= ~STWOPEN;
			if (SV_BLKD(stp->sd_open)) {
				UNLOCK(stp->sd_mutex, pl);
				SV_BROADCAST(stp->sd_open, 0);
			} else {
				UNLOCK(stp->sd_mutex, pl);
			}
			shfree(stp, 1);
			freeq(qp);
		}
#ifdef DEBUG
		if (error == ECLNRACE) {
			pl = LOCK(&vstr_mutex, PLSTR);
			clnopen_restartcnt++;
			UNLOCK(&vstr_mutex, pl);
		}
#endif /*DEBUG*/
		return(error);
	}
	if (olddev != *devp) {
		struct vnode *cvp;
		/*
		 * the open cloned on us, tell specfs.  We need to do this
		 * here to close a window between the return and the shuffling
		 * of the vnodes.
		 */
		pl = LOCK(&vstr_mutex, PLSTR);
		vp->v_stream = NULL;
		(*vpp)->v_stream = NULL;
		UNLOCK(&vstr_mutex, pl);
		/*
		 * the open cloned on us; create snodes for new
		 * device (*devp) and have the stream head point to
		 * common vnode in the new common snode.
		 * spec_mskestrcln() returns the pointer to the
		 * new vnode pointer in vpp. The exclusive
		 * lock on the new common snode is held on return.
		 *
		 * NOTE: For cloning opens the exlcusive lock
		 *	 on new common snode is held on return
		 *	 from stropen.
		 */
		(void)spec_makestrcln(vp, *devp, vpp);
		cvp = common_specvp(*vpp);
		pl = LOCK(stp->sd_mutex, PLSTR);
		/*
		 * STREAMS clones inherit fsid and stream
		 */
		LOCK(&vstr_mutex, PLSTR);
		cvp->v_stream = stp;
		(*vpp)->v_stream = stp;
		UNLOCK(&vstr_mutex, PLSTR);
		stp->sd_vnode = cvp;
		stp->sd_strtab = cdevsw[getmajor(*devp)].d_str;
	} else
		pl = LOCK(stp->sd_mutex, PLSTR);
	/*
	 * Controlling tty allocation handled here, remember FNOCTTY for I_PUSH
	 */
	if (flag & FNOCTTY)
		stp->sd_flag |= STRNOCTTY;

	if (stp->sd_flag & STRISTTY) {
		/*
		 * this is a tty driver - try to allocate it as a
		 * controlling terminal
		 */
		if (!(flag & FNOCTTY))
			stralloctty(stp);
	}
	/*
	 * Release pending openers
	 */
	stp->sd_flag &= ~STWOPEN;
	if (SV_BLKD(stp->sd_open)) {
		UNLOCK(stp->sd_mutex, pl);
		SV_BROADCAST(stp->sd_open, 0);
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}
	return(0);
}

/*
 * int
 * strclose(struct vnode *vp, int flag, cred_t *crp)
 *	Close a stream.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.  Assumes vstr_mutex unlocked.  Upon
 *	return, the stream has been dismantled.
 *
 * Description:
 *	This is called from closef() on the last close of an open stream.
 *	If there is still something on the siglist (from an lwp in this process
 *	that has not exited yet, clean it up.  Remove all multiplexor links
 *	for the stream, pop all the modules (and the driver), and free the
 *	stream structure.
 */

int
strclose(struct vnode *vp, int flag, cred_t *crp)
{
	pl_t pl;
	pl_t pls;
	queue_t *qp;
	queue_t *rq;
	stdata_t *stp;
	struct lwp *lwp;
	struct qsvc *svcp;
	struct strevent *sep;
	struct strevent *psep;
	struct strevent *tsep;
	struct strevent *savesep;
	qband_t *qbp;
	toid_t id;
	int rval;
	int unbind;
	struct engine *engp;
	int freemutex;

	unbind = 0;
	freemutex = 1;
	stp = vp->v_stream;
	pls = LOCK(&strsig_mutex, PLSTR);
	pl = LOCK(stp->sd_mutex, PLSTR);
	if (stp->sd_siglist) {
		/* only have to pull these off the chains */
		sep = stp->sd_siglist;
		while (sep) {
			savesep = sep->se_next;
			lwp = sep->se_lwpp;
			psep = NULL;
			tsep = sep->se_lwpp->l_sep;
			while (tsep != sep) {
				psep = tsep;
				tsep = tsep->se_chain;
				ASSERT(tsep);
			}
			if (psep)
				psep->se_chain = tsep->se_chain;
			else
				sep->se_lwpp->l_sep = tsep->se_chain;
			sefree(sep);
			sep = savesep;
		}
		stp->sd_siglist = NULL;
	}
	/*
	 * for the sake of clarity, drop the locks and reacquire the one we
	 * need (instead of dropping in a different order than acquired
	 */
	UNLOCK(stp->sd_mutex, pl);
	UNLOCK(&strsig_mutex, pls);
	pl = LOCK(stp->sd_mutex, PLSTR);
	qp = stp->sd_wrq;

	ASSERT(stp->sd_pollist->ph_list == NULL);

        /* freectty should have been called by now */
	ASSERT(stp->sd_sessp == NULL);
	ASSERT(stp->sd_pgidp == NULL);

	stp->sd_flag |= STWOPEN;

	stp->sd_flag &= ~(STRDERR|STWRERR);	/* help unlink succeed */
	stp->sd_rerror = 0;
	stp->sd_werror = 0;
	/*
	 * STWOPEN protects us from subsequent opens.  Nothing else can
	 * happen because no new references can occur.
	 */
	UNLOCK(stp->sd_mutex, pl);
	(void) munlinkall(stp, LINKCLOSE|LINKNORMAL, crp, &rval);

	STRBIND(stp, engp, &vp->v_rdev, unbind);

	while (SAMESTR(qp)) {
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (!(flag & (FNDELAY|FNONBLOCK)) && (stp->sd_closetime > 0)) {
			id = itimeout(strtime, (caddr_t)stp, stp->sd_closetime, PLSTR);
			if (id == 0) {
				UNLOCK(stp->sd_mutex, pl);
				goto nograce;
			}
			stp->sd_flag |= (STRTIME | WSLEEP);

			/*
			 * sleep until awakened by strwsrv() or strtime()
			 */
			while ((stp->sd_flag & STRTIME) && qp->q_next->q_first) {
				stp->sd_flag |= WSLEEP;
				/* ensure strwsrv gets enabled */
				qp->q_next->q_flag |= QWANTW;
				/* note: SV_WAIT_SIG returns at pl0 with no lock held */
				if (SV_WAIT_SIG(stp->sd_timer, PRIMED, stp->sd_mutex) == B_FALSE) {
					LOCK(stp->sd_mutex, PLSTR);
					break;
				}
				LOCK(stp->sd_mutex, PLSTR);
			}
			stp->sd_flag &= ~(STRTIME | WSLEEP);
			UNLOCK(stp->sd_mutex, pl);
			untimeout(id);
		} else {
			UNLOCK(stp->sd_mutex, pl);
		}
nograce:
		qdetach(RD(qp->q_next), 1, flag, crp);
	}

	pl = LOCK(stp->sd_mutex, PLSTR);
	/*
	 * The following nearly duplicates qprocsoff, but because of pipes,
	 * we have to do things a little differently.  Note, stream may
	 * no longer be bound, that's why we need to recheck.
	 */

	rq = RD(qp);
	if (stp->sd_cpu) {
		svcp = &l.qsvc;
	} else {
		svcp = &qsvc;
		LOCK(&svc_mutex, PLSTR);
	}
	if (qp->q_svcflag & QENAB) {
		svc_dequeue(qp, svcp);
		qp->q_svcflag &= ~QENAB;
	}
	if (rq->q_svcflag & QENAB) {
		svc_dequeue(rq, svcp);
		rq->q_svcflag &= ~QENAB;
	}
	if (stp->sd_cpu == NULL)
		UNLOCK(&svc_mutex, PLSTR);
	freezeprocs(rq);
	if (qp->q_next && (qp->q_next == rq)) {
		/* need special handling for fifos, don't allow back-enabling */
		rq->q_flag &= ~QWANTW;
		for (qbp = rq->q_bandp; qbp; qbp = qbp->qb_next)
			qbp->qb_flag &= ~QB_WANTW;
	}
	flushq_l(qp, FLUSHALL);
	flushq_l(rq, FLUSHALL);

	/*
	 * If the write queue of the stream head is pointing to a
	 * read queue, we have a twisted stream.  If the read queue
	 * is alive, convert the stream head queues into a dead end.
	 * If the read queue is dead, free the dead pair.
	 */
	if (qp->q_next && !SAMESTR_l(qp)) {
		if (qp->q_next->q_qinfo == &deadrend) {
			/* half-closed pipe, toss queues */
			freeq(qp->q_next);
			freeq(rq);
		} else if (qp->q_next == rq) {
			/* fifo, toss queue */
			freeq(rq);
		} else {
			/* pipe, need to keep queues around as message sink */
			qp->q_qinfo = &deadwend;
			rq->q_qinfo = &deadrend;
			rq->q_putp = rq->q_qinfo->qi_putp;
			/*
			 * save q_str because we need to use q_str->sd_mutex
			 * for putnext and qreply
			 */
			qp->q_str = qp->q_next->q_str;
			rq->q_str = qp->q_str;
			freemutex = 0;
		}
	} else {
		freeq(rq); /* free stream head queue pair */
	}

	if (stp->sd_iocblk) {
#ifdef STRPERF
		stp->sd_iocblk->b_sh += castimer() - stp->sd_iocblk->b_stamp;
#endif
		freemsg(stp->sd_iocblk);
		stp->sd_iocblk = NULL;
	}
	stp->sd_vnode = NULL;
	LOCK(&vstr_mutex, PLSTR);
	vp->v_stream = NULL;
	UNLOCK(&vstr_mutex, PLSTR);
	stp->sd_flag &= ~STWOPEN;
	if (SV_BLKD(stp->sd_open)) {
		UNLOCK(stp->sd_mutex, pl);
		SV_BROADCAST(stp->sd_open, 0);
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}

	STRUNBIND(engp, unbind);

	shfree(stp, freemutex);
	return(0);
}

/*
 * void
 * strclean(struct vnode *vp, boolean_t lockheld)
 *	Clean up after a process when it closes a stream.  This is called
 *	from closef for all closes, whereas strclose is called only for the
 *	last close on a stream.  The siglist is scanned for entries for the
 *	current lwp, and these are removed.  This routine is also called
 *	when an lwp exits.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked, strsig_mutex may be held.
 */

void
strclean(struct vnode *vp, boolean_t lockheld)
{
	struct strevent *sep;
	struct strevent *psep;
	struct strevent *tsep;
	struct strevent *savesep;
	pl_t pl;
	pl_t pls;
	struct stdata *stp;
	int update;

	update = 0;
	/*
	 * don't need to grab vstr_mutex here since v_stream is invariant at
	 * this point.
	 */
	stp = vp->v_stream;
	psep = NULL;
	if (!lockheld)
		pls = LOCK(&strsig_mutex, PLSTR);
	pl = LOCK(stp->sd_mutex, PLSTR);
	sep = stp->sd_siglist;
	while (sep) {
		if (sep->se_lwpp == u.u_lwpp) {
			/* Note, branch is only entered once */
			savesep = sep->se_next;
			if (psep)
				psep->se_next = savesep;
			else
				stp->sd_siglist = savesep;
			/* remove from lwp chain */
			psep = NULL;
			tsep = sep->se_lwpp->l_sep;
			while (tsep != sep) {
				psep = tsep;
				tsep = tsep->se_chain;
				ASSERT(tsep);
			}
			if (psep)
				psep->se_chain = tsep->se_chain;
			else
				sep->se_lwpp->l_sep = tsep->se_chain;
			sefree(sep);
			update = 1;
			sep = savesep;
		} else {
			psep = sep;
			sep = sep->se_next;
		}
	}
	if (update) {
		stp->sd_sigflags = 0;
		for (sep = stp->sd_siglist; sep; sep = sep->se_next)
			stp->sd_sigflags |= sep->se_events;
		update = 0;
	}
	UNLOCK(stp->sd_mutex, pl);
	if (!lockheld)
		UNLOCK(&strsig_mutex, pls);
}

/*
 * void
 * strlwpclean(struct lwp *lwpp)
 *	Clean up after an lwp when it exits.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex and strsig_mutex not held.
 *
 * Description:
 *	An lwp may exit without calling close.  This routine runs down a
 *	chain of strevents associated with this lwp removing them from
 *	the siglist.  strsig_mutex serializes updates to the se_chain list.
 *	It is held throughout.  strclose will hold this lock to remove
 *	vestige strevents from lwps registered on a stream head that have
 *	not exited.  No lock is needed for walking the siglist because by
 *	virtue of walking the list, the stream can not be closed, which is
 *	the problem case this lock handles.
 */

void
strlwpclean(struct lwp *lwpp)
{
	pl_t pl;
	struct strevent *sep;

	pl = LOCK(&strsig_mutex, PLSTR);
	sep = lwpp->l_sep;
	while (sep) {
		strclean(sep->se_vp, B_TRUE);
		/*
		 * Since strclean has gotten the head of the l_sep list,
		 * it will always remove the strevent pointed at by l_sep
		 * and will reset it to the next in line, thus there is no
		 * potential for an infinite loop
		 */
		sep = lwpp->l_sep;
	}
	UNLOCK(&strsig_mutex, pl);
}


/*
 * int
 * strread(struct vnode *vp, struct uio *uiop, cred_t *crp)
 *	Read a stream according to the mode flags in sd_flag:
 *
 *	(default mode)              - Byte stream, msg boundries are ignored
 *	RMSGDIS (msg discard)       - Read on msg boundries and throw away
 *	                              any data remaining in msg
 *	RMSGNODIS (msg non-discard) - Read on msg boundries and put back
 *				      any remaining data on head of read queue
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 *
 * Description:
 *	Consume readable messages on the front of the queue until u.u_count
 *	is satisfied, the readable messages are exhausted, or a message
 *	boundary is reached in a message mode.  If no data was read and
 *	the stream was not opened with the NDELAY flag, block until data
 *	arrives.  Otherwise return the data read and update the count.
 *
 *	In default mode a 0 length message signifies end-of-file and terminates
 *	a read in progress.  The 0 length message is removed from the queue
 *	only if it is the only message read (no data is read).
 *
 *	An attempt to read an M_PROTO or M_PCPROTO message results in an
 *	EBADMSG error return, unless either RDPROTDAT or RDPROTDIS are set.
 *	If RDPROTDAT is set, M_PROTO and M_PCPROTO messages are read as data.
 *	If RDPROTDIS is set, the M_PROTO and M_PCPROTO parts of the message
 *	are unlinked from and M_DATA blocks in the message, the protos are
 *	thrown away, and the data is read.
 */

/* ARGSUSED */
int
strread(struct vnode *vp, struct uio *uiop, cred_t *crp)
{
	pl_t pl;
	stdata_t *stp;
	mblk_t *bp;
	mblk_t *nbp;
	int n;
	int done;
	int error;
	int broadcast;
	char rflg;
	char waitflag;
	short mark;
	short delim;
	unsigned char pri;
#ifdef STRPERF
	int stamp;
#endif

	done = 0;
	broadcast = 0;
	error = 0;
	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	pl = LOCK(stp->sd_mutex, PLSTR);
loop:
	if (stp->sd_flag & (STRDERR|STPLEX)) {
		error = (stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror;
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}
	switch (error = straccess(stp, JCREAD)) {
	case CTTY_STOPPED:		/* LWP was stopped */
		goto loop;
	case CTTY_EOF:			/* Return end-of-file */
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	case CTTY_OK:			/* read */
		break;
	default:			/* error */
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}

	/*
	 * Loop terminates when uiop->uio_resid == 0.
	 */
	rflg = 0;
	for (;;) {
		mark = 0;
		delim = 0;
		waitflag = 0;
		while (!(bp = getq_l(RD(stp->sd_wrq)))) {
			if (stp->sd_flag & STRHUP) {
				UNLOCK(stp->sd_mutex, pl);
				return(error);
			}
			if (rflg) {
				if (stp->sd_flag & STRDELIM) {
					waitflag = NOINTR;
				} else {
					UNLOCK(stp->sd_mutex, pl);
					return(error);
				}
			}

			/*
			 * if FIFO/pipe, don't sleep here. Sleep in the
			 * fifo read routine.
			 */
			if (vp->v_type == VFIFO) {
				UNLOCK(stp->sd_mutex, pl);
				return(ESTRPIPE);
			}

			error = strwaitq(stp, READWAIT|waitflag, uiop->uio_resid,
					 uiop->uio_fmode, &done);
			if (error || done) {
				/* strwaitq drops lock, recheck errors */
				if (stp->sd_flag & (STRDERR|STPLEX)) {
					error = (stp->sd_flag & STPLEX) ?
						EINVAL : stp->sd_rerror;
					UNLOCK(stp->sd_mutex, pl);
					return(error);
				}
				if ((uiop->uio_fmode & FNDELAY) &&
				    (stp->sd_flag & OLDNDELAY) &&
				    (error == EAGAIN))
					error = 0;
				UNLOCK(stp->sd_mutex, pl);
				return(error);
			}
			/* strwaitq drops lock, recheck errors */
			if (stp->sd_flag & (STRDERR|STPLEX)) {
				error = (stp->sd_flag & STPLEX) ?
					EINVAL : stp->sd_rerror;
				UNLOCK(stp->sd_mutex, pl);
				return(error);
			}
		}
#ifdef STRPERF
		stamp = castimer();
#endif
		if (stp->sd_mark == bp) {
			if (rflg) {
#ifdef STRPERF
				bp->b_sh += castimer() - stamp;
#endif
				putbq_l(RD(stp->sd_wrq), bp);
				UNLOCK(stp->sd_mutex, pl);
				return(error);
			}
			mark = 1;
			stp->sd_mark = NULL;
		}
		if ((stp->sd_flag & STRDELIM) && (bp->b_flag & MSGDELIM))
			delim = 1;

		pri = bp->b_band;

		/* sd_mutex still locked at this point */
		switch (bp->b_datap->db_type) {

		case M_DATA:
ismdata:
			if (msgdsize(bp) == 0) {
				if (mark || delim) {
#ifdef STRPERF
					bp->b_sh += castimer() - stamp;
#endif
					freemsg(bp);
				} else if (rflg) {

					/*
					 * If already read data put zero
					 * length message back on queue else
					 * free msg and return 0.
					 */
					bp->b_band = pri;
#ifdef STRPERF
					bp->b_sh += castimer() - stamp;
#endif
					putbq_l(RD(stp->sd_wrq), bp);
				} else {
#ifdef STRPERF
					bp->b_sh += castimer() - stamp;
#endif
					freemsg(bp);
				}
				UNLOCK(stp->sd_mutex, pl);
				return(0);
			}

			/* drop lock for xfer to user space */
			UNLOCK(stp->sd_mutex, pl);
			rflg = 1;
			while (bp && uiop->uio_resid) {
				n = MIN(uiop->uio_resid, bp->b_wptr - bp->b_rptr);
				if (n) {
#ifdef STRPERF
					bp->b_stamp = castimer();
#endif
					error = uiomove((char *)bp->b_rptr, n, UIO_READ, uiop);
#ifdef STRPERF
					bp->b_copyout += castimer() - bp->b_stamp;
					bp->b_stamp = 0;
#endif
				}

				if (error) {
#ifdef STRPERF
					bp->b_sh += castimer() - stamp;
#endif
					freemsg(bp);
					return(error);
				}

				bp->b_rptr += n;
				while (bp && (bp->b_rptr >= bp->b_wptr)) {
					nbp = bp;
					bp = bp->b_cont;
#ifdef STRPERF
					nbp->b_sh += castimer() - stamp;
#endif
					freeb(nbp);
				}
			}

			/*
			 * The data may have been the leftover of a PCPROTO, so
			 * if none is left reset the STRPRI flag just in case.
			 */
			pl = LOCK(stp->sd_mutex, PLSTR);
			if (bp) {
				/*
				 * Have remaining data in message.
				 * Free msg if in discard mode.
				 */
				if (stp->sd_flag & RMSGDIS) {
#ifdef STRPERF
					bp->b_sh += castimer() - stamp;
#endif
					freemsg(bp);
					stp->sd_flag &= ~STRPRI;
				} else {
					bp->b_band = pri;
					if (mark && !stp->sd_mark) {
						stp->sd_mark = bp;
						bp->b_flag |= MSGMARK;
					}
					if (delim)
						bp->b_flag |= MSGDELIM;
#ifdef STRPERF
					bp->b_sh += castimer() - stamp;
#endif
					putbq_l(RD(stp->sd_wrq),bp);
				}
			} else {
				stp->sd_flag &= ~STRPRI;
			}

			/*
			 * Check for signal messages at the front of the read
			 * queue and generate the signal(s) if appropriate.
			 * The only signal that can be on queue is M_SIG at
			 * this point.
			 */
			while (((bp = RD(stp->sd_wrq)->q_first) != NULL) &&
				(bp->b_datap->db_type == M_SIG)) {
				bp = getq_l(RD(stp->sd_wrq));
#ifdef STRPERF
				stamp = castimer();
#endif
				strsignal(stp, *bp->b_rptr, (long)bp->b_band);
#ifdef STRPERF
				bp->b_sh += castimer() - stamp;
#endif
				freemsg(bp);
			}

			if ((uiop->uio_resid == 0) || mark || delim ||
			    (stp->sd_flag & (RMSGDIS|RMSGNODIS))) {
				UNLOCK(stp->sd_mutex, pl);
				return(error);
			}
			break;

		case M_PROTO:
		case M_PCPROTO:
			/*
			 * Only data messages are readable.
			 * Any others generate an error, unless
			 * RDPROTDIS or RDPROTDAT is set.
			 */
			if (stp->sd_flag & RDPROTDAT) {
				for (nbp = bp; nbp; nbp = nbp->b_cont)
					nbp->b_datap->db_type = M_DATA;
				stp->sd_flag &= ~STRPRI;
				goto ismdata;
			} else if (stp->sd_flag & RDPROTDIS) {
				while (bp &&
				    ((bp->b_datap->db_type == M_PROTO) ||
				    (bp->b_datap->db_type == M_PCPROTO))) {
					nbp = unlinkb(bp);
#ifdef STRPERF
					bp->b_sh += castimer() - stamp;
#endif
					freeb(bp);
					bp = nbp;
				}
				stp->sd_flag &= ~STRPRI;
				if (bp) {
					bp->b_band = pri;
					goto ismdata;
				} else {
					break;
				}
			}
			/* FALLTHROUGH */

		case M_PASSFP:
			if ((bp->b_datap->db_type == M_PASSFP) &&
			    (stp->sd_flag & RDPROTDIS)) {
#ifdef STRPERF
				bp->b_sh += castimer() - stamp;
#endif
				freemsg(bp);
				break;
			}
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			putbq_l(RD(stp->sd_wrq), bp);
			UNLOCK(stp->sd_mutex, pl);
			return(EBADMSG);

		case M_TRAIL:
			stp->sd_flag &= ~STRTOHUP;
			stp->sd_flag |= STRHUP;
			/* Don't miss any wakeups */
			if (SV_BLKD(stp->sd_read))
				broadcast |= RBLOCK;
			if (SV_BLKD(stp->sd_write))
				broadcast |= WBLOCK;
			if (SV_BLKD(stp->sd_timer3))
				broadcast |= IBLOCK;
			UNLOCK(stp->sd_mutex, pl);
			if (broadcast) {
				if (broadcast & RBLOCK)
					SV_BROADCAST(stp->sd_read, 0);
				if (broadcast & WBLOCK)
					SV_BROADCAST(stp->sd_write, 0);
				if (broadcast & IBLOCK)
					SV_BROADCAST(stp->sd_timer3, 0);
			}
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			freemsg(bp);
			pl = LOCK(stp->sd_mutex, PLSTR);
			break;

		default:
			/*
			 * Garbage on stream head read queue.
			 */
#ifndef lint
			ASSERT(0);
#endif
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			freemsg(bp);
			break;
		}
	}

	/* NOTREACHED */
}

/*
 * int
 * strrput(queue_t *q, mblk_t *bp)
 *	Stream read put procedure.  Called from downstream driver/module
 *	with messages for the stream head.  Data, protocol, and in-stream
 *	signal messages are placed on the queue, others are handled directly.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 */

STATIC int
strrput(queue_t *q, mblk_t *bp)
{
	struct stdata *stp;
	struct iocblk *iocbp;
	pl_t pl;
	struct stroptions *sop;
	struct copyreq *reqp;
	struct copyresp *resp;
	struct striopst *sp;
	int bpri;
	qband_t *qbp;
	int broadcast;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(q);
	stp = (struct stdata *)q->q_ptr;
	ASSERT(stp);
	ASSERT(! (stp->sd_flag & STPLEX));
	broadcast = 0;

	pl = LOCK(stp->sd_mutex, PLSTR);
	switch (bp->b_datap->db_type) {

	case M_DATA:
	case M_PROTO:
	case M_PCPROTO:
	case M_PASSFP:
		if (bp->b_datap->db_type == M_PCPROTO) {
			/*
			 * Only one priority protocol message is allowed at the
			 * stream head at a time.
			 */
			if (stp->sd_flag & STRPRI) {
				UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
				bp->b_sh += castimer() - stamp;
#endif
				freemsg(bp);
				return(0);
			}
		}

		/*
		 * Marking doesn't work well when messages
		 * are marked in more than one band.  We only
		 * remember the last message received, even if
		 * it is placed on the queue ahead of other
		 * marked messages.
		 */
		if (bp->b_flag & MSGMARK)
			stp->sd_mark = bp;

		/*
		 * This must actually be here for certain types of
		 * signalling to work correctly
		 */
		putq_l(q, bp);

		if (bp->b_datap->db_type == M_PCPROTO) {
			stp->sd_flag |= STRPRI;
			if (stp->sd_sigflags & S_HIPRI)
				strsendsig(stp->sd_siglist, S_HIPRI, 0L);
			if ((stp->sd_pollist->ph_events & POLLPRI) ||
			    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLPRI))) {
				pollwakeup(stp->sd_pollist, POLLPRI);
				stp->sd_flag &= ~STRPOLL;
				stp->sd_pevents = 0;
			}
		} else if (q->q_first == bp) {
			if (stp->sd_sigflags & S_INPUT)
				strsendsig(stp->sd_siglist, S_INPUT,
				    (long)bp->b_band);
			if ((stp->sd_pollist->ph_events & POLLIN) ||
			    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLIN))) {
				pollwakeup(stp->sd_pollist, POLLIN);
				stp->sd_flag &= ~STRPOLL;
				stp->sd_pevents = 0;
			}
			if (bp->b_band == 0) {
			    if (stp->sd_sigflags & S_RDNORM)
				    strsendsig(stp->sd_siglist, S_RDNORM, 0L);
			    if ((stp->sd_pollist->ph_events & POLLRDNORM) ||
			        ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLRDNORM))) {
				    pollwakeup(stp->sd_pollist, POLLRDNORM);
				    stp->sd_flag &= ~STRPOLL;
				    stp->sd_pevents = 0;
			    }
			} else {
			    if (stp->sd_sigflags & S_RDBAND)
				    strsendsig(stp->sd_siglist, S_RDBAND,
					(long)bp->b_band);
			    if ((stp->sd_pollist->ph_events & POLLRDBAND) ||
			        ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLRDBAND))) {
				    pollwakeup(stp->sd_pollist, POLLRDBAND);
				    stp->sd_flag &= ~STRPOLL;
				    stp->sd_pevents = 0;
			    }
			}
		}

		/*
		 * Wake sleeping read/getmsg
		 */
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		if (stp->sd_flag & RSLEEP) {
			stp->sd_flag &= ~RSLEEP;
			if (stp->sd_vnode->v_type == VFIFO) {
				UNLOCK(stp->sd_mutex, pl);
				SV_BROADCAST(stp->sd_read, KS_NOPRMPT); /*don't thrash */
			} else {
				UNLOCK(stp->sd_mutex, pl);
				SV_BROADCAST(stp->sd_read, 0);
			}
		} else {
			UNLOCK(stp->sd_mutex, pl);
		}

		return(0);


	case M_ERROR:
		/*
		 * An error has occured downstream, the errno is in the first
		 * byte of the message.
		 */
		if ((bp->b_wptr - bp->b_rptr) == 2) {	/* New flavor */
			unsigned char rw;

			rw = 0;
			if (*bp->b_rptr != NOERROR) {	/* read error */
				if (*bp->b_rptr != 0) {
					stp->sd_flag |= STRDERR;
					rw |= FLUSHR;
				} else {
					stp->sd_flag &= ~STRDERR;
				}
				stp->sd_rerror = *bp->b_rptr;
			}
			bp->b_rptr++;
			if (*bp->b_rptr != NOERROR) {	/* write error */
				if (*bp->b_rptr != 0) {
					stp->sd_flag |= STWRERR;
					rw |= FLUSHW;
				} else {
					stp->sd_flag &= ~STWRERR;
				}
				stp->sd_werror = *bp->b_rptr;
			}
			if (rw) {
				if (SV_BLKD(stp->sd_read))
					broadcast |= RBLOCK;
				if (SV_BLKD(stp->sd_write))
					broadcast |= WBLOCK;
				if (SV_BLKD(stp->sd_timer3))
					broadcast |= IBLOCK;
				if (stp->sd_sigflags & S_ERROR)
					strsendsig(stp->sd_siglist, S_ERROR,
					    ((rw & FLUSHR) ?
					    (long)stp->sd_rerror :
					    (long)stp->sd_werror));
				pollwakeup(stp->sd_pollist, POLLERR);
				stp->sd_pevents = 0;
				stp->sd_flag &= ~STRPOLL;
				UNLOCK(stp->sd_mutex, pl);
				if (broadcast) {
					if (broadcast & RBLOCK)
						SV_BROADCAST(stp->sd_read, 0);
					if (broadcast & WBLOCK)
						SV_BROADCAST(stp->sd_write, 0);
					if (broadcast & IBLOCK)
						SV_BROADCAST(stp->sd_timer3, 0);
				}
#ifdef STRPERF
				bp->b_sh += castimer() - stamp;
#endif
				freemsg(bp);
				(void) putctl1(WR(q)->q_next, M_FLUSH, rw);
				return(0);
			}
		} else if (*bp->b_rptr != 0) {		/* Old flavor */
			stp->sd_flag |= (STRDERR|STWRERR);
			stp->sd_rerror = *bp->b_rptr;
			stp->sd_werror = *bp->b_rptr;
			if (SV_BLKD(stp->sd_read))
				broadcast |= RBLOCK;
			if (SV_BLKD(stp->sd_write))
				broadcast |= WBLOCK;
			if (SV_BLKD(stp->sd_timer3))
				broadcast |= IBLOCK;

			if (stp->sd_sigflags & S_ERROR)
				strsendsig(stp->sd_siglist, S_ERROR,
				    (stp->sd_werror ? (long)stp->sd_werror :
				    (long)stp->sd_rerror));
			pollwakeup(stp->sd_pollist, POLLERR);
			stp->sd_pevents = 0;
			stp->sd_flag &= ~STRPOLL;
			UNLOCK(stp->sd_mutex, pl);
			if (broadcast) {
				if (broadcast & RBLOCK)
					SV_BROADCAST(stp->sd_read, 0);
				if (broadcast & WBLOCK)
					SV_BROADCAST(stp->sd_write, 0);
				if (broadcast & IBLOCK)
					SV_BROADCAST(stp->sd_timer3, 0);
			}
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			freemsg(bp);
			(void) putctl1(WR(q)->q_next, M_FLUSH, FLUSHRW);
			return(0);
		}
		UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
		return(0);

	case M_HANGUP:

hlost:
		stp->sd_werror = ENXIO;
		if ((bp->b_wptr - bp->b_rptr) == 1)
			/* new semantic indicated by phantom byte */
			stp->sd_flag |= STRTOHUP;
		else
			stp->sd_flag |= STRHUP;
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);

		/*
		 * send signal if controlling tty
		 */

		if (stp->sd_sessp)
			sigtoproc((proc_t *)stp->sd_sessp->s_sidp->pid_procp,
			    SIGHUP, (sigqueue_t *)NULL);

		if (SV_BLKD(stp->sd_read))
			broadcast |= RBLOCK;
		if (SV_BLKD(stp->sd_write))
			broadcast |= WBLOCK;
		if (SV_BLKD(stp->sd_timer3))
			broadcast |= IBLOCK;

		/*
		 * wake up read, write, and exception pollers and
		 * reset wakeup mechanism.
		 */

		strhup(stp);
		UNLOCK(stp->sd_mutex, pl);
		if (broadcast) {
			if (broadcast & RBLOCK)
				SV_BROADCAST(stp->sd_read, 0);
			if (broadcast & WBLOCK)
				SV_BROADCAST(stp->sd_write, 0);
			if (broadcast & IBLOCK)
				SV_BROADCAST(stp->sd_timer3, 0);
		}
		return(0);

	case M_TRAIL:
		if (stp->sd_flag & STRTOHUP) {
			if (q->q_first) {
				/* messages queued- handle later */
				putq_l(q, bp);
				UNLOCK(stp->sd_mutex, pl);
				return(0);
			}
			stp->sd_flag &= ~STRTOHUP;
			stp->sd_flag |= STRHUP;
			/* Don't miss any wakeups */
			if (SV_BLKD(stp->sd_read))
				broadcast |= RBLOCK;
			if (SV_BLKD(stp->sd_write))
				broadcast |= WBLOCK;
			if (SV_BLKD(stp->sd_timer3))
				broadcast |= IBLOCK;
			UNLOCK(stp->sd_mutex, pl);
			if (broadcast) {
				if (broadcast & RBLOCK)
					SV_BROADCAST(stp->sd_read, 0);
				if (broadcast & WBLOCK)
					SV_BROADCAST(stp->sd_write, 0);
				if (broadcast & IBLOCK)
					SV_BROADCAST(stp->sd_timer3, 0);
			}
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			freemsg(bp);
			return(0);
		}

		/*
		 * If we get here, the original M_HANGUP was lost.  Go to
		 * the M_HANGUP case above and process normally.
		 */
		bp->b_wptr = bp->b_rptr;
		goto hlost;

	case M_SIG:
		/*
		 * Someone downstream wants to post a signal.  The
		 * signal to post is contained in the first byte of the
		 * message.  If the message would go on the front of
		 * the queue, send a signal to the process group
		 * (if not SIGPOLL) or to the siglist processes
		 * (SIGPOLL).  If something is already on the queue,
		 * just enqueue the message.
		 */
		if (q->q_first) {
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			putq_l(q, bp);
			UNLOCK(stp->sd_mutex, pl);
			return(0);
		}
		/* FALLTHROUGH */

	case M_PCSIG:
		/*
		 * Don't enqueue, just post the signal.
		 */
		strsignal(stp, *bp->b_rptr, 0L);
		UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
		return(0);

	case M_FLUSH:
		/*
		 * Flush queues.  The indication of which queues to flush
		 * is in the first byte of the message.  If the read queue
		 * is specified, then flush it.  If FLUSHBAND is set, just
		 * flush the band specified by the second byte of the message.
		 */
	    {
		mblk_t *mp;
		mblk_t *nmp;
		mblk_t *last;
		int backenable;
		int band;
		int i;
		queue_t *nq;
		unsigned char pri;


		backenable = 0;
		if (*bp->b_rptr & FLUSHR) {
		    if (*bp->b_rptr & FLUSHBAND) {
			ASSERT((bp->b_wptr - bp->b_rptr) >= 2);
			pri = *(bp->b_rptr + 1);
			if (pri > q->q_nband)
			    goto wrapflush;
			if (pri == 0) {
			    mp = q->q_first;
			    q->q_first = NULL;
			    q->q_last = NULL;
			    q->q_count = 0;
			    for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
				qbp->qb_first = NULL;
				qbp->qb_last = NULL;
				qbp->qb_count = 0;
				qbp->qb_flag &= ~QB_FULL;
			    }
			    q->q_blocked = 0;
			    q->q_flag &= ~QFULL;
			    while (mp) {
				nmp = mp->b_next;
				if (mp->b_band == 0) {
#ifdef STRPERF
				    mp->b_sched += castimer() - mp->b_stamp;
				    mp->b_stamp = 0;
				    mp->b_sh += castimer() - stamp;
#endif
				    freemsg(mp);
				} else {
					putq_l(q, mp);
				}
				mp = nmp;
			    }
			    if ((q->q_flag & QWANTW) &&
			      (q->q_count <= q->q_lowat)) {
				/* find nearest back queue with service proc */
				q->q_flag &= ~QWANTW;
				for (nq = backq_l(q); nq && !(nq->q_qinfo->qi_srvp && (nq->q_flag & QPROCSON));
				  nq = backq_l(nq))
				    ;
				if (nq) {
				    qenable_l(nq);
				    setqback(nq, 0);
				}
			    }
			} else {	/* pri != 0 */
			    band = pri;
			    qbp = q->q_bandp;
			    while (--band > 0)
				qbp = qbp->qb_next;
			    mp = qbp->qb_first;
			    if (mp == NULL)
				goto wrapflush;
			    last = qbp->qb_last;
			    if (mp == last)	/* only message in band */
				last = mp->b_next;
			    while (mp != last) {
				nmp = mp->b_next;
				if (mp->b_band == pri) {
				    rmvq(q, mp);
#ifdef STRPERF
				    mp->b_sched += castimer() - mp->b_stamp;
				    mp->b_stamp = 0;
				    mp->b_sh += castimer() - stamp;
#endif
				    freemsg(mp);
				}
				mp = nmp;
			    }
			    if (mp && mp->b_band == pri) {
				rmvq(q, mp);
#ifdef STRPERF
				mp->b_sched += castimer() - mp->b_stamp;
				mp->b_stamp = 0;
				mp->b_sh += castimer() - stamp;
#endif
				freemsg(mp);
			    }
			}
		    } else {	/* flush entire queue */
			mp = q->q_first;
			q->q_first = NULL;
			q->q_last = NULL;
			q->q_count = 0;
			stp->sd_flag &= ~STRPRI;
			for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
			    qbp->qb_first = NULL;
			    qbp->qb_last = NULL;
			    qbp->qb_count = 0;
			    qbp->qb_flag &= ~QB_FULL;
			}
			q->q_blocked = 0;
			q->q_flag &= ~QFULL;
			while (mp) {
			    nmp = mp->b_next;
#ifdef STRPERF
			    mp->b_sched += castimer() - mp->b_stamp;
			    mp->b_stamp = 0;
			    mp->b_sh = castimer() - stamp;
#endif
			    freemsg(mp);
			    mp = nmp;
			}
			bzero((caddr_t)l.qbf, NBAND);
			bpri = 1;
			for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next) {
			    if ((qbp->qb_flag & QB_WANTW) &&
			      (qbp->qb_count <= qbp->qb_lowat)) {
				qbp->qb_flag &= ~QB_WANTW;
				backenable = 1;
				l.qbf[bpri] = 1;
			    }
			    bpri++;
			}
			if ((q->q_flag & QWANTW) && (q->q_count <= q->q_lowat)) {
			    q->q_flag &= ~QWANTW;
			    backenable = 1;
			    l.qbf[0] = 1;
			}

			/*
			 * If any band can now be written to, and there is a
			 * writer for that band, then backenable the closest
			 * service procedure.
			 */
			if (backenable) {
			    /* find nearest back queue with service proc */
			    for (nq = backq_l(q); nq && !(nq->q_qinfo->qi_srvp && (nq->q_flag & QPROCSON));
			      nq = backq_l(nq))
				;
			    if (nq) {
				qenable_l(nq);
				bpri = (int)nq->q_nband;
				for (i = 0; i <= bpri; i++) {
					if (l.qbf[i])
						setqback(nq, i);
				}
			    }
			}
		    }
		}
wrapflush:
		UNLOCK(stp->sd_mutex, pl);
		if ((*bp->b_rptr & FLUSHW) && !(bp->b_flag & MSGNOLOOP)) {
			*bp->b_rptr &= ~FLUSHR;
			bp->b_flag |= MSGNOLOOP;
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			qreply(q, bp);
			return(0);
		}
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
		return(0);
	    }

	case M_IOCACK:
	case M_IOCNAK:
		/* LINTED pointer alignment */
		iocbp = (struct iocblk *)bp->b_rptr;
		/*
		 * If not waiting for ACK or NAK then just free msg.
		 * If already have ACK or NAK for user then just free msg.
		 * If incorrect id sequence number then just free msg.
		 */
		if ((stp->sd_flag & IOCWAIT) == 0 || stp->sd_iocblk ||
		    (stp->sd_iocid != iocbp->ioc_id)) {
			UNLOCK(stp->sd_mutex, pl);
			/* toss associated post-processing request */
			sp = findioc(iocbp->ioc_id);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			freemsg(bp);
			return(0);
		}

		/*
		 * Assign ACK or NAK to user and wake up.
		 */
#ifdef STRPERF
		bp->b_stamp = castimer();
		bp->b_sh += bp->b_stamp - stamp;
#endif
		stp->sd_iocblk = bp;
		if (SV_BLKD(stp->sd_timer3)) {
			UNLOCK(stp->sd_mutex, pl);
			SV_BROADCAST(stp->sd_timer3, 0); /* ioctller */
		} else {
			UNLOCK(stp->sd_mutex, pl);
		}
		return(0);

	case M_COPYIN:
	case M_COPYOUT:
		/* LINTED pointer alignment */
		reqp = (struct copyreq *)bp->b_rptr;

		/*
		 * If not waiting for ACK or NAK then just fail request.
		 * If already have ACK, NAK, or copy request, then just
		 * fail request.
		 * If incorrect id sequence number then just fail request.
		 */
		if ((stp->sd_flag & IOCWAIT) == 0 || stp->sd_iocblk ||
		    (stp->sd_iocid != reqp->cq_id)) {
			UNLOCK(stp->sd_mutex, pl);
			if (bp->b_cont) {
#ifdef STRPERF
				bp->b_cont->b_sh += castimer() - stamp;
#endif
				freemsg(bp->b_cont);
				bp->b_cont = NULL;
			}
			bp->b_datap->db_type = M_IOCDATA;
			/* LINTED pointer alignment */
			resp = (struct copyresp *)bp->b_rptr;
			resp->cp_rval = (caddr_t)1;	/* failure */
			/* toss associated post-processing request */
			sp = findioc(reqp->cq_id);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			qreply(q, bp);
			return(0);
		}

		/*
		 * Assign copy request to user and wake up.
		 */
#ifdef STRPERF
		bp->b_stamp = castimer();
		bp->b_sh += bp->b_stamp - stamp;
#endif
		stp->sd_iocblk = bp;
		if (SV_BLKD(stp->sd_timer3)) {
			UNLOCK(stp->sd_mutex, pl);
			SV_BROADCAST(stp->sd_timer3, 0); /* ioctller */
		} else {
			UNLOCK(stp->sd_mutex, pl);
		}
		return(0);

	case M_SETOPTS:
	case M_PCSETOPTS:
		/*
		 * Set stream head options (read option, write offset,
		 * min/max packet size, and/or high/low water marks for
		 * the read side only).
		 */

		bpri = 0;
		ASSERT((bp->b_wptr - bp->b_rptr) == sizeof(struct stroptions));
		/* LINTED pointer alignment */
		sop = (struct stroptions *)bp->b_rptr;
		if (sop->so_flags & SO_READOPT) {
			switch (sop->so_readopt & RMODEMASK) {
			case RNORM:
				stp->sd_flag &= ~(RMSGDIS | RMSGNODIS);
				break;

			case RMSGD:
				stp->sd_flag = ((stp->sd_flag & ~RMSGNODIS) |
				    RMSGDIS);
				break;

			case RMSGN:
				stp->sd_flag = ((stp->sd_flag & ~RMSGDIS) |
				    RMSGNODIS);
				break;
			}
			switch(sop->so_readopt & RPROTMASK) {
			case RPROTNORM:
				stp->sd_flag &= ~(RDPROTDAT | RDPROTDIS);
				break;

			case RPROTDAT:
				stp->sd_flag = ((stp->sd_flag & ~RDPROTDIS) |
				    RDPROTDAT);
				break;

			case RPROTDIS:
				stp->sd_flag = ((stp->sd_flag & ~RDPROTDAT) |
				    RDPROTDIS);
				break;
			}
		}

		if (sop->so_flags & SO_WROFF)
			stp->sd_wroff = sop->so_wroff;
		if (sop->so_flags & SO_MINPSZ)
			q->q_minpsz = sop->so_minpsz;
		if (sop->so_flags & SO_MAXPSZ)
			q->q_maxpsz = sop->so_maxpsz;
		if (sop->so_flags & SO_HIWAT) {
			if (sop->so_flags & SO_BAND) {
				if (strqset(q, QHIWAT, sop->so_band, sop->so_hiwat))
					/*
					 *+ Kernel could not allocate memory.
					 */
					cmn_err(CE_WARN, "strrput: could not allocate qband\n");
				else
					bpri = sop->so_band;
		 	} else {
				q->q_hiwat = sop->so_hiwat;
			}
		}
		if (sop->so_flags & SO_LOWAT) {
			if (sop->so_flags & SO_BAND) {
				if (strqset(q, QLOWAT, sop->so_band, sop->so_lowat))
					/*
					 *+ Kernel could not allocate memory.
					 */
					cmn_err(CE_WARN, "strrput: could not allocate qband\n");
				else
					bpri = sop->so_band;
		 	} else {
				q->q_lowat = sop->so_lowat;
			}
		}
		if (sop->so_flags & SO_MREADON)
			stp->sd_flag |= SNDMREAD;
		if (sop->so_flags & SO_MREADOFF)
			stp->sd_flag &= ~SNDMREAD;
		if (sop->so_flags & SO_NDELON)
			stp->sd_flag |= OLDNDELAY;
		if (sop->so_flags & SO_NDELOFF)
			stp->sd_flag &= ~OLDNDELAY;
		if (sop->so_flags & SO_ISTTY)
			stp->sd_flag |= STRISTTY;
		if (sop->so_flags & SO_ISNTTY)
			stp->sd_flag &= ~STRISTTY;
		if (sop->so_flags & SO_TOSTOP)
			stp->sd_flag |= STRTOSTOP;
		if (sop->so_flags & SO_TONSTOP)
			stp->sd_flag &= ~STRTOSTOP;
		if (sop->so_flags & SO_DELIM)
			stp->sd_flag |= STRDELIM;
		if (sop->so_flags & SO_NODELIM)
			stp->sd_flag &= ~STRDELIM;
		if (sop->so_flags & SO_LOOP)
			stp->sd_flag |= STRLOOP;
		/* STRHOLD no longer used */

#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);

		if (bpri == 0) {
			if ((q->q_count <= q->q_lowat) &&
			    (q->q_flag & QWANTW)) {
				q->q_flag &= ~QWANTW;
				for (q = backq_l(q); q && !(q->q_qinfo->qi_srvp && (q->q_flag & QPROCSON));
				    q = backq_l(q))
					;
				if (q) {
					qenable_l(q);
					setqback(q, bpri);
				}
			}
		} else {
			char i;

			qbp = q->q_bandp;
			for (i = 1; i < bpri; i++)
				qbp = qbp->qb_next;
			if ((qbp->qb_count <= qbp->qb_lowat) &&
			    (qbp->qb_flag & QB_WANTW)) {
				qbp->qb_flag &= ~QB_WANTW;
				for (q = backq_l(q); q && !(q->q_qinfo->qi_srvp && (q->q_flag & QPROCSON));
				    q = backq_l(q))
					;
				if (q) {
					qenable_l(q);
					setqback(q, bpri);
				}
			}
		}
		UNLOCK(stp->sd_mutex, pl);

		return(0);

	/*
	 * The following set of cases deal with situations where two stream
	 * heads are connected to each other (twisted streams).  These messages
	 * have no meaning at the stream head.
	 */
	case M_BREAK:
	case M_CTL:
	case M_PCCTL:
	case M_DELAY:
	case M_START:
	case M_STOP:
	case M_IOCDATA:
	case M_STARTI:
	case M_STOPI:
		UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
		return(0);

	case M_IOCTL:
		/*
		 * Always NAK this condition
		 * (makes no sense)
		 */
		UNLOCK(stp->sd_mutex, pl);
		bp->b_datap->db_type = M_IOCNAK;
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		qreply(q, bp);
		return(0);

	default:
		UNLOCK(stp->sd_mutex, pl);
#ifndef lint
		ASSERT(0);
#endif
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
		return(0);
	}
}

/*
 * int
 * strwrite(struct vnode *vp, struct uio *uiop, cred_t *crp)
 *	Write data to a stream.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 *
 * Description:
 *	Write attempts to break the request into messages conforming
 *	with the minimum and maximum packet sizes set downstream.
 *
 *	Write will not block if downstream queue is full and O_NDELAY is set,
 *	otherwise it will block waiting for the queue to get room.
 *
 *	A write of zero bytes gets packaged into a zero length message and sent
 *	downstream like any other message.
 *
 *	If buffers of the requested sizes are not available, the write will
 *	sleep until the buffers become available.
 *
 *	Write (if specified) will supply a write offset in a message if it
 *	makes sense. This can be specified by downstream modules as part of
 *	a M_SETOPTS message.  Write will not supply the write offset if it
 *	cannot supply any data in a buffer.  In other words, write will never
 *	send down an empty packet due to a write offset.
 */

/*ARGSUSED*/
int
strwrite(struct vnode *vp, struct uio *uiop, cred_t *crp)
{
	struct stdata *stp;
	struct queue *wqp;
	mblk_t *mp;
	pl_t pl;
	long rmin;
	long rmax;
	long iosize;
	char waitflag;
	int tempmode;
	int done;
	int error;
	int unbind;
	int wroff;
	struct engine *engp;
#ifdef STRPERF
	int stamp;
#endif

	done = 0;
	error = 0;
	unbind = 0;
	/* keep lint happy */
	engp = NULL;
	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	pl = LOCK(stp->sd_mutex, PLSTR);
loop:
	if (stp->sd_flag & STPLEX) {
		UNLOCK(stp->sd_mutex, pl);
		return(EINVAL);
	}
	if (stp->sd_flag & (STWRERR|STRHUP)) {
		if (stp->sd_flag & STRSIGPIPE)
			sigtolwp(u.u_lwpp, SIGPIPE, (sigqueue_t *) NULL);

		/*
		 * this is for POSIX compatibility
		 */
		error = (stp->sd_flag & STRHUP) ? EIO : stp->sd_werror;
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}
	switch (error = straccess(stp, JCWRITE)) {
	case CTTY_STOPPED:		/* LWP was stopped */
		goto loop;
	case CTTY_EOF:			/* end-of-file */
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	case CTTY_OK:			/* write */
		break;
	default:			/* error */
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}

	/*
	 * firewall - don't use too much memory. 
	 */
	if (strthresh && (Strcount > strthresh) && 
			pm_denied(CRED(), P_SYSOPS)) {
		UNLOCK(stp->sd_mutex, pl);
		return(ENOSR);
	}

	/*
	 * Check the min/max packet size constraints.  If min packet size
	 * is non-zero, the write cannot be split into multiple messages
	 * and still guarantee the size constraints.
	 */
	wqp = stp->sd_wrq;
	rmin = wqp->q_next->q_minpsz;
	rmax = wqp->q_next->q_maxpsz;
	ASSERT((rmax >= 0) || (rmax == INFPSZ));
	if (rmax == 0) {
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	}
	if (strmsgsz != 0) {
		if (rmax == INFPSZ)
			rmax = strmsgsz;
		else
			rmax = MIN(strmsgsz, rmax);
	}
	if (rmin > 0) {
		if (uiop->uio_resid < rmin) {
			UNLOCK(stp->sd_mutex, pl);
			return(ERANGE);
		}
	    	if ((rmax != INFPSZ) && (uiop->uio_resid > rmax)) {
			UNLOCK(stp->sd_mutex, pl);
			return(ERANGE);
		}
	}

	/*
	 * Do until count satisfied or error.
	 */
	waitflag = WRITEWAIT;
	if (stp->sd_flag & OLDNDELAY)
		tempmode = uiop->uio_fmode & ~FNDELAY;
	else
		tempmode = uiop->uio_fmode;

	do {
		mblk_t *amp;	/* auto */

		while ((stp->sd_wrq->q_flag & QFREEZE) || !bcanput_l(wqp->q_next, 0)) {

			/*
			 * if FIFO/pipe, don't sleep here. Sleep in the
			 * fifo write routine.
			 */
			if (vp->v_type == VFIFO) {
				UNLOCK(stp->sd_mutex, pl);
				STRKUNBIND(engp, unbind);
				return(ESTRPIPE);
			}
			/*
			 * Note: strwaitq will atomically go to sleep on
			 * sd_write, dropping the lock at that point.  Since
			 * the lock is never dropped between the check for
			 * flow control and going to sleep, we can't miss
			 * the flow control clearing.
			 */
			error = strwaitq(stp, waitflag, (off_t)0, tempmode, &done);
			if (error || done) {
				UNLOCK(stp->sd_mutex, pl);
				STRKUNBIND(engp, unbind);
				return(error);
			}
			/*
			 * strwaitq dropped the lock, recheck for errors.
			 */
			if (stp->sd_flag & STPLEX) {
				UNLOCK(stp->sd_mutex, pl);
				STRKUNBIND(engp, unbind);
				return(EINVAL);
			}
			if (stp->sd_flag & (STWRERR|STRHUP)) {
				if (stp->sd_flag & STRSIGPIPE)
					sigtolwp(u.u_lwpp,
						 SIGPIPE, (sigqueue_t *)NULL);

				/*
				 * this is for POSIX compatibility
				 */
				error = (stp->sd_flag & STRHUP) ? EIO : stp->sd_werror;
				UNLOCK(stp->sd_mutex, pl);
				STRKUNBIND(engp, unbind);
				return(error);
			}
		}
		/* take a snapshot */
		wroff = (int) stp->sd_wroff;
		UNLOCK(stp->sd_mutex, pl);

		/*
		 * Determine the size of the next message to be
		 * packaged.  May have to break write into several
		 * messages based on max packet size.
		 */
		if (rmax == INFPSZ)
			iosize = uiop->uio_resid;
		else
			iosize = MIN(uiop->uio_resid, rmax);

		error = strmakemsg((struct strbuf *)NULL, iosize, uiop, stp, (long)0, &amp, wroff);
		if (error || !amp) {
			STRKUNBIND(engp, unbind);
			return(error);
		}
#ifdef STRPERF
		stamp = castimer();
#endif
		mp = amp;

		/*
		 * Put block downstream.
		 */
		pl = LOCK(stp->sd_mutex, PLSTR);
		if ((uiop->uio_resid == 0) && (stp->sd_flag & STRDELIM))
			mp->b_flag |= MSGDELIM;
		stp->sd_upbcnt++;
		/*
		 * Unfortunately, since writes may occur in a loop and the
		 * stream is free to change in the interim, we may have to
		 * rebind on each iteration.
		 *
		 * Some important notes: we may bind here even if we don't
		 * strictly have to (i.e. the uniplexor is around to switch
		 * things).  It doesn't hurt to start in the right place.
		 * Also, if we aren't bound, but a UP module gets pushed
		 * before the putnext, putnext can deal with it.
		 */
		if (stp->sd_cpu && (stp->sd_cpu != l.eng)) {
			UNLOCK(stp->sd_mutex, pl);
			STRKUNBIND(engp, unbind);
			STRKBIND(stp, engp, unbind);
			pl = LOCK(stp->sd_mutex, PLSTR);
		}
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		if ((vp->v_type == VFIFO) && (wqp->q_next->q_qinfo == &strdata))
			/*
			 * If this is a pipe and the next thing in line is
			 * a stream head, then there are no modules pushed and
			 * this is a live pipe so bypass the normal putnext
			 * processing and just do what needs to be done.
			 */
			strpipeput(wqp->q_next, mp);
		else
			putnext_l(wqp, mp);
		waitflag |= NOINTR;
		/* need lock for next iteration */
		if ((--stp->sd_upbcnt == 0) && (stp->sd_flag & UPBLOCK))
			SV_BROADCAST(stp->sd_upblock, 0);

	} while (uiop->uio_resid);

	UNLOCK(stp->sd_mutex, pl);

	STRKUNBIND(engp, unbind)

	return(0);
}

/*
 * int
 * strwsrv(queue_t *q)
 *	Stream head write service routine.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 *
 * Description:
 *	This routine wakes up any sleeping writers when a queue
 *	downstream needs data (part of the flow control in putq and getq).
 *	It also must wake anyone sleeping on a poll().
 */

STATIC int
strwsrv(queue_t *q)
{
	struct stdata *stp;
	pl_t pl;
	queue_t *tq;
	qband_t *qbp;
	int i;
	qband_t *myqbp;
	int isevent;
	int broadcast;

	broadcast = 0;
	stp = (struct stdata *)q->q_ptr;
	ASSERT(!(stp->sd_flag & STPLEX));
	pl = LOCK(stp->sd_mutex, PLSTR);

	/*
	 * if freezing, just mark at QTOENAB and return
	 */

	if (q->q_flag & QFREEZE) {
		q->q_flag |= QTOENAB;
		q->q_flag &= ~QBACK;
		for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next)
			qbp->qb_flag &= ~QB_BACK;
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	}
	if (stp->sd_flag & WSLEEP) {
		stp->sd_flag &= ~WSLEEP;
		if (SV_BLKD(stp->sd_write))
			broadcast |= WBLOCK;
		if (SV_BLKD(stp->sd_timer))
			broadcast |= TBLOCK;
	}

	if ((tq = q->q_next) == NULL) {

		/*
		 * The other end of a stream pipe went away.
		 */
		q->q_flag &= ~QBACK;
		for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next)
			qbp->qb_flag &= ~QB_BACK;
		UNLOCK(stp->sd_mutex, pl);
		if (broadcast & WBLOCK) {
			/* sd_vnode is invariant */
			if (stp->sd_vnode->v_type == VFIFO)
				SV_BROADCAST(stp->sd_write, KS_NOPRMPT);
			else
				SV_BROADCAST(stp->sd_write, 0);
		}
		if (broadcast & TBLOCK) {
			/* sd_vnode is invariant */
			if (stp->sd_vnode->v_type == VFIFO)
				SV_BROADCAST(stp->sd_timer, KS_NOPRMPT);
			else
				SV_BROADCAST(stp->sd_timer, 0);
		}
		return(0);
	}
	while (tq->q_next && !(tq->q_qinfo->qi_srvp && (tq->q_flag & QPROCSON)))
		tq = tq->q_next;

	if (q->q_flag & QBACK) {
		if (tq->q_flag & QFULL) {
			tq->q_flag |= QWANTW;
		} else {
			if (stp->sd_sigflags & S_WRNORM)
				strsendsig(stp->sd_siglist, S_WRNORM, 0L);
			if ((stp->sd_pollist->ph_events & POLLWRNORM) ||
			    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLWRNORM))) {
				pollwakeup(stp->sd_pollist, POLLWRNORM);
				stp->sd_flag &= ~STRPOLL;
				stp->sd_pevents = 0;
			}
		}
	}

	isevent = 0;
	i = 1;
	bzero((caddr_t)l.qbf, NBAND);
	myqbp = q->q_bandp;
	for (qbp = tq->q_bandp; qbp; qbp = qbp->qb_next) {
		if (!myqbp)
			break;
		if (myqbp->qb_flag & QB_BACK) {
			if (qbp->qb_flag & QB_FULL) {
				qbp->qb_flag |= QB_WANTW;
			} else {
				isevent = 1;
				l.qbf[i] = 1;
			}
		}
		myqbp = myqbp->qb_next;
		i++;
	}
	while (myqbp) {
		if (myqbp->qb_flag & QB_BACK) {
			isevent = 1;
			l.qbf[i] = 1;
		}
		myqbp = myqbp->qb_next;
		i++;
	}

	if (isevent) {
	    for (i--; i; i--) {
		if (l.qbf[i]) {
			if (stp->sd_sigflags & S_WRBAND)
				strsendsig(stp->sd_siglist, S_WRBAND, (long)i);
			if ((stp->sd_pollist->ph_events & POLLWRBAND) ||
			    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLWRBAND))) {
				pollwakeup(stp->sd_pollist, POLLWRBAND);
				stp->sd_flag &= ~STRPOLL;
				stp->sd_pevents = 0;
			}
		}
	    }
	}

	q->q_flag &= ~QBACK;
	for (qbp = q->q_bandp; qbp; qbp = qbp->qb_next)
		qbp->qb_flag &= ~QB_BACK;
	UNLOCK(stp->sd_mutex, pl);
	if (broadcast & WBLOCK) {
		/* sd_vnode is invariant */
		if (stp->sd_vnode->v_type == VFIFO)
			SV_BROADCAST(stp->sd_write, KS_NOPRMPT);
		else
			SV_BROADCAST(stp->sd_write, 0);
	}
	if (broadcast & TBLOCK) {
		/* sd_vnode is invariant */
		if (stp->sd_vnode->v_type == VFIFO)
			SV_BROADCAST(stp->sd_timer, KS_NOPRMPT);
		else
			SV_BROADCAST(stp->sd_timer, 0);
	}
	return(0);
}

/*
 * int
 * strioctl(struct vnode *vp, int cmd, int arg, int flag, int copyflag,
 *	    cred_t *crp, int *rvalp)
 *	ioctl for streams
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 *
 * Description:
 *	The plumbing operations block on the sd_plumb with a non-interruptible
 *	priority.  The assumption is that the previous plumbing operation will,
 *	in fact, complete.  Since at the very bottom all driver and module
 *	open/close operations must be written in an interruptible way, this is
 *	a safe assumption (i.e. in the event of a hang, the process which is
 *	actually hung within the driver open/close should be interrupted; this
 *	will in turn unlock all the processes blocked on sd_plumb behind it.
 */

int
strioctl(struct vnode *vp, int cmd, int arg, int flag, int copyflag,
	cred_t *crp, int *rvalp)
{
	struct stdata *stp;
	queue_t *q;
	pl_t pl;
	struct strioctl strioc;
	enum jcaccess access;
	mblk_t *mp;
	struct engine *engp;
	int unbind;
	int error;
	int done;
	int haveplumb;
	int broadcast;
	char *fmt;
#ifdef STRPERF
	extern fspin_t stat_mutex;
	extern struct strperf strperf;
	int stamp;
#endif
	void *iocstatep;
	mblk_t *bp1;

	iocstatep = NULL;
	bp1 = NULL;
	error = 0;
	broadcast = 0;
	done = 0;
	unbind = 0;
	haveplumb = 0;
	fmt = NULL;

	ASSERT(vp->v_stream);
	ASSERT(copyflag == U_TO_K || copyflag == K_TO_K);
	stp = vp->v_stream;

	/*
	 * Handle any platform-specific ioctl conversion or remapping
	 * for SCO compatibility.
	 */
	STRIOCTL_P(vp, &cmd, &arg, flag, copyflag, crp, rvalp,
		(struct strioctlstate **) &iocstatep, &bp1, &error);
	if (error)
		return(error);

	switch ((unsigned int) cmd) {
	case I_RECVFD:
	case I_E_RECVFD:
	case I_S_RECVFD:
		access = JCREAD;
		break;

	case I_FDINSERT:
	case I_SENDFD:
	case TIOCSTI:
		access = JCWRITE;
		break;

	case TCGETA:
	case TCGETS:
	case TIOCGETP:
	case TIOCGPGRP:
	case TIOCGSID:
	case TIOCMGET:
	case LDGETT:
	case TIOCGETC:
	case TIOCLGET:
	case TIOCGLTC:
	case TIOCGETD:
	case TIOCGWINSZ:
	case LDGMAP:
	case I_CANPUT:
	case I_NREAD:
	case FIONREAD:
	case FIORDCHK:
	case I_FIND:
	case I_LOOK:
	case I_GRDOPT:
	case I_GETSIG:
	case I_PEEK:
	case I_GWROPT:
	case I_LIST:
	case I_CKBAND:
	case I_GETBAND:
	case I_GETCLTIME:
		access = JCGETP;
		break;

	default:
		access = STRACCMAP(cmd);
		break;
	}


	pl = LOCK(stp->sd_mutex, PLSTR);
loop:
	if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
		error = (stp->sd_flag & STPLEX) ? EINVAL :
		    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}
	switch (error = straccess(stp, access)) {
	case CTTY_STOPPED:
		goto loop;
	case CTTY_EOF:
		UNLOCK(stp->sd_mutex, pl);
		return(EIO);
	case CTTY_OK:
		break;
	default:
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}

	/* sd_mutex held at this point */
	switch ((unsigned int) cmd) {

	default:
		if (((cmd & IOCTYPE) == LDIOC) ||
		    ((cmd & IOCTYPE) == tIOC) ||
		    ((cmd & IOCTYPE) == TIOC)) {

			/*
			 * The ioctl is a tty ioctl - set up strioc buffer
			 * and call strdoioctl() to do the work.
			 */
			if (stp->sd_flag & STRHUP) {
				UNLOCK(stp->sd_mutex, pl);
				return(ENXIO);
			}

			/*
			 * NOTE - getplumb returns with sd_mutex unlocked
			 */
			if (error = getplumb(stp))
				return(error);

			strioc.ic_cmd = cmd;
			strioc.ic_timout = INFTIM;

			switch (cmd) {
			case TCXONC:
			case TCSBRK:
			case TCFLSH:
			case TCDSET:
				strioc.ic_len = sizeof(int);
				strioc.ic_dp = (char *) &arg;
				error = strdoioctl(stp, &strioc, NULL, K_TO_K,
						STRINT, crp, rvalp);
				relplumb(stp);
				return(error);

			case TCSETA:
			case TCSETAW:
			case TCSETAF:
				strioc.ic_len = sizeof(struct termio);
				strioc.ic_dp = (char *)arg;
				error = strdoioctl(stp, &strioc, NULL, copyflag,
						STRTERMIO, crp, rvalp);
				relplumb(stp);
				return(error);

			case TCSETS:
			case TCSETSW:
			case TCSETSF:
				strioc.ic_len = sizeof(struct termios);
				strioc.ic_dp = (char *)arg;
				error = strdoioctl(stp, &strioc, NULL, copyflag,
						STRTERMIOS, crp, rvalp);
				relplumb(stp);
				return(error);

			case LDSETT:
				strioc.ic_len = sizeof(struct termcb);
				strioc.ic_dp = (char *)arg;
				error = strdoioctl(stp, &strioc, NULL,
				    copyflag, STRTERMCB, crp, rvalp);
				relplumb(stp);
				return(error);

			case TIOCSETP:
				strioc.ic_len = sizeof(struct sgttyb);
				strioc.ic_dp = (char *)arg;
				error = strdoioctl(stp, &strioc, NULL,
				    copyflag, STRSGTTYB, crp, rvalp);
				relplumb(stp);
				return(error);

			case TIOCSTI:
				if (pm_denied(crp, P_DACREAD)) {
					if ((flag & FREAD) == 0) {
						relplumb(stp);
						return(EPERM);
					}
					pl = LOCK(stp->sd_mutex, PLSTR);
					(void) LOCK(&u.u_procp->p_sess_mutex, PL_SESS);
					if (stp->sd_sessp != u.u_procp->p_sessp) {
						UNLOCK(&u.u_procp->p_sess_mutex, PLSTR);
						UNLOCK(stp->sd_mutex, pl);
						relplumb(stp);
						return(EACCES);
					}
					else {
						UNLOCK(&u.u_procp->p_sess_mutex, PLSTR);
						UNLOCK(stp->sd_mutex, pl);
					}
				}
				strioc.ic_len = sizeof(char);
				strioc.ic_dp = (char *)arg;
				error = strdoioctl(stp, &strioc, NULL,
				    copyflag, "c", crp, rvalp);
				relplumb(stp);
				return(error);

			case TCGETA:
				fmt = STRTERMIO;
				/* FALLTHROUGH */

			case TCGETS:
				if (fmt == NULL)
					fmt = STRTERMIOS;
				/* FALLTHROUGH */

			case LDGETT:
				if (fmt == NULL)
					fmt = STRTERMCB;
				/* FALLTHROUGH */

			case TIOCGETP:
				if (fmt == NULL)
					fmt = STRSGTTYB;
				strioc.ic_len = 0;
				strioc.ic_dp = (char *)arg;
				error = strdoioctl(stp, &strioc, NULL,
				    copyflag, fmt, crp, rvalp);
				relplumb(stp);
				return(error);
			default:
				/*
				 * note that sd_plumb is held since this ioctl
				 * will get handled below.
				 */
				haveplumb = 1;
				break;
			}
		}

		/*
		 * Unknown cmd - send down request to support
		 * transparent ioctls.
		 */

		/*
		 * NOTE - getplumb returns with sd_mutex unlocked
		 */
		if (!haveplumb && (error = getplumb(stp)))
			return(error);

		/*
		 * But first check if any platform-specific transparent 
		 * ioctl data is required to send downstream.
		 */
		STRIOCTL_P(vp, &cmd, &arg, flag, copyflag, crp, rvalp,
			(struct strioctlstate **) &iocstatep, &bp1, &error);
		if (error) {
			relplumb(stp);
			return(error);
		}

		strioc.ic_cmd = cmd;
		strioc.ic_timout = INFTIM;
		strioc.ic_len = TRANSPARENT;
		strioc.ic_dp = (char *)&arg;

		error = strdoioctl(stp, &strioc, bp1, copyflag, (char *)NULL,
		    crp, rvalp);
		relplumb(stp);
		return(error);

#ifdef STRPERF
	case I_STATS: {
		struct strperf tmp;
		struct strperf *tmpp;

		tmpp = &tmp;
		FSPIN_LOCK(&stat_mutex);
		*tmpp = strperf;
		FSPIN_UNLOCK(&stat_mutex);
		UNLOCK(stp->sd_mutex, pl);
		error = strcopyout((caddr_t) tmpp, (caddr_t) arg, sizeof(struct strperf), NULL, U_TO_K);
		return(error);
	}
#endif
	case I_STR:
		/*
		 * Stream ioctl.  Read in an strioctl buffer from the user
		 * along with any data specified and send it downstream.
		 * Strdoioctl will wait allow only one ioctl message at
		 * a time, and waits for the acknowledgement.
		 */

		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENXIO);
		}
		UNLOCK(stp->sd_mutex, pl);
		error = strcopyin((caddr_t)arg, (caddr_t)&strioc,
		    sizeof(struct strioctl), STRIOCTL, copyflag);
		if (error)
			return(error);
		if ((strioc.ic_len < 0) || (strioc.ic_timout < -1))
			return(EINVAL);
		/* recheck errors */
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
			error = (stp->sd_flag & STPLEX) ? EINVAL :
		    	(stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}

		/*
		 * NOTE - getplumb returns with sd_mutex unlocked
		 */
		if (error = getplumb(stp))
			return(error);
		STRIOCMAP(stp, strioc);
		error = strdoioctl(stp, &strioc, NULL, copyflag, (char *)NULL,
		    crp, rvalp);
		relplumb(stp);
		if (error == 0)
			error = strcopyout((caddr_t)&strioc, (caddr_t)arg,
			    sizeof(struct strioctl), STRIOCTL, copyflag);
		return(error);

	case I_NREAD:
	case FIONREAD:
		/*
		 * Return number of bytes of data in first message
		 * in queue in "arg" and return the number of messages
		 * in queue in return value.
		 */
	    {
		int size;
		int count;

		size = 0;
		count = 0;
#ifdef STRPERF
		stamp = castimer();
#endif
		mp = RD(stp->sd_wrq)->q_first;
		while (mp) {
			if (!(mp->b_flag & MSGNOGET))
				break;
			mp = mp->b_next;
		}
		if (mp) {
			size = msgdsize(mp);
#ifdef STRPERF
			mp->b_sh += castimer() - stamp;
#endif
		}
		if (cmd == FIONREAD) {
			UNLOCK(stp->sd_mutex, pl);
			error = strcopyout((caddr_t)&size, (caddr_t)arg, 
					sizeof(size), STRINT, copyflag);
			return(error);
		}
#ifdef STRPERF
		/* Don't bother counting this */
#endif
		for (; mp; mp = mp->b_next)
			if (!(mp->b_flag & MSGNOGET))
				count++;
		UNLOCK(stp->sd_mutex, pl);
		*rvalp = count;
		error = strcopyout((caddr_t)&size, (caddr_t)arg, sizeof(size), STRINT,
				copyflag);
		return(error);
	    }
	case FIORDCHK:
		/*
		 * FIORDCHK does not use arg value (like FIONREAD),
	         * instead a count is returned. I_NREAD value may
		 * not be accurate but safe. The real thing to do is
		 * to add the msgdsizes of all data  messages until
		 * a non-data message.
		 */
	    {
		int size;

		size = 0;
		mp = RD(stp->sd_wrq)->q_first;
#ifdef STRPERF
		stamp = castimer();
#endif
		while (mp) {
			if (!(mp->b_flag & MSGNOGET))
				break;
			mp = mp->b_next;
		}
		if (mp) {
			size = msgdsize(mp);
#ifdef STRPERF
			mp->b_sh += castimer() - stamp;
#endif
		}
		UNLOCK(stp->sd_mutex, pl);
		*rvalp = size;
		return(0);
	    }

	case I_FIND:
		/*
		 * Get module name.
		 */
	    {
		char mname[FMNAMESZ+1];
		int error;

		UNLOCK(stp->sd_mutex, pl);
		error = strcopyin((caddr_t)arg, mname, FMNAMESZ+1, STRNAME,
		    copyflag);
		if (error)
			return(error);

		*rvalp = 0;
		error = strmodpushed(stp, mname);
		if (error == 0) {
			/* found it */
			*rvalp = 1;
		} else if (error < 0) {
			/* not found, *rvalp already == 0 */
			error = 0;
		}
		return(error);
	    }

	case I_PUSH:
		/*
		 * Push a module.
		 */

	    {
		int i;
		queue_t *q;
		queue_t *tq;
		qband_t *qbp;
		int idx;
		dev_t dummydev;
		struct streamtab *qinfop;
		uchar_t *rqbf;
		uchar_t *wqbf;
		int renab;
		int wenab;
		int nrband;
		int nwband;
		char mname[FMNAMESZ+1];
		pl_t swpl;

		/*
		 * first do the things we have to do without sd_mutex held.
		 * May have to toss this work, but not in the usual case.  This
		 * eliminates redundent error checking.
		 *
		 * Get module name and look up in fmodsw.
		 */
		UNLOCK(stp->sd_mutex, pl);
		error = strcopyin((caddr_t)arg, mname, FMNAMESZ+1, STRNAME,
		    copyflag);
		if (error)
			return(error);
		if ((idx = findmod(mname)) < 0)
			return(EINVAL);

		/* Don't allow the uniplexor to be pushed */
		swpl = RW_RDLOCK(&mod_fmodsw_lock, PLDLM);
		qinfop = fmodsw[idx].f_str;
		if (qinfop->st_rdinit->qi_minfo->mi_idnum == UNI_ID) {
			RW_UNLOCK(&mod_fmodsw_lock, swpl);
			return(EINVAL);
		}
		RW_UNLOCK(&mod_fmodsw_lock, swpl);

		if (!mem_resv_check())
			return(ENOSR);
		rqbf = kmem_zalloc(NBAND, KM_SLEEP);
		wqbf = kmem_zalloc(NBAND, KM_SLEEP);

		/* Now we need sd_mutex.  Clean up on failures. */
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			kmem_free(rqbf, NBAND);
			kmem_free(wqbf, NBAND);
			return(ENXIO);
		}
		if (stp->sd_pushcnt >= nstrpush) {
			UNLOCK(stp->sd_mutex, pl);
			kmem_free(rqbf, NBAND);
			kmem_free(wqbf, NBAND);
			return(EINVAL);
		}
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
			error = (stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
			UNLOCK(stp->sd_mutex, pl);
			kmem_free(rqbf, NBAND);
			kmem_free(wqbf, NBAND);
			return(error);
		}

		/*
		 * firewall - don't use too much memory
		 */
		if (strthresh && (Strcount > strthresh) && 
			    pm_denied(CRED(), P_SYSOPS)) {
			UNLOCK(stp->sd_mutex, pl);
			kmem_free(rqbf, NBAND);
			kmem_free(wqbf, NBAND);
			return(ENOSR);
		}
		if (SLEEP_TRYLOCK(stp->sd_plumb) == B_FALSE) {
			UNLOCK(stp->sd_mutex, pl);
			if (flag & (FNDELAY|FNONBLOCK)) {
				/* user doesn't want to wait */
				kmem_free(rqbf, NBAND);
				kmem_free(wqbf, NBAND);
				return(EAGAIN);
			}
			/* wait for sd_plumb */
			if (SLEEP_LOCK_SIG(stp->sd_plumb, PRIMED) == B_FALSE) {
				/* interrupt */
				kmem_free(rqbf, NBAND);
				kmem_free(wqbf, NBAND);
				return(EINTR);
			}
			/* recheck errors */
			pl = LOCK(stp->sd_mutex, PLSTR);
			if (stp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
				error = (stp->sd_flag & STPLEX) ? EINVAL :
				    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				kmem_free(rqbf, NBAND);
				kmem_free(wqbf, NBAND);
				return(error);
			}
			if (stp->sd_flag & STRHUP) {
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				kmem_free(rqbf, NBAND);
				kmem_free(wqbf, NBAND);
				return(ENXIO);
			}
			if (stp->sd_pushcnt >= nstrpush) {
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				kmem_free(rqbf, NBAND);
				kmem_free(wqbf, NBAND);
				return(EINVAL);
			}
		}

		/*
		 * now we have sd_plumb
		 */
		q = RD(stp->sd_wrq);
		renab = 0;
		if (q->q_flag & QWANTW) {
			renab = 1;
			rqbf[0] = 1;
		}
		nrband = (int)q->q_nband;
		for (i = 1, qbp = q->q_bandp; i <= nrband; i++) {
			if (qbp->qb_flag & QB_WANTW) {
				renab = 1;
				rqbf[i] = 1;
			}
			qbp = qbp->qb_next;
		}
		q = stp->sd_wrq->q_next;
		for ( ; q && !q->q_qinfo->qi_srvp; q = q->q_next)
			;
		wenab = 0;
		nwband = 0;
		if (q) {
			if (q->q_flag & QWANTW) {
				wenab = 1;
				wqbf[0] = 1;
			}
			nwband = (int)q->q_nband;
			for (i = 1, qbp = q->q_bandp; i <= nwband; i++) {
				if (qbp->qb_flag & QB_WANTW) {
					wenab = 1;
					wqbf[i] = 1;
				}
				qbp = qbp->qb_next;
			}
		}
		stp->sd_wrq->q_flag |= QFREEZE;
		RD(stp->sd_wrq)->q_flag |= QFREEZE;
		UNLOCK(stp->sd_mutex, pl);

		/*
		 * Push new module and call its open routine
		 * via qattach().  Modules don't change device
		 * numbers, so just ignore dummydev here.  Since we hold
		 * sd_plumb, sd_cpu is invariant.
		 */

		STRKBIND(stp, engp, unbind);

		dummydev = vp->v_rdev;
		if ((error = qattach(RD(stp->sd_wrq), &dummydev, 0, FMODSW, idx, crp, 0)) == 0) {
			pl = LOCK(stp->sd_mutex, PLSTR);
			stp->sd_pushcnt++;
			if (vp->v_type == VCHR) { /* sorry, no pipes allowed */
				/*
				 * Controlling tty allocation handled here
				 */
				if (stp->sd_flag & STRISTTY) {
					/*
					 * this is a tty driver - try to
					 * allocate it as a controlling tty
					 */
					if (!(stp->sd_flag & STRNOCTTY))
						(void) stralloctty(stp);
				}
			}
		} else {
			LOCK(stp->sd_mutex, PLSTR);
			tq = RD(stp->sd_wrq);
			for (q= backq_l(tq); q && !q->q_qinfo->qi_srvp; q = backq_l(q))
				;
			if (q) {
				done = 0;
				if (rqbf[0] && !(tq->q_flag & QWANTW))
					done = 1;
				else
					rqbf[0] = 0;
				qbp = tq->q_bandp;
				for (i = 1; i <= nrband; i++) {
					if (rqbf[i] && !(qbp->qb_flag&QB_WANTW))
						done = 1;
					else
						rqbf[i] = 0;
					qbp = qbp->qb_next;
				}
				if (done) {
					qenable_l(q);
					for (i = 0; i <= nrband; i++) {
						if (rqbf[i])
							setqback(q, i);
					}
				}
			}
			for (q = stp->sd_wrq->q_next; q && !q->q_qinfo->qi_srvp;
			    q = q->q_next)
				;
			if (q) {
				done = 0;
				if (wqbf[0] && !(q->q_flag & QWANTW))
					done = 1;
				else
					wqbf[0] = 0;
				qbp = q->q_bandp;
				for (i = 1; i <= nwband; i++) {
					if (wqbf[i] && !(qbp->qb_flag&QB_WANTW))
						done = 1;
					else
						wqbf[i] = 0;
					qbp = qbp->qb_next;
				}
				if (done) {
					qenable_l(stp->sd_wrq);
					for (i = 0; i <= nwband; i++) {
						if (wqbf[i])
						    setqback(stp->sd_wrq, i);
					}
				}
			}
			goto pushdone;
		}

		/*
		 * If flow control is on, don't break it - enable
		 * first back queue with svc procedure.
		 * sd_mutex still held at this point.
		 */
		q = RD(stp->sd_wrq->q_next);
		if (q->q_qinfo->qi_srvp && renab) {
			for (q = backq_l(q); q && !q->q_qinfo->qi_srvp;
			    q = backq_l(q))
				;
			if (q) {
				qenable_l(q);
				for (i = 0; i <= nrband; i++) {
					if (rqbf[i])
						setqback(q, i);
				}
			}
		}
		q = stp->sd_wrq->q_next;
		if (q->q_qinfo->qi_srvp) {
			if (wenab) {
				qenable_l(stp->sd_wrq);
				for (i = 0; i <= nwband; i++) {
					if (wqbf[i])
					    setqback(stp->sd_wrq, i);
				}
			}
		}
pushdone:
		stp->sd_wrq->q_flag &= ~QFREEZE;
		RD(stp->sd_wrq)->q_flag &= ~QFREEZE;

		if ((stp->sd_wrq->q_flag & QTOENAB) ||
		     bcanput_l(stp->sd_wrq->q_next, 0)) {
			qenable_l(stp->sd_wrq);
		}
		if (SV_BLKD(stp->sd_read))
			broadcast |= RBLOCK;
		if (SV_BLKD(stp->sd_write))
			broadcast |= WBLOCK;
		if (SV_BLKD(stp->sd_timer2))
			broadcast |= MBLOCK;

		SLEEP_UNLOCK(stp->sd_plumb);
		UNLOCK(stp->sd_mutex, pl);

		if (broadcast) {
			if (broadcast & RBLOCK)
				SV_BROADCAST(stp->sd_read, 0);
			if (broadcast & WBLOCK)
				SV_BROADCAST(stp->sd_write, 0);
			if (broadcast & MBLOCK)
				SV_BROADCAST(stp->sd_timer2, 0);
		}

		STRKUNBIND(engp, unbind);

		kmem_free((caddr_t)rqbf, NBAND);
		kmem_free((caddr_t)wqbf, NBAND);
		return(error);
	    }

	case I_POP: {
		int renab;
		int nrband;
		qband_t *qbp;
		uchar_t *rqbf;
		int i;

		/*
		 * Pop module (if module exists).
		 * Can't use l.qbf because we might switch cpus down
		 * below, which invalidates that info.
		 */
		rqbf = kmem_zalloc(NBAND, KM_NOSLEEP);
		if (rqbf == NULL) {
			/* not our lucky day */
			UNLOCK(stp->sd_mutex, pl);
			rqbf = kmem_zalloc(NBAND, KM_SLEEP);
			pl = LOCK(stp->sd_mutex, PLSTR);
		}
		if (stp->sd_flag&STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			kmem_free(rqbf, NBAND);
			return(ENXIO);
		}
		/* for broken pipes */
		if (!stp->sd_wrq->q_next) {
			UNLOCK(stp->sd_mutex, pl);
			kmem_free(rqbf, NBAND);
			return(EINVAL);
		}

		if (SLEEP_TRYLOCK(stp->sd_plumb) == B_FALSE) {
			UNLOCK(stp->sd_mutex, pl);
			if (flag & (FNDELAY|FNONBLOCK)) {
				/* user doesn't want to wait */
				kmem_free(rqbf, NBAND);
				return(EAGAIN);
			}
			/* wait for sd_plumb */
			if (SLEEP_LOCK_SIG(stp->sd_plumb, PRIMED) == B_FALSE) {
				/* interrupt */
				kmem_free(rqbf, NBAND);
				return(EINTR);
			}
			/* recheck errors */
			pl = LOCK(stp->sd_mutex, PLSTR);
			if (stp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
				error = (stp->sd_flag & STPLEX) ? EINVAL :
				    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				kmem_free(rqbf, NBAND);
				return(error);
			}
			if (stp->sd_flag & STRHUP) {
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				kmem_free(rqbf, NBAND);
				return(ENXIO);
			}
			/* for broken pipes */
			if (!stp->sd_wrq->q_next) {
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				kmem_free(rqbf, NBAND);
				return(EINVAL);
			}
		}

		/*
		 * now we have sd_plumb
		 */
		stp->sd_wrq->q_flag |= QFREEZE;
		RD(stp->sd_wrq)->q_flag |= QFREEZE;

		if (stp->sd_wrq->q_next->q_next &&
		    !(stp->sd_wrq->q_next->q_flag & QREADR)) {
			stp->sd_pushcnt--;
			q = RD(stp->sd_wrq->q_next);
			renab = 0;
			if (q->q_flag & QWANTW) {
				renab = 1;
				rqbf[0] = 1;
			}
			nrband = (int)q->q_nband;
			for (i = 1, qbp = q->q_bandp; i <= nrband; i++) {
				if (qbp->qb_flag & QB_WANTW) {
					renab = 1;
					rqbf[i] = 1;
				}
				qbp = qbp->qb_next;
			}
			UNLOCK(stp->sd_mutex, pl);

			STRKBIND(stp, engp, unbind);
			qdetach(RD(stp->sd_wrq->q_next), 1, flag, crp);
			STRKUNBIND(engp, unbind);

			pl = LOCK(stp->sd_mutex, PLSTR);
		} else {
			error = EINVAL;
		}

		stp->sd_wrq->q_flag &= ~QFREEZE;
		RD(stp->sd_wrq)->q_flag &= ~QFREEZE;

		if ((stp->sd_wrq->q_flag & QTOENAB) || bcanput_l(stp->sd_wrq->q_next, 0)) {
			qenable_l(stp->sd_wrq);
		}
		/* wakeup writers */
		if (SV_BLKD(stp->sd_write))
			broadcast |= WBLOCK;

		if (renab && !error) {
			q = backq_l(RD(stp->sd_wrq->q_next));
			for (; q && !q->q_qinfo->qi_srvp; q = backq_l(q))
				;
			if (q) {
				qenable_l(q);
				for (i = 0; i <= nrband; i++) {
					if (rqbf[i])
						setqback(q, i);
				}
			}
		}
		/* wakeup readers */
		if (SV_BLKD(stp->sd_read))
			broadcast |= RBLOCK;

		SLEEP_UNLOCK(stp->sd_plumb);
		if (SV_BLKD(stp->sd_timer2))
			broadcast |= MBLOCK;

		UNLOCK(stp->sd_mutex, pl);
		if (broadcast) {
			if (broadcast & RBLOCK)
				SV_BROADCAST(stp->sd_read, 0);
			if (broadcast & WBLOCK)
				SV_BROADCAST(stp->sd_write, 0);
			if (broadcast & MBLOCK)
				SV_BROADCAST(stp->sd_timer2, 0);
		}
		kmem_free(rqbf, NBAND);
		return(error);
	    }

	case I_LOOK:
		/*
		 * Get name of first module downstream.
		 * If no module, return an error.
		 */
	    {
		int i;
		pl_t swpl;

		swpl = RW_RDLOCK(&mod_fmodsw_lock, PLDLM);
                for (i = 0; i < fmodswsz; i++) {
                        if (fmodsw[i].f_str == NULL) {
				/* No more modules. */
                                break;
			}
			if (fmodsw[i].f_str->st_wrinit == stp->sd_wrq->q_next->q_qinfo) {
				RW_UNLOCK(&mod_fmodsw_lock, swpl);
				UNLOCK(stp->sd_mutex, pl);
				error = strcopyout(fmodsw[i].f_name,
				    (char *)arg, FMNAMESZ+1, STRNAME, copyflag);
				return(error);
			}
		}
		RW_UNLOCK(&mod_fmodsw_lock, swpl);
		UNLOCK(stp->sd_mutex, pl);
		return(EINVAL);
	    }

	case I_LINK:
	case I_PLINK:
		/*
		 * Link a multiplexor.
		 */
	    {
		struct file *fpdown;
		struct linkinfo *linkp;
		struct stdata *stpdown;
		queue_t *rq;
		int tmpidx;
		dev_t dummydev;
		int upmux;
		int special;
		int renab;
		int nrband;
		int i;
		qband_t *qbp;

		special = 0;
		UNLOCK(stp->sd_mutex, pl);
		/*
		 * NOTE - getf() invokes FTE_HOLD() which does f_count++;
		 * hereafter, whenever we do error return, we have to call
		 * FTE_RELE() to decrement the reference.
		 */ 
		if (error = getf(arg, &fpdown))
			return(error);
		/* Test for invalid lower stream. */
		stpdown = fpdown->f_vnode->v_stream;
		if (! stpdown || (stpdown == stp) || linkcycle(stp, stpdown)) {
			error = EINVAL;
			goto link_exit;
		}

		/*
		 * Test for invalid upper stream
		 */
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			error = ENXIO;
			goto link_exit;
		}
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
			error = (stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
			UNLOCK(stp->sd_mutex, pl);
			goto link_exit;
		}
		if (vp->v_type == VFIFO) {
			UNLOCK(stp->sd_mutex, pl);
			error = EINVAL;
			goto link_exit;
		}
		if (! stp->sd_strtab->st_muxwinit) {
			UNLOCK(stp->sd_mutex, pl);
			error = EINVAL;
			goto link_exit;
		}
		/*
		 * NOTE - getplumb returns with sd_mutex unlocked
		 */
		if (error = getplumb(stp))
			goto link_exit;
		pl = LOCK(stpdown->sd_mutex, PLSTR);
		if (stpdown->sd_flag & (STPLEX|STRHUP|STRDERR|STWRERR|IOCWAIT)) {
			UNLOCK(stpdown->sd_mutex, pl);
			relplumb(stp);
			error = EINVAL;
			goto link_exit;
		}
		/*
		 * To avoid deadlock, check if the bottom stream's sd_plumb is
		 * available before trying to acquire it.  If unavailable, don't
		 * bother and return with an error.
		 */
		if (SLEEP_TRYLOCK(stpdown->sd_plumb) == B_FALSE) {
			UNLOCK(stpdown->sd_mutex, pl);
			relplumb(stp);
			error = EINVAL;
			goto link_exit;
		}
		UNLOCK(stpdown->sd_mutex, pl);

		/*
		 * At this point, both stream's sd_plumb's have been acquired
		 * and both sd_mutex's have been released.  sd_cpu is invarient.
		 *
		 * if both are bound, but not to the same place then fail
		 */
		if (stp->sd_cpu && stpdown->sd_cpu && (stp->sd_cpu != stpdown->sd_cpu)) {
			pl = LOCK(stpdown->sd_mutex, PLSTR);
			SLEEP_UNLOCK(stpdown->sd_plumb);
			if (SV_BLKD(stpdown->sd_timer2)) {
				UNLOCK(stpdown->sd_mutex, pl);
				SV_BROADCAST(stpdown->sd_timer2, 0);
			} else {
				UNLOCK(stpdown->sd_mutex, pl);
			}
			relplumb(stp);
			return(EINVAL);
		}
		rq = getendq(stp->sd_wrq);
		/* QUP flag doesn't change so no lock needed */
		upmux = rq->q_flag & QUP;
		if (cmd == I_PLINK)
			rq = NULL;
		if (! (linkp = alloclink(rq, stpdown->sd_wrq, fpdown))) {
			error = EAGAIN;
			goto link_exit;
		}
		strioc.ic_cmd = cmd;
		strioc.ic_timout = 0;
		strioc.ic_len = sizeof(struct linkblk);
		strioc.ic_dp = (char *) &linkp->li_lblk;

		/* Set up queues for link */
		pl = LOCK(stpdown->sd_mutex, PLSTR);
		rq = RD(stpdown->sd_wrq);
		freezeprocs(rq);
		ASSERT(rq->q_flag & QPROCSON);
		ASSERT(WR(rq)->q_flag & QPROCSON);
		if (!stpdown->sd_cpu && upmux) {
			/*
			 * ugly, this is a upmux over and unbound stream.
			 * Have to do special stuff to prevent messages
			 * from getting through on the wrong processor.
			 */
			stpdown->sd_wrq->q_flag |= (QBOUND|QUP);
			rq->q_flag |= (QBOUND|QUP);
			stpdown->sd_cpu = stp->sd_cpu;
			special = 1;
		}
		setq(rq, stp->sd_strtab->st_muxrinit, stp->sd_strtab->st_muxwinit);
		rq->q_ptr = WR(rq)->q_ptr = NULL;
		rq->q_putp = rq->q_qinfo->qi_putp;
		WR(rq)->q_putp = WR(rq)->q_qinfo->qi_putp;
		rq->q_flag |= QWANTR;
		WR(rq)->q_flag |= QWANTR;
		/*
		 * Unfreeze head queue-pair.  The only thing to do is reset the
		 * read side flow-control since write side is not active on
		 * the bottom stream.
		 */
		renab = 0;
		bzero((caddr_t)l.qbf, NBAND);
		if (rq->q_flag & QWANTW) {
			renab = 1;
			l.qbf[0] = 1;
		}
		nrband = (int)rq->q_nband;
		for (i = 1, qbp = rq->q_bandp; i <= nrband; i++) {
			if (qbp->qb_flag & QB_WANTW) {
				renab = 1;
				l.qbf[i] = 1;
			}
			qbp = qbp->qb_next;
		}
		if (renab) {
			for (q = backq_l(RD(stpdown->sd_wrq->q_next));
			     q && ! q->q_qinfo->qi_srvp; q = backq_l(q))
				;
			if (q) {
				qenable_l(q);
				for (i = 0; i <= nrband; i++) {
					if (l.qbf[i])
						setqback(q, i);
				}
			}
		}
		UNLOCK(stpdown->sd_mutex, pl);

		if (stpdown->sd_cpu && !upmux) {
			/*
			 * Soon-to-be bottom half of mux is bound
			 */
			pl = LOCK(stp->sd_mutex, PLSTR);
			if ((Nengine > 1) && !(stp->sd_flag & UPF)) {
				/* multiplexor is not UP-friendly */
				UNLOCK(stp->sd_mutex, pl);
				if ((tmpidx = findmod("uni")) < 0) {
					/*
					 *+ Uniprocessor compatibility module
					 *+ is missing
					 */
					cmn_err(CE_WARN, "KERNEL:strioctl: uniplexor is missing\n");
					error = EINVAL;
					goto link_exit2;
				}
				dummydev = vp->v_rdev;
				/*
				 * uniplexor does not send messages downstream
				 * so do not need to kbind here
				 */
				(void) qattach(RD(stpdown->sd_wrq), &dummydev, 0, FMODSW, tmpidx, crp, 0);
			} else {
				UNLOCK(stp->sd_mutex, pl);
			}
		} else if (special && (Nengine > 1)) {
			/*
			 * this is an unbound lower stream under a upmux.  Two
			 * choices exist, bind the lower stream or stick in
			 * the uniplexor.  Neither will perform very well, but
			 * pushing the uniplexor is easier than binding the
			 * stream at this point.
			 */
			if ((tmpidx = findmod("uni")) < 0) {
				/*
				 *+ Uniprocessor compatibility module
				 *+ is missing
				 */
				cmn_err(CE_WARN, "KERNEL:strioctl: uniplexor is missing\n");
				error = EINVAL;
				goto link_exit2;
			}
			dummydev = vp->v_rdev;
			/*
			 * uniplexor does not send messages downstream
			 * so do not need to kbind here
			 */
			(void) qattach(RD(stpdown->sd_wrq), &dummydev, 0, FMODSW, tmpidx, crp, 0);
		}

		/* both streams are unlocked at this point */
		if (error = strdoioctl(stp, &strioc, NULL, K_TO_K, STRLINK, crp, rvalp)) {
link_exit2:
			FTE_RELE(fpdown);
			lbfree(linkp);
			setq(rq, &strdata, &stwdata);
			rq->q_ptr = WR(rq)->q_ptr = (caddr_t)stpdown;
			pl = LOCK(stpdown->sd_mutex, PLSTR);
			SLEEP_UNLOCK(stpdown->sd_plumb);
			if (SV_BLKD(stpdown->sd_timer2)) {
				UNLOCK(stpdown->sd_mutex, pl);
				SV_BROADCAST(stpdown->sd_timer2, 0);
			} else {
				UNLOCK(stpdown->sd_mutex, pl);
			}
			relplumb(stp);
			return(error);
		}
		pl = LOCK(stpdown->sd_mutex, PLSTR);
		stpdown->sd_flag |= STPLEX;
		if (error = mux_addedge(stp, stpdown, linkp->li_lblk.l_index)) {
			int type;

			UNLOCK(stpdown->sd_mutex, pl);
			if (cmd == I_LINK)
				type = LINKIOCTL|LINKNORMAL|LINKNOEDGE;
			else	/* I_PLINK */
				type = LINKIOCTL|LINKPERSIST|LINKNOEDGE;
			FTE_RELE(fpdown);
			(void) munlink(stp, linkp, type, crp, rvalp);
			pl = LOCK(stpdown->sd_mutex, PLSTR);
			SLEEP_UNLOCK(stpdown->sd_plumb);
			if (SV_BLKD(stpdown->sd_timer2)) {
				UNLOCK(stpdown->sd_mutex, pl);
				SV_BROADCAST(stpdown->sd_timer2, 0);
			} else {
				UNLOCK(stpdown->sd_mutex, pl);
			}
			relplumb(stp);
			return(error);
		}
		/*
		 * Wake up any other processes that may have been
		 * waiting on the lower stream.  These will all
		 * error out.
		 */
		SLEEP_UNLOCK(stpdown->sd_plumb);
		if (SV_BLKD(stp->sd_read))
			broadcast |= RBLOCK;
		if (SV_BLKD(stp->sd_write))
			broadcast |= WBLOCK;
		if (SV_BLKD(stp->sd_timer2))
			broadcast |= MBLOCK;
		UNLOCK(stpdown->sd_mutex, pl);
		if (broadcast) {
			if (broadcast & RBLOCK)
				SV_BROADCAST(stp->sd_read, 0);
			if (broadcast & WBLOCK)
				SV_BROADCAST(stp->sd_write, 0);
			if (broadcast & MBLOCK)
				SV_BROADCAST(stp->sd_timer2, 0);
		}
		relplumb(stp);
		*rvalp = linkp->li_lblk.l_index;
		return(0);
link_exit:
		FTE_RELE(fpdown);
		return(error);
	    }

	case I_UNLINK:
	case I_PUNLINK:
		/*
		 * Unlink a multiplexor.
		 * If arg is -1, unlink all links for which this is the
		 * controlling stream.  Otherwise, arg is a index number
		 * for a link to be removed.
		 *
		 * Note that as long as the STPLEX flag is set on the bottom
		 * stream, all operations from user level will be refused;
		 * thus, we do not need to worry about acquiring its sd_plumb.
		 */
	    {
		struct linkinfo *linkp;
		int type;

		if (vp->v_type == VFIFO) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		if (arg == 0) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		if (cmd == I_UNLINK)
			type = LINKIOCTL|LINKNORMAL;
		else	/* I_PUNLINK */
			type = LINKIOCTL|LINKPERSIST;

		if (SLEEP_TRYLOCK(stp->sd_plumb) == B_FALSE) {
			UNLOCK(stp->sd_mutex, pl);
			if (flag & (FNDELAY|FNONBLOCK)) {
				/* user doesn't want to wait */
				return(EAGAIN);
			}
			/* wait for sd_plumb */
			if (SLEEP_LOCK_SIG(stp->sd_plumb, PRIMED) == B_FALSE) {
				/* interrupt */
				return(EINTR);
			}
			/* recheck errors */
			pl = LOCK(stp->sd_mutex, PLSTR);
			if (stp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
				error = (stp->sd_flag & STPLEX) ? EINVAL :
				    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				return(error);
			}
		}

		/*
		 * now we have sd_plumb, can release sd_mutex
		 */
		UNLOCK(stp->sd_mutex, pl);

		if (arg == MUXID_ALL)
			error = munlinkall(stp, type, crp, rvalp);
		else {
			if (! (linkp = findlinks(stp, arg, type))) {
				/* invalid user supplied index number */
				pl = LOCK(stp->sd_mutex, PLSTR);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				return(EINVAL);
			}
			error = munlink(stp, linkp, type, crp, rvalp);
		}
		pl = LOCK(stp->sd_mutex, PLSTR);
		SLEEP_UNLOCK(stp->sd_plumb);
		if (SV_BLKD(stp->sd_timer2)) {
			UNLOCK(stp->sd_mutex, pl);
			SV_BROADCAST(stp->sd_timer2, 0);
		} else {
			UNLOCK(stp->sd_mutex, pl);
		}
		return(error);
	    }

	case I_FLUSH:
		/*
		 * send a flush message downstream
		 * flush message can indicate
		 * FLUSHR - flush read queue
		 * FLUSHW - flush write queue
		 * FLUSHRW - flush read/write queue
		 */
		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENXIO);
		}
		if (arg & ~FLUSHRW) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		if (SLEEP_TRYLOCK(stp->sd_plumb) == B_FALSE) {
			UNLOCK(stp->sd_mutex, pl);
			if (flag & (FNDELAY|FNONBLOCK)) {
				/* user doesn't want to wait */
				return(EAGAIN);
			}
			/* wait for sd_plumb */
			if (SLEEP_LOCK_SIG(stp->sd_plumb, PRIMED) == B_FALSE) {
				/* interrupt */
				return(EINTR);
			}
			/* recheck errors */
			pl = LOCK(stp->sd_mutex, PLSTR);
			if (stp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
				error = (stp->sd_flag & STPLEX) ? EINVAL :
				    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				return(error);
			}
			if (stp->sd_flag & STRHUP) {
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				return(ENXIO);
			}
		}

		/* Now we have sd_plumb, that'll stop UP pushes too */
		UNLOCK(stp->sd_mutex, pl);

		STRKBIND(stp, engp, unbind);

		while (!putnextctl1(stp->sd_wrq, M_FLUSH, arg)) {
			if (error = strwaitbuf(1, BPRI_HI, stp)) {
				pl = LOCK(stp->sd_mutex, PLSTR);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				STRKUNBIND(engp, unbind);
				return(error);
			}
		}

		pl = LOCK(stp->sd_mutex, PLSTR);
		SLEEP_UNLOCK(stp->sd_plumb);
		if (SV_BLKD(stp->sd_timer2)) {
			UNLOCK(stp->sd_mutex, pl);
			SV_BROADCAST(stp->sd_timer2, 0);
		} else {
			UNLOCK(stp->sd_mutex, pl);
		}
		STRKUNBIND(engp, unbind);
		return(0);

	case I_FLUSHBAND:
	    {
		struct bandinfo binfo;

		UNLOCK(stp->sd_mutex, pl);
		error = strcopyin((caddr_t)arg, (caddr_t)&binfo, sizeof(struct
		    bandinfo), STRBANDINFO, copyflag);
		if (error)
			return(error);
		if (binfo.bi_flag & ~FLUSHRW)
			return(EINVAL);

		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENXIO);
		}
		if (SLEEP_TRYLOCK(stp->sd_plumb) == B_FALSE) {
			UNLOCK(stp->sd_mutex, pl);
			if (flag & (FNDELAY|FNONBLOCK)) {
				/* user doesn't want to wait */
				return(EAGAIN);
			}
			/* wait for sd_plumb */
			if (SLEEP_LOCK_SIG(stp->sd_plumb, PRIMED) == B_FALSE) {
				/* interrupt */
				return(EINTR);
			}
			/* recheck errors */
			pl = LOCK(stp->sd_mutex, PLSTR);
			if (stp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
				error = (stp->sd_flag & STPLEX) ? EINVAL :
				    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				return(error);
			}
			if (stp->sd_flag & STRHUP) {
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				return(ENXIO);
			}
		}

		/* now we have sd_plumb */
		UNLOCK(stp->sd_mutex, pl);
		while (!(mp = allocb(2, BPRI_HI))) {
			if (error = strwaitbuf(2, BPRI_HI, stp)) {
				pl = LOCK(stp->sd_mutex, PLSTR);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				return(error);
			}
		}
#ifdef STRPERF
		stamp = castimer();
#endif
		mp->b_datap->db_type = M_FLUSH;
		*mp->b_wptr++ = binfo.bi_flag | FLUSHBAND;
		*mp->b_wptr++ = binfo.bi_pri;
		STRKBIND(stp, engp, unbind);
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		putnext(stp->sd_wrq, mp);
		pl = LOCK(stp->sd_mutex, PLSTR);
		SLEEP_UNLOCK(stp->sd_plumb);
		if (SV_BLKD(stp->sd_timer2)) {
			UNLOCK(stp->sd_mutex, pl);
			SV_BROADCAST(stp->sd_timer2, 0);
		} else {
			UNLOCK(stp->sd_mutex, pl);
		}
		STRKUNBIND(engp, unbind);
		return(0);
	    }

	case I_SRDOPT:
		/*
		 * Set read options
		 *
		 * RNORM - default stream mode
		 * RMSGN - message no discard
		 * RMSGD - message discard
		 * RPROTNORM - fail read with EBADMSG for M_[PC]PROTOs
		 * RPROTDAT - convert M_[PC]PROTOs to M_DATAs
		 * RPROTDIS - discard M_[PC]PROTOs and retain M_DATAs
		 */
	    {
		long oldflag;

		if (arg & ~(RMODEMASK | RPROTMASK)) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}

		oldflag = stp->sd_flag;
		switch (arg & RMODEMASK) {
		case RNORM:
			stp->sd_flag &= ~(RMSGDIS | RMSGNODIS);
			break;
		case RMSGD:
			stp->sd_flag = (stp->sd_flag & ~RMSGNODIS) | RMSGDIS;
			break;
		case RMSGN:
			stp->sd_flag = (stp->sd_flag & ~RMSGDIS) | RMSGNODIS;
			break;
		default:	/* more than 1 bit set, note: RNORM is 0 */
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}

		switch(arg & RPROTMASK) {
		case RPROTNORM:
			stp->sd_flag &= ~(RDPROTDAT | RDPROTDIS);
			break;

		case RPROTDAT:
			stp->sd_flag = ((stp->sd_flag & ~RDPROTDIS) |
			    RDPROTDAT);
			break;

		case RPROTDIS:
			stp->sd_flag = ((stp->sd_flag & ~RDPROTDAT) |
			    RDPROTDIS);
			break;

		case 0:	/* setting none is ok */
			break;

		default:	/* more than 1 bit set */
			/* undo above */
			stp->sd_flag = oldflag;
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	    }

	case I_GRDOPT:
		/*
		 * Get read option and return the value
		 * to spot pointed to by arg
		 */
	    {
		int rdopt;

		rdopt = ((stp->sd_flag & RMSGDIS) ? RMSGD :
			  ((stp->sd_flag & RMSGNODIS) ? RMSGN : RNORM));
		rdopt |= ((stp->sd_flag & RDPROTDAT) ? RPROTDAT :
			  ((stp->sd_flag & RDPROTDIS) ? RPROTDIS : RPROTNORM));

		UNLOCK(stp->sd_mutex, pl);
		error = strcopyout((caddr_t)&rdopt, (caddr_t)arg, sizeof(rdopt), STRINT,
		    copyflag);
		return(error);
	    }
	case I_SETSIG:
		/*
		 * Register the calling lwp to receive the SIGPOLL
		 * signal based on the events given in arg.  If
		 * arg is zero, remove the lwp from register list.
		 */
	    {
		struct strevent *sep;
		struct strevent *psep;
		struct strevent *tsep;
		pl_t pls;

		/* Have to play around to get the locks we need */
		UNLOCK(stp->sd_mutex, pl);
		pls = LOCK(&strsig_mutex, PLSTR);
		pl = LOCK(stp->sd_mutex, PLSTR);
		/* recheck errors */
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
			error = (stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
			UNLOCK(stp->sd_mutex, pl);
			UNLOCK(&strsig_mutex, pls);
			return(error);
		}
		psep = NULL;
		for (sep = stp->sd_siglist; sep && (sep->se_lwpp !=
		     u.u_lwpp); psep = sep, sep = sep->se_next)
			;

		if (arg) {
			if (arg & ~(S_INPUT|S_HIPRI|S_MSG|S_HANGUP|S_ERROR|
			    S_RDNORM|S_WRNORM|S_RDBAND|S_WRBAND|S_BANDURG)) {
				UNLOCK(stp->sd_mutex, pl);
				UNLOCK(&strsig_mutex, pls);
				return(EINVAL);
			}
			if ((arg & S_BANDURG) && !(arg & S_RDBAND)) {
				UNLOCK(stp->sd_mutex, pl);
				UNLOCK(&strsig_mutex, pls);
				return(EINVAL);
			}

			/*
			 * If lwp not already registered, add it
			 * to list.
			 */
			if (!sep) {
				if (!(sep = sealloc(SE_NOSLP))) {
					UNLOCK(stp->sd_mutex, pl);
					UNLOCK(&strsig_mutex, pls);
					return(EAGAIN);
				}
				if (psep)
					psep->se_next = sep;
				else
					stp->sd_siglist = sep;
				sep->se_lwpp = u.u_lwpp;
				sep->se_vp = vp;
				sep->se_chain = sep->se_lwpp->l_sep;
				sep->se_lwpp->l_sep = sep;
			}

			/*
			 * Set events.
			 */
			sep->se_events = arg;
		} else {

			/*
			 * Remove lwp from register list.
			 */
			if (sep) {
				if (psep)
					psep->se_next = sep->se_next;
				else
					stp->sd_siglist = sep->se_next;
				psep = NULL;
				tsep = sep->se_lwpp->l_sep;
				while (tsep != sep) {
					psep = tsep;
					tsep = tsep->se_chain;
					ASSERT(tsep);
				}
				if (psep)
					psep->se_chain = tsep->se_chain;
				else
					sep->se_lwpp->l_sep = tsep->se_chain;
				sefree(sep);
			} else {
				UNLOCK(stp->sd_mutex, pl);
				UNLOCK(&strsig_mutex, pls);
				return(EINVAL);
			}
		}

		/*
		 * Recalculate OR of sig events.
		 */
		stp->sd_sigflags = 0;
		for (sep = stp->sd_siglist; sep; sep = sep->se_next)
			stp->sd_sigflags |= sep->se_events;
		UNLOCK(stp->sd_mutex, pl);
		UNLOCK(&strsig_mutex, pls);
		return(0);
	    }

	case I_GETSIG:
		/*
		 * Return (in arg) the current registration of events
		 * for which the calling proc is to be signalled.
		 */
	    {
		struct strevent *sep;

		for (sep = stp->sd_siglist; sep; sep = sep->se_next)
			if (sep->se_lwpp == u.u_lwpp) {
				UNLOCK(stp->sd_mutex, pl);
				error = strcopyout((caddr_t)&sep->se_events,
				    (caddr_t)arg, sizeof(int), STRINT, copyflag);
				return(error);
			}
		UNLOCK(stp->sd_mutex, pl);
		return(EINVAL);
	    }

	case I_PEEK:
		UNLOCK(stp->sd_mutex, pl);
		error = strpeek(stp, arg, copyflag, rvalp);
		return(error);

	case I_FDINSERT:
		UNLOCK(stp->sd_mutex, pl);
		error = strfdinsert(stp, arg, copyflag, flag);
		return(error);

	case I_SENDFD:
	    {
		queue_t *qp;
		struct adt_strrecvfd *adtstr;
		struct strrecvfd *srf;
		struct adtrecvfd *arf;
		struct file *fp;

		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENXIO);
		}

		for (qp = stp->sd_wrq; qp->q_next; qp = qp->q_next)
			;
		if ((qp->q_qinfo != &strdata) && ((stp->sd_flag & STRLOOP) == 0)) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		UNLOCK(stp->sd_mutex, pl);
		/*
		 * NOTE - There is no corresponding FTE_RELE for this getf(),
		 * except that I_SENFD is failed, because we need the
		 * incremented reference to prevent the file from going away
		 * until someone receives the fp.
		 */
	 	if (error = getf(arg, &fp))
			return(error);
		if (! canputnext(stp->sd_wrq) ||
		    ! (mp = allocb(sizeof(struct adt_strrecvfd), BPRI_MED))) {
			FTE_RELE(fp);
			return(EAGAIN);
		}
#ifdef STRPERF
		stamp = castimer();
#endif
		/* LINTED pointer alignment */
		adtstr = (struct adt_strrecvfd *) mp->b_rptr;
		srf = (struct strrecvfd *) &adtstr->adt_strrecvfd;
		arf = (struct adtrecvfd *) &adtstr->adt_adtrecvfd;
		mp->b_wptr += sizeof(struct adt_strrecvfd);
		mp->b_datap->db_type = M_PASSFP;
		srf->f.fp = fp;
		srf->uid = crp->cr_uid;
		srf->gid = crp->cr_gid;
		arf->adtrfd_sendpid = u.u_procp->p_pidp->pid_id;
		arf->adtrfd_sendlwpid = u.u_lwpp->l_lwpid;
		pl = LOCK(stp->sd_mutex, PLSTR);
		stp->sd_upbcnt++;
		UNLOCK(stp->sd_mutex, pl);
		if (stp->sd_cpu) {
			engp = kbind(stp->sd_cpu);
			DISABLE_PRMPT();
			u.u_lwpp->l_notrt++;
			unbind = 1;
		}
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		putnext(stp->sd_wrq, mp);
		pl = LOCK(stp->sd_mutex, PLSTR);
		if ((--stp->sd_upbcnt == 0) && (stp->sd_flag & UPBLOCK)) {
			UNLOCK(stp->sd_mutex, pl);
			SV_BROADCAST(stp->sd_upblock, 0);
		} else {
			UNLOCK(stp->sd_mutex, pl);
		}
		if (unbind) {
			ASSERT(u.u_lwpp->l_notrt != 0);
			u.u_lwpp->l_notrt--;
			ENABLE_PRMPT();
			kunbind(engp);
		}
		return(0);
	    }

	case I_RECVFD:
	case I_E_RECVFD:
	case I_S_RECVFD:
		error = strrecvfd(stp, crp, copyflag, cmd, flag, arg, pl);
		return(error);

	case I_SWROPT:
		/*
		 * Set/clear the write options. arg is a bit
		 * mask with any of the following bits set...
		 * 	SNDZERO - send zero length message
		 *	SNDPIPE - send sigpipe to process if
		 *		sd_werror is set and process is
		 *		doing a write or putmsg.
		 * The new stream head write options should reflect
		 * what is in arg.
		 */
		if (arg & ~(SNDZERO|SNDPIPE)) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}

		stp->sd_flag &= ~(STRSNDZERO|STRSIGPIPE);
		if (arg & SNDZERO)
			stp->sd_flag |= STRSNDZERO;
		if (arg & SNDPIPE)
			stp->sd_flag |= STRSIGPIPE;
		UNLOCK(stp->sd_mutex, pl);
		return(0);

	case I_GWROPT:
	    {
		int wropt;

		wropt = 0;
		if (stp->sd_flag & STRSNDZERO)
			wropt |= SNDZERO;
		if (stp->sd_flag & STRSIGPIPE)
			wropt |= SNDPIPE;
		UNLOCK(stp->sd_mutex, pl);
		error = strcopyout((caddr_t)&wropt, (caddr_t)arg, sizeof(wropt), STRINT,
		    copyflag);
		return(error);
	    }

	case I_LIST:
		/*
		 * Returns all the modules found on this stream,
		 * upto the driver. If argument is NULL, return the
		 * number of modules (including driver). If argument
		 * is not NULL, copy the names into the structure
		 * provided.
		 */

	    {
		queue_t *q;
		int num_modules;
		int space_allocated;
		struct str_list strlist;

		if (arg == NULL) {
			/*
			 * Return number of modules plus driver.  If the
			 * uniplexor is on the stream, it's not included
			 * in this total.
			 */
			*rvalp = stp->sd_pushcnt + 1;
			UNLOCK(stp->sd_mutex, pl);
			return(0);
		} else {
			UNLOCK(stp->sd_mutex, pl);
			error = strcopyin((caddr_t)arg, (caddr_t)&strlist,
			    sizeof(struct str_list), STRLIST, copyflag);
			if (error)
				return(error);
			if ((space_allocated = strlist.sl_nmods) <= 0)
				return(EINVAL);
			q = stp->sd_wrq;
			num_modules = 0;

			/*
			 * since we're going to be perusing the entire
			 * stream, grab sd_plumb so it doesn't change
			 * configuration on us
			 */
			pl = LOCK(stp->sd_mutex, PLSTR);
			/* recheck errors */
			if (stp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
				error = (stp->sd_flag & STPLEX) ? EINVAL :
				    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
				UNLOCK(stp->sd_mutex, pl);
				return(error);
			}
			if (SLEEP_TRYLOCK(stp->sd_plumb) == B_FALSE) {
				UNLOCK(stp->sd_mutex, pl);
				if (flag & (FNDELAY|FNONBLOCK)) {
					/* user doesn't want to wait */
					return(EAGAIN);
				}
				/* wait for sd_plumb */
				if (SLEEP_LOCK_SIG(stp->sd_plumb, PRIMED) == B_FALSE) {
					/* interrupt */
					return(EINTR);
				}
				/* recheck errors */
				pl = LOCK(stp->sd_mutex, PLSTR);
				if (stp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
					error = (stp->sd_flag & STPLEX) ? EINVAL :
					    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
					SLEEP_UNLOCK(stp->sd_plumb);
					if (SV_BLKD(stp->sd_timer2)) {
						UNLOCK(stp->sd_mutex, pl);
						SV_BROADCAST(stp->sd_timer2, 0);
					} else {
						UNLOCK(stp->sd_mutex, pl);
					}
					return(error);
				}
			}

			/*
			 * now we have sd_plumb, can release sd_mutex
			 */
			UNLOCK(stp->sd_mutex, pl);
			while (SAMESTR(q) && (space_allocated != 0)) {
				/*
				 * If we hit the uniplexor, skip it - it's
				 * transparent to the user.  Doing this
				 * requires knowledge of the internals of
				 * the uniplexor.  Since we hold sd_plumb,
				 * the uniplexor can't be in open or close so
				 * this is safe.
				 */
				if (q->q_next->q_qinfo->qi_minfo->mi_idnum == UNI_ID) {
					q = q->q_next->q_ptr;
					continue;
				}
				error = strcopyout(
				    q->q_next->q_qinfo->qi_minfo->mi_idname,
				    (caddr_t)strlist.sl_modlist, (FMNAMESZ + 1),
				    STRNAME, copyflag);
				if (error) {
					pl = LOCK(stp->sd_mutex, PLSTR);
					SLEEP_UNLOCK(stp->sd_plumb);
					if (SV_BLKD(stp->sd_timer2)) {
						UNLOCK(stp->sd_mutex, pl);
						SV_BROADCAST(stp->sd_timer2, 0);
					} else {
						UNLOCK(stp->sd_mutex, pl);
					}
					return(error);
				}
				q = q->q_next;
				space_allocated--;
				num_modules++;
				strlist.sl_modlist++;
			}
			if (SAMESTR(q)) {
				/* space ran out */
				pl = LOCK(stp->sd_mutex, PLSTR);
				SLEEP_UNLOCK(stp->sd_plumb);
				if (SV_BLKD(stp->sd_timer2)) {
					UNLOCK(stp->sd_mutex, pl);
					SV_BROADCAST(stp->sd_timer2, 0);
				} else {
					UNLOCK(stp->sd_mutex, pl);
				}
				return(ENOSPC);
			}
			error = strcopyout((caddr_t) &num_modules, (caddr_t)arg,
			    sizeof(int), STRINT, copyflag);
			pl = LOCK(stp->sd_mutex, PLSTR);
			SLEEP_UNLOCK(stp->sd_plumb);
			if (SV_BLKD(stp->sd_timer2)) {
				UNLOCK(stp->sd_mutex, pl);
				SV_BROADCAST(stp->sd_timer2, 0);
			} else {
				UNLOCK(stp->sd_mutex, pl);
			}
			return(error);
		}
	    }

	case I_CKBAND:
	    {
		queue_t *q;
		qband_t *qbp;

		q = RD(stp->sd_wrq);
		/*
		 * Ignores MSGNOGET.
		 */
		if ((arg < 0) || (arg >= NBAND)) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		if (arg > (int)q->q_nband) {
			*rvalp = 0;
		} else {
			if (arg == 0) {
				mp = q->q_last;
				if (mp) {
					if ((mp->b_band == 0) && (queclass(mp) == QNORM))
						*rvalp = 1;
					else
						*rvalp = 0;
				}
				else
					*rvalp = 0;
			} else {
				qbp = q->q_bandp;
				while (--arg > 0)
					qbp = qbp->qb_next;
				if (qbp->qb_first)
					*rvalp = 1;
				else
					*rvalp = 0;
			}
		}
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	    }

	case I_GETBAND:
	    {
		int intpri;

		/*
		 * Ignores MSGNOGET.
		 */
		mp = RD(stp->sd_wrq)->q_first;
#ifdef STRPERF
		stamp = castimer();
#endif
		if (!mp) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENODATA);
		}
		intpri = (int)mp->b_band;
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		UNLOCK(stp->sd_mutex, pl);
		error = strcopyout((caddr_t)&intpri, (caddr_t)arg, sizeof(int),
		    STRINT, copyflag);
		return(error);
	    }

	case I_ATMARK:
	    {
		queue_t *q;

		q = RD(stp->sd_wrq);
		if (!arg || (arg & ~(ANYMARK|LASTMARK))) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		mp = q->q_first;
#ifdef STRPERF
		stamp = castimer();
#endif

		/*
		 * Hack for sockets compatibility.  We need to
		 * ignore any messages at the stream head that
		 * are marked MSGNOGET and are not marked MSGMARK.
		 */
		while (mp && ((mp->b_flag & (MSGNOGET|MSGMARK)) == MSGNOGET))
			mp = mp->b_next;

		if (!mp)
			*rvalp = 0;
		else if ((arg & ANYMARK) && (mp->b_flag & MSGMARK))
			*rvalp = 1;
		else if ((arg & LASTMARK) && (mp == stp->sd_mark))
			*rvalp = 1;
		else
			*rvalp = 0;
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	    }

	case I_CANPUT:
	    {
		char band;

		if ((arg < 0) || (arg >= NBAND)) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		band = (char)arg;
		*rvalp = bcanput_l(stp->sd_wrq->q_next, band);
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	    }

	case I_SETCLTIME:
	    {
		long closetime;

		UNLOCK(stp->sd_mutex, pl);
		error = strcopyin((caddr_t)arg, (caddr_t)&closetime,
		    sizeof(long), STRLONG, copyflag);
		if (error)
			return(error);
		if (closetime < 0)
			return(EINVAL);

		/*
		 *  Convert between milliseconds and clock ticks.
		 */
		pl = LOCK(stp->sd_mutex, PLSTR);
		stp->sd_closetime = ((closetime / 1000) * HZ) +
		    ((((closetime % 1000) * HZ) + 999) / 1000);
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	    }

	case I_GETCLTIME:
	    {
		long closetime;

		/*
		 * Convert between clock ticks and milliseconds.
		 */
		closetime = ((stp->sd_closetime / HZ) * 1000) +
		    (((stp->sd_closetime % HZ) * 1000) / HZ);
		UNLOCK(stp->sd_mutex, pl);
		error = strcopyout((caddr_t)&closetime, (caddr_t)arg,
		    sizeof(long), STRLONG, copyflag);
		return(error);
	    }

	case I_BIGPIPE:
	{
		queue_t *qp;

		if (vp->v_type != VFIFO) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		RD(stp->sd_wrq)->q_maxpsz = INFPSZ;
		for (qp = stp->sd_wrq; qp->q_next; qp = qp->q_next)
			;
		qp->q_maxpsz = INFPSZ;
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	}


	case TIOCGSID:
	{
		pid_t id;
		if (stp->sd_sessp == NULL) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENOTTY);
		}
		id = stp->sd_sessp->s_sidp->pid_id;
		UNLOCK(stp->sd_mutex, pl);
		return(strcopyout((caddr_t)&id, (caddr_t) arg,
			sizeof(pid_t), STRPIDT, copyflag));
	}

	case TIOCSPGRP:
	{
		pid_t pgid;
		struct pid *pgidp;
		struct pid *opgidp;

		UNLOCK(stp->sd_mutex, pl);
		if (error = strcopyin((caddr_t) arg, (caddr_t)&pgid, sizeof(pid_t),
		    STRPIDT, copyflag))
			return(error);
		if (pgid <= 0 || pgid >= MAXPID)
			return(EINVAL);

		pl = LOCK(stp->sd_mutex, PLSTR);
loop1:
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
			error = (stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
		switch (error = straccess(stp, access)) {   /* access==JCSETP */
		case CTTY_STOPPED:
			goto loop1;
		case CTTY_EOF:
			UNLOCK(stp->sd_mutex, pl);
			return(0);
		case CTTY_OK:
			break;
		default:
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
		(void) LOCK(&u.u_procp->p_sess_mutex, PL_SESS);

		if (stp->sd_sessp != u.u_procp->p_sessp) {
			UNLOCK(&u.u_procp->p_sess_mutex, PLSTR);
			UNLOCK(stp->sd_mutex, pl);
			return(ENOTTY);
		}
		if (pgid == stp->sd_pgidp->pid_id) {
			UNLOCK(&u.u_procp->p_sess_mutex, PLSTR);
			UNLOCK(stp->sd_mutex, pl);
			return(0);
		}
		(void) LOCK(&stp->sd_sessp->s_mutex, PL_SESS);
		if ((pgidp = pgfind_sess(pgid)) == NULL) {
			UNLOCK(&stp->sd_sessp->s_mutex, PL_SESS);
			UNLOCK(&u.u_procp->p_sess_mutex, PLSTR);
			UNLOCK(stp->sd_mutex, pl);
			return(EPERM);
		}
		UNLOCK(&stp->sd_sessp->s_mutex, PL_SESS);
		opgidp = stp->sd_pgidp;
		stp->sd_pgidp = pgidp;
		pid_hold(stp->sd_pgidp);
		UNLOCK(&u.u_procp->p_sess_mutex, PLSTR);
		UNLOCK(stp->sd_mutex, pl);
		pid_rele(opgidp);
		return(0);
	}

	case TIOCGPGRP:
	{
		pid_t id;
		if (stp->sd_sessp == NULL) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENOTTY);
		}
		id = stp->sd_pgidp->pid_id;
		UNLOCK(stp->sd_mutex, pl);
		return(strcopyout((caddr_t)&id, (caddr_t) arg, sizeof(pid_t),
			STRPIDT, copyflag));
	}

	case FIONBIO:
	case FIOASYNC:
		UNLOCK(stp->sd_mutex, pl);
		return(0);	/* handled by the upper layer */
	}
}

/*
 * int
 * strdoioctl(stdata_t *stp, struct strioctl *strioc, mblk_t *ebp,
 *		int copyflag, char *fmtp, cred_t *crp, int *rvalp)
 *	Send an ioctl message downstream and wait for acknowledgement.
 *
 * Calling/Exit State:
 *	Assumes sd_plumb held (or we're in close).  Assumes sd_mutex
 *	unlocked.
 */

int
strdoioctl(stdata_t *stp, struct strioctl *strioc, mblk_t *ebp, int copyflag,
	char *fmtp, cred_t *crp, int *rvalp)
{
	mblk_t *bp;
	pl_t pl;
	struct iocblk *iocbp;
	struct copyreq *reqp;
	struct copyresp *resp;
	struct striopst *sp;
	mblk_t *fmtbp;
	int id;
	toid_t tid;
	long saveiocid;
	int transparent;
	int error;
	int len;
	int unbind;
	struct engine *engp;
	long ticks;
	caddr_t taddr;
#ifdef STRPERF
	int stamp1;
	int stamp2;
#endif

#ifdef STRPERF
	stamp1 = castimer();
#endif
	transparent = 0;
	error = 0;
	len = 0;
	unbind = 0;
	if (strioc->ic_len == TRANSPARENT) {	/* send arg in M_DATA block */
		transparent = 1;
		strioc->ic_len = sizeof(int);
	}

	if ((strioc->ic_len < 0) ||
	    ((strmsgsz > 0) && (strioc->ic_len > strmsgsz))) {
		if (ebp) {
#ifdef STRPERF
			ebp->b_sh += castimer() - stamp1;
#endif
			freeb(ebp);
		}
		return(EINVAL);
	}

	while (!(bp = allocb(max(sizeof(struct iocblk), sizeof(struct copyreq)), BPRI_HI))) {
		if (error = strwaitbuf(sizeof(struct iocblk), BPRI_HI, stp)) {
			if (ebp) {
#ifdef STRPERF
				ebp->b_sh += castimer() - stamp1;
#endif
				freeb(ebp);
			}
			return(error);
		}
	}
#ifdef STRPERF
	stamp2 = castimer();
#endif
	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_wptr;
	iocbp->ioc_count = strioc->ic_len;
	iocbp->ioc_cmd = strioc->ic_cmd;
	crhold(crp);
	iocbp->ioc_cr = crp;
	iocbp->ioc_error = 0;
	iocbp->ioc_rval = 0;
	bp->b_datap->db_type = M_IOCTL;
	bp->b_wptr += sizeof(struct iocblk);

	/*
	 * If there is data to copy into ioctl block, do so.
	 */
	if (iocbp->ioc_count) {
		if (transparent)
			id = K_TO_K;
		else
			id = copyflag;
		if (error = putiocd(bp, ebp, strioc->ic_dp, id, SE_NOSLP, fmtp, stp)) {
#ifdef STRPERF
			bp->b_sh += castimer() - stamp2;
#endif
			freemsg(bp);
			if (ebp) {
#ifdef STRPERF
				ebp->b_sh += castimer() - stamp1;
#endif
				freeb(ebp);
			}
			crfree(crp);
			return(error);
		}

		/*
		 * We could have slept copying in user pages.
		 * Recheck the stream head state (the other end
		 * of a pipe could have gone away).
		 */
		pl = LOCK(stp->sd_mutex, PLSTR);
		if (stp->sd_flag & (STRHUP|STRDERR|STWRERR|STPLEX)) {
			error = ((stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror));
			UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
			bp->b_sh += castimer() - stamp2;
#endif
			freemsg(bp);
			crfree(crp);
			return(error);
		}
		UNLOCK(stp->sd_mutex, pl);
	} else {
		bp->b_cont = ebp;
	}

	if (transparent)
		iocbp->ioc_count = TRANSPARENT;

	STRKBIND(stp, engp, unbind);

	pl = LOCK(stp->sd_mutex, PLSTR);
	if (stp->sd_iocblk) {
#ifdef STRPERF
		stp->sd_iocblk->b_sh += castimer() - stp->sd_iocblk->b_stamp;
#endif
		freemsg(stp->sd_iocblk);
		stp->sd_iocblk = NULL;
	}
	stp->sd_flag |= IOCWAIT;

	/*
	 * Assign sequence number.
	 */
	FSPIN_LOCK(&id_mutex);
	iocbp->ioc_id = ++ioc_id;
	FSPIN_UNLOCK(&id_mutex);
	stp->sd_iocid = iocbp->ioc_id;
	saveiocid = stp->sd_iocid;

#ifdef STRPERF
	bp->b_sh += castimer() - stamp2;
#endif
	putnext_l(stp->sd_wrq, bp);

	/*
	 * Timed wait for acknowledgment.  The wait time is limited by the
	 * timeout value, which must be a positive integer (number of seconds
	 * to wait, or 0 (use default value of STRTIMOUT seconds), or -1
	 * (wait forever).  This will be awakened either by an ACK/NAK
	 * message arriving, the timer expiring, or the timer expiring
	 * on another ioctl waiting for control of the mechanism.
	 */
waitioc:
	tid = 0;
	if (!(stp->sd_flag & STR3TIME) && strioc->ic_timout >= 0) {
		ticks = (strioc->ic_timout ? strioc->ic_timout : STRTIMOUT) * HZ;
		tid = itimeout(str3time, (caddr_t) stp, ticks, PLSTR);
	}

	stp->sd_flag |= STR3TIME;
	/*
	 * If the reply has already arrived, don't sleep.  If awakened from
	 * the sleep, fail only if the reply has not arrived by then.
	 * Otherwise, process the reply.
	 */
	while (!stp->sd_iocblk) {
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX|STRHUP)) {
			error = ((stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror));
			stp->sd_flag &= ~(STR3TIME|IOCWAIT);
			UNLOCK(stp->sd_mutex, pl);
			if (tid && (strioc->ic_timout >= 0))
				untimeout(tid);
			/* purge any outstanding post processing request */
			sp = findioc(saveiocid);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			crfree(crp);
			STRKUNBIND(engp, unbind);
			return(error);
		}

		if ((tid == 0) && (strioc->ic_timout >= 0)) {
			/* timed wait, but itimeout failed */
			error = EAGAIN;
		} else {
			/* note: SV_WAIT_SIG returns at pl0 with no lock held */
			if (SV_WAIT_SIG(stp->sd_timer3, PRIMED, stp->sd_mutex) == B_TRUE) {
				(void) LOCK(stp->sd_mutex, PLSTR);

				/*
				 * Now we have to figure out what happened.
				 * Could have been a wakeup because the timer
				 * expired, or could have been a wakeup because
				 * the response is here.
				 */
				if (stp->sd_flag & STR3TIME)
					/* response here-back to while loop */
					continue;
				else
					/* timer expired */
					error = ETIME;
			} else {
				(void) LOCK(stp->sd_mutex, PLSTR);
				error = EINTR;
			}
		}

		/*
		 * If we're here, there was an error, which was set above.
		 */
		stp->sd_flag &= ~(STR3TIME|IOCWAIT);
		bp = NULL;
		/*
		 * A message could have come in after we were scheduled
		 * but before we were actually run.
		 */
		if (stp->sd_iocblk) {
			bp = stp->sd_iocblk;
#ifdef STRPERF
			bp->b_sh += castimer() - bp->b_stamp;
			bp->b_stamp = 0;
			stamp1 = castimer();
#endif
			stp->sd_iocblk = NULL;
		}
		UNLOCK(stp->sd_mutex, pl);
		/* purge any outstanding post processing request */
		sp = findioc(saveiocid);
		if (sp)
			kmem_free(sp, sizeof(struct striopst));
		if (bp) {
			if ((bp->b_datap->db_type == M_COPYIN) ||
			    (bp->b_datap->db_type == M_COPYOUT)) {
				if (bp->b_cont) {
#ifdef STRPERF
					bp->b_cont->b_sh += castimer() - stamp1;
#endif
					freemsg(bp->b_cont);
					bp->b_cont = NULL;
				}
				bp->b_datap->db_type = M_IOCDATA;
				/* LINTED pointer alignment */
				resp = (struct copyresp *)bp->b_rptr;
				resp->cp_rval = (caddr_t)1;	/* failure */
#ifdef STRPERF
				bp->b_sh += castimer() - stamp1;
#endif
				putnext(stp->sd_wrq, bp);
			} else {
#ifdef STRPERF
				bp->b_sh += castimer() - stamp1;
#endif
				freemsg(bp);
			}
		}
		if (tid && (strioc->ic_timout >= 0))
			untimeout(tid);
		crfree(crp);
		STRKUNBIND(engp, unbind);
		return(error);
	}
	/* sd_mutex still held at this point */
	ASSERT(stp->sd_iocblk);
	bp = stp->sd_iocblk;
#ifdef STRPERF
	bp->b_sh += castimer() - bp->b_stamp;
	bp->b_stamp = 0;
	stamp1 = castimer();
#endif
	stp->sd_iocblk = NULL;
	if ((bp->b_datap->db_type == M_IOCACK) || (bp->b_datap->db_type == M_IOCNAK)) {
		stp->sd_flag &= ~(STR3TIME|IOCWAIT);
		UNLOCK(stp->sd_mutex, pl);
		if (tid && (strioc->ic_timout >= 0))
			untimeout(tid);
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}


	/*
	 * Have received acknowlegment, sd_mutex no longer held.
	 */

	switch (bp->b_datap->db_type) {
	case M_IOCACK:
		/*
		 * Positive ack.
		 */
		/* LINTED pointer alignment */
		iocbp = (struct iocblk *)bp->b_rptr;

		/*
		 * Set error if indicated.
		 */
		if (iocbp->ioc_error) {
			error = iocbp->ioc_error;
			break;
		}

		/*
		 * Set return value.
		 */
		*rvalp = iocbp->ioc_rval;

		/*
		 * Data may have been returned in ACK message (ioc_count > 0).
		 * If so, copy it out to the user's buffer.
		 */
		if (iocbp->ioc_count && !transparent) {
			if (strioc->ic_cmd == TCGETA ||
			    strioc->ic_cmd == TCGETS ||
			    strioc->ic_cmd == TIOCGETP ||
			    strioc->ic_cmd == LDGETT) {
				if (error = getiocd(bp, strioc->ic_dp,
				    copyflag, fmtp))
					break;
			} else {
				STRACKMAP(stp, strioc, iocbp);
				if (error = getiocd(bp, strioc->ic_dp, copyflag, NULL))
					break;
			}
		}
		if (!transparent) {
			if (len)	/* an M_COPYOUT was used with I_STR */
				strioc->ic_len = len;
			else
				strioc->ic_len = iocbp->ioc_count;
		}

/*
 * If there was a post processing request, now is the time to do it
 */
		sp = findioc(saveiocid);
		if (sp) {
			/*
			 * Got a post-processing request.
			 * First, figure out which driver's context this
			 * is in.  Set l_cdevswp accordingly, so things
			 * like drv_mmap work correctly.
			 */
			u.u_lwpp->l_cdevswp =
			  &cdevsw[getmajor(sp->sio_q->q_str->sd_vnode->v_rdev)];
			error = (*sp->sio_func)(sp->sio_arg, sp->sio_iocid,
						sp->sio_q, rvalp);
			kmem_free(sp, sizeof(struct striopst));
			u.u_lwpp->l_cdevswp = NULL;
		}
		break;

	case M_IOCNAK:
		/*
		 * Negative ack.
		 *
		 * The only thing to do is set error as specified
		 * in neg ack packet.
		 */
		/* LINTED pointer alignment */
		iocbp = (struct iocblk *)bp->b_rptr;

		error = (iocbp->ioc_error ? iocbp->ioc_error : EINVAL);
		break;

	case M_COPYIN:
		/*
		 * Driver or module has requested user ioctl data.
		 */
		/* LINTED pointer alignment */
		reqp = (struct copyreq *)bp->b_rptr;
		fmtbp = bp->b_cont;
		bp->b_cont = NULL;
		if (reqp->cq_flag & RECOPY) {
			/* redo I_STR copyin with canonical processing */
			ASSERT(fmtbp);
			reqp->cq_size = strioc->ic_len;
			error = putiocd(bp, NULL, strioc->ic_dp, copyflag, SE_SLEEP,
			    (fmtbp ? (char *)fmtbp->b_rptr : (char *)NULL), stp);
		} else if (reqp->cq_flag & STRCANON) {
			/* copyin with canonical processing */
			ASSERT(fmtbp);
			error = putiocd(bp, NULL, reqp->cq_addr, copyflag, SE_SLEEP,
			    (fmtbp ? (char *)fmtbp->b_rptr : (char *) NULL), stp);
		} else {
			/* copyin raw data (i.e. no canonical processing) */
			error = putiocd(bp, NULL, reqp->cq_addr, copyflag,
			    SE_SLEEP, (char *)NULL, stp);
		}
		if (fmtbp) {
#ifdef STRPERF
			fmtbp->b_sh += castimer() - stamp1;
#endif
			freeb(fmtbp);
		}
		if (error && bp->b_cont) {
#ifdef STRPERF
			bp->b_cont->b_sh += castimer() - stamp1;
#endif
			freemsg(bp->b_cont);
			bp->b_cont = NULL;
		}

		bp->b_wptr = bp->b_rptr + sizeof(struct copyresp);
		bp->b_datap->db_type = M_IOCDATA;
		/* LINTED pointer alignment */
		resp = (struct copyresp *)bp->b_rptr;
		resp->cp_rval = (caddr_t)error;

		/*
		 * No checks for error necessary since sd_plumb was held
		 * throughout (or we're in close) and the following reply
		 * must be made anyway.
		 */
#ifdef STRPERF
		bp->b_sh += castimer() - stamp1;
#endif
		putnext(stp->sd_wrq, bp);

		if (error) {
			if (tid && (strioc->ic_timout >= 0))
				untimeout(tid);
			crfree(crp);
			/* purge any outstanding post processing request */
			sp = findioc(saveiocid);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			STRKUNBIND(engp, unbind);
			return(error);
		}

		(void) LOCK(stp->sd_mutex, PLSTR);
		goto waitioc;

	case M_COPYOUT:
		/*
		 * Driver or module has ioctl data for a user.
		 */
		/* LINTED pointer alignment */
		reqp = (struct copyreq *)bp->b_rptr;
		ASSERT(bp->b_cont);
		if (transparent) {
			taddr = reqp->cq_addr;
		} else {
			taddr = strioc->ic_dp;
			len = reqp->cq_size;
		}
		if (reqp->cq_flag & STRCANON) {
			/* copyout with canonical processing */
			if ((fmtbp = bp->b_cont) != NULL) {
				bp->b_cont = fmtbp->b_cont;
				fmtbp->b_cont = NULL;
			}
			error = getiocd(bp, taddr, copyflag,
			    (fmtbp ? (char *)fmtbp->b_rptr : (char *)NULL));
			if (fmtbp) {
#ifdef STRPERF
				fmtbp->b_sh += castimer() - stamp1;
#endif
				freeb(fmtbp);
			}
		} else {
			/* copyout raw data (i.e. no canonical processing) */
			error = getiocd(bp, taddr, copyflag, (char *)NULL);
		}
#ifdef STRPERF
		bp->b_cont->b_sh += castimer() - stamp1;
#endif
		freemsg(bp->b_cont);
		bp->b_cont = NULL;

		bp->b_wptr = bp->b_rptr + sizeof(struct copyresp);
		bp->b_datap->db_type = M_IOCDATA;
		/* LINTED pointer alignment */
		resp = (struct copyresp *)bp->b_rptr;
		resp->cp_rval = (caddr_t)error;

#ifdef STRPERF
		bp->b_sh += castimer() - stamp1;
#endif
		putnext(stp->sd_wrq, bp);

		if (error) {
			if (tid && (strioc->ic_timout >= 0))
				untimeout(tid);
			crfree(crp);
			/* purge any outstanding post processing request */
			sp = findioc(saveiocid);
			if (sp)
				kmem_free(sp, sizeof(struct striopst));
			STRKUNBIND(engp, unbind);
			return(error);
		}
		(void) LOCK(stp->sd_mutex, PLSTR);
		goto waitioc;

	default:
#ifndef lint
		ASSERT(0);
#endif
		break;
	}

#ifdef STRPERF
	bp->b_sh += castimer() - stamp1;
#endif
	freemsg(bp);
	crfree(crp);
	/*
	 * purge any outstanding post processing request (this could happen
	 * on any errors above)
	 */
	sp = findioc(saveiocid);
	if (sp)
		kmem_free(sp, sizeof(struct striopst));
	STRKUNBIND(engp, unbind);
	return(error);
}

/*
 * int
 * strrecvfd(stdata_t *stp, cred_t *crp, int copyflag, int cmd, int flag, int arg, pl_t pl)
 *	Handle I_RECVFD family of ioctls
 *
 * Calling/Exit State:
 *	sd_mutex held on entry, and released on return
 */

STATIC int
strrecvfd(stdata_t *stp, cred_t *crp, int copyflag, int cmd, int flag, int arg, pl_t pl)
{
	struct adt_strrecvfd *adtstr;
	struct strrecvfd *srf;
	struct adtrecvfd *arf;
	vattr_t *va;
	int vmode;
	int error;
	int i;
	int done;
	int broadcast;
	int fd;
	struct file *fp;
	mblk_t *mp;
	union {
		struct e_strrecvfd estrfd; /* EFT data structure */
		struct o_strrecvfd ostrfd; /* non-EFT data structure -
					    * SVR3 compatibility mode.
					    */
	} str;
#ifdef STRPERF
	int stamp;
#endif

	done = 0;
	broadcast = 0;
	while (! (mp = getq_l(RD(stp->sd_wrq)))) {
		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENXIO);
		}
		error = strwaitq(stp, GETWAIT, (off_t) 0, flag, &done);
		if (error || done) {
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
		/* recheck errors since strwaitq drops sd_mutex */
		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			return(ENXIO);
		}
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
			error = (stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
	}
#ifdef STRPERF
	stamp = castimer();
#endif
	if (mp->b_datap->db_type == M_TRAIL) {
		stp->sd_flag &= ~STRTOHUP;
		stp->sd_flag |= STRHUP;
		/* Don't miss any wakeups */
		if (SV_BLKD(stp->sd_read))
			broadcast |= RBLOCK;
		if (SV_BLKD(stp->sd_write))
			broadcast |= WBLOCK;
		if (SV_BLKD(stp->sd_timer3))
			broadcast |= IBLOCK;
		UNLOCK(stp->sd_mutex, pl);
		if (broadcast) {
			if (broadcast & RBLOCK)
				SV_BROADCAST(stp->sd_read, 0);
			if (broadcast & WBLOCK)
				SV_BROADCAST(stp->sd_write, 0);
			if (broadcast & IBLOCK)
				SV_BROADCAST(stp->sd_timer3, 0);
		}
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		freemsg(mp);
		/* there shouldn't be any more messages after an M_TRAIL */
		return(ENXIO);
	}

	if (mp->b_datap->db_type != M_PASSFP) {
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		putbq_l(RD(stp->sd_wrq), mp);
		UNLOCK(stp->sd_mutex, pl);
		return(EBADMSG);
	}
	UNLOCK(stp->sd_mutex, pl);
	va = (vattr_t *) kmem_alloc(sizeof(vattr_t), KM_SLEEP);
	/* LINTED pointer alignment */
	adtstr = (struct adt_strrecvfd *) mp->b_rptr;
	srf = (struct strrecvfd *) &adtstr->adt_strrecvfd;
	arf = (struct adtrecvfd *) &adtstr->adt_adtrecvfd;

 	/*
 	 * make sure the process receiving this file
 	 * descriptor has the appropriate domination and/or
 	 * privileges to use this file descriptor.
 	 */
	fp = srf->f.fp;
	/*
	 * if type is VFIFO then check for MAC_WRITE regardless
	 * of flag value.
	 */
	if (fp->f_vnode->v_type == VFIFO)
		vmode = VWRITE;
	else
		vmode = (fp->f_flag & (FWRITE|FTRUNC)) ? VWRITE : VREAD;

	if ((error = MAC_VACCESS(fp->f_vnode, vmode, crp)) != 0) {
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
 		putbq(RD(stp->sd_wrq), mp);
		kmem_free(va, sizeof(vattr_t));
 		return(error);
 	}

	/*
	 * If sender of the file was not its owner, the receiver must
	 * pass DAC on the file to be received.  For compatability,
	 * the check is only performed if MAC is installed.
	 */
	if (mac_installed) {
		/*
		 * Using files open time credentials instead of the
		 * receivers credentials to get the file attributes
		 * because the VOP_GETATTR is on behalf of the kernel
		 * and not the user attempting to receive the file.
		 */
		va->va_mask = AT_STAT;
		if (error = VOP_GETATTR(fp->f_vnode, va, 0, fp->f_cred)) {
#ifdef STRPERF
			mp->b_sh += castimer() - stamp;
#endif
	 		putbq(RD(stp->sd_wrq), mp);
			kmem_free(va, sizeof(vattr_t));
 			return(error);
 		}
		if (srf->uid != va->va_uid) {
			if (error = VOP_ACCESS(fp->f_vnode, vmode, DAC_ACC, crp)) {
#ifdef STRPERF
				mp->b_sh += castimer() - stamp;
#endif
				putbq(RD(stp->sd_wrq), mp);
				kmem_free(va, sizeof(vattr_t));
				return(error);
			}
 		}
	}
	if (error = fdalloc(0, &fd)) {
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		putbq(RD(stp->sd_wrq), mp);
		kmem_free(va, sizeof(vattr_t));
		return(error);
	}
	srf->f.fd = fd;

	if (cmd == I_RECVFD) {
		/* check to see if uid/gid values are too large. */

		if (srf->uid > USHRT_MAX || srf->gid > USHRT_MAX) {
			srf->f.fp = fp;
#ifdef STRPERF
			mp->b_sh += castimer() - stamp;
#endif
			putbq(RD(stp->sd_wrq), mp);
			setf(fd, NULLFP);
			kmem_free(va, sizeof(vattr_t));
			return(EOVERFLOW);
		}
		str.ostrfd.f.fd = srf->f.fd;
		str.ostrfd.uid = (o_uid_t) srf->uid;
		str.ostrfd.gid = (o_gid_t) srf->gid;
		for(i = 0; i < 8; i++)
			str.ostrfd.fill[i] = 0;		/* zero pad */

#ifdef STRPERF
		mp->b_stamp = castimer();
#endif
		if (error = strcopyout((caddr_t)&str.ostrfd,
		    (caddr_t)arg, sizeof(struct o_strrecvfd),
		    O_STRRECVFD, copyflag)) {
			srf->f.fp = fp;
#ifdef STRPERF
			mp->b_copyout += castimer() - mp->b_stamp;
			mp->b_stamp = 0;
			mp->b_sh += castimer() - stamp;
#endif
			putbq(RD(stp->sd_wrq), mp);
			setf(fd, NULLFP);
			kmem_free(va, sizeof(vattr_t));
			return(error);
		}
#ifdef STRPERF
		mp->b_copyout += castimer() - mp->b_stamp;
		mp->b_stamp = 0;
#endif
	} else if (cmd == I_S_RECVFD) {
		struct s_strrecvfd *sstrfd;
		static size_t sstrfdsize;

		sstrfd = (struct s_strrecvfd *) NULL;
		if (sstrfdsize == 0)
			sstrfdsize = sizeof(struct s_strrecvfd) +
			    crgetsize() - sizeof(struct sub_attr);
		if ((sstrfd = (struct s_strrecvfd *)
		    kmem_alloc(sstrfdsize, KM_SLEEP)) == NULL) {
			srf->f.fp = fp;
#ifdef STRPERF
			mp->b_sh += castimer() - stamp;
#endif
			putbq(RD(stp->sd_wrq), mp);
			setf(fd, NULLFP);
			kmem_free(va, sizeof(vattr_t));
			return(ENOMEM);
		}

		sstrfd->fd = srf->f.fd;
		sstrfd->uid = (uid_t)srf->uid;
		sstrfd->gid = (gid_t)srf->gid;
		bcopy((caddr_t)fp->f_cred, (caddr_t)&sstrfd->s_attrs,
		    crgetsize());

#ifdef STRPERF
		mp->b_stamp = castimer();
#endif
		if (error = strcopyout((caddr_t)sstrfd, (caddr_t)arg,
		    sstrfdsize, S_STRRECVFD, copyflag)) {
			srf->f.fp = fp;
#ifdef STRPERF
			mp->b_copyout += castimer() - mp->b_stamp;
			mp->b_stamp = 0;
			mp->b_sh += castimer() - stamp;
#endif
			putbq(RD(stp->sd_wrq), mp);
			setf(fd, NULLFP);
			kmem_free(sstrfd, sstrfdsize);
			kmem_free(va, sizeof(vattr_t));
			return(error);
		}
#ifdef STRPERF
		mp->b_copyout += castimer() - mp->b_stamp;
		mp->b_stamp = 0;
#endif
		kmem_free(sstrfd, sstrfdsize);
	} else {		/* I_E_RECVFD */
		str.estrfd.f.fd = srf->f.fd;
		str.estrfd.uid = (uid_t)srf->uid;
		str.estrfd.gid = (gid_t)srf->gid;
		for(i = 0; i < 8; i++)
			str.estrfd.fill[i] = 0;		/* zero pad */

#ifdef STRPERF
		mp->b_stamp = castimer();
#endif
		if (error = strcopyout((caddr_t)&str.estrfd,
		    (caddr_t)arg, sizeof(struct e_strrecvfd),
		    STRRECVFD, copyflag)) {
			srf->f.fp = fp;
#ifdef STRPERF
			mp->b_copyout += castimer() - mp->b_stamp;
			mp->b_stamp = 0;
			mp->b_sh += castimer() - stamp;
#endif
			putbq(RD(stp->sd_wrq), mp);
			setf(fd, NULLFP);
			kmem_free(va, sizeof(vattr_t));
			return(error);
		}
#ifdef STRPERF
		mp->b_copyout += castimer() - mp->b_stamp;
		mp->b_stamp = 0;
#endif
	}
	/*
	 * Get audit information needed from receiver and dump and
	 * audit record
	 */
	ADT_GET_RECVFD(arf, fp->f_vnode, (mac_installed ? va : NULL));

	mp->b_datap->db_type = M_DATA;
#ifdef STRPERF
	mp->b_sh += castimer() - stamp;
#endif
	freemsg(mp);
	setf(fd, fp);
	kmem_free(va, sizeof(vattr_t));
	return(0);
}

/*
 * int
 * strfdinsert(stdata_t *stp, int arg, int copyflag, int flag)
 *	Handle I_FDINSERT
 *
 * Calling/Exit State:
 *	Assumes sd_mutex not held.
 */

STATIC int
strfdinsert(stdata_t *stp, int arg, int copyflag, int flag)
{
	struct strfdinsert strfdinsert;
	struct file *resftp;
	struct stdata *resstp;
	struct uio uio;
	struct iovec iov;
	queue_t *q;
	mblk_t *mp;
	struct engine *engp;
	int unbind;
	long msgsize;
	long rmin;
	long rmax;
	int done;
	int error;
	int wroff;
	pl_t pl;
#ifdef STRPERF
	int stamp;
#endif

	unbind = 0;
	done = 0;
	error = strcopyin((caddr_t)arg, (caddr_t)&strfdinsert,
	    sizeof(strfdinsert), STRFDINSERT, copyflag);
	if (error)
		return(error);

	if (strfdinsert.offset < 0 ||
	   (strfdinsert.offset % sizeof(queue_t *)) != 0)
		return(EINVAL);
	if (getf(strfdinsert.fildes, &resftp))
		return(EINVAL);
	if (! (resstp = resftp->f_vnode->v_stream)) { 
		error = EINVAL;
		goto fdinsert_exit;
	}

	pl = LOCK(resstp->sd_mutex, PLSTR);
	if (resstp->sd_flag & (STRDERR|STWRERR|STRHUP|STPLEX)) {
		error = (resstp->sd_flag & STPLEX) ? EINVAL :
		    (resstp->sd_werror ? resstp->sd_werror :
		    resstp->sd_rerror);
		UNLOCK(resstp->sd_mutex, pl);
		goto fdinsert_exit;
	}

	/* get read queue of stream terminus */
	for (q = resstp->sd_wrq->q_next; q->q_next; q = q->q_next)
		;
	q = RD(q);
	UNLOCK(resstp->sd_mutex, pl);

	if (strfdinsert.ctlbuf.len < strfdinsert.offset + sizeof(queue_t *)) {
		error = EINVAL;
		goto fdinsert_exit;
	}

	/*
	 * Check for legal flag value.
	 */
	if (strfdinsert.flags & ~RS_HIPRI) {
		error = EINVAL;
		goto fdinsert_exit;
	}

	/*
	 * Make sure ctl and data sizes together fall within
	 * the limits of the max and min receive packet sizes
	 * and do not exceed system limit.  A negative data
	 * length means that no data part is to be sent.
	 */
	pl = LOCK(stp->sd_mutex, PLSTR);
	if (stp->sd_flag & STRHUP) {
		UNLOCK(stp->sd_mutex, pl);
		error = ENXIO;
		goto fdinsert_exit;
	}
	if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
		error = (stp->sd_flag & STPLEX) ? EINVAL :
		    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
		UNLOCK(stp->sd_mutex, pl);
		goto fdinsert_exit;
	}

	rmin = stp->sd_wrq->q_next->q_minpsz;
	rmax = stp->sd_wrq->q_next->q_maxpsz;
	ASSERT((rmax >= 0) || (rmax == INFPSZ));
	if (rmax == 0) {
		UNLOCK(stp->sd_mutex, pl);
		error = ERANGE;
		goto fdinsert_exit;
	}
	if (strmsgsz != 0) {
		if (rmax == INFPSZ)
			rmax = strmsgsz;
		else
			rmax = MIN(strmsgsz, rmax);
	}
	if ((msgsize = strfdinsert.databuf.len) < 0)
		msgsize = 0;
	if ((msgsize < rmin) ||
	    ((msgsize > rmax) && (rmax != INFPSZ)) ||
	    (strfdinsert.ctlbuf.len > strctlsz)) {
		UNLOCK(stp->sd_mutex, pl);
		error = ERANGE;
		goto fdinsert_exit;
	}

	while ((stp->sd_wrq->q_flag & QFREEZE) ||
	        !(strfdinsert.flags & RS_HIPRI) &&
	        !bcanput_l(stp->sd_wrq->q_next, 0)) {
		error = strwaitq(stp, WRITEWAIT, (off_t)0, flag, &done);
		if (error || done) {
			UNLOCK(stp->sd_mutex, pl);
			goto fdinsert_exit;
		}
		/* recheck errors since strwaitq drops sd_mutex */
		if (stp->sd_flag & STRHUP) {
			UNLOCK(stp->sd_mutex, pl);
			error = ENXIO;
			goto fdinsert_exit;
		}
		if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
			error = (stp->sd_flag & STPLEX) ? EINVAL :
			    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
			UNLOCK(stp->sd_mutex, pl);
			goto fdinsert_exit;
		}
	}
	/* take a snapshot */
	wroff = (int) stp->sd_wroff;
	UNLOCK(stp->sd_mutex, pl);

	iov.iov_base = strfdinsert.databuf.buf;
	iov.iov_len = strfdinsert.databuf.len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_segflg = (copyflag == U_TO_K) ? UIO_USERSPACE : UIO_SYSSPACE;
	uio.uio_fmode = 0;
	uio.uio_resid = iov.iov_len;
	error = strmakemsg(&strfdinsert.ctlbuf, strfdinsert.databuf.len,
	    &uio, stp, strfdinsert.flags, &mp, wroff);
	if (error || ! mp)
		goto fdinsert_exit;
#ifdef STRPERF
	stamp = castimer();
#endif

	/*
	 * Place pointer to queue 'offset' bytes from the
	 * start of the control portion of the message.
	 */
	/* LINTED pointer alignment */
	*((queue_t **)(mp->b_rptr + strfdinsert.offset)) = q;

	/*
	 * Put message downstream.
	 */
	pl = LOCK(stp->sd_mutex, PLSTR);
	stp->sd_upbcnt++;
	UNLOCK(stp->sd_mutex, pl);

	STRKBIND(stp, engp, unbind);

#ifdef STRPERF
	mp->b_sh += castimer() - stamp;
#endif
	putnext(stp->sd_wrq, mp);
	pl = LOCK(stp->sd_mutex, PLSTR);
	if ((--stp->sd_upbcnt == 0) && (stp->sd_flag & UPBLOCK)) {
		UNLOCK(stp->sd_mutex, pl);
		SV_BROADCAST(stp->sd_upblock, 0);
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}

	STRKUNBIND(engp, unbind);

fdinsert_exit:
	FTE_RELE(resftp);
	return(error);
}

/*
 * int
 * strpeek(stdata_t *stp, int arg, int copyflag, int *rvalp)
 *	Handle I_PEEK
 *
 * Calling/Exit State:
 *	Assumes sd_mutex not held
 */

STATIC int
strpeek(stdata_t *stp, int arg, int copyflag, int *rvalp)
{
	struct strpeek strpeek;
	mblk_t *bp;
	mblk_t *mp;
	mblk_t *bpsave;
	int n;
	int error;
	struct uio uio;
	struct iovec iov;
	pl_t pl;
#ifdef STRPERF
	int stamp;
#endif

	error = strcopyin((caddr_t)arg, (caddr_t)&strpeek,
	  sizeof(strpeek), STRPEEK, copyflag);
	if (error)
		return(error);

	pl = LOCK(stp->sd_mutex, PLSTR);
	/* recheck errors */
	if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
		error = (stp->sd_flag & STPLEX) ? EINVAL :
		    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}
	mp = RD(stp->sd_wrq)->q_first;
#ifdef STRPERF
	stamp = castimer();
#endif
	while (mp) {
		if (!(mp->b_flag & MSGNOGET))
			break;
		mp = mp->b_next;
	}
	if (!mp || ((strpeek.flags & RS_HIPRI) &&
	    queclass(mp) == QNORM)) {
		*rvalp = 0;
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	}

	if (mp->b_datap->db_type == M_PASSFP) {
		UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		return(EBADMSG);
	}

	if (mp->b_datap->db_type == M_PCPROTO)
		strpeek.flags = RS_HIPRI;
	else
		strpeek.flags = 0;

	if ((bp = dupmsg(mp)) == NULL) {
		UNLOCK(stp->sd_mutex, pl);
#ifdef STRPERF
		mp->b_sh += castimer() - stamp;
#endif
		return(EAGAIN);
	}
#ifdef STRPERF
	stamp = castimer();
#endif
	bpsave = bp;

	/*
	 * At this point, bp has the complete data in it, so we do
	 * not need the stream lock anymore.
	 */
	UNLOCK(stp->sd_mutex, pl);

	/*
	 * First process PROTO blocks, if any.
	 */
	iov.iov_base = strpeek.ctlbuf.buf;
	iov.iov_len = strpeek.ctlbuf.maxlen;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_segflg = (copyflag == U_TO_K) ? UIO_USERSPACE : UIO_SYSSPACE;
	uio.uio_fmode = 0;
	uio.uio_resid = iov.iov_len;
	while (bp && bp->b_datap->db_type != M_DATA && uio.uio_resid >= 0) {
#ifdef STRPERF
		bpsave->b_stamp = castimer();
#endif
		if (((n = MIN(uio.uio_resid, bp->b_wptr - bp->b_rptr)) > 0) &&
		    ((error = uiomove((caddr_t)bp->b_rptr, n, UIO_READ, &uio)) > 0)) {
#ifdef STRPERF
			bpsave->b_copyout += castimer() - bp->b_stamp;
			bpsave->b_stamp = 0;
			bpsave->b_sh += castimer() - stamp;
#endif
			freemsg(bpsave);
			return(error);
		}
#ifdef STRPERF
		bpsave->b_copyout += castimer() - bp->b_stamp;
		bpsave->b_stamp = 0;
#endif
		bp = bp->b_cont;
	}
	strpeek.ctlbuf.len = strpeek.ctlbuf.maxlen - uio.uio_resid;

	/*
	 * Now process DATA blocks, if any.
	 */
	while (bp && bp->b_datap->db_type != M_DATA)
		bp = bp->b_cont;
	iov.iov_base = strpeek.databuf.buf;
	iov.iov_len = strpeek.databuf.maxlen;
	uio.uio_iovcnt = 1;
	uio.uio_resid = iov.iov_len;
	while (bp && uio.uio_resid >= 0) {
#ifdef STRPERF
		bpsave->b_stamp = castimer();
#endif
		if (((n = MIN(uio.uio_resid, bp->b_wptr - bp->b_rptr)) > 0) &&
		    ((error = uiomove((char *)bp->b_rptr, n, UIO_READ, &uio)) > 0)) {
#ifdef STRPERF
			bpsave->b_copyout += castimer() - bpsave->b_stamp;
			bpsave->b_stamp = 0;
			bpsave->b_sh += castimer() - stamp;
#endif
			freemsg(bpsave);
			return(error);
		}
#ifdef STRPERF
		bpsave->b_copyout += castimer() - bpsave->b_stamp;
		bpsave->b_stamp = 0;
#endif
		bp = bp->b_cont;
	}

	strpeek.databuf.len = strpeek.databuf.maxlen - uio.uio_resid;
	error = strcopyout((caddr_t)&strpeek, (caddr_t)arg,
	    sizeof(strpeek), STRPEEK, copyflag);
	if (error) {
#ifdef STRPERF
		bpsave->b_sh += castimer() - stamp;
#endif
		freemsg(bpsave);
		return(error);
	}
	*rvalp = 1;
#ifdef STRPERF
	bpsave->b_sh += castimer() - stamp;
#endif
	freemsg(bpsave);
	return(0);
}


/*
 * int
 * strgetmsg(struct vnode *vp, struct strbuf *mctl, struct strbuf *mdata,
 *	unsigned char *prip, int *flagsp, int fmode, int copyflag, rval_t *rvp)
 *
 *	Get the next message from the read queue.  If the message is
 *	priority, STRPRI will have been set by strrput().  This flag
 *	should be reset only when the entire message at the front of the
 *	queue as been consumed.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 */

int
strgetmsg(struct vnode *vp, struct strbuf *mctl, struct strbuf *mdata,
	unsigned char *prip, int *flagsp, int fmode, int copyflag, rval_t *rvp)
{
	pl_t pl;
	struct stdata *stp;
	mblk_t *bp;
	mblk_t *nbp;
	mblk_t *savemp;
	mblk_t *savemptail;
	int n;
	int bcnt;
	int done;
	int flg;
	int more;
	int error;
	int broadcast;
	char *ubuf;
	int mark;
	unsigned char pri;
	queue_t *q;
#ifdef STRPERF
	int stamp;
#endif

	savemp = NULL;
	savemptail = NULL;
	broadcast = 0;
	done = 0;
	flg = 0;
	more = 0;
	error = 0;
	ASSERT(vp->v_stream);
	stp = vp->v_stream;
	/* stp->sd_wrq is invariant and doesn't need a lock */
	q = RD(stp->sd_wrq);
	rvp->r_val1 = 0;


	switch (*flagsp) {
	case MSG_HIPRI:
		if (*prip != 0)
			return(EINVAL);
		break;

	case MSG_ANY:
	case MSG_BAND:
		break;

	default:
		return(EINVAL);
	}

	pl = LOCK(stp->sd_mutex, PLSTR);
loop:
	if (stp->sd_flag & (STRDERR|STPLEX)) {
		error = (stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror;
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}
	switch (error = straccess(stp, JCREAD)) {
	case CTTY_STOPPED:		/* LWP was stopped */
		goto loop;
	case CTTY_EOF:			/* Return end-of-file */
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	case CTTY_OK:			/* read */
		break;
	default:			/* error */
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}

	mark = 0;
	while (((*flagsp & MSG_HIPRI) && !(stp->sd_flag & STRPRI)) ||
	    ((*flagsp & MSG_BAND) && (!q->q_first ||
	    ((q->q_first->b_band < *prip) && !(stp->sd_flag & STRPRI)))) ||
	    !(bp = getq_l(q))) {
		/*
		 * If STRHUP, return 0 length control and data.
		 */
		if (stp->sd_flag & STRHUP) {
			mctl->len = mdata->len = 0;
			*flagsp = flg;
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
		error = strwaitq(stp, GETWAIT, (off_t)0, fmode, &done);
		if (error || done) {
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
		/* strwaitq drops lock so need to recheck errors */
		if (stp->sd_flag & (STRDERR|STPLEX)) {
			error = (stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror;
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
	}
#ifdef STRPERF
	stamp = castimer();
#endif
	if (bp == stp->sd_mark) {
		mark = 1;
		stp->sd_mark = NULL;
	}

	if (bp->b_datap->db_type == M_PASSFP) {
		if (mark && !stp->sd_mark)
			stp->sd_mark = bp;
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		putbq_l(q, bp);
		UNLOCK(stp->sd_mutex, pl);
		return(EBADMSG);
	}

	if (bp->b_datap->db_type == M_TRAIL) {
		stp->sd_flag &= ~STRTOHUP;
		stp->sd_flag |= STRHUP;
		/* Don't miss any wakeups */
		if (SV_BLKD(stp->sd_read))
			broadcast |= RBLOCK;
		if (SV_BLKD(stp->sd_write))
			broadcast |= WBLOCK;
		if (SV_BLKD(stp->sd_timer3))
			broadcast |= IBLOCK;
		UNLOCK(stp->sd_mutex, pl);
		if (broadcast) {
			if (broadcast & RBLOCK)
				SV_BROADCAST(stp->sd_read, 0);
			if (broadcast & WBLOCK)
				SV_BROADCAST(stp->sd_write, 0);
			if (broadcast & IBLOCK)
				SV_BROADCAST(stp->sd_timer3, 0);
		}
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
		pl = LOCK(stp->sd_mutex, PLSTR);
		goto loop;
	}

	pri = bp->b_band;

	/*
	 * Set HIPRI flag if message is priority.
	 */
	if (stp->sd_flag & STRPRI)
		flg = MSG_HIPRI;
	else
		flg = MSG_BAND;

	/*
	 * First process PROTO or PCPROTO blocks, if any.
	 */
	UNLOCK(stp->sd_mutex, pl);
	if (mctl->maxlen >= 0 && bp && bp->b_datap->db_type != M_DATA) {
		bcnt = mctl->maxlen;
		ubuf = mctl->buf;
		while (bp && bp->b_datap->db_type != M_DATA && bcnt >= 0) {
#ifdef STRPERF
			bp->b_stamp = castimer();
#endif
			if (((n = MIN(bcnt, bp->b_wptr - bp->b_rptr)) > 0) &&
			    strcopyout((caddr_t)bp->b_rptr, ubuf, n, NULL, copyflag)) {
#ifdef STRPERF
				bp->b_copyout += castimer() - bp->b_stamp;
				bp->b_stamp = 0;
#endif
				error = EFAULT;
				pl = LOCK(stp->sd_mutex, PLSTR);
				stp->sd_flag &= ~STRPRI;
				more = 0;
#ifdef STRPERF
				bp->b_sh += castimer() - stamp;
#endif
				freemsg(bp);
				goto getmout;
			}
#ifdef STRPERF
			bp->b_copyout += castimer() - bp->b_stamp;
			bp->b_stamp = 0;
#endif
			ubuf += n;
			bp->b_rptr += n;
			if (bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
#ifdef STRPERF
				nbp->b_sh += castimer() - stamp;
#endif
				freeb(nbp);
			}
			if ((bcnt -= n) <= 0)
				break;
		}
		mctl->len = mctl->maxlen - bcnt;
	} else
		mctl->len = -1;


	if (bp && bp->b_datap->db_type != M_DATA) {
		/*
		 * More PROTO blocks in msg.
		 */
		more |= MORECTL;
		savemp = bp;
		while (bp && bp->b_datap->db_type != M_DATA) {
			savemptail = bp;
			bp = bp->b_cont;
		}
		savemptail->b_cont = NULL;
	}

	/*
	 * Now process DATA blocks, if any.
	 */
	if (mdata->maxlen >= 0 && bp) {
		bcnt = mdata->maxlen;
		ubuf = mdata->buf;
		while (bp && bcnt >= 0) {
#ifdef STRPERF
			bp->b_stamp = castimer();
#endif
			if (((n = MIN(bcnt, bp->b_wptr - bp->b_rptr)) > 0) &&
			    strcopyout((caddr_t)bp->b_rptr, ubuf, n, NULL, copyflag)) {
#ifdef STRPERF
				bp->b_copyout += castimer() - bp->b_stamp;
				bp->b_stamp = 0;
#endif
				error = EFAULT;
				pl = LOCK(stp->sd_mutex, PLSTR);
				stp->sd_flag &= ~STRPRI;
				more = 0;
#ifdef STRPERF
				bp->b_sh += castimer() - stamp;
#endif
				freemsg(bp);
				goto getmout;
			}
#ifdef STRPERF
			bp->b_copyout += castimer() - bp->b_stamp;
			bp->b_stamp = 0;
#endif
			ubuf += n;
			bp->b_rptr += n;
			if (bp->b_rptr >= bp->b_wptr) {
				nbp = bp;
				bp = bp->b_cont;
#ifdef STRPERF
				nbp->b_sh += castimer() - stamp;
#endif
				freeb(nbp);
			}
			if ((bcnt -= n) <= 0)
				break;
		}
		mdata->len = mdata->maxlen - bcnt;
	} else
		mdata->len = -1;

	if (bp) {			/* more data blocks in msg */
		more |= MOREDATA;
		if (savemp)
			savemptail->b_cont = bp;
		else
			savemp = bp;
	}

	pl = LOCK(stp->sd_mutex, PLSTR);
	if (savemp) {
		savemp->b_band = pri;
		if (mark && !stp->sd_mark) {
			savemp->b_flag |= MSGMARK;
			stp->sd_mark = savemp;
		}
#ifdef STRPERF
		savemp->b_sh += castimer() - stamp;
#endif
		putbq_l(q, savemp);
	} else {
		stp->sd_flag &= ~STRPRI;
	}

	*flagsp = flg;
	*prip = pri;

	/*
	 * Getmsg cleanup processing - if the state of the queue has changed
	 * some signals may need to be sent and/or poll awakened.
	 */
getmout:
	/*
	 * sd_mutex locked at this point.  Lock has been dropped so need
	 * to recheck errors
	 */
	if (stp->sd_flag & (STRDERR|STPLEX)) {
		error = (stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror;
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}
	while (((bp = q->q_first) != NULL) && (bp->b_datap->db_type == M_SIG)) {
		bp = getq_l(q);
#ifdef STRPERF
		stamp = castimer();
#endif
		strsignal(stp, *bp->b_rptr, (long)bp->b_band);
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
	}

	/*
	 * If we have just received a high priority message and a
	 * regular message is now at the front of the queue, send
	 * signals in S_INPUT processes and wake up processes polling
	 * on POLLIN.
	 */
	if (((bp = q->q_first) != NULL) && !(stp->sd_flag & STRPRI)) {
#ifdef STRPERF
	    stamp = castimer();
#endif
 	    if (flg & MSG_HIPRI) {
		if (stp->sd_sigflags & S_INPUT)
			strsendsig(stp->sd_siglist, S_INPUT, (long)bp->b_band);
		if ((stp->sd_pollist->ph_events & POLLIN) ||
		    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLIN))) {
			pollwakeup(stp->sd_pollist, POLLIN);
			stp->sd_flag &= ~STRPOLL;
			stp->sd_pevents = 0;
		}

		if (bp->b_band == 0) {
		    if (stp->sd_sigflags & S_RDNORM)
			    strsendsig(stp->sd_siglist, S_RDNORM, 0L);
		    if ((stp->sd_pollist->ph_events & POLLRDNORM) ||
			((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLRDNORM))) {
			    pollwakeup(stp->sd_pollist, POLLRDNORM);
			    stp->sd_flag &= ~STRPOLL;
			    stp->sd_pevents = 0;
		    }
		} else {
		    if (stp->sd_sigflags & S_RDBAND)
			    strsendsig(stp->sd_siglist, S_RDBAND,
				(long)bp->b_band);
		    if ((stp->sd_pollist->ph_events & POLLRDBAND) ||
			((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLRDBAND))) {
			    pollwakeup(stp->sd_pollist, POLLRDBAND);
			    stp->sd_flag &= ~STRPOLL;
			    stp->sd_pevents = 0;
		    }
		}
	    } else {
		if (pri != bp->b_band) {
		    if (bp->b_band == 0) {
			if (stp->sd_sigflags & S_RDNORM)
				strsendsig(stp->sd_siglist, S_RDNORM, 0L);
			if ((stp->sd_pollist->ph_events & POLLRDNORM) ||
			    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLRDNORM))) {
				pollwakeup(stp->sd_pollist, POLLRDNORM);
				stp->sd_flag &= ~STRPOLL;
				stp->sd_pevents = 0;
			}
		    } else {
			if (stp->sd_sigflags & S_RDBAND)
				strsendsig(stp->sd_siglist, S_RDBAND,
				    (long)bp->b_band);
			if ((stp->sd_pollist->ph_events & POLLRDBAND) ||
			    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLRDBAND))) {
				pollwakeup(stp->sd_pollist, POLLRDBAND);
				stp->sd_flag &= ~STRPOLL;
				stp->sd_pevents = 0;
			}
		    }
		}
	    }
#ifdef STRPERF
	    bp->b_sh += castimer() - stamp;
#endif
	}
	UNLOCK(stp->sd_mutex, pl);
	rvp->r_val1 = more;
	return(error);
}

/*
 * int
 * strputmsg(struct vnode *vp, struct strbuf, *mctl, struct strbuf *mdata,
 *	     unsigned char pri, int flag, int fmode, int copyflag, cred_t *crp)
 *	Put a message downstream.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 */

/*ARGSUSED*/
int
strputmsg(struct vnode *vp, struct strbuf *mctl, struct strbuf *mdata,
	unsigned char pri, int flag, int fmode, int copyflag, cred_t *crp)
{
	struct stdata *stp;
	mblk_t *mp;
	pl_t pl;
	long msgsize;
	long rmin;
	long rmax;
	int error;
	int unbind;
	struct engine *engp;
	int done;
	int wroff;
	struct uio uio;
	struct iovec iov;
#ifdef STRPERF
	int stamp;
#endif

	unbind = 0;
	ASSERT(vp->v_stream);
	stp = vp->v_stream;

	/*
	 * Check for legal flag value.
	 */
	switch (flag) {
	case MSG_HIPRI:
		if ((mctl->len < 0) || (pri != 0))
			return(EINVAL);
		break;
	case MSG_BAND:
		break;

	default:
		return(EINVAL);
	}

	pl = LOCK(stp->sd_mutex, PLSTR);
loop:
	if (stp->sd_flag & STPLEX) {
		UNLOCK(stp->sd_mutex, pl);
		return(EINVAL);
	}
	if (stp->sd_flag & (STWRERR|STRHUP)) {
		if (stp->sd_flag & STRSIGPIPE)
			sigtolwp(u.u_lwpp, SIGPIPE, (sigqueue_t *) NULL);
		/*
		 * this is for POSIX compatibility
		 */
		error = (stp->sd_flag & STRHUP) ? EIO : stp->sd_werror;
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}
	switch (error = straccess(stp, JCWRITE)) {
	case CTTY_STOPPED:		/* LWP was stopped */
		goto loop;
	case CTTY_EOF:			/* Return end-of-file */
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	case CTTY_OK:			/* read */
		break;
	default:			/* error */
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}

	/*
	 * firewall - don't use too much memory
	 */
	if (strthresh && (Strcount > strthresh) && 
			pm_denied(CRED(), P_SYSOPS)){
		UNLOCK(stp->sd_mutex, pl);
		return(ENOSR);
	}

	/*
	 * Make sure ctl and data sizes together fall within the
	 * limits of the max and min receive packet sizes and do
	 * not exceed system limit.
	 */
	rmin = stp->sd_wrq->q_next->q_minpsz;
	rmax = stp->sd_wrq->q_next->q_maxpsz;
	ASSERT((rmax >= 0) || (rmax == INFPSZ));
	if (rmax == 0) {
		UNLOCK(stp->sd_mutex, pl);
		return(ERANGE);
	}
	if (strmsgsz != 0) {
		if (rmax == INFPSZ)
			rmax = strmsgsz;
		else
			rmax = MIN(strmsgsz, rmax);
	}
	if ((msgsize = mdata->len) < 0) {
		msgsize = 0;
		rmin = 0;	/* no range check for NULL data part */
	}
	if ((msgsize < rmin) || ((msgsize > rmax) && (rmax != INFPSZ)) ||
	    (mctl->len > strctlsz)) {
		UNLOCK(stp->sd_mutex, pl);
		return(ERANGE);
	}

	while ((stp->sd_wrq->q_flag & QFREEZE) ||
		(!(flag & MSG_HIPRI) && !bcanput_l(stp->sd_wrq->q_next, pri))) {
		error = strwaitq(stp, WRITEWAIT, (off_t) 0, fmode, &done);
		if (error || done) {
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
		/*
		 * strwaitq drops sd_mutex, so need to recheck errors.
		 */
		if (stp->sd_flag & STPLEX) {
			UNLOCK(stp->sd_mutex, pl);
			return(EINVAL);
		}
		if (stp->sd_flag & (STRHUP|STWRERR)) {
			if (stp->sd_flag & STRSIGPIPE)
				sigtolwp(u.u_lwpp, SIGPIPE, (sigqueue_t *)NULL);
			error = stp->sd_werror;
			UNLOCK(stp->sd_mutex, pl);
			return(error);
		}
	}
	wroff = (int) stp->sd_wroff;
	UNLOCK(stp->sd_mutex, pl);

	iov.iov_base = mdata->buf;
	iov.iov_len = mdata->len;
	uio.uio_iov = &iov;
	uio.uio_iovcnt = 1;
	uio.uio_offset = 0;
	uio.uio_segflg = (copyflag == U_TO_K) ? UIO_USERSPACE : UIO_SYSSPACE;
	uio.uio_fmode = 0;
	uio.uio_resid = iov.iov_len;
	error = strmakemsg(mctl, mdata->len, &uio, stp, (long)flag, &mp, wroff);
	if (error || !mp)
		return(error);
#ifdef STRPERF
	stamp = castimer();
#endif
	mp->b_band = pri;

	/*
	 * Put message downstream, sd_wrq is invariant and needs no lock.
	 */
	pl = LOCK(stp->sd_mutex, PLSTR);
	stp->sd_upbcnt++;
	UNLOCK(stp->sd_mutex, pl);

	STRKBIND(stp, engp, unbind);

#ifdef STRPERF
	mp->b_sh += castimer() - stamp;
#endif
	putnext(stp->sd_wrq, mp);
	pl = LOCK(stp->sd_mutex, PLSTR);
	if ((--stp->sd_upbcnt == 0) && (stp->sd_flag & UPBLOCK)) {
		UNLOCK(stp->sd_mutex, pl);
		SV_BROADCAST(stp->sd_upblock, 0);
	} else {
		UNLOCK(stp->sd_mutex, pl);
	}

	STRKUNBIND(engp, unbind);

	return(0);
}

/*
 * int
 * strpoll(stdata_t *stp, short events, int anyyet, short *reventsp,
 *	   struct pollhead **phpp)
 *	Determines whether the necessary conditions are set on a stream
 *	for it to be readable, writeable, or have exceptions.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex unlocked.
 */

int
strpoll(stdata_t *stp, short events, int anyyet, short *reventsp,
	struct pollhead **phpp)
{
	short retevents;
	pl_t pl;
	mblk_t *mp;
	qband_t *qbp;
#ifdef STRPERF
	int stamp;
#endif

	retevents = 0;
	pl = LOCK(stp->sd_mutex, PLSTR);
	if (stp->sd_flag & STPLEX) {
		UNLOCK(stp->sd_mutex, pl);
		*reventsp = POLLNVAL;
		return(0);
	}

	if (stp->sd_flag & STRDERR) {
		if ((events == 0) ||
		    (events & (POLLIN|POLLPRI|POLLRDNORM|POLLRDBAND))) {
			UNLOCK(stp->sd_mutex, pl);
			*reventsp = POLLERR;
			return(0);
		}
	}
	if (stp->sd_flag & STWRERR) {
		if ((events == 0) || (events & (POLLOUT|POLLWRBAND))) {
			UNLOCK(stp->sd_mutex, pl);
			*reventsp = POLLERR;
			return(0);
		}
	}

	if (stp->sd_flag & (STRHUP|STRTOHUP)) {
		retevents |= POLLHUP;
	} else {
		queue_t *tq;

		tq = stp->sd_wrq->q_next;
		while (tq->q_next &&
		       !(tq->q_qinfo->qi_srvp && (tq->q_flag & QPROCSON)))
	    		tq = tq->q_next;
		if (events & POLLWRNORM) {
			if (tq->q_flag & QFULL)
				/* ensure backq svc procedure runs */
				tq->q_flag |= QWANTW;
			else
				retevents |= POLLOUT;
		}
		if (events & POLLWRBAND) {
			qbp = tq->q_bandp;
			if (qbp) {
				while (qbp) {
					if (qbp->qb_flag & QB_FULL)
						qbp->qb_flag |= QB_WANTW;
					else
						retevents |= POLLWRBAND;
					qbp = qbp->qb_next;
				}
			} else {
				retevents |= POLLWRBAND;
			}
		}
	}

#ifdef STRPERF
	stamp = castimer();
#endif
	mp = RD(stp->sd_wrq)->q_first;
	if (!(stp->sd_flag & STRPRI)) {
		while (mp) {
			if ((events & POLLRDNORM) && (mp->b_band == 0))
				retevents |= POLLRDNORM;
			if ((events & POLLRDBAND) && (mp->b_band != 0))
				retevents |= POLLRDBAND;
			if (events & POLLIN)
				retevents |= POLLIN;

			/*
			 * MSGNOGET is really only to make poll return
			 * the intended events when the module is really
			 * holding onto the data.  Yeah, it's a hack and
			 * we need a better solution.
			 */
			if (mp->b_flag & MSGNOGET)
				mp = mp->b_next;
			else
				break;
		}
	} else {
		ASSERT(mp != NULL);
		if (events & POLLPRI)
			retevents |= POLLPRI;
	}
#ifdef STRPERF
	if (mp)
		mp->b_sh += castimer() - stamp;
#endif
	*reventsp = retevents;
	if (retevents) {
		UNLOCK(stp->sd_mutex, pl);
		return(0);
	}

	if (!anyyet) {
		*phpp = stp->sd_pollist;
		/*
		 * This is to eliminate a race.  There is nothing here now,
		 * but before the polldat is stuck on the chain, the stream
		 * head queue could fill up and the poll would never awaken.
		 * This flag is cleared when a pollwakeup is done
		 */
		stp->sd_flag |= STRPOLL;
		stp->sd_pevents = events;
	}
	UNLOCK(stp->sd_mutex, pl);
	return(0);
}

/*
 * int
 * strsink(queue_t *q, mblk_t *bp)
 *	Pipe "sink" routine
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	This is used as the read put procedure on the closed end of a half
 *	closed pipe.  It provides a mechanism for handling any messages that
 *	come down the stream.
 */

STATIC int
strsink(queue_t *q, mblk_t *bp)
{
	struct copyresp *resp;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	switch (bp->b_datap->db_type) {
	case M_FLUSH:
		if ((*bp->b_rptr & FLUSHW) && !(bp->b_flag & MSGNOLOOP)) {
			*bp->b_rptr &= ~FLUSHR;
			bp->b_flag |= MSGNOLOOP;
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			qreply(q, bp);
		} else {
#ifdef STRPERF
			bp->b_sh += castimer() - stamp;
#endif
			freemsg(bp);
		}
		break;

	case M_COPYIN:
	case M_COPYOUT:
		if (bp->b_cont) {
#ifdef STRPERF
			bp->b_cont->b_sh += castimer() - stamp;
#endif
			freemsg(bp->b_cont);
			bp->b_cont = NULL;
		}
		bp->b_datap->db_type = M_IOCDATA;
		/* LINTED pointer alignment */
		resp = (struct copyresp *)bp->b_rptr;
		resp->cp_rval = (caddr_t)1;	/* failure */
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		qreply(q, bp);
		break;

	case M_IOCTL:
		if (bp->b_cont) {
#ifdef STRPERF
			bp->b_cont->b_sh += castimer() - stamp;
#endif
			freemsg(bp->b_cont);
			bp->b_cont = NULL;
		}
		bp->b_datap->db_type = M_IOCNAK;
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		qreply(q, bp);
		break;

	default:
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
		break;
	}
	return(0);
}


/*
 * void
 * strpipeput(queue_t *q, mblk_t *bp)
 *	Stream alternate read put procedure.  This is a highly optimized
 *	version os strrput that is only called for simple pipes (i.e. that
 *	have no modules pushed on them).  Changes to strrput must be
 *	synchronized with this code.  This routine will only get M_DATA
 *	messages in band 0.
 *
 * Calling/Exit State:
 *	Assumes sd_mutex locked.  This routine may not drop sd_mutex, the
 *	operation must be atomic with respect to the stream.
 */

STATIC void
strpipeput(queue_t *q, mblk_t *bp)
{
	struct stdata *stp;
#ifdef STRPERF
	int stamp;

	stamp = castimer();
#endif

	ASSERT(q);
	stp = (struct stdata *)q->q_ptr;
	ASSERT(stp);
	ASSERT(! (stp->sd_flag & STPLEX));

	switch (bp->b_datap->db_type) {

	case M_DATA:
		if (!q->q_first) {
			if (stp->sd_sigflags & S_INPUT)
				strsendsig(stp->sd_siglist, S_INPUT, 0L);
			if ((stp->sd_pollist->ph_events & POLLIN) ||
			    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLIN))) {
				pollwakeup(stp->sd_pollist, POLLIN);
				stp->sd_flag &= ~STRPOLL;
				stp->sd_pevents = 0;
			}
			if (stp->sd_sigflags & S_RDNORM)
				strsendsig(stp->sd_siglist, S_RDNORM, 0L);
			if ((stp->sd_pollist->ph_events & POLLRDNORM) ||
			    ((stp->sd_flag & STRPOLL) && (stp->sd_pevents & POLLRDNORM))) {
				pollwakeup(stp->sd_pollist, POLLRDNORM);
				stp->sd_flag &= ~STRPOLL;
				stp->sd_pevents = 0;
			}
		}

		/*
		 * Wake sleeping read/getmsg
		 */
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		putq_l(q, bp);
#ifdef STRPERF
		stamp = castimer();
#endif
		if (stp->sd_flag & RSLEEP) {
			stp->sd_flag &= ~RSLEEP;
			SV_BROADCAST(stp->sd_read, KS_NOPRMPT); /*don't thrash*/
		}
		return;

	default:
#ifndef lint
		ASSERT(0);
#endif
#ifdef STRPERF
		bp->b_sh += castimer() - stamp;
#endif
		freemsg(bp);
		return;
	}
}

/*
 * int
 * strmodpushed(stdata_t *stp, char *mname)
 *	Find out whether a module is pushed on a given stream
 *
 * Calling/Exit State:
 *	Assumes sd_mutex not held.  Returns 0 if module found, errno if
 *	an error occurred, and -1 if the module is not found on the stream.
 */

int
strmodpushed(stdata_t *stp, char *mname)
{
	queue_t *q;
	int i;
	pl_t pl, swpl;
	int error;

	/*
	 * Find module in fmodsw.
	 */
	if ((i = findmod(mname)) < 0)
		return(EINVAL);

	/* recheck errors, lock has been dropped */
	pl = LOCK(stp->sd_mutex, PLSTR);
	if (stp->sd_flag & (STRDERR|STWRERR|STPLEX)) {
		error = (stp->sd_flag & STPLEX) ? EINVAL :
		    (stp->sd_werror ? stp->sd_werror : stp->sd_rerror);
		UNLOCK(stp->sd_mutex, pl);
		return(error);
	}

	/* Look downstream to see if module is there. */
	swpl = RW_RDLOCK(&mod_fmodsw_lock, PLDLM);
	for (q = stp->sd_wrq->q_next; q &&
	    (fmodsw[i].f_str->st_wrinit != q->q_qinfo); q = q->q_next)
		;
	RW_UNLOCK(&mod_fmodsw_lock, swpl);

	UNLOCK(stp->sd_mutex, pl);
	if (q)
		return(0);
	else
		return(-1);
}
