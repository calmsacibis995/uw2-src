/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/asy/asyhp/asyhp.c	1.24"
#ident	"$Header: $"

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */


/*
 * High Performance Serial Port Driver
 *
 * This driver forms the hardware dependent part of the serial driver.
 * It uses the iasy driver interface to communicate to the modules upstream.
 *
 * It supports National 16550 and compatible chips. The chip has 16-deep
 * fifo. The driver writes as many as 16  characters at a time when
 * transmitter empty interrupt is received. Also, the chip is programmed
 * to interrupt when the fifo level reaches 14 or it timesout.
 */


#include <io/asy/asyhp/asyhp.h>
#include <io/asy/iasy.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <io/stream.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/tty.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/kdb/xdebug.h>
#include <util/param.h>
#include <util/types.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>

#include <io/ddi.h>	/* Must come last */

/* Character to enter kernel debugger (if on console device) */
#define DEBUGGER_CHAR	('K' - '@')	/* Ctrl-K */

/* Character to panic the mahcine (if on console device) */
#define PANIC_CHAR	('_' - '@')	/* Ctrl-hyphen */

/* Character to reboot, via "init 6" (if on console device) */
#define REBOOT_CHAR	('\\' - '@')	/* Ctrl-backslash */

#ifdef DKTEST
/* Character to invoke flavors (if on console device) */
#define FLAVORS_CHAR	('F' - '@')	/* Ctrl-F */
extern void invoke_flavors(void);
#endif /* DKTEST */

#ifdef        DEBUG
STATIC        int     asyhp_debug = 0;
#define       DEBUG1(a)       if (asyhp_debug == 1) printf a   /* General debugs */
#define       DEBUG2(a)       if (asyhp_debug <= 2) printf a   /* Allocations */
#define       DEBUG3(a)       if (asyhp_debug >= 3) printf a   /* M_CTL stuff ! */
#else
#define       DEBUG1(a)
#define       DEBUG2(a)
#define       DEBUG3(a)
#endif        /* DEBUG */

/*
 * Routine for putting a char in the input c-list 
 */
#define PutInChar(tp, c) if ((tp)->t_in.bu_ptr != NULL) { \
		*(tp->t_in.bu_ptr++) = c; \
		if (--tp->t_in.bu_cnt == 0) \
			iasy_input(tp, L_BUF); \
	}


void		asyhp_asyinit(void);
void		asyhpintr(int);

STATIC void	asyhpProcessInChar(struct strtty *, ushort *, ushort);
STATIC int	asyhpproc(struct strtty *, int);
STATIC void	asyhpstartio(struct strtty *);
STATIC int	asyhpparam(struct strtty *);
STATIC void	asyhpmodem(struct strtty *);
STATIC void	asyhpwakeup(struct strtty *);
STATIC void	asyhp_prog(struct strtty *, ushort);
STATIC void	asyhp_reset(struct asyhp *);
STATIC void	asyhpsetdtr(struct asyhp *);
STATIC void	asyhphwdep(queue_t *, mblk_t *, struct strtty *);

STATIC dev_t	asyhpcnopen(minor_t, boolean_t, const char *);
STATIC void	asyhpcnclose(minor_t, boolean_t);
STATIC int	asyhpcnputc(minor_t, int);
STATIC int	asyhpcngetc(minor_t);
STATIC void	asyhpcnsuspend(minor_t);
STATIC void	asyhpcnresume(minor_t);

STATIC struct 	asyhp *asyhp_verify_init(ushort_t);
STATIC int 	asyhp_verifyport(ushort);
struct asyhp 	*asyhp_verify_init(ushort_t); 
STATIC void 	asyhp_portinit(int);

struct asyhp	*cons_asyhptab = NULL;
int 		cons_asyhp_num = 0;

struct asyhp *asyhptab = NULL;		/* asyhp structs for each port */
unsigned int asyhp_num = 0;		/* number of asyhp serial port units */
int 	asyhpdevflag = 0;
int 	*asyhp_outchars;

extern unsigned int asyhp_sminor;	/* start minor number */
extern struct 	strtty asy_tty[];	/* iasy_tty changed to asy_tty for merge */

/*
 * Number of characters that we will ship whenever the 
 * transmitter is empty
 */

extern int	asyhp_cnt;
extern asyhp_base_t asyhp_base[];

unsigned int	asyhp_alive = 0;
unsigned int	asyhpinitialized = 0;
struct strtty	*asyhp_tty;
int		asyhp_id = 0;
int             asyhp_tick=10;
int             asyhp_rxoverun;


struct conssw	asyhpconssw = {
	asyhpcnopen, asyhpcnclose, asyhpcnputc, asyhpcngetc,
	asyhpcnsuspend, asyhpcnresume
};

/*
 * Baud rate table. Indexed by #defines found in io/termios.h
 */

#define MAXBAUDS		17

ushort asyhpspdtab[] = {
	0,	/* 0 baud rate */
	0x900,	/* 50 baud rate */
	0x600,	/* 75 baud rate */
	0x417,	/* 110 baud rate ( %0.026 ) */
	0x359,	/* 134.5 baud rate ( %0.058 ) */
	0x300,	/* 150 baud rate */
	0x240,	/* 200 baud rate */
	0x180,	/* 300 baud rate */
	0x0c0,	/* 600 baud rate */
	0x060,	/* 1200 baud rate */
	0x040,	/* 1800 baud rate */
	0x030,	/* 2400 baud rate */
	0x018,	/* 4800 baud rate */
	0x00c,	/* 9600 baud rate */
	0x006,	/* 19200 baud rate */
	0x003	/* 38400 baud rate */
};

#ifndef lint
static char asyhp_copyright[] = "Copyright Intel Corporation xxxxxx";
#endif /* lint */



/*
 * void
 * asyhp_asyinit(void) 
 *	initialize driver with interrupts off
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This is called before the interrupts are turned on to initalize
 *	the device.  Reset the puppy and program it do asyhpnc IO and to
 *	give interrupts for input and output.
 */
void
asyhp_asyinit(void)
{	
	int	unit;
	struct asyhp *asyhp;

	DEBUG1(("ASYHP: In asyhp_asyinit() .... \n"));

	/* Check if initialized earlier */
	if (asyhpinitialized)
		return;


	/* Get the number of boards installed */
	if ((asyhp_num = asyhp_getbase()) == 0)
		return;
	
	DEBUG1(("ASYHP: The number of boards detected is %d \n",asyhp_num));

	/*
	 * Register terminal server 
	 */
	asyhp_id = iasy_register(asyhp_sminor, asyhp_num, asyhpproc, asyhphwdep,
				 &asyhpconssw);
	asyhp_tty = IASY_UNIT_TO_TP(asyhp_id, 0);
}

/*
 * int
 * asyhpstart(void)
 *
 * Check the type of init and reinitialize the asyhp struct and
 * initialize the serial ports
 */

