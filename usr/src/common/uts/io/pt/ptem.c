/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/pt/ptem.c	1.11"
#ident	"$Header: $"

/*
 * PTEM - a streams module which serves as a pseudo driver emulator;
 * emulate the ioctl() functions of a terminal device driver.
 */

#include <io/pt/ptem.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/termios.h>
#include <io/jioctl.h>
#include <mem/kmem.h>
#include <proc/cred.h>
#include <proc/signal.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/param.h>
#include <util/types.h>
#include <util/debug.h>
#include <util/mod/moddefs.h>
#include <io/ddi.h>

#define MODNAME "ptem - Loadable pseudo driver emulator."

MOD_STR_WRAPPER(ptem, NULL, NULL, MODNAME);

STATIC int ptemopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int ptemclose(queue_t *, int, cred_t *);
STATIC int ptemrput(queue_t *, mblk_t *);
STATIC int ptemwput(queue_t *, mblk_t *);
STATIC int ptemwsrv(queue_t *);
STATIC void ptemioctl(queue_t *, mblk_t *, int);

/* Hi water mark is set to 1 purposely to propagate flow control correctly */

STATIC struct module_info ptem_info = {
	0xabcd, "ptem", 0, 512, 1, 0
};

STATIC struct qinit ptemrinit = {
	ptemrput, NULL, ptemopen, ptemclose, NULL, &ptem_info, NULL
};

STATIC struct qinit ptemwinit = {
	ptemwput, ptemwsrv, NULL, NULL, NULL, &ptem_info, NULL
};

struct streamtab pteminfo = {
	&ptemrinit, &ptemwinit, NULL, NULL
};

int ptemdevflag = D_MP;

#define	PTEM_HIER 1
	/*+ to protect ptem structure */
STATIC LKINFO_DECL(ptem_lkinfo, "IM:PT/PTEM:struct ptem/ptem_lock", LK_BASIC);

/* ARGSUSED */
/*
 * STATIC int
 * ptemopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
 *	PTEM MODULE OPEN ROUTINE
 *
 * Calling/Exit State:
 *	gets called when the module gets pushed onto the stream.
 */
STATIC int
ptemopen(queue_t *q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	struct ptem *ntp;
	mblk_t *mop;
	struct stroptions *sop;

	if (sflag != MODOPEN)
		return(EINVAL);
	if (q->q_ptr) 		/* already attached */
		return(0);
	if (! (ntp =
	    (struct ptem *) kmem_zalloc(sizeof(struct ptem), KM_NOSLEEP)))
		return(EAGAIN);
	if (! (ntp->ptem_lock =
	    LOCK_ALLOC(PTEM_HIER, plstr, &ptem_lkinfo, KM_NOSLEEP))) {
		kmem_free(ntp, sizeof(struct ptem));
		return(EAGAIN);
	}
	/*
	 * allocate a message block, used to pass the "zero-length"
	 * message for "stty 0"
	 * NOTE: its better to find out if such a message block can be
	 * allocated before its needed than to not be able to
	 * deliver(for possible lack of buffers) when a hang-up
	 * occurs.
	 */
	if (! (ntp->ptem_dackp = (mblk_t *) allocb(4, 0))) {
		LOCK_DEALLOC(ntp->ptem_lock);
		kmem_free(ntp, sizeof(struct ptem));
		return(EAGAIN);
	}
	/*
	 * set up hi/lo water marks on stream head read queue
	 * and add controlling tty if not set
	 */
	if (! (mop = allocb(sizeof(struct stroptions), 0))) {
		freemsg(ntp->ptem_dackp);
		LOCK_DEALLOC(ntp->ptem_lock);
		kmem_free(ntp, sizeof(struct ptem));
		return(EAGAIN);
	}
	/*
	 * Assign default cflag values cf termio(7)
	 * zero out the windowing paramters
	 */
	ntp->ptem_cflags = B300|CS8|CREAD|HUPCL;
	ntp->ptem_wsz.ws_row = 0;
	ntp->ptem_wsz.ws_col = 0;
	ntp->ptem_wsz.ws_xpixel = 0;
	ntp->ptem_wsz.ws_ypixel = 0;
	q->q_ptr = (caddr_t) ntp;
	WR(q)->q_ptr = (caddr_t) ntp;
	mop->b_datap->db_type = M_SETOPTS;
	mop->b_wptr += sizeof(struct stroptions);
	/* LINTED pointer alignment */
	sop = (struct stroptions *) mop->b_rptr;
	sop->so_flags = SO_HIWAT|SO_LOWAT|SO_ISTTY;
	sop->so_hiwat = 512;
	sop->so_lowat = 256;
	putnext(q, mop);
	qprocson(q);
	return(0);
}

