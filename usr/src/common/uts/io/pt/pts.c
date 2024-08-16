/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/pt/pts.c	1.13"
#ident	"$Header: $"

/*
 * PTS - Slave Stream Pseudo Terminal
 */

#include <io/pt/ptms.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/mod/moddefs.h>
#ifdef DBUG
#include <util/cmn_err.h>
STATIC int pts_debug = 0;
#define DBG(a)	 if (pts_debug) cmn_err(CE_CONT, a) 
#else
#define DBG(a)
#endif 

#include <io/ddi.h>

#define DRVNAME "pts - pseudo terminal slave driver"

MOD_DRV_WRAPPER(pts, NULL, NULL, NULL, DRVNAME);

STATIC int ptsopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int ptsclose(queue_t *, int, cred_t *);
STATIC int ptswput(queue_t *, mblk_t *);
STATIC int ptswsrv(queue_t *);
STATIC int ptsrsrv(queue_t *);

STATIC struct module_info pts_info = {
	0xface, "pts", 0, 512, 512, 128
};

STATIC struct qinit ptsrint = {
	NULL, ptsrsrv, ptsopen, ptsclose, NULL, &pts_info, NULL
};

STATIC struct qinit ptswint = {
	ptswput, ptswsrv, NULL, NULL, NULL, &pts_info, NULL
};

struct streamtab ptsinfo = {
	&ptsrint, &ptswint, NULL, NULL
};

int ptsdevflag = D_MP;


/* ARGSUSED */
/*
 * STATIC int 
 * ptsopen(queue_t *q, dev_t devp, int oflag, int sflag, cred_t *crp)
 *	PTS DRIVER OPEN ROUTINE
 *
 * Calling/Exit State:
 *	Nothing special.
 *
 * Description:
 *	Open the master device.  Reject a clone open and do not allow the
 *	driver to be pushed.  If the slave/master pair is locked or if
 *	the slave is not open, return EACCES.  If cannot allocate zero
 *	length data buffer, fail open.
 *	Upon success, store the write queue pointer in private data and
 *	set the PTSOPEN bit in the sflag field.
 */
STATIC int 
ptsopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	struct stroptions *sop;
	struct pt_ttys *ptsp;
	mblk_t *mp;
	mblk_t *mop;
	minor_t min;
	pl_t pl;

	DBG(("pts: entering ptsopen()\n"));

	min = getminor(*devp);
	if (sflag) {
		DBG(("pts: sflag is set\n"));
		return EINVAL;
	}
	if (min >= pt_cnt + pt_sco_cnt) {
		DBG(("pts: invalid minor number\n"));
		return ENXIO;
	}
	ptsp = &ptms_tty[min];
	if (ptsp->pt_state & PTBAD) {
		DBG(("pts: unusable minor number\n"));
		return ENXIO;
	}
	pl = LOCK(ptsp->pt_lock, plstr);
	if ((ptsp->pt_state & PTLOCK) || ! (ptsp->pt_state & PTMOPEN)) {
		UNLOCK(ptsp->pt_lock, pl);
		DBG(("pts: master is locked or slave is closed\n"));
		return EACCES;
	}
	/* 
	 * if already, open simply return
	 */
	if (ptsp->pt_state & PTSOPEN) {
		UNLOCK(ptsp->pt_lock, pl);
		DBG(("pts: master already open\n"));
		return 0;
	}
	/*
	 * set up hi/lo water marks on stream head read queue
	 * and add controlling tty if not set
	 */
	if (! (mop = allocb(sizeof(struct stroptions), 0))) {
		UNLOCK(ptsp->pt_lock, pl);
		DBG(("pts: could not allocb(sizeof(struct stroptions), 0)\n"));
		return EAGAIN;
	}
	if (! (mp = allocb(0, 0))) {
		UNLOCK(ptsp->pt_lock, pl);
		DBG(("pts: could not allocb(0, 0)\n"));
		freemsg(mop);
		return EAGAIN;
	}
	ptsp->pt_bufp = mp;
	ptsp->pts_wrq = WR(q);
	WR(q)->q_ptr = (char *) ptsp;
	q->q_ptr = (char *) ptsp;
	ptsp->pt_state |= PTSOPEN;
	UNLOCK(ptsp->pt_lock, pl);

	mop->b_datap->db_type = M_SETOPTS;
	mop->b_wptr += sizeof(struct stroptions);
	/* LINTED pointer alignment */
	sop = (struct stroptions *) mop->b_rptr;
	sop->so_flags = SO_HIWAT|SO_LOWAT|SO_ISTTY;
	sop->so_hiwat = 512;
	sop->so_lowat = 256;
	putnext(q, mop);
	qprocson(q);
	DBG(("pts: returning from ptsopen()\n"));
	return 0;
}

