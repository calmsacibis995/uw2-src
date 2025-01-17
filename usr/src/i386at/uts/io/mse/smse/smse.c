/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/mse/smse/smse.c	1.13"
#ident	"$Header: $"

/*
 * Serial Mouse Module - Streams
 */

#include <util/param.h>
#include <util/types.h>
#include <mem/kmem.h>
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
#include <util/cmn_err.h>
#include <io/ws/chan.h>
#include <io/mouse.h>
#include <io/mse/mse.h>
#include <io/asy/iasy.h>
#ifndef ESMP
#include <io/asy/asy.h>
#endif
#include <io/ddi.h>
#ifdef ESMP
#include <util/ksynch.h>
#endif

#include <util/mod/moddefs.h>

STATIC int smse_load(), smse_unload();

MOD_STR_WRAPPER(smse, smse_load, smse_unload, "smse - serial mouse driver");

STATIC int
smse_load(void)
{
	/* Module specific load processing... */
	cmn_err(CE_NOTE, "!MOD: in smse_load()");
	return(0);
}


STATIC int
smse_unload(void)
{
	/* Module specific unload processing... */
	cmn_err(CE_NOTE, "!MOD: in smse_unload()");
	return(0);
}


#ifdef ESMP
#define	SMSEHIER		1
#define	SMSEPL			plstr
#endif

#ifndef TRUE
#define TRUE			1
#define FALSE			0
#endif

/*
 * States while parsing M+ format from Microsoft compatible serial mice
 */
#define WAIT_FOR_START_BYTE	0
#define WAIT_FOR_X_DELTA	1
#define WAIT_FOR_Y_DELTA	2
#define WAIT_FOR_MIDDLE_BUTTON	3	/* for Logitech 3-button */

/*
 * Additional states to parse the 5 byte binary MSC protocol.
 */
#define WAIT_FOR_X0_DELTA	4
#define WAIT_FOR_Y0_DELTA	5
#define WAIT_FOR_X1_DELTA	6
#define WAIT_FOR_Y1_DELTA	7

/*
 * States while parsing configuration information
 */
#define START_CONFIG		4
#define MM_PROTOCOL		5
#define M_PLUS_PROTOCOL		6

#ifdef DEBUG
STATIC int smse_debug = 0;
#define DEBUG1(a)	if (smse_debug == 1) printf a
#define DEBUG2(a)	if (smse_debug >= 2) printf a /* allocations */
#define DEBUG3(a)	if (smse_debug >= 3) printf a /* M_CTL Stuff */
#else
#define DEBUG1(a)
#define DEBUG2(a)
#define DEBUG3(a)
#endif /* DEBUG */


extern void	mse_iocack(queue_t *, mblk_t *, struct iocblk *, int);
extern void	mse_iocnack(queue_t *, mblk_t *, struct iocblk *, int, int);
extern void	mse_copyout(queue_t *, register mblk_t *, register mblk_t *,
	                uint, unsigned long);
extern void	mse_copyin(queue_t *, register mblk_t *, int, unsigned long);
extern void	mseproc(struct strmseinfo *);

#ifdef ESMP
int		smsestart(void);
#endif

STATIC int	smseopen(queue_t *, dev_t *, register int, 
			register int, struct cred *); 
STATIC int	smseclose(queue_t *, register int, struct cred *);
STATIC int	smse_rput(queue_t *, mblk_t *);
STATIC int	smse_wput(queue_t *, mblk_t *);
STATIC int	smse_srvp(queue_t *);
STATIC void	smse_MM_parse(queue_t *, mblk_t *);
STATIC void	smse_M_plus_parse(queue_t *, mblk_t *);
STATIC void	smse_MSC_parse(queue_t *, mblk_t *);
STATIC void	smse_config_parse(queue_t *, mblk_t *);
STATIC void	smse_config_port(struct strmseinfo *);
#ifdef ESMP
STATIC void	smse_wakeup(void);
#else
extern void	wakeup(caddr_t);
#endif

struct module_info smse_info = {
	24, "smse", 0, INFPSZ, 256, 128
};