int
asyhpstart(void)
{	

	int		unit, rval;
	struct asyhp	*asyhp;			/* Temp ptr to asyhp struct */
	struct asyhp	*cons_asyhp;

	struct	cm_addr_rng	asyhp_ioaddr;	/* Board start and end addr */
	cm_args_t		cm_args;	/* ConfMgr struct */
	int			cm_ver_no;	/* ConfMgr version number */
	int 			oldpl;

	oldpl = spltty();

	DEBUG1(("ASYHP: asyhpstart() entered ...... \n"));

	/* Check if port initialised through the config manager database */
	if (asyhpinitialized & ASYHP_COM_INIT) {
		DEBUG1(("ASYHP: In asyhpstart() already initialised \n"));
		splx(oldpl);
		return;
	}

	/* 
	 * Check if port initialized thru' console init. 
	 * Reconfigure the port with the values from the
  	 * configuration manager data base.
	 */
	if (asyhpinitialized & ASYHP_CONSOLE_INIT) {

		ASSERT(asyhptab); 

		/* Override the console initialization */
		asyhpinitialized &= ~ASYHP_CONSOLE_INIT;

		/* Cache the memory allocated from the console entry points 
		 * since we don't have a way to free this memory.
		 */
		cons_asyhptab = asyhptab;
	}
	cons_asyhp_num = asyhp_num;

	/* Set flag to indicate init'ed from Config manager */
	asyhpinitialized = ASYHP_COM_INIT;

	/* Get the number of Serial Ports configured */
	asyhp_num = cm_getnbrd("asyhp");

	DEBUG1(("ASYHP: The number of boards configured is %d \n",asyhp_num));


	/* De-register and register terminal server only if mis-match */
	if ((asyhp_num) && (cons_asyhp_num != asyhp_num)) {
		/* De-register the old terminal server */
		if ((asyhp_id = iasy_deregister(asyhp_sminor, cons_asyhp_num)) != -1) {
			/*  Register terminal server. */
			asyhp_id = iasy_register(asyhp_sminor, asyhp_num, 
				asyhpproc, asyhphwdep, &asyhpconssw);
			asyhp_tty = IASY_UNIT_TO_TP(asyhp_id, 0);
		}
	}


	/* Allocate memory for the boards configured */
	if ( (rval = asyhp_memalloc()) != 0) {
		splx(oldpl);
		return rval;
	}

	/* Loop and get the configuration parameters from Config Manager */
	for (unit = 0; unit < asyhp_num; unit++) {
		asyhp = &asyhptab[unit];

		/* Get board key from Config Manager */
		if((cm_args.cm_key = cm_getbrdkey("asyhp",unit))==RM_NULL_KEY) {
			DEBUG1(("ASYHP: Error getting board key !! \n"));
			continue;
		}
		cm_args.cm_n = 0;

		/*
		 * Get Interrupt vector
		 */
		cm_args.cm_param = CM_IRQ;
		cm_args.cm_val = &(asyhp->asyhp_vect);
		cm_args.cm_vallen = sizeof(asyhp->asyhp_vect);

		if (cm_getval(&cm_args)) {	
			/* Error getting IRQ */
			asyhp->asyhp_vect = 0;	 
			DEBUG1(("ASYHP: Error getting board IRQ !! \n"));
			continue;
		} else {
			/* validate the IRQ, must be a usable one */
			if ((asyhp->asyhp_vect < 2) || (asyhp->asyhp_vect > 15) ||
				( asyhp->asyhp_vect == 13)) {
				DEBUG1(("ASYHP: Unusable IRQ !! \n"));
				continue;
			}
		}

		/* 
		 *	Get IO address range of board 
		 */
		cm_args.cm_param = CM_IOADDR;
		cm_args.cm_val = &(asyhp_ioaddr);
		cm_args.cm_vallen = sizeof(asyhp_ioaddr);
		
		if (cm_getval(&cm_args)) {
			/* Error occured in getting IO base */
			DEBUG1(("ASYHP: Error getting board IO base !! \n"));
			continue;
		}
		DEBUG1(("ASYHP: IO base and IRQ returned by Config Manager is 0x%x and %02d \n", \
			asyhp_ioaddr.startaddr, asyhp->asyhp_vect));

		asyhp->asyhp_dat = asyhp_ioaddr.startaddr+DAT;
		if ((cons_asyhp = asyhp_verify_init(asyhp->asyhp_dat)) == NULL)
			asyhp_portinit(unit);
		else {
			bcopy((caddr_t)cons_asyhp,(caddr_t)asyhp,sizeof(struct asyhp));
		}

		/* Attach interrupt handler */
		cm_intr_attach(cm_args.cm_key, asyhpintr, &asyhpdevflag,NULL);
	}
	splx(oldpl);
	return 0;
}


#define	ICNT		50

/*
 * void
 * asyhpintr(int) 
 *	process device interrupt
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	INTERRUPT TIME EXECUTION THREAD

 *	This procedure is called by system with interrupts off
 *	when the USART interrupts the processor. 
 */
