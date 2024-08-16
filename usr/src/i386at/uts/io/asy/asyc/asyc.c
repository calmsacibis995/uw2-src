/*	Copyright (c) 1990, 1991, 1992, 1993, 1994, 1995 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/asy/asyc/asyc.c	1.49"
#ident	"$header: $"


/*	Copyright (c) 1991 Intel Corporation	*/
/*	All Rights Reserved	*/

/*	INTEL CORPORATION PROPRIETARY INFORMATION	*/

/*	This software is supplied to AT & T under the terms of a license   */
/*	agreement with Intel Corporation and may not be copied nor         */
/*	disclosed except in accordance with the terms of that agreement.   */


/*
 * This driver forms the hardware dependent part of the serial driver.
 * It uses the iasy driver interface to communicate to the modules upstream.
 *
 * It supports National 16450 and compatible chips, the lowest common
 * denominator UART.
 */

#include <io/asy/asyc/asyc.h>
#include <io/asy/iasy.h>
#include <io/conf.h>
#include <io/conssw.h>
#include <io/stream.h>
#include <io/stropts.h>
#include <io/strtty.h>
#include <io/termio.h>
#include <io/tty.h>
#include <mem/kmem.h>
#include <svc/errno.h>
#include <svc/uadmin.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/kdb/xdebug.h>
#include <util/param.h>
#include <util/types.h>
#include <io/autoconf/resmgr/resmgr.h>
#include <io/autoconf/confmgr/confmgr.h>
#include <io/autoconf/confmgr/cm_i386at.h>

#ifdef DEBUGSTR
#include <io/strlog.h>
#include <io/log/log.h>

#define	DSTRLOG(x)	strlog x 
#else
#define	DSTRLOG(x)
#endif /* DEBUGSTR */

#include <io/ddi.h>	/* must come last */

/* Character to enter kernel debugger (if on console device) */
#define DEBUGGER_CHAR	('K' - '@')	/* Ctrl-K */

/* Character to panic the mahcine (if on console device) */
#define PANIC_CHAR	('_' - '@')	/* Ctrl-hyphen */

/* Character to reboot, via "init 6" (if on console device) */
#define REBOOT_CHAR	('\\' - '@')	/* Ctrl-backslash */

#define ASYC_CONSMEM_ALLOC	0x1
#define ASYC_KMEM_ALLOC		0x2

#ifdef DKTEST
/* Character to invoke flavors (if on console device) */
#define FLAVORS_CHAR	('F' - '@')	/* Ctrl-F */
extern void invoke_flavors(void);
#endif /* DKTEST */

#ifdef INEXT
#undef INEXT
#endif /* INEXT */
#define	ONEXT(x)	(((x) < (ushort)(OSIZE - 1)) ? ((x) + 1) : 0)
#define	INEXT(x)	(((x) < (ushort)(ISIZE - 1)) ? ((x) + 1) : 0)


#ifdef	DEBUG
STATIC	int	asyc_debug = 0;
#define	DEBUG1(a)	if (asyc_debug == 1) printf a	/* General debugs */
#define	DEBUG2(a)	if (asyc_debug <= 2) printf a	/* Allocations */
#define	DEBUG3(a)	if (asyc_debug >= 3) printf a	/* M_CTL stuff ! */
#else
#define	DEBUG1(a)
#define	DEBUG2(a)
#define	DEBUG3(a)
#endif	/* DEBUG */


void		asyc_asyinit(void);
void		asycintr(int);
int		asycstart(void);

STATIC void	asycProcessInChar(struct strtty *, uchar_t, uchar_t);
STATIC void	asycpoll(int);
STATIC int	asycproc(struct strtty *, int);
STATIC void	asycstartio(struct strtty *);
STATIC int	asycparam(struct strtty *);
STATIC void	asycmodem(struct strtty *);
STATIC void	asycwakeup(struct strtty *);
STATIC void	asyct_prog(struct strtty *, ushort);
STATIC void	asycu_reset(struct asyc *);
STATIC void	asycsetdtr(struct asyc *);
STATIC void	asychwdep(queue_t *, mblk_t *, struct strtty *);

STATIC dev_t	asyccnopen(minor_t, boolean_t, const char *);
STATIC void	asyccnclose(minor_t, boolean_t);
STATIC int	asyccnputc(minor_t, int);
STATIC int	asyccngetc(minor_t);
STATIC void	asyccnsuspend(minor_t);
STATIC void	asyccnresume(minor_t);

STATIC int 	asyc_getbase(void);
STATIC int 	asyc_memalloc(ushort_t);
STATIC void 	asyc_portinit(int,ushort_t enable_interrupts);
STATIC struct asyc *asyc_verify_init(ulong_t);
STATIC int 	asyc_verifyport(ushort);

struct asyc	*cons_asyctab = NULL;
struct asyc_aux *cons_asyc_bufp = NULL;
int 		cons_asyc_num = 0;

struct asyc 	*asyctab  = NULL;	/* Ptr to asyc structs */
struct asyc_aux *asyc_bufp = NULL ;	/* Ptr to buffer struct */
unsigned int 	asyc_num = 0;		/* number of configured ports */
int		asycdevflag = 0;
int		asyc_rxoverun;		/* Receive overrun indicator */
int             asyc_softrxoverun;	/* Ring buffer overun indicator */
int		asyc_wdexp = 0;
int		asyc_maxwdtick = 50;

extern unsigned int asyc_sminor;	/* start minor number */
extern struct strtty asy_tty[];		/* strtty struct for each port 
					 * iasy_tty changed to asy_tty for 
					 * merge.
					 */
extern asyc_base_t asyc_base[];

/*
 * Baud rate table. Indexed by #defines found in io/termios.h
 */

#define MAXBAUDS	17

