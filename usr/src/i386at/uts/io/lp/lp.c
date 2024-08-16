/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/lp/lp.c	1.22"

/*
 *      LP (Line Printer) Driver        EUC handling version
 */

#include <util/param.h>
#include <util/types.h>
#include <proc/signal.h>
#include <io/lp/lp.h>
#include <io/dma.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <io/termio.h>
#include <io/termios.h>
#include <util/cmn_err.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <util/debug.h>
#include <io/ldterm/eucioctl.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <util/mod/moddefs.h>
#include <svc/errno.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>

/* THESE MUST COME LAST */
#include <io/ddi.h>
#include <io/ddi_i386at.h>

int	lpinit();

#define DRVNAME         "lp - line printer driver"

extern int get_mca_io();

STATIC int lp_load(void), lp_unload(void);
int     lp_verify();
void *lp_intr_cookie;	/* cookie for interrupts */

MOD_ACDRV_WRAPPER(lp, lp_load, lp_unload, NULL, lp_verify, DRVNAME);
extern	void    mod_drvattach(), mod_drvdetach();

/*
 * lp_load(void)
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC int
lp_load(void)
{
	int ret;

	if ((ret = lpinit()) != 0) 
		return(ret);
	mod_drvattach(&lp_attach_info);
	return(0);
}

/*
 * lp_unload(void)
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC int
lp_unload(void)
{
	(void)cm_intr_detach(lp_intr_cookie);
	return(0);
}

struct 	strtty *lp_tty;   /* tty structs for each device */
time_t  *last_time;     /* output char watchdog timeout */
struct 	lpcfg *lpcfg; 
int     NUMLP;
int	lp_num;
                                

#define MAXTIME         2               /* 2 sec */
int			lpdevflag=0;
static char             lptmrun=0;      /* timer set up? */
static int              dummy;          /* placeholder for callout timer */

/* extern  debugger();  -- comment in to use kernel debug on lpinit() */


STATIC void	lpgetoblk(), lpflush(), lpdelay(), lptmout(), lpproc(), lpxintr() ;

STATIC 	int timeflag=0;
STATIC 	int lpopen(), lpclose();
STATIC 	int lpoput(); 
int 	lpintr(), lpisrv();
STATIC 	void lpputioc(), lpsrvioc();

static struct module_info lp_info = { 
/* id, name, min pkt siz, max pkt siz, hi water, low water */
        42, "lp", 0, INFPSZ, 256, 128};
static struct qinit lp_rint = { 
        putq, NULL, lpopen, lpclose, NULL, &lp_info, NULL};
static struct qinit lp_wint = { 
        lpoput, NULL, lpopen, lpclose, NULL, &lp_info, NULL};
struct streamtab lpinfo = {
	&lp_rint, &lp_wint, NULL, NULL};

/*
 * lpopen()
 *
 * Calling/Exit State:
 *
 * Description:
 */
/* ARGSUSED */
STATIC int
lpopen(q, devp, flag, sflag, cred_p) 
queue_t *q;
dev_t *devp;
register flag;
register sflag;
struct cred *cred_p;

{
        register struct strtty *tp;
        register int    oldpri;
        mblk_t *mop;
        minor_t dev;
	int	lp_status=0;
	clock_t	lp_delay = drv_usectohz(100000);   /* 100 msec interval */
	unsigned long lp_cnt = 100;		   /* 10 sec max */

#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
        cmn_err(CE_NOTE,"IN LPOPEN");
#endif
        dev = getminor(*devp);
	
	/* Check if device number is valid */
	if (dev >= NUMLP)
		return(ENXIO);

	/* Was device detected ?? */
        if ((lpcfg[dev].flag & LPPRES) == 0)
                return(ENXIO);

	/* Exclusive usage */
        if (lpcfg[dev].flag & OPEN)
                return(EBUSY);

        /*
         * Check printer status. Return EIO if printer
         * status is not LP_OK (LP_OK indicates printer is connected, powered
         * up, and ready.) A very slow printer may still be in the act of
         * performing a reset (especially if the driver was auto-loaded), so
         * allow a delay of up to 10 seconds to wait for this.
         */

        /* Supplemental read of status port for reliability */
        lp_status = inb(lpcfg[dev].status);

        while (((lp_status = (inb(lpcfg[dev].status) & LP_STATUSMASK)) != LP_OK) && lp_cnt) {
                delay(lp_delay);
                lp_cnt--;
        }

