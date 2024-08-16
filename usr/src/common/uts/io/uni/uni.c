/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/uni/uni.c	1.17"
#ident	"$Header: $"

/*
 * Uniprocessor compatibility module
 *
 * This "module" is used to support non-multithreaded modules/drivers
 * on a stream that is attached to a multiplexor that does not explicitly
 * support non-multithreaded entities.
 *
 * Warning!  This module does many things that are non-DDI compliant.
 * Utilizing techniques used here will guarantee breakage in the future.
 * The interactions are quite subtle - make any changes with extreme care.
 *
 * General note on M_FLUSH handling.  It's a little odd because this is
 * really a "module" and thus, should pass M_FLUSH through.  However, since
 * its purpose is to switch processors when scheduling service procedures,
 * M_FLUSH's are queued after flushing the local queue.  Since this is
 * a priority message, it'll jump to the front of the queue and speed on
 * its way regardless of flow control.  In practice, this should not be
 * an issue since M_FLUSH's by their nature are rather racy.
 *
 * Flow control:  The flow control handling here is tricky.  Once uni
 * transforms itself from a module to a mux, the upper and lower halves
 * are linked by the q_ptr field.  There are 4 q_ptr's available, the upper
 * read and write pair and the lower read and write pair.  The upper write
 * q_ptr points to the lower half write queue and the lower read q_ptr points
 * to the upper half read queue.  These are used for message flow.  When a
 * flow control situation is noticed, the corresponding q_ptr on the other
 * half of the mux is set to 1 to indicate that backenabling is necessary
 * (e.g. the upper write service procedure will set the q_ptr in the lower
 * write queue).  The q_ptr is set and cleared under the protection of
 * sd_mutex, which is used throughout for synchronization.  When a service
 * procedure runs, before returning, it will look at q_ptr to see if a
 * backenable is necessary, and will do it if so.  Note that this may cause
 * some "useless" qenables to be done since flow control might not have
 * been lifted, but the logic is much simpler this way.
 */

#include <util/types.h>
#include <util/param.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/stropts.h>
#include <proc/bind.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <util/debug.h>
#include <util/plocal.h>
#include <util/mod/moddefs.h>

#include <io/ddi.h>

#define MODNAME "uni - UP support module"

MOD_STR_WRAPPER(uni, NULL, NULL, MODNAME);

STATIC struct module_info uni_minfo = {
	UNI_ID, "uni", 0, INFPSZ, 16384, 1024
};

STATIC int uniopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int uniclose(queue_t *, int, cred_t *);
STATIC int uniurput(queue_t *, mblk_t *);
STATIC int uniuwput(queue_t *, mblk_t *);
STATIC int unilrput(queue_t *, mblk_t *);
STATIC int unilwput(queue_t *, mblk_t *);
STATIC int uniursrv(queue_t *);
STATIC int uniuwsrv(queue_t *);
STATIC int unilrsrv(queue_t *);
STATIC int unilwsrv(queue_t *);

extern int nulldev(void);

STATIC struct qinit uni_urinit = {
	uniurput, uniursrv, uniopen, uniclose, NULL, &uni_minfo, NULL
};

STATIC struct qinit uni_uwinit = {
	uniuwput, uniuwsrv, NULL, NULL, NULL, &uni_minfo, NULL
};

STATIC struct qinit uni_lrinit = {
	unilrput, unilrsrv, nulldev, NULL, NULL, &uni_minfo, NULL
};

STATIC struct qinit uni_lwinit = {
	unilwput, unilwsrv, NULL, NULL, NULL, &uni_minfo, NULL
};

struct streamtab uniinfo = {
	&uni_urinit, &uni_uwinit, NULL, NULL
};

int unidevflag = D_MP;


/*
 * int
 * uniopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
 *	Turn self into a multiplexing driver
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Remarks:
 *	The open routine is called when this module is pushed on the
 *	stream.  As a byproduct of the open, the module turns itself
 *	into a multiplexor to break the existing stream into 2 distinct
 *	ones, one of which is MP, the other is bound.
 */

