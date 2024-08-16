/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/asy/iasy.c	1.26"
#ident	"$Header: $"


/*
 * Generic Terminal Driver (STREAMS version)
 * This forms the hardware independent part of a serial driver.
 */


#include <fs/fcntl.h>
#include <fs/file.h>
#include <io/conssw.h>
#include <io/ldterm/eucioctl.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <proc/cred.h>
#include <proc/signal.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>
#include <io/asy/iasy.h>

#ifdef DEBUGSTR			/* To turn on/off the strlog messages */
#include <io/strlog.h>
#include <io/log/log.h>
#define	DSTRLOG(x)	strlog x
#else
#define	DSTRLOG(x)
#endif /* DEBUGSTR */

#include <io/ddi.h>		/* Must come last */


#define	IASYHIER		1
#define IASYPL			plstr
#define	CL_TIME			(8*HZ)	/* Timeout to remove block state
					 * while closing 
					 */
/*
 * Values for t_dstat
 */
#define IASY_EXCL_OPEN		(1 << 0)

/*
 * various macro definitions
 */
#define TP_TO_Q(tp)		((tp)->t_rdqp)
#define Q_TO_TP(q)		((struct strtty *)(q)->q_ptr)
#define TP_TO_HW(tp)		(&iasy_hw[IASY_MINOR_TO_UNIT((tp)->t_dev)])
#define TP_TO_SV(tp)		(&iasy_sv[IASY_MINOR_TO_UNIT((tp)->t_dev)])
#define TP_TO_TOID(tp)		(iasy_toid[IASY_MINOR_TO_UNIT((tp)->t_dev)])
#define HW_PROC(tp, func)	((*TP_TO_HW(tp)->proc)(tp, func))


/*
 * function prototype definitions
 */
void		iasyinit(void);
void		iasystart(void);
void		iasy_hup(struct strtty *);

STATIC int	iasyopen(queue_t *, dev_t *, int, int, cred_t *);
STATIC int	iasyclose(queue_t *, int, cred_t *);
STATIC void	iasy_drain(struct strtty *);
STATIC void	iasydelay(struct strtty *);
STATIC void	iasyputioc(queue_t *, mblk_t *);
STATIC int	iasyoput(queue_t *, mblk_t *);
STATIC mblk_t	*iasygetoblk(struct queue *);
STATIC void	iasysrvioc(queue_t *, mblk_t *);
STATIC void	iasyflush(queue_t *, int);
STATIC int	iasyisrv(queue_t *);
STATIC int	iasyosrv(queue_t *);
STATIC void	iasybufwake(struct strtty *);

STATIC dev_t	iasycnopen(minor_t, boolean_t, const char *);
STATIC void	iasycnclose(minor_t, boolean_t);
STATIC int	iasycnputc(minor_t, int);
STATIC int	iasycngetc(minor_t);
STATIC void	iasycnsuspend(minor_t);
STATIC void	iasycnresume(minor_t);


/*
 * variables defined in iasy.cf/Space.c
 */
extern struct strtty	asy_tty[];	/* tty structs for each device. 
					 * changed from iasy_tty to asy_tty for
					 * for merge.
					 */
extern struct iasy_hw	iasy_hw[];	/* hardware info per device */
extern int		iasy_num;	/* maximum number of hardware ports */
extern struct iasy_sv	iasy_sv[];	/* sync variables per port */
extern int 		iasy_toid[];	/* timeout variables per port */


/*
 * Initialize iasy STREAMs module_info, qinits and 
 * streamtab structures
 */
struct module_info iasy_info = {
	901, "iasy", 0, INFPSZ, IASY_HIWAT, IASY_LOWAT };

static struct qinit iasy_rint = {
	putq, iasyisrv, iasyopen, iasyclose, NULL, &iasy_info, NULL};

static struct qinit iasy_wint = {
	iasyoput, iasyosrv, iasyopen, iasyclose, NULL, &iasy_info, NULL};

struct streamtab iasyinfo = {
	&iasy_rint, &iasy_wint, NULL, NULL};


/*
 * global variables
 */
int	iasydevflag = 0;	/* driver attributes */
int	iasy_cnt = 0;		/* - /etc/crash requirement
				 * - count of ports registered
				 */ 
conssw_t iasyconssw = { 
	iasycnopen, iasycnclose, iasycnputc, iasycngetc,
	iasycnsuspend, iasycnresume
};

STATIC lock_t *iasy_lock;		/* iasy mutex lock */

LKINFO_DECL(iasy_lkinfo, "IASY::iasy_lock", 0);

STATIC boolean_t iasy_initialized;

#ifndef lint
static char iasy_copyright[] = "Copyright 1991 Intel Corporation xxxxxx";
#endif /*lint*/

extern int strhead_iasy_hiwat ; 
extern int strhead_iasy_lowat ;

/*
 * STATIC void
 * iasybufwake(struct strtty *)
 *
 * Calling/Exit State:
 *	None.
 * 
 * Description:
 *	Wakeup sleep function calls sleeping for a STREAMS buffer
 *	to become available
 */
STATIC void
iasybufwake(struct strtty *tp)
{
	SV_SIGNAL((TP_TO_SV(tp))->iasv_buf, 0);
}


/*
 * STATIC int
 * iasy_drain(struct strtty *)
 *
 * Calling/Exit State:
 *	- Called at plstr after the timeout has expired.
 */
STATIC void 
iasy_drain(struct strtty *tp)
{
	HW_PROC(tp, T_RESUME);
	tp->t_state &= ~TTIOW;
	SV_SIGNAL((TP_TO_SV(tp))->iasv_drain, 0);
	TP_TO_TOID(tp) = 0;
}