        /* Exit if the status is not LP_OK */
        if (lp_status !=  LP_OK ) {
                return(EIO);
        }

#ifdef MERGE386  /* Provide exclusive access to device for MERGE */
	if (!portalloc(lpcfg[dev].data, lpcfg[dev].control))
		return(EBUSY);
#endif /* MERGE386 */

	/* Set device open flag */
	lpcfg[dev].flag |= OPEN;

        tp = &lp_tty[dev];
        q->q_ptr = (caddr_t) tp;
        WR(q)->q_ptr = (caddr_t) tp;
        tp->t_rdqp = q;
        tp->t_dev = dev;

        if (mop = allocb(sizeof(struct stroptions), BPRI_MED)) {
                register struct stroptions *sop;

                mop->b_datap->db_type = M_SETOPTS;
                mop->b_wptr += sizeof(struct stroptions);
				/* LINTED pointer alignment */
                sop = (struct stroptions *)mop->b_rptr;
                sop->so_flags = SO_HIWAT | SO_LOWAT | SO_ISTTY;
                sop->so_hiwat = 512;
                sop->so_lowat = 256;
                putnext(q, mop);
        } else 
                return(EAGAIN);

        if ((tp->t_state & (ISOPEN | WOPEN)) == 0) {
                tp->t_iflag = IGNPAR;
                tp->t_cflag = B300|CS8|CLOCAL;
                oldpri = SPL();
                if (!lptmrun) {
                        last_time[dev] = 0x7fffffffL;
                        lptmout(dummy);
                }
                splx(oldpri); 
        }

        oldpri = SPL();

        tp->t_state |=  CARR_ON;

        tp->t_state &= ~WOPEN;
        tp->t_state |= ISOPEN;

        splx(oldpri);
        return(0);
}


/*
 * lpclose()
 *
 * Calling/Exit State:
 *
 * Description:
 */
/* ARGSUSED */
STATIC int
lpclose(q, flag, cred_p)
queue_t *q;
register flag;
cred_t *cred_p;
{
        register struct strtty *tp;
        register int    oldpri;
        int dev;

#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
        cmn_err(CE_NOTE,"IN LPCLOSE");
#endif
        tp = (struct strtty *)q->q_ptr;
        dev = tp - lp_tty;


        if(!(tp->t_state & ISOPEN)) {  /* See if it's closed already */
                return(0);
        }

        if ( !( flag & (FNDELAY|FNONBLOCK))) {
                /* Drain queued output to the printer. */
                oldpri = spltty();
                while (( tp->t_state & CARR_ON) ){
                        if((tp->t_out.bu_bp == 0 ) && (WR(q)->q_first == NULL)) 
                                break;
                        tp->t_state |= TTIOW;
                        if( sleep((caddr_t) &tp->t_oflag,TTIPRI|PCATCH)){
                                tp->t_state &= ~(TTIOW|CARR_ON);
                                break;
                        }
                }
                splx(oldpri);
        }

        /* do not >>> outb(lpcfg[tp->t_dev].control, 0) -- because 
        close() gets called before all characters are sent, therefore, 
        the last chars do not get output with the interrupt turned off */ 

        untimeout(timeflag);  
        lptmrun = 0;
        lpcfg[dev].flag &= ~OPEN;
        tp->t_state &= ~(ISOPEN|CARR_ON);

#ifdef MERGE386
	portfree(lpcfg[dev].data, lpcfg[dev].control);
#endif /* MERGE386 */

        tp->t_rdqp = NULL;
        q->q_ptr = WR(q)->q_ptr = NULL;
		return ( 1 ) ;
}

/*
 * lpoput()
 *
 * Calling/Exit State:
 *
 * Description:
 */

