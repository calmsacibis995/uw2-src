/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/sc/sc.c	1.8"
#ident	"$Header: $"

/* $Copyright:	$
 * Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 
 * Sequent Computer Systems, Inc.   All rights reserved.
 *  
 * This software is furnished under a license and may be used
 * only in accordance with the terms of that license and with the
 * inclusion of the above copyright notice.   This software may not
 * be provided or otherwise made available to, or used by, any
 * other person.  No title to or ownership of the software is
 * hereby transferred.
 */

/*
 * sc.c
 *     SSM console (serial port) driver software (STREAMS version).
 */

#include <util/types.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/cmn_err.h>
#include <util/ipl.h>
#include <mem/kmem.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <io/autoconf.h>
#include <io/cfg.h>
#include <io/sc/sc.h>
#include <io/slic.h>
#include <io/ssm/ssm.h>
#include <io/stream.h>
#include <io/strmdep.h>
#include <io/stropts.h>
#include <io/termio.h>
#include <io/termios.h>
#include <util/kdb/xdebug.h>

#include <io/ddi.h>	/* Must come last */

/*
 * Declarations not shared with other modules.
 */
#define CTL(c) 		((c) & 037)	/* Mask off a control character */

#define SC_AVGCNT       10              /* Size of typical xmit packets; used
                                         * in calculating waiting times for
                                         * data to siphon out during close(). */

#define SCRXSIZE	128		/* Size of recieved char buffer, in the
					 * sc_info control structure. */

#define SC_VECS_PER_PORT  2		/* # of SLIC interrupt vectors required
					 * per SSM serial port. */

#define SC_BUF_WAIT     10000           /* 10 millisec wait for streams buf */

/* Control structure for SSM serial ports.  One allocated per console port. */
struct	sc_info {
	int	sc_state;		/* State of driver */
	tcflag_t sc_cflags;		/* Configuration flags */
	tcflag_t sc_iflags;		/* Input data flags */
	minor_t	sc_unit;		/* sc logical unit number/its minor # */
	dev_t	sc_dev;			/* Port's major/minor device number */
	struct 	ssm_desc *sc_ssm; 	/* Description of our ssm */
	lock_t	*sc_lock;		/* General access lock */
	sv_t	*sc_drain_wait; 	/* Wait for output to drain on close */
	sv_t	*sc_carrier_wait;	/* Wait for carrier synch variable */
	sv_t	*sc_open_wait;		/* Wait for message block during open */
	mblk_t  *sc_msg;		/* msg currently being sent */
	queue_t *sc_qptr;		/* Local write queue addr */
	struct cons_rcb *sc_rcb;	/* Addr of recieve CB for this port */
	struct cons_xcb *sc_xcb;	/* Addr of transmit CB for this port */
	struct cons_mcb *sc_mcb;	/* Addr of message CB for this port */
	toid_t	sc_owaitid;		/* Bufcall or timeout ID if a wait for
					 * a buffer occurs during open. Use the
					 * same ID for either since they are not
					 * undone later. */
	toid_t	sc_bufcallid;		/* ID of bufcall */
	toid_t	sc_timeoutid;		/* Timeout ID, used if bufcall fails */
	toid_t	sc_delayid;		/* ID of output delay timeout */
	toid_t	sc_drainid;		/* ID of output drain timeout */
	unchar	sc_alive;	  	/* Device is alive */
	ushort  sc_flags;
	ushort	sc_modem;		/* Modem status settings  */
	ushort	sc_tx_count;            /* Number of chars to tx */
	unchar 	sc_baud;
	unchar 	sc_parsiz;
	unchar	sc_devno;		/* Local/Remote port flag */
	unchar	sc_restart_read;	/* Start new reads be ? */
	ulong	sc_frame_errs;        	/* Number of framing errors */
	ulong	sc_parity_errs; 	/* Number of parity errors */
	ulong	sc_overruns;		/* Number of overrun errors */
	unchar	sc_ld0flow;		/* Output flow control for fw */
	unchar	sc_ld0xon;		/* Output start char for fw */
	unchar	sc_ld0xoff;		/* Output stop char for fw */
	unchar	sc_rx_buf[SCRXSIZE+3];	/* Received character buffer. Note that
					 *  it is padded in event a parity mark
					 *  must be appended by the driver. */
	struct winsize sc_wsz;        /* hold the windowing info. */

};

#define SC_LOCK_HEIR	1		/* Hierarchy for sc_info.sc_lock */

/* state bits for sc_info.sc_state */
#define WOPEN		0x001
#define ISOPEN		0x002
#define CARR_ON		0x004
#define BUSY		0x008
#define TXDELAY		0x010
#define WCLOSE		0x020
#define SUSPOUT		0x040

/* bits for sc_info.sc_flags */
#define DRAINTIME	0x001

/* scmsg() return status */
#define MSGDONE         0               /* Message executed or started */
#define PRIPUTBACK      1               /* Retry a high priority msg later */
#define STDPUTBACK      2               /* Retry a normal priority msg later */

/*
 * Forward function references.
 */
int scopen(queue_t *, dev_t *, int, int, cred_t *);
int scclose(queue_t *, int, cred_t *);
int scrsrv(queue_t *);
int scwsrv(queue_t *);

STATIC void scintr(int);
STATIC void init_ssm_cons_dev(const struct sc_info *);
STATIC void sc_abort_read(const struct sc_info *);
STATIC void sc_abort_write(const struct sc_info *);
STATIC void sc_draintime(queue_t *);
STATIC void sc_delaytime(struct sc_info *);
STATIC void sc_get_modes(struct sc_info *);
STATIC void sc_next_msg(struct sc_info *, mblk_t *);
STATIC int  sc_param(struct sc_info *);
STATIC void sc_restart_out(const struct sc_info *);
STATIC void sc_set_modes(const struct sc_info *);
STATIC void sc_start_read(const struct sc_info *);  
STATIC void sc_start_write(struct sc_info *, int, uchar_t *);
STATIC void sc_suspend_out(const struct sc_info *);

int     scdevflag = D_MP;	/* Required for DDI binding */

/*
 * Static data local to this module.
 */
STATIC int sc_base_vector;	/* 1st intr vec in series for sc-devices */
STATIC int sc_ndevs;		/* # of sc_info elements that may be valid */
STATIC struct sc_info *sc_info[SSM_SERIAL_PORTS] = {NULL, NULL};

STATIC ulong_t sc_unit_flags[SSM_SERIAL_PORTS] = {
	CFG_SSM_LOCAL,
	CFG_SSM_REMOTE
};

#define STRID_SC 10
static struct module_info minfo = {
/*  module ID     module name     min pkt size    max pkt siz   hiwat  lowat */
      STRID_SC,      "sc",            0,            INFPSZ,     150,    50
};

/* Table of ms-per-byte serial output rate (indexed by baud B0-EXTA) */
static uchar_t scbaud_time[] = {
	0, 200, 143, 91, 77, 67, 50, 33, 17, 8, 6, 4, 2, 1, 1 };


static struct qinit rinit = {
	NULL,  scrsrv,  scopen,  scclose,  NULL,  &minfo,  NULL
};

static struct qinit winit = {
	putq,  scwsrv,  NULL,  NULL,  NULL,  &minfo,  NULL
};

struct streamtab scinfo = { &rinit, &winit,  NULL,  NULL };

/*
 * This table defines the SSM command bits for setting asynchronous
 * baud rates in sc_param().  '-1' is the signal to turn off the DTR
 * output.  '-2' indicates that a baud rate is not available.
 */
#define BHANGUP		((uchar_t)0xff)		/* hangup */
#define BNOSUPP		((uchar_t)0xfe)		/* baud rate not supported */

uchar_t sc_mrates[] = {
	BHANGUP,	/* B0	  (hangup)	*/
	BNOSUPP,	/* B50	  (not avail.)	*/
	BNOSUPP,	/* B75	  (not avail.)	*/
	B110,		/* B110			*/
	BNOSUPP,	/* B134.5 (not avail.)	*/
	BNOSUPP,	/* B150	  (not avail.)	*/
	BNOSUPP,	/* B200	  (not avail.)	*/
	B300,		/* B300			*/
	BNOSUPP,	/* B600	  (not avail.)	*/
	B1200,		/* B1200		*/
	BNOSUPP,	/* B1800  (not avail.)	*/
	B2400,		/* B2400		*/
	B4800,		/* B4800		*/
	B9600,		/* B9600		*/
	B19200,		/* B19200		*/
	B38400		/* B38400		*/
};

STATIC LKINFO_DECL(sc_lkinfo, "sc unit info lock", 0);

#ifndef NOFLAVORS

extern int flavors_initted;
extern lock_t flavors_lock;
extern sv_t flavors_sv;
extern int inflavors;
STATIC int flv_input_nchars = 0;
STATIC char flv_inputbuf[64];
STATIC char *flv_input_drain = &flv_inputbuf[0];
STATIC char *flv_input_empty = &flv_inputbuf[0];
#define _FIE    (&flv_inputbuf[sizeof(flv_inputbuf)])

/*
 * int
 * getchar(void)
 *	Return a char from the console to flavors.
 *
 * Calling/Exit State:
 *	Must not be holding any locks upon entry or exit.
 *
 * Description:
 *	flv_input_buff is used as a fifo between the console
 *	driver and flavors.  The console driver diverts input
 *	to it and then attempts wakes up getchar() if it is
 *	awaiting data.  Likewise, getchar() takes input from
 *	the fifo and waits on flavors_sv if none is available.
 */
int
getchar(void)
{
	pl_t pl;
	int c;

	if (!inflavors) {
		/*+
		 *+ The getchar() routine can only be called
		 *+ from flavors or something invoked from flavors.
		 */
		cmn_err(CE_PANIC, "getchar() only available when in flavors");
	}

	pl = LOCK(&flavors_lock, PLHI);
	while (flv_input_nchars == 0) {
		SV_WAIT(&flavors_sv, PRISLEP, &flavors_lock);
		(void) LOCK(&flavors_lock, PLHI);
	}
	c = *flv_input_drain++;
	if (flv_input_drain >= _FIE)
		flv_input_drain = flv_inputbuf;
	flv_input_nchars--;
	UNLOCK(&flavors_lock, pl);

	return(c);
}
#endif /* NOFLAVORS */

/*
 * void
 * scinit()
 * 	Serial port driver initialization of front-panel/console SSM. 
 *
 * Calling/Exit State:
 *	No mutex required, since this function is called while
 *		still running single threaded.  Does not block.
 *
 *	SSM_cons must already have been determined by ssminit();
 *		NULL if not found, otherwise it addresses a descriptor 
 *		for the controlling SSM.
 *
 *	sc_ndevs implicilty initialized to zero.  Upon return
 *		reflects the number of ports found, that may
 *		be usable.
 *
 *	sc_info array elements are set to address allocated state 
 *		information about each usable port.  sc_info[X] 
 *		is NULL if port-X is currently deconfigured. 
 *	
 *	sc_base_vector is set to the first in the sequence of SLIC 
 *		interrupt vector numbers allocated for these devices,
 *		from which the source of an interrupt can be determined.
 *
 *	sc_global.cons_id and
 *	sc_global.altcons_id are established so the /dev/console 
 *		psuedo-driver can distinguish the role of each port.
 *
 *	No return value.
 *
 * Description:
 *	Determine which, if any, SSM serial ports are usable, 
 *	then allocate necessary structures to make use of them
 *	and enable message passing and interrupts between the
 *	SSM and host for that port.
 */