/* ARGSUSED */
void
asyhpintr(int vect)
{	
	register struct strtty	*tp;	/* strtty structure */
	register struct asyhp	*asyhp;
	register int		unit;
	register ushort		c;
	ushort	 chars[ICNT];
	ushort	 charcount;
	unsigned char	interrupt_id;
	unsigned char	line_status;

	/* Is it a stray interrupt */
	if ((asyhptab == NULL) || (asyhp_num == 0))
		return;

	for (asyhp = &asyhptab[0], unit = 0; unit < asyhp_num; asyhp++, unit++) {

		/* Check if the vector matches, device present and interrupt pending */
		if ((asyhp->asyhp_vect != vect) || (!(asyhp->asyhp_flags & ASYHERE)))
			continue;

		if (unit >= asyhp_num)
			return;

#ifdef MERGE386
		/* needed for com port attachment */
		if (asyhp->mrg_data.com_ppi_func &&
		   (*asyhp->mrg_data.com_ppi_func)(&asyhp->mrg_data, -1))
			return;
#endif /* MERGE386 */

		for (;;) {

		interrupt_id = inb(asyhp->asyhp_isr) & 0x0f; 

		/* Check if device interrupt pending */
		if (interrupt_id & ISR_NOINTRPEND)
			break;
		line_status  = inb(asyhp->asyhp_lsr);
	
		tp = IASY_UNIT_TO_TP(asyhp_id, unit);

		if ((interrupt_id == ISR_RxRDY) || 
		    (interrupt_id == ISR_FFTMOUT) || 
		    (line_status == LSR_RCA) || 
		    (interrupt_id == ISR_RSTATUS)) {
	
			charcount = 0;

			drv_setparm(SYSRINT, 1);
			while (line_status & LSR_RCA) {
				c = inb(asyhp->asyhp_dat) & 0xff;

				switch (c) {
#ifndef NODEBUGGER
				case DEBUGGER_CHAR:
					if (!(asyhp->asyhp_flags & 
						ASYHP_SYSCON))
						break;

					(*cdebugger)(DR_USER, NO_FRAME);
					continue;
#endif
				case REBOOT_CHAR:
					if ((!(asyhp->asyhp_flags &
						ASYHP_SYSCON)) ||
					     !(console_security &
						 CONS_REBOOT_OK))
						break;

					drv_shutdown(SD_SOFT, AD_BOOT);
					continue;

				case PANIC_CHAR:
					if (!(asyhp->asyhp_flags & 
						ASYHP_SYSCON) ||
					    !(console_security &
						 CONS_PANIC_OK))
						break;

					drv_shutdown(SD_PANIC, AD_QUERY);
					continue;
#ifdef DKTEST
				case FLAVORS_CHAR:
					if (!(asyhp->asyhp_flags & ASYHP_SYSCON))
						break;
					invoke_flavors();
					continue;
#endif
				default:
					break;
				}

				if ((tp->t_cflag & CREAD) != CREAD)  {
					if (asyhp->asyhp_flags & ASY16550) {
						/* Clear the receiver fifo */
						while ((line_status = 
						inb(asyhp->asyhp_lsr)) & LSR_RCA) {
							inb(asyhp->asyhp_dat);
						}
					}
					continue;
				}

				if (line_status & LSR_OVRRUN)
					asyhp_rxoverun ++;

				if (line_status & (LSR_PARERR|LSR_FRMERR|LSR_BRKDET)) {
					if (line_status & LSR_PARERR)
						c |= PERROR;
					if (line_status & (LSR_FRMERR|LSR_BRKDET))
						c |= FRERROR;
				}

				chars[charcount++] = c;
				line_status = inb(asyhp->asyhp_lsr);

			} /* while end */

			if (charcount)
				asyhpProcessInChar(tp, chars, charcount);

		} else if (interrupt_id == ISR_TxRDY || 
		    (line_status & LSR_XHRE) &&
		    (tp->t_state & BUSY)) {

			drv_setparm(SYSXINT, 1);
			asyhp_outchars[unit] = 0;
			/* clear busy bit */
			tp->t_state &= ~BUSY;
			(void) asyhpproc(tp, T_OUTPUT);

		} else if (interrupt_id == ISR_MSTATUS)  
			asyhpmodem(tp);
		else
			return;

		} /* for end */
	}
}


/*
 * STATIC void
 * asyhpProcessInChar(struct strtty *, ushort *, ushort) 
 *	do the input processing for incoming characters.
 *
 * Calling/Exit State:
 *	This is called from the interrupt routine to handle all the per
 *	character processing stuff.
 */
STATIC void
asyhpProcessInChar(struct strtty *tp, ushort chars[], ushort cnt)
{
	ushort	c;
	ushort	charcount;


	if (!(tp->t_state & ISOPEN))
		return;

	for (charcount = 0; charcount < cnt; charcount++) {
	
		if (tp->t_in.bu_cnt < 3) {
			/*
			 * Not enough room in the buffer, try to ship
			 * this buffer upstream.
			 */
			if (iasy_input(tp, L_BUF)) {
				break;	/* Drop characters */
			}
		}

		c = chars[charcount];
	
		if (c & PERROR && !(tp->t_iflag & INPCK)) {
			if ((tp->t_cflag & (PARODD|PARENB)) != (PARODD|PARENB)
						&& ((c & 0377) != 0 ))
				c &= ~PERROR;
		}
	
		if (c & (FRERROR|PERROR|OVERRUN)) {
			if ((c & (FRERROR|0377)) == FRERROR) {
				if (!(tp->t_iflag & IGNBRK)) {
					/*
					 * ship upstream whatever we have 
					 * seen so far.
					 */
					iasy_input(tp, L_BUF);
					iasy_input(tp, L_BREAK);
			                continue;
				} else
					continue;
			} else if (tp->t_iflag & IGNPAR) {
				continue;
			}
	
		        if (tp->t_iflag & PARMRK) {
		        	/* Mark the Parity errors */
				PutInChar(tp, 0377);
				PutInChar(tp, 0);
				PutInChar(tp, c & 0377);
				continue;
			} else {
				/* Send up a NULL character */
				PutInChar(tp, 0);
				continue;
			}
		} else {	
			if (tp->t_iflag & ISTRIP) {
				c &= 0177;
			} else {
				if (c == 0377 && tp->t_iflag & PARMRK) {
					/*
					 * if marking parity errors, 
					 * this character gets doubled 
					 */
					PutInChar(tp, 0377);
					PutInChar(tp, 0377);
					continue;
				}
			}
		}
	
		if (tp->t_iflag & IXON) {
			/* if stopped, see if to turn on output */
			if (tp->t_state & TTSTOP) {
				if (c == tp->t_cc[VSTART] || tp->t_iflag & IXANY) {
					(void) asyhpproc(tp, T_RESUME);
				}
			} else {
				/* maybe we're supposed to stop output */
				if (c == tp->t_cc[VSTOP] && (c != 0)) {
					(void) asyhpproc(tp, T_SUSPEND);
				}
			}

			if ((c != 0) && ((c == tp->t_cc[VSTOP]) || (c == tp->t_cc[VSTART])))
				continue;
		}
	
		PutInChar(tp, (unsigned char) c);
	}

	/* Ship any characters upstream */
	iasy_input(tp, L_BUF);
}


/*
 * STATIC int
 * asyhpproc(struct strtty *, int) 
 *	low level device dependant operations
 *
 * Calling/Exit State:
 *	It is called at both task time by the line discipline routines,
 *	and at interrupt time by asyhpintr().
 *
 * Description:
 *	asyhpproc() handles any device dependent functions required upon
 *	suspending, resuming, blocking, or unblocking output; flushing
 *	the input or output queues; timing out; sending break characters,
 *	or starting output.
 */