STATIC int
lpoput(q, bp)
queue_t *q;
mblk_t *bp;
{
        register struct msgb *bp1;
        register struct strtty *tp;
        int s;

#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
        cmn_err(CE_NOTE,"IN LPOPUT");
#endif
        tp = (struct strtty *)q->q_ptr;

        switch(bp->b_datap->db_type) {

        case M_DATA:
                s = spltty();
                while (bp) {
                        bp->b_datap->db_type = M_DATA;
                        bp1 = unlinkb(bp);
                        bp->b_cont = NULL;
                        if ((bp->b_wptr - bp->b_rptr) <= 0) {
                                freeb(bp);
                        } else {
                                putq(q,bp);
                        }
                        bp = bp1;
                }
                splx(s);
                if (q->q_first != NULL) {
                        lpgetoblk(tp);
                }
                break;

        case M_IOCTL:
                lpputioc(q, bp);
                if (q->q_first != NULL) {
                        lpgetoblk(tp);
                }
                break;

        case M_DELAY:
                s = spltty();
                if( (tp->t_state & TIMEOUT ) || (q->q_first != NULL ) 
                        || (tp->t_out.bu_bp != NULL)) {
                        putq( q, bp );
                        splx(s);
                        break;
                }
                tp->t_state |= TIMEOUT;
                timeout ( lpdelay, (caddr_t)tp, ((int)*(bp->b_rptr))*HZ/60);
                splx (s);
                freemsg (bp);
                break;

        case M_FLUSH:
                s = SPL();
                switch( *(bp->b_rptr) ) {

                case FLUSHRW:
                        lpflush(tp, (FREAD|FWRITE));
                        *(bp->b_rptr) = FLUSHR;
                        qreply(q, bp);
                        break;

                case FLUSHR:
                        lpflush(tp, FREAD);
                        qreply(q, bp);
                        break;

                case FLUSHW:
                        lpflush(tp, FWRITE);
                        freemsg(bp);
                        break;

                default:
                        break;
                }
                splx(s);
                break;

        case M_START:
                s = SPL();
                lpproc(tp, T_RESUME);
                splx(s);
                freemsg(bp);
                break;

        case M_STOP:
                s = SPL();
                lpproc(tp, T_SUSPEND);
                splx(s);
                freemsg(bp);
                break;

        default:
                freemsg(bp);
                break;
        }
		return ( 0 ) ;
}

/*
 * lpgetoblk()
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC void
lpgetoblk(tp)
register struct strtty *tp;
{
        register int s;
        register struct queue *q;
        register struct msgb    *bp;

#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
		cmn_err(CE_NOTE,"in lpgetoblk");
#endif
        if (tp->t_rdqp == NULL) {
                return;
        }
        q = WR(tp->t_rdqp);

        s = SPL();

        while (!(tp->t_state & BUSY) && (bp = getq(q)) != NULL) {
                
                switch (bp->b_datap->db_type) {

                case M_DATA:
                        if (tp->t_state & (TTSTOP | TIMEOUT)) {
                                putbq(q, bp);
                                splx(s);
                                return;
                        }

                        /* start output processing for bp */
                        tp->t_out.bu_bp = bp;
                        lpproc(tp, T_OUTPUT);
                        break;

                case M_DELAY:
                        if(tp->t_state & TIMEOUT ) {
                                putbq( q, bp );
                                splx(s);
                                return;
                        }
                        tp->t_state |= TIMEOUT;
                        timeout ( lpdelay, (caddr_t)tp, ((int)*(bp->b_rptr))*HZ/60 );
                        freemsg (bp);
                        break;

                case M_IOCTL:
                        lpsrvioc(q, bp);
                        break;

                default:
                        freemsg(bp);
                        break;
                }
        }
        /* Wakeup any process sleeping waiting for drain to complete */
        if (( tp->t_out.bu_bp == 0 ) && (tp->t_state & TTIOW)) {
                tp->t_state &= ~(TTIOW);
                wakeup((caddr_t) &tp->t_oflag);
        }  
        splx(s);
}

/*
 * lpputioc()
 *
 * Calling/Exit State:
 *
 * Description:
 * ioctl handler for output PUT procedure
 */
