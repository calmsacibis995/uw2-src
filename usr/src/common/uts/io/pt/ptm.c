/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/pt/ptm.c	1.14"
#ident	"$Header: $"

/*
 * PTM - Master Stream Pseudo Terminal
 */

#include <io/pt/ptms.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#ifdef DBUG
#include <util/cmn_err.h>
STATIC int ptm_debug = 0;
#define DBG(a)	 if (ptm_debug) cmn_err(CE_CONT, a) 
#else
#define DBG(a)
#endif 

#include <io/ddi.h>

STATIC int ptm_load(void);
STATIC int ptm_unload(void);

#define DRVNAME "ptm - pseudo terminal master driver"

MOD_DRV_WRAPPER(ptm, ptm_load, ptm_unload, NULL, DRVNAME);

void ptminit(void);
STATIC int ptmopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int ptmclose(queue_t *, int, cred_t *);
STATIC int ptmwput(queue_t *, mblk_t *);
STATIC int ptmwsrv(queue_t *);
STATIC int ptmrsrv(queue_t *);

STATIC struct module_info ptm_info = {
	0xdead, "ptm", 0, 512, 512, 128
};

STATIC struct qinit ptmrint = {
	NULL, ptmrsrv, ptmopen, ptmclose, NULL, &ptm_info, NULL
};

STATIC struct qinit ptmwint = {
	ptmwput, ptmwsrv, NULL, NULL, NULL, &ptm_info, NULL
};

struct streamtab ptminfo = {
	&ptmrint, &ptmwint, NULL, NULL
};

int ptmdevflag = D_MP;

#define	PT_HIER	1
	/*+ to protect pt_ttys structure */
STATIC LKINFO_DECL(pt_lkinfo, "ID:PT:*ptms_tty[].pt_lock", LK_BASIC);


/*
 * int
 * ptm_load(void)
 *	Load routine
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

STATIC int
ptm_load(void)
{
	ptminit();
	return(0);
}

/*
 * int
 * ptm_unload(void)
 *	Unload routine
 *
 * Calling/Exit State:
 *	No locking assumptions
 */

STATIC int
ptm_unload(void)
{
	int i;
	struct pt_ttys *ptmp;

	for (i = 0, ptmp = &ptms_tty[0]; i < pt_cnt + pt_sco_cnt; i++, ptmp++) {
		if (!(ptmp->pt_state & PTBAD)) {
			LOCK_DEALLOC(ptmp->pt_lock);
			SV_DEALLOC(ptmp->ptm_sv);
			SV_DEALLOC(ptmp->pts_sv);
		}
	}
	return(0);
}


/*
 * void
 * ptminit(void)
 *	PTM DRIVER INITIALIZATION ROUTINE
 *
 * Calling/Exit State:
 *	Nothing special.
 */
void
ptminit(void)
{
	int i;
	struct pt_ttys *ptmp;

#ifdef DBUG
	cmn_err(CE_CONT, "ptm: ptminit() entering, %d slots\n", pt_cnt+pt_sco_cnt);
#endif
	for (i = 0, ptmp = &ptms_tty[0]; i < (pt_cnt + pt_sco_cnt); i++, ptmp++) {
		ptmp->pt_lock =
		    LOCK_ALLOC(PT_HIER, plstr, &pt_lkinfo, KM_NOSLEEP);
		if (ptmp->pt_lock == NULL) {
			ptmp->pt_state |= PTBAD;
			continue;
		}
		ptmp->ptm_sv = SV_ALLOC(KM_NOSLEEP);
		if (ptmp->ptm_sv == NULL) {
			LOCK_DEALLOC(ptmp->pt_lock);
			ptmp->pt_state |= PTBAD;
			continue;
		}
		ptmp->pts_sv = SV_ALLOC(KM_NOSLEEP);
		if (ptmp->pts_sv == NULL) {
			LOCK_DEALLOC(ptmp->pt_lock);
			SV_DEALLOC(ptmp->ptm_sv);
			ptmp->pt_state |= PTBAD;
			continue;
		}
	}
#ifdef DBUG
	cmn_err(CE_CONT, "ptm: ptminit() leaving\n");
#endif
}

/* ARGSUSED */
/*
 * STATIC int 
 * ptmopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	PTM DRIVER OPEN ROUTINE
 *
 * Calling/Exit State:
 *	Nothing special.
 *
 * Description:
 *	Find an unused entry in the ptms_tty array.
 *	Store the write queue pointer and set the pt_state field to
 *	(PTMOPEN|PTLOCK).
 */