static struct qinit smse_rinit = {
	smse_rput, smse_srvp, smseopen, smseclose, NULL, &smse_info, NULL
};

static struct qinit smse_winit = {
	smse_wput, NULL, NULL, NULL, NULL, &smse_info, NULL
};

struct streamtab smseinfo = {
	&smse_rinit, &smse_winit, NULL, NULL
};

/* 
 * Extern variable in space.c to reflect the type of mice 
 * selected, this variable is used to configure parse function.
 * The variable in space.c is smse_MSC_selected, to select 
 * Microsoft non-programmable this is initialised to zero, and one
 * for Mouse Systems Corpn (MSC) 5 byte binary protocol mice.
 */
extern 	int smse_MSC_selected;

int		smsedevflag = 0;
boolean_t	first_button;	/* Drop first button click: Since the types
		                 * of MSC compatible mice are of two types,
                                 * with switch and without switch. The ones
                                 * without switch need the button to be dep-
                                 * ressed to switch modes. If the first button
                                 * change is not dropped, the button release
                                 * would be considered as a button click and
                                 * wouldn't allow the user to test the mouse.
			         * So by dropping this first button click, the
                                 * MSC compatible mice without switches would
 				 * also be supported any problems.
                                 */

#ifdef ESMP
lock_t		*smse_lock;
sv_t		*smse_bufcallsv;

LKINFO_DECL(smse_lkinfo, "MSE:SMSE:smse mutex lock", 0);


/*
 * int 
 * smsestart(void)
 * 
 * Calling/Exit State:
 *	None.
 */
int
smsestart(void)
{
	smse_lock = LOCK_ALLOC(SMSEHIER, SMSEPL, &smse_lkinfo, KM_SLEEP);
	smse_bufcallsv = SV_ALLOC(KM_SLEEP);
	return(0);
}
#endif


/*
 * STATIC void
 * smse_config_mouse(struct strmseinfo *)
 *	Configure the mouse
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
smse_config_mouse(struct strmseinfo *msp)
{
	mblk_t	*bp;

	DEBUG1(("smse_config_mouse:entered\n"));

	if ((bp = allocb( sizeof(int), 0)) == NULL)
		return;

	bp->b_datap->db_type = M_DATA;

	*bp->b_wptr++ = 'S';	/* select MM series format */
	*bp->b_wptr++ = 'D';	/* select prompt mode */
	*bp->b_wptr++ = '*';	/* change baud rate */
	*bp->b_wptr++ = 'q';	/* select 9600 baud */

	putnext(msp->wqp, bp);	/* send to serial port driver */

	smse_config_port(msp);

	/* re-configure mouse */

	if ((bp = allocb(sizeof(int), 0)) == NULL)
		return;

	bp->b_datap->db_type = M_DATA;

	*bp->b_wptr++ = 'S';	/* select MM series format */
	*bp->b_wptr++ = 'K';	/* select 20 reports/sec rate */
	*bp->b_wptr++ = 0;	
	*bp->b_wptr++ = 0;

	putnext(msp->wqp, bp);	/* send to serial port driver */

	DEBUG1(("smse_config_mouse:exited\n"));
}