/*
 * void
 * iasyinit(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
iasyinit(void)
{
	void (**funcpp)();
	extern void (*asyinit_funcs[])();

	/* Call asy initialization functions */
	for (funcpp = asyinit_funcs; *funcpp;)
		(*(*funcpp++))();

	iasy_initialized = B_TRUE;
}


/*
 * void
 * iasystart(void)
 *
 * Calling/Exit State:
 *	None.
 */
void
iasystart(void)
{

	iasy_lock = LOCK_ALLOC(IASYHIER, IASYPL, &iasy_lkinfo, KM_NOSLEEP);

	if (!iasy_lock)
		/*
		 *+ There is not enough memory available to allocate
		 *+ serial port (iasy) mutex lock.
		 */
		cmn_err(CE_PANIC, 
			"Not enough memory available");
}


/*
 * STATIC int
 * iasyopen(queue_t *, dev_t *, int, int, cred_t *)
 *	Open an iasy line
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
iasyopen(queue_t *q, dev_t *devp, int flag, int sflag, cred_t *crp)
{	
	struct strtty *tp;
	struct stroptions *sop;
	struct iasy_sv *iasv;
	minor_t	ldev, dev;
	pl_t	oldpri;
	mblk_t	*mop;
	int	ret;
	toid_t	bid;			/* bufcall id */

	ldev = getminor(*devp);
	dev = IASY_MINOR_TO_UNIT(ldev);

	/* Check for valid device Number */
	if (dev >= iasy_num)
		return (ENXIO);

	/* Check if the HW device routine registered */
	if (iasy_hw[dev].proc == 0)
		/* No hardware for this minor number */
		return (ENXIO);

	tp = &asy_tty[dev];

	iasv = &iasy_sv[dev];
	if (iasv->iasv_drain == NULL) {
		iasv->iasv_drain = SV_ALLOC(KM_SLEEP);
		iasv->iasv_carrier = SV_ALLOC(KM_SLEEP);
		iasv->iasv_buf = SV_ALLOC(KM_SLEEP);
	}

	oldpri = SPL();

	/* 
	 * Do the required things on first open 
	 */
	if ((tp->t_state & (ISOPEN|WOPEN)) == 0) {
		tp->t_dev = ldev;
		tp->t_rdqp = q;
		q->q_ptr = (caddr_t) tp;
		WR(q)->q_ptr = (caddr_t) tp;

		/*
		 * set process group on first tty open 
		 */
		while ((mop = allocb(sizeof(struct stroptions), BPRI_MED)) == NULL) {
			if (flag & (FNDELAY|FNONBLOCK)) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return (EAGAIN);
			}

			bid = bufcall((uint)sizeof(struct stroptions), 
					BPRI_MED, iasybufwake, (long)tp);
			if (bid == 0) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				/*
				 *+ bufcall() was unsuccessful in scheduling
				 *+ a buffer allocation request. Check the
				 *+ memory configuration.
				 */
				cmn_err(CE_WARN, 
					"Not enough memory available");
				return (ENOMEM);
			}

			(void) LOCK(iasy_lock, plstr); 
			if (SV_WAIT_SIG((TP_TO_SV(tp))->iasv_buf, TTIPRI, 
						iasy_lock) == B_FALSE) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return (EINTR);
			}
		}

		mop->b_datap->db_type = M_SETOPTS;
		mop->b_wptr += sizeof(struct stroptions);
		/* LINTED pointer alignment */
		sop = (struct stroptions *)mop->b_rptr;
		sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
		sop->so_hiwat = strhead_iasy_hiwat;
		sop->so_lowat = strhead_iasy_lowat;
		(void) putnext(q, mop);
	
		/*
		 * Set water marks on write q 
		 */
		strqset(WR(q), QHIWAT, 0, IASY_HIWAT);
		strqset(WR(q), QLOWAT, 0, IASY_LOWAT);

                tp->t_cc[VSTART] = CSTART;
                tp->t_cc[VSTOP] = CSTOP;

		tp->t_iflag = 0;
		tp->t_oflag = 0;
		tp->t_cflag = B9600|CS8|CREAD|HUPCL;
		tp->t_lflag = 0;

		/*
		 * allocate RX buffer 
		 */
		while ((tp->t_in.bu_bp = 
				allocb(IASY_BUFSZ, BPRI_MED)) == NULL) {
			if (flag & (FNDELAY|FNONBLOCK)) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return (EAGAIN);
			}

			bid = bufcall((uint)sizeof(struct stroptions),
					BPRI_MED, iasybufwake, (long)tp);
			if (bid == 0) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				/*
				 *+ bufcall() was unsuccessful in scheduling
				 *+ a buffer allocation request. Check the
				 *+ memory configuration. 
				 */
				cmn_err(CE_WARN,
					"Not enough memory available");
				return (ENOMEM);
			}

			(void) LOCK(iasy_lock, plstr);
			if (SV_WAIT_SIG((TP_TO_SV(tp))->iasv_buf, TTIPRI, 
						iasy_lock) == B_FALSE) {
				tp->t_rdqp = NULL;
				splx(oldpri);
				return (EINTR);
			}
		}

		tp->t_in.bu_cnt = IASY_BUFSZ;
		tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
		tp->t_out.bu_bp = 0;
		tp->t_out.bu_cnt = 0;
		tp->t_out.bu_ptr = 0;

		if (ret = HW_PROC(tp, T_FIRSTOPEN)) {
			tp->t_rdqp = NULL;
			if (tp->t_in.bu_bp) {
				freeb(tp->t_in.bu_bp);
				tp->t_in.bu_bp = 0;
			}
			splx(oldpri);
			return (ret);
		}

		tp->t_dstat = 0;
		if (flag & FEXCL)
			tp->t_dstat = IASY_EXCL_OPEN;
	} else {
		if (ldev != tp->t_dev || (tp->t_dstat & IASY_EXCL_OPEN) ||
		    (flag & FEXCL)) {
			return (EBUSY);
		}
	}
	
	/*
	 * Init HW and SW state 
	 */
	if (ret = HW_PROC(tp, T_CONNECT)) { /* T_CONNECT must compute CARR_ON */
		(void) HW_PROC(tp, T_DISCONNECT);
		if (!(tp->t_state & ISOPEN))
			(void) HW_PROC(tp, T_LASTCLOSE);
		tp->t_rdqp = NULL;
		if (tp->t_in.bu_bp) {
			freeb(tp->t_in.bu_bp);
			tp->t_in.bu_bp = 0;
		}
		splx(oldpri);
		return (ret);
	}

	if (tp->t_cflag & CLOCAL)
		tp->t_state |= CARR_ON;

	/*
	 * wait for carrier 
	 */
	if (!(flag & (FNDELAY|FNONBLOCK))) {
		while ((tp->t_state & CARR_ON) == 0) {
			tp->t_state |= WOPEN;
			(void) LOCK(iasy_lock, plstr);
			if (SV_WAIT_SIG((TP_TO_SV(tp))->iasv_carrier, TTIPRI, 
						iasy_lock) == B_FALSE) {
				if (!(tp->t_state & ISOPEN)) {
					(void) HW_PROC(tp, T_LASTCLOSE);
					(void) HW_PROC(tp, T_DISCONNECT);
					q->q_ptr = NULL;
					WR(q)->q_ptr = NULL;
					tp->t_rdqp = NULL;
					if (tp->t_in.bu_bp) {
						freeb(tp->t_in.bu_bp);
						tp->t_in.bu_bp = 0;
					}
				}
				tp->t_state &= ~WOPEN;
				splx(oldpri);
				return (EINTR);
			}
		}
	}

	tp->t_state &= ~WOPEN;
	tp->t_state |= ISOPEN;

	/*
	 * switch on put and srv. routines.
	 */
	qprocson(q);

	splx(oldpri);

	return (0);
}


