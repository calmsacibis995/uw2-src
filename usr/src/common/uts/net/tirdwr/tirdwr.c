/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/tirdwr/tirdwr.c	1.7"
#ident	"$Header: $"

/*
 * Transport Interface Library read/write module - issue 1
 */

#include <io/stream.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <mem/kmem.h>
#include <net/tihdr.h>
#include <proc/cred.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define MODNAME "tirdwr - Loadable TLI read/write interface module"

MOD_STR_WRAPPER(trw, NULL, NULL, MODNAME);

struct tirdwr {
	long	 trw_flags;	/* module state; see below */
	queue_t	*trw_rdq;	/* pointer to read queue */
	mblk_t	*trw_mp;	/* pointer to message blocks; used to send */
				/* T_ORDREL_REQ and M_SETOPTS messages */
				/* upon close */
	lock_t	*trw_lock;	/*+ to protect fields of this structure */
};

/*
 * trw_flags values:
 */
#define ORDREL 	002	/* received T_ORDREL_IND */
#define DISCON 	004	/* received T_DISCON_IND */
#define FATAL	010	/* some fatal condition occured */

#define	TIRDWR_ID	4
#define	TIRDWR_HIER	20

STATIC int tirdwropen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int tirdwrclose(queue_t *, int, cred_t *);
STATIC int tirdwrrput(queue_t *, mblk_t *);
STATIC int tirdwrwput(queue_t *, mblk_t *);
static void send_fatal(queue_t *, mblk_t *);
static int check_strhead(queue_t *);
static void strip_strhead(queue_t *);

STATIC struct module_info tirdwr_minfo = {
	TIRDWR_ID, "tirdwr", 0, INFPSZ, 4096, 1024
};

STATIC struct qinit tirdwrrinit = {
	tirdwrrput, NULL, tirdwropen, tirdwrclose, NULL, &tirdwr_minfo, NULL
};

STATIC struct qinit tirdwrwinit = {
	tirdwrwput, NULL, NULL, NULL, NULL, &tirdwr_minfo, NULL
};

struct streamtab trwinfo = {
	&tirdwrrinit, &tirdwrwinit, NULL, NULL
};

int trwdevflag = D_MP;
	/*+ to protect struct tirdwr */
STATIC LKINFO_DECL(tirdwr_lkinfo, "IN:TIRDWR:trw_lock", LK_BASIC);


/*ARGSUSED*/
/*
 * STATIC int
 * tirdwropen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	TIRDWR MODULE OPEN ROUTINE
 *
 * Calling/Exit State:
 * 	 gets called when the module gets pushed onto the stream.
 */