STATIC int
asyhpproc(struct strtty *tp, int cmd)
{
	unsigned char line_ctl;
	unsigned char fcr_val;
	unsigned char line_status;
	struct asyhp *asyhp;
	pl_t	s;
	int	ret_val = 0;


	s = splstr();

	/*
 	 * get device number and control port
 	 */
	asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id, tp)];

	switch(cmd)  {
	case T_WFLUSH:			/* flush output queue */
		fcr_val = inb(asyhp->asyhp_isr);
		/* Flush XMIT FIFO */
		outb(asyhp->asyhp_isr, (fcr_val|0x04));
		/* abandon this buffer */
		tp->t_out.bu_cnt = 0;
		/* FALLTHROUGH */

	case T_RESUME:			/* resume output */
		/* Check if CTS is present in case of Hardware device */
 		if ((asyhp->asyhp_flags & (HWDEV|HWFLWO) == HWDEV)) {
			break;
		}
		tp->t_state &= ~TTSTOP;
		asyhpstartio(tp);
		break;

	case T_SUSPEND:			/* suspend output */
		tp->t_state |= TTSTOP;
		break;

	case T_BLOCK:			/* send stop char */
		tp->t_state &= ~TTXON;
		tp->t_state |= TBLOCK|TTXOFF;
		asyhpstartio(tp);
		break;

	case T_RFLUSH:			/* flush input queue */
		fcr_val = inb(asyhp->asyhp_isr);
		/* Flush RCVR FIFO */
		outb(asyhp->asyhp_isr, (fcr_val|0x02));
		tp->t_in.bu_cnt = IASY_BUFSZ;
		tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
		if (!(tp->t_state & TBLOCK))
			break;
		/* FALLTHROUGH */

	case T_UNBLOCK:			/* send start char */
		tp->t_state &= ~(TTXOFF|TBLOCK);
		tp->t_state |= TTXON;
		asyhpstartio(tp);
		break;

	case T_TIME:			/* time out */
		tp->t_state &= ~TIMEOUT;
		line_ctl = inb(asyhp->asyhp_lcr);
		outb(asyhp->asyhp_lcr, line_ctl & ~LCR_SETBREAK);
		asyhpstartio(tp);
		break;

	case T_BREAK:			/* send null for .25 sec */
		line_status = inb(asyhp->asyhp_lsr);
                while (!(line_status & LSR_XSRE)) {
                        drv_usecwait(10);
                        line_status = inb(asyhp->asyhp_lsr);
                }
		tp->t_state |= TIMEOUT;
		line_ctl = inb(asyhp->asyhp_lcr);
		outb(asyhp->asyhp_lcr, line_ctl | LCR_SETBREAK);
		(void) timeout(asyhpwakeup, (caddr_t)tp, HZ/4);
		break;

	case T_OUTPUT:			/* start output */
		asyhpstartio(tp);
		break;

	case T_CONNECT:			/* connect to the server */
		if (!(asyhp->asyhp_flags & ASYHERE)) {
			ret_val = ENXIO;
			break;
		}

		if ((tp->t_state & (ISOPEN|WOPEN)) == 0) {
			outb(asyhp->asyhp_lcr, LCR_DLAB);
			outb(asyhp->asyhp_dat, asyhpspdtab[B9600] & 0xff);
			outb(asyhp->asyhp_icr, asyhpspdtab[B9600] >> 8);
			outb(asyhp->asyhp_lcr, LCR_BITS8); 
			outb(asyhp->asyhp_mcr, MCR_DTR|MCR_RTS|MCR_OUT2);	

			tp->t_state &= ~BUSY;
			asyhp_reset(asyhp);
			outb(asyhp->asyhp_isr, FIFOEN);  /* fifo trigger set */
			asyhpparam(tp);

			if (IASY_HWDEV(tp->t_dev)) {
				asyhp->asyhp_flags |= HWDEV;
				asyhp->asyhp_flags &= ~(HWFLWO);
			} else {
				asyhp->asyhp_flags &= ~(HWDEV|HWFLWO);
			}
		}

		asyhpsetdtr(asyhp);

		if (tp->t_cflag & CLOCAL)
			tp->t_state |= CARR_ON;
		else {
			if (inb(asyhp->asyhp_msr) & MSR_DCD) {
				tp->t_state |= CARR_ON;
				iasy_carrier(tp);
			} else
				tp->t_state &= ~CARR_ON;
		}     

		break;

	case T_PARM:			/* output parameters */
		if (!(inb(asyhp->asyhp_lsr) & LSR_XSRE))
			/*
			 * Wait for one character time for Transmitter
			 * shift register to get empty. 
			 */
			ret_val = iasy_ctime(tp, 1);
		else
			ret_val = asyhpparam(tp);
		break;

	case T_DISCONNECT:

		outb (asyhp->asyhp_mcr, MCR_OUT2);
                drv_usecwait(asyhp_tick);
		break;

	case T_DATAPRESENT:
		break;

	case T_TRLVL1:
		outb(asyhp->asyhp_isr, 0x0);
		outb(asyhp->asyhp_isr, TRLVL1);
                break;

        case T_TRLVL2:
		outb(asyhp->asyhp_isr, 0x0);
		outb(asyhp->asyhp_isr, TRLVL2);
                break;

        case T_TRLVL3:
		outb(asyhp->asyhp_isr, 0x0);
		outb(asyhp->asyhp_isr, TRLVL3);
                break;

        case T_TRLVL4:
		outb(asyhp->asyhp_isr, 0x0);
		outb(asyhp->asyhp_isr, TRLVL4);
                break;

	case T_SWTCH:
		break;

	case T_FIRSTOPEN:
#ifdef MERGE386
		if (!portalloc(asyhp->asyhp_dat, asyhp->asyhp_dat + 0x7))
			ret_val = EBUSY;
#endif /* MERGE386 */
		break;

	case T_LASTCLOSE:
#ifdef MERGE386
		portfree(asyhp->asyhp_dat, asyhp->asyhp_dat + 0x7);
#endif /* MERGE386 */
		break;

	default:
		break;
	}; /* end switch */

	splx(s);

	return(ret_val);
}


/*
 * STATIC void
 * asyhpstartio(struct strtty *) 
 *	start output on an serial channel if needed.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Get a character from the character queue, output it to the
 *	channel and set the BUSY flag. The BUSY flag gets reset by
 *	asyhpintr() when the character has been transmitted.
 */
STATIC void
asyhpstartio(struct strtty *tp)
{
	struct t_buf *tbuf;
	struct asyhp *asyhp;
	int	unit;
	int	*charcount;


	unit = IASY_TP_TO_UNIT(asyhp_id, tp);
	asyhp = &asyhptab[unit];
	charcount = &asyhp_outchars[unit];
	
	/* busy or timing out? or stopped?? */
	if (tp->t_state & (TIMEOUT|BUSY)){
		return;			/* wait until a more opportune time */
	}

	if (tp->t_state & (TTXON|TTXOFF)) {
		if (tp->t_state & TTXON) {
			tp->t_state &= ~TTXON;
	 		if (asyhp->asyhp_flags & HWDEV) {
				char mcr = inb(asyhp->asyhp_mcr);
				outb(asyhp->asyhp_mcr, mcr | MCR_RTS);
 			} else { 
 				outb(asyhp->asyhp_dat, tp->t_cc[VSTART]);
				++(*charcount);
				tp->t_state |= BUSY;
			}
		} else {
			tp->t_state &= ~TTXOFF;
 			if (asyhp->asyhp_flags & HWDEV) {
				char mcr = inb(asyhp->asyhp_mcr);
				outb(asyhp->asyhp_mcr, mcr & ~MCR_RTS);
 			} else { 
				outb(asyhp->asyhp_dat, tp->t_cc[VSTOP]);
				++(*charcount);
				tp->t_state |= BUSY;
			}
		}
		return;
	}

	/* Don't output data while output is blocked */
	if (tp->t_state & TTSTOP)
		return;

	tbuf = &tp->t_out;
	while ((*charcount) < TX_FIFOSIZE) {
		if (!(tbuf->bu_ptr && tbuf->bu_cnt)) {
			if (!(CPRES & iasy_output(tp)))
				return;
		}

		tp->t_state |= BUSY;
		outb(asyhp->asyhp_dat, (char)(*(tbuf->bu_ptr++)));
		++(*charcount);
		--(tbuf->bu_cnt);
	}
}