STATIC void
lpputioc(q, bp)
queue_t *q;
mblk_t *bp;
{
        struct strtty *tp;
        struct iocblk *iocbp;
        mblk_t *bp1;

		/* LINTED pointer alignment */
        iocbp = (struct iocblk *)bp->b_rptr;
        tp = (struct strtty *)q->q_ptr;

        /* Only called for M_IOCTL messages. */
#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
        cmn_err(CE_NOTE,"IN LPPUTIOC ioctl=%x", iocbp->ioc_cmd);
#endif

        switch( iocbp->ioc_cmd ) {

        case TCSBRK:
        case TCSETAW:
        case TCSETSW:
        case TCSETSF:
        case TCSETAF:/* run these now, if possible */

                if( q->q_first != NULL || (tp->t_state & (BUSY|TIMEOUT) ) 
                        || (tp->t_out.bu_bp != NULL)) 
                {
                        putq(q, bp);
                        break;
                }
                lpsrvioc(q, bp);
                break;

        case TCSETS:
        case TCSETA:    /* immediate parm set   */

                if ( tp->t_state & BUSY) {
                        putbq( q, bp); /* queue these for later */
                        break;
                }

                lpsrvioc(q, bp);
                break;

        case TCGETS:
        case TCGETA:    /* immediate parm retrieve */
                lpsrvioc(q, bp);
                break;

        case EUC_MSAVE:
        case EUC_MREST:
        case EUC_IXLOFF:
        case EUC_IXLON:
        case EUC_OXLOFF:
        case EUC_OXLON: 
                bp->b_datap->db_type = M_IOCACK;
                iocbp->ioc_count = 0;
                qreply(q, bp);
                break;

        default:
                if ((iocbp->ioc_cmd&IOCTYPE) == LDIOC) {
                        bp->b_datap->db_type = M_IOCACK; /* ignore LDIOC cmds */
                        bp1 = unlinkb(bp);
                        if (bp1) {
                                freeb(bp1);
                        }
                        iocbp->ioc_count = 0;
                } else {
/*
 * Unknown IOCTLs aren't errors, they just may have been intended for an
 * upper module that isn't present.  NAK them...
 */
                        iocbp->ioc_error = EINVAL;
                        iocbp->ioc_rval = (-1);
                        bp->b_datap->db_type = M_IOCNAK;
                }
                qreply(q, bp);
                break;
        }
}

/*
 * lpsrvioc()
 *
 * Calling/Exit State:
 *
 * Description:
 * Ioctl processor for queued ioctl messages.
 */