STATIC int
tirdwropen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	struct tirdwr *tp;
	mblk_t *mp;
	struct stroptions *sop;
	pl_t pl;

	ASSERT(q);
	if (q->q_ptr)		/* already open */
		return 0;
	pl = freezestr(q);
	if (! check_strhead(q)) {
		unfreezestr(q, pl);
		return EPROTO;
	}
	if (! (tp = (struct tirdwr *) kmem_zalloc(sizeof(struct tirdwr),
	    KM_NOSLEEP))) {
		unfreezestr(q, pl);
		return ENOSPC;
	}
	tp->trw_rdq = q;
	if (! (mp = allocb(sizeof(struct T_ordrel_req), BPRI_MED))) {
		unfreezestr(q, pl);
		kmem_free(tp, sizeof(struct tirdwr));
		return EAGAIN;
	}
	if (! (tp->trw_mp = allocb(sizeof(struct stroptions), BPRI_MED))) {
		unfreezestr(q, pl);
		freeb(mp);
		kmem_free(tp, sizeof(struct tirdwr));
		return EAGAIN;
	}
	tp->trw_mp->b_next = mp;
	if (! (tp->trw_lock =
	    LOCK_ALLOC(TIRDWR_HIER, plstr, &tirdwr_lkinfo, KM_NOSLEEP))) {
		unfreezestr(q, pl);
		freeb(mp);
		freemsg(tp->trw_mp);
		kmem_free(tp, sizeof(struct tirdwr));
		return ENOSPC;
	}
	/*
	 * set RPROTDIS so that zero-len M_PROTO messages generated
	 * by tirdwr in response to T_ORDREL_IND get past any modules
	 * that may be pushed above us (some modules will discard
	 * zero-len messages)
	 */
	if (! (mp = allocb(sizeof(struct stroptions), BPRI_MED))) {
		unfreezestr(q, pl);
		LOCK_DEALLOC(tp->trw_lock);
		freemsg(tp->trw_mp);
		kmem_free(tp, sizeof(struct tirdwr));
		return EAGAIN;
	}
	strip_strhead(q);
	/*
	 * initialize data structure
	 */
	q->q_ptr = (caddr_t) tp;
	WR(q)->q_ptr = (caddr_t) tp;
	/*
	 * NOTE: since we are in the open routine of a module,
	 * it is guaranteed that q_next of the given queue is
	 * of the Stream Head.
	 */
	(void) strqget(q->q_next, QMAXPSZ, 0, &q->q_maxpsz);
	(void) strqget(WR(q)->q_next, QMAXPSZ, 0, &WR(q)->q_maxpsz);
	unfreezestr(q, pl);

	mp->b_datap->db_type = M_SETOPTS;
	/* LINTED pointer alignment */
	sop = (struct stroptions *) mp->b_rptr;
	sop->so_flags = SO_READOPT;
	sop->so_readopt = RPROTDIS;
	mp->b_wptr += sizeof(struct stroptions);
	putnext(q, mp);

	qprocson(q);
	return 0;
}

/*ARGSUSED*/
/*
 * STATIC int
 * tirdwrclose(queue_t *q, int cflag, cred_t *crp)
 *	TIRDWR MODULE CLOSE ROUTINE
 *
 * Calling/Exit State:
 *	gets called when the module gets popped off of the stream.
 */
STATIC int
tirdwrclose(queue_t *q, int cflag, cred_t *crp)
{
	struct tirdwr *tp;
	mblk_t *mp;
	union T_primitives *pptr;
	struct stroptions *sop;

	ASSERT(q);
	qprocsoff(q);
	tp = (struct tirdwr *) q->q_ptr;
	ASSERT(tp);

	/*
	 * NOTE: It is not required to hold trw_lock here,
	 * since we already have disabled put and service
	 * procedures on the given queue.
	 */
	if ((tp->trw_flags & ORDREL) && ! (tp->trw_flags & FATAL)) {
		mp = tp->trw_mp->b_next;
		tp->trw_mp->b_next = NULL;
		/* LINTED pointer alignment */
		pptr = (union T_primitives *) mp->b_rptr;
		mp->b_wptr = mp->b_rptr + sizeof(struct T_ordrel_req);
		pptr->type = T_ORDREL_REQ;
		mp->b_datap->db_type = M_PROTO;
		putnext(WR(q), mp);
	} else {
		freeb(tp->trw_mp->b_next);
		tp->trw_mp->b_next = NULL;
	}

	mp = tp->trw_mp;
	mp->b_datap->db_type = M_SETOPTS;
	/* LINTED pointer alignment */
	sop = (struct stroptions *) mp->b_rptr;
	sop->so_flags = SO_READOPT;
	sop->so_readopt = RPROTNORM;
	mp->b_wptr += sizeof(struct stroptions);
	putnext(q, mp);

	LOCK_DEALLOC(tp->trw_lock);
	kmem_free(tp, sizeof(struct tirdwr));
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;

	return 0;
}

/*
 * STATIC int
 * tirdwrrput(queue_t *q, mblk_t *mp)
 *	TIRDWR MODULE READ PUT PROCEUDRE
 *
 * Calling/Exit State:
 *	This is called from the module or driver downstream.
 */