/*
 * STATIC int
 * asyhpparam(struct strtty *)
 * 
 * Calling/Exit State:
 *	- Return 0 on success, otherwise return appropriate errno.
 *
 * Description:
 *	Param detects modifications to the line characteristics and reprograms 
 */
STATIC int
asyhpparam(struct strtty *tp)
{
	struct asyhp *asyhp;
	unsigned int flags;	/* To ease access to c_flags    */
	int	s;		/* conversion table index for COM baud rate */
	int	x;
	pl_t	oldpri;


	asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id, tp)];
	flags = tp->t_cflag;
	x = (ICR_TIEN|ICR_SIEN|ICR_MIEN|ICR_RIEN);

	if (tp->t_cflag & CLOCAL)
		tp->t_state |= CARR_ON;
	else
		asyhpmodem(tp);

	outb(asyhp->asyhp_icr, x);

	x = inb(asyhp->asyhp_lcr);
	x &= ~(LCR_WLS0|LCR_WLS1|LCR_STB|LCR_PEN|LCR_EPS);
	if ((tp->t_cflag & CSTOPB) == CSTOPB)
		x |= LCR_STB;  

	if ((tp->t_cflag & CS6) == CS6)
		x |= LCR_BITS6;
	if ((tp->t_cflag & CS7) == CS7 )
		x |= LCR_BITS7;
	if ((tp->t_cflag & CS8) == CS8 )
		x |= LCR_BITS8;
	if ((tp->t_cflag & PARENB) == PARENB )
		x |= LCR_PEN;
	if ((tp->t_cflag & PARODD) == 0)
		x |= LCR_EPS;
	outb(asyhp->asyhp_lcr, x);

	/*
	 * Set the baud rate:
	 * Bounds check baud rate index and then verify that the chip supports
	 * that speed.  Then program it. Default to 9600 baud.
	 */
	s = flags & CBAUD;
	if (s > MAXBAUDS || s < 0)
		s = B9600;

	if (s == 0) {
		/*
		 * only disable modem line
		 */
		if (!(tp->t_cflag & CLOCAL)) {
			(void) asyhpproc(tp, T_DISCONNECT);
			return(0);
		} else {
			return(EINVAL);
		}
	}

	oldpri = splstr();
	asyhp_prog(tp, s);
	splx(oldpri);

	return(0);
}


/*
 * STATIC void
 * asyhpmodem(struct strtty *)
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	called for Modem Status register changes while in DOSMODE.
 */
STATIC void
asyhpmodem(struct strtty *tp)
{
	int	msr;
	struct asyhp *asyhpp;


	asyhpp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id, tp)];

	/* this resets the interrupt */
        drv_usecwait(asyhp_tick);
 	msr = inb(asyhpp->asyhp_msr);

	if (asyhpp->asyhp_flags & ASY82510) {
		outb(asyhpp->asyhp_msr, msr & 0xF0);
	}

 	if ((asyhpp->asyhp_flags & HWDEV)) {
 		if ((msr & MSR_CTS)) {
 			asyhpp->asyhp_flags |= HWFLWO;
			asyhpproc(tp, T_RESUME);
 		} else {
 			asyhpp->asyhp_flags &= ~HWFLWO;
			asyhpproc(tp, T_SUSPEND);
		}
	}

	if (!(tp->t_cflag & CLOCAL)) {
		if (msr & MSR_DCD) {
			if (!(tp->t_state & CARR_ON)) {
				iasy_carrier(tp);
				tp->t_state |= CARR_ON;
			}
		} else {
			if ((tp->t_state & CARR_ON) &&
			    (tp->t_state & ISOPEN)) {      
				iasy_hup(tp);
			}
			tp->t_state &= ~CARR_ON;
		}
	}
}


/*
 *
 * LOW LEVEL UTILITY ROUTINES
 *
 */

/*
 * STATIC void
 * asyhpwakeup(struct strtty *) 
 *	release transmitter output.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	It is used by the TCSBRK ioctl command.  After .25 sec
 *	timeout (see case BREAK in asyhpproc), this procedure is called.
 *
 */
STATIC void
asyhpwakeup(struct strtty *tp)
{
	asyhpproc(tp, T_TIME);
}


/*
 * STATIC void 
 * asyhpsetdtr(struct asyhp *) 
 *	assert the DTR line for this port
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
asyhpsetdtr(struct asyhp *asyhp)
{
	int	mcr;


	mcr = inb(asyhp->asyhp_mcr);
	outb(asyhp->asyhp_mcr, (mcr|MCR_DTR));
        drv_usecwait(asyhp_tick);
}


/*
 * STATIC void
 * asyhp_prog(struct strtty *, ushort)
 *	This procedure programs the baud rate generator.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
asyhp_prog(struct strtty *tp, ushort speed)
{
	ushort y;
	unsigned char x;
	struct asyhp *asyhp;


	asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id, tp)];
	x = inb(asyhp->asyhp_lcr);
	outb(asyhp->asyhp_lcr, x | LCR_DLAB);
	y = asyhpspdtab[speed];
	outb(asyhp->asyhp_dat, y & 0xff);
	outb(asyhp->asyhp_icr, y >> 8);
	outb(asyhp->asyhp_lcr, x);	
}


/*
 * STATIC void
 * asyhp_reset(struct asyhp *)
 *	This procedure does the initial reset on an COM USART.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
asyhp_reset(struct asyhp *asyhp)
{
	(void)inb(asyhp->asyhp_isr);
	(void)inb(asyhp->asyhp_lsr);
	(void)inb(asyhp->asyhp_msr);
	(void)inb(asyhp->asyhp_dat);
	
	outb(asyhp->asyhp_lcr, LCR_DLAB);
	outb(asyhp->asyhp_dat, asyhpspdtab[B9600] & 0xff);		
	outb(asyhp->asyhp_icr, asyhpspdtab[B9600]  >> 8);		
	outb(asyhp->asyhp_lcr, LCR_BITS8); 
	outb(asyhp->asyhp_mcr, MCR_DTR|MCR_RTS|MCR_OUT2);	
        drv_usecwait(asyhp_tick);
}


/*
 * STATIC void
 * asyhphwdep(queue_t *, mblk_t *, struct strtty *)
 *	Hardware dependent ioctl support.
 *
 * Calling/Exit State:
 *	- q points to the write queue pointer.
 *	- bp contains the ioctl or ioctl data not understood by the DI code.
 */