/* ARGSUSED */
STATIC int
uniopen(queue_t *qp, dev_t *devp, int flag, int sflag, cred_t *credp)
{
	pl_t pl;
	queue_t *nqp;

	if (WR(qp)->q_ptr != NULL)
		return(0);

	if (sflag != MODOPEN)
		return(EINVAL);

	/*
	 * sd_plumb should be held at this point since we're in the midst
	 * of a plumbing operation.
	 */
	nqp = allocq();
	pl = freezestr(qp);
	nqp->q_str = qp->q_str;
	WR(nqp)->q_str = qp->q_str;
	setq(nqp, &uni_lrinit, &uni_lwinit);
	WR(nqp)->q_next = WR(qp)->q_next;
	WR(qp)->q_next = NULL;
	OTHERQ(WR(nqp)->q_next)->q_next = nqp;
	nqp->q_flag |= QWANTR;
	WR(nqp)->q_flag |= QWANTR;
	qp->q_ptr = (void *) 0;
	WR(qp)->q_ptr = (void *) WR(nqp);
	nqp->q_ptr = (void *) qp;
	WR(nqp)->q_ptr = (void *) 0;
	/*
	 * Manual qprocson - can't drop sd_mutex otherwise a putnext
	 * would panic on the null q_next.
	 */
	qp->q_putp = qp->q_qinfo->qi_putp;
	WR(qp)->q_putp = WR(qp)->q_qinfo->qi_putp;
	nqp->q_putp = nqp->q_qinfo->qi_putp;
	WR(nqp)->q_putp = WR(nqp)->q_qinfo->qi_putp;
	qp->q_flag |= QPROCSON;
	WR(qp)->q_flag |= QPROCSON;
	nqp->q_flag |= QPROCSON;
	WR(nqp)->q_flag |= QPROCSON;
	if (qp->q_flag & QBOUND) {
		/*
		 * We've been pushed on an already bound stream.
		 * Two cases exist: the lower stream is bound and is about
		 * to end up under a UP-unfriendly mux, or the lower stream
		 * is unbound and is about to end up under a UP mux.  Figure
		 * out which case is true and fix the bindings as appropriate.
		 */
		if (WR(nqp)->q_next->q_flag & QBOUND) {
			/*
			 * lower stream is bound so our bottom half should
			 * be bound and not our top half.
			 */
			qp->q_flag &= ~QBOUND;
			WR(qp)->q_flag &= ~QBOUND;
			nqp->q_flag |= QBOUND;
			WR(nqp)->q_flag |= QBOUND;
		}
	}
	unfreezestr(qp, pl);
	return(0);
}

/*
 * int
 * uniclose(queue_t *qp, int flag, cret_t *credp)
 *	Turn self back into a module so we can be popped correctly.
 *	Because race conditions abound, just flush anything that is
 *	unfortunate enough to be queued right now.  Trying to finish
 *	message processing is just a heuristic anyhow.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	This is a very subtle routine.  Everything is done in a particular
 *	order for very specific reasons.  Heed the comments!
 *
 *	Either the top half or the bottom half might be bound, but never both.
 *	We enter the driver based on the top half so if the bottom half is
 *	bound, e have to switch processors and then start floating again.  If
 *	the top is bound, the bottom can be handled with no switching and we're
 *	already n the correct processor for the top half.
 */