ushort asycspdtab[] = {
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

struct strtty	*asyc_tty;		/* strtty structs for asyc ports */
unsigned int	asyc_alive = 0;
unsigned int	asycinitialized = 0;
int		asyc_id;		/* asyc ports id == start minor num */
int             asyc_tick=10;           /* time to wait for CD to settle*/


struct conssw	asycconssw = {
	asyccnopen, asyccnclose, asyccnputc, asyccngetc,
	asyccnsuspend, asyccnresume
};

#ifndef lint
static char asyc_copyright[] = "Copyright 1991 Intel Corporation xxxxxx";
#endif /* lint */

/*
 * void
 * asyc_asyinit(void)
 *	initialize driver with interrupts off
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	This is called before the interrupts are turned on to initalize
 *	the device.  Get the number of boards detected, and register the
 * 	hardware dependent routines.
 */

void
asyc_asyinit(void)
{

	DEBUG1(("ASYC: In asyc_asyinit() \n"));

	/* Check if initialised earlier */
	if (asycinitialized)
		return;

	/* Get the Number of boards detected */
	if((asyc_num = asyc_getbase()) == 0) 
		return;

	DEBUG1(("ASYC: The number of boards detected is %d \n",asyc_num));

	/*  Register terminal server. */
	asyc_id = iasy_register(asyc_sminor, asyc_num, asycproc, 
				asychwdep, &asycconssw);
	asyc_tty = IASY_UNIT_TO_TP(asyc_id, 0);
}

/*
 * int
 * asycstart(void)
 *
 * Check the type of init and reinitialize the asyc struct and
 * initialize the ports
 */

int
asycstart(void)
{	
	int		unit, rval;
	struct asyc	*asyc;			/* Temp ptr to asyc struct */
	struct asyc_aux	*bufp;			/* Temp ptr to asyc_aux   */
	struct asyc	*cons_asyc;

	struct	cm_addr_rng	asyc_ioaddr;	/* Board start and end addr */
	cm_args_t		cm_args;	/* ConfMgr struct */
	int			cm_ver_no;	/* ConfMgr version number */
	int 			oldpl;
	int 	init_state;


	DEBUG1(("ASYC: asycstart() entered \n"));

	/* Check if port initialised through the config manager database */
	if (asycinitialized & ASYC_COM_INIT) {
		DEBUG1(("In asycstart() already initialised \n"));
		return;
	}

	/* 
	 * Check if port initialized thru' console init. 
	 * Reconfigure the port with the values from the
  	 * configuration manager data base.
	 */
	if (asycinitialized & ASYC_CONSOLE_INIT) {

		ASSERT(asyctab); ASSERT(asyc_bufp);

		/* Override the console initialization */
		asycinitialized &= ~ASYC_CONSOLE_INIT;

		/* Cache the memory allocated from the console entry points 
		 * since we don't have a way to free this memory.
		 */
		cons_asyctab = asyctab;
		cons_asyc_bufp = asyc_bufp;
	}
	cons_asyc_num = asyc_num;

	/* Get the number of Serial Ports configured */
	asyc_num = cm_getnbrd("asyc");

	DEBUG1(("ASYC: The number of boards configured is %d \n",asyc_num));


	/* De-register and register terminal server only if mis-match */
	if ((asyc_num) && (cons_asyc_num != asyc_num)) {
		/* De-register the old terminal server */
		if ((asyc_id = iasy_deregister(asyc_sminor,
			 cons_asyc_num)) != -1) {

			/*  Register terminal server. */
			asyc_id = iasy_register(asyc_sminor, asyc_num, 
				asycproc, asychwdep, &asycconssw);
			asyc_tty = IASY_UNIT_TO_TP(asyc_id, 0);
		}
	}


	/* Allocate memory for the boards configured */
	if ( (rval = asyc_memalloc(ASYC_KMEM_ALLOC)) != 0) {
		return rval;
	}

	/* Loop and get the configuration parameters from Config Manager */
	for (unit = 0; unit < asyc_num; unit++) {
		asyc = &asyctab[unit];

		/* Get board key from Config Manager */
		if((cm_args.cm_key = cm_getbrdkey("asyc",unit))==RM_NULL_KEY) {
			DEBUG1(("ASYC: Error getting board key !! \n"));
			continue;
		}
		cm_args.cm_n = 0;

		/*
		 * Get Interrupt vector
		 */
		cm_args.cm_param = CM_IRQ;
		cm_args.cm_val = &(asyc->asyc_vect);
		cm_args.cm_vallen = sizeof(asyc->asyc_vect);

		if(cm_getval(&cm_args)) {	
			/* Error getting IRQ */
			asyc->asyc_vect = 0;	 
			DEBUG1(("ASYC: Error getting board IRQ !! \n"));
			continue;
		} else {
			/* validate the IRQ, must be a usable one */
			if ((asyc->asyc_vect < 2) || (asyc->asyc_vect > 15) ||
				( asyc->asyc_vect == 13)) {
				DEBUG1(("ASYC: Unusable IRQ !! \n"));
				continue;
			}
		}
		DEBUG1(("ASYC: IRQ returned by Config Manager is %d \n", \
			asyc->asyc_vect));

		/* 
		 *	Get IO address range of board 
		 */
		cm_args.cm_param = CM_IOADDR;
		cm_args.cm_val = &(asyc_ioaddr);
		cm_args.cm_vallen = sizeof(asyc_ioaddr);
		
		if (cm_getval(&cm_args)) {
			/* Error occured in getting IO base */
			DEBUG1(("ASYC: Error getting board IO base !! \n"));
			continue;
		}
		DEBUG1(("ASYC: IO base returned by Config Manager is 0x%x \n", \
			asyc_ioaddr.startaddr));

		asyc->asyc_dat = asyc_ioaddr.startaddr+DAT;
		if ((cons_asyc = asyc_verify_init(asyc->asyc_dat)) == NULL)
			asyc_portinit(unit, (ushort_t)1);
		else {
			bcopy((caddr_t)cons_asyc,(caddr_t)asyc,sizeof(struct asyc));
			asyc->asyc_bp = &asyc_bufp[unit];
			bcopy((caddr_t)cons_asyc->asyc_bp,(caddr_t)asyc->asyc_bp,
				sizeof(struct asyc_aux));
		}

		/* Attach asyc interrupt handler if device detected */
		if (asyc->asyc_flags & ASYHERE) 
			rval = cm_intr_attach(cm_args.cm_key,asycintr,&asycdevflag,NULL);
		/* Check if the asyc intr handler was sucessfully attached */
		if (!rval) {
                	cmn_err(CE_CONT,"!asyc:cm_intr_attach failed !");
			/* Flag off the device not present */
			asyc->asyc_flags &= ~ASYHERE;
		}
	}
	if(asyc_num) {
		asycinitialized = ASYC_COM_INIT;
		if (itimeout(asycpoll,(caddr_t)0, TO_PERIODIC|drv_usectohz(20000),plstr) == 0)
			cmn_err(CE_PANIC, "Can't get timeout for the serial ports ");
	}
	return 0;
}


/* 
 * STATIC struct asyc *
 * asyc_verify_init(ushort_t io_base)
 *
 * Given an I/O port, try and verify that the COM port has been previously
 * detected during the console verification.
 */

struct asyc *
asyc_verify_init(io_base)
ushort_t io_base; 
{
	int unit;
	struct asyc *cons_asyc;
	
	if ( cons_asyc_num == 0 || !cons_asyctab )
		return NULL;
	for (unit = 0; unit < cons_asyc_num; unit++) {
		cons_asyc = &cons_asyctab[unit];
		if (cons_asyc->asyc_dat == io_base)
			return cons_asyc;
	}
	return NULL;
}


/*
 * void
 * asycintr(int) 
 *	process device interrupt
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	INTERRUPT TIME EXECUTION THREAD
 *	This procedure is called by system with interrupts off
 *	when the USART interrupts the processor. 
 */
/*ARGSUSED*/
void
asycintr(int vect)
{	
	register struct	strtty	*tp;	/* strtty structure */
	register unsigned int	unit;
	register struct asyc	*asyc;
	register struct asyc_aux *ap;
	register uchar_t c;
	uchar_t	 interrupt_id;
	uchar_t	 line_status;
	register int fifo_charcount;


	/* Is it a stay interrupt ?? */
	if (!(asycinitialized & ASYC_COM_INIT) || (asyctab == NULL) || 
		(asyc_num == 0))
		return;

	for (asyc = &asyctab[0], unit = 0;  unit < asyc_num; asyc++, unit++) {

		/* Check if the vector matches, device present */
		if ((asyc->asyc_vect != vect)||(!(asyc->asyc_flags & ASYHERE)))
			continue;
		
		/* Initialise Rx Fifo count */
		fifo_charcount=0;

#ifdef MERGE386
		/*
		 * needed for com port attachment 
		 */
		if (asyc->mrg_data.com_ppi_func && 
		   (*asyc->mrg_data.com_ppi_func)(&asyc->mrg_data, -1))
			return;
#endif /* MERGE386 */

		tp = IASY_UNIT_TO_TP(asyc_id, unit);
		ap = asyctab[unit].asyc_bp;

	for (;;) {
nextloop:
		interrupt_id =  inb(asyc->asyc_isr) & 0x0f; 

		/* Check if device interrupt pending */
		if (interrupt_id & ISR_NOINTRPEND)
			break;
		line_status  =  inb(asyc->asyc_lsr);
	

		if ((interrupt_id == ISR_RxRDY) || 
		    (interrupt_id == ISR_FFTMOUT) || 
		    (line_status == LSR_RCA) || 
		    (interrupt_id == ISR_RSTATUS)) {
	
			while (line_status & LSR_RCA) {
				c = inb(asyc->asyc_dat) & 0xff;
				fifo_charcount ++;
				if (asyc->asyc_flags & ASYC_SYSCON) {
					switch (c) {
#ifndef NODEBUGGER
					case DEBUGGER_CHAR:
						asyc->asyc_lstate |= DEBUGGER_CHAR_RCVD;
  						goto nextloop;
#endif
                                        case REBOOT_CHAR:
						if (!(console_security &
							CONS_REBOOT_OK))
						break;
						asyc->asyc_lstate |= REBOOT_CHAR_RCVD;
						goto nextloop;
					case PANIC_CHAR:
						if (!(console_security & CONS_PANIC_OK))
						break;
						asyc->asyc_lstate |= PANIC_CHAR_RCVD;
						goto nextloop;
#ifdef DKTEST
					case FLAVORS_CHAR:
						asyc->asyc_lstate |= FLAVORS_CHAR_RCVD;
						goto nextloop;
#endif
					default:
						break;
					}
				}
				if ((tp->t_cflag & CREAD) != CREAD)
					goto nextloop;
				asycProcessInChar(tp, c, line_status);	

				/* 
				 * Read line status only if characters are 
				 * in fifo 
				 */
				if (fifo_charcount >= asyc->asyc_bp->fifo_size)
					break;

				line_status  =  inb(asyc->asyc_lsr);
			}
			drv_setparm(SYSRINT, 1);

		} else if (interrupt_id == ISR_TxRDY || 
		    (line_status & LSR_XHRE) &&
		    (ap->asyc_state & BUSY)) {

			drv_setparm(SYSXINT, 1);
			ap->asyc_state &= ~BUSY;
			asyc->asyc_wdtick = 0;
			asycstartio(tp);

		} else if (interrupt_id == ISR_MSTATUS)  
			asycmodem(tp);
		else
			break;
		}
	}
}


/*
 * STATIC void
 * asycProcessInChar(struct strtty *, uchar_t, uchar_t)
 *
 * Calling/Exit State:
 *	- Called at pltty from the interrupt thread. 
 *
 * Description:
 *	asycProcessInChar() -- do the input processing for a character
 *	This driver is for basic USARTs that don't have any on-chip storage.
 *	So in order to ensure we don't lose any character, the input logic below
 *	simply stuffs the character into the driver input buffer for the 
 *	specific device. The processing of the character and sending it 
 *	upstream is taken care of in asycpoll().
 */
STATIC void
asycProcessInChar(struct strtty *tp, uchar_t c, uchar_t line_status)
{
	struct asyc_aux	*ap;
	unsigned short	ierr = 0;
	struct asyc	*asyc;
	int		lcnt;

	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];
	ap = asyc->asyc_bp;

	if (!(tp->t_state & ISOPEN))
		return;

	if (ap->asyc_iflag & IXON) {
		/* if stopped, see if to turn on output */
		if (ap->asyc_state & TTSTOP) {
			if (((c == tp->t_cc[VSTART]) && (c != 0))||
					ap->asyc_iflag & IXANY) {
				ap->asyc_state &= ~TTSTOP;
				tp->t_state &= ~TTSTOP;
				asycstartio(tp);
			}
		} else {
			/* maybe we're supposed to stop output */
			if (c == tp->t_cc[VSTOP] && (c != 0)) {
				ap->asyc_state |= TTSTOP;
				tp->t_state |= TTSTOP;
			}
		}

		if ((c != 0) && ((c == tp->t_cc[VSTOP]) || 
				 (c == tp->t_cc[VSTART])))
			return;
	}

	/* Check if rx overun occured */
	if (line_status & LSR_OVRRUN) 
		asyc_rxoverun ++;

	if (INEXT(ap->iput) != ap->iget) {
		if (line_status & (LSR_PARERR|LSR_FRMERR|LSR_BRKDET)) {
			if (line_status & LSR_PARERR)
				ierr = PERROR;
			if (line_status & (LSR_FRMERR|LSR_BRKDET))
				ierr = FRERROR;
		}

#ifdef ASYC_DEBUG
		if (ierr)
			/*
			 *+ line status error
			 */
			cmn_err(CE_WARN, 
				"asycProcessInChar: line status error lsr=%x, ierr = %x", line_status, ierr);
#endif /* ASYC_DEBUG */

		ap->ierrs[ap->iput] = (ierr >> 8) & 0xff;
		ap->ibuf[ap->iput] = c;
		ap->iput = INEXT(ap->iput);

		if ((asyc->asyc_flags & (HWDEV|ASYC_IFLOWCNTL)) &&
			!(ap->asyc_state & TBLOCK)) {

			/* Get buffer character count */
			lcnt = ap->iput - ap->iget;

			if (lcnt < 0)
				lcnt += ISIZE;
	
			/*
		 	* Check if the available buffer size is less than or 
		 	* equal to 25%.
		 	*/
			if (lcnt >= (ISIZE * 3 / 4)) {

				ap->asyc_state |= TTXOFF|TBLOCK;
				ap->asyc_state &= ~TTXON;
				asycstartio(tp);
			}
		}
	} else 
		asyc_softrxoverun++;
}