void
scinit()
{
	struct sc_info	*infop;
	uchar_t port, vector;

	/* 
	 * Verify that there is an SSM running the console.
	 * SSM initialization code determines this and leaves
	 * the information in a global.
	 */
	if (! SSM_cons) 
		return;		/* No SSMs own the front panel */

	/*
	 * Determine which port is driving the front-panel
	 * selected console and load the appropriate device 
	 * numbers into sc_global.cons_id and sc_global.altcons_id 
	 * for the console pseudo-driver's use in distinguishing
	 * the role of each port.
	 */
	switch (SSM_cons->ssm_is_cons) {
	case CDSC_LOCAL:
		sc_global.cons_id = makedevice(sc_global.c_major, CCB_LOCAL);
		sc_global.altcons_id = makedevice(sc_global.c_major, CCB_REMOTE);
		break;
	case CDSC_REMOTE:
		sc_global.cons_id = makedevice(sc_global.c_major, CCB_REMOTE);
		sc_global.altcons_id = makedevice(sc_global.c_major, CCB_LOCAL);
		break;
	}

	/* 
	 * Reserve two SLIC interrupt vectors for each serial
	 * port on the controlling SSM; one for input, one for
	 * output.  Deconfigure ports if this fails.
	 */
	sc_base_vector = ivec_alloc_group(sc_global.bin, 
					  SSM_SERIAL_PORTS * SC_VECS_PER_PORT);
	if (sc_base_vector < 0) {
		/*
		 *+ The SSM serial port driver was unable to allocate
		 *+ interrupt vectors for use with its devices.  The
		 *+ ports are unusable as a result.  The probable cause
		 *+ is that too many drivers/devices are already 
		 *+ configured and initialized to use the same interrupt
		 *+ "bin".
		 */
		cmn_err(CE_WARN, 
			"sc: unable to assign interrupts; ports deconfigured.");
		return;
	}

	/* 
	 * Verify that each port is usable, i.e., it 
	 * passed powerup diagnostics and was not
	 * explicitly deconfigured from the firmware
	 * monitor.  If usable, set up data structures 
	 * to manage the port and initialize its state.
	 */
	for (port = 0, vector = (uchar_t)sc_base_vector; 
	     port < SSM_SERIAL_PORTS; port++, vector += SC_VECS_PER_PORT) {

		if (SSM_cons->ssm_diag_flags & sc_unit_flags[port] )
			continue;	  /* Port not usable */

		sc_info[port] = infop = (struct sc_info *)
			kmem_zalloc(sizeof(struct sc_info), KM_NOSLEEP);
		if (!infop) {
			/*
		 	 *+ The SSM serial port driver was unable to allocate
		 	 *+ memory for managing the specified port, which has
			 *+ been deconfigured.
			 */
			cmn_err(CE_WARN,
			  "sc%d: no memory for info structure; deconfigured.",
			   port);
			return;
		}
		infop->sc_lock = LOCK_ALLOC(SC_LOCK_HEIR, plstr, 
			&sc_lkinfo, KM_NOSLEEP);
		infop->sc_carrier_wait = SV_ALLOC(KM_NOSLEEP);
		infop->sc_drain_wait = SV_ALLOC(KM_NOSLEEP);
		infop->sc_open_wait = SV_ALLOC(KM_NOSLEEP);

		if (! infop->sc_lock || ! infop->sc_carrier_wait 
		||  ! infop->sc_drain_wait || ! infop->sc_open_wait) {
			/* Undo the other allocations and deconfig the port */
			if (infop->sc_lock)
				 LOCK_DEALLOC(infop->sc_lock);
			if (infop->sc_carrier_wait)
				 SV_DEALLOC(infop->sc_carrier_wait);
			if (infop->sc_drain_wait)
				 SV_DEALLOC(infop->sc_drain_wait);
			if (infop->sc_open_wait)
				 SV_DEALLOC(infop->sc_open_wait);

			kmem_free(infop, sizeof(struct sc_info));
			sc_info[port] = (struct sc_info *) NULL;
			/*
		 	 *+ The SSM serial port driver was unable to allocate
		 	 *+ synchronization structures for managing the 
			 *+ specified port, which has been deconfigured.
			 */
			cmn_err( CE_WARN,
			  "sc%d: no memory for sync structures; deconfigured.",
			   port);
			return;
		}

		/*
		 * Finish the explicit port data initialization.
		 * All other members were zeroed upon allocation.
		 * The rest of this port's initialization is
		 * supposed to succeed...note that sc_devno and sc_unit
		 * fields are currently the same, because we support
		 * only the front panel SSM's ports - auxilary SSM's
		 * ports are not connected and usable for now.
		 */
		infop->sc_alive = 1;
		infop->sc_ssm = SSM_cons;
		infop->sc_unit = port;		
		infop->sc_devno = port;	/* Essentially unit%SSM_SERIAL_PORTS */
		infop->sc_dev = makedevice(sc_global.c_major, port);
		infop->sc_rcb = (struct cons_rcb *) 
			CONS_BASE_CB(infop->sc_ssm->ssm_cons_cbs, 
				     infop->sc_devno) + CCB_RECV_CB;
		infop->sc_xcb = (struct cons_xcb *) 
			CONS_BASE_CB(infop->sc_ssm->ssm_cons_cbs, 
				     infop->sc_devno) + CCB_XMIT_CB;
		infop->sc_mcb = (struct cons_mcb *) 
			CONS_BASE_CB(infop->sc_ssm->ssm_cons_cbs, 
				     infop->sc_devno) + CCB_MSG_CB;

		/*
		 * Install the driver's interrupt handler for
		 * this port's possible interrupts, then enable
		 * message passing for the port with the SSM.
		 */
		ivec_init(sc_global.bin, vector + CCB_RECV_CB, scintr);
		ivec_init(sc_global.bin, vector + CCB_XMIT_CB, scintr);
		init_ssm_cons_dev(infop);

		/* 
		 *+ The SSM serial port driver is announcing that 
		 *+ if has located and validated one of its devices
		 *+ for operation.  No action required.
		 */
		cmn_err(CE_CONT,
			"sc %d found on ssm %d\n", port, SSM_cons - SSM_desc);

		sc_ndevs = port + 1;
	}
}

/*
 * STATIC void
 * scopen_wakeup(struct sc_info *)
 * 	Wakeup open()s waiting on ip->sc_open_wait for
 *	stream buffers to become available.
 *
 * Calling/Exit State:
 *	Has no process context and does not block.
 *
 *	Clears ip->sc_owaitid to indicate that a
 *		bufcall or timeout is no longer pending.
 *	
 *	Broadcasts a wakeup to all processes blocked
 *		on the ip->openwait synch variable.
 *	
 *	No return value.
 */
STATIC void
scopen_wakeup(struct sc_info *ip)
{
	pl_t tplock = LOCK(ip->sc_lock, plstr);

	ip->sc_owaitid = 0;
	SV_BROADCAST(ip->sc_open_wait, 0);
	UNLOCK(ip->sc_lock, tplock);
}

/*
 * int
 * scopen(queue_t *, dev_t *, int, int, cred_t *)
 * 	Open an SSM serial port stream.
 *
 * Calling/Exit State:
 *	This function may block.  
 *	
 *	The port's sc_lock must not be held upon entry or exit.
 *
 *	Upon successful completion, the device's state information
 *	structure is attached to the queue descriptor.
 *
 *	Possible returns:
 *	EAGAIN  unable to wait for resource allocation.
 *	EBUSY	the queue for this device appears to already 
 *		be in use, although this is the device's first open.
 *	EINVAL	attempt to initialize the line parameters failed.
 *	EINTR	interrupted by a signal while waiting for streams
 *		message buffer or carrier.
 *	ENXIO	sflag not DRVOPEN.	
 *	ENXIO	minor device specified is invalid or unusable.
 *	0	open succeeded.
 *
 * Description:
 *	Verify that the minor device is valid prior to locating
 *	driver state information for the port.  If not already
 *	open, determine initial line parameters and communicate
 *	them to the SSM.  Then wait for carrier (if necessary)
 *	and inform the stream head that this device can serve as 
 *	a ctty.  Lastly, enable the queue' service routines if
 *	the device is not already open.
 */
/*ARGSUSED*/
int
scopen(queue_t *rd_q, dev_t *devp, int oflag, int sflag, cred_t *crp)
{
	pl_t tplock;
	minor_t unit = getminor(*devp);
	uchar_t firstopen;
	register struct sc_info *ip;
	mblk_t	*bp;
        struct stroptions *sop;

	/* Check for non-driver open */
	if (sflag != DRVOPEN || unit >= sc_ndevs 
	||  (ip = sc_info[unit]) == NULL || ! ip->sc_alive)
		return (ENXIO);

	/*
	 *  If the device is not already open, initialize it.
	 */
	tplock = LOCK(ip->sc_lock, plstr);
	if (ip->sc_state & ISOPEN) {
		firstopen = 0;
	} else {
		firstopen = 1;
		if (rd_q->q_ptr != NULL) {
			/*
			 *+ The console open routine has been passed a
			 *+ pointer to a stream that is already in use
			 *+ by another port or that was not cleaned up
			 *+ correctly.
			 */
			cmn_err(CE_WARN, "sc%d: first open found struct ptr", 
				ip->sc_unit);
			UNLOCK(ip->sc_lock, tplock);
			return(EBUSY);
		}
		ip->sc_msg = NULL;
		ip->sc_qptr = WR(rd_q);
	 	ip->sc_bufcallid = 0;
	 	ip->sc_timeoutid = 0;
	 	ip->sc_delayid = 0;
		rd_q->q_ptr = (char *) ip; 
		WR(rd_q)->q_ptr = (char *) ip; 
		ip->sc_cflags = sc_global.cflags;
		ip->sc_iflags = sc_global.iflags;
		ip->sc_state |= ISOPEN;
		ip->sc_modem = CCM_RTS | CCM_DTR;
		ip->sc_ld0flow = CCF_XOFF;
		ip->sc_ld0xon  = CSTART;
		ip->sc_ld0xoff = CSTOP;
		
		/*
		 * Send a message to the SSM to establish line
		 * settings and baud rate of this console port.
		 */
		if (sc_param(ip)) {
			ip->sc_state = 0;
			ip->sc_qptr = NULL;
			rd_q->q_ptr = NULL;
			WR(rd_q)->q_ptr = NULL;
			UNLOCK(ip->sc_lock, tplock);
			return (EINVAL);
		}

		sc_abort_read(ip);

		if (ip->sc_cflags & CREAD) {
			ip->sc_restart_read = 1;
			sc_start_read(ip);
		} else
			ip->sc_restart_read = 0;
	}
	
	if (ip->sc_cflags & CLOCAL)
		ip->sc_state |= CARR_ON;
	else
		ip->sc_state &= ~CARR_ON;
	
	/*
	 *  Wait for carrier, if required. 
	 */
 	if (!(oflag & FNDELAY)) {
		sc_get_modes(ip);	
		if (ip->sc_modem & CCM_DCD)
			ip->sc_state |= CARR_ON;
		while ((ip->sc_state & CARR_ON) == 0) {
			ip->sc_state |= WOPEN;
			if (SV_WAIT_SIG(ip->sc_carrier_wait, pritty,
					ip->sc_lock) == FALSE) {
				if (firstopen) {
					tplock = LOCK(ip->sc_lock, plstr);
					ip->sc_state = 0;
					ip->sc_qptr = NULL;
					rd_q->q_ptr = NULL;
					WR(rd_q)->q_ptr = NULL;
					ip->sc_restart_read = 0;
					UNLOCK(ip->sc_lock, tplock);
				}
				return(EINTR);
			}
			tplock = LOCK(ip->sc_lock, plstr);
		}
		ip->sc_state &= ~WOPEN;
	}
	UNLOCK(ip->sc_lock, tplock);

	/* 
	 * Inform the stream head that this device can be 
	 * a ctty.  In the event of multiple opens, let it 
	 * sort out which process is the session leader and
	 * actually registers the ctty.  
	 */
	while ((bp = allocb( sizeof(struct stroptions), BPRI_MED)) == NULL) {
 		if (oflag & (FNDELAY | FNONBLOCK)) {
			if (firstopen) {
				tplock = LOCK(ip->sc_lock, plstr);
				ip->sc_state = 0;
				ip->sc_qptr = NULL;
				rd_q->q_ptr = NULL;
				WR(rd_q)->q_ptr = NULL;
				ip->sc_restart_read = 0;
				UNLOCK(ip->sc_lock, tplock);
			}
               		return (EAGAIN);	/* Don't wait for allocation */
                }
		/*
		 * Wait for the resources to become available
		 * and try again to aquire them.  If a bufcall() 
		 * wait is already in progress join it.  If bufcall()
		 * fails, try scheduling a timeout to do the same
		 * thing.  If that fails, then give up and fail the
		 * open as well.
		 */
		tplock = LOCK(ip->sc_lock, plstr);
		if (! ip->sc_owaitid) {
			ip->sc_owaitid = 
				bufcall((uint_t)sizeof(struct stroptions), 
					BPRI_MED, scopen_wakeup, (long)ip);
			if (ip->sc_owaitid == 0) {
				ip->sc_owaitid = itimeout(scopen_wakeup,
					(caddr_t)ip, drv_usectohz(SC_BUF_WAIT),
					plstr);
				if (ip->sc_owaitid == 0) {
					if (firstopen) {
						ip->sc_state = 0;
						ip->sc_qptr = NULL;
						rd_q->q_ptr = NULL;
						WR(rd_q)->q_ptr = NULL;
						ip->sc_restart_read = 0;
					}
					UNLOCK(ip->sc_lock, tplock);
					return (ENOSR);
				}
			}
		}
		if (SV_WAIT_SIG(ip->sc_open_wait, pritty, ip->sc_lock) == FALSE) {
			if (firstopen) {
				tplock = LOCK(ip->sc_lock, plstr);
				ip->sc_state = 0;
				ip->sc_qptr = NULL;
				rd_q->q_ptr = NULL;
				WR(rd_q)->q_ptr = NULL;
				ip->sc_restart_read = 0;
				UNLOCK(ip->sc_lock, tplock);
			}
                	return (EINTR);		/* Signalled out of wait */
		}
        }
        bp->b_datap->db_type = M_SETOPTS;
        bp->b_wptr += sizeof(struct stroptions);
	/*LINTED*/
        sop = (struct stroptions *)bp->b_rptr;
        sop->so_flags = SO_ISTTY;
        putnext(rd_q, bp);

	if (firstopen)
		qprocson(rd_q);		/* Enable the queue service routines */

	return(0);
}