/* ARGSUSED */
STATIC void
asyhphwdep(queue_t *q, mblk_t *bp, struct strtty *tp)
{	
	struct iocblk	*ioc;
	struct asyhp	*asyhp;
	int 		s, cmd;
	int		modval=0;
	uint		id;
	caddr_t		arg;
	unsigned char	msr, mcr;
	cred_t		*credp;
	struct copyreq	*cpreq;
	struct copyresp	*cpresp;

	/* LINTED pointer alignment */
	ioc = (struct iocblk *)bp->b_rptr;
	asyhp = &asyhptab[IASY_TP_TO_UNIT(asyhp_id, tp)];

	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		switch (ioc->ioc_cmd) {
		case TIOCMGET:
			if(!bp->b_cont) /* Fall thru and send an M_IOCNACK */
				break;
			s = SPL();
			msr = inb(asyhp->asyhp_msr);
			mcr = inb(asyhp->asyhp_mcr);
			splx(s);

			if (mcr & MCR_DTR)
				modval |= TIOCM_DTR;
			if (mcr & MCR_RTS)
				modval |= TIOCM_RTS;
			if (msr & MSR_DCD)
				modval |= TIOCM_CD;
			if (msr & MSR_RI)
				modval |= TIOCM_RI;
			if (msr & MSR_DSR)
				modval |= TIOCM_DSR;
			if (msr & MSR_CTS)
				modval |= TIOCM_CTS;

			if (ioc->ioc_count == TRANSPARENT) {
				bp->b_datap->db_type = M_COPYOUT;
				bp->b_cont->b_datap->db_type = M_DATA;
				cmd = ioc->ioc_cmd;
				id = ioc->ioc_id;
				credp = ioc->ioc_cr;
				arg = *((caddr_t*)bp->b_cont->b_rptr);

				cpreq = (struct copyreq *)bp->b_rptr;
				cpreq->cq_cmd = cmd;
				cpreq->cq_id = id;
				cpreq->cq_addr = arg;
				cpreq->cq_size = sizeof(int);
				*((int*)bp->b_cont->b_rptr) = modval;
				(void) putnext(RD(q), bp);
			} else {
				*((int*)bp->b_cont->b_rptr) = modval;
				bp->b_cont->b_wptr = bp->b_cont->b_rptr + sizeof(int);
				ioc->ioc_count = sizeof(int);
				bp->b_datap->db_type = M_IOCACK;
				ioc->ioc_rval = 0;
				ioc->ioc_error = 0;
				(void) putnext(RD(q), bp);
			}
			return;	
		case TIOCMSET:
			if (ioc->ioc_count == TRANSPARENT) {
				bp->b_datap->db_type = M_COPYIN;
				bp->b_cont->b_datap->db_type = M_DATA;
				cmd = ioc->ioc_cmd;
				id = ioc->ioc_id;
				credp = ioc->ioc_cr;
				arg = *((caddr_t*)bp->b_cont->b_rptr);
				
				cpreq = (struct copyreq *)bp->b_rptr;
				cpreq->cq_cmd = cmd;
				cpreq->cq_id = id;
				cpreq->cq_addr = arg;
				cpreq->cq_size = sizeof(int);
				(void) putnext(RD(q), bp);
			} else {
				mcr = inb(asyhp->asyhp_mcr);
				modval = *((int*)bp->b_cont->b_rptr);
				if (modval & TIOCM_DTR)
					mcr |= MCR_DTR;
				else
					mcr &= ~MCR_DTR;
				s = SPL();
				outb(asyhp->asyhp_mcr, mcr);
				splx(s);
				bp->b_datap->db_type = M_IOCACK;
				ioc->ioc_count = 0;
				ioc->ioc_rval = 0;
				ioc->ioc_error = 0;
				(void) putnext(RD(q), bp);
			}
			return;
		default:
			break;
		}
		break;

	case M_IOCDATA:
		cpresp = (struct copyresp *)bp->b_rptr;

		cmd = cpresp->cp_cmd;
		id  = cpresp->cp_id;
		credp = cpresp->cp_cr;
		if (cpresp->cp_rval == 0) { 
			/* Data has been successfully copied */
			if (cmd == TIOCMSET) {
				mcr = inb(asyhp->asyhp_mcr);
				modval = *((int*)bp->b_cont->b_rptr);
				if (modval & TIOCM_DTR)
					mcr |= MCR_DTR;
				else
					mcr &= ~MCR_DTR;

				if (modval & TIOCM_RTS)
					mcr |= MCR_RTS;
				else
					mcr &= ~MCR_RTS;

				s = SPL();
				outb(asyhp->asyhp_mcr, mcr);
				splx(s);
			}
			bp->b_datap->db_type = M_IOCACK;
			ioc = (struct iocblk *)bp->b_rptr;
			ioc->ioc_cmd = cmd;
			ioc->ioc_id = id;
			ioc->ioc_cr = credp;

			ioc->ioc_error = 0;
			ioc->ioc_rval = 0;
			ioc->ioc_count = 0;
			(void) putnext(RD(q), bp);

		} else { /* Data not successfully copied, freemsg and return */
			cmn_err(CE_NOTE,"!asyhp:Data Copy failed !! \n");
			freemsg(bp);
		}
		return;
	default:

		/*
		 *+ An unknown message type.
		 */
 		cmn_err(CE_WARN, "!asyhp: Unknown message type obtained\n");
                freemsg(bp);
                return;
        }

#ifdef MERGE386
	if (!com_ppi_strioctl(q, bp, &asyhp->mrg_data, ioc->ioc_cmd))
#endif /* MERGE386 */
	{
		/* NAK unknown ioctls */
		bp->b_datap->db_type = M_IOCNAK;
		ioc->ioc_error = EINVAL;
		ioc->ioc_rval = -1;
		ioc->ioc_count = 0;
		(void) putnext(RD(q), bp);
	}
}


/*
 *
 * debugger/console support routines.
 *
 */

/*
 * STATIC dev_t
 * asyhpcnopen(minor_t, boolean_t, const char *)
 * 
 * Calling/Exit State:
 *	Return device minor number.
 */