/*
 * STATIC void
 * asycpoll(int)
 *
 * Calling/Exit State:
 *	Called at plstr.	
 *
 * Description:
 *	asycpoll() loops for all the devices, checking if there is input in the
 *	driver buffers (asyc_aux) put there by the interrupt routine. If so, it
 *	does any needed processing and calls iasy_input() to ship the characters
 *	upstream. 
 *
 *	Also, for each device, if there is anything in the tty output buffer 
 *	(t_out) put there by iasy_output(), it transfers them to the driver 
 *	output buffer to be processed by asycstartio().
 *
 * Note:
 *	splx(s) sets the priority level from plhi to plstr.
 */
/* ARGSUSED */
STATIC void
asycpoll(int dummy)
{
	struct strtty	*tp;
	struct asyc_aux	*ap;
	int		unit;
	short		lcnt;
	uchar_t		lbuf[3], c;
	unsigned short	stat;
	boolean_t	asyc_input_flag;
	int		efl;
        register struct asyc    *asycp;
	uchar_t 	lstate;


	for (unit = 0, tp = asyc_tty, asycp = &asyctab[unit];
			unit < asyc_num; unit++, tp++, asycp++) { 

		if (!(asycp->asyc_flags & ASYHERE))
			continue;

		ap = asycp->asyc_bp;

		lstate = asycp->asyc_lstate;
		asycp->asyc_lstate &= ~lstate;

		if (lstate & DCD_TURNED_ON) { 
			/* The ISOPEN flag of t_state variable is not set here*/
			tp->t_state |= CARR_ON;
			iasy_carrier(tp);
		}

		if (!(tp->t_state & ISOPEN)) {
			ap->asyc_state &= ~ISOPEN;
			continue;
		}

		if (lstate) {
			if (lstate & DEBUGGER_CHAR_RCVD)
				(*cdebugger)(DR_USER, NO_FRAME);

			if (lstate & REBOOT_CHAR_RCVD)
				drv_shutdown(SD_SOFT, AD_BOOT);

			if (lstate & PANIC_CHAR_RCVD)
				drv_shutdown(SD_PANIC, AD_QUERY);
#ifdef DKTEST
			if (lstate & FLAVORS_CHAR_RCVD)
				invoke_flavors();
#endif
			if (lstate & DCD_TURNED_OFF) {
				tp->t_state &= ~CARR_ON;
				timeout(iasy_hup, (caddr_t)tp, 1);
			}
		}

		asyc_input_flag = B_FALSE;

		if ((ap->asyc_state & TBLOCK) && !(tp->t_state & TBLOCK)) {

			/*
			 * Get the input buffer character count 
			 */
			lcnt = ap->iput - ap->iget;
			if (lcnt < 0) 
				lcnt += ISIZE;

			/*
			 * Check if the available buffer size has dropped
			 * below the 25% mark.
			 */
			if (lcnt < (ISIZE/4)) {
				efl = ASYC_INTR_DISABLE();

				ap->asyc_state &= ~(TTXOFF|TBLOCK);
				ap->asyc_state |= TTXON;
				asycstartio(tp);

				ASYC_INTR_RESTORE(efl);
			}
		}

		/*
		 * input ready at interrupt level? 
		 */
		while (ap->iget != ap->iput) {
			if (tp->t_state & TBLOCK) 
				break;
			if (tp->t_in.bu_cnt < 3) {
				if (!(iasy_input(tp, L_BUF))) {
					asyc_input_flag = B_FALSE;
				} else {
					break;
				}
			}

			lcnt = 1;
			c = ap->ibuf[ap->iget];
			stat = (ap->ierrs[ap->iget] << 8) & 0xff00;

			if (stat & PERROR && !(tp->t_iflag & INPCK)) {
				if ((tp->t_cflag & (PARODD|PARENB)) != (PARODD|PARENB) &&
				    ((c & 0377) != 0))
					stat &= ~PERROR;
			}

			if (stat & (FRERROR|PERROR)) {
				if ((stat & FRERROR) && (c & 0377) == 0) {
					if (!(tp->t_iflag & IGNBRK)) {
						ap->iget = INEXT(ap->iget);
						if (asyc_input_flag == B_TRUE) {
							/*
							 * Ship this upstream 
							 * first. 
							 */
							if (!(iasy_input(tp, L_BUF))) {
								asyc_input_flag = B_FALSE;
							}
						}
						iasy_input(tp, L_BREAK);
						break;
					} else {
						ap->iget = INEXT(ap->iget);
						continue;
					}
		            	} else {
					if ((tp->t_iflag & IGNPAR)) {
						/*
						 * If IGNPAR is set, characters
						 * with framing and parity
						 * errors are ignored (except 
						 * BREAK) 
						 */
						ap->iget = INEXT(ap->iget);
	                    			continue;
					}
				}

				if (tp->t_iflag & PARMRK) {
					/* Mark the parity errors */
					lcnt = 3;
					lbuf[2] = (unchar)0377;	
					lbuf[1] = 0;
				} else
					/* Send up a NULL character */
					c = 0;
			} else {
				if (tp->t_iflag & ISTRIP) {
					c &= 0177;
				} else {
					if (c == 0377 && tp->t_iflag & PARMRK) {
						/*
						 * if marking parity errors, 
						 * this character gets doubled 
						 */
						lcnt = 2;
						lbuf[1] = c;
					}
				}
			}

			lbuf[0] = c;

			/*
			 * In the couple of pieces of code below, 
			 * we are trying to optimize by calling 
			 * iasy_input() when the input buffer is full.  
			 * The asyc_input_flag is there to keep track 
			 * of anytime anything is put into the input
			 * buffer for the current tty. After exiting 
			 * this for loop, we still want to ship whatever 
			 * is in the buffer upstream. We could call
			 * iasy_input() everytime, even if there is
			 * nothing in the buffer, but it is more expensive.
			 * So we only call it if something is there to send.
			 */
			if (tp->t_in.bu_ptr != NULL) {
				while (lcnt > 0) {
					*(tp->t_in.bu_ptr++) = lbuf[--lcnt];
					/* something is in the buffer */
					asyc_input_flag = B_TRUE;
					tp->t_in.bu_cnt--;
					if (tp->t_in.bu_cnt == 0) {
						if (!(iasy_input(tp, L_BUF))) {
							/*
							 * nothing in the 
							 * buffer
							 */
							asyc_input_flag = B_FALSE;
						} else {
							break;
						}
					}
				}
			}

			ap->iget = INEXT(ap->iget);

		} /* while ap->iget != ap->iput */

		/*
		 * Of course, after exiting the for loop above, we want to ship 
		 * anything else that might be in the input buffer right away.
		 */
		if (tp->t_in.bu_cnt != IASY_BUFSZ)
			iasy_input(tp, L_BUF);
		/* 
		 * copy output to interrupt level buffer
		 */

		efl = ASYC_INTR_DISABLE();
		if (!(ap->asyc_state & BUSY))
			tp->t_state &= ~BUSY;
		else
			if (asycp->asyc_wdtick++ > asyc_maxwdtick) {
				asycp->asyc_wdtick = 0;
				ap->asyc_state &= ~BUSY;
				tp->t_state &= ~BUSY;
				asyc_wdexp++;
			}
				
		ASYC_INTR_RESTORE(efl);

		if (!(tp->t_out.bu_ptr && tp->t_out.bu_cnt)) {
			if (!(CPRES & iasy_output(tp))) {
				if (ap->oput != ap->oget) {
					efl = ASYC_INTR_DISABLE();

					if (!(ap->asyc_state & BUSY))
						tp->t_state &= ~BUSY;
					if ((tp->t_state & (BUSY|TTSTOP|TIMEOUT)) == 0) {
						asycstartio(tp);
						if (ap->asyc_state & BUSY)
							tp->t_state |= BUSY;
						ASYC_INTR_RESTORE(efl);
						continue;
					}
					ASYC_INTR_RESTORE(efl);
				}
				continue;
			}
		}

		while (ONEXT(ap->oput) != ap->oget) {
			ap->obuf[ap->oput] = *tp->t_out.bu_ptr++;
			ap->oput = ONEXT(ap->oput);
			tp->t_out.bu_cnt--;
			if (tp->t_out.bu_cnt == 0) {
				if (!(CPRES & iasy_output(tp))) {
					break;
				}
			}
		} /* while there are characters to output */

		efl = ASYC_INTR_DISABLE();

		if (!(ap->asyc_state & BUSY))
			tp->t_state &= ~BUSY;

		if ((tp->t_state & (BUSY|TTSTOP|TIMEOUT)) == 0) {
			asycstartio(tp);
			if (ap->asyc_state & BUSY)
				tp->t_state |= BUSY;
		}

		ASYC_INTR_RESTORE(efl);

	} /* for all ports */
}