/*
 * int
 * scclose(queue_t *, int, cred_t *)
 * 	Close an SSM serial port stream.
 *
 * Calling/Exit State:
 *	This function may block.  
 *	
 *	The port's sc_lock must not be held upon entry or exit.
 *
 *	The queue is emptied of all input and output, then
 *	detached from the device's state information structure.
 *
 *	Always returns zero.
 *
 * Description:
 *	Stop solicitation of input from the SSM, pushing buffered
 *	input upstream or flushing it.  If output remains buffered,
 *	allow it a reasonable amount of time to drain.  If unacceptable 
 *	to wait or it doesn't drain fast enough, abort the remainder.  
 * 	Then diable service routines for the queue, deallocate remaining 
 *	message blocks, and perform disconnect prior to returning.
 */
/*ARGSUSED*/
int
scclose(queue_t *rd_q, int flag, cred_t *crp)
{
	register struct sc_info *ip = (struct sc_info *) rd_q->q_ptr;
	pl_t	tplock;
	short wait, numq;
	toid_t	bid;
	int	drainid;
	bool_t	status;

	tplock = LOCK(ip->sc_lock, plstr);
	ip->sc_restart_read = 0;
	/*
	 * If the carrier is still active and the 
	 * NDELAY flag is not set, try to process 
	 * all queued output.
	 */
	if ((ip->sc_state & CARR_ON) && ((flag & FNDELAY) == 0)) {
		numq = qsize(WR(rd_q));
		if (numq || (ip->sc_state & BUSY)){
			/*
			 * In the following code segment, WCLOSE is cleared
			 * either when sc_draintime() fires or when scwsrv()
			 * observes that the output queue is empty.  
			 * sc_draintime() also sets the DRAINTIME flag if
			 * the queue is not empty.
			 */
			ip->sc_state |= WCLOSE;
			ip->sc_flags &= ~DRAINTIME;
			wait = ( (short)(((numq * SC_AVGCNT) + ip->sc_tx_count)
				* (short)scbaud_time[(ip->sc_cflags & CBAUD)])
				/ (short)1000) + (short)sc_global.addwait;
			drainid = itimeout(sc_draintime, 
					   (caddr_t)rd_q, wait * HZ, plstr);

			if (drainid == 0) {
				/*
				 *+ The sc-driver encountered internal condition
				 *+ during an attempt to close the indicated 
				 *+ console port, in which it was unable to 
				 *+ wait for remaining output to be performed.  
				 *+ The extraneous data has been flushed in 
				 *+ order to recover the port.
				 */
				cmn_err(CE_WARN, "sc%d: Output lost on close.", 
					ip->sc_unit);
				ip->sc_state &= ~WCLOSE;
			}

			while (ip->sc_state & WCLOSE) {
				status = SV_WAIT_SIG(ip->sc_drain_wait, 
					pritty, ip->sc_lock);
				tplock = LOCK(ip->sc_lock, plstr);

				if (ip->sc_flags & DRAINTIME) {
					/* Output not finish; we timed out. */
					sc_abort_write(ip);
				} else if (ip->sc_state & WCLOSE) {
					/* 
					 * Output is still draining, timeout
					 * has not occurred; awakened due to 
					 * a signal(s).  Resume wait if it 
					 * was a job control stop/resume.  
					 * Otherwise, flush the remaining data 
					 * and complete the close().
					 */
					if (status == FALSE) {
						/* Not job control */
						UNLOCK(ip->sc_lock, tplock);
						untimeout(drainid);
						tplock = LOCK(ip->sc_lock, 
							plstr);
						ip->sc_state &= ~WCLOSE;
						sc_abort_write(ip);
					}
				} else {
					/* 
					 * The output has drained and the
					 * timeout needs to be canceled.
					 */
					UNLOCK(ip->sc_lock, tplock);
					untimeout(drainid);
					tplock = LOCK(ip->sc_lock, plstr);
				}
			}

		} 
	} else {
		/*
		 * If the carrier is off, or NDELAY is set,
		 * kill the current output.
		 */
		sc_abort_write(ip);
	}

	/* in all close cases, release all allocated messages */
	flushq(WR(rd_q), FLUSHALL);
	freemsg(ip->sc_msg);
	ip->sc_msg = NULL;

	if ((ip->sc_cflags & HUPCL) == 0) {
		ip->sc_modem &= ~(CCM_DTR | CCM_RTS | CCM_BREAK);
		sc_set_modes(ip);
	}

	sc_abort_read(ip);
	bid = ip->sc_bufcallid;
	ip->sc_bufcallid = 1;	/* prevent further buffcalls */
	ip->sc_state &= ~ISOPEN;
	UNLOCK(ip->sc_lock, tplock);
	qprocsoff(rd_q);
	if (bid) {
		unbufcall(bid);
	}
	if (ip->sc_timeoutid != 0)
		untimeout(ip->sc_timeoutid);
	if (ip->sc_delayid != 0)
		untimeout(ip->sc_delayid);

	tplock = LOCK(ip->sc_lock, plstr);
	ip->sc_state = 0;
	freemsg(ip->sc_msg);
	ip->sc_msg = NULL;
	rd_q->q_ptr = NULL;
	WR(rd_q)->q_ptr = NULL;
	ip->sc_qptr = NULL;
	UNLOCK(ip->sc_lock, tplock);
	return (0);
}

/*
 * STATIC void
 * screcover(struct sc_info *)
 * 	Input bufcall() recovery interface.
 *
 * Calling/Exit State:
 *	Has no process context and does not block.
 *
 *	The port's sc_lock must not be held upon entry or exit.
 *
 * 	Read requests to the SSM for this port have
 *	been halted until data already in the receive
 *	buffer can be passed up the stream.
 *
 *	Clears ip->sc_bufcallid and ip->sc_timeoutid if a buffer 
 *	was available, indicating that no input bufcalls or
 *	timeouts are pending.  Resets it with another bufcall()
 *	or itimeout(), if bufcall() fails, otherwise. 
 *	
 *	No return value.
 *
 * Description:
 * 	Try to get a streams buffer for the input data already
 *	stored in the receiver buffer.  If we *still* cannot get 
 * 	a buffer, the only thing to do is bufcall again.  Otherwise, 
 *	load the buffer with input data and send it upstream.
 */
STATIC void
screcover(struct sc_info *ip)
{
	mblk_t	*bp;
	pl_t tplock = LOCK(ip->sc_lock, plstr);
	int count = SCRXSIZE - ip->sc_rcb->rcb_count;

	if ((bp = allocb(count, BPRI_MED)) == NULL) {
		ip->sc_bufcallid = 
			bufcall((uint_t) count, BPRI_MED, screcover, (long)ip);
		if (ip->sc_bufcallid == 0) {
			ip->sc_timeoutid = itimeout(screcover, (caddr_t)ip,
				drv_usectohz(SC_BUF_WAIT), plstr);
			if (ip->sc_timeoutid == 0) {
				ip->sc_restart_read = 1;
				UNLOCK(ip->sc_lock, tplock);
				sc_start_read(ip);
				(void)putctl(WR(ip->sc_qptr), M_STARTI);
                                /*
                                 *+ The sc-driver encountered an input-overrun
                                 *+ condition internally on the indicated port.
                                 *+ The extraneous data has been lost and 
				 *+ input restarted on that port in an attempt
				 *+ to recover.
                                 */
                                cmn_err(CE_WARN,
                                	"sc%d: Input overrun - data lost.",
					ip->sc_unit);
                                return;

			}
		} else {
			ip->sc_timeoutid = 0;
		} 
	} else {
		ip->sc_bufcallid = 0;
		ip->sc_timeoutid = 0;
		bcopy((char *) &ip->sc_rx_buf[0], 
		      (char *)bp->b_wptr, (unsigned) count);
		bp->b_wptr += count;
		if (canputnext(RD(ip->sc_qptr))) {
			ip->sc_restart_read = 1;
			UNLOCK(ip->sc_lock, tplock);
			putnext(RD(ip->sc_qptr), bp);
			sc_start_read(ip);
			(void)putctl(WR(ip->sc_qptr), M_STARTI);
			return;		/* All done */
		} else
			putq(RD(ip->sc_qptr), bp);
	}
	UNLOCK(ip->sc_lock, tplock);
}