STATIC int 
ptmopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	struct stroptions *sop;
	struct pt_ttys *ptmp;
	mblk_t *mop;
	minor_t min;
	queue_t *pts_rdq;
	pl_t pl;

	DBG(("ptm: entering ptmopen()\n"));

	if (sflag != CLONEOPEN) {	/* SCO_COMPAT: */
		min = geteminor(*devp);
		if (min < pt_cnt) {
		    DBG(("ptm: direct open not allowed on non-SCO pseudo ttys\n"));
		    return ENXIO;
		}
		if (min >= (pt_cnt + pt_sco_cnt)) {
		    DBG(("ptm: no more SCO devices left to allocate\n"));
		    return ENXIO;
		}
		ptmp = &ptms_tty[min];
		if (! (ptmp->pt_state & (PTMOPEN|PTSOPEN|PTLOCK|PTBAD))) {
		    pl = LOCK(ptmp->pt_lock, plstr);
		    if (ptmp->pt_state & (PTMOPEN|PTSOPEN|PTLOCK)) {
			    UNLOCK(ptmp->pt_lock, pl);
			    DBG(("ptm: device already open\n"));
			    return EIO;
		    }
		} else {
			DBG(("ptm: device already open\n"));
			return EIO;
		}
					/* END SCO_COMPAT: */
	} else {
	    for (min = 0, ptmp = &ptms_tty[0]; min < pt_cnt; min++, ptmp++) {
		if (! (ptmp->pt_state & (PTMOPEN|PTSOPEN|PTLOCK|PTBAD))) {
			pl = LOCK(ptmp->pt_lock, plstr);
			if (! (ptmp->pt_state & (PTMOPEN|PTSOPEN|PTLOCK)))
				break;
			else
				UNLOCK(ptmp->pt_lock, pl);
		}
	    }
	    if (min >= pt_cnt) {
		DBG(("ptm: no more devices left to allocate\n"));
		return ENXIO;
	    }
	}
	/* NOTE: ptmp->pt_lock held */
	if (ptmp->pts_wrq) {
		pts_rdq = RD(ptmp->pts_wrq);
		ptmp->ptm_active++;
		UNLOCK(ptmp->pt_lock, pl);
		if (pts_rdq) {
			DBG(("ptm: send hangup to an already existing slave\n"));
			putnextctl(pts_rdq, M_HANGUP);
		}
		(void) LOCK(ptmp->pt_lock, plstr);
		if (--ptmp->ptm_active == 0 && (ptmp->pt_state & PTSCLOSE))
			SV_SIGNAL(ptmp->pts_sv, 0);
	}
	/* NOTE: ptmp->pt_lock still held */
	/*
	 * set up hi/lo water marks on stream head read queue
	 * and add controlling tty if not set
	 */
	if (! (mop = allocb(sizeof(struct stroptions), 0))) {
		UNLOCK(ptmp->pt_lock, pl);
		return EAGAIN;
	}
	/*
	 * set up the entries in the pt_ttys structure for this device.
	 * Note: if we get here, PTBAD wasn't set
	 */
	if (sflag == CLONEOPEN)
		ptmp->pt_state = (PTMOPEN|PTLOCK);
	/*
	 * SCO: So that slave pty can be opened without having to
	 * grantpt and unlockpt. 
	 */
	else
		ptmp->pt_state = PTMOPEN;
	ptmp->ptm_wrq = WR(q);
	ptmp->pts_wrq = NULL;
	ptmp->pt_bufp = NULL;
	q->q_ptr = (char *) ptmp;
	WR(q)->q_ptr = (char *) ptmp;
	UNLOCK(ptmp->pt_lock, pl);

	mop->b_datap->db_type = M_SETOPTS;
	mop->b_wptr += sizeof(struct stroptions);
	/* LINTED pointer alignment */
	sop = (struct stroptions *) mop->b_rptr;
	sop->so_flags = SO_HIWAT|SO_LOWAT|SO_ISTTY;
	sop->so_hiwat = 512;
	sop->so_lowat = 256;
	putnext(q, mop);

	if (sflag == CLONEOPEN)
	    *devp = makedevice(getemajor(*devp), min);
	qprocson(q);
	DBG(("ptm: returning from ptmopen()\n"));
	return 0;
}

/* ARGSUSED */
/*
 * STATIC int 
 * ptmclose(queue_t *q, int cflag, cred_t *crp)
 *	PTM DRIVER CLOSE ROUTINE
 *
 * Calling/Exit State:
 *	Nothing special.
 *
 * Description:
 *	Find the address to private data identifying the slave's write queue.
 *	Send a hang-up message up the slave's read queue to designate the
 *	master/slave pair is tearing down. Uattach the master and slave by 
 *	nulling out the write queue fields in the private data structure.  
 *	Finally, unlock the master/slave pair and mark the master as closed.
 */