/*
 * STATIC void
 * smse_config_port(struct strmseinfo *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
smse_config_port(struct strmseinfo *msp)
{
	mblk_t		*bp;
	struct iocblk	*iocbp;
	struct termio	*cb;

	DEBUG1(("smse_config_port:entered\n"));

	if ((bp = allocb(sizeof(struct iocblk), 0)) == NULL)
		return;

	bp->b_datap->db_type = M_IOCTL;
	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_rptr;
	bp->b_wptr += sizeof(struct iocblk);
	iocbp->ioc_cmd = TCSETAF;
	iocbp->ioc_count = sizeof(struct termio);

	if ((bp->b_cont = allocb(sizeof(struct termio), 0)) == NULL) {
		freemsg(bp);
		return;
	}

	/* LINTED pointer alignment */
	cb = (struct termio *)bp->b_cont->b_rptr;
	bp->b_cont->b_wptr += sizeof(struct termio);
	cb->c_iflag = IGNBRK | IGNPAR;
	cb->c_oflag = 0;

	/* Set Serial port parameters depending on the type of mice */
	if (msp->smseparse == smse_MM_parse) {
		cb->c_cflag = B9600 | CS8 | CREAD | CLOCAL | PARENB | PARODD;
	} else if (msp->smseparse == smse_MSC_parse) {
		/* Configure port for MSC mice 8 bits/char and No parity */
		cb->c_cflag = B1200 | CS8 | CREAD | CLOCAL;
	} else {
		cb->c_cflag = B1200 | CS7 | CREAD | CLOCAL;
	}

	cb->c_lflag = 0;
	cb->c_line = 0;
	cb->c_cc[VMIN] = 1;
	cb->c_cc[VTIME] = 0;
	putnext(msp->wqp, bp);

	if ((bp = allocb(sizeof(struct iocblk), 0)) == NULL)
		return;

	bp->b_datap->db_type = M_IOCTL;
	/* LINTED pointer alignment */
	iocbp = (struct iocblk *)bp->b_rptr;
	bp->b_wptr += sizeof(struct iocblk);
	iocbp->ioc_cmd = SETRTRLVL;
	iocbp->ioc_count = sizeof(int);

	if ((bp->b_cont = allocb(sizeof(int), 0)) == NULL) {
		freemsg(bp);
		return;
	}

	/* LINTED pointer alignment */
	*(int *)bp->b_cont->b_wptr = T_TRLVL1;
	bp->b_cont->b_wptr += sizeof(int);
	putnext(msp->wqp, bp);

	DEBUG1(("smse_config_port:exited\n"));
}


/*
 * STATIC void
 * smse_default_parse(struct strmseinfo *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
smse_default_parse(struct strmseinfo *msp)
{
	/* User selected protocol MSC/Microsoft, in space.c file */
	if (smse_MSC_selected == 1) {
		msp->smseparse = smse_MSC_parse;
	} else {
		msp->smseparse = smse_M_plus_parse;
	}
	msp->report_not_sent = TRUE;
	msp->middle_button_down = FALSE;
	smse_config_port(msp);
}


/*
 * STATIC int
 * smseopen(queue_t *, dev_t *, register int, register int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
smseopen(queue_t *q, dev_t *devp, register int flag, 
			register int sflag, struct cred *cred_p)
{
	mblk_t			*bp;
	struct strmseinfo	*msp;

	if (q->q_ptr != NULL)
		return EBUSY;

	DEBUG1(("smseopen:entered\n"));

	/* allocate and initialize state structure */

	if ((q->q_ptr = (caddr_t)kmem_zalloc(sizeof(struct strmseinfo),
						KM_SLEEP)) == NULL) {
		/*
		 *+ There is not enough memory available to allocate
		 *+ for strmseinfo structure. Check memory configured
		 *+ in the system.
		 */
		cmn_err(CE_WARN, 
			"SMSE: open fails, can't allocate state structure\n" );
		return (ENOMEM);
	}

	msp = (struct strmseinfo *)q->q_ptr;
	msp->rqp = q;
	msp->wqp = WR(q);
	msp->wqp->q_ptr = q->q_ptr;
	/* Initialize to all buttons up */
	msp->old_buttons = 0x07;

	/*
	 * size is determined by the no. of bytes sent to the
	 * the serial port driver to find the type of serial
	 * mouse configured
	 */
	while ((bp = allocb(5, BPRI_MED)) == NULL) {
#ifdef ESMP   
		(void) bufcall(sizeof(int), BPRI_MED, smse_wakeup, NULL);
		(void) LOCK(smse_lock, plstr);
		/* In SVR4.2 the priority was STIPRI|PCATCH */
		SV_WAIT_SIG(smse_bufcallsv, primed+3, smse_lock);
#else
		(void)bufcall( sizeof( int ), BPRI_MED, wakeup,
				(caddr_t)&q->q_ptr );
		sleep( &q->q_ptr, STIPRI | PCATCH );
#endif
	}

	bp->b_datap->db_type = M_DATA;

	/*
	 * The following bytes are being sent to whatever mouse
	 * happens to be attached to the serial port.  If it is
	 * a Microsoft-compatible Logitech NON-Programmable mouse,
	 * it will echo "*?SDT" back.  If it is a Logitech type C
	 * mouse, it will ignore the "*?" and respond to the "SDt".
	 * If it is a Microsoft-compatible programmable, it will 
	 * respond with 4 bytes of configuration information.
	 */

	*bp->b_wptr++ = '*';	/* query general configuration */
	*bp->b_wptr++ = '?';
	*bp->b_wptr++ = 'S';	/* select MM series format */
	*bp->b_wptr++ = 'D';	/* select prompt mode */
	*bp->b_wptr++ = 't';	/* query format and prompt mode */

	msp->state = START_CONFIG;
	msp->smseparse = smse_config_parse;

	/*
	 * switch on put() and srv() routines
	 */