/* ARGSUSED */
STATIC int
uniclose(queue_t *qp, int flag, cred_t *credp)
{
	pl_t pl;
	pl_t pl_1;
	queue_t *wbqp;
	queue_t *rbqp;
	queue_t *wqp;
	int bound;
	struct qsvc *svcp;
	struct engine *engp;
	int unbind;

	unbind = 0;
	wbqp = WR(qp)->q_ptr;

	/*
	 * We use the stream lock to coordinate activity with all of the put
	 * and service procedures that could be running.  Freezing the stream
	 * keeps things relatively quiet.  We clean up the bottom half first
	 * since that's what we have to toss.
	 */
	pl = freezestr(wbqp);
	bound = wbqp->q_flag & QBOUND;
	if (bound) {
		/*
		 * Bottom half was bound, have to get onto the right cpu
		 * so we can dequeue any pending service procedures.
		 */
		unfreezestr(wbqp, pl);
		engp = kbind(wbqp->q_str->sd_cpu);
		block_preemption();
		pl = freezestr(wbqp);
		unbind = 1;
	}
	if (bound) {
		svcp = &l.qsvc;
	} else {
		svcp = &qsvc;
		pl_1 = LOCK(&svc_mutex, plstr);
	}

	/*
	 * This is an in-line qprocsoff.  We can't let sd_mutex go or the
	 * world might come crashing down on us
	 */
	if (wbqp->q_svcflag & QENAB) {
		svc_dequeue(wbqp, svcp);
		wbqp->q_svcflag &= ~QENAB;
	}
	rbqp = RD(wbqp);
	if (rbqp->q_svcflag & QENAB) {
		svc_dequeue(rbqp, svcp);
		rbqp->q_svcflag &= ~QENAB;
	}
	if (!bound)
		UNLOCK(&svc_mutex, pl_1);
	
	/* At this point, no pending service procs */
	freezeprocs(rbqp);

	/*
	 * At this point nothing pending and nothing running on bqp.  The
	 * same is NOT true of qp.  The only problem case is uniuwsrv.  sd_mutex
	 * is used to coordinate activity.  Once we reach this point, that
	 * service proc may be running, but if it is, it is spinning on
	 * sd_mutex.  When it gets released, we'll be done and q_ptr will be
	 * set accordingly.
	 */

	flushq_l(rbqp, FLUSHALL);
	flushq_l(wbqp, FLUSHALL);

	/* This is the indication that a close is in progress */
	wqp = WR(qp);
	qp->q_ptr = NULL;
	wqp->q_ptr = NULL;

	/* Eliminate bottom half */
	wqp->q_next = wbqp->q_next;
	OTHERQ(wbqp->q_next)->q_next = qp;
	freeq(rbqp);

	/*
	 * This handles any future puts - q_ptr being NULL handles those
	 * that are stuck on sd_mutex at the moment.
	 */
	qp->q_putp = putnext;
	wqp->q_putp = putnext;

	/*
	 * Note: for freezing/unfreezing the stream bqp and qp are
	 * interchangable.
	 */
	unfreezestr(qp, pl);
	if (unbind) {
		unblock_preemption();
		/* Get back where we were */
		kunbind(engp);
	}

	/* At this point, we're a normal module again */
	qprocsoff(qp);
	return(0);
}


/*
 * int
 * uniurput(queue_t *qp, mblk_t *mp)
 *	Upper read put procedure
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

int
uniurput(queue_t *qp, mblk_t *mp)
{
	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(qp, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(RD(qp), FLUSHDATA);
		putq(qp, mp);
		break;
	default:
		putq(qp, mp);
		break;
	}
	return(0);
}

/*
 * int
 * uniuwput(queue_t *qp, mblk_t *mp)
 *	Upper write put procedure
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

int
uniuwput(queue_t *qp, mblk_t *mp)
{
	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(qp, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(RD(qp), FLUSHDATA);
		putq(qp, mp);
		break;
	default:
		putq(qp, mp);
		break;
	}
	return(0);
}

/*
 * int
 * unilrput(queue_t *qp, mblk_t *mp)
 *	Lower read put procedure
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

int
unilrput(queue_t *qp, mblk_t *mp)
{
	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR)
			flushq(qp, FLUSHDATA);
		if (*mp->b_rptr & FLUSHW)
			flushq(WR(qp), FLUSHDATA);
		putq(qp, mp);
		break;
	default:
		putq(qp, mp);
		break;
	}
	return(0);
}

/*
 * int
 * unilwput(queue_t *qp, mblk_t *mp)
 *	Lower write put procedure
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

int
unilwput(queue_t *qp, mblk_t *mp)
{
	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR)
			flushq(qp, FLUSHDATA);
		if (*mp->b_rptr & FLUSHW)
			flushq(WR(qp), FLUSHDATA);
		putq(qp, mp);
		break;
	default:
		putq(qp, mp);
		break;
	}
	return(0);
}

/*
 * int
 * uniursrv(queue_t *qp)
 *	Upper read service procedure
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

int
uniursrv(queue_t *qp)
{
	mblk_t *mp;
	pl_t pl;

	while ((mp = getq(qp)) != NULL) {
		if (!pcmsg(mp->b_datap->db_type) && !bcanputnext(qp, 0)) {
			putbq(qp, mp);
			pl = freezestr(qp);
			if (qp->q_ptr) {
				qp->q_ptr = (void *) 0;
				qenable_l(RD(WR(qp)->q_ptr));
			}
			unfreezestr(qp, pl);
			return(0);
		}
		putnext(qp, mp);
	}
	pl = freezestr(qp);
	if (qp->q_ptr) {
		qp->q_ptr = (void *) 0;
		qenable_l(RD(WR(qp)->q_ptr));
	}
	unfreezestr(qp, pl);
	return(0);
}

/*
 * int
 * uniuwsrv(queue_t *qp)
 *	Upper write service procedure
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	This routine uses sd_mutex to coordinate with the close routine.  If
 *	the close starts and we get stuck on sd_mutex, then q_ptr will be
 *	set to NULL, which tells us to bail out.  flushq isn't strictly
 *	necessary since qprocsoff will flush for us (the freemsg is
 *	necessary, or a putbq must be done).
 */