/*
 * STATIC int
 * iasyclose(queue_t *, int, cred_t *)
 *	Close an iasy line
 *
 * Calling/Exit State:
 */
/* ARGSUSED */
STATIC int
iasyclose(queue_t *q, int flag, cred_t *cred_p)
{	
	struct strtty *tp;
	pl_t	oldpri;


	tp = Q_TO_TP(q);

	oldpri = SPL();

	/*
	 * Drain queued output to the user's terminal only if FNONBLOCK
	 * or FNDELAY flag not set. 
	 */
	if (!(flag & (FNDELAY|FNONBLOCK))) {
		while (tp->t_state & CARR_ON) {
			TP_TO_TOID(tp) = 0;
			if ((tp->t_out.bu_bp==0) && (WR(q)->q_first==NULL) &&
			    !HW_PROC(tp, T_DATAPRESENT))
				break;
			tp->t_state |= TTIOW;
			TP_TO_TOID(tp) = timeout((void(*)())iasy_drain, 
						 (caddr_t)tp, CL_TIME);
			(void) LOCK(iasy_lock, plstr);
			if (SV_WAIT_SIG((TP_TO_SV(tp))->iasv_drain, TTOPRI, 
						iasy_lock) == B_FALSE) {
				tp->t_state &= ~TTIOW;
				if (TP_TO_TOID(tp)) {
					untimeout(TP_TO_TOID(tp));
					TP_TO_TOID(tp) = 0;
				}
				break;
			}
		}
	}

	ASSERT(WR(q)->q_first == NULL);
	ASSERT(tp->t_state & ISOPEN);

	if (tp->t_cflag & HUPCL) {
		iasy_hup(tp);
		(void) HW_PROC(tp, T_DISCONNECT);
	}

	tp->t_state &= ~(ISOPEN|BUSY|TIMEOUT|TTSTOP);
	iasyflush(WR(q), FLUSHR);

	/*
	 * switch off put and srv routines.
	 */
	qprocsoff(q);

	(void) HW_PROC(tp, T_LASTCLOSE);

	if (tp->t_in.bu_bp) {
		freeb((mblk_t *)tp->t_in.bu_bp);	
		tp->t_in.bu_bp  = 0;
		tp->t_in.bu_ptr = 0;
		tp->t_in.bu_cnt = 0;
	}

	if (tp->t_out.bu_bp) {
		freeb((mblk_t *)tp->t_out.bu_bp);	
		tp->t_out.bu_bp  = 0;
		tp->t_out.bu_ptr = 0;
		tp->t_out.bu_cnt = 0;
	}

	tp->t_rdqp = NULL;
	tp->t_iflag = 0;
	tp->t_oflag = 0;
	tp->t_cflag = 0;
	tp->t_lflag = 0;

	q->q_ptr = WR(q)->q_ptr = NULL;

	splx(oldpri);

	return (0);
}