#ifdef ESMP
	qprocson(msp->rqp);
#endif

	putnext(msp->wqp, bp);	/* send to serial port driver */

	/*
	 * The routine to parse messages from the asy module is
	 * initially set to smse_config_parse().  The timeout()
	 * is used to default to Microsoft compatible mouse in
	 * case a programmable one attached.  smse_config_parse()
	 * is only looking for a response from the Logitech Type C
	 * and the NON-programmable Microsoft-compatible since
	 * they both respond deterministically. Since support for
	 * MSC mice was added recently, the available family of MSC
	 * and Microsoft compatible mice overlay, and the detection
  	 * scheme fails. In order to configure the mice, user input
 	 * is requested, and the appropriate parse function is
 	 * initialized.
	 */

	msp->msetimeid = timeout(smse_default_parse,
					(caddr_t)msp, drv_usectohz(500000));
	DEBUG1(("smseopen:leaving\n"));

	first_button = B_FALSE;
	return(0);
}


/*
 * STATIC int
 * smseclose(queue_t *, register int, struct cred *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC int
smseclose(queue_t *q, register int flag, struct cred *cred_p)
{
	register int		oldpri;
	struct strmseinfo	*msp;
	mblk_t			*bp;


	DEBUG1(("smseclose:entered\n"));

	msp = (struct strmseinfo *) q->q_ptr;

 	untimeout(msp->msetimeid);

	if ((bp = allocb(sizeof(long), BPRI_MED)) == NULL)
		/*
		 *+ Allocation of message block of size int failed. 
		 *+ Check memory configured in the system. The allocation
		 *+ failed while attempting to send a message block to
		 *+ the serial port driver to reset the baud rate.
		 */
		cmn_err(CE_WARN, 
			"smseclose: reset of serial mouse baud rate failed");
	else {	/* reset baud rate to 1200 bps */
		if (msp->smseparse == smse_MM_parse) {
			*bp->b_wptr++ = 'S';	/* select MM series format */
			*bp->b_wptr++ = 'D';	/* select prompt mode */
		}

		*bp->b_wptr++ = '*';	/* change baud rate */
		*bp->b_wptr++ = 'n';	/* select 1200 baud */

		putnext(msp->wqp, bp);
	}

	/*
	 * switch off put() and srv() routines
	 */
#ifdef ESMP
	qprocsoff(msp->rqp);
#endif
	
	oldpri = splstr();
	kmem_free((caddr_t) msp, sizeof(struct strmseinfo));
	q->q_ptr = (caddr_t) NULL;
	WR(q)->q_ptr = (caddr_t) NULL;
	splx(oldpri);

	DEBUG1(("smseclose:leaving\n"));

	return 0;
}