/*
 * STATIC void
 * scintr(int)
 * 	SSM serial port interrupt service routine.
 *
 * Calling/Exit State:
 *	Has no process context and does not block.
 *
 *	The port's sc_lock must not be held upon entry or exit.
 *
 * 	Depending on the basis of the interrupt, may
 *	queue incoming data upstream or release message
 *	buffers for outgoing requests.
 *
 *	No return value.
 *
 * Description:
 *	Verify that the specified interrupt vector maps to a valid
 * 	sc-device and that it is functional; discard the interrupt
 * 	otherwise.  Translate the vector to its corresponding transfer
 * 	or receive SSM CB and dispatch it and update system statistics
 *	based upon its current status.
 *
 *	Receive CBs generally are processed by aquiring a streams buffer,
 *	copying their data there from a driver buffer, then queuing the 
 *	message upstream.  If flow control inhibits upstream data movement 
 *	the message is queued temporarily to the device's read queue for 
 *	processing later and no further input is solicited from the SSM
 *	until the stream unblocks.  If a streams buffer cannot be acquired, 
 *	a bufcall is scheduled, for which screcover() will be invoked 
 *	and continue processing of the input data.
 *
 * 	Modem status changes for hangups may result in upstream 
 *	notification.  For carrier detect, processes blocked waiting 
 *	for it are notified.  
 *	
 *	Input overrun errors are ignored.  Both framing errors and line 
 *	breaks result in an M_BREAK message being sent upstream for 
 *	processing.  Since there is not a separate message for denoting
 *	parity errors and there are separate terminal settings to control
 *	their filtering, they are processed at this level instead of being
 *	treated much as a break is.  
 *
 *	If parity errors are not to be ignored, they are passed upstream 
 *	as a NUL character, preceded by a 2 character mark if marking has 
 *	been specified.  Note that the actual character value that was
 *	read had already been discarded by the SSM.
 *	
 *	Transfer CBs generally are processed by locating their associated
 *	streams buffer, which is at the head of the devices information
 *	structure's "sc_msg" list, updating their completed transfer
 *	count, and releasing that message if it had been completed entirely.
 *	If more messages are queued, the next is started.  Otherwise, 
 *	qenable() is started to schedule the write-side service routine to
 *	run when there is more to do, in case a noenable was used to put
 *	something back on the head of the queue.
 */
STATIC void
scintr(int vector)
{
	register int count;
	struct cons_rcb *rptr;
	struct cons_xcb *xptr;
	int unit = (vector -= sc_base_vector) >> CCB_INTRSHFT;
	int status, func = vector & 1;
	struct sc_info *ip;
	mblk_t  *bp;
	pl_t tplock;
#ifndef NOFLAVORS
	pl_t flv_pl;
	int flv_copy, i;
	char *cp;
#endif /* NOFLAVORS */

	if (unit < 0 || unit >= sc_ndevs) {
		/*
		 *+ An interrupt was delivered to the sc-driver which
		 *+ translates to an unknown/unused port.  The interrupt 
		 *+ has been discarded.  
		 */
		cmn_err(CE_NOTE,
			"sc: stray interrupt, unit (%d) out of range",unit);
		return;
	}
	ip = sc_info[unit];
	if (!ip->sc_alive) {
		/*
		 *+ An interrupt was delivered to the to the sc-driver
		 *+ which appears to be for a deconfigured, non-functional 
		 *+ port.  The interrupt has been discarded. 
		 */
		cmn_err(CE_NOTE,"sc: stray interrupt, dead unit %d", unit);
		return;
	}
	tplock = LOCK(ip->sc_lock, plstr);

	if ((ip->sc_state & ISOPEN) == 0) {
		UNLOCK(ip->sc_lock, tplock);
		return;
	}
	
	/*
	 * Handle read-CB-done interrupts.
	 */
	if (func == CCB_RECV_CB) {
		(void)drv_setparm(SYSRINT, 1);
		rptr = ip->sc_rcb;
		count = SCRXSIZE - rptr->rcb_count;	/* bytes received */

		switch (rptr->rcb_status) {
		case CCB_BUSY:
			/*
			 * Note:  The cmn_err call that used to be here was
			 * removed due to a funny with the ssm f/w handling
			 * of read flush.  Since the f/w doesn't re-read the
			 * current read CB when a flush command is received,
			 * the old INABLE/NOT IENABLE flag is used.  We don't
			 * want to be interrupted on a read flush, but the
			 * f/w does it anyway.  So, don't print anything here.
			 * Just ignore it.
			 */
			UNLOCK(ip->sc_lock, tplock);
			return;
		case CCB_PERR:
			/*
			 * A parity error occurred on the last character
			 * transmitted.  The character has been discarded,
			 * but we still need to determine if anything 
			 * should be passed upstream.  To pass data upstream
			 * put the data into the xmit buffer and let the
			 * normal data handling code at the end of this
			 * case statement pass it upstream.
			 */
			if (! (ip->sc_iflags & IGNPAR)) {
				if (ip->sc_iflags & PARMRK) {
					/* insert a parity mark */
					/*LINTED*/
					ip->sc_rx_buf[count++] = '\377';
					ip->sc_rx_buf[count++] = '\0';
					ip->sc_rx_buf[count++] = '\0';
					rptr->rcb_count -= 3;
				} else {
					/* Just insert a NUL character */
					ip->sc_rx_buf[count++] = '\0';
					rptr->rcb_count--;
				}
			}
			ip->sc_parity_errs++;
			break;	

		/* 
		 * The SSM may have returned data recieved
		 * along with reporting an event occurring
		 * after that data was recieved. In some
		 * of these cases its desirable to handle
		 * the event *after* attempting to send 
		 * data upstream. 
		 */
		case CCB_FERR:
		case CCB_OVERR:
		case CCB_BREAK:
		case CCB_MS_CHG:
		case CCB_TIMEO:
		case CCB_OK:	
	                break;
		default:
			/*
			 *+ An interrupt was delivered to the to the 
			 *+ sc-driver for the specified port with an 
			 *+ unexpected of termination status.  This 
			 *+ may be an internal error between this driver 
			 *+ and the SSM firmware.  Any associated data 
			 *+ will be processed, but the event will otherwise 
			 *+ be ignored.
			 */ 
			cmn_err(CE_WARN,
				"sc%d: unexpected reciever interrupt", unit);
			break;
		}

		/* Send any received data upstream. */
		if (count > 0) {
			(void)drv_setparm(SYSRAWC, (ulong_t)count);

#ifndef NODEBUGGER
			if (ip->sc_dev == sc_global.cons_id && 
			    ip->sc_rx_buf[0] == CTL('k')) {
				/* 
				 * Effectively throw away the input and
				 * then invoke the Kernel Debugger.  All
				 * processing will be suspended until it
				 * returns, afterwhich input is restarted.
				 */
				(*cdebugger)(DR_USER, NO_FRAME);
			} else
#endif /* !NODEBUGGER */
#ifndef NOFLAVORS
		/*
		 * XXX - The following support is a temporary 
		 * 	 solution to allow flavors to coexist 
		 * 	 with this driver.  Basically, if the
		 * 	 flavors escape is seen, it is be 
		 * 	 discarded and flavors notified.  While
		 *	 the global inflavors flag is set, all
		 * 	 solicited input will be directed to
		 * 	 the flavors input buffer instead of
		 * 	 shipped up the stream.  Finally, note
		 *	 that the kdb trap has precedence over
		 * 	 trapping input for flavors.
		 */
			if (((ip->sc_rx_buf[0] == CTL('f')
			       && ip->sc_dev == sc_global.cons_id)
			    || inflavors) 
			&& flavors_initted) {
				/* 
				 * Siphon off the input to the flavors 
				 * buffer and notify the flavors task 
				 * that may be waiting for the data.
				 * Don't allow the buffer to overflow.
				 */
                                flv_pl = LOCK(&flavors_lock, PLHI);
				if (!inflavors) {
					/*
					 * Delete the escape char and 
					 * activate flavors.
					 */
                                	inflavors = 1;
					cp = (char *)ip->sc_rx_buf + 1;
					count--;
				} else {
					cp = (char *)ip->sc_rx_buf;
					
				}
				flv_copy = min(count, (_FIE - flv_input_empty));
                                flv_input_nchars += flv_copy;
				for (i = 0; i < flv_copy; i++) {
					*flv_input_empty++ = *cp++;
                                	if (flv_input_empty >= _FIE)
                                		flv_input_empty = flv_inputbuf;
				}
                                SV_SIGNAL(&flavors_sv, 0);
                                UNLOCK(&flavors_lock, flv_pl);

				/* 
				 * If the flavor's buffer would 
				 * have overflowed, ignore the
				 * extraneous characters and try
				 * to issue a warning.
				 */
				if (flv_copy > count 
				&& (bp = allocb(1, BPRI_MED)) != NULL) {
				      	*bp->b_wptr++ = CTL('g');
					UNLOCK(ip->sc_lock, tplock);
					(void) put(ip->sc_qptr, bp);
					tplock = LOCK(ip->sc_lock, plstr);
				}
			} else
#endif /* NOFLAVORS */
			if ((bp = allocb(count, BPRI_MED)) == NULL) {
				if (sc_global.printalocfail)
					/*
					  *+ A driver attempt to allocate a
					  *+ stream buffer for incoming data
					  *+ failed.  
				 	 */
					cmn_err(CE_WARN,
					     "sc%d: unable to allocb %d bytes",
						unit, count);
				/*
               			 * bufcall() to wait for streams buffer.
                		 * If that fails, try to schedule a itimeout()
                		 * to perform a similar action.  If that fails
                		 * as well, then we have little choice but to
                		 * pitch the data and continue, since there is
                		 * no apparent way to reschedule a callback to
                 		 * restart input later.
               			 * If bufcall() succeeds, then also attempt to
                		 * control flow the channel to stop more input
                		 * from being received.  To do this, we must
                		 * send a message to our output service routine
                		 * to perform control flow.  So, yes, we
                		 * really do mean to talk to ourselves!
                		 */
				ip->sc_restart_read = 0;
				if (ip->sc_bufcallid == 0 
				&&  ip->sc_timeoutid == 0) {
					ip->sc_bufcallid = 
						bufcall((uint_t)count, BPRI_MED,
							screcover, (long)ip);
					if (ip->sc_bufcallid == 0) {
						ip->sc_timeoutid = itimeout(
							screcover, (caddr_t)ip,
							drv_usectohz(SC_BUF_WAIT), 
							plstr);
						if (ip->sc_timeoutid == 0) {
							ip->sc_restart_read = 1;
                                			/*
                                			 *+ The sc-driver encountered an input-overrun
                                			 *+ condition internally on the indicated port.
                               				 *+ The extraneous data has been lost and 
							 *+ input restarted on that port in an attempt
							 *+ to recover.
                                			 */
                                			cmn_err(CE_WARN,
                                				"sc%d: Input overrun - data lost.",
							ip->sc_unit);
						}
					} else {
						ip->sc_timeoutid = 0;
					} 
					if (ip->sc_restart_read == 0) {
						UNLOCK(ip->sc_lock, tplock);
						(void) putctl(WR(ip->sc_qptr), 
								M_STOPI);
						tplock = LOCK(ip->sc_lock, 
								plstr);
					}
				}
			} else {
				bcopy((char *) &ip->sc_rx_buf[0], 
				      (char *) bp->b_wptr, (unsigned) count);
				bp->b_wptr += count;
				/*
				 * Put the message upstream if possible.  
				 * Otherwise, put the message on the local 
				 * queue, stop solicitation of more data on 
				 * the correct line while noting we are 
				 * blocked, and tell the write-side service
				 * routine to control flow input (if it is
				 * set).  This control flow may be redundant
				 * in some cases, but harmless.  The message 
				 * on the local queue will be processed by 
				 * the service procedure, scrsrv(), when the
				 * queue is back-enabled.
				 */
				if (canputnext(RD(ip->sc_qptr))) {
					UNLOCK(ip->sc_lock, tplock);
					putnext(RD(ip->sc_qptr), bp);
				} else {
					putq(RD(ip->sc_qptr), bp);
					ip->sc_restart_read = 0;
					UNLOCK(ip->sc_lock, tplock);
					(void) putctl(WR(ip->sc_qptr), M_STOPI);
				}
				tplock = LOCK(ip->sc_lock, plstr);
			} 
		}

		/* Perform event post-processing now */
		switch (rptr->rcb_status) {
		case CCB_MS_CHG:
			(void)drv_setparm(SYSRINT, 1);
			/*
		  	 * If this port's DCD* line changed state,
		 	 * send a message upstream.
			 * We must temporarily reacquire sc_lock 
			 * to get the modes from the SSM and
			 * while changing any port state bits.
			 */
			sc_get_modes(ip);
			if ((ip->sc_modem & CCM_DCD) != 0) {
				ip->sc_state |= CARR_ON;
				SV_BROADCAST(ip->sc_carrier_wait, 0);

			} else if ((ip->sc_cflags & CLOCAL) == 0) {
				ip->sc_state &= ~CARR_ON;
				if (!(ip->sc_state & WOPEN)) {
					UNLOCK(ip->sc_lock, tplock);
					if (ip->sc_dev == sc_global.cons_id) {
						status = putnextctl1( 
								RD(ip->sc_qptr),
								M_SIG, SIGHUP);
					} else {
						status = putnextctl(
								RD(ip->sc_qptr),
								M_HANGUP);
					}
					if (!status) {
						/*
						 *+ An attempt to report carrier
						 *+ drop upstream failed. 
				 	 	 */
						cmn_err(CE_WARN,
						"sc%d:lost carrier drop report",
							unit);
					}
					tplock = LOCK(ip->sc_lock, plstr);
				}
			}
			break;
		
		case CCB_FERR:
			ip->sc_frame_errs++;
			/* Fall into the BREAK code */
			/* LINTED */
		case CCB_BREAK:
			UNLOCK(ip->sc_lock, tplock);
			if (!putnextctl(RD(ip->sc_qptr), M_BREAK)) {
				/*
				 *+ The sc-driver's attempt to report a break
				 *+ detect upstream failed.  
		 		 */
				cmn_err(CE_WARN,
					"sc%d: break detect report failed", 
					unit);	
			}
			tplock = LOCK(ip->sc_lock, plstr);
			break;	

		case CCB_OVERR:
			ip->sc_overruns++;
			break;	
		
		default:
	                break;		/* already done */
		}

		if (ip->sc_restart_read)
			sc_start_read(ip);

	} else {  
		/* Handle completed write operations */
		(void)drv_setparm(SYSXINT, 1);
		xptr = ip->sc_xcb;
		/* If CB still busy, ignore spurious interrupt */	
		if (xptr->xcb_status == CCB_BUSY) {
			UNLOCK(ip->sc_lock, tplock);
			return;
		}
		ip->sc_state &= ~BUSY;
		if ((bp = ip->sc_msg) != NULL) {
			if (xptr->xcb_count != 0)
				bp->b_rptr += 
					(ip->sc_tx_count - xptr->xcb_count);
			else {
				bp = ip->sc_msg->b_cont;
				ip->sc_msg->b_cont = NULL;
				freeb(ip->sc_msg);
				ip->sc_msg = bp;
			}
			if (ip->sc_msg != NULL) {
				ASSERT(!(ip->sc_state & TXDELAY));
				sc_next_msg(ip, ip->sc_msg);
			} else {
				/*
				 * The queue must be re-enabled
				 * because there had been message
				 * put back message using noenable().
				 */
				qenable(ip->sc_qptr);
			}
		}
	}
	UNLOCK(ip->sc_lock, tplock);
}