/* ARGSUSED */
/*
 * STATIC int
 * ptemclose(queue_t *q, int cflag, cred_t *crp)
 *	PTEM MODULE CLOSE ROUTINE
 *
 * Calling/Exit State:
 *	gets called when the module gets popped off of the stream.
 */
STATIC int
ptemclose(queue_t *q, int cflag, cred_t *crp)
{
	register struct ptem *ntp;

	ASSERT(q);
	qprocsoff(q);
	ntp = (struct ptem *) q->q_ptr;
	ASSERT(ntp);
	q->q_ptr = NULL;
	WR(q)->q_ptr = NULL;
	/*
	 * NOTE: it not required to hold ptem_lock here, since we
	 * have already disable put and service procedures.
	 */
	if (ntp->ptem_dackp)
		freemsg(ntp->ptem_dackp);
	LOCK_DEALLOC(ntp->ptem_lock);
	kmem_free(ntp, sizeof(struct ptem));
	return(0);
}

/*
 * STATIC int
 * ptemrput(queue_t *q, mblk_t *mp)
 *	PTEM MODULE READ PUT PROCEDURE
 *
 * Calling/Exit State:
 *	called from the module or driver downstream.
 */
STATIC int
ptemrput(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocp;
	struct copyresp *resp;	/* the transparent ioctl response structure */
	struct copyreq *get_buf_p;
	mblk_t *mctlp;	/* M_CTL message */
	int sig_number;	/* an ioctl(TIOCSIGNAL) setable signal number */

	ASSERT(q && mp);

	switch (mp->b_datap->db_type) {

	case M_DELAY:
	case M_READ:
		freemsg(mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(WR(q), FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);
		putnext(q, mp);
		break;

	case M_IOCTL:
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;

		switch (iocp->ioc_cmd) {

		case TCSBRK:
		    {
			mblk_t *bp = NULL;

			/*
			 * Send a break message upstream
			 */
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			if (! (*(int *)mp->b_cont->b_rptr)) {
				if (! (bp = allocb(0, 0))) {
					/*
					 * Send an NAK reply back
					 */
					iocp->ioc_error = EAGAIN;
					mp->b_datap->db_type = M_IOCNAK;
					qreply(q, mp);
					break;
				}
				bp->b_datap->db_type = M_BREAK;
			}
			/*
			 * Send a reply back, default is a M_IOCACK
			 */
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			qreply(q, mp);
			/*
			 * Then putnextctl(q, M_BREAK), if it has
			 */
			if (bp)
				putnext(q, bp);
			break;
		    }

		case TIOCSWINSZ:
			if (iocp->ioc_count != TRANSPARENT) {
				ptemioctl(q, mp, RDSIDE);
				break;
			}
			/* LINTED pointer alignment */
			get_buf_p = (struct copyreq *) mp->b_rptr;
			get_buf_p->cq_private = NULL;
			get_buf_p->cq_flag = 0;
			get_buf_p->cq_size = sizeof(struct winsize);
			get_buf_p->cq_addr =	/* LINTED pointer alignment */
			    (caddr_t) (*(long *)(mp->b_cont->b_rptr));
			ASSERT(mp->b_cont);
			freeb(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_COPYIN;
			mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
			qreply(q, mp);
			break;

		case JWINSIZE:
		case TIOCGWINSZ:
			ptemioctl(q, mp, RDSIDE);
			break;

		case TIOCSIGNAL:
		    {
			mblk_t *bp;

			/*
			 * This ioctl can emanate from the master side
			 * in remote mode only
			 */
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			sig_number = *(int *) mp->b_cont->b_rptr;
			if (sig_number < 1 || sig_number > NSIG) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EINVAL;
				iocp->ioc_count = 0;
				qreply(q, mp);
				break;
			}
			if (! (bp = allocb(1, 0))) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EAGAIN;
				iocp->ioc_count = 0;
				qreply(q, mp);
				break;
			}
			bp->b_datap->db_type = M_PCSIG;
			*bp->b_wptr++ = (char) sig_number;
			/*
			 * Reply to the ioctl first
			 */
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			qreply(q, mp);
			/*
			 * Then putnextctl1(M_PCSIG, sig_number)
			 */
			putnext(q, bp);
			break;
		    }

		case TIOCREMOTE:
			/*
			 * Send M_CTL up using the iocblk format
			 */
			if (! (mctlp = allocb(sizeof(struct iocblk), 0))) {
				iocp->ioc_count = 0;
				iocp->ioc_error = EAGAIN;
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				break;
			}

			mctlp->b_wptr = mctlp->b_rptr + sizeof(struct iocblk);
			mctlp->b_datap->db_type = M_CTL;
			/* LINTED pointer alignment */
			iocp = (struct iocblk *) mctlp->b_rptr;
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			if (*(int *) mp->b_cont->b_rptr)
				iocp->ioc_cmd = MC_NO_CANON;
			else
				iocp->ioc_cmd = MC_DO_CANON;
			/*
			 * Send the M_CTL upstream to LDTERM
			 */
			putnext(q, mctlp);

			/*
			 * Format IOCACK message and pass back to
			 * the master
			 */
			/* LINTED pointer alignment */
			iocp = (struct iocblk *) mp->b_rptr;
			iocp->ioc_count = 0;
			iocp->ioc_error = 0;
			iocp->ioc_rval = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;

		default:
			putnext(q, mp);
			break;
		}
		break;

	case M_IOCDATA:
		/* LINTED pointer alignment */
		resp = (struct copyresp *) mp->b_rptr;
		if (resp->cp_rval) {
			/*
			 * Just free message on failure
			 */
			freemsg(mp);
			break;
		}
		/*
		 * Only need to copy data for the SET case
		 */
		switch (resp->cp_cmd) {

		case  TIOCSWINSZ:
			ptemioctl(q, mp, RDSIDE);
			break;

		case  JWINSIZE:
		case  TIOCGWINSZ:
			/* LINTED pointer alignment */
			iocp = (struct iocblk *) mp->b_rptr;
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			iocp->ioc_rval = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;

		default:
			freemsg(mp);
		}
		break;

	case M_IOCNAK:
		/*
		 * If the PCKT module is not pushed on the master
		 * side all M_IOCTLs will be NAKed by the master
		 * stream head. Free them here.
		 */
		freemsg(mp);
		break;

	default:
		putnext(q, mp);
		break;
	}
	return(0);
}