STATIC int 
ptmclose(queue_t *q, int cflag, cred_t *crp)
{
	struct pt_ttys *ptmp;
	queue_t *pts_rdq;
	pl_t pl;

	DBG(("ptm: entering ptmclose()\n"));
	ASSERT(q);
	ptmp = (struct pt_ttys *) q->q_ptr;
	ASSERT(ptmp);

	pl = LOCK(ptmp->pt_lock, plstr);
	ptmp->pt_state |= PTMOFF;
	if (ptmp->pts_wrq) {
		pts_rdq = RD(ptmp->pts_wrq);
		ptmp->ptm_active++;
		UNLOCK(ptmp->pt_lock, pl);
		if (pts_rdq) {
			DBG(("ptm: send hangup message to slave\n"));
			putnextctl(pts_rdq, M_HANGUP);
		}
		pl = LOCK(ptmp->pt_lock, plstr);
		if (--ptmp->ptm_active == 0 && (ptmp->pt_state & PTSCLOSE))
			SV_SIGNAL(ptmp->pts_sv, 0);
	}
	freemsg(ptmp->pt_bufp);
	ptmp->pt_bufp = NULL;
	UNLOCK(ptmp->pt_lock, pl);
	qprocsoff(q);
	pl = LOCK(ptmp->pt_lock, plstr);
	ptmp->pt_state |= PTMCLOSE;
	while (ptmp->pts_active > 0) {
		SV_WAIT(ptmp->ptm_sv, primed, ptmp->pt_lock);
		(void) LOCK(ptmp->pt_lock, plstr);
	}
	ptmp->ptm_wrq = NULL;
	ptmp->pt_state &= ~(PTMOPEN|PTLOCK|PTMCLOSE|PTMOFF);
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	UNLOCK(ptmp->pt_lock, pl);
	DBG(("ptm: returning from ptmclose()\n"));
	return 0;
}

/*
 * STATIC int 
 * ptmwput(queue_t *q, mblk_t *mp)
 *	PTM DRIVER WRITE PUT PROCEDURE
 *
 * Calling/Exit State:
 *	Nothing special.
 *
 * Description:
 *	will only handle ioctl and flush messages.
 */
