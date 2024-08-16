/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ntty/ntty.c	1.7"
#ident	"$Header: $"

#include <io/conf.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/termio.h>
#include <io/ttold.h>
#include <net/tihdr.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/param.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <mem/kmem.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define MODNAME "ntty - Loadable tty emulation module"

MOD_STR_WRAPPER(nt, NULL, NULL, MODNAME);

#define	EX_BREAK_HI	0x00
#define EX_BREAK_LO	0x01
#define	EX_BREAK_SIZE	2

#ifndef O_HUPCL
#define O_HUPCL 01
#endif

struct ntty {
	lock_t	 *lock;		/* lock to protect data structure */
	event_t	 *event;	/* event for waiting for T_INFO_ACK */
	mblk_t	 *mp;		/* pointer to preallocated message block */
	mblk_t	 *ioc_mp;	/* saved reply for stty 0 -disconnect request */
	tcflag_t cflags;	/* copy of cflags to keep setty happy */
	int	 state;		/* state of ntty device */
	tcflag_t brkflag;	/* break flags in iflags */
};

/* state flags */
#define WACK 		0x1
#define W_INFO_ACK	0x2
#define	EX_DATA_SUP	0x4

#define NTTY_ID	4444
#define NTTYHIER 1


/* stream data structure definintons */
STATIC int ntopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int ntclose(queue_t *, int, cred_t *);
STATIC int ntrput(queue_t *, mblk_t *);
STATIC int ntwput(queue_t *, mblk_t *);

STATIC struct module_info nt_info = {NTTY_ID, "ntty", 0, INFPSZ, 4096, 1024};

STATIC struct qinit ntrinit = {
	ntrput, NULL, ntopen, ntclose, NULL, &nt_info, NULL
};

STATIC struct qinit ntwinit = {
	ntwput, NULL, ntopen, ntclose, NULL, &nt_info, NULL
};

struct streamtab ntinfo = { &ntrinit, &ntwinit, NULL, NULL };

int ntdevflag = D_MP;

/*
 *+ lock is a per module instantiation spin lock that protects the state
 *+ information
 */
LKINFO_DECL(ntty_lkinfo, "NTTY::lock", 0);