/*
 * STATIC int
 * ptemwput(queue_t *q, mblk_t *mp)
 *	PTEM MODULE WRITE PUT PROCEDURE
 *
 * Calling/Exit State:
 *	called from the module or stream head upstream.
 */
STATIC int
ptemwput(queue_t *q, mblk_t *mp)
{
	struct iocblk *iocp;	/* the outgoing ioctl structure */
	struct termio *termiop;
	struct termios *termiosp;
	struct ptem *ntp;
	struct copyresp *resp;
	struct copyreq *get_buf_p;
	mblk_t *dackp;		/* the disconnect message ACK block */
	mblk_t *pckt_msgp;	/* the message sent to the PCKT module */
	pl_t pl;

	ASSERT(q && mp);
	ntp = (struct ptem *) q->q_ptr;
	ASSERT(ntp);

	switch (mp->b_datap->db_type) {

	case M_IOCDATA:
		/* LINTED pointer alignment */
		resp = (struct copyresp *) mp->b_rptr;
		if (resp->cp_rval) {
			/*
			 * Just free message on failure
			 */
			freemsg(mp);
			break;
		}
		/*
		 * Only need to copy data for the SET case
		 */
		switch (resp->cp_cmd) {

		case  TIOCSWINSZ:
			ptemioctl(q, mp, WRSIDE);
			break;

		case JWINSIZE:
		case TIOCGWINSZ:
			/* LINTED pointer alignment */
			iocp = (struct iocblk *) mp->b_rptr;
			iocp->ioc_error = 0;
			iocp->ioc_count = 0;
			iocp->ioc_rval = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;

		default:
			freemsg(mp);
		}
		break;

	case M_IOCTL:
		/*
		 * NOTE: for each "set" type operation a copy of the M_IOCTL
		 * message is made and passed downstream. Eventually the
		 * PCKT module, if it has been pushed, should pick up this
		 * message. If the PCKT module has not been pushed the master
		 * side stream head will free it
		 */
		/* LINTED pointer alignment */
		iocp = (struct iocblk *) mp->b_rptr;

		switch (iocp->ioc_cmd) {

		case TCSETAF:
			/*
			 * Flush the read queue
			 */
			if (! putnextctl1(q, M_FLUSH, FLUSHR)) {
				iocp->ioc_count = 0;
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EAGAIN;
				qreply(q, mp);
				break;
			}
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			termiop = (struct termio *) mp->b_cont->b_rptr;

			pl = LOCK(ntp->ptem_lock, plstr);
			ntp->ptem_cflags =
			    (ntp->ptem_cflags & 0xffff0000 | termiop->c_cflag);

			if ((termiop->c_cflag&CBAUD) == B0) {
				/*
				 * hang-up: Send a zero length message
				 */
				dackp = ntp->ptem_dackp;
				if (dackp) {
					ntp->ptem_dackp = NULL; /* use only once */
					UNLOCK(ntp->ptem_lock, pl);
					/*
					 * Send a zero length message downstream
					 */
					putnext(q, dackp);
				} else
					UNLOCK(ntp->ptem_lock, pl);
			} else {
				UNLOCK(ntp->ptem_lock, pl);
				/*
				 * Need a copy of this message to pass on to
				 * the PCKT module
				 */
				if (! (pckt_msgp = copymsg(mp))) {
					iocp->ioc_count = 0;
					iocp->ioc_error = EAGAIN;
					mp->b_datap->db_type = M_IOCNAK;
					qreply(q, mp);
					break;
				}
				/*
				 * Send a copy of the M_IOCTL to PCKT module
				 */
				putnext(q, pckt_msgp);
			}
			/*
			 * Send ACK upstream
			 */
			iocp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;

		case TCSETA:
		case TCSETAW:
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			termiop = (struct termio *) mp->b_cont->b_rptr;

			pl = LOCK(ntp->ptem_lock, plstr);
			ntp->ptem_cflags =
			    (ntp->ptem_cflags & 0xffff0000 | termiop->c_cflag);

			if ((termiop->c_cflag&CBAUD) == B0) {
				/*
				 * hang-up: Send a zero length message
				 */
				dackp = ntp->ptem_dackp;
				if (dackp) {
					ntp->ptem_dackp = NULL; /* use only once */
					UNLOCK(ntp->ptem_lock, pl);
					/*
					 * Send a zero length message downstream
					 */
					putnext(q, dackp);
				} else
					UNLOCK(ntp->ptem_lock, pl);
			} else {
				UNLOCK(ntp->ptem_lock, pl);
				/*
				 * Need a copy of this message to pass on to
				 * the PCKT module
				 */
				if (! (pckt_msgp = copymsg(mp))) {
					iocp->ioc_count = 0;
					iocp->ioc_error = EAGAIN;
					mp->b_datap->db_type = M_IOCNAK;
					qreply(q, mp);
					break;
				}
				/*
				 * Send a copy of the M_IOCTL to the PCKT module
				 */
				putnext(q, pckt_msgp);
			}
			/*
			 * Send ACK upstream
			 */
			iocp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;

		case TCSETSF:
			/*
			 * Flush the read queue
			 */
			if (! putnextctl1(q, M_FLUSH, FLUSHR)) {
				iocp->ioc_count = 0;
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EAGAIN;
				qreply(q, mp);
				break;
			}
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			termiosp = (struct termios *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->ptem_lock, plstr);
			ntp->ptem_cflags = termiosp->c_cflag;

			if ((termiosp->c_cflag&CBAUD) == B0) {
				/*
				 * hang-up: Send a zero length message
				 */
				dackp = ntp->ptem_dackp;
				if (dackp) {
					ntp->ptem_dackp = NULL; /* use only once */
					UNLOCK(ntp->ptem_lock, pl);
					/*
					 * Send a zero length message downstream
					 */
					putnext(q, dackp);
				} else
					UNLOCK(ntp->ptem_lock, pl);
			} else {
				UNLOCK(ntp->ptem_lock, pl);
				/*
				 * Need a copy of this message to pass on to
				 * the PCKT module
				 */
				if (! (pckt_msgp = copymsg(mp))) {
					iocp->ioc_count = 0;
					iocp->ioc_error = EAGAIN;
					mp->b_datap->db_type = M_IOCNAK;
					qreply(q, mp);
					break;
				}
				/*
				 * Send the orginal M_IOCTL to the PCKT module
				 */
				putnext(q, pckt_msgp);
			}
			/*
			 * Send ACK upstream
			 */
			iocp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;

		case TCSETS:
		case TCSETSW:
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			termiosp = (struct termios *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->ptem_lock, plstr);
			ntp->ptem_cflags = termiosp->c_cflag;

			if ((termiosp->c_cflag&CBAUD) == B0) {
				/*
				 * hang-up: Send a zero length message
				 */
				dackp = ntp->ptem_dackp;
				if (dackp) {
					ntp->ptem_dackp = NULL; /* use only once */
					UNLOCK(ntp->ptem_lock, pl);
					/*
					 * Send a zero length message downstream
					 */
					putnext(q, dackp);
				} else
					UNLOCK(ntp->ptem_lock, pl);
			} else {
				UNLOCK(ntp->ptem_lock, pl);
				/*
				 * Need a copy of this message to pass on to
				 * the PCKT module
				 */
				if (! (pckt_msgp = copymsg(mp))) {
					iocp->ioc_count = 0;
					iocp->ioc_error = EAGAIN;
					mp->b_datap->db_type = M_IOCNAK;
					qreply(q, mp);
					break;
				}
				/*
				 * Send the orginal M_IOCTL to the PCKT module
				 */
				putnext(q, pckt_msgp);
			}
			/*
			 * Send ACK upstream
			 */
			iocp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;

		case TCGETA:
			if (mp->b_cont)
				freemsg(mp->b_cont);

			if (! (mp->b_cont = allocb(sizeof(struct termio), 0))) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EAGAIN;
				iocp->ioc_count = 0;
				qreply(q, mp);
				break;
			}
			mp->b_cont->b_wptr =
			    mp->b_cont->b_rptr + sizeof(struct termio);
			/* LINTED pointer alignment */
			termiop = (struct termio *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->ptem_lock, plstr);
			termiop->c_cflag = (ushort) ntp->ptem_cflags;
			UNLOCK(ntp->ptem_lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = sizeof(struct termio);
			qreply(q, mp);
			break;

		case TCGETS:
			if (mp->b_cont)
				freemsg(mp->b_cont);

			if (! (mp->b_cont = allocb(sizeof(struct termios), 0))) {
				mp->b_datap->db_type = M_IOCNAK;
				iocp->ioc_error = EAGAIN;
				iocp->ioc_count = 0;
				qreply(q, mp);
				break;
			}

			mp->b_cont->b_wptr =
			    mp->b_cont->b_rptr + sizeof(struct termios);
			/* LINTED pointer alignment */
			termiosp = (struct termios *) mp->b_cont->b_rptr;
			pl = LOCK(ntp->ptem_lock, plstr);
			termiosp->c_cflag = ntp->ptem_cflags;
			UNLOCK(ntp->ptem_lock, pl);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = sizeof(struct termios);
			qreply(q, mp);
			break;

		case TCSBRK:
		    {
			mblk_t *bp = NULL;

			/*
			 * Need a copy of this message to pass it on to
			 * the PCKT module
			 */
			if (! (pckt_msgp = copymsg(mp))) {
				iocp->ioc_count = 0;
				iocp->ioc_error = EAGAIN;
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				break;
			}
			/*
			 * Send a copy of the M_IOCTL to the PCKT module
			 */
			putnext(q, pckt_msgp);
			/*
			 * TCSBRK meaningful if data part of message is 0
			 * cf. termio(7)
			 */
			/* LINTED pointer alignment */
			if (! (*(int *) mp->b_cont->b_rptr)) {
				if (! (bp = allocb(0, 0))) {
					iocp->ioc_count = 0;
					iocp->ioc_error = EAGAIN;
					mp->b_datap->db_type = M_IOCNAK;
					qreply(q, mp);
					break;
				}
				bp->b_datap->db_type = M_BREAK;
			}
			/*
			 * ACK the ioctl, first
			 */
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			qreply(q, mp);
			/*
			 * then, putnextctl(q, M_BREAK) if it has
			 */
			if (bp)
				putnext(q, bp);
			break;
		    }

		case JWINSIZE:
		case TIOCGWINSZ:
			ptemioctl(q, mp, WRSIDE);
			break;

		case TIOCSWINSZ:
			if (iocp->ioc_count != TRANSPARENT) {
				ptemioctl(q, mp, WRSIDE);
				break;
			}
			ASSERT(mp->b_cont);
			/* LINTED pointer alignment */
			get_buf_p = (struct copyreq *) mp->b_rptr;
			get_buf_p->cq_private = NULL;
			get_buf_p->cq_flag = 0;
			get_buf_p->cq_size = sizeof(struct winsize);
			get_buf_p->cq_addr = /* LINTED pointer alignment */
			    (caddr_t) (*(long*)(mp->b_cont->b_rptr));
			freeb(mp->b_cont);
			mp->b_cont = NULL;
			mp->b_datap->db_type = M_COPYIN;
			mp->b_wptr = mp->b_rptr + sizeof(struct copyreq);
			qreply(q, mp);
			break;

		case TIOCSTI: {
			/*
			 * Simulate typing of a character at the terminal.
			 */
			register mblk_t *bp;

			/*
			 * The permission checking has already been done at
			 * the stream head, since it has to be done in the
			 * context of the process doing the call.
			 */
			if (bp = allocb(1, 0)) {
				if (! canputnext(RD(q)))
					freemsg(bp);
				else {
					*bp->b_wptr++ = *mp->b_cont->b_rptr;
					putnext(RD(q), bp);
				}
			}
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = 0;
			putnext(RD(q), mp);
			break;
		}

		default:
			iocp->ioc_count = 0;
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_error = EINVAL;
			qreply(q, mp);
			break;
		}
		break;

	case M_READ:
	case M_DELAY: /* tty delays not supported */
		freemsg(mp);
		break;

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);
		if (*mp->b_rptr & FLUSHR)
			flushq(RD(q), FLUSHDATA);
		putnext(q, mp);
		break;

	case M_STOP:
		/*
		 * Set the output flow control state
		 */
		putnext(q, mp);
		pl = LOCK(ntp->ptem_lock, plstr);
		ntp->ptem_state |= OFLOW_CTL;
		noenable(q);
		UNLOCK(ntp->ptem_lock, pl);
		break;

	case M_START:
		/*
		 * Relieve the output flow control state
		 */
		putnext(q, mp);
		pl = LOCK(ntp->ptem_lock, plstr);
		enableok(q);
		ntp->ptem_state &= ~OFLOW_CTL;
		UNLOCK(ntp->ptem_lock, pl);
		qenable(q);
		break;

	case M_DATA:
		if ((mp->b_wptr - mp->b_rptr) <= 0)
			/*
			 * Free all zero length messages
			 */
			freemsg(mp);
		else {
			pl = LOCK(ntp->ptem_lock, plstr);
			if (ntp->ptem_state & OFLOW_CTL) {
				/*
				 * Queue data messages in the flow control case
				 * Note that M_STOP above did a noenable, so the
				 * putq will not schedule the queue
				 */
				putq(q, mp);
				UNLOCK(ntp->ptem_lock, pl);
			} else {
				if (((ntp->ptem_state & STRFLOW) == 0) && canputnext(q)) {
					UNLOCK(ntp->ptem_lock, pl);
					putnext(q, mp);
				} else {
					ntp->ptem_state |= STRFLOW;

/*
 * The noenable/enableok is just to prevent the service procedure from
 * being scheduled by the putq.  It will eventually be scheduled by a
 * backenable.  There is no race here because backenabling does not check
 * the QNOENB bit.
 */

					noenable(q);
					putq(q, mp);
					enableok(q);
					UNLOCK(ntp->ptem_lock, pl);
				}
			}
		}
		break;

	default:
		putnext(q, mp);
		break;
	}
	return(0);
}