STATIC int
tirdwrrput(queue_t *q, mblk_t *mp)
{
	union T_primitives *pptr;
	struct tirdwr *tp;
	mblk_t *tmp;
	pl_t pl;

	ASSERT(q);
	tp = (struct tirdwr *) q->q_ptr;
	ASSERT(tp);

	if (tp->trw_flags & FATAL) {
		freemsg(mp);
		return 0;
	}

	switch (mp->b_datap->db_type) {

	default:
		putnext(q, mp);
		break;

	case M_DATA:
		if (msgdsize(mp) == 0) {
			freemsg(mp);
			break;
		}
		putnext(q, mp);
		break;

	case M_PCPROTO:
	case M_PROTO:
		/*
		 * is there enough data to check type
		 */
		if ((mp->b_wptr - mp->b_rptr) < sizeof(long)) {
			send_fatal(q, mp);
			break;
		}
		/* LINTED pointer alignment */
		pptr = (union T_primitives *) mp->b_rptr;

		switch (pptr->type) {

		case T_EXDATA_IND:
			send_fatal(q, mp);
			break;

		case T_DATA_IND:
			if (msgdsize(mp) == 0) {
				freemsg(mp);
				break;
			}
			tmp = (mblk_t *) unlinkb(mp);
			freemsg(mp);
			putnext(q, tmp);
			break;

		case T_ORDREL_IND:
			/*
			 * hide zero-len message in M_PROTO
			 * so won't be discarded by any modules
			 * that may be pushed above us
			 */
			pl = LOCK(tp->trw_lock, plstr);
			tp->trw_flags |= ORDREL;
			UNLOCK(tp->trw_lock, pl);
			mp->b_datap->db_type = M_PROTO;
			mp->b_wptr = mp->b_rptr;
			if (! (mp->b_cont = allocb(0, 0)))
				/*
				 * send up zero-len M_DATA, then, on the
				 * chance it may get through.
				 */
				mp->b_datap->db_type = M_DATA;
			putnext(q, mp);
			break;

		case T_DISCON_IND:
			pl = LOCK(tp->trw_lock, plstr);
			tp->trw_flags |= DISCON;
			tp->trw_flags &= ~ORDREL;
			UNLOCK(tp->trw_lock, pl);
			if (msgdsize(mp) != 0) {
				tmp = (mblk_t *) unlinkb(mp);
				putnext(q, tmp);
			}
			mp->b_datap->db_type = M_HANGUP;
			mp->b_wptr = mp->b_rptr;
			putnext(q, mp);
			break;

		default:
			send_fatal(q, mp);
			break;
		}
	}
	return 0;
}

/*
 * STATIC int
 * tirdwrwput(queue_t *q, mblk_t *mp)
 *	TIRDWR MODULE WRITE PUT PROCEDURE
 *
 * Calling/Exit State:
 *	This is called from the module or stream head upstream.
 */
STATIC int
tirdwrwput(queue_t *q, mblk_t *mp)
{
	struct tirdwr *tp;

	ASSERT(q);
	tp = (struct tirdwr *) q->q_ptr;
	ASSERT(tp);

	if (tp->trw_flags & FATAL) {
		freemsg(mp);
		return 0;
	}

	switch (mp->b_datap->db_type) {

	default:
		putnext(q, mp);
		break;

	case M_DATA:
		if (msgdsize(mp) == 0) {
			freemsg(mp);
			break;
		}
		putnext(q, mp);
		break;

	case M_PROTO:
	case M_PCPROTO:
		send_fatal(q, mp);
		break;
	}
	return 0;
}

/*
 * static void
 * send_fatal(queue_t *q, mblk_t *mp)
 *	Send a M_ERROR message with EPROTO upstream using given message block.
 *
 * Calling/Exit State:
 * 	Should not be called trw_lock of the queue held.
 */