/* ARGSUSED */
STATIC dev_t
asyhpcnopen(minor_t minor, boolean_t syscon, const char *params)
{
	extern major_t iasy_major;	/* major num. of serial port device */
	struct asyhp	*asyhp;
	int		unit;
	int		rval;

	if (minor < asyhp_sminor ||
	    minor > (asyhp_sminor + IASY_UNIT_TO_MINOR(asyhp_num) - 1))
		return NODEV;

	/* Check if port initialized */
	if (!asyhpinitialized) {
		/* Is there atleast one port ? */
		if( asyhp_num == 0)
			return(ENODEV);

		/* Set flag to inidicate console init */
		asyhpinitialized = ASYHP_CONSOLE_INIT;
		if ((rval = asyhp_tabinit()) != 0)
			return(ENODEV);
	}

	unit = IASY_MINOR_TO_UNIT(minor - asyhp_sminor);
	asyhp = &asyhptab[unit];

	asyhp_reset(asyhp);

	if (IASY_HWDEV(minor)) {
		asyhp->asyhp_flags |= HWDEV;
		if (inb(asyhp->asyhp_msr) & MSR_CTS) {
			asyhp->asyhp_flags |= HWFLWO;
		} else {
			asyhp->asyhp_flags &= ~HWFLWO;
		}
	} else {
		asyhp->asyhp_flags &= ~(HWDEV|HWFLWO);
	}

	if (syscon)
		asyhp->asyhp_flags |= ASYHP_SYSCON;

	return makedevice(iasy_major, minor);
}


/*
 * STATIC void
 * asyhpcnclose(minor_t, boolean_t)
 *
 * Calling/Exit State:
 *	- None
 */
STATIC void
asyhpcnclose(minor_t minor, boolean_t syscon)
{
	struct asyhp	*asyhp;
	int		unit;

	ASSERT(asyhpinitialized);
	ASSERT(minor >= asyhp_sminor &&
	       minor <= (asyhp_sminor + IASY_UNIT_TO_MINOR(asyhp_num)));

	unit = IASY_MINOR_TO_UNIT(minor - asyhp_sminor);
	asyhp = &asyhptab[unit];

	if (syscon)
		asyhp->asyhp_flags &= ~ASYHP_SYSCON;
}


/*
 * STATIC int
 * asyhpcnputc(minor_t, int)
 *
 * Calling/Exit State:
 *	Return 1 to indicate that the character is successfully
 *	transmitted or the device was busy.
 *
 * Description:
 *	put a character out the first serial port.
 *	Do not use interrupts.  If char is LF, put out LF, CR.
 */
STATIC int
asyhpcnputc(minor_t minor, int c)
{
	int	p_asyhp;	/* start i/o address */


	ASSERT(asyhpinitialized);
	ASSERT((minor >= asyhp_sminor) &&
	       (minor <= (asyhp_sminor + IASY_UNIT_TO_MINOR(asyhp_num))));
	
	p_asyhp = asyhptab[IASY_MINOR_TO_UNIT(minor) - asyhp_sminor].asyhp_dat;

	if (inb(p_asyhp + ISR) & 0x38)
		return(1);

	/*
	 * wait for xmit to finish
	 */
	while ((inb(p_asyhp + LSR) & LSR_XHRE) == 0) { 
		if ((inb(p_asyhp + MSR) & MSR_DCD) == 0)
			return(1);

		drv_usecwait(10);
	}

	/*
	 * put the character out
	 */
	outb(p_asyhp + DAT, c);

	return(1);
}


/*
 * STATIC int
 * asyhpcngetc(minor_t)
 *	get a character from the first serial port.
 *
 * Calling/Exit State:
 *	If no character is available, return -1.
 *
 * Remarks:
 *	Run in polled mode, no interrupts.
 */
STATIC int
asyhpcngetc(minor_t minor)
{
	int	p_asyhp;	/* start i/o address */
	int	c;


	ASSERT(asyhpinitialized);
	ASSERT((minor >= asyhp_sminor) &&
	       (minor <= (asyhp_sminor + IASY_UNIT_TO_MINOR(asyhp_num))));
	
	p_asyhp = asyhptab[IASY_MINOR_TO_UNIT(minor) - asyhp_sminor].asyhp_dat;

	if ((inb(p_asyhp + ISR) & 0x38) || (inb(p_asyhp + LSR) & LSR_RCA) == 0) {
		return -1;
	}

	c = inb(p_asyhp + DAT);

#ifndef NODEBUGGER
	if (c == DEBUGGER_CHAR) {
		(*cdebugger)(DR_USER, NO_FRAME);
		return -1;
	}
#endif

	return c;
}


/*
 * STATIC void
 * asyhpcnsuspend(minor_t)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
asyhpcnsuspend(minor_t minor)
{
	struct asyhp *asyhp =
		&asyhptab[IASY_MINOR_TO_UNIT(minor) - asyhp_sminor];
	uchar_t lcr;

	ASSERT(asyhpinitialized);
	lcr = inb(asyhp->asyhp_lcr);
	outb(asyhp->asyhp_lcr, lcr & ~LCR_DLAB);
	asyhp->asyhp_save_icr = inb(asyhp->asyhp_icr);
	outb(asyhp->asyhp_icr, 0);
	outb(asyhp->asyhp_lcr, lcr);
}


/*
 * STATIC void
 * asyhpcnresume(minor_t)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
asyhpcnresume(minor_t minor)
{
	struct asyhp *asyhp =
		&asyhptab[IASY_MINOR_TO_UNIT(minor) - asyhp_sminor];
	uchar_t lcr;

	ASSERT(asyhpinitialized);
	lcr = inb(asyhp->asyhp_lcr);
	outb(asyhp->asyhp_lcr, lcr & ~LCR_DLAB);
	outb(asyhp->asyhp_icr, asyhp->asyhp_save_icr);
	outb(asyhp->asyhp_lcr, lcr);
}

/*
 *
 *	STATIC int
 *	asyhp_getbase(void)
 *	
 *	Return value :	The actual number of boards installed on the system
 *	Description  : 	Check if board detected, and verify if the it's 
 *			a Serial port by performing a loop back test.
 */

STATIC int
asyhp_getbase(void)
{
	asyhp_base_t 	*pbase;	
	int 		i;
	ushort_t 	asyhp_bnum = 0;
	ushort_t	asyhp_iercopy;

	DEBUG1(("ASYHP: asyhp_getbase() entered \n"));
	
	pbase = asyhp_base;

	for (i = 0; i < MAX_CONPORTS; i++, pbase++) {

		/* Check 1 : Existance of port */
		asyhp_iercopy = inb(pbase->io_base+ICR);	
		
		/* Clear ICR register and then read it back */
		outb(pbase->io_base+ICR,0);
		if(inb(pbase->io_base+ICR))
			continue;

		/* Restore the ICR contents */
		outb(pbase->io_base+ICR,asyhp_iercopy);

	   	/* Check 2: Verify existance of COMM port */
		if(asyhp_verifyport(pbase->io_base)) {
			DEBUG1(("ASYHP: Verified asyhp port OK \n"));
			asyhp_bnum++;
		}
	}
	return (asyhp_bnum);	/* Number of boards detected */
}

/*
 *
 *	STATIC int
 *	asyhp_verifyport(ushort io_base)
 *	
 *	Return value :	True on success 
 *	Description  : 	This function checks if the port is present
 *			on the specified io base by performing a loopback
 *			test. (i.e By Putting the UART in loopback mode,
 *			Setting the Handshake bits in the MCR and checking 
 *			for valid responses in the modem status register.
 */