/* ARGSUSED */
/*
 * STATIC int 
 * ptsclose(queue_t *q, int cflag, cred_t *crp)
 *	PTS DRIVER CLOSE ROUTINE
 *
 * Calling/Exit State:
 *	Nothing special.
 *
 * Description:
 *	Find the address to private data identifying the slave side write 
 *	queue. Send a 0-length msg up the slave side read queue to designate 
 *	the master is closing. Uattach the master from the slave by nulling 
 *	out master side write queue field in private data.
 */
STATIC int 
ptsclose(queue_t *q, int cflag, cred_t *crp)
{
	struct pt_ttys *ptsp;
	queue_t *ptm_rdq;
	mblk_t *pt_bufp;
	pl_t pl;

	DBG(("pts: entering ptsclose()\n"));
	ASSERT(q);
	/*
	 * if no private data
	 */
	if (! q->q_ptr)
		return 0;
	ptsp = (struct pt_ttys *) q->q_ptr;
	ASSERT(ptsp);
	pl = LOCK(ptsp->pt_lock, plstr);
	if ((pt_bufp = ptsp->pt_bufp) != NULL) {
		if (ptsp->ptm_wrq) {
			DBG(("pts: putnext() a zero-length message\n"));
			ptm_rdq = RD(ptsp->ptm_wrq);
			ptsp->pts_active++;
			UNLOCK(ptsp->pt_lock, pl);
			putnext(ptm_rdq, pt_bufp);
			(void) LOCK(ptsp->pt_lock, plstr);
			if (--ptsp->pts_active == 0 && (ptsp->pt_state & PTMCLOSE))
				SV_SIGNAL(ptsp->ptm_sv, 0);
		} else
			freemsg(pt_bufp);
		ptsp->pt_bufp = NULL;
	}
	UNLOCK(ptsp->pt_lock, pl);
	qprocsoff(q);
	pl = LOCK(ptsp->pt_lock, plstr);
	ptsp->pt_state |= PTSCLOSE;
	while (ptsp->ptm_active > 0) {
		SV_WAIT(ptsp->pts_sv, primed, ptsp->pt_lock);
		(void) LOCK(ptsp->pt_lock, plstr);
	}
	ptsp->pts_wrq = NULL;
	ptsp->pt_state &= ~(PTSOPEN|PTSCLOSE);
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	UNLOCK(ptsp->pt_lock, pl);
	DBG(("pts: returning from ptsclose()\n"));
	return 0;
}

/*
 * STATIC int 
 * ptswput(queue_t *q, mblk_t *mp)
 *	PTS DRIVER WRITE PUT PROCEDURE
 *
 * Calling/Exit State:
 *	Nothing special.
 *
 * Description:
 *	only handle flush messages.
 *	All other messages are queued and the write side
 *	service procedure sends them off to the master side.
 */
STATIC int 
ptswput(queue_t *q, mblk_t *mp)
{
	struct pt_ttys *ptsp;
	queue_t *ptm_rdq;
	pl_t pl;

	DBG(("pts: entering ptswput()\n"));
	ASSERT(q && mp);
	ptsp = (struct pt_ttys *) q->q_ptr;
	ASSERT(ptsp);

	switch (mp->b_datap->db_type) {

	case M_FLUSH:
		/*
		 * if write queue request, flush slave side write queue and
		 * send FLUSHR to PTM.
		 * If read request, send FLUSHR to PTM.
		 */
		DBG(("pts: PTS got a flush request\n"));
		pl = LOCK(ptsp->pt_lock, plstr);
		/* don't forward flush if master is gone or is going away */
		if (! ptsp->ptm_wrq || (ptsp->pt_state & PTMOFF)) {
			UNLOCK(ptsp->pt_lock, pl);
			freemsg(mp);
			return 0;
		}
		ptm_rdq = RD(ptsp->ptm_wrq);
		ptsp->pts_active++;
		UNLOCK(ptsp->pt_lock, pl);
		if (*mp->b_rptr & FLUSHW) {
			DBG(("pts: flush PTS write queue\n"));
			flushq(q, FLUSHDATA);
			DBG(("pts: putnextctl1 FLUSHR to PTM\n"));
			putnextctl1(ptm_rdq, M_FLUSH, FLUSHR);
		}
		if (*mp->b_rptr & FLUSHR) {
			mblk_t *bp;

			DBG(("pts: flush PTM write queue\n"));
			flushq(ptsp->ptm_wrq, FLUSHDATA);
			DBG(("pts: putnextctl1 FLUSHW to PTM\n"));
			if (bp = allocb(1, 0)) {
				bp->b_datap->db_type = M_FLUSH;
				*bp->b_wptr++ = FLUSHW;
				putnext(ptm_rdq, bp);
			}
		}
		freemsg(mp);
		pl = LOCK(ptsp->pt_lock, plstr);
		if (--ptsp->pts_active == 0 && (ptsp->pt_state & PTMCLOSE))
			SV_SIGNAL(ptsp->ptm_sv, 0);
		UNLOCK(ptsp->pt_lock, pl);
		break;

	default:
		/*
		 * send other messages to the master
		 */
		DBG(("pts: put the message on slave side write queue\n"));
		if (! ptsp->ptm_wrq) { /* just a hint */
			DBG(("pts: in write side put procedure but no master\n"));
			return 0;
		}
		putq(q, mp);
		break;
	}
	DBG(("pts: return from ptswput()\n"));
	return 0;
}