/*
 * STATIC int
 * smse_rput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
smse_rput(queue_t *q, mblk_t *mp)
{
	DEBUG1(("smse_rput:entered\n"));

	switch (mp->b_datap->db_type) {
	case M_DATA:
		/* Queue has been attached but not opened yet */
		if (q->q_ptr == NULL)
			putnext(q, mp);
		else
			putq(q, mp);
		break;

	case M_IOCACK: {
		struct iocblk *iocp;

		/* LINTED pointer alignment */
		iocp = (struct iocblk *)mp->b_rptr;

		if (iocp->ioc_cmd == TCSETAF || 
			    iocp->ioc_cmd == SETRTRLVL)
			freemsg(mp);
		else
			putnext(q, mp);

		break;
	}

	case M_FLUSH:
		if (*mp->b_rptr & FLUSHR)
			flushq(q, FLUSHDATA);

		putnext(q, mp);
		break;

	default:
		putnext(q, mp);
		break;
	}

	DEBUG1(("smse_rput:leaving\n"));
	return(0);
}


/*
 * STATIC int
 * smse_wput(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
smse_wput(queue_t *q, mblk_t *mp)
{
	struct iocblk			*iocbp;
	register struct strmseinfo	*mseptr;
	register mblk_t			*bp;
	register struct copyresp	*csp;
	int				oldpri;


	DEBUG1(("smse_wput:entered\n"));

	mseptr = (struct strmseinfo *)q->q_ptr;
	/* LINTED pointer alignment */
	iocbp = (struct iocblk *) mp->b_rptr;

	switch (mp->b_datap->db_type) {
	case M_FLUSH:
		DEBUG1(("smse_wput:M_FLUSH\n"));

		if (*mp->b_rptr & FLUSHW)
			flushq(q, FLUSHDATA);

		putnext(q, mp);
		break;

	case M_IOCTL:
		DEBUG1(("smse_wput:M_IOCTL\n"));

		switch(iocbp->ioc_cmd) {
		case MOUSEIOCREAD:

			if ((bp = allocb(sizeof(struct mouseinfo), BPRI_MED)) == NULL) {
				mse_iocnack(q, mp, iocbp, EAGAIN, 0);
				break;
			}
			oldpri = splhi();
			bcopy(&mseptr->mseinfo, bp->b_rptr, 
					sizeof(struct mouseinfo));
			mseptr->mseinfo.xmotion = 
					mseptr->mseinfo.ymotion = 0;
			mseptr->mseinfo.status &= BUTSTATMASK;
			bp->b_wptr += sizeof(struct mouseinfo);
			splx(oldpri);

			if (iocbp->ioc_count == TRANSPARENT)
				mse_copyout(q, mp, bp, 
						sizeof(struct mouseinfo), 0);
			else {
				mp->b_datap->db_type = M_IOCACK;
				iocbp->ioc_count = sizeof(struct mouseinfo);
				if (mp->b_cont)
					freemsg(mp->b_cont);
				mp->b_cont = bp;
				qreply(q, mp);
			}
			break;

		default:
			mse_iocnack(q, mp, iocbp, EINVAL, 0);
			break;
		}
		break;

	case M_IOCDATA:
		DEBUG1(("smse_wput:M_IOCDATA\n"));

		/* LINTED pointer alignment */
		csp = (struct copyresp *)mp->b_rptr;

		if (csp->cp_cmd != MOUSEIOCREAD) {
			putnext(q, mp);
			break;
		}
		if (csp->cp_rval) {
			freemsg(mp);
			return(0);
		}
		mse_iocack(q, mp, iocbp, 0);
		break;

	default:
		putnext(q, mp);
		break;
	}
	DEBUG1(("smse_wput:leaving\n"));
	return(0);
}