STATIC void
lpsrvioc(q, bp)
queue_t *q;
mblk_t *bp;
{
        struct strtty *tp;
        struct iocblk *iocbp;
        struct termio  *cb;
        struct termios *scb;
/*
        int arg;
*/
        mblk_t *bpr;
        mblk_t *bp1;

		/* LINTED pointer alignment */
        iocbp = (struct iocblk *)bp->b_rptr;
        tp = (struct strtty *)q->q_ptr;
#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
        cmn_err(CE_NOTE,"IN LPSRVIOC ioctl=%x", iocbp->ioc_cmd); 
#endif
        switch( iocbp->ioc_cmd ) {

        case TCSETSF: /* The output has drained now. */
                lpflush(tp, FREAD); /* fall thru .. */

        /* (couldn't get block before...) */

				/* FALLTHRU */
        case TCSETS: 
        case TCSETSW:
                if ( !bp->b_cont) {
                        iocbp->ioc_error = EINVAL;
                        bp->b_datap->db_type = M_IOCNAK;
                        iocbp->ioc_count = 0;
                        qreply(q, bp);
                        break;
                }

				/* LINTED pointer alignment */
                scb = (struct termios *)bp->b_cont->b_rptr;
                tp->t_cflag = scb->c_cflag;
                tp->t_iflag = scb->c_iflag;
                bp->b_datap->db_type = M_IOCACK;
                bp1 = unlinkb(bp);
                if (bp1) {
                        freeb(bp1);
                }
                iocbp->ioc_count = 0;
                qreply(q, bp);
                break;

        case TCSETAF: /* The output has drained now. */
                lpflush(tp, FREAD); /* fall thru .. */

				/* FALLTHRU */
        case TCSETA: 
        case TCSETAW:
                if ( !bp->b_cont) {
                        iocbp->ioc_error = EINVAL;
                        bp->b_datap->db_type = M_IOCNAK;
                        iocbp->ioc_count = 0;
                        qreply(q, bp);
                        break;
                }

				/* LINTED pointer alignment */
                cb = (struct termio *)bp->b_cont->b_rptr;
                tp->t_cflag = cb->c_cflag;
                tp->t_iflag = cb->c_iflag;
                bp->b_datap->db_type = M_IOCACK;
                bp1 = unlinkb(bp);
                if (bp1) {
                        freeb(bp1);
                }
                iocbp->ioc_count = 0;
                qreply(q, bp);
                break;

        case TCGETS:    /* immediate parm retrieve */
                if ( bp->b_cont )
                        freemsg(bp->b_cont);
		bp->b_cont = NULL;

                if( (bpr = allocb(sizeof(struct termios),BPRI_MED)) == NULL ) {
                        ASSERT(bp->b_next == NULL);
                        putbq(q, bp);
                        bufcall((ushort)sizeof(struct termios), BPRI_MED,
                                        lpgetoblk,(long)tp);
                        break;
                }
                bp->b_cont = bpr;

				/* LINTED pointer alignment */
                scb = (struct termios *)bp->b_cont->b_rptr;

                scb->c_iflag = tp->t_iflag;
                scb->c_cflag = tp->t_cflag;

                bp->b_cont->b_wptr += sizeof(struct termios);
                bp->b_datap->db_type = M_IOCACK;
                iocbp->ioc_count = sizeof(struct termios);
                qreply(q, bp);
                break;

        case TCGETA:    /* immediate parm retrieve */
                if ( bp->b_cont )
                        freemsg(bp); /* bad user supplied parameter */
		bp->b_cont = NULL;

                if( (bpr = allocb(sizeof(struct termio),BPRI_MED)) == NULL ) {
                        ASSERT(bp->b_next == NULL);
                        putbq(q, bp);
                        bufcall((ushort)sizeof(struct termio), BPRI_MED,
                                        lpgetoblk,(long)tp);
                        break;
                }
                bp->b_cont = bpr;
				/* LINTED pointer alignment */
                cb = (struct termio *)bp->b_cont->b_rptr;

                cb->c_iflag = tp->t_iflag;
                cb->c_cflag = tp->t_cflag;

                bp->b_cont->b_wptr += sizeof(struct termio);
                bp->b_datap->db_type = M_IOCACK;
                iocbp->ioc_count = sizeof(struct termio);
                qreply(q, bp);
                break;

        case TCSBRK:
                /* Skip the break since it's a parallel port. */
/*
                arg = *(int *)bp->b_cont->b_rptr;
*/
                bp->b_datap->db_type = M_IOCACK;
                bp1 = unlinkb(bp);
                if (bp1) {
                        freeb(bp1);
                }
                iocbp->ioc_count = 0;
                qreply(q, bp);
                break;

        case EUC_MSAVE: /* put these here just in case... */
        case EUC_MREST:
        case EUC_IXLOFF:
        case EUC_IXLON:
        case EUC_OXLOFF:
        case EUC_OXLON: 
                bp->b_datap->db_type = M_IOCACK;
                iocbp->ioc_count = 0;
                qreply(q, bp);
                break;

        default: /* unexpected ioctl type */
                if( canput(RD(q)->q_next) == 1 ) {
                        bp->b_datap->db_type = M_IOCNAK;
                	iocbp->ioc_count = 0;
                        qreply(q, bp);
                } else {
                        putbq(q, bp);
                }
                break;
        }
}

/*
 * lpflush()
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC void
lpflush(tp, cmd)
struct strtty *tp;
{
        queue_t *q;
        int s;

        s = SPL();
#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
        cmn_err(CE_NOTE,"IN LPFLUSH");
#endif
        if (cmd&FWRITE) {
                q = WR(tp->t_rdqp);
                /* Discard all messages on the output queue. */
                flushq(q,FLUSHDATA);
                tp->t_state &= ~(BUSY);
                tp->t_state &= ~(TBLOCK);
                if (tp->t_state & TTIOW) {
                        tp->t_state &= ~(TTIOW);
                        wakeup((caddr_t) &tp->t_oflag);
                }

        }
        if (cmd&FREAD) {
                tp->t_state &= ~(BUSY);
        }
        splx(s);
        lpgetoblk(tp);

}

/*
 * lpintr()
 *
 * Calling/Exit State:
 *
 * Description:
 * lpintr is the entry point for all interrupts. 
 */
lpintr(vec)
unsigned int    vec;
{
        register        mdev;
        register unsigned char  status;

#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
		cmn_err(CE_NOTE,"in lpintr");
#endif
        for (mdev=0; mdev <= NUMLP; mdev++) 
                if( (lpcfg[mdev].vect == vec) && lpcfg[mdev].flag) 
                        break;
                
        if (mdev > NUMLP)
                return(ENXIO);

        status = inb(lpcfg[mdev].status);

        if ( status & NOPAPER )
                return(1);

        if ( status & UNBUSY  ) 
                lpxintr(mdev);

		return ( 0 ) ;
}