/*
 * STATIC int 
 * ptswsrv(queue_t *q)
 *	PTS DRIVER WRITE SERVICE PROCEDURE
 *
 * Calling/Exit State:
 *	Nothing special.
 *
 * Description:
 *	If there are messages on this queue that can be sent to 
 *	master, send them via putnext().  Else, if queued messages 
 *	cannot be sent, leave them on this queue.  If priority 
 *	messages on this queue, send them to master no matter what.
 */
STATIC int 
ptswsrv(queue_t *q)
{
	struct pt_ttys *ptsp;
	queue_t *ptm_rdq;
	mblk_t *mp;
	pl_t pl;
	
	DBG(("pts: entering ptswsrv()\n"));
	ASSERT(q);
	ptsp = (struct pt_ttys *) q->q_ptr;
	ASSERT(ptsp);
	pl = LOCK(ptsp->pt_lock, plstr);
	if (! ptsp->ptm_wrq) {
		UNLOCK(ptsp->pt_lock, pl);
		DBG(("pts: in write side service procedure but no master\n"));
		return 0;
	}
	ptm_rdq = RD(ptsp->ptm_wrq);
	ptsp->pts_active++;
	UNLOCK(ptsp->pt_lock, pl);
	/*
	 * while there are messages on this write queue...
	 */
	while (mp = getq(q)) {
		/*
		 * if do not have control message and cannot put
		 * message on master side read queue, put it back on
		 * this queue.
		 */
		if (! pcmsg(mp->b_datap->db_type) && ! canputnext(ptm_rdq)) {
			DBG(("pts: put message back on the queue\n"));
			putbq(q, mp);
			break;
		}
		/*
		 * else send the message up master side stream
		 */
		DBG(("pts: send the message to master\n"));
		putnext(ptm_rdq, mp);
	}
	pl = LOCK(ptsp->pt_lock, plstr);
	if (--ptsp->pts_active == 0 && (ptsp->pt_state & PTMCLOSE))
		SV_SIGNAL(ptsp->ptm_sv, 0);
	UNLOCK(ptsp->pt_lock, pl);
	DBG(("pts: leaving ptswsrv()\n"));
	return 0;
}

/*
 * STATIC int 
 * ptsrsrv(queue_t *q)
 *	PTS DRIVER READ SERVICE PROCEDURE
 *
 * Calling/Exit State:
 *	Nothing special.
 *
 * Description:
 *	enable the write side of the master.  This triggers the 
 *	master to send any messages queued on its write side to
 *	the read side of this slave.
 */
STATIC int 
ptsrsrv(queue_t *q)
{
	struct pt_ttys *ptsp;
	pl_t pl;
	
	DBG(("pts: entering ptsrsrv()\n"));
	ASSERT(q);
	ptsp = (struct pt_ttys *) q->q_ptr;
	ASSERT(ptsp);
	pl = LOCK(ptsp->pt_lock, plstr);
	if (! ptsp->ptm_wrq) {
		UNLOCK(ptsp->pt_lock, pl);
		DBG(("pts: in read side service procedure but no master\n"));
		return 0;
	}
	qenable(ptsp->ptm_wrq);
	UNLOCK(ptsp->pt_lock, pl);
	DBG(("pts: leaving ptsrsrv()\n"));
	return 0;
}