/*
 * STATIC int
 * asycproc(struct strtty *, int) 
 *	low level device dependant operations
 *
 * Calling/Exit State:
 *	- Return 0 on success, otherwise return appropriate errno.
 *
 * Description:
 *	It is called at both task time by the line discipline routines,
 *	and at interrupt time by asycintr().
 *	asycproc handles any device dependent functions required
 *	upon suspending, resuming, blocking, or unblocking output; flushing
 *	the input or output queues; timing out; sending break characters,
 *	or starting output.
 */
STATIC int
asycproc(struct strtty *tp, int cmd)
{
	pl_t		s;
	unsigned char	line_ctl;
	struct asyc	*asyc;
	int		ret = 0;
	struct asyc_aux	*ap;
	int efl;

    	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];
	ap = asyc->asyc_bp;

	efl = ASYC_INTR_DISABLE();

	switch (cmd) {
	case T_TIME:
		tp->t_state &= ~TIMEOUT;
        	line_ctl = inb(asyc->asyc_lcr);
        	outb(asyc->asyc_lcr, line_ctl & ~LCR_SETBREAK );
		asycstartio(tp);
		break;

	case T_WFLUSH:
		tp->t_out.bu_cnt = 0;   /* abandon this buffer */
		ap->oput = ap->oget;
		break;

	case T_RESUME:
		tp->t_state &= ~TTSTOP;
		ap->asyc_state &= ~TTSTOP;
		asycstartio(tp);
		break;

	case T_SUSPEND:
		tp->t_state |= TTSTOP;
		ap->asyc_state |= TTSTOP;
		break;

	case T_RFLUSH:
		ap->iget = ap->iput;
		tp->t_in.bu_cnt = IASY_BUFSZ;
		tp->t_in.bu_ptr = tp->t_in.bu_bp->b_wptr;
		if (!(tp->t_state & TBLOCK))
			break;
		/* FALLTHRU */

	case T_UNBLOCK:
		tp->t_state &= ~TBLOCK;
		ap->asyc_state &= ~(TTXOFF|TBLOCK);
		ap->asyc_state |= TTXON;
		asycstartio(tp);
		break;

	case T_BLOCK:
		tp->t_state |= TBLOCK;
		ap->asyc_state &= ~TTXON;
		ap->asyc_state |= TBLOCK|TTXOFF;
		asycstartio(tp);
		break;

	case T_BREAK:
		tp->t_state |= TIMEOUT;
        	line_ctl = inb(asyc->asyc_lcr);
        	outb(asyc->asyc_lcr, line_ctl | LCR_SETBREAK);
		ASYC_INTR_RESTORE(efl);
		(void) timeout(asycwakeup, (caddr_t)tp, HZ/4);
		return;

	case T_OUTPUT:
		ASYC_INTR_RESTORE(efl);

		if (!(tp->t_out.bu_ptr && tp->t_out.bu_cnt)) {
			if (!(CPRES & iasy_output(tp))) {

				efl = ASYC_INTR_DISABLE();
				if (ap->oput != ap->oget) {
					if (!(ap->asyc_state & BUSY))
						tp->t_state &= ~BUSY;
					if ((tp->t_state & (BUSY|TTSTOP|TIMEOUT)) == 0) {
						asycstartio(tp);
						if (ap->asyc_state & BUSY)
							tp->t_state |= BUSY;
						break;
					}
				}
                                break;
                        }
                }

                while (ONEXT(ap->oput) != ap->oget) {
                        ap->obuf[ap->oput] = *tp->t_out.bu_ptr++;
                        ap->oput = ONEXT(ap->oput);
                        tp->t_out.bu_cnt--;
                        if (tp->t_out.bu_cnt == 0) {
                                if (!(CPRES & iasy_output(tp))) {
					return (ret);
                                }
                        }
                }

		efl = ASYC_INTR_DISABLE();
                if (!(ap->asyc_state & BUSY))
                        tp->t_state &= ~BUSY;

                if ((tp->t_state & (BUSY|TTSTOP|TIMEOUT)) != 0)
                        break;

                asycstartio(tp);

                if (ap->asyc_state & BUSY)
                        tp->t_state |= BUSY;
                break;

	case T_CONNECT:
		if (!(asyc->asyc_flags & ASYHERE)) {
			ret = ENXIO;
			break;
		}
		
		if (asyc->asyc_flags & ASY82510)
			outb(asyc->asyc_isr, 0x0);

		if ((tp->t_state & (ISOPEN|WOPEN)) == 0) {
			if ((asyc->asyc_flags & ASY82510) == 0) {
				/* clear both fifos */
				outb(asyc->asyc_isr, 0x0);
				if (asyc->asyc_flags & ASY16550) {
					/* fifo trigger for 16550 chip */
					outb(asyc->asyc_isr, TRLVL3);
				}
			}

			tp->t_state &= ~BUSY;
			asycu_reset(asyc);
			asycparam(tp);

			ap->iget = ap->iput = 0;
			ap->oget = ap->oput = 0;
			ap->asyc_state = ISOPEN;

			if (IASY_HWDEV(tp->t_dev)) {
				asyc->asyc_flags |= HWDEV;
				asyc->asyc_flags &= ~(HWFLWO);
			} else {
				asyc->asyc_flags &= ~(HWDEV|HWFLWO);
			}
		}

		asycsetdtr(asyc);

		if (tp->t_cflag & CLOCAL)
			tp->t_state |= CARR_ON;
		else {
			if (inb(asyc->asyc_msr) & MSR_DCD) {
				tp->t_state |= CARR_ON;
				iasy_carrier(tp);
			} else
				tp->t_state &= ~CARR_ON;
		}     

		if (IASY_HWDEV(tp->t_dev)) {
			if (inb(asyc->asyc_msr) & MSR_CTS) {
				asyc->asyc_flags |= HWFLWO;
			} else {
				asyc->asyc_flags &= ~HWFLWO;
			}
		}

		break;

	case T_DISCONNECT:
		outb(asyc->asyc_mcr,MCR_OUT2);
                drv_usecwait(asyc_tick);
		break;

	case T_PARM:
		if (!(inb(asyc->asyc_lsr) & LSR_XSRE)) {
			/*
			 * Wait for one character time for Transmitter shift
			 * register to get empty	
			 */
			ASYC_INTR_RESTORE(efl);
			return(iasy_ctime(tp, 1));
		} else {
			ret = asycparam(tp);
		}
		break;

	case T_DATAPRESENT:
		if (ap->oput != ap->oget)
			ret = ENOTEMPTY;
		else
			ret = 0;
		break;

	case T_TRLVL1:
		outb (asyc->asyc_isr, 0x0);
		outb (asyc->asyc_isr, TRLVL1);
		asyc->asyc_bp->fifo_size = 1;
		break;

	case T_TRLVL2:
		outb (asyc->asyc_isr, 0x0);
		outb (asyc->asyc_isr, TRLVL2);
		asyc->asyc_bp->fifo_size = 4;
		break;

	case T_TRLVL3:
		outb (asyc->asyc_isr, 0x0);
		outb (asyc->asyc_isr, TRLVL3);
		asyc->asyc_bp->fifo_size = 8;
		break;

	case T_TRLVL4:
		outb (asyc->asyc_isr, 0x0);
		outb (asyc->asyc_isr, TRLVL4);
		asyc->asyc_bp->fifo_size = 14;
		break;

	case T_FIRSTOPEN:
#ifdef MERGE386
		if (!portalloc(asyc->asyc_dat, asyc->asyc_dat + 0x7))
			ret = EBUSY;
#endif /* MERGE386 */
		break;

	case T_LASTCLOSE:
#ifdef MERGE386
		portfree(asyc->asyc_dat, asyc->asyc_dat + 0x7);

		/* 
		 * Restore serial port ipl for Merge
		 */
		if (asyc->asyc_mrg_state & ASYC_MRG_ATTACH) {
			restore_asyc_ipl(asyc->asyc_vect, (void *) asycintr);
			asyc->asyc_mrg_state &= ~ASYC_MRG_ATTACH;
		}
#endif /* MERGE386 */
		break;
	}

	ASYC_INTR_RESTORE(efl);

	return(ret);
}