/*
 * lpxintr()
 *
 * Calling/Exit State:
 *
 * Description:
 * This is logically a part of lpintr.  This code
 * handles transmit buffer empty interrupts, 
 * It works in  conjunction with lptmout() to insure that lost 
 * interrupts don't hang  the driver:  
 * if a char is xmitted and we go more than 2s (MAXTIME) without
 * an interrupt, lptmout will supply it.
 */
STATIC void
lpxintr(mdev)
register int    mdev;
{
        struct strtty      *tp;

#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
		cmn_err(CE_NOTE,"in lpxintr");
#endif
        last_time[mdev] = 0x7fffffffL;  /* don't time out */
        tp = &lp_tty[mdev];

        if (tp->t_state & BUSY) {
                tp->t_state &= ~BUSY;
                lpproc( tp, T_OUTPUT );

                /* if output didn't start get a new message */
                if (!(tp->t_state & BUSY)) {
                        if (tp->t_out.bu_bp) {
                                freemsg(tp->t_out.bu_bp);
                        }
                        tp->t_out.bu_bp = 0;
                        lpgetoblk(tp);
                }
        }
}

/*
 * lpproc()
 *
 * Calling/Exit State:
 *
 * Description:
 *      General command routine that performs device specific operations for
 * generic i/o commands.  All commands are performed with tty level interrupts
 * disabled.
 */
STATIC void
lpproc(tp, cmd)
struct strtty   *tp;
int             cmd;
{
        int             dev;
        register int    oldpri;
        register struct msgb *bp;

        
        oldpri = SPL();
        /*
         * get device number and control port
         */
        dev = tp - lp_tty;

#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
        cmn_err(CE_NOTE,"in lpproc cmd=%x", cmd);
#endif
        /*
         * based on cmd, do various things to the device
         */

        switch (cmd) {

        case T_TIME:            /* stop sending a break -- disabled for LP */
                goto start;

        case T_RESUME:          /* enable output        */

                tp->t_state &= ~TTSTOP;
                /* fall through */

			/* FALLTHRU */
        case T_OUTPUT:          /* do some output       */
start:

                /* If we are busy, do nothing */
                if ( tp->t_state & ( BUSY|TTSTOP|TIMEOUT ) ) break;

                /*
                 * Check for characters ready to be output.
                 * If there are any, ship one out.
                 */
                bp = tp->t_out.bu_bp;
                if (bp == NULL || bp->b_wptr <= bp->b_rptr) {
                        if (tp->t_out.bu_bp) {
                                freemsg(tp->t_out.bu_bp);
                        }
                        tp->t_out.bu_bp = 0;
                        lpgetoblk(tp);
                        break;
                }

                /* specify busy, and output a char */
                tp->t_state |= BUSY;
                /* reset the time so we can catch a missed interrupt */
                outb(lpcfg[dev].control, SEL|RESET);
                outb(lpcfg[dev].data, *bp->b_rptr++ );
                outb(lpcfg[dev].control, SEL|RESET|STROBE|INTR_ON);
                drv_usecwait(10);
                outb(lpcfg[dev].control, SEL|RESET|INTR_ON);
                drv_getparm(TIME, (ulong*)&last_time[dev]);

                break;

        case T_SUSPEND:         /* block on output      */

                tp->t_state |= TTSTOP;
                break;

        case T_BREAK:           /* send a break -- disabled for LP    */
                break;
        }
        splx(oldpri);
}

/*
 * lpinit()
 *
 * Calling/Exit State:
 *
 * Description:
 * Initialization code that the kernel runs when coming up.  Detect
 * any LPs we see hanging around.
 */