/*
 * int
 * scrsrv(queue_t *)
 * 	Read service procedure for queued messages.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must not be held upon entry or exit.
 *
 *	Returns zero althought it is expected to be ignored.
 *
 * Description:
 *	Takes messages off the queue and attempts to pass them
 *	up the stream.  If it cannot, then it requeues the request
 *	and exits.  It really expects to find only one message
 *	queued, but is prepared for more just in case.
 *
 *	Once the queue is emptied, and since it likely was invoked
 *	after messages were blocked from being passed up the stream, 
 *	this function must also restart solicitation of new input 
 *	from the SSM, which had been stopped also.
 */
int
scrsrv(queue_t *rd_q)
{
	pl_t tplock;
	mblk_t *bp;
	struct sc_info *ip;

	ASSERT(rd_q->q_ptr != NULL);

	while ((bp = getq(rd_q)) != NULL) {
		if (canputnext(rd_q)) {
			putnext(rd_q, bp);
		} else {
			putbq(rd_q, bp);
			return (0);
		}
	}

	ip = (struct sc_info *) rd_q->q_ptr;
	tplock = LOCK(ip->sc_lock, plstr);
	ip->sc_restart_read = 1;
	sc_start_read(ip);
	UNLOCK(ip->sc_lock, tplock);
	(void) putctl(WR(rd_q), M_STARTI);
	return (0);
}

/*
 * STATIC int
 * sc_doioctl(struct sc_info *, mblk_t *)
 *	Process the specified M_IOCTL message.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must not be held upon entry or exit.
 *
 *	Returns zero if the port currently is busy and the command
 *	could not be executed at this time, one otherwise.
 *	
 * Description:
 * 	Process the message by calling sc_param() to set up hardware
 * 	line parameters, using the message cb.  Acknowlege the message, 
 *	setting its completion status accordingly, and send it back 
 *	upstream with qreply().  If we do not recognize the request 
 *	set its completion status to M_IOCNAK.
 */
STATIC int
sc_doioctl(struct sc_info *ip, mblk_t *mp)
{
	struct iocblk *iocp;
	struct termio *tptr;
	struct termios *sptr;
	mblk_t *bp;
	queue_t *wr_q = (queue_t *) ip->sc_qptr;
	int oldcflags;
	uchar_t oldflow, oldstart, oldstop;
	pl_t tplock;

	tplock = LOCK(ip->sc_lock, plstr);
	if((ip->sc_state & BUSY) != 0) {
		UNLOCK(ip->sc_lock, tplock);
		return (0);
	}

	if (mp->b_wptr - mp->b_rptr < sizeof (struct iocblk)) {
		/*
		 *+ A driver write put routine received a stream
		 *+ message of type M_IOCTL or M_PCIOCTL, but the message did
		 *+ not include a data block containing subcommands and
		 *+ parameters.  
		 */
		cmn_err(CE_WARN, 
			"sc%d: IOCTL message received with NULL data ptr",
			ip->sc_unit);
		mp->b_datap->db_type = M_IOCNAK;
		UNLOCK(ip->sc_lock, tplock);
		qreply(wr_q, mp);
		return (1);
	}
	/*LINTED*/
	iocp = (struct iocblk *) mp->b_rptr;

	switch (iocp->ioc_cmd) {
	case TCSETS:
	case TCSETSF:
	case TCSETSW:
	case TCSETA:
	case TCSETAF:
	case TCSETAW:
	case TCSBRK:
		if (mp->b_cont == NULL) {
			/*
			 *+ A driver write put routine received a stream
			 *+ message of type M_IOCTL or M_PCIOCTL, but the 
			 *+ message did not include a continuation block 
			 *+ for its parameters.  
			 */
			cmn_err(CE_WARN, 
			"sc%d: IOCTL message received w/ NULL continuation ptr",
				ip->sc_unit);
			mp->b_datap->db_type = M_IOCNAK;
			UNLOCK(ip->sc_lock, tplock);
			qreply(wr_q, mp);
			return (1);
		}
		break;

	case TCGETA:
	case TCGETS:
	default:
		/* mp->b_cont doesn't need to be set in these cases */
		break;
	}

	switch (iocp->ioc_cmd) {
	case TCSETAF:			/* Set params w/ read flush (termio) */
	case TCSETAW:			/* Set params w/o flush (termio) */
		/* 
		 * Set params using a struct termio after waiting
		 * for current output to drain before processing
		 * port parameters.  Identical to the TCSETA case 
		 * which follows except that we abort read operations 
	 	 * and then restart them upon completion.  
		 */
		sc_abort_read(ip);
		/* just fall through */
		/* LINTED */
	case TCSETA:			/* Set parameters (termio) */
		/*LINTED*/
		tptr = (struct termio *) mp->b_cont->b_rptr;
		oldcflags = ip->sc_cflags;
		ip->sc_cflags = (ip->sc_cflags & 0xffff0000) | tptr->c_cflag;

		/*
		 * Look at the termio iflags to see what flow control
		 * is requested in this message.  Set up the appropriate
		 * flags for later transfer to the SSM f/w via sc_param().
		 */
		oldflow = ip->sc_ld0flow;
		if (tptr->c_iflag & IXON) {
			ip->sc_ld0flow = CCF_XOFF;
		} else if (tptr->c_iflag & IXANY) {
			ip->sc_ld0flow = CCF_XANY;
		} else {	
			ip->sc_ld0flow = CCF_NOFLOW;
		}

		if ((iocp->ioc_error = sc_param(ip))) {
			ip->sc_cflags = oldcflags;
			ip->sc_ld0flow = oldflow;
			mp->b_datap->db_type = M_IOCNAK;
		} else {
			ip->sc_iflags = 
				(ip->sc_iflags & 0xffff0000) | tptr->c_iflag;
			if (ip->sc_cflags & CLOCAL)
				ip->sc_state |= CARR_ON;
			mp->b_datap->db_type = M_IOCACK;
		}
		
		if (iocp->ioc_cmd == TCSETAF) {
			sc_start_read(ip);
			UNLOCK(ip->sc_lock, tplock);
			if (!putnextctl1(RD(wr_q), M_FLUSH, FLUSHR)) {
				/*
				 *+ A driver attempt to send a read flush 
				 *+ message upstream after processing a 
				 *+ TCSETAF message failed.
				 */
				cmn_err(CE_WARN, 
					"sc%d: TCSETAF flush message failed",
					ip->sc_unit);
			}
			tplock = LOCK(ip->sc_lock, plstr);
		}
		iocp->ioc_count = 0;
		break;

	case TCSBRK:
		/*LINTED*/
		if (*(int *) mp->b_cont->b_rptr == 0) {
			ip->sc_modem |= CCM_BREAK;
			sc_set_modes(ip);
			ip->sc_modem &= ~CCM_BREAK;
		}
		mp->b_datap->db_type = M_IOCACK;
		iocp->ioc_count = 0;
		break;

	case TCGETA:					/* Get parameters */
		if (mp->b_cont)
			freemsg(mp->b_cont);
		if ((bp = allocb(sizeof(struct termio), BPRI_MED)) == NULL) {
			/*
			 *+ An attempt to allocate a streams data 
			 *+ buffer for TCGETA parameters failed.  
			 */
			cmn_err(CE_WARN, 
				"sc%d: TCGETA data block allocb failed",
				ip->sc_unit);
			mp->b_datap->db_type = M_IOCNAK;
			break;
		}
		bzero((char *) bp->b_rptr, sizeof(struct termio));
		mp->b_cont = bp;

		/*LINTED*/
		tptr = (struct termio *) mp->b_cont->b_wptr;
		tptr->c_cflag = (ushort)ip->sc_cflags;
		tptr->c_iflag = (ushort)ip->sc_iflags;
		mp->b_cont->b_wptr += sizeof(struct termio);
		iocp->ioc_count = sizeof(struct termio);
		mp->b_datap->db_type = M_IOCACK;
		break;

	case TCSETSF:			/* Set params w/ read flush (termios) */
	case TCSETSW:			/* Set params w/o flush (termios) */
		/* 
		 * Set params using a struct termios after waiting
		 * for current output to drain before processing
		 * port parameters.  Identical to the TCSETS case 
		 * which follows except that we abort read operations 
	 	 * and then restart them upon completion.  
		 */
		sc_abort_read(ip);
		/* just fall through */
		/* LINTED */
	case TCSETS:		/* Set parameters (termios) */
		/*LINTED*/
		sptr = (struct termios *) mp->b_cont->b_rptr;
		oldcflags = ip->sc_cflags;
		ip->sc_cflags = (int) sptr->c_cflag;

		/*
		 * Look at the termios iflags to see what flow control
		 * is requested in this message.  Set up the appropriate
		 * flags for later transfer to the SSM f/w via sc_param().
		 */
		oldflow = ip->sc_ld0flow;
		if (sptr->c_iflag & IXON) {
			ip->sc_ld0flow = CCF_XOFF;
		} else if (sptr->c_iflag & IXANY) {
			ip->sc_ld0flow = CCF_XANY;
		} else {	
			ip->sc_ld0flow = CCF_NOFLOW;
		}

		oldstart = ip->sc_ld0xon;
		oldstop = ip->sc_ld0xoff;
		ip->sc_ld0xon  = sptr->c_cc[VSTART];
		ip->sc_ld0xoff = sptr->c_cc[VSTOP];

		if ((iocp->ioc_error = sc_param(ip))) {
			ip->sc_cflags = oldcflags;
			ip->sc_ld0flow = oldflow;
			ip->sc_ld0xon  = oldstart;
			ip->sc_ld0xoff = oldstop;
			mp->b_datap->db_type = M_IOCNAK;
		} else {
			ip->sc_iflags = sptr->c_iflag;
			if (ip->sc_cflags & CLOCAL)
				ip->sc_state |= CARR_ON;
			mp->b_datap->db_type = M_IOCACK;
		}
		
		if (iocp->ioc_cmd == TCSETSF) {
			sc_start_read(ip);
			UNLOCK(ip->sc_lock, tplock);
			if (!putnextctl1(RD(wr_q), M_FLUSH, FLUSHR)) {
				/*
				 *+ A driver attempt to send a read flush 
				 *+ message upstream after processing a 
				 *+ TCSETSF message failed.
				 */
				cmn_err(CE_WARN, 
					"sc%d: TCSETSF flush message failed",
					ip->sc_unit);
			}
			tplock = LOCK(ip->sc_lock, plstr);
		}
		iocp->ioc_count = 0;
		break;

	case TCGETS:
		if (mp->b_cont)
			freemsg(mp->b_cont);
		if ((bp = allocb(sizeof(struct termios), BPRI_MED)) == NULL) {
			/*
			 *+ An attempt to allocate a streams data 
			 *+ buffer for TCGETS parameters failed.  
			 */
			cmn_err(CE_WARN, 
				"sc%d: TCGETS data block allocb failed",
				ip->sc_unit);
			mp->b_datap->db_type = M_IOCNAK;
			break;
		}
		bzero((char *) bp->b_rptr, sizeof(struct termios));
		mp->b_cont = bp;
		/*LINTED*/
		sptr = (struct termios *)mp->b_cont->b_wptr;
		sptr->c_cflag = (tcflag_t) ip->sc_cflags;
		sptr->c_iflag = (tcflag_t) ip->sc_iflags;
		sptr->c_cc[VSTART] = ip->sc_ld0xon;
		sptr->c_cc[VSTOP] = ip->sc_ld0xoff;

		mp->b_cont->b_wptr += sizeof(struct termios);
		iocp->ioc_count = sizeof(struct termios);
		mp->b_datap->db_type = M_IOCACK;
		break;

	default:
		/* nak the ioctl */
		mp->b_datap->db_type = M_IOCNAK;
		break;
	}
	UNLOCK(ip->sc_lock, tplock);
	qreply(wr_q, mp);
	return (1);
}