STATIC int 
asyhp_verifyport(ushort io_base)
{
	ushort_t mcr_copy;

	/* Copy of modem contol register */
	mcr_copy = inb(io_base+MCR);

	/* Put UART in loopback mode */
	outb(io_base+MCR,SET_LBKMODE);

	/* Check for valid response */
	if((inb(io_base+MSR) & 0xF0) == LBK_EXPECTED) { 
		DEBUG1(("ASYHP: Detected port thru' loopback\n"));
		outb(io_base+MCR, mcr_copy);
		return(1);
	} else {
		outb(io_base+MCR, mcr_copy);
		return(0);
	}
}

/* 
 * STATIC struct asyhp *
 * asyhp_verify_init(ushort_t io_base)
 *
 * Given an I/O port, try and verify that the COM port has been previously
 * detected during the console verification.
 */

struct asyhp *
asyhp_verify_init(ushort_t io_base)
{
	int unit;
	struct asyhp *cons_asyhp;
	
	if ( cons_asyhp_num == 0 || !cons_asyhptab )
		return NULL;
	for (unit = 0; unit < cons_asyhp_num; unit++) {
		cons_asyhp = &cons_asyhptab[unit];
		if (cons_asyhp->asyhp_dat == io_base)
			return cons_asyhp;
	}
	return NULL;
}

/* 
 * STATIC void
 * asyhp_portinit(int unit)
 *
 * Verify the existence of a serial port. If present, identify the 
 * chararcteristics and allocate the necessary buffers.
 */

STATIC void
asyhp_portinit(int unit)
{

	struct asyhp	*asyhp;			/* Temp ptr to asyhp struct */

	ASSERT(unit < asyhp_num);

	asyhp = &asyhptab[unit];

	/* Initialise the UART register port addresses */
	asyhp->asyhp_isr = asyhp->asyhp_dat+ISR;

	/* 
	 * Bit 4 & 5 of ISR are wired low. If bit 4 or 5 
	 * appears on inb(), board is not there.
	 *
	 * Note: A more robust method of detecting the Serial port
	 * is by writting a pattern and reading the same (0x55)
	 */ 

	if ((inb(asyhp->asyhp_isr) & 0x30)) {
		/* no serial adapter here */
		asyhp->asyhp_flags &= ~ASYHERE;
		return;
	}

	asyhp->asyhp_icr = asyhp->asyhp_dat+ICR;
	asyhp->asyhp_lcr = asyhp->asyhp_dat+LCR;
	asyhp->asyhp_mcr = asyhp->asyhp_dat+MCR;
	asyhp->asyhp_lsr = asyhp->asyhp_dat+LSR;
	asyhp->asyhp_msr = asyhp->asyhp_dat+MSR;

	asyhp->asyhp_flags |= ASYHERE;

	/*
	 * Reset all registors for soft restart.
	 */
	outb(asyhp->asyhp_icr, 0x00);
	outb(asyhp->asyhp_isr, 0x00);
	outb(asyhp->asyhp_lcr, 0x00);
	outb(asyhp->asyhp_mcr, 0x00);

	outb(asyhp->asyhp_isr, 0xc1);
	if ((inb(asyhp->asyhp_isr) & 0xc0) == 0xc0) {
		/*
		 *+ The system is configured with the NS16550 or
		 *+ compatible chip.
		 */
		DEBUG1(("ASYHP: Found National 16550 UART \n"));
		++asyhp_alive;
		asyhp->asyhp_flags |= ASY16550;	/* 16550 chip present */
		outb(asyhp->asyhp_isr, 0x0);	/* clear both fifos */
		outb(asyhp->asyhp_isr, FIFOEN);	/* fifo trigger set */

		/*
		 * Reset all interrupts for soft restart.
		 */
		(void)inb(asyhp->asyhp_isr);
		(void)inb(asyhp->asyhp_lsr);
		(void)inb(asyhp->asyhp_msr);
		(void)inb(asyhp->asyhp_dat);
	} else {
		/*
		 *+ The system is not configured with the NS16550 or
		 *+ compatible chip.
		 */
		cmn_err(CE_NOTE, 
			"ASYHP: asyhp initialization failed : National 16550 NOT found");
	}

#ifdef MERGE386
		asyhp->mrg_data.baseport = asyhp->asyhp_dat;
#endif
	/*
	 * Enable receive interrupts, to allow reciept of special
	 * characters, such as kernel debugger invocation, even
	 * when driver not open.
	 */
	if(asyhpinitialized == ASYHP_CONSOLE_INIT)
		outb(asyhp->asyhp_icr, 0);
	else
		outb(asyhp->asyhp_icr, ICR_RIEN);
}

/*
 * STATIC int
 * asyhp_memalloc(void)
 *
 * Allocate memory for asyhp struct depending on the type of 
 * init. 
 */

STATIC int
asyhp_memalloc(void)
{

	ASSERT(asyhpinitialized);

	if (asyhpinitialized & ASYHP_CONSOLE_INIT) {
		/* Console init sequence */
		if ((asyhptab = (struct asyhp *) 
		consmem_alloc(asyhp_num*sizeof(struct asyhp),KM_NOSLEEP))==NULL)
			return(ENOMEM);
		bzero(asyhptab,asyhp_num*sizeof(struct asyhp));

		if ((asyhp_outchars = (int *) 
		consmem_alloc(asyhp_num*sizeof(int),KM_NOSLEEP))==NULL)
			return(ENOMEM);
		bzero(asyhp_outchars,asyhp_num*sizeof(int));
	} else {
		/* Normal init sequence */
		if ((asyhptab = (struct asyhp *)
		kmem_zalloc(asyhp_num*sizeof(struct asyhp), KM_NOSLEEP)) == NULL)
			return(ENOMEM);
		if ((asyhp_outchars = (int *)
		kmem_zalloc(asyhp_num*sizeof(int), KM_NOSLEEP)) == NULL)
			return(ENOMEM);
	}
	return 0;
}

/*
 * STATIC int 
 * asyhp_tabinit(void)
 *
 * Called from asyhpcnopen for early initialization only  
 */

STATIC int 
asyhp_tabinit(void)
{
	int		unit;
	struct asyhp	*asyhp;			/* Temp ptr to asyhp struct */
	asyhp_base_t 	*tp;
	int 	rval;

	if ((rval = asyhp_memalloc()) != 0)
		return (ENOMEM);

	/* 
	 * Init routine should have already picked up the base 
	 * address of the available ports thru' asyhp_getbase()
	 */
	tp = asyhp_base;
	for (unit = 0; unit < asyhp_num; unit++) {
		asyhp = &asyhptab[unit];
		asyhp->asyhp_vect = tp->int_vect;
		asyhp->asyhp_dat =  tp->io_base;

		/* 
		 * Initialize every port based on the base 
		 * address detected from the BIOS.
		 */
		(void) asyhp_portinit(unit);
		tp++;
	}
	return 0;
}