/*
 * STATIC void
 * iasydelay(struct strtty *)
 *	Resume output after a delay
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
iasydelay(struct strtty *tp)
{	
	pl_t	s;


	s = SPL();

	tp->t_state &= ~TIMEOUT;
	(void) HW_PROC(tp, T_OUTPUT);

	splx(s);
}


/*
 * STATIC void
 * iasyputioc(queue_t *, mblk_t *)
 *	ioctl handler for output PUT procedure
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
iasyputioc(queue_t *q, mblk_t *bp)
{	
	struct strtty *tp;
	struct iocblk *iocbp;
	mblk_t *bp1;

	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_rptr;

	tp = Q_TO_TP(q);

	switch (iocbp->ioc_cmd) {
	case SETRTRLVL:
	case TCSETSW:
	case TCSETSF:
	case TCSETAW:
	case TCSETAF:
	case TCSBRK:	/* run these now, if possible */
		if (q->q_first != NULL || 
		    (tp->t_state & (BUSY|TIMEOUT|TTSTOP)) ||
		    HW_PROC(tp, T_DATAPRESENT)) {
			/* queue ioctl behind output */
			(void) putq(q, bp);
			break;
		}
		
		/* No output, do it now */
		iasysrvioc(q, bp);
		break;

	case TCSETA:	/* immediate parm set */
	case TCSETS:
	case TCGETA:
	case TCGETS:	/* immediate parm retrieve */
		iasysrvioc(q, bp);		/* Do these anytime */
		break;

	case TIOCSTI: { /* Simulate typing of a character at the terminal. */
		mblk_t *mp;

		/*
		 * The permission checking has already been done at the stream
		 * head, since it has to be done in the context of the process
		 * doing the call.
		 */
		if ((mp = allocb(1, BPRI_MED)) != NULL) {
			if (!canputnext(RD(q)))
				freemsg(mp);
			else {
				*mp->b_wptr++ = *bp->b_cont->b_rptr;
				putq(tp->t_rdqp, mp);
			}
		}

		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		putnext(RD(q), bp);
		break;
	}

	case EUC_MSAVE:
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	case TIOCMGET:
	case TIOCMSET:
		(*(TP_TO_HW(tp)->hwdep))(q, bp, tp);
		break;

	default:
		if ((iocbp->ioc_cmd & IOCTYPE) != LDIOC) {
			/* Handle in service routine. */
			putq(q, bp);
			return;
		}

		/* ignore LDIOC cmds */
		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		qreply(q, bp);
		break;
	}
}


/*
 * STATIC int
 * iasyoput(queue_t *, mblk_t *)
 *	A message has arrived for the output q
 *
 * Calling/Exit State:
 *	- Return 0 on success.
 */
STATIC int
iasyoput(queue_t *q, mblk_t *bp)
{	
	mblk_t	*bp1;
	struct strtty *tp;
	pl_t	s;


	tp = Q_TO_TP(q);

	s = SPL();

	switch (bp->b_datap->db_type) {
	case M_DATA:
		while (bp) {		/* Normalize the messages */
			bp->b_datap->db_type = M_DATA;
			bp1 = unlinkb(bp);
			bp->b_cont = NULL;
			if ((bp->b_wptr - bp->b_rptr) <= 0) {
				freeb(bp);
			} else {
				(void) putq(q, bp);
			}
			bp = bp1;
		}

		(void) HW_PROC(tp, T_OUTPUT);	/* Start output */
		break;

	case M_IOCTL:
		iasyputioc(q, bp);		/* Queue it or do it */
		(void) HW_PROC(tp, T_OUTPUT);	/* just in case */
		break;

	case M_FLUSH:
#if FLUSHRW != (FLUSHR|FLUSHW)
		/*
		 *+ Incorrect M_FLUSH implementation assumption.
		 */
		cmn_err(CE_PANIC, 
			"iasyoput: implementation assumption botched\n");
#endif
		switch (*(bp->b_rptr)) {
		case FLUSHRW:
			iasyflush(q, (FLUSHR|FLUSHW));
			freemsg(bp);	/* iasyflush has done FLUSHR */
			break;
		case FLUSHR:
			iasyflush(q, FLUSHR);
			freemsg(bp);	/* iasyflush has done FLUSHR */
			break;
		case FLUSHW:
			iasyflush(q, FLUSHW);
			freemsg(bp);
			break;
		default:
			freemsg(bp);
			break;
		}
		break;

	case M_START:
		(void) HW_PROC(tp, T_RESUME);
		freemsg(bp);
		break;

	case M_STOP:
		(void) HW_PROC(tp, T_SUSPEND);
		freemsg(bp);
		break;

	case M_BREAK:
		if (q->q_first != NULL || (tp->t_state & BUSY)) {
			/* Device busy, queue for later */
			(void) putq(q, bp);
			break;
		}
		(void) HW_PROC(tp, T_BREAK);	/* Do break now */
		freemsg(bp);
		break;

	case M_DELAY:
		tp->t_state |= TIMEOUT;
		(void) timeout((void(*)())iasydelay, (caddr_t)tp, 
					(int)*(bp->b_rptr));
		freemsg (bp);
		break;

	case M_STARTI:
		(void) HW_PROC(tp, T_UNBLOCK);
		freemsg(bp);
		break;

	case M_STOPI:
		(void) HW_PROC(tp, T_BLOCK);
		freemsg(bp);
		break;

       case M_CTL: {
		struct termios *termp;
		struct iocblk *iocbp;

		if ((bp->b_wptr - bp->b_rptr) != sizeof(struct iocblk)) {
			freeb(bp);
			break;
		}

		/* LINTED pointer alignment */
		iocbp = (struct iocblk *)bp->b_rptr;

		if (iocbp->ioc_cmd  == MC_CANONQUERY ) {
			if ((bp1 = allocb(sizeof(struct termios), BPRI_HI)) == 
						(mblk_t *) NULL) {
				freeb(bp);
				break;
			}

			bp->b_datap->db_type = M_CTL;
			iocbp->ioc_cmd = MC_PART_CANON;
			bp->b_cont = bp1;
			bp1->b_wptr += sizeof(struct termios);
			/* LINTED pointer alignment */
			termp = (struct termios *)bp1->b_rptr;
			termp->c_iflag = ISTRIP | IXON | IXANY;
			termp->c_cflag = 0;
			termp->c_oflag = 0;
			termp->c_lflag = 0;
			qreply(q, bp);
		} else
			freemsg(bp);

		break;
	}

	case M_IOCDATA:
		/*
		 * HW dep ioctl data has arrived 
		 */
		(*(TP_TO_HW(tp)->hwdep))(q, bp, tp);
		break;

	default:
		freemsg(bp);
		break;
	}

	splx(s);

	return (0);
}


/*
 * STATIC mblk_t *
 * iasygetoblk(struct queue *)
 *
 * Calling/Exit State:
 *	Return the next data block -- if none, return NULL
 */