/*
 * STATIC void
 * scwinioctl(queue_t *q, mblk_t *mp)
 *	Process *WINSZ IOCTL messages
 *
 * Calling/Exit State:
 *	Message must be of type M_IOCTL or M_IOCDATA
 *	to be called.
 */
STATIC void
scwinioctl(queue_t *q, mblk_t *mp)
{
	struct sc_info *ip = (struct sc_info *) q->q_ptr;
	struct iocblk *iocp;
	struct winsize *wb;
	struct copyreq *send_buf_p;
	mblk_t *tmp;
	pl_t pl;

	ASSERT(q == WR(q));
	ASSERT(ip);
	/* LINTED pointer alignment */
	iocp = (struct iocblk *) mp->b_rptr;

	switch (iocp->ioc_cmd) {

	case TIOCGWINSZ:

		pl = LOCK(ip->sc_lock, plstr);
		if (! (tmp = allocb(sizeof(struct winsize), 0))) {
			UNLOCK(ip->sc_lock, pl);
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
		wb->ws_row = ip->sc_wsz.ws_row;
		wb->ws_col = ip->sc_wsz.ws_col;
		wb->ws_xpixel = ip->sc_wsz.ws_xpixel;
		wb->ws_ypixel = ip->sc_wsz.ws_ypixel;
		UNLOCK(ip->sc_lock, pl);
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
		pl = LOCK(ip->sc_lock, plstr);
		if ((ip->sc_wsz.ws_row == wb->ws_row) &&
		    (ip->sc_wsz.ws_col == wb->ws_col) &&
		    (ip->sc_wsz.ws_xpixel == wb->ws_xpixel) &&
		    (ip->sc_wsz.ws_ypixel == wb->ws_xpixel)) {
			UNLOCK(ip->sc_lock, pl);
			iocp->ioc_count = 0;
			iocp->ioc_error = 0;
			iocp->ioc_rval = 0;
			mp->b_datap->db_type = M_IOCACK;
			qreply(q, mp);
			break;
		}
		ip->sc_wsz.ws_row = wb->ws_row;
		ip->sc_wsz.ws_col = wb->ws_col;
		ip->sc_wsz.ws_xpixel = wb->ws_xpixel;
		ip->sc_wsz.ws_ypixel = wb->ws_ypixel;
		UNLOCK(ip->sc_lock, pl);
		iocp->ioc_count = 0;
		iocp->ioc_error = 0;
		iocp->ioc_rval = 0;
		mp->b_datap->db_type = M_IOCACK;
		qreply(q, mp);
		/*
		 * SIGWINCH is sent always upstream
		 */
		putnextctl1(RD(q), M_SIG, SIGWINCH);
		break;

	default:
		cmn_err(CE_PANIC,"scwinioctl: Illegal ioctl");
			/*NOTREACHED*/
	}
}


/*
 * STATIC int
 * scmsg(queue_t *, mblk_t *)
 * 	write-side stream message handler.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must not be held upon entry or exit.
 *
 *	Possible return values:
 *
 *	MSGDONE 	the message completed successfully and has 
 *			been release or acknowledged accordingly.
 *
 *	STDPUTBACK	the message could not be executed. Return it
 *			to the queue and reschedule its execution.
 *
 *	PRIPUTBACK	a control flow stop message could not be 
 *			executed.  Return it to the head of the
 *			queue, but be aware that an external event
 *			is required to restart its execution; do
 *			not reschedule it right now.
 *
 * Remarks:
 *  	If the message heading downstream is of type M_FLUSH,
 *  	flush the appropriate queues (read or write.)  If the
 *	flush message is NOT for the upstream path (read flush),
 *	it is released.  Otherwise, the message is sent upstream.
 * 
 *	scmsg() may be called when the console port's output side
 *	is busy.  It is up to the code to decide whether a message
 *	can be processed if the output side is busy.  
 */
STATIC int
scmsg(queue_t *wr_q, mblk_t *mp)
{
	struct sc_info *ip = (struct sc_info *) wr_q->q_ptr;
	pl_t tplock;
	struct iocblk *iocp;
        struct winsize *wb;
	struct copyreq *get_buf_p;
        mblk_t *tmp;
	struct copyresp *resp;  /* the transparent ioctl response structure */


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
                        scwinioctl(wr_q, mp);
                        break;
		case TIOCGWINSZ:
                        /* LINTED pointer alignment */
                        iocp = (struct iocblk *) mp->b_rptr;
                        iocp->ioc_error = 0;
                        iocp->ioc_count = 0;
                        iocp->ioc_rval = 0;
                        mp->b_datap->db_type = M_IOCACK;
                        qreply(wr_q, mp);
                        break;
		}

		break;

	case M_IOCTL:
                /*
                 * NOTE: for each "set" type operation a copy of the M_IOCTL
                 * message is made and passed downstream.
                 */
                /* LINTED pointer alignment */
                iocp = (struct iocblk *) mp->b_rptr;

                switch (iocp->ioc_cmd) {

	        case TIOCGWINSZ:
                        scwinioctl(wr_q, mp);
                        break;

              	case TIOCSWINSZ:
                        if (iocp->ioc_count != TRANSPARENT) {
                                scwinioctl(wr_q, mp);
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
                        qreply(wr_q, mp);
                        break;

		default:
			if (sc_doioctl(ip, mp) == 0)
				return(STDPUTBACK);  /* It was not executed */
			break;
		}
		break;

	case M_STOP:		/* Suspend data to device */
		tplock = LOCK(ip->sc_lock, plstr);
		ip->sc_state |= SUSPOUT;
		if (ip->sc_state & BUSY) 
			sc_suspend_out(ip);
		UNLOCK(ip->sc_lock, tplock);
		freemsg(mp);
		break;
	case M_START:		/* Unsuspend data to device */
		tplock = LOCK(ip->sc_lock, plstr);
		if (ip->sc_state & SUSPOUT) {
			ip->sc_state &= ~SUSPOUT;
			if (ip->sc_state & BUSY)
				sc_restart_out(ip);
		}
		UNLOCK(ip->sc_lock, tplock);
		freemsg(mp);
		break;
	case M_STOPI:		/* Suspend data from device */
		tplock = LOCK(ip->sc_lock, plstr);
		if (ip->sc_state & BUSY) {
			UNLOCK(ip->sc_lock, tplock);
			return(PRIPUTBACK);
		}
		ip->sc_modem &= ~CCM_RTS;
		sc_set_modes(ip);
		if ((ip->sc_iflags & IXOFF) != 0) {
			sc_start_write(ip, 1, &ip->sc_ld0xoff);
		}
		UNLOCK(ip->sc_lock, tplock);
		freemsg(mp);
		break;
	case M_STARTI:		/* Unsuspend data from device */
		tplock = LOCK(ip->sc_lock, plstr);
		if (ip->sc_state & BUSY) {
			UNLOCK(ip->sc_lock, tplock);
			return(PRIPUTBACK);
		}
		ip->sc_modem |= CCM_RTS;
		sc_set_modes(ip);
		if ((ip->sc_iflags & IXOFF) != 0) {
			sc_start_write(ip, 1, &ip->sc_ld0xon);
		}
		UNLOCK(ip->sc_lock, tplock);
		freemsg(mp);
		break;
	case M_PCCTL:
		/*
		 *+ A driver write put routine received a stream
		 *+ message of type M_PCCTL, but the message did
		 *+ not include a data block containing a subcommand and
		 *+ its parameters.  
		 */
		cmn_err(CE_WARN, "sc%d: unsupported M_PCCTL message received",
			ip->sc_unit);
		freemsg(mp);
		break;
	case M_FLUSH:				/* Flush data and SSM */
		if (*mp->b_rptr & FLUSHW) {
			tplock = LOCK(ip->sc_lock, plstr);
			flushq(wr_q, FLUSHDATA);
			sc_abort_write(ip);
			if (ip->sc_msg) {
				freemsg(ip->sc_msg);
				ip->sc_msg = NULL;
			}
			ip->sc_state &= ~BUSY;
			UNLOCK(ip->sc_lock, tplock);
		}
		if (*mp->b_rptr & FLUSHR) {
			tplock = LOCK(ip->sc_lock, plstr);
			flushq(RD(wr_q), FLUSHDATA);
			sc_abort_read(ip);
			if (ip->sc_restart_read)
				sc_start_read(ip);
			UNLOCK(ip->sc_lock, tplock);
			*mp->b_rptr &= ~FLUSHW;
			qreply(wr_q, mp);
		} else {
			freemsg(mp);
		}
		break;
	case M_DELAY:
	case M_DATA:	                      /* Std. data or ioctl messages */
		tplock = LOCK(ip->sc_lock, plstr);
		if ((ip->sc_state & (BUSY | SUSPOUT | TXDELAY)) == 0) {
			sc_next_msg(ip, mp);
			UNLOCK(ip->sc_lock, tplock);
		} else {
			UNLOCK(ip->sc_lock, tplock);
			return(STDPUTBACK);
		}
		break;
	default:
		freemsg(mp);
		break;
	}
	return(MSGDONE);
}