/*
 * STATIC void
 * asycstartio(struct strtty *) 
 *	start output on an serial channel if needed.
 *
 * Calling/Exit State:
 *	- Called at plhi/pltty. In SVR4 the function was called at pltty.
 *
 * Description:
 *	Get a character from the character queue, output it to the
 *	channel and set the BUSY flag. The BUSY flag gets reset by asycintr
 *	when the character has been transmitted.
 */
STATIC void
asycstartio(struct strtty *tp)
{
	struct asyc	*asyc;
	struct asyc_aux	*ap;
	int		c;
	ushort		limit;
	ushort		charcount = 0;


	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];
	ap = asyc->asyc_bp;

	if (ap->asyc_state & BUSY)
		/* Wait for a better time */
		return;

	if (ap->asyc_state & (TTXON|TTXOFF)) {
		if (ap->asyc_state & TTXON) {
			ap->asyc_state &= ~TTXON;
			if (asyc->asyc_flags & HWDEV) {
				char mcr = inb(asyc->asyc_mcr);
				outb(asyc->asyc_mcr, mcr|MCR_RTS);
			}  
			if (ap->asyc_iflag & IXOFF) { 
 				outb(asyc->asyc_dat, tp->t_cc[VSTART]);
				charcount++;
				ap->asyc_state |= BUSY;
			}
		} else {
			ap->asyc_state &= ~TTXOFF;
			if (asyc->asyc_flags & HWDEV) {
				char mcr = inb(asyc->asyc_mcr);
				outb(asyc->asyc_mcr, mcr & ~MCR_RTS);
			} 
			if (ap->asyc_iflag & IXOFF) { 
		 		outb(asyc->asyc_dat, tp->t_cc[VSTOP]);
				charcount++;
				ap->asyc_state |= BUSY;
			}
		}
	}

	if ((asyc->asyc_flags & (HWDEV|HWFLWO)) == HWDEV)
		return;


	/* Don't output data while output is blocked */
	if (ap->asyc_state & TTSTOP)
		return;

	limit = (asyc->asyc_flags & ASY16550) ? 16 : 1;
	while (charcount < limit) {
		if (ap->oput == ap->oget)
			return;
	
		ap->asyc_state |= BUSY;
		c = ap->obuf[ap->oget];
		/* send the character */
		outb(asyc->asyc_dat, c);
		ap->oget = ONEXT(ap->oget);

		charcount++;
	}
}


/*
 * STATIC int
 * asycparam(struct strtty *)
 *
 * Calling/Exit State:
 *	- Called at plhi. In SVR4 the function was called at pltty.
 *
 * Description:
 *	Param detects modifications to the line characteristics and reprograms 
 */