STATIC mblk_t *
iasygetoblk(struct queue *q)
{	
	struct strtty *tp;
	pl_t	s;
	mblk_t	*bp;

	s = SPL();

	tp = Q_TO_TP(q);
	if (!tp) {	/* This can happen only if closed while no carrier */
		splx(s);
		return (0);
	}

	while ((bp = getq(q)) != NULL) {
		/*
		 * wakeup close write queue drain 
		 */
		switch (bp->b_datap->db_type) {
		case M_DATA:
			splx(s);
			return(bp);

		case M_IOCTL:
			if (!(tp->t_state & (TTSTOP|BUSY|TIMEOUT)) &&
			    !HW_PROC(tp, T_DATAPRESENT)) {
				/* Do ioctl, then return output */
				iasysrvioc(q, bp);
			} else {
				(void)putbq(q, bp);
				splx(s);
				return (0);
			}

			break;

		case M_BREAK:
			if (!(tp->t_state & (TTSTOP|BUSY|TIMEOUT))) {
				/* Do break now */
				(void) HW_PROC(tp, T_BREAK);
				freemsg(bp);
			} else {
				(void)putbq(q, bp);
				splx(s);
				return (0);
			}

			break;

		default:
			/* Ignore junk mail */
			freemsg(bp);
			break;
		} /* end switch */

	} /* part of while loop AMS */

	splx(s);

	return (0);
}


/*
 * STATIC void 
 * iasysrvioc(queue_t *, mblk_t *)
 *	Routine to execute ioctl messages.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
iasysrvioc(queue_t *q, mblk_t *bp)
{	
	struct strtty *tp;
	struct iocblk *iocbp;
	int	arg;
	pl_t	s;
	mblk_t	*bpr;
	mblk_t	*bp1;
	int	return_val;


	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_rptr;
	tp = Q_TO_TP(q);

	switch (iocbp->ioc_cmd) {
	/* The output has drained now. */
	case TCSETAF:
		iasyflush(q, FLUSHR);
		/* FALLTHROUGH */

	case TCSETA:
	case TCSETAW: {
		struct termio *cb;

		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		/* LINTED pointer alignment */
		cb = (struct termio *)bp->b_cont->b_rptr;
		tp->t_cflag = (tp->t_cflag & 0xffff0000 | cb->c_cflag);
		tp->t_iflag = (tp->t_iflag & 0xffff0000 | cb->c_iflag);
		bcopy((caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCC);

		s = SPL();

		if (HW_PROC(tp, T_PARM)) {
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		splx(s);

		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	}

	case TCSETSF:
		iasyflush(q, FLUSHR);
		/* FALLTHROUGH */

	case TCSETS:
	case TCSETSW: {
		struct termios *cb;

		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		/* LINTED pointer alignment */
		cb = (struct termios *)bp->b_cont->b_rptr;

		tp->t_cflag = cb->c_cflag;
		tp->t_iflag = cb->c_iflag;
		bcopy((caddr_t)cb->c_cc, (caddr_t)tp->t_cc, NCCS);

		s = SPL();

		if (HW_PROC(tp, T_PARM)) {
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		splx(s);

		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	}

	case TCGETA: {	/* immediate parm retrieve */
		struct termio *cb;

		if (bp->b_cont)	/* Bad user supplied parameter */
			freemsg(bp->b_cont);

		if ((bpr = allocb(sizeof(struct termio), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void) putbq(q, bp);
			(void) bufcall((ushort)sizeof(struct termio),
						BPRI_MED, iasydelay, (long)tp);
			return;
		}

		bp->b_cont = bpr;
		/* LINTED pointer alignment */
		cb = (struct termio *)bp->b_cont->b_rptr;
		cb->c_iflag = (ushort)tp->t_iflag;
		cb->c_cflag = (ushort)tp->t_cflag;
		cb->c_cflag &= ~CIBAUD;
		cb->c_cflag |= (tp->t_cflag&CBAUD)<<IBSHIFT;
		bcopy ((caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCC);
		bp->b_cont->b_wptr += sizeof(struct termio);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termio);
		(void) putnext(RD(q), bp);
		break;

	}

	case TCGETS: {	/* immediate parm retrieve */
		struct termios *cb;

		if (bp->b_cont)	/* Bad user supplied parameter */
			freemsg(bp->b_cont);

		if ((bpr = allocb(sizeof(struct termios), BPRI_MED)) == NULL) {
			ASSERT(bp->b_next == NULL);
			(void) putbq(q, bp);
			(void) bufcall((ushort)sizeof(struct termios),
						BPRI_MED, iasydelay, (long)tp);
			return;
		}

		bp->b_cont = bpr;
		/* LINTED pointer alignment */
		cb = (struct termios *)bp->b_cont->b_rptr;
		cb->c_iflag = tp->t_iflag;
		cb->c_cflag = tp->t_cflag;
		cb->c_cflag &= ~CIBAUD;
		cb->c_cflag |= (tp->t_cflag&CBAUD)<<IBSHIFT;
		bcopy((caddr_t)tp->t_cc, (caddr_t)cb->c_cc, NCCS);
		bp->b_cont->b_wptr += sizeof(struct termios);
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = sizeof(struct termios);
		(void) putnext(RD(q), bp);
		break;
	}

	case TCSBRK:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}
		
		/* LINTED pointer alignment */
		arg = *(int *)bp->b_cont->b_rptr;
		if (arg == 0) {
			s = SPL();
			(void) HW_PROC(tp, T_BREAK);
			splx(s);
		}

		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	case SETRTRLVL:
		if (!bp->b_cont) {
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		s = SPL();

		/* LINTED pointer alignment */
		switch (*(int *)bp->b_cont->b_rptr) {
		case T_TRLVL1: 
			return_val = HW_PROC(tp, T_TRLVL1);
			break;
		case T_TRLVL2: 
			return_val = HW_PROC(tp, T_TRLVL2);
			break;
		case T_TRLVL3: 
			return_val = HW_PROC(tp, T_TRLVL3);
			break;
		case T_TRLVL4: 
			return_val = HW_PROC(tp, T_TRLVL4);
			break;
		default:
			return_val = 1;
			break;
		}

		if (return_val) {
			splx(s);
			iocbp->ioc_error = EINVAL;
			bp->b_datap->db_type = M_IOCNAK;
			iocbp->ioc_count = 0;
			(void) putnext(RD(q), bp);
			break;
		}

		splx(s);

		bp->b_datap->db_type = M_IOCACK;
		bp1 = unlinkb(bp);
		if (bp1)
			freeb(bp1);
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	case EUC_MSAVE:	/* put these here just in case... */
	case EUC_MREST:
	case EUC_IXLOFF:
	case EUC_IXLON:
	case EUC_OXLOFF:
	case EUC_OXLON:
		bp->b_datap->db_type = M_IOCACK;
		iocbp->ioc_count = 0;
		(void) putnext(RD(q), bp);
		break;

	default:
		/*
		 * Unknown ioctls are either intended for the hardware 
		 * dependent code or an upstream module that is not 
		 * present. Pass the request to the HW dependent code
		 * to handle it.
		 */
		(*(TP_TO_HW(tp)->hwdep))(q, bp, tp);
		break;
	}
}


/*
 * STATIC void 
 * iasyflush(queue_t *, int) 
 *	Flush input and/or output queues
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
iasyflush(queue_t *q, int cmd)
{	
	struct strtty *tp;
	pl_t	s;


	s = SPL();

	tp = Q_TO_TP(q);

	if (cmd & FLUSHW) {
		flushq(q, FLUSHDATA);
		(void) HW_PROC(tp, T_WFLUSH);
	}

	if (cmd & FLUSHR) {
		q = RD(q);
		(void) HW_PROC(tp, T_RFLUSH);
		flushq(q, FLUSHDATA);
		(void) putnextctl1(q, M_FLUSH, FLUSHR);
	}

	splx(s);
}


/*
 * STATIC int
 * iasyisrv(queue_t *)
 *	New service procedure. Pass everything upstream.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
iasyisrv(queue_t *q)
{	
	mblk_t	*mp;
	struct strtty *tp;
	pl_t	s;


	tp = Q_TO_TP(q);

	s = SPL();

	while ((mp = getq(q)) != NULL) {
		/*
		 * If we can't put, then put it back if it's not
		 * a priority message.  Priority messages go up
		 * whether the queue is "full" or not.  This should
		 * allow an interrupt in, even if the queue is hopelessly
		 * backed up.
		 */
		if (!canputnext(q)) {
			(void) putbq(q, mp);
			splx(s);
			return (0);
		}

		(void) putnext(q, mp);

		if (tp->t_state & TBLOCK) {
			(void) HW_PROC(tp, T_UNBLOCK);
		}
	}

	splx(s);

	return (0);
}