STATIC int 
ptmwput(queue_t *q, mblk_t *mp)
{
	struct pt_ttys *ptmp;
	queue_t *pts_rdq;
	struct iocblk *iocp;
	pl_t pl;
	
	DBG(("ptm: entering ptmwput()\n"));
	ASSERT(q && mp);
	ptmp = (struct pt_ttys *) q->q_ptr;
	ASSERT(ptmp);
		
	switch (mp->b_datap->db_type) {
	/*
	 * if write queue request, flush master's write
	 * queue and send FLUSHR up slave side. If read 
	 * queue request, convert to FLUSHW and putctl1().
	 */
	case M_FLUSH:
		DBG(("ptm: PTM got a flush request\n"));
		if (! (*mp->b_rptr & FLUSHW) && ! (*mp->b_rptr & FLUSHR)) {
			freemsg(mp);
			break;
		}
		pl = LOCK(ptmp->pt_lock, plstr);
		if ((ptmp->pt_state & PTLOCK) || ! ptmp->pts_wrq) {
			UNLOCK(ptmp->pt_lock, pl);
			freemsg(mp);
			break;
		}
		pts_rdq = RD(ptmp->pts_wrq);
		ptmp->ptm_active++;
		UNLOCK(ptmp->pt_lock, pl);
		if (*mp->b_rptr & FLUSHW) {
			DBG(("ptm: flush PTM write queue\n"));
			flushq(q, FLUSHDATA);
			DBG(("ptm: putnextctl1 FLUSHR to PTS\n"));
			putnextctl1(pts_rdq, M_FLUSH, FLUSHR);
		}
		if (*mp->b_rptr & FLUSHR) {
			mblk_t *bp;

			DBG(("ptm: flush PTS write queue\n"));
			flushq(ptmp->pts_wrq, FLUSHDATA);
			DBG(("ptm: got read, putnextctl1 FLUSHW\n"));
			if (bp = allocb(1, 0)) {
				bp->b_datap->db_type = M_FLUSH;
				*bp->b_wptr++ = FLUSHW;
				putnext(pts_rdq, bp);
			}
		}
		pl = LOCK(ptmp->pt_lock, plstr);
		if (--ptmp->ptm_active == 0 && (ptmp->pt_state & PTSCLOSE))
			SV_SIGNAL(ptmp->pts_sv, 0);
		UNLOCK(ptmp->pt_lock, pl);
		freemsg(mp);
		break;

	case M_IOCTL:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;

		switch (iocp->ioc_cmd) {

		default:
			pl = LOCK(ptmp->pt_lock, plstr);
			if ((ptmp->pt_state & PTLOCK) || ! ptmp->pts_wrq) {
				UNLOCK(ptmp->pt_lock, pl);
				DBG(("ptm: got M_IOCTL but no slave\n"));
				mp->b_datap->db_type = M_IOCNAK;
				freemsg(mp->b_cont);
				mp->b_cont = NULL;
				qreply(q, mp);
				return 0;
			}
			UNLOCK(ptmp->pt_lock, pl);
			putq(q, mp);
			break;

		case ISPTM:
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			DBG(("ptm: ack the ISPTM\n"));
			qreply(q, mp);
			break;

		case UNLKPT:
			pl = LOCK(ptmp->pt_lock, plstr);
			ptmp->pt_state &= ~PTLOCK;
			UNLOCK(ptmp->pt_lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			DBG(("ptm: ack the UNLKPT\n"));
			qreply(q, mp);
			break;
		}
		break;
		
	default:
		/*
		 * send other messages to slave
		 */
		pl = LOCK(ptmp->pt_lock, plstr);
		if ((ptmp->pt_state & PTLOCK) || ! ptmp->pts_wrq) {
			UNLOCK(ptmp->pt_lock, pl);
			DBG(("ptm: got message but no slave\n"));
			putnextctl1(RD(q), M_ERROR, EINVAL);
			freemsg(mp);
			return 0;
		}
		UNLOCK(ptmp->pt_lock, pl);
		DBG(("ptm: put message on master side write queue\n"));
		putq(q, mp);
		break;
	}
	DBG(("ptm: return from ptmwput()\n"));
	return 0;
}

/*
 * STATIC int 
 * ptmwsrv(queue_t *q)
 *	PTM DRIVER WRITE SERVICE PROCEDURE
 *
 * Calling/Exit State:
 *	If there are messages on this queue that can be sent to 
 *	slave, send them via putnext(). Else, if queued messages 
 *	cannot be sent, leave them on this queue. If priority 
 *	messages on this queue, send them to slave no matter what.
 */
STATIC int 
ptmwsrv(queue_t *q)
{
	struct pt_ttys *ptmp;
	queue_t *pts_rdq;
	mblk_t *mp;
	pl_t pl;
	
	DBG(("ptm: entering ptmwsrv()\n"));
	ASSERT(q);
	ptmp = (struct pt_ttys *) q->q_ptr;
	ASSERT(ptmp);

	pl = LOCK(ptmp->pt_lock, plstr);
	if (ptmp->pts_wrq)
		pts_rdq = RD(ptmp->pts_wrq);
	else
		pts_rdq = NULL;
		
	if ((ptmp->pt_state & PTLOCK) || ! pts_rdq) {
		UNLOCK(ptmp->pt_lock, pl);
		DBG(("ptm: in master side write service procedure but no slave\n"));
		putnextctl1(RD(q), M_ERROR, EINVAL);
		return 0;
	}
	ptmp->ptm_active++;
	UNLOCK(ptmp->pt_lock, pl);
	/*
	 * while there are messages on this write queue...
	 */
	while (mp = getq(q)) {
		/*
		 * if do not have control message and cannot put
		 * msg. on slave's read queue, put it back on 
		 * this queue.
		 */
		if (! pcmsg(mp->b_datap->db_type) &&
		    ! canputnext(pts_rdq)) {
			DBG(("ptm: put message back on queue\n"));
			putbq(q, mp);
			break;
		}
		/*
		 * else send the message up slave's stream
		 */
		DBG(("ptm: send message to slave\n"));
		putnext(pts_rdq, mp);
	}
	pl = LOCK(ptmp->pt_lock, plstr);
	if (--ptmp->ptm_active == 0 && (ptmp->pt_state & PTSCLOSE))
		SV_SIGNAL(ptmp->pts_sv, 0);
	UNLOCK(ptmp->pt_lock, pl);
	DBG(("ptm: leaving ptmwsrv()\n"));
	return 0;
}

/*
 * STATIC int 
 * ptmrsrv(queue_t *q)
 *	PTM DRIVER READ SERVICE PROCEDURE
 *
 * Calling/Exit State:
 *	enable the write side of the slave. This triggers the 
 *	slave to send any messages queued on its write side to
 *	the read side of this master.
 */
STATIC int 
ptmrsrv(queue_t *q)
{
	struct pt_ttys *ptmp;
	pl_t pl;
	
	DBG(("ptm: entering ptmrsrv()\n"));
	ASSERT(q);
	ptmp = (struct pt_ttys *) q->q_ptr;
	ASSERT(ptmp);

	pl = LOCK(ptmp->pt_lock, plstr);
	if (ptmp->pts_wrq)
		qenable(ptmp->pts_wrq);
	UNLOCK(ptmp->pt_lock, pl);
	DBG(("ptm: leaving ptmrsrv()\n"));
	return 0;
}