/*
 * int
 * ntopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	Open routine gets called when the module gets pushed onto the stream.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

/*ARGSUSED*/
ntopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	struct ntty *ntp;
	mblk_t *mop;
	struct T_info_req *t_prim;
	struct stroptions *sop;

	if (q->q_ptr)
		return(0);

	if (sflag != MODOPEN)
		return(EINVAL);

	ntp = (struct ntty *) kmem_zalloc(sizeof(struct ntty), KM_NOSLEEP);
	if (ntp == NULL)
		return(ENOMEM);

	ntp->lock = LOCK_ALLOC(NTTYHIER, plstr, &ntty_lkinfo, KM_NOSLEEP);
	if (ntp->lock == NULL) {
		kmem_free(ntp, sizeof(struct ntty));
		return(ENOMEM);
	}

	ntp->event = EVENT_ALLOC(KM_NOSLEEP);
	if (ntp->event == NULL) {
		LOCK_DEALLOC(ntp->lock);
		kmem_free(ntp, sizeof(struct ntty));
		return(ENOMEM);
	}

	/*
	 * allocate a message block, used to generate disconnect
	 * for "stty 0"
	 */
	if ((ntp->mp = allocb(sizeof(struct T_discon_req), BPRI_HI)) == NULL) {
		LOCK_DEALLOC(ntp->lock);
		EVENT_DEALLOC(ntp->event);
		kmem_free(ntp, sizeof(struct ntty));
		return(ENOMEM);
	}

	/*
	 * set up hi/lo water marks on stream head read queue
	 */
	if (mop = allocb(sizeof(struct stroptions), BPRI_HI)) {
		mop->b_datap->db_type = M_SETOPTS;
		mop->b_wptr += sizeof(struct stroptions);
		/* LINTED pointer alignment */
		sop = (struct stroptions *) mop->b_rptr;
		sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
		sop->so_hiwat = 512;
		sop->so_lowat = 256;
		putnext(q, mop);
	} else {
		freemsg(ntp->mp);
		LOCK_DEALLOC(ntp->lock);
		EVENT_DEALLOC(ntp->event);
		kmem_free(ntp, sizeof(struct ntty));
		return(ENOMEM);
	}

	q->q_ptr = (void *) ntp;
	WR(q)->q_ptr = (void *) ntp;
	ntp->cflags = (tcflag_t) (B300|CS8|CREAD|HUPCL);

	/* Find out if expedited data is used by transport */

	if ((mop = allocb(sizeof(struct T_info_req), BPRI_MED)) == NULL) {
		freemsg(ntp->mp);
		LOCK_DEALLOC(ntp->lock);
		EVENT_DEALLOC(ntp->event);
		kmem_free(ntp, sizeof(struct ntty));
		return(ENOMEM);
	}

	/*
	 * Send a T_info_req to the transport to find out if expedited
	 * data is supported
	 */

	ntp->state |= W_INFO_ACK;
	mop->b_datap->db_type = M_PCPROTO;
	mop->b_wptr = mop->b_rptr + sizeof(struct T_info_req);
	/* LINTED pointer alignment */
	t_prim = (struct T_info_req *) mop->b_rptr;
	t_prim->PRIM_type = T_INFO_REQ;
	qprocson(q);
	putnext(WR(q), mop);

	if (EVENT_WAIT_SIG(ntp->event, primed) == B_FALSE) {
		freemsg(ntp->mp);
		LOCK_DEALLOC(ntp->lock);
		EVENT_DEALLOC(ntp->event);
		kmem_free(ntp, sizeof(struct ntty));
		return(EINTR);
	}

	return(0);
}


/*
 * int
 * ntclose(queue_t *q, int flag, cred_t *crp)
 *	This rountne gets called when the module gets popped off of the stream.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

/*ARGSUSED*/
ntclose(queue_t *q, int flag, cred_t *crp)
{
	struct ntty *ntp;

	ntp = (struct ntty *) q->q_ptr;

	qprocsoff(q);

	/* Try our best */
	(void) putnextctl1(q, M_PCSIG, SIGHUP);

	q->q_ptr = (void *) NULL;
	WR(q)->q_ptr = (void *) NULL;
	if (ntp->mp)
		freemsg(ntp->mp);
	if (ntp->ioc_mp)
		freemsg(ntp->ioc_mp);
	EVENT_DEALLOC(ntp->event);
	LOCK_DEALLOC(ntp->lock);
	kmem_free(ntp, sizeof(struct ntty));
	return(0);
}


/*
 * int
 * ntrput(queue_t *q, mblk_t *mp)
 *	Module read queue put procedure.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit
 */