/*
 * STATIC int
 * iasyosrv(queue_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
iasyosrv(queue_t *q)
{
	return (0);
}


/*
 * int
 * iasy_input(struct strtty *, unsigned int)
 *	Modify your interrupt thread to use this routine instead of l_input. It
 *	takes the data from tp->t_in, ships it upstream to the line discipline,
 *	and allocates another buffer for tp->t_in.
 *
 * Calling/Exit State:
 *	- Return 0, if there are no character in the strtty input buffers.
 *	- Return 1, if there is no upstream queue or no memory.
 *	- Return cnt, if there are cnt character in the strtty input buffer. 
 *
 * Description:
 *	Device with input to report 
 *	BUF or L_BREAK 
 */
int
iasy_input(struct strtty *tp, unsigned int cmd)
{	
	queue_t	*q;
	mblk_t	*bp;
	int	cnt;


	q = TP_TO_Q(tp);
	if (!q)
		return(1);

	switch (cmd) {
	case L_BUF:
		cnt = IASY_BUFSZ - tp->t_in.bu_cnt;
		if (cnt && canputnext(q)) {
			bp = allocb(IASY_BUFSZ, BPRI_MED);
			if (bp) {	/* pass up old bp contents */
				tp->t_in.bu_bp->b_wptr += cnt;
				tp->t_in.bu_bp->b_datap->db_type = M_DATA;
				(void) putnext(q, tp->t_in.bu_bp);
				tp->t_in.bu_bp = bp;
				/* Reset to go again */
				tp->t_in.bu_cnt = IASY_BUFSZ;
				tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
			} else
				return (1);
		} else
			return (cnt);

		return (0);

	case L_BREAK:
		/* signal "break detected" */
		(void) putnextctl(q, M_BREAK);
		break;

	default:
		/*
		 *+ An unknown command is received.
		 */
		cmn_err(CE_WARN, 
			"iasy_input: unknown command\n");
	}
	
	return (0);
}


/*
 * int
 * iasy_output(struct strtty *)
 *	Modify your interrupt thread to use this routine instead of l_output.
 *	It retrieves the next output block from the stream and hooks it into
 *	tp->t_out.
 *
 * Calling/Exit State:
 *	Device desiring to get more output 
 */