int
lpinit()
{

        register ushort 	testval;
        int                 	dev;
        unsigned int    	bus_p;
        int             	addr_bit;
	struct lpcfg 		*lpcfgptr;

	cm_args_t               cm_args;
	struct  cm_addr_rng     lp_ioaddr;

#ifdef NOTYET
	int			cm_ver_no;

	cm_ver_no = cm_getversion();  /* NOT YET USED */
#endif /* NOTYET */

	/* 
	 * Get the number of Parallel Ports configured 
	 */
	lp_num = cm_getnbrd("lp");
	NUMLP = lp_num;

	/* 
	 * Allocate an array of lp device data structures, one 
	 * structure for each board instance.
	 */
	if ((lpcfg = (struct lpcfg *)
		kmem_zalloc(lp_num*sizeof(struct lpcfg), KM_NOSLEEP)) == NULL) {
		/* no memory */
		return (ENOMEM);
	}

	if ((lp_tty = (struct strtty *)
		kmem_zalloc(lp_num*sizeof(struct strtty), KM_NOSLEEP)) == NULL) {

		/* no memory */
		return (ENOMEM);
	}

	if ((last_time = (time_t *)
		kmem_zalloc(lp_num*sizeof(time_t), KM_NOSLEEP)) == NULL) {
		/* no memory */
		return (ENOMEM);
	}

	
	if ((drv_gethardware(IOBUS_TYPE, &bus_p)) < 0) {
		cmn_err(CE_WARN,"lp: can't decide bus type");
		return (EINVAL);
	}

	/*
	 * Loop for each board instance.
	 */

        for (dev=0; dev < NUMLP; dev++) {
		lpcfgptr = &lpcfg[dev];
		if (bus_p & BUS_MCA) {

			/*
   	 	 	 * Get key for lp.
	 	 	 */
			cm_args.cm_key = cm_getbrdkey("lp", dev);
			cm_args.cm_n   = 0;

			addr_bit = get_mca_io();
			switch(addr_bit) {
				case 0:
					/* parallel 1, 03bc-03be */
					/* lpcfg[dev].data = 0x03BC; */
					lpcfgptr->data = 0x03BC;
					break;
				case 1:
					/* parallel 2, 0378-037a */
					/* lpcfg[dev].data = 0x0378; */
					lpcfgptr->data = 0x0378;
					break;
				case 2:
					/* parallel 3, 0278-027a */
					/* lpcfg[dev].data = 0x0278; */
					lpcfgptr->data = 0x0278;
					break;
				case -1:
					cmn_err(CE_WARN, "lp: port disabled\n");
					return(EINVAL);
				default:
					cmn_err(CE_WARN, "lp: configuration error, can't decide io address\n");
					return(EINVAL);
			}
   			lpcfgptr->status = lpcfgptr->data + 1;
   			lpcfgptr->control = lpcfgptr->data + 2;
		
			lpcfgptr->vect = 7;
                	outb(lpcfgptr->data, 0x55);
                	testval = ((short)inb(lpcfgptr->data) & 0xFF) ;

                	if (testval != 0x55) {
                		lpcfgptr->flag &= ~LPPRES; /* controller not present */
                       		continue;
			}
		}
		else {  /* non-MCA */
			/*
   	 	 	 * Get key for lp.
	 	 	 */
			cm_args.cm_key = cm_getbrdkey("lp", dev);
			cm_args.cm_n   = 0;
   
			/*
	 		 * Get interrupt vector.
	 		 */
			cm_args.cm_param = CM_IRQ;
			cm_args.cm_val = &(lpcfgptr->vect);
   			cm_args.cm_vallen = sizeof(lpcfgptr->vect);
   			if (cm_getval(&cm_args)) {
				lpcfgptr->vect = 0;
				continue;
   			} else {
   				if ( lpcfgptr->vect == 0 ) {
   					continue;
   				}
			}

			/*
   	 		 * Get I/O address range.
   	 	 	 */
   			cm_args.cm_param = CM_IOADDR;
   			cm_args.cm_val = &lp_ioaddr;
   			cm_args.cm_vallen = sizeof(struct cm_addr_rng);
   			if (cm_getval(&cm_args)) {
   				lpcfgptr->data = 0;
				continue;
   			} else {
   				lpcfgptr->data = lp_ioaddr.startaddr;
   				lpcfgptr->status = lp_ioaddr.startaddr + 1;
   				lpcfgptr->control = lp_ioaddr.startaddr + 2;
   			}
  		 
   			/*
                	 * Probe for the board.
   			 */
                	outb(lpcfgptr->data, 0x55);
                	testval = ((short)inb(lpcfgptr->data) & 0xFF) ;

                	if (testval != 0x55) {
                		lpcfgptr->flag &= ~LPPRES; /* controller not present */
                       		continue;
			}


		} /* if BUS_MCA */

                /* We found a live one 
                 * Program the device to be benign.
                 */
                outb(lpcfgptr->control, SEL);
                drv_usecwait(750);
                outb(lpcfgptr->control, SEL|RESET);
                lpcfgptr->flag = LPPRES; /* controller present */

	} /* for loop */

	(void)cm_intr_attach(cm_args.cm_key, lpintr, &lpdevflag, &lp_intr_cookie);

	return(0);
}
/*
* int
* lp_verify(rm_key_t key)
*
* Calling/Exit State:
*      None
*/
int
lp_verify(rm_key_t key)
{

	register ushort testval;
	int            	i;
	cm_args_t      	cm_args;
	struct  lpcfg	inst;

#ifdef NOTYET
	if (cm_getversion() == NULL) {
		return (0);
	}
#endif /* NOTYET */

	cm_args.cm_key = key;
	cm_args.cm_n = 0;

	/*
	 * Get interrupt vector.
	 */
	cm_args.cm_param = CM_IRQ;
	cm_args.cm_val = &(inst.vect);
   	cm_args.cm_vallen = sizeof(inst.vect);
   	if (cm_getval(&cm_args)) {
		return (EINVAL);
   	} 

	/*
   	 * Get I/O address range.
   	 */
   	cm_args.cm_param = CM_IOADDR;
   	cm_args.cm_val = &inst.data;
   	cm_args.cm_vallen = sizeof(struct cm_addr_rng);
   	if (cm_getval(&cm_args)) {
		return (EINVAL);
   	}

	/*
   	 * Driver specific code to verify the existence
   	 * of hardware using the parameters obtained above,
   	 * return 0 if the hardware is found, ENODEV otherwise.
   	 */
  		 
        outb(inst.data, 0x55);
        testval = ((short)inb(inst.data) & 0xFF) ;

        if (testval != 0x55) {
 		return (ENODEV);
	} else {
        	/* We found a live one 
        	 * Program the device to be benign.
        	 */
        	outb(inst.control, SEL);
        	drv_usecwait(750);
        	outb(inst.control, SEL|RESET);
        	inst.flag = LPPRES; /* controller present */
		return (0);
	}
}