static void
send_fatal(queue_t *q, mblk_t *mp)
{
	struct tirdwr *tp;
	pl_t pl;

	tp = (struct tirdwr *) q->q_ptr;
	ASSERT(tp);
	pl = LOCK(tp->trw_lock, plstr);
	tp->trw_flags |= FATAL;
	UNLOCK(tp->trw_lock, pl);
	mp->b_datap->db_type = M_ERROR;
	/*
	 * mp->b_datap->db_base always has room
	 */
	*mp->b_datap->db_base = EPROTO;
	mp->b_rptr = mp->b_datap->db_base;
	mp->b_wptr = mp->b_datap->db_base + sizeof(char);
	freemsg(unlinkb(mp));
	putnext(RD(q), mp);
}

/*
 * static int
 * check_strhead(queue_t *q)
 *	An internal routine for tirdwropen() -
 *	it checks messages of the next read queue, assuming that
 *	it is streams head.
 *
 * Calling/Exit State:
 *	Assumes the Streams frozen on entry which remains
 *	frozen on exit.
 *	It returns 1 when all of the messages are either
 *	M_DATA, M_SIG, or M_PROTO/T_DATA_IND followed by
 *	a M_DATA); it returns 0 otherwise.
 */
static int
check_strhead(queue_t *q)
{
	mblk_t *mp;
	union T_primitives *pptr;

	/*
	 * get to the sterams head.
	 * NOTE: a use of q_str like this way does not conform DDI/DKI.
	 */
	q = RD(q->q_str->sd_wrq);

	for (mp = q->q_first; mp; mp = mp->b_next) {

		switch (mp->b_datap->db_type) {

		case M_PROTO:
			if ((mp->b_wptr - mp->b_rptr) < sizeof(long))
				return 0;

			/* LINTED pointer alignment */
			pptr = (union T_primitives *) mp->b_rptr;

			switch (pptr->type) {

			case T_DATA_IND:
				if (mp->b_cont &&
				    (mp->b_cont->b_datap->db_type != M_DATA))
					return 0;
				break;

			case T_EXDATA_IND:
			default:
				return 0;
			}
			break;

		case M_PCPROTO:
			return 0;

		case M_DATA:
		case M_SIG:
			break;

		default:
			return 0;
		}
	}
	return 1;
}

/*
 * static void
 * strip_strhead(queue_t *q)
 *	An internal routine for tirdwropen() - it removes
 *	the first message of the next read queue, assuming
 *	that it is Streams Head.
 * Calling/Exit State:
 *	Assumes the Streams frozen on entry which remains
 *	frozen on exit.
 */
static void
strip_strhead(queue_t *q)
{
	mblk_t *mp;
	mblk_t *emp;
	mblk_t *tmp;
	union T_primitives *pptr;

	/*
	 * get to the streams head
	 * NOTE: a use of q_str like this way does not conform DDI/DKI.
	 */
	q = RD(q->q_str->sd_wrq);

	for (mp = q->q_first; mp; ) {

		switch (mp->b_datap->db_type) {

		case M_PROTO:
			/* LINTED pointer alignment */
			pptr = (union T_primitives *) mp->b_rptr;

			switch (pptr->type) {

			case T_DATA_IND:
				if (msgdsize(mp) == 0)
					goto strip0;
				emp = mp->b_next;
				rmvq(q, mp);
				tmp = (mblk_t *) unlinkb(mp);
				freeb(mp);
				insq(q, emp, tmp);
				mp = emp;
				break;
			}
			break;

		case M_DATA:
			if (msgdsize(mp) == 0) {
strip0:
				tmp = mp->b_next;
				rmvq(q, mp);
				freemsg(mp);
				mp = tmp;
				break;
			}
			mp = mp->b_next;
			break;

		case M_SIG:
			mp = mp->b_next;
			break;
		}
	}
}