int
iasy_output(struct strtty *tp)
{	
	queue_t	*q;
	mblk_t	*bp;


	if (tp->t_out.bu_bp) {
		/* As stashed by previous call */
		freeb((mblk_t *)tp->t_out.bu_bp);
		tp->t_out.bu_bp = 0;
		tp->t_out.bu_ptr = 0;
		tp->t_out.bu_cnt = 0;
	}

	q = TP_TO_Q(tp);
	if (!q)
		return(0);
	q = WR(q);
	bp = iasygetoblk(q);
	if (bp) {
		/*
		 * Our put procedure insures each message consists of one
		 * block. Give the block to the user.
		 */
		tp->t_out.bu_ptr = bp->b_rptr;
		tp->t_out.bu_cnt = bp->b_wptr - bp->b_rptr;
		tp->t_out.bu_bp = bp;
		return(CPRES);
	}

	if ((q->q_first == NULL) && (tp->t_state & TTIOW)) {
		tp->t_state &= ~TTIOW;
		if (TP_TO_TOID(tp)) {
			untimeout(TP_TO_TOID(tp));
			TP_TO_TOID(tp) = 0;
		}
		SV_SIGNAL((TP_TO_SV(tp))->iasv_drain, 0);
	}

	return(0);
}


/*
 * int
 * iasy_register(minor_t, int, int (*)(), void (*)(), int (*)(), void (*)(),
 *		 conssw_t *);
 *
 * Calling/Exit State:
 *	- fmin		Starting minor number 
 *	- count		Number of minor numbers requested 
 *	- (*proc)()	proc routine 
 *	- (*hwdep)()	Hardware dependant ioctl routine 
 *	- cswp		ptr to hardware dependent init/put/get/suspend/resume
 *			console routines.
 *
 * Description:
 *	Register a terminal server.  This makes an interrupt thread
 *	available via the iasy major number.
 */
int
iasy_register(minor_t fmin, int count, int (*proc)(), void (*hwdep)(), 
	      conssw_t *cnswp)
{	
	struct iasy_hw *hp;
	minor_t	i;
	minor_t	lmin;


	fmin = IASY_MINOR_TO_UNIT(fmin);

	if (count <= 0)
		return (-1);

	if (fmin + count > iasy_num) {
		/*
		 *+ The minor number for the serial port device
		 *+ is not within the range of the allowed
		 *+ minor numbers.
		 */
		cmn_err(CE_WARN,
			"iasy_register: minor %d is out of range\n", fmin+count);
		return (-1);
	}

	lmin = fmin + count - 1;

	/*
	 * Scan for allocation problems
	 */
	hp = iasy_hw + fmin;
	for (i = fmin; i <= lmin; i++, hp++) {
		if (hp->proc) {
			/*
			 *+ The minor number range for the serial port device
			 *+ conflicts with another device.
			 */
			cmn_err(CE_WARN,
				"iasy_register: minor %d conflict  0x%x vs 0x%x\n",
				i, hp->proc, proc);
			return (-1);
		}
	}

	/*
	 * Allocate the range of minor numbers
	 * and initialize hw-dependent structures.
	 */
	hp = iasy_hw + fmin;
	for (i = fmin; i <= lmin; i++, hp++) {
		hp->proc = proc;
		hp->hwdep = hwdep;
		hp->consswp = cnswp;
	}

	if (lmin + 1 > iasy_cnt)
		iasy_cnt = lmin + 1;

	return(fmin);
}

/*
 * int
 * iasy_deregister(minor_t, int, int (*)())
 *
 * Calling/Exit State:
 *	- fmin		Starting minor number 
 *	- count		Number of minor devices requested for 
 *			deregister.
 *
 * Description:
 *	Deregister a terminal server.
 */
int
iasy_deregister(minor_t fmin, int count)
{	
	struct iasy_hw *hp;
	minor_t	i;
	minor_t	lmin;


	fmin = IASY_MINOR_TO_UNIT(fmin);

	if (count <= 0)
		return (-1);

	if (fmin + count > iasy_num) {
		/*
		 *+ Validate the minor number for the serial port 
		 *+ device, is it within the range of the allowed
		 *+ minor numbers ?
		 */
		cmn_err(CE_WARN,
			"iasy_deregister: minor %d is out of range\n", fmin+count);
		return (-1);
	}

	/*
	 * De-allocate the range of minor numbers
	 * and clear hw-dependent structures.
	 */

	lmin = fmin + count - 1;
	hp = iasy_hw + fmin;

	for (i = fmin; i <= lmin; i++, hp++) {
		hp->proc = 0;
		hp->hwdep = 0;
		hp->consswp = 0;
	}
	iasy_cnt = 0;
	return(0);
}


/*
 * void
 * iasyhwdep(queue_t *, mblk_t *, struct strtty *)
 *	Default Hardware dependent ioctl support (i.e. none).
 *	Use this routine as your hwdep() routine if you don't have any
 *	special ioctls to implement.
 *
 * Calling/Exit State:
 *	- q points to the write queue pointer.
 *	- bp contains the ioctl or ioctl data not understood by the DI code.
 */
/* ARGSUSED */
void
iasyhwdep(queue_t *q, mblk_t *bp, struct strtty *tp)
{	
	struct iocblk *ioc;


	/* LINTED pointer alignment */
	ioc = (struct iocblk *)bp->b_rptr;

	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		ioc->ioc_error = EINVAL;	/* NAK unknown ioctls */
		ioc->ioc_rval = -1;
		bp->b_datap->db_type = M_IOCNAK;
		qreply(q, bp);
		return;

	default:
		/*
		 *+ An unknown message type.
		 */
		cmn_err(CE_PANIC, 
			"iasyhwdep: illegal message type");
	}
}


/*
 * void
 * iasy_hup(struct strtty *)
 *	Send a hangup upstream to indicate loss of the connection.
 *
 * Calling/Exit State:
 *	None.
 */
void
iasy_hup(struct strtty *tp)
{	
	queue_t *q;
	pl_t	s;


	q = TP_TO_Q(tp);
	if (!q)
		return;

	s = SPL();

	iasyflush(WR(q), FLUSHR|FLUSHW);
	(void) putnextctl(q, M_HANGUP);

	splx(s);
}