/*
 * int
 * scwsrv(queue_t *)
 * 	Write service procedure for queued messages.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must not be held upon entry or exit.
 *
 *	Returns zero althought it is expected to be ignored.
 *
 * Description:
 *	Takes messages off the queue and invokes scmsg() to process 
 *	them.  If it cannot complete the message at this time, it 
 *	requeues the request, scheduling it for later execution,
 *	and exits.
 *
 *	Once the queue is emptied, awaken any process blocked 
 *	waiting for this queue to drain.
 */
int 
scwsrv(queue_t *wr_q)
{
	pl_t	tplock;
	mblk_t *bp;
	mblk_t *mb;
	struct sc_info *ip;

	ASSERT(wr_q->q_ptr != NULL);

	while ((bp = getq(wr_q)) != NULL) {
		switch (scmsg(wr_q, bp)) {
		case STDPUTBACK:
			putbq(wr_q, bp);
			return (0);
		case PRIPUTBACK:
			noenable(wr_q);
			tplock = freezestr(wr_q);
			strqget(wr_q, QFIRST, QNORM, (long *)&mb);
			insq(wr_q, mb, bp);
			unfreezestr(wr_q, tplock);
			enableok(wr_q);
			return (0);
		case MSGDONE:
			break;
#ifdef DEBUG
		default:
			/*
			 *+ An internal driver error has occurred;
			 *+ the value returned from one internal
			 *+ function was unexpected by its caller.
			 */
			cmn_err(CE_PANIC,"scwsrv: unexpected scmsg() return");
			/*NOTREACHED*/
#endif
		}
 	}

	ip = (struct sc_info *) wr_q->q_ptr;
	tplock = LOCK(ip->sc_lock, plstr);
	if (ip->sc_state & WCLOSE) {
		ip->sc_state &= ~WCLOSE;
		SV_SIGNAL(ip->sc_drain_wait, 0);
	}
	UNLOCK(ip->sc_lock, tplock);
	return (0);
}

/*
 * STATIC int
 * sc_param(struct sc_info *)
 * 	set line parameters on the SSM serial port hardware.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must be held upon entry.
 *
 * 	Returns EINVAL if baud rate not supported, zero otherwise
 *
 * Description:
 *	Translates the current settings withing ip->sc_cflags into
 *	a format that can be communicated to the SSM, then does
 *	so.  This resets the current baud rate, character size, 
 *	stop bit mask, and parity. 
 */
STATIC int
sc_param(register struct sc_info *ip)
{
	ushort_t cflags = ip->sc_cflags;
	uchar_t speed = sc_mrates[(cflags & CBAUD)];

	ASSERT(!(ip->sc_state & BUSY));

	if (speed == BNOSUPP) {
		/*
		 *+ The console does not support 1800, 600, 200, 150, 134.5,
		 *+ 110, 75, or 50 baud.
		 */
		cmn_err(CE_NOTE, "sc%d: illegal baud rate requested",
			ip->sc_unit);
		return (EINVAL);
	}
	if (speed == BHANGUP) {
		ip->sc_modem &= ~(CCM_RTS | CCM_BREAK | CCM_DTR);
		sc_set_modes(ip);
		return (0);
	} 
	ip->sc_baud = speed;		/* Now set the baud rate */

	/* Clear parameter bits to be reset */
	ip->sc_parsiz &= ~(CCP_CSMASK | CCP_PMASK | CCP_STMASK);		

	switch (cflags & CSIZE) {	/* Set character size */
	case CS8:
		ip->sc_parsiz |= CCP_CSIZ8;
		break;
	case CS7:
		ip->sc_parsiz |= CCP_CSIZ7;
		break;
	case CS6:
		ip->sc_parsiz |= CCP_CSIZ6;
		break;
	case CS5:
		/*LINTED*/
		ip->sc_parsiz |= CCP_CSIZ5;
		break;
	}

	if (cflags & PARENB) { 		/* Set parity bits */
		if (cflags & PARODD) 
			ip->sc_parsiz |= CCP_ODDP;
		else
			ip->sc_parsiz |= CCP_EVENP;
	}
	

	if (cflags & CSTOPB) 		/* Clear stop bit mask*/
		ip->sc_parsiz |= CCP_ST2;
	else
		/* LINTED */
		ip->sc_parsiz |= CCP_ST1;

	sc_set_modes(ip); 		/* Send new parameters to the SSM */

	return (0);
}

/*
 * STATIC void
 * sc_next_msg(struct sc_info *, mblk_t *)
 * 	Send characters to the serial port or delay its output.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must be held and the
 *	output busy flag must be clear upon entry.
 *
 *	The port's SSM "message" cb must not already be in use.
 *
 *	No return value.
 *	
 * Description:
 *	If the message is of type M_DELAY then the data must
 *	be a struct specifying the delay period.  Locate it,
 *	stop output, and schedule a call into sc_delaytime()
 *	to restart output after the specified time period.
 *
 *	For messages of type M_DATA, verify that they still 
 *	contain unwritten data, mark the device as busy, then 
 *	call sc_start_write to communicate the request to the 
 *	SSM, which will execute it.
 */
STATIC void
sc_next_msg(struct sc_info *ip, mblk_t *bp)
{
	ulong_t nch;

	ASSERT(!(ip->sc_state & BUSY));
	switch (bp->b_datap->db_type) {
	case M_DELAY:
		ASSERT(ip->sc_delayid == 0);
		ip->sc_delayid = 
			itimeout(sc_delaytime, (caddr_t) ip,
				 /*LINTED*/
				 *(int *) bp->b_rptr, plstr);
		/* If itimeout() fails, forget the delay... */
		if (ip->sc_delayid != 0)
			ip->sc_state |= TXDELAY;
		freemsg(bp);
		return;
	case M_DATA:
		ip->sc_msg = bp;
		if (bp->b_rptr >= bp->b_wptr) {
			bp = ip->sc_msg->b_cont;
			ip->sc_msg->b_cont = NULL;
			freeb(ip->sc_msg);
			ip->sc_msg = bp;
			if (ip->sc_msg == NULL)
				return;
		}
		/* send block of data in msg to port */
		nch = bp->b_wptr - bp->b_rptr;
		(void)drv_setparm(SYSOUTC, nch);
		ip->sc_state |= BUSY;
		sc_start_write(ip, nch, bp->b_rptr);
		break;
#ifdef DEBUG
	default:
		/*
		 *+ The driver write routine sc_next_msg() expected 
		 *+ only messages of type M_DELAY or M_DATA, but 
		 *+ recieved and unexpected type instead.
		 */
		cmn_err(CE_WARN, 
			"sc%d: M_DELAY or M_DATA msg expected by sc_next_msg",
			ip->sc_unit);
		break;
#endif /* DEBUG */
	}
}

/*
 * STATIC void
 * sc_ssm_send(const struct sc_info *, int)
 * 	Notify the SSM that there is a serial port CB to execute.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock may be held upon entry,
 *	but is not required.
 *
 *	The CB specified by "func" must contain the message 
 *	to send, which is assumed owned by the caller.
 *
 *	No return value.
 *	
 * Description:
 *	Use "func" to determine which of the device's CBs is to be 
 *	executed and notify the SSM via a prearranged SLIC message.
 */
STATIC void
sc_ssm_send(const struct sc_info *ip, int func)
{
	pl_t s = splhi();
	slic_mIntr(ip->sc_ssm->ssm_slicaddr, CONS_BIN,
	      (uchar_t)COVEC(ip->sc_devno, func));
	splx(s);
}

/*
 * STATIC void
 * sc_start_read(const struct sc_info *)  
 * 	Initiate an SSM read request; don't wait for completion.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock may be held upon entry,
 *	but is not required.
 *
 *	The device's read CB is assumed to be idle,
 *	but owned by the caller.
 *
 *	No return value.
 *	
 * Description:
 *	Use the "read" CB of the specified device to solicit input
 *	from the SSM, using a driver local buffer of a fixed size.  
 *	Fill out the CB, including the binary configurable timeout
 *	value permited, then invoke sc_ssm_send() to notify the SSM. 
 */
STATIC void
sc_start_read(const struct sc_info *ip)  
{
	struct cons_rcb *rptr;
	
	rptr = ip->sc_rcb;
	rptr->rcb_cmd = CCB_RECV | CCB_IENABLE | CCB_TERM_MS;
	rptr->rcb_status = CCB_BUSY;
	rptr->rcb_count = SCRXSIZE;
	rptr->rcb_addr = (ulong_t) vtop((caddr_t)ip->sc_rx_buf, NULL);
	rptr->rcb_timeo = sc_global.rxtime;
	sc_ssm_send(ip, CCB_RECV_CB);
}

/*
 * STATIC void
 * sc_start_write(struct sc_info *, int, uchar_t *)
 * 	Initiate an SSM write request; don't wait for completion.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock may be held upon entry,
 *	but is not required.
 *
 *	The device's transmit CB is assumed to be idle,
 *	but owned by the caller.
 *
 *	No return value.
 *	
 * Description:
 *	Use the "write" CB of the specified device to initiate
 *	a data transfer to it.  "addr" is the virtual address of 
 *	the data buffer, containing "nch" data bytes.  Fill out 
 *	the CB and invoke sc_ssm_send() notify the SSM. 
 */