STATIC int
asycparam(struct strtty *tp)
{
	struct asyc	*asyc;
	int		s;	/* Index to conversion table for COM baudrate */
	int		x;


	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];
	x = (ICR_TIEN|ICR_SIEN|ICR_MIEN|ICR_RIEN);

	if (tp->t_cflag & CLOCAL)
		tp->t_state |= CARR_ON;
	else
		asycmodem(tp);

	outb(asyc->asyc_icr, x);

	x = inb(asyc->asyc_lcr);
	x &= ~(LCR_WLS0|LCR_WLS1|LCR_STB|LCR_PEN|LCR_EPS);

	if ((tp->t_cflag & CSTOPB) == CSTOPB)
		x |= LCR_STB;  
	if ((tp->t_cflag & CS6) == CS6)
		x |= LCR_BITS6;
	if ((tp->t_cflag & CS7) == CS7)
		x |= LCR_BITS7;
	if ((tp->t_cflag & CS8) == CS8)
		x |= LCR_BITS8;
	if ((tp->t_cflag & PARENB) == PARENB)
		x |= LCR_PEN;
	if ((tp->t_cflag & PARODD) == 0)
		x |= LCR_EPS;

	outb(asyc->asyc_lcr, x);

	/*
	 * Set the baud rate:
	 * Bounds check baud rate index and then verify that the chip supports
	 * that speed.  Then program it. Default to 9600 baud.
	 */
	s = tp->t_cflag & CBAUD;
	if (s > MAXBAUDS || s < 0)
		s = B9600;

	if (s == 0) {
		/*
		 * only disable modem line
		 */
		if (!(tp->t_cflag & CLOCAL)) {
			outb(asyc->asyc_mcr,(MCR_RTS|MCR_OUT2));
			return(0);
		} else {
			return(EINVAL);
		}
	}

	asyct_prog(tp, s);

	asyc->asyc_bp->asyc_iflag &= ~(IXON|IXANY|IXOFF);
	asyc->asyc_bp->asyc_iflag |= (tp->t_iflag & (IXON|IXANY|IXOFF));

	/* Turn on the input flow control flag if IXOFF set */
	if(tp->t_iflag & IXOFF)
		asyc->asyc_flags |= ASYC_IFLOWCNTL;

	return (0);
}


/*
 * STATIC void
 * asycmodem(struct strtty *)
 * 
 * Calling/Exit State:
 *	- Called at plhi. In SVR4 the function was called at pltty.
 *
 * Description:
 *	Changes Modem Status register while in DOSMODE.
 */