/*
 * void
 * iasy_carrier(struct strtty *)
 *	wakeup/signal a process to indicate the establishment of connection.
 *
 * Calling/Exit State:
 *	None.
 */
void
iasy_carrier(struct strtty *tp)
{	
	SV_SIGNAL((TP_TO_SV(tp))->iasv_carrier, 0);
}


/*
 * STATIC void
 * iasyparam(struct strtty *)
 * 	Do parameter settings now.
 *
 * Calling/Exit State:
 *	- Called after timeout has expired.
 *	- Called at plstr.
 */
STATIC void
iasyparam(struct strtty *tp)
{	
	ASSERT(getpl() >= plstr);
	tp->t_state &= ~TIMEOUT;
	(void) HW_PROC(tp, T_PARM);
}


/*
 * int
 * iasy_ctime(struct strtty *, int)
 *	Delay "count" character times to allow for devices which prematurely
 *	clear BUSY.
 *
 * Calling/Exit State:
 *	- Return 0 on success.
 */
int
iasy_ctime(struct strtty *tp, int count)
{	
	pl_t	oldpri;
	static int rate[] = {
		HZ+1,	/* avoid divide-by-zero, as well as unnecessary delay */
		50,
		75,
		110,
		134,
		150,
		200,
		300,
		600,
		1200,
		1800,
		2400,
		4800,
		9600,
		19200,
		38400,
	};


	/*
	 * Delay 11 bit times to allow uart to empty.
	 * Add one to allow for truncation and one to
	 * allow for partial clock tick.
	 */
	count *= 1 + 1 + 11 * HZ / rate[tp->t_cflag & CBAUD];

	oldpri = SPL();

	tp->t_state |= TIMEOUT;
	(void) timeout((void(*)())iasyparam, (caddr_t)tp, count );

	splx(oldpri);

	return (0);
}


/*
 * STATIC dev_t
 * iasycnopen(minor_t, boolean_t, const char *)
 *
 * Calling/Exit State:
 */
STATIC dev_t
iasycnopen(minor_t minor, boolean_t syscon, const char *params)
{
	struct iasy_hw *hp;

	if (!iasy_initialized)
		iasyinit();

	hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];
	if (IASY_MINOR_TO_UNIT(minor) > iasy_cnt || hp->consswp == NULL)
		return(NODEV);

	return((*hp->consswp->cn_open)(minor, syscon, params));
}


/*
 * STATIC void
 * iasycnclose(minor_t, boolean_t)
 *
 * Calling/Exit State:
 */
STATIC void
iasycnclose(minor_t minor, boolean_t syscon)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	(*hp->consswp->cn_close)(minor, syscon);
}

		
/*
 * STATIC int
 * iasycngetc(minor_t)
 * 
 * Calling/Exit State:
 *	- Return -1, if its an illegal minor number, otherwise
 *	  return the character.
 */
STATIC int
iasycngetc(minor_t minor)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	return((*hp->consswp->cn_getc)(minor));
}


/*
 * STATIC int 
 * iasycnputc(minor_t, int)
 *
 * Calling/Exit State:
 *	- Return 1, if the character is displayed.
 */
STATIC int
iasycnputc(minor_t minor, int c)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	return((*hp->consswp->cn_putc)(minor, c));
}


/*
 * STATIC void
 * iasycnsuspend(minor_t)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
iasycnsuspend(minor_t minor)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	(*hp->consswp->cn_suspend)(minor);
}


/*
 * STATIC void
 * iasycnresume(minor_t)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
iasycnresume(minor_t minor)
{
	struct iasy_hw *hp = &iasy_hw[IASY_MINOR_TO_UNIT(minor)];

	ASSERT(IASY_MINOR_TO_UNIT(minor) < iasy_cnt);
	ASSERT(hp->consswp != NULL);

	(*hp->consswp->cn_resume)(minor);
}


#ifdef DEBUG
/*
 * void
 * strtty_dump(struct strtty *)
 *	Formatted print of strtty structure. Can be invoked from
 *	kernel debugger.
 *
 * Calling/Exit State:
 *	None.
 */
void
strtty_dump(struct strtty *tp)
{
	cmn_err(CE_CONT, "\n STREAM strtty struct: size=0x%x(%d)\n",
		sizeof(struct strtty), sizeof(struct strtty));
	cmn_err(CE_CONT, "\tt_in.bu_bp=0x%x, \tt_in.bu_ptr=0x%x\n",
		tp->t_in.bu_bp, tp->t_in.bu_ptr);
	cmn_err(CE_CONT, "\tt_in.bu_cnt=0x%x, \tt_out.bu_bp=0x%x\n",
		tp->t_in.bu_cnt, tp->t_out.bu_bp);
	cmn_err(CE_CONT, "\tt_out.bu_ptr=0x%x, \tt_out.bu_cnt=0x%x\n",
		tp->t_out.bu_ptr, tp->t_out.bu_cnt);
	cmn_err(CE_CONT, "\tt_rdqp=0x%x, \tt_ioctlp=0x%x\n",
		tp->t_rdqp, tp->t_ioctlp);
	cmn_err(CE_CONT, "\tt_lbuf=0x%x, \tt_dev=0x%x\n",
		tp->t_lbuf, tp->t_dev);
	cmn_err(CE_CONT, "\tt_iflag=0x%x, \tt_oflag=0x%x\n",
		tp->t_iflag, tp->t_oflag);
	cmn_err(CE_CONT, "\tt_cflag=0x%x, \tt_lflag=0x%x\n",
		tp->t_cflag, tp->t_lflag);
	cmn_err(CE_CONT, "\tt_state=0x%x, \tt_line=0x%x\n",
		tp->t_state, tp->t_line);
	cmn_err(CE_CONT, "\tt_dstat=0x%x, \tt_cc=%19x\n",
		tp->t_dstat, tp->t_cc);
}
#endif /* DEBUG */