/*
 * lptmout()
 *
 * Calling/Exit State:
 *
 * Description:
 * Watchdog timer handler.
 */
/* ARGSUSED */
STATIC void
lptmout(notused)
int notused; /* dummy variable that callout will supply */
{
        time_t diff, lptime;
        register int    dev;
        register int    oldpri;
        unsigned char  status;

#ifdef LP_DEBUG
		/* 
		 *+ debug
		 */
		cmn_err(CE_NOTE,"in lptmout");
#endif

        for (dev=0; dev <= NUMLP; dev++) {
                drv_getparm(TIME, (ulong*)&lptime);
                if ((diff = lptime-last_time[dev]) > MAXTIME &&
                    diff <= MAXTIME+2)  {
        		status = inb(lpcfg[dev].status);

        		if ( (status & NOPAPER) || !(status & UNBUSY) ){
                		drv_getparm(TIME, (ulong*)&last_time[dev]);
			}else{
                        	oldpri = SPL();
                        	lpxintr(dev);
                        	splx(oldpri);
			}
                }
        }
        lptmrun = 1;
        timeflag = timeout(lptmout, (caddr_t)(&dummy), drv_usectohz(1000000));
}

/*
 * lpdelay()
 *
 * Calling/Exit State:
 *
 * Description:
 */
STATIC void
lpdelay(tp)
register struct strtty *tp;
{
        int s;

        s=spltty();
        tp->t_state &= ~TIMEOUT;
        lpproc(tp, T_OUTPUT);
        splx(s);
}


STATIC int
get_mca_io()
{
	unsigned char	port = 0;
	unsigned char	pos;

	/* turn on set up mode */
	port |= 0x20;	
	outb(PORT_ENAB, port);

	pos = inb(SYS_IO);
	outb(PORT_ENAB, 0xFF); /* unset set up */
	if (!(pos & 0x10)) { /* parallel port disabled */
		return(-1);
	}
	pos = (pos >> 5) & 3;
	switch(pos) {
		case 0: return(0);
			/* NOTREACHED */
			break;
		case 1: return(1);
			/* NOTREACHED */
			break;
		case 2: return(2);
			/* NOTREACHED */
			break;
		default: return(-1);
			/* NOTREACHED */
			 break;
	}
}
			