STATIC void
asycmodem(struct strtty *tp)
{
	struct asyc	*asycp;
	int		msr;


	asycp = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];

	/* this resets the interrupts */
        drv_usecwait(asyc_tick);
 	msr = inb(asycp->asyc_msr);

	if (asycp->asyc_flags & ASY82510 ) {
		outb(asycp->asyc_msr, msr & 0xF0);
	}

	if (!(tp->t_state & (ISOPEN|WOPEN))) 
		return;

 	if ((asycp->asyc_flags & HWDEV)) {
 		if ((msr & MSR_CTS)) {
 			asycp->asyc_flags |= HWFLWO ;
			asycstartio(tp);
 		} else {
 			asycp->asyc_flags &= ~HWFLWO ;
		}
	}

	if (!(tp->t_cflag & CLOCAL)) {
		if (msr & MSR_DCD) {
			if (!(tp->t_state & CARR_ON))  {
				/*
				 * iasy_carrier() will wakeup the waiting
				 * process for the carrier.
				 */
				asycp->asyc_lstate |= DCD_TURNED_ON;
			}
		} else {
 			if ((tp->t_state & CARR_ON) &&
					(tp->t_state & ISOPEN)) {
				asycp->asyc_lstate |= DCD_TURNED_OFF;

			}
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
 * asycwakeup(struct strtty *) 
 *	release transmitter output.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	It is used by the TCSBRK ioctl command.  After .25 sec
 *	timeout (see case BREAK in asycproc), this procedure is called.
 */
STATIC void
asycwakeup(struct strtty *tp)
{
	asycproc(tp, T_TIME);
}


/*
 * STATIC void
 * asycsetdtr(struct asyc *) 
 *	assert the DTR line for this port
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
asycsetdtr(struct asyc *asyc)
{
	int	mcr;

	mcr = inb(asyc->asyc_mcr);
	outb(asyc->asyc_mcr, (mcr|MCR_DTR));
        drv_usecwait(asyc_tick);
}


/*
 * STATIC void
 * asyct_prog(struct strtty *, ushort)
 *	This procedure programs the baud rate generator.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
asyct_prog(struct strtty *tp, ushort speed)
{
	ushort		y;
	unsigned char	x;
	struct asyc	*asyc;


	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];

	x = inb(asyc->asyc_lcr);
	outb(asyc->asyc_lcr, x | LCR_DLAB);
	y = asycspdtab[speed];
	outb(asyc->asyc_dat, y & 0xff);
	outb(asyc->asyc_icr, y >> 8);
	outb(asyc->asyc_lcr, x);	
}


/*
 * STATIC void
 * asycu_reset(struct asyc *)
 *	This procedure does the initial reset on an COM USART.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
asycu_reset(struct asyc *asyc)
{
	(void)inb(asyc->asyc_isr);
	(void)inb(asyc->asyc_lsr);
	(void)inb(asyc->asyc_msr);
	(void)inb(asyc->asyc_dat);
	
	outb(asyc->asyc_lcr, LCR_DLAB);
	outb(asyc->asyc_dat, asycspdtab[B9600] & 0xff);		
	outb(asyc->asyc_icr, asycspdtab[B9600]  >> 8);		
	outb(asyc->asyc_lcr, LCR_BITS8); 
	outb(asyc->asyc_mcr, MCR_DTR|MCR_RTS|MCR_OUT2);	
        drv_usecwait(asyc_tick);
}


/*
 * STATIC void
 * asychwdep(queue_t *, mblk_t *, struct strtty *)
 *	Hardware dependent ioctl support.
 *
 * Calling/Exit State:
 *	- q points to the write queue pointer.
 *	- bp contains the ioctl or ioctl data not understood by the DI code.
 */
/* ARGSUSED */
STATIC void
asychwdep(queue_t *q, mblk_t *bp, struct strtty *tp)
{	
	struct iocblk	*ioc;
	struct asyc	*asyc;
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
	asyc = &asyctab[IASY_TP_TO_UNIT(asyc_id, tp)];

	switch (bp->b_datap->db_type) {
	case M_IOCTL:
		switch (ioc->ioc_cmd) {
		case TIOCMGET:
			if(!bp->b_cont) /* Fall thru and send an M_IOCNACK */
				break;
			s = SPL();
			msr = inb(asyc->asyc_msr);
			mcr = inb(asyc->asyc_mcr);
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
				mcr = inb(asyc->asyc_mcr);
				modval = *((int*)bp->b_cont->b_rptr);
				if (modval & TIOCM_DTR)
					mcr |= MCR_DTR;
				else
					mcr &= ~MCR_DTR;
				s = SPL();
				outb(asyc->asyc_mcr, mcr);
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
				mcr = inb(asyc->asyc_mcr);
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
				outb(asyc->asyc_mcr, mcr);
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
			cmn_err(CE_NOTE,"!asyc:Data Copy failed !! \n");
			freemsg(bp);
		}
		return;
	default:

		/*
		 *+ An unknown message type.
		 */
 		cmn_err(CE_WARN, "!asyc: Unknown message type obtained\n");
                freemsg(bp);
                return;
        }

#ifdef MERGE386
	if (com_ppi_strioctl(q, bp, &asyc->mrg_data, ioc->ioc_cmd)) {
		
		/*
		 * Drop the serial port ipl for merge 
		 */
		if (!(asyc->asyc_mrg_state & ASYC_MRG_ATTACH)) {
			downgrade_asyc_ipl(asyc->asyc_vect, (void *)asycintr);
			asyc->asyc_mrg_state |= ASYC_MRG_ATTACH;
		}
	} else
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
 * asyccnopen(minor_t, boolean_t, const char *)
 *
 * Calling/Exit State:
 *	- Returns the minor number for the device.
 *
 * Remark:
 *	The hardware portion of the serial port driver decides if
 *	the port can receive console I/O. Also note that only one 
 *	port at anytime can act as a console port.
 */
/* ARGSUSED */
STATIC dev_t
asyccnopen(minor_t minor, boolean_t syscon, const char *params)
{
	extern major_t	iasy_major;	/* major num. of serial port device */
	struct asyc	*asyc;
	int		unit;
	int 		rval;

	if (minor < asyc_sminor ||
	    minor > (asyc_sminor + IASY_UNIT_TO_MINOR(asyc_num) - 1))
		return NODEV;

	/* Check if port initialised */
	if (!asycinitialized) {
		/* Is there atleast one port ? */ 
		if( asyc_num == 0)
			return(ENODEV);

		/* Set flag to indicate console init */
		asycinitialized = ASYC_CONSOLE_INIT;
		if ((rval = asyc_tabinit()) != 0)
			return(ENODEV);
	}

	unit = IASY_MINOR_TO_UNIT(minor - asyc_sminor);
    	asyc = &asyctab[unit];

	asycu_reset(asyc);

	if (IASY_HWDEV(minor)) {
		asyc->asyc_flags |= HWDEV;
		if (inb(asyc->asyc_msr) & MSR_CTS) {
			asyc->asyc_flags |= HWFLWO;
		} else {
			asyc->asyc_flags &= ~HWFLWO;
		}
	} else {
		asyc->asyc_flags &= ~(HWDEV|HWFLWO);
	}

	if (syscon)
		asyc->asyc_flags |= ASYC_SYSCON;

	return makedevice(iasy_major, minor);
}

/*
 * STATIC int 
 * asyc_tabinit(void)
 *
 * Called from asyccnopen for early initialization only  
 */

STATIC int 
asyc_tabinit(void)
{
	int		unit;
	struct asyc	*asyc;			/* Temp ptr to asyc struct */
	asyc_base_t 	*tp;
	int 	rval;

	if ((rval = asyc_memalloc(ASYC_CONSMEM_ALLOC)) != 0)
		return (ENOMEM);

	/* 
	 * Init routine should have already picked up the base 
	 * address of the available ports thru' asyc_getbase()
	 */
	tp = asyc_base;
	for (unit = 0; unit < asyc_num; unit++) {
		asyc = &asyctab[unit];
		asyc->asyc_vect = tp->int_vect;
		asyc->asyc_dat =  tp->io_base;

		/* 
		 * Initialize every port based on the base 
		 * address detected from the BIOS.
		 */
		(void) asyc_portinit(unit, (ushort_t)0);
		tp++;
	}
	return 0;
}

/*
 * STATIC void
 * asyccnclose(minor_t, boolean_t)
 *
 * Calling/Exit State:
 *	- None
 */
STATIC void
asyccnclose(minor_t minor, boolean_t syscon)
{
	struct asyc	*asyc;
	int		unit;


	ASSERT(asycinitialized);
	ASSERT((minor >= asyc_sminor) &&
		(minor <= (asyc_sminor + IASY_UNIT_TO_MINOR(asyc_num))));

	unit = IASY_MINOR_TO_UNIT(minor - asyc_sminor);
    	asyc = &asyctab[unit];

	if (syscon)
		asyc->asyc_flags &= ~ASYC_SYSCON;
}


/*
 * STATIC int 
 * asyccnputc(minor_t, int)
 *
 * Calling/Exit State:
 *	- Return 1 on success.
 *
 * Description:
 *	put a character out the first serial port.
 *	Do not use interrupts.  If char is LF, put out LF, CR.
 */
STATIC int
asyccnputc(minor_t minor, int c)
{
	int	p_asyc;		/* start i/o address */


	ASSERT(asycinitialized);
	ASSERT((minor >= asyc_sminor) &&
		(minor <= (asyc_sminor + IASY_UNIT_TO_MINOR(asyc_num))));

	p_asyc = asyctab[IASY_MINOR_TO_UNIT(minor) - asyc_sminor].asyc_dat;

	if (inb(p_asyc + ISR) & 0x38)
		return (1);

	/*
	 * wait for xmit to finish by checking the 
	 * transmission hold register bit of the line 
	 * status register.
	 */
	while ((inb(p_asyc + LSR) & LSR_XHRE) == 0) {
		/*
		 * if there is no carrier, then return immediately
		 */
		if ((inb(p_asyc + MSR) & MSR_DCD) == 0)
			return (1);

		drv_usecwait(10);
	}

	/*
	 * put the character out 
	 */
	outb(p_asyc + DAT, c);

	return (1);
}


/*
 * STATIC int
 * asyccngetc(minor_t)
 *	get a character from the first serial port.
 *
 * Calling/Exit State:
 *	If no character is available, return -1.
 *	Run in polled mode, no interrupts.
 */
STATIC int
asyccngetc(minor_t minor)
{
	int	p_asyc;		/* start i/o address */
	int	c;


	ASSERT(asycinitialized);
	ASSERT((minor >= asyc_sminor) &&
		(minor <= (asyc_sminor + IASY_UNIT_TO_MINOR(asyc_num))));

	p_asyc = asyctab[IASY_MINOR_TO_UNIT(minor) - asyc_sminor].asyc_dat;

	if ((inb(p_asyc + ISR) & 0x38) || (inb(p_asyc + LSR) & LSR_RCA) == 0) {
		return (-1);
	}

	c = inb(p_asyc + DAT);

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
 * asyccnsuspend(minor_t minor)
 *	Suspend normal input processing in preparation for cngetc.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
asyccnsuspend(minor_t minor)
{
	struct asyc *asyc = &asyctab[IASY_MINOR_TO_UNIT(minor) - asyc_sminor];
	uchar_t lcr;

	ASSERT(asycinitialized);
	lcr = inb(asyc->asyc_lcr);
	outb(asyc->asyc_lcr, lcr & ~LCR_DLAB);
	asyc->asyc_save_icr = inb(asyc->asyc_icr);
	outb(asyc->asyc_icr, 0);
	outb(asyc->asyc_lcr, lcr);
}


/*
 * STATIC void
 * asyccnresume(minor_t minor)
 *	Resume normal input processing after cngetc.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
STATIC void
asyccnresume(minor_t minor)
{
	struct asyc *asyc = &asyctab[IASY_MINOR_TO_UNIT(minor) - asyc_sminor];
	uchar_t lcr;

	ASSERT(asycinitialized);
	lcr = inb(asyc->asyc_lcr);
	outb(asyc->asyc_lcr, lcr & ~LCR_DLAB);
	outb(asyc->asyc_icr, asyc->asyc_save_icr);
	outb(asyc->asyc_lcr, lcr);
}


/*
 * STATIC void
 * asyc_dump(int)
 *
 * Calling/Exit State:
 *	None.
 */
void
print_asyc(int unit)
{
	struct strtty	*tp;
	struct asyc_aux	*ap;
	struct	asyc	*asyc;


	if (unit >= asyc_num)
		return;

	tp = IASY_UNIT_TO_TP(asyc_id, unit);
	asyc = &asyctab[unit];
	ap = asyctab[unit].asyc_bp;
	
	cmn_err(CE_CONT, "\n asyc_aux struct: size=0x%x(%d)\n",
		sizeof(struct asyc_aux), sizeof(struct asyc_aux));
	cmn_err(CE_CONT, "\tasyc_bp=0x%x, \tstrtty=0x%x\n", ap, tp);
	cmn_err(CE_CONT, "\tibuf=0x%x\n", &ap->ibuf[ap->iget]);
	cmn_err(CE_CONT, "\tobuf=0x%x\n", &ap->obuf[ap->oget]);
	cmn_err(CE_CONT, "\tierrs=0x%x\n", ap->ierrs);
	cmn_err(CE_CONT, "\toput=0x%x, \toget=0x%x\n",
                        ap->oput, ap->oget);
	cmn_err(CE_CONT, "\tiput=0x%x, \tiget=0x%x\n",
                        ap->iput, ap->iget);
	cmn_err(CE_CONT, "\tasyc_state=0x%x, \tfifo_size=0x%x\n",
                        ap->asyc_state, ap->fifo_size);
	cmn_err(CE_CONT, "\tasyc_iflag=0x%x\n", ap->asyc_iflag);
	cmn_err(CE_CONT, "\tasyc_lstate=0x%x\n",asyc->asyc_lstate);
	cmn_err(CE_CONT, "\tasyc_wdtick=0x%x\n",asyc->asyc_wdtick);
}

/*
 *
 *	STATIC int
 *	asyc_getbase(void)
 *	
 *	Return value :	The number of boards found thru BIOS
 *	Description  : 	Check if board detected, and verify if the it's 
 *			a Serial port by performing a loop back test.
 */

STATIC int
asyc_getbase(void)
{
	asyc_base_t 	*pbase;	
	int 		i;
	ushort_t 	asyc_bnum = 0;
	ushort_t	asyc_iercopy;

	DEBUG1(("ASYC: asyc_getbase() entered \n"));
	
	pbase = asyc_base;

	for (i = 0; i < MAX_CONPORTS; i++, pbase++) {

		/* Check 1 : Existance of port */
		asyc_iercopy = inb(pbase->io_base+ICR);	
		
		/* Clear ICR register and then read it back */
		outb(pbase->io_base+ICR,0);
		if(inb(pbase->io_base+ICR))
			continue;

		/* Restore the ICR contents */
		outb(pbase->io_base+ICR,asyc_iercopy);

	   	/* Check 2: Verify existance of COMM port */
		if(asyc_verifyport(pbase->io_base)) 
			asyc_bnum++;
	}
	return (asyc_bnum);	/* Number of boards detected */
}

/*
 *
 *	STATIC int
 *	asyc_verifyport(ushort io_base)
 *	
 *	Return value :	True on success or Zero on failure
 *	Description  : 	This function checks if the port is present
 *			on the specified io base by performing a loopback
 *			test. (i.e By Putting the UART in loopback mode,
 *			Setting the Handshake bits in the MCR and checking 
 *			for valid responses in the modem status register.
 */

STATIC int 
asyc_verifyport(ushort io_base)
{
	ushort_t mcr_copy;

	/* Copy of modem contol register */
	mcr_copy = inb(io_base+MCR);

	/* Put UART in loopback mode */
	outb(io_base+MCR,SET_LBKMODE);

	/* Check for valid response */
	if((inb(io_base+MSR) & 0xF0) == LBK_EXPECTED) { 
		DEBUG1(("ASYC: Detected port thru' loopback\n"));
		outb(io_base+MCR, mcr_copy);
		return(1);
	} else {
		outb(io_base+MCR, mcr_copy);
		return(0);
	}
}

/*
 * STATIC int
 * asyc_memalloc(void)
 *
 * Allocate memory for asyc struct depending on the type of 
 * init. 
 */

STATIC int
asyc_memalloc(ushort_t alloc_type)
{

	if (alloc_type == ASYC_CONSMEM_ALLOC) {
		/* Console init sequence */
		if ((asyctab = (struct asyc *) 
		consmem_alloc(asyc_num*sizeof(struct asyc),KM_NOSLEEP))==NULL)
			return(ENOMEM);
		bzero(asyctab,(asyc_num * sizeof(struct asyc)));

		if ((asyc_bufp = (struct asyc_aux *)
		consmem_alloc(asyc_num*sizeof(struct asyc_aux),KM_NOSLEEP))==NULL)
			return(ENOMEM);
		bzero(asyc_bufp,(asyc_num * sizeof(struct asyc_aux)));
	} else {
		/* Normal init sequence */
		if ((asyctab = (struct asyc *)
		kmem_zalloc(asyc_num*sizeof(struct asyc), KM_NOSLEEP)) == NULL)
			return(ENOMEM);
		if ((asyc_bufp = (struct asyc_aux *)
		kmem_zalloc(asyc_num*sizeof(struct asyc_aux),KM_NOSLEEP))==NULL)
			return(ENOMEM);
	}
	return 0;
}

/* 
 * STATIC void
 * asyc_portinit(int unit,ushort_t enable_interrupts)
 *
 * Verify the existence of a serial port. If present, identify the 
 * chararcteristics and allocate the necessary buffers.
 */

STATIC void
asyc_portinit(int unit,ushort_t enable_interrupts)
{
	struct asyc	*asyc;			/* Temp ptr to asyc struct */
	struct asyc_aux	*bufp;			/* Temp ptr to asyc_aux   */

	ASSERT(unit < asyc_num);

	asyc = &asyctab[unit];

	/* Initialise the UART register port addresses */
	asyc->asyc_isr = asyc->asyc_dat+ISR;
	asyc->asyc_icr = asyc->asyc_dat+ICR;
	asyc->asyc_lcr = asyc->asyc_dat+LCR;
	asyc->asyc_mcr = asyc->asyc_dat+MCR;
	asyc->asyc_lsr = asyc->asyc_dat+LSR;
	asyc->asyc_msr = asyc->asyc_dat+MSR;
	asyc->asyc_bp = bufp = &asyc_bufp[unit]; 

	/* 
	 * Bit 4 & 5 of ISR are wired low. If bit 4 or 5 
	 * appears on inb(), board is not there.
	 *
	 * Note: A more robust method of detecting the Serial port
	 * is by writting a pattern and reading the same (0x55)
	 */ 

	if ((inb(asyc->asyc_isr) & 0x30)) {
		asyc->asyc_flags &= ~ASYHERE;
		return;
	}
	asyc->asyc_flags |= ASYHERE;

	/* Reset all registers for soft restart. */

	outb(asyc->asyc_icr, 0x00);
	outb(asyc->asyc_isr, 0x00);
	outb(asyc->asyc_lcr, 0x00);
	outb(asyc->asyc_mcr, 0x00);

	outb(asyc->asyc_isr, 0x20);
	if (inb(asyc->asyc_isr) & 0x20) {
		asyc->asyc_flags |= ASY82510;	/* 82510 chip present */
		outb((asyc->asyc_dat + 0x7), 0x04);   /* Status clear */
		outb(asyc->asyc_isr, 0x40);	/* set to bank 2 */
		outb(asyc->asyc_mcr, 0x08);	/*  IMD */
		outb(asyc->asyc_dat, 0x21);	/*  FMD */
		outb(asyc->asyc_isr, 0x00);	/* set to bank 0 */
	}

	++asyc_alive;
#ifdef MERGE386
	asyc->mrg_data.baseport = asyc->asyc_dat;
#endif

	asyc->asyc_bp = bufp;
	asyc->asyc_bp->oput = asyc->asyc_bp->oget = 0;
	asyc->asyc_bp->iget = asyc->asyc_bp->iput = 0;
	asyc->asyc_bp->asyc_state = 0;
	asyc->asyc_bp->asyc_iflag = 0;
	asyc->asyc_bp->fifo_size = 1;

	/* Set each UART in FIFO mode */
	if ((asyc->asyc_flags & ASY82510) == 0) {
		outb(asyc->asyc_isr, 0xc1);	/* check for 550 */
		if ((inb(asyc->asyc_isr) & 0xc0) == 0xc0) { /* 16550? */
			asyc->asyc_flags |= ASY16550;
			/* Receive fifo default size */
			asyc->asyc_bp->fifo_size = 8; 
			/* clear both fifos */
			outb(asyc->asyc_isr, 0x0);
			/* fifo trigger for 16550 chip */
			outb(asyc->asyc_isr, TRLVL3);
		}
	}

	/* Reset all interrupts for soft restart. */
	(void)inb(asyc->asyc_isr);
	(void)inb(asyc->asyc_lsr);
	(void)inb(asyc->asyc_msr);
	(void)inb(asyc->asyc_dat);

	/*
	 * Enable receive interrupts, to allow reciept of special
	 * characters, such as kernel debugger invocation, even
	 * when driver not open.
	 */

	if (enable_interrupts)
		outb(asyc->asyc_icr, ICR_RIEN);
	else
		outb(asyc->asyc_icr, 0);
}