/*
 * STATIC void
 * ptemioctl(queue_t *q, mblk_t *mp, int qside)
 *	Process IOCTL messages
 *
 * Calling/Exit State:
 *	Message must be of type M_IOCTL or M_IOCDATA for this routine
 *	to be called.
 */
STATIC void
ptemioctl(queue_t *q, mblk_t *mp, int qside)
{
	struct ptem *tp;
	struct iocblk *iocp;
	struct winsize *wb;
	struct jwinsize *jwb;
	struct copyreq *send_buf_p;
	mblk_t *tmp;
	mblk_t *pckt_msgp;	/* message sent to the PCKT module */
	pl_t pl;

	ASSERT(qside == RDSIDE || qside == WRSIDE);
	ASSERT(q);
	tp = (struct ptem *) q->q_ptr;
	ASSERT(tp);
	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;

	switch (iocp->ioc_cmd) {

	case JWINSIZE:
		/*
		 * For compatibility:
		 * If all zeros, NAK the message for dumb terminals
		 */
		pl = LOCK(tp->ptem_lock, plstr);
		if ((tp->ptem_wsz.ws_row == 0) &&
		    (tp->ptem_wsz.ws_col == 0) &&
		    (tp->ptem_wsz.ws_xpixel == 0) &&
		    (tp->ptem_wsz.ws_ypixel == 0)) {
			UNLOCK(tp->ptem_lock, pl);
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_count = 0;
			iocp->ioc_error = EINVAL;
			qreply(q, mp);
			return;
		}
		if (! (tmp = allocb(sizeof(struct jwinsize), BPRI_MED))) {
			UNLOCK(tp->ptem_lock, pl);
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_count = 0;
			iocp->ioc_error = EAGAIN;
			qreply(q, mp);
			return;
		}
		if (iocp->ioc_count == TRANSPARENT) {
			/* LINTED pointer alignment */
			send_buf_p = (struct copyreq *) mp->b_rptr;
			/* LINTED pointer alignment */
			send_buf_p->cq_addr = (caddr_t) (*(long *)(mp->b_cont->b_rptr));
			freemsg(mp->b_cont);
			mp->b_cont = tmp;
			tmp->b_wptr += sizeof(struct jwinsize);
			send_buf_p->cq_private = NULL;
			send_buf_p->cq_flag = 0;
			send_buf_p->cq_size = sizeof(struct jwinsize);
			mp->b_datap->db_type = M_COPYOUT;
			mp->b_wptr =  mp->b_rptr + sizeof(struct copyreq);
		} else {
			if (mp->b_cont)
				freemsg(mp->b_cont);
			mp->b_cont = tmp;
			mp->b_datap->db_type = M_IOCACK;
			tmp->b_wptr += sizeof(struct jwinsize);
			iocp->ioc_count = sizeof(struct jwinsize);
			iocp->ioc_error = 0;
			iocp->ioc_rval = 0;
		}
		/* LINTED pointer alignment */
		jwb = (struct jwinsize *) mp->b_cont->b_rptr;
		jwb->bytesx = tp->ptem_wsz.ws_col;
		jwb->bytesy = tp->ptem_wsz.ws_row;
		jwb->bitsx = tp->ptem_wsz.ws_xpixel;
		jwb->bitsy = tp->ptem_wsz.ws_ypixel;
		UNLOCK(tp->ptem_lock, pl);

		qreply(q, mp);
		break;

	case TIOCGWINSZ:
		/*
		 * If all zeros NAK the message for dumb terminals
		 */
		pl = LOCK(tp->ptem_lock, plstr);
		if ((tp->ptem_wsz.ws_row == 0) &&
		    (tp->ptem_wsz.ws_col == 0) &&
		    (tp->ptem_wsz.ws_xpixel == 0) &&
	  	    (tp->ptem_wsz.ws_ypixel == 0)) {
			UNLOCK(tp->ptem_lock, pl);
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_count = 0;
			iocp->ioc_error = EINVAL;
			qreply(q, mp);
			return;
		}
		if (! (tmp = allocb(sizeof(struct winsize), 0))) {
			UNLOCK(tp->ptem_lock, pl);
			mp->b_datap->db_type = M_IOCNAK;
			iocp->ioc_count = 0;
			iocp->ioc_error = EAGAIN;
			qreply(q, mp);
			return;
		}
		if (iocp->ioc_count == TRANSPARENT) {
			/* LINTED pointer alignment */
			send_buf_p = (struct copyreq *) mp->b_rptr;
			send_buf_p->cq_addr =	/* LINTED pointer alignment */
			    (caddr_t)(*(long *)(mp->b_cont->b_rptr));
			freemsg(mp->b_cont);
			mp->b_cont = tmp;
			tmp->b_wptr += sizeof(struct winsize);
			send_buf_p->cq_private = NULL;
			send_buf_p->cq_flag = 0;
			send_buf_p->cq_size = sizeof(struct winsize);
			mp->b_datap->db_type = M_COPYOUT;
		} else {
			if (mp->b_cont)
				freemsg(mp->b_cont);
			mp->b_cont = tmp;
			tmp->b_wptr += sizeof(struct winsize);
			mp->b_datap->db_type = M_IOCACK;
			iocp->ioc_count = sizeof(struct winsize);
			iocp->ioc_error = 0;
			iocp->ioc_rval = 0;
		}
		/* LINTED pointer alignment */
		wb = (struct winsize *) mp->b_cont->b_rptr;
		wb->ws_row = tp->ptem_wsz.ws_row;
		wb->ws_col = tp->ptem_wsz.ws_col;
		wb->ws_xpixel = tp->ptem_wsz.ws_xpixel;
		wb->ws_ypixel = tp->ptem_wsz.ws_ypixel;
		UNLOCK(tp->ptem_lock, pl);
		qreply(q, mp);
		break;

	case TIOCSWINSZ:
		ASSERT(mp->b_cont);
		/* LINTED pointer alignment */
		wb = (struct winsize *) mp->b_cont->b_rptr;
		/*
		 * Send a SIGWINCH signal if the row/col information
		 * has changed.
		 */
		pl = LOCK(tp->ptem_lock, plstr);
		if ((tp->ptem_wsz.ws_row == wb->ws_row) &&
		    (tp->ptem_wsz.ws_col == wb->ws_col) &&
		    (tp->ptem_wsz.ws_xpixel == wb->ws_xpixel) &&
		    (tp->ptem_wsz.ws_ypixel == wb->ws_xpixel)) {
			UNLOCK(tp->ptem_lock, pl);
			iocp->ioc_count = 0;
			iocp->ioc_error = 0;
			iocp->ioc_rval = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;
		}
		tp->ptem_wsz.ws_row = wb->ws_row;
		tp->ptem_wsz.ws_col = wb->ws_col;
		tp->ptem_wsz.ws_xpixel = wb->ws_xpixel;
		tp->ptem_wsz.ws_ypixel = wb->ws_ypixel;
		UNLOCK(tp->ptem_lock, pl);
		/*
		 * message may have come in as an M_IOCDATA
		 * pass it to the master side as an M_IOCTL
		 */
		if (qside == WRSIDE) {
			/*
			 * Need a copy of this message to pass on to
			 * the PCKT module, only if the M_IOCTL
			 * orginated from the slave side.
			 */
			mp->b_datap->db_type = M_IOCTL;
			if (! (pckt_msgp = copymsg(mp))) {
				iocp->ioc_count = 0;
				iocp->ioc_error = EAGAIN;
				mp->b_datap->db_type = M_IOCNAK;
				qreply(q, mp);
				goto winch;
			}
			putnext(q, pckt_msgp);
		}
		iocp->ioc_count = 0;
		iocp->ioc_error = 0;
		iocp->ioc_rval = 0;
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
winch:
		/*
		 * SIGWINCH is sent always upstream
		 */
		if (qside == WRSIDE)
			putnextctl1(RD(q), M_SIG, SIGWINCH);
		else
			putnextctl1(q, M_SIG, SIGWINCH);
		break;

	default:
		putnext(q, mp);
		break;
	}
}