int
uniuwsrv(queue_t *qp)
{
	mblk_t *mp;
	pl_t pl;
	queue_t *dqp;

	pl = freezestr(qp);
	while ((mp = getq_l(qp)) != NULL) {
		dqp = qp->q_ptr;
		if (dqp == NULL) {
			/* we're closing down */
			freemsg(mp);
			flushq_l(qp, FLUSHALL);
			unfreezestr(qp, pl);
			return(0);
		}
		if (!pcmsg(mp->b_datap->db_type) && !bcanput_l((queue_t *)dqp, 0)) {
			putbq_l(qp, mp);
			dqp->q_ptr = (void *) 1;
			unfreezestr(qp, pl);
			return(0);
		}
		putq_l(dqp, mp);
	}
	unfreezestr(qp, pl);
	return(0);
}

/*
 * int
 * unilrsrv(queue_t *qp)
 *	Lower read service procedure
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

int
unilrsrv(queue_t *qp)
{
	mblk_t *mp;
	pl_t pl;

	pl = freezestr(qp);
	while ((mp = getq_l(qp)) != NULL) {
		if (qp->q_ptr == NULL) {
			/* we're closing down */
			freemsg(mp);
			flushq_l(qp, FLUSHALL);
			unfreezestr(qp, pl);
			return(0);
		}
		if (!pcmsg(mp->b_datap->db_type) && !bcanput_l((queue_t *)qp->q_ptr, 0)) {
			putbq_l(qp, mp);
			((queue_t *) qp->q_ptr)->q_ptr = (void *) 1;
			unfreezestr(qp, pl);
			return(0);
		}
		putq_l((queue_t *)qp->q_ptr, mp);
	}
	unfreezestr(qp, pl);
	return(0);
}

/*
 * int
 * unilwsrv(queue_t *qp)
 *	Lower write service procedure
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

int
unilwsrv(queue_t *qp)
{
	mblk_t *mp;
	pl_t pl;

	while ((mp = getq(qp)) != NULL) {
		if (!pcmsg(mp->b_datap->db_type) && !bcanputnext(qp, 0)) {
			putbq(qp, mp);
			pl = freezestr(qp);
			if (qp->q_ptr) {
				qp->q_ptr = (void *) 0;
				qenable_l(WR(RD(qp)->q_ptr));
			}
			unfreezestr(qp, pl);
			return(0);
		}
		putnext(qp, mp);
	}
	pl = freezestr(qp);
	if (qp->q_ptr) {
		qp->q_ptr = (void *) 0;
		qenable_l(WR(RD(qp)->q_ptr));
	}
	unfreezestr(qp, pl);
	return(0);
}
