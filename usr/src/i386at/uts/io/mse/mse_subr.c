/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/mse/mse_subr.c	1.4"
#ident	"$Header: $"


#include <util/param.h>
#include <util/types.h>
#include <proc/signal.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <io/termio.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <util/debug.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <io/ws/chan.h>
#include <io/mouse.h>
#include <io/mse/mse.h>
#include <util/cmn_err.h>
#include <io/ddi.h>


#ifdef DEBUG
STATIC int mse_subr_debug = 0;
#define DEBUG1(a)	if (mse_subr_debug == 1) printf a
#define DEBUG2(a)       if (mse_subr_debug >= 2) printf a /* allocations */
#else
#define DEBUG1(a)
#define DEBUG2(a)
#endif /* DEBUG */



/*
 * void
 * mse_iocack(queue_t *, mblk_t *, struct iocblk *, int)
 *
 * Calling/Exit State:
 *	None.
 */
void
mse_iocack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int rval)
{
	mp->b_datap->db_type = M_IOCACK;
	iocp->ioc_rval = rval;
	iocp->ioc_count = iocp->ioc_error = 0;
	qreply(qp, mp);
}


/* 
 * void
 * mse_iocnack(queue_t *, mblk_t *, struct iocblk *, int, int)
 *
 * Calling/Exit State:
 *	None.
 */
void
mse_iocnack(queue_t *qp, mblk_t *mp, struct iocblk *iocp, int error, int rval)
{
	mp->b_datap->db_type = M_IOCNAK;
	iocp->ioc_rval = rval;
	iocp->ioc_error = error;
	qreply(qp, mp);
}


/*
 * void
 * mse_copyout(queue_t *, register mblk_t *, register mblk_t *, uint,
 *		unsigned long)
 *
 * Calling/Exit State:
 *	None.
 */
void
mse_copyout(queue_t *qp, register mblk_t *mp, register mblk_t *nmp, 
		uint size, unsigned long state)
{
	register struct copyreq *cqp;
	struct strmseinfo *cp;
	struct msecopy	*copyp;


	DEBUG1(("In mse_copyout\n"));

	cp = (struct strmseinfo *)qp->q_ptr;
	copyp = &cp->copystate;
	copyp->state = state;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) * (long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
	mp->b_datap->db_type = M_COPYOUT;

	if (mp->b_cont) 
		freemsg(mp->b_cont);
	mp->b_cont = nmp;

	qreply(qp, mp);

	DEBUG1(("leaving mse_copyout\n"));
}


/*
 * void
 * mse_copyin(queue_t *, register mblk_t *, int, unsigned long)
 *
 * Calling/Exit State:
 *	None.
 */
void
mse_copyin(queue_t *qp, register mblk_t *mp, int size, unsigned long state)
{
	register struct copyreq *cqp;
	struct msecopy *copyp;
	struct strmseinfo *cp;


	DEBUG1(("In mse_copyin\n"));

	cp = (struct strmseinfo *) qp->q_ptr;
	copyp = &cp->copystate;

	copyp->state = state;
	/* LINTED pointer alignment */
	cqp = (struct copyreq *)mp->b_rptr;
	cqp->cq_size = size;
	/* LINTED pointer alignment */
	cqp->cq_addr = (caddr_t) * (long *)mp->b_cont->b_rptr;
	cqp->cq_flag = 0;
	cqp->cq_private = (mblk_t *)copyp;

	if (mp->b_cont) {
		freemsg(mp->b_cont);
		mp->b_cont = NULL;
	}

	mp->b_datap->db_type = M_COPYIN;
	mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);

	qreply(qp, mp);

	DEBUG1(("leaving mse_copyin\n"));
}


/*
 * void
 * mseproc(struct strmseinfo *)
 *
 * Calling/Exit State:
 *	None.
 */
void
mseproc(struct strmseinfo *qp)
{
	register mblk_t 	*bp;
	register mblk_t 	*mp;
	struct ch_protocol	*protop;
	struct mse_event 	*minfo;


	DEBUG1(("In mseproc\n"));

	/* If no change, don't load a new event */
	if (qp->x | qp->y)
		qp->type = MSE_MOTION;
	else if (qp->button != qp->old_buttons)
		qp->type = MSE_BUTTON;
	else
		return;

	qp->mseinfo.status = (~qp->button & 7) | 
				((qp->button ^ qp->old_buttons) << 3) | 
				 (qp->mseinfo.status & BUTCHNGMASK) | 
				 (qp->mseinfo.status & MOVEMENT);

	if (qp->type == MSE_MOTION) {
		register int sum;

		qp->mseinfo.status |= MOVEMENT;

		/*
		 * See sys/mouse.h for UPPERLIM = 127 and LOWERLIM = -128
		 */

		sum = qp->mseinfo.xmotion + qp->x;

		if (sum > UPPERLIM)
			qp->mseinfo.xmotion = UPPERLIM;
		else if (sum < LOWERLIM)
			qp->mseinfo.xmotion = LOWERLIM;
		else
			qp->mseinfo.xmotion = (char)sum;

		sum = qp->mseinfo.ymotion + qp->y;

		if (sum > UPPERLIM)
			qp->mseinfo.ymotion = UPPERLIM;
		else if (sum < LOWERLIM)
			qp->mseinfo.ymotion = LOWERLIM;
		else
			qp->mseinfo.ymotion = (char)sum;
	}

	/* Note the button state */
	qp->old_buttons = qp->button;
	if ((bp = allocb(sizeof(struct ch_protocol), BPRI_MED)) == NULL) { 
		return;
	}
	bp->b_datap->db_type = M_PROTO;
	bp->b_wptr += sizeof(struct ch_protocol);
	/* LINTED pointer alignment */
	protop = (struct ch_protocol *) bp->b_rptr;
	protop->chp_type = CH_DATA;
	protop->chp_stype = CH_MSE;
	drv_getparm(LBOLT, (clock_t *)&protop->chp_tstmp);
	if ((mp = allocb(sizeof(struct mse_event), BPRI_MED)) == NULL) { 
		freemsg(bp);
		return;
	}
	bp->b_cont = mp;
	minfo = (struct mse_event *)mp->b_rptr;
	minfo->type = qp->type;	
	minfo->code = qp->button;	
	minfo->x = qp->x;	
	minfo->y = qp->y;	
	mp->b_wptr += sizeof(struct mse_event);
	putnext(qp->rqp, bp);

	DEBUG1(("leaving mseproc\n"));
}