/* ARGSUSED */
/*
 * STATIC int
 * ptemwsrv(queue_t *q)
 *	PTEM MODULE WRITE SERVICE PROCEDURE
 *	The service procedure is only invoked after flow control has
 *	subsided downstream or if an M_START has been received.  It
 *	is important to note that the hi water mark is 1; thus, the
 *	queueing of any message will propagate flow control upstream.
 *
 * Calling/Exit State:
 *	Assumes no locks held
 */
STATIC int
ptemwsrv(queue_t *q)
{
	pl_t pl;
	struct ptem *ptp;
	mblk_t *mp;

	ptp = (struct ptem *) q->q_ptr;
	pl = LOCK(ptp->ptem_lock, plstr);
	/* may be reset below */
	ptp->ptem_state &= ~STRFLOW;
	UNLOCK(ptp->ptem_lock, pl);
	while ((mp = getq(q)) != NULL) {
		if (canputnext(q)) {
			pl = LOCK(ptp->ptem_lock, plstr);

/*
 * Just in case an M_STOP came in in the middle of this, check again.
 * It could come in after the unlock and before the putnext, but the
 * receipt of a stop is racy anyhow and sending 1 extra  message downstream
 * is ok
 */
			if (ptp->ptem_state & OFLOW_CTL) {
				noenable(q);	/* to be safe */
				putbq(q, mp);
				UNLOCK(ptp->ptem_lock, pl);
				return(0);
			}
			UNLOCK(ptp->ptem_lock, pl);
			putnext(q, mp);
		} else {
			pl = LOCK(ptp->ptem_lock, plstr);
			ptp->ptem_state |= STRFLOW;
			noenable(q);
			putbq(q, mp);
			enableok(q);
			UNLOCK(ptp->ptem_lock, pl);
			return(0);
		}
	}
	return(0);
}