int
ntrput(queue_t *q, mblk_t *mp)
{
	union T_primitives *pptr;
	mblk_t *ctlmp;
	struct ntty *ntp;
	pl_t pl;

	ntp = (struct ntty *) q->q_ptr;

	switch(mp->b_datap->db_type) {
	case M_DATA:
		pl = LOCK(ntp->lock, plstr);
		if (!(ntp->cflags & CREAD)) {
			UNLOCK(ntp->lock, pl);
			freemsg(mp);
			return(0);
		}
		UNLOCK(ntp->lock, pl);
		break;
	case M_DELAY:
		freemsg(mp);
		return(0);
		
	case M_PCPROTO:
	case M_PROTO:
		while (mp->b_wptr == mp->b_rptr) {
			if (mp->b_cont == NULL) {
				freeb(mp);
				return(0);
			}

			ctlmp = mp->b_cont;
			mp->b_cont = NULL;
			freeb(mp);
			mp = ctlmp;
			ctlmp = NULL;
		}

		/* LINTED pointer alignment */
		pptr = (union T_primitives *) mp->b_rptr;
		switch (pptr->type) {
		default:
			putnext(q, mp);
			return(0);

		case T_DATA_IND:
			pl = LOCK(ntp->lock, plstr);
			if (!(ntp->cflags & CREAD)) {
				UNLOCK(ntp->lock, pl);
				freemsg(mp);
				return(0);
			}
			UNLOCK(ntp->lock, pl);
			break;

		case T_INFO_ACK:
			pl = LOCK(ntp->lock, plstr);
			if (ntp->state & W_INFO_ACK) {
				ntp->state &= ~W_INFO_ACK;
				if (pptr->info_ack.ETSDU_size <= 0)
					ntp->state &= ~EX_DATA_SUP;
				else
					ntp->state |= EX_DATA_SUP;
				EVENT_SIGNAL(ntp->event, 0);
				UNLOCK(ntp->lock, pl);
				freemsg(mp);
				return(0);
			}
			UNLOCK(ntp->lock, pl);
			break;

		case T_ERROR_ACK:
		case T_OK_ACK:
			pl = LOCK(ntp->lock, plstr);
			if (ntp->state & WACK) {
				ntp->state &= ~WACK;
				freemsg(mp);
				if (ntp->ioc_mp) {
					mp = ntp->ioc_mp;
					ntp->ioc_mp = NULL;
					UNLOCK(ntp->lock, pl);
					putnext(q, mp);
				} else
					UNLOCK(ntp->lock, pl);
				/* Try our best */
				(void) putnextctl(q, M_HANGUP);
				return(0);
			}
			UNLOCK(ntp->lock, pl);
			break;

		case T_EXDATA_IND:
			ctlmp = mp->b_cont;
			if (*ctlmp->b_rptr == EX_BREAK_HI &&
			    *(ctlmp->b_rptr + 1) == EX_BREAK_LO) {
				pl = LOCK(ntp->lock, plstr);
				if ((!(ntp->brkflag & IGNBRK)) &&
				     (ntp->brkflag & BRKINT)) {
					UNLOCK(ntp->lock, pl);
					mp->b_cont = NULL;
					ctlmp->b_wptr = ctlmp->b_rptr = ctlmp->b_datap->db_base;
					*ctlmp->b_wptr++ = SIGINT;
					ctlmp->b_datap->db_type = M_PCSIG;
					putnext(q, ctlmp);
				} else
					UNLOCK(ntp->lock, pl);

				freemsg(mp);
				return(0);
			} else
				break;
		}
		break;
	default:
		break;
	}
	putnext(q, mp);
	return(0);
}


/*
 * int
 * ntwput(queue_t *q, mblk_t *mp)
 *	Module write queue put procedure.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */

int
ntwput(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocp;
	struct termio *cb;
	struct termios *termiosp;
	struct sgttyb *gb;
	struct ntty *ntp;
	struct T_discon_req *tp;
	struct T_exdata_req *ep;
	mblk_t *ctlmp;
	pl_t pl;

	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;
	ntp = (struct ntty *) q->q_ptr;

	switch (mp->b_datap->db_type) {
	case M_DATA:
		putnext(q, mp);
		break;

	case M_IOCTL:
		switch (iocp->ioc_cmd) {
		case TCSETA:
		case TCSETAF:
		case TCSETAW:
			if (mp->b_cont == NULL) {
				freemsg(mp);
				return(0);
			}
			/* LINTED pointer alignment */
			cb = (struct termio *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->lock, plstr);
			ntp->cflags = cb->c_cflag;
			ntp->brkflag = cb->c_iflag & (BRKINT|IGNBRK);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;

			if ((cb->c_cflag & CBAUD) == B0) {
				/* hang-up: generate a disconnect request */
				ntp->ioc_mp = mp;
				mp = ntp->mp;
				if (mp) {
					ntp->mp = NULL;	/* use only once */
					ntp->state |= WACK;
					UNLOCK(ntp->lock, pl);
					/* LINTED pointer alignment */
					tp = (struct T_discon_req *) mp->b_rptr;
					mp->b_wptr = mp->b_rptr + sizeof(struct T_discon_req);
					tp->PRIM_type = T_DISCON_REQ;
					tp->SEQ_number = -1;
					mp->b_datap->db_type = M_PROTO;
					putnext(q, mp);
				} else
					UNLOCK(ntp->lock, pl);
				return(0);
			}
			UNLOCK(ntp->lock, pl);
			qreply(q, mp);
			return(0);

		case TCSETS:
		case TCSETSF:
		case TCSETSW:
			if (mp->b_cont == NULL) {
				freemsg(mp);
				return(0);
			}
			/* LINTED pointer alignment */
			termiosp = (struct termios *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->lock, plstr);
			ntp->cflags = termiosp->c_cflag;
			ntp->brkflag = termiosp->c_iflag & (BRKINT|IGNBRK);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;

			if ((termiosp->c_cflag & CBAUD) == B0) {
				/* hang-up: generate a disconnect request */
				ntp->ioc_mp = mp;
				mp = ntp->mp;
				if (mp) {
					ntp->mp = NULL; /* use only once */
					ntp->state |= WACK;
					UNLOCK(ntp->lock, pl);
					/* LINTED pointer alignment */
					tp = (struct T_discon_req *) mp->b_rptr;
					mp->b_wptr = mp->b_rptr + sizeof(struct T_discon_req);
					tp->PRIM_type = T_DISCON_REQ;
					tp->SEQ_number = -1;
					mp->b_datap->db_type = M_PROTO;
					putnext(q, mp);
				} else
					UNLOCK(ntp->lock, pl);
				return(0);
			}
			UNLOCK(ntp->lock, pl);
			qreply(q, mp);
			return(0);

		case TCGETA:
			mp->b_cont = allocb(sizeof(struct termio), BPRI_HI);
			if (mp->b_cont == NULL) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = ENOSR;
				iocp->ioc_count = 0;
				qreply(q, mp);
				return(0);
			}
			bzero((caddr_t) mp->b_cont->b_rptr, sizeof(struct termio));
			mp->b_cont->b_wptr = mp->b_cont->b_rptr + sizeof(struct termio);
			/* LINTED pointer alignment */
			cb = (struct termio *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->lock, plstr);
			cb->c_cflag = ntp->cflags;
			UNLOCK(ntp->lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = sizeof(struct termio);
			qreply(q, mp);
			return(0);

		case TCGETS:
			if (mp->b_cont)
				freemsg(mp->b_cont);

			mp->b_cont = allocb(sizeof(struct termios), BPRI_MED);
			if (mp->b_cont == NULL) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EAGAIN;
				iocp->ioc_count = 0;
				qreply( q, mp);
				break;
			}

			bzero((caddr_t) mp->b_cont->b_rptr, sizeof(struct termios));
			mp->b_cont->b_wptr =  mp->b_cont->b_rptr + sizeof(struct termios);
			/* LINTED pointer alignment */
			termiosp = (struct termios *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->lock, plstr);
			termiosp->c_cflag = ntp->cflags;
			UNLOCK(ntp->lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = sizeof(struct termios);
			qreply(q, mp);
			break;

		case TCSBRK:
			if (mp->b_cont == NULL) {
				freemsg(mp);
				return(0);
			}
			/* LINTED pointer alignment */
			if (!(*(int *) mp->b_cont->b_rptr)) {
				/* EX_DATA_SUP is invariant */
				if (ntp->state & EX_DATA_SUP) {
					if ((ctlmp = allocb(sizeof(struct T_exdata_req), BPRI_MED)) == NULL) {
						mp->b_datap->db_type = M_IOCNAK;
						iocp->ioc_error = ENOSR;
						iocp->ioc_count = 0;
						qreply(q, mp);
						return(0);
					}

					if ((ctlmp->b_cont = allocb(EX_BREAK_SIZE, BPRI_MED)) == NULL) {
						mp->b_datap->db_type = M_IOCNAK;
						iocp->ioc_error = ENOSR;
						iocp->ioc_count = 0;
						qreply(q, mp);
						return(0);
					}

					ctlmp->b_wptr = ctlmp->b_rptr + sizeof(struct T_exdata_req);
					/* LINTED pointer alignment */
					ep = (struct T_exdata_req *) ctlmp->b_rptr;
					ep->PRIM_type = T_EXDATA_REQ;
					ep->MORE_flag = 0;
					ctlmp->b_datap->db_type = M_PROTO;

					*ctlmp->b_cont->b_wptr++ = EX_BREAK_HI;
					*ctlmp->b_cont->b_wptr++ = EX_BREAK_LO;
					putnext(q, ctlmp);
				} else {
					/* try our best */
					(void) putnextctl(q, M_BREAK);
				}
			}

			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			qreply(q, mp);
			return(0);

		case TIOCGETP:
			mp->b_cont = allocb(sizeof(struct sgttyb), BPRI_HI);
			if (mp->b_cont == NULL) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = ENOMEM;
				qreply(q, mp);
			}

			/* LINTED pointer alignment */
			gb = (struct sgttyb *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->lock, plstr);
			gb->sg_ispeed = ntp->cflags & CBAUD;
			gb->sg_ospeed = gb->sg_ispeed;
			gb->sg_flags = 0;

			if (ntp->cflags & HUPCL)
				gb->sg_flags |= O_HUPCL;

			if (ntp->cflags & PARODD)
				gb->sg_flags |= O_ODDP;

			UNLOCK(ntp->lock, pl);
			mp->b_cont->b_wptr += sizeof(struct sgttyb);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_error = 0;
			iocp->ioc_count = sizeof(struct sgttyb);
			qreply(q, mp);
			return(0);

		case TIOCSETP:
			if (mp->b_cont == NULL) {
				freemsg(mp);
				return(0);
			}
			/* LINTED pointer alignment */
			gb = (struct sgttyb *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->lock, plstr);
			ntp->cflags = (gb->sg_ispeed & CBAUD) | CREAD;
			if ((ntp->cflags & CBAUD) == B110)
				ntp->cflags |= CSTOPB;
			if (gb->sg_flags & O_HUPCL)
				ntp->cflags |= HUPCL;
			if (!(gb->sg_flags & O_RAW))
				ntp->cflags |= CS7|PARENB;
			if (gb->sg_flags & O_ODDP)
				if (!(gb->sg_flags & O_EVENP))
					ntp->cflags |= PARODD;

			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			iocp->ioc_error = 0;

			if ((ntp->cflags & CBAUD) == B0) {
				/* hang-up: generate a disconnect request */
				ntp->ioc_mp = mp;
				mp = ntp->mp;
				if (mp) {
					ntp->mp = NULL;	/* use only once */
					ntp->state |= WACK;
					UNLOCK(ntp->lock, pl);
					/* LINTED pointer alignment */
					tp = (struct T_discon_req *) mp->b_rptr;

					mp->b_wptr = mp->b_rptr + sizeof(struct T_discon_req);
					tp->PRIM_type = T_DISCON_REQ;
					tp->SEQ_number = -1;
					mp->b_datap->db_type = M_PROTO;
					putnext(q, mp);
				} else
					UNLOCK(ntp->lock, pl);
				return(0);
			}
			UNLOCK(ntp->lock, pl);
			qreply(q, mp);
			return(0);

		default:
			putnext(q, mp);
			return(0);
	}
	break;
	
	case M_DELAY:
		/* tty delays not supported over network at this time */
		freemsg(mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
	return(0);
}