/*
 * STATIC void
 * smse_M_plus_parse(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
smse_M_plus_parse(queue_t *q, mblk_t *mp)
{
	register mblk_t			*bp;
	register struct strmseinfo	*mseptr;
	register unchar			c;
	register int			lcv;

	/*
	 * Parse the next byte of serial input.
	 * This assumes the input is in M+ format.
	 */
	mseptr = (struct strmseinfo *)q->q_ptr;

	for (bp = mp; bp != (mblk_t *)NULL; bp = bp->b_cont) {
		lcv = bp->b_wptr - bp->b_rptr;

		while (lcv--) {
			c = *bp->b_rptr++;

			if (c & 0x40) {		/* start byte */
				if (mseptr->state == WAIT_FOR_MIDDLE_BUTTON 
						&& mseptr->report_not_sent)
					mseproc(mseptr);

				mseptr->x = (c & 0x03) << 6;	/* MS X bits */
				mseptr->y = (c & 0x0C) << 4;	/* MS Y bits */
				mseptr->state = WAIT_FOR_X_DELTA;

				c = ~c;
				mseptr->button = (c >> 4) & 0x01
						 | (c >> 3) & 0x04
						 | 0x2;
				continue;
			}

			switch (mseptr->state) {
			case WAIT_FOR_X_DELTA:
				mseptr->x |= c & 0x3F;
				mseptr->state = WAIT_FOR_Y_DELTA;
				break;

			case WAIT_FOR_Y_DELTA:
				mseptr->y |= c & 0x3F;
				mseptr->state = WAIT_FOR_MIDDLE_BUTTON;
				mseptr->report_not_sent = TRUE;
				break;

			case WAIT_FOR_MIDDLE_BUTTON:
				c = (c >> 4) & 0x02;
				mseptr->button ^= c;
				mseptr->middle_button_down = c;
				mseptr->state = WAIT_FOR_START_BYTE;
				mseproc(mseptr);
				break;
			}
   		}
	}
	freemsg(mp);
	if (mseptr->state == WAIT_FOR_MIDDLE_BUTTON &&
					!mseptr->middle_button_down) {
		mseproc(mseptr);
		mseptr->x = mseptr->y = 0;
		mseptr->report_not_sent = FALSE;
	}
}

/*
 * STATIC void
 * smse_MSC_parse(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
smse_MSC_parse(queue_t *q, mblk_t *mp)
{
	mblk_t			*bp;
	struct strmseinfo 	*mseptr;
	char			c;
	int			lcv;

	/*
	 * Parse the next byte of serial input.
	 * This assumes the input is in MSC five byte binary format.
	 */
	mseptr = (struct strmseinfo *)q->q_ptr;

	for (bp = mp; bp != (mblk_t *)NULL; bp = bp->b_cont) {
		lcv = bp->b_wptr - bp->b_rptr;

		while (lcv--) {
			c = *bp->b_rptr++;

			switch (mseptr->state) {
				case WAIT_FOR_START_BYTE:
					if (c & 0x80) {	  /* start byte */
						mseptr->button = (c & 0x07);
						mseptr->state=WAIT_FOR_X0_DELTA;
					}
					break;

				case WAIT_FOR_X0_DELTA:
					mseptr->x = c;
					mseptr->state = WAIT_FOR_Y0_DELTA;
					break;

				case WAIT_FOR_Y0_DELTA:
					mseptr->y = - c;
					mseptr->state = WAIT_FOR_X1_DELTA;
					break;

				case WAIT_FOR_X1_DELTA:
					mseptr->x += c; 
					mseptr->state = WAIT_FOR_Y1_DELTA;
					break;

				case WAIT_FOR_Y1_DELTA:
					mseptr->y += - c;
					mseptr->state = WAIT_FOR_START_BYTE;
					if (first_button == B_FALSE) {
						first_button = B_TRUE;
						break;
					}
					mseproc(mseptr);
					break;
			}
   		}
	}
	freemsg(mp);
}