STATIC void
sc_start_write(struct sc_info *ip, int nch, uchar_t *addr)
{
	register struct cons_xcb *xptr;

	xptr = ip->sc_xcb;
	xptr->xcb_cmd = CCB_XMIT | CCB_IENABLE | CCB_TERM_MS;
	xptr->xcb_status = CCB_BUSY;
	xptr->xcb_addr = (unchar *) vtop((caddr_t)addr, NULL);
	xptr->xcb_count = ip->sc_tx_count = (ushort_t)nch;
	sc_ssm_send(ip, CCB_XMIT_CB);
}

/*
 * STATIC void
 * sc_set_modes(const struct sc_info *)
 * 	Set SSM serial port settings.      
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must be held upon entry.
 *
 *	The device's "message" CB is assumed to be idle.
 *	but owned by the caller as a result of holding
 *	the sc_lock.
 *
 *	The modes to be set are assumed to be saved in 
 *	the device's state information structure.
 *
 *	No return value.
 *	
 * Description:
 *	Use the "message" CB of the specified device to set the
 *	terminal characteristics applied by the SSM to that port.
 *	Copy the appropriate data from the port's information
 *	structure to the CB, invoke sc_ssm_send() to notify the SSM,
 *	then busy-wait until the SSM acknowledges completion of 
 *	the request by changing the CB's status field. 
 */
STATIC void
sc_set_modes(const struct sc_info *ip)
{
	volatile struct cons_mcb *mptr;
	
	mptr = (volatile struct cons_mcb *)ip->sc_mcb;
	mptr->mcb_cmd = CCB_STTY;
	mptr->mcb_status = CCB_BUSY;
	mptr->mcb_modem = ip->sc_modem;
	mptr->mcb_baud = ip->sc_baud;
	mptr->mcb_parsiz = ip->sc_parsiz; 
	mptr->mcb_flow = sc_global.flow;
	mptr->mcb_oflow = ip->sc_ld0flow;
	mptr->mcb_oxoff = ip->sc_ld0xoff;
	mptr->mcb_oxon  = ip->sc_ld0xon;
	sc_ssm_send(ip, CCB_MSG_CB);
	while (mptr->mcb_status == CCB_BUSY)
		;
}

/*
 * STATIC void
 * sc_get_modes(struct sc_info *)
 * 	Retrieve SSM serial port settings.      
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must be held upon entry.
 *
 *	The device's "message" CB is assumed to be idle.
 *	but owned by the caller as a result of holding
 *	the sc_lock.
 *
 *	No return value.
 *	
 * Description:
 *	Use the "message" CB of the specified device to retrieve 
 *	terminal characteristics currently being applied by the SSM 
 *	to that port.  Place the appropriate command into the CB,
 *	invoke sc_ssm_send() to notify the SSM, busy-wait until 
 *	the SSM acknowledges its completion, then copy the terminal
 *	characteristics from the updated CB to the port's information
 *	structure.
 */
STATIC void
sc_get_modes(struct sc_info *ip)
{
	volatile struct cons_mcb *mptr;

	mptr = (volatile struct cons_mcb *)ip->sc_mcb;
	mptr->mcb_cmd = CCB_GTTY;
	mptr->mcb_status = CCB_BUSY;
	sc_ssm_send(ip, CCB_MSG_CB);
	while (mptr->mcb_status == CCB_BUSY)
		;
	ip->sc_modem = mptr->mcb_modem;
	ip->sc_baud = mptr->mcb_baud;
	ip->sc_parsiz = mptr->mcb_parsiz;
}

/*
 * STATIC void
 * sc_abort_read(const struct sc_info *)
 * 	Abort a read CB being executed by the SSM.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock may be held upon entry,
 *	but is not required.
 *
 *	Assumes the specified device's read CB is being
 *	executed by the SSM, but owned by the caller 
 *	as a result of driver state and holding the sc_lock.
 *	
 *	No return value.
 *	
 * Description:
 *	Send a predefined slic message and id to notify the
 *	SSM to abort execution of the read CB for the 
 *	specified SSM serial port.  Then busy wait until
 *	it has been terminated, indicated by a change
 *	in the CB's status field.
 */
STATIC void
sc_abort_read(const struct sc_info *ip)
{
	volatile struct cons_rcb *rptr;
	pl_t s;

	rptr = (volatile struct cons_rcb *)ip->sc_rcb;
	rptr->rcb_status = CCB_BUSY;
	s = splhi();
	slic_mIntr(ip->sc_ssm->ssm_slicaddr, CONS_BIN,
	      CONS_FLUSH(ip->sc_devno, CCB_RECV_CB));
	splx(s);
	while (rptr->rcb_status == CCB_BUSY)
		;
}


/*
 * STATIC void
 * sc_suspend_out(const struct sc_info *)
 * 	Suspend data output for the specified SSM serial port.
 * 
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must be held upon entry.
 *	
 *	Assumes the specified device's write CB is being
 *	executed by the SSM, but owned by the caller 
 *	as a result of driver state and holding the sc_lock.
 *
 *	No return value.
 *	
 * Description:
 *	Send a predefined slic message and id to advise the
 *	SSM to stop writing data to the specified SSM serial 
 *	port.  Then busy wait until the request has been 
 *	acknowledged by a change in the CB's status field.
 */
STATIC void
sc_suspend_out(const struct sc_info *ip)
{
	volatile struct cons_xcb *xptr;
	pl_t s;

	xptr = (volatile struct cons_xcb *)ip->sc_xcb;
	xptr->xcb_status = CCB_BUSY;
	s = splhi();
	slic_mIntr(ip->sc_ssm->ssm_slicaddr, CONS_BIN,
	      CONS_STOP(ip->sc_devno));
	splx(s);
	while (xptr->xcb_status == CCB_BUSY)
		;
}


/*
 * STATIC void
 * sc_restart_out(const struct sc_info *)
 * 	Restart suspended data output for the specified SSM serial port.
 * 
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must be held upon entry.
 *	
 *	Assumes the SSM had previously been executing the 
 *	specified device's write CB, but was suspended.
 *
 *	No return value.
 *	
 * Description:
 *	Send a predefined slic message and id to advise the
 *	SSM to continue writing data to the specified SSM serial 
 *	port using its write CB.  Then busy wait until the 
 *	request has been acknowledged by a change in the CB's 
 *	status field.
 */
STATIC void
sc_restart_out(const struct sc_info *ip)
{
	volatile struct cons_xcb *xptr;
	pl_t s;

	xptr = (volatile struct cons_xcb *)ip->sc_xcb;
	xptr->xcb_status = CCB_BUSY;
	s = splhi();
	slic_mIntr(ip->sc_ssm->ssm_slicaddr, CONS_BIN,
	      (uchar_t) COVEC(ip->sc_devno, CCB_XMIT_CB));
	splx(s);
	while (xptr->xcb_status == CCB_BUSY)
		;
}

/* 
 * STATIC void
 * sc_abort_write(const struct sc_info *)
 * 	Abort a write CB being executed by the SSM.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must be held upon entry.
 *	
 *	Assumes the specified device's write CB is being
 *	executed by the SSM, but owned by the caller 
 *	as a result of driver state and holding the sc_lock.
 *
 *	No return value.
 *	
 * Description:
 *	Send a predefined slic message and id to notify the
 *	SSM to abort execution of the write CB for the 
 *	specified SSM serial port.  Then busy wait until
 *	it has been terminated, indicated by a change
 *	in the CB's status field.
 */
STATIC void
sc_abort_write(const struct sc_info *ip)
{
	volatile struct cons_xcb *xptr;
	pl_t	s;

	xptr = (volatile struct cons_xcb *)ip->sc_xcb;
	xptr->xcb_status = CCB_BUSY;
	s = splhi();
	slic_mIntr(ip->sc_ssm->ssm_slicaddr, CONS_BIN,
	      CONS_FLUSH(ip->sc_devno, CCB_XMIT_CB));
	splx(s);
	while (xptr->xcb_status == CCB_BUSY)
		;
}

/*
 * STATIC void
 * init_ssm_cons_dev(const struct sc_info *)
 * 	Establish communications with the SSM for using 
 *	one of its serial ports.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	Assumes execution occurs while running single
 *	threaded during system initialization.
 *
 *	The port's CBs must already have allocated and their
 *	location communicated to the SSM.
 *
 *	The device's "message" CB is assumed to be idle.
 *
 *	No return value.
 *	
 * Description:
 *	Generate an initialization message in the device's
 *	message CB to enable the use of interrupts for
 *	communicating CB completions.  Next send a predefined 
 *	slic message and id to request the SSM to accept and
 *	acknowldge the message.  Then busy wait until it has 
 *	been acknowledged by a change in the CB's status field.
 */
STATIC void
init_ssm_cons_dev(const struct sc_info *ip)
{
	volatile struct cons_icb *iptr;
	
	iptr = (struct cons_icb *) ip->sc_mcb;
	iptr->icb_cmd = CCB_INIT;
	iptr->icb_dest = SL_GROUP | PROC_GROUP;
	iptr->icb_basevec = sc_base_vector + ip->sc_unit * SC_VECS_PER_PORT;
	iptr->icb_scmd = SL_MINTR | sc_global.bin;
	iptr->icb_status = CCB_BUSY;

	slic_mIntr(ip->sc_ssm->ssm_slicaddr, CONS_BIN, 
		   (uchar_t)COVEC(ip->sc_devno, CCB_MSG_CB));
	while (iptr->icb_status == CCB_BUSY)
		;
}

/*
 * STATIC void
 * sc_draintime(queue_t *)
 *	Awakens close() operation waiting for output to drain.
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must not be held upon entry or exit.
 *
 *	Sets DRAINTIME in ip->sc_flags if output queue is not empty.
 *
 *	No return value.
 *
 * Description:
 * 	Called when a timer expires while waiting for output 
 *	to drain during a close() operation.  If the output
 *	has not completely drained, set the DRAINTIME flag
 *	to notify the blocked process when it is awakened.
 *	In either case, awaken the sleeping process.
 */
STATIC void
sc_draintime(queue_t *rd_q)
{
	pl_t s;
	struct sc_info *ip = (struct sc_info *) rd_q->q_ptr;

	ASSERT(ip != NULL);
	s = LOCK(ip->sc_lock, plstr);
	if (ip->sc_state & WCLOSE) {
		ip->sc_state &=~WCLOSE;
		if (qsize(WR(rd_q))) {
			ip->sc_flags |= DRAINTIME;
		}
		SV_SIGNAL(ip->sc_drain_wait, 0);
	}
	UNLOCK(ip->sc_lock, s);
}

/*
 * STATIC void
 * sc_delaytime(struct sc_info *)
 * 	Re-enables the queue service procedures once a scheduled
 *	delay has expired, resulting from an M_DELAY message. 
 *
 * Calling/Exit State:
 *	Does not block.
 *
 *	The port's sc_lock must not be held upon entry or exit.
 *
 *	Clears ip->sc_delayid and the TXDELAY device state flag, 
 *	indicating the delay is no longer active and transmissions
 *	may resume.
 *
 *	Re-enables the queue service procedures, allowing message
 *	servicing to be scheduled.
 *
 *	No return value.
 *
 */
STATIC void
sc_delaytime(struct sc_info *ip)
{
	pl_t s;

	s = LOCK(ip->sc_lock, plstr);
	ip->sc_state &= ~TXDELAY;
	ip->sc_delayid = 0;
	qenable(ip->sc_qptr);
	UNLOCK(ip->sc_lock, s);
}