/*
 * STATIC void
 * smse_config_parse(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
smse_config_parse(queue_t *q, mblk_t *mp)
{
	register mblk_t			*bp;
	register struct strmseinfo	*mseptr;
	register unchar			c;
	register int			lcv;

	/*
	 * Need to determine what type of serial mouse is attached:
	 *
	 * If it is a Logitech Type C, it will respond to to the 't'
	 * I sent in smseopen() with 2 bytes, 'S' followed by 'D'.
	 * If it is a NON-programmable Microsoft-compatible, it will
	 * echo the "*?DSt" I sent in smseopen() right back.  The
	 * programmable Microsoft-compatible responds non-
	 * deterministically, so we will default to that if no
	 * recognizable response is forth coming.
	 *
	 * NOTE: Now that suppport for MSC mice has been added, the
	 * 	 family of MSC mice overlaps with the Microsoft
	 *	 compatible ones. So in order to support both types,
 	 *	 a tunable in space.c is provided to switch between
	 *	 these two types. The parse function for these are
	 *	 initialized in the timeout handler depending on the
	 *	 tunable selected by the user.
	 *
	 *	 For Example: 	For Mouse Systems Corpn compatible mice
	 *			Set the tunable smse_MSC_selected in space.c 
	 *			To bring it back to Microsoft compatible
	 *			mice, reset the tunable to zero.
	 */

	mseptr = (struct strmseinfo *)q->q_ptr;

	for (bp = mp; bp != (mblk_t *)NULL; bp = bp->b_cont) {
		lcv = bp->b_wptr - bp->b_rptr;

		while (lcv--) {
			c = *bp->b_rptr++;
			DEBUG1(("smse_config_parse: %x \n",c));

			switch (mseptr->state) {
			case START_CONFIG:
				if (c == 'S')
					mseptr->state = MM_PROTOCOL;
				else if (c == '*')
					mseptr->state = M_PLUS_PROTOCOL;
				break;

			case MM_PROTOCOL:
				if (c == 'D') {
					untimeout(mseptr->msetimeid);
					mseptr->smseparse = smse_MM_parse;
					mseptr->msetimeid = timeout(
					smse_config_mouse, (caddr_t)mseptr, 
						drv_usectohz(200000));
					goto free;
				}
				mseptr->state = START_CONFIG;
				break;

			case M_PLUS_PROTOCOL:
				if (c == '?') {
					untimeout(mseptr->msetimeid);
					mseptr->smseparse = smse_M_plus_parse;
					mseptr->report_not_sent = TRUE;
					mseptr->middle_button_down = FALSE;
					smse_config_port(mseptr);
					goto free;
				}
				mseptr->state = START_CONFIG;
				break;
			}
   		}
	}
free:
	freemsg(mp);
}


/*
 * STATIC void
 * smse_MM_parse(queue_t *, mblk_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
smse_MM_parse(queue_t *q, mblk_t *mp)
{
	register mblk_t			*bp;
	register struct strmseinfo	*mseptr;
	register unchar			c;

	/*
	 * Parse the next byte of serial input.
	 * This assumes the input is in MM Series format.
	 */
	mseptr = (struct strmseinfo *)q->q_ptr;

	for (bp = mp; bp != (mblk_t *)NULL; bp = bp->b_cont) {
		while (bp->b_wptr - bp->b_rptr) {

			c = *bp->b_rptr++;

			if (c & 0x80) {
				/* Bit seven set always means the first byte */
				mseptr->button = (~c & 0x07);	/* Buttons */
				mseptr->x = (c & 0x10) ? 1 : -1;/* X sign bit */
				mseptr->y = (c & 0x08) ? -1 : 1;/* Y sign bit */
				mseptr->state = 1;
				continue;
			}

			switch (mseptr->state) {
			case 1:	/* Second byte is X movement */
				mseptr->x *= c;
				mseptr->state = 2;
				break;

			case 2:	/* Third byte is Y movement */
				mseptr->y *= c;
				mseptr->state = 0;
				mseproc(mseptr);
				break;
			}
   		}
	}
	freemsg(mp);
}


/*
 * STATIC int
 * smse_srvp(queue_t *)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC int
smse_srvp(queue_t *q)
{
	register mblk_t *mp;

	while ((mp = getq(q)) != NULL) {
		switch (mp->b_datap->db_type) {
		case M_DATA:
			(*((struct strmseinfo *)q->q_ptr)->smseparse)(q, mp);
			break;

		default:
			putnext(q, mp);
			break;
		}
	}
	return(0);
}


#ifdef ESMP
/*
 * STATIC void
 * smse_wakeup(void)
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
smse_wakeup(void)
{
	SV_SIGNAL(smse_bufcallsv, 0);
}
#endif
