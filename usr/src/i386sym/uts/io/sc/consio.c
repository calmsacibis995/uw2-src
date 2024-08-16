/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/sc/consio.c	1.19"
#ident	"$Header: $"

/*
 * Routines dealing with console I/O.
 */

#include <util/types.h>
#include <util/param.h>
#include <util/debug.h>
#include <io/conssw.h>
#include <io/cfg.h>
#include <util/sysmacros.h>
#include <util/plocal.h>
#include <mem/vmparam.h>
#include <mem/vm_mdep.h>
#include <io/slic.h>
#include <io/slicreg.h>
#include <io/ssm/ssm_misc.h>
#include <io/ssm/ssm_cb.h>
#include <io/ssm/ssm.h>
#include <io/sc/sc.h>
#include <svc/systm.h>
#include <util/cmn_err.h>

#include <io/ddi.h>	/* Must come last */

#define CONS_BUFF_SIZE 32		/* size of console buffer */

/* private functions */
STATIC	dev_t	sc_consopen(minor_t, boolean_t);
STATIC	void	sc_consclose(minor_t, boolean_t);
STATIC	int	scputc(minor_t, int);
STATIC	int	scgetc(minor_t);
STATIC	int	sec_getc(void);
STATIC	int	ssm_getc(void);
STATIC 	void 	ssm_cons_suspend(minor_t);
STATIC 	void 	ssm_cons_resume(minor_t);

/* global variables */
struct conssw scconssw = { 
	sc_consopen, 
	sc_consclose, 
	scputc, 
	scgetc, 
	ssm_cons_suspend, 
	ssm_cons_resume
};
int     console_slic_id = -1;           /* SLIC id of console (SCED/SSM) */
struct cons_cb *cb;

/*
 * STATIC dev_t
 * sc_consopen(minor_t minor, boolean_t syscon)
 *
 *	Perform any console initialization needed before 
 *	printf() can be called.  This includes determining 
 *	the console monitor SLIC id for scputc().
 *
 * Calling/Exit State:
 *
 *	Returns "sc" dev_t.  Assumed to be called very early in the
 *	system initialization process.  Sets the global
 *	"console_slic_id" to the SLIC id of the console monitor
 *	controller board.  
 */

/*ARGSUSED*/
STATIC dev_t
sc_consopen(minor_t minor, boolean_t syscon)
{
	if (minor != 0)
		return NODEV;
	if (cb == NULL) {
		console_slic_id = KVCD_LOC->c_cons->cd_slic;
		cb = init_ssm_cons(console_slic_id);
	}
	return makedevice(sc_global.c_major, 0);
}

/*
 * STATIC void
 * sc_consclose(minor_t minor, boolean_t syscon)
 *
 * Calling/Exit State:
 *
 *	No-op.
 */

/*ARGSUSED*/
STATIC void
sc_consclose(minor_t minor, boolean_t syscon)
{
}

/*
 * STATIC int
 * scputc(minor_t minor, int c)
 *
 *	Put a char on the console device.
 *	Returns 0 if device is busy.
 *
 * Calling/Exit State:
 *
 *	If the system is not yet up, no locking is done and the
 *	char is simply passed to the SSM/SCED via the slic BIN 1
 *	interrupt.  This assumes a higher-level lock is held across
 *	cmn_err calls.  There is no return value.
 */

/*ARGSUSED*/
STATIC int
scputc(minor_t minor, int c)
{
	/*
	 * Output character.
	 */
	slic_mIntr(console_slic_id, PUTCHAR_BIN, c);

	return 1;
}

/*
 * STATIC int
 * scgetc(minor_t minor)
 *
 *	Call function to return a char from the console.
 *
 * Calling/Exit State:
 *
 *	Returns the return value of ssm/sec_getc().
 */

/*ARGSUSED*/
STATIC int
scgetc(minor_t minor)
{
	if (console_board_type == SLB_SCSIBOARD) {
		return sec_getc();
	}
	else {
		return ssm_getc();
	}
}

/*
 * STATIC int
 * sec_getc(void)
 *
 *	Return a char from the SEC console by reading the SLIC.
 *
 * Calling/Exit State:
 *
 *	Returns the char gotten from the SLIC.
 */

STATIC int
sec_getc(void)
{
	struct cpuslic *sl = (struct cpuslic *)KVSLIC;
	int	val;

	if ((sl->sl_ictl & SL_HARDINT) == 0)
		return -1;

	val = sl->sl_binint;
	sl->sl_binint = 0;		/* ack the interrupt */
	return val;
}

STATIC int ssm_rcb_sent = 0;		/* Is an RCB from ssm_getc() active? */
STATIC int ssm_rcb_rcount = 0;		/* #bytes last read by ssm_getc' RCB */

/*
 * STATIC int
 * ssm_getc(void)
 *
 * Calling/Exit State:
 *
 *	ssm_rcb_rcount reflects the number of valid input data bytes
 *	available from the most recent read CB completion.  buf_index 
 *	is the index of the next byte of that input to use.
 *	
 *	ssm_rcb_sent is 1 if a read CB has previously been started
 *	by this function, but not yet been completed or has not been
 *	cancelled by ssm_cons_resume().  It is zero otherwise.
 *
 *	Return a char from the SSM.  If none available now, return -1.
 *
 * Description:
 *
 *	If input data is not already buffered and ready to be
 *	returned by this function, then issue a command to the 
 *	SSM to return chars input on the console port.  Buffer 
 *	them and request more when these are gone.  Upon its
 *	completion, note the number of bytes it read into the
 *	buffer.
 *
 *	If data is not immediately available from the buffer,
 *	then return -1, otherwise return the next unused byte
 * 	from it.
 *
 * Remarks:
 *	Always use the CB's for the local port on SSM.
 *
 *	To coordinate with the sc-driver, the sc-driver's
 *	use of the read CB must be suspended upon entry to
 *	this input mode and restored afterwards.  The is
 *	handled by ssm_cons_suspend() and ssm_cons_resume().
 *	Note that ssm_cons_resume() is responsible for flushing 
 *	the read CB started by this function if it has not
 *	completed when this input mode is exited.
 *
 *	A different approach to this problem is to always issue
 *	complete or timeout read CB's started by this function.
 *	An implementation of that approach saturated the SSM with
 *	read CB's and their subsequent cancellations since the
 *	timeout period had to be short in order to maintain good
 *	output performance.
 */
STATIC int
ssm_getc(void)
{
	static char input_buff[CONS_BUFF_SIZE];
	static int buff_index = 0;
	volatile struct cons_rcb *rcb = (struct cons_rcb *) 
				(CONS_BASE_CB(cb, CCB_LOCAL) + CCB_RECV_CB);

	for ( ; buff_index >= ssm_rcb_rcount; ) {
		/* Solicit input */
		if (ssm_rcb_sent == 0) {
			/* 
			 * Generate and send the console a request 
			 * to read data.  It's termination will be
			 * noted either here or in ssm_cons_resume().
			 */
			rcb = (struct cons_rcb *) 
				(CONS_BASE_CB(cb, CCB_LOCAL) + CCB_RECV_CB);
			rcb->rcb_addr = kvtophys((vaddr_t) input_buff);
			rcb->rcb_timeo = 30;	/* allow 30msec between chars */
			rcb->rcb_count = CONS_BUFF_SIZE;
			rcb->rcb_cmd = CCB_RECV;
			rcb->rcb_status = CCB_BUSY;	/* Transfer status */
			slic_mIntr(console_slic_id, CONS_BIN, 
				   COVEC(CCB_LOCAL, CCB_RECV_CB));
			buff_index = ssm_rcb_rcount = 0;
			ssm_rcb_sent = 1;
		}

		if (rcb->rcb_status == CCB_BUSY) {
			/*
			 * Console is busy fetching data, none yet,
			 * thus, we'll need to try again later.
			 */
			 return(-1);
		}

		/*
 		 * Note that the CB has now terminated.
		 * If any data was read then pass along 
		 * the first byte and save the rest for 
		 * future requests. 
		 */
		ssm_rcb_sent = 0;
		ssm_rcb_rcount = CONS_BUFF_SIZE - rcb->rcb_count;
	}
	/*
 	 * Once here, there is input available.  
	 * Return the next byte to the caller.
	 */
	return(input_buff[buff_index++]);
}

STATIC struct cons_rcb saved_cons_rcb;
/*
 * STATIC void
 * ssm_cons_suspend(minor_t minor)
 *	Suspend normal driver console activity while in the debugger.
 *
 * Calling/Exit State:
 *	Only called upon entry to the debugger.
 *
 *	Terminates any outstanding read requests on the console.
 *
 *	Saves a copy of the console's most recent read CB in
 * 	the static structure "saved_cons_rcb".
 *
 * Description:
 *	In order to coordinate the use of console read CB with
 *	its use by the debugger input hooks, it is necessary to 
 *	cancel any currently outstanding rcb request on the console.
 *	Once cancelled, this function must also save a copy of
 *	that read CB, to be restored by ssm_cons_reenable which
 *	must be called when the debugger is exiting.
 *
 * Remarks:
 *	Always use the CB's for the local port on SSM.
 */
/*ARGSUSED*/
STATIC void 
ssm_cons_suspend(minor_t minor)
{
	volatile struct cons_rcb *rcb = (struct cons_rcb *) 
			(CONS_BASE_CB(cb, CCB_LOCAL) + CCB_RECV_CB);

	if (rcb->rcb_status == CCB_BUSY) {
		/* The request must be cancelled */
		rcb->rcb_status = CCB_BUSY;
		slic_mIntr(console_slic_id, CONS_BIN,
			CONS_FLUSH(CCB_LOCAL, CCB_RECV_CB));
		while (rcb->rcb_status == CCB_BUSY)
			continue;
	}
	bcopy((const void *)rcb, &saved_cons_rcb, sizeof(struct cons_rcb));
}

/*
 * STATIC void
 * ssm_cons_resume(minor_t minor)
 *	Restores normal driver console activity upon exiting the debugger.
 *
 * Calling/Exit State:
 *	Only called upon exit of the debugger.
 *
 *	Flushes the console's read CB if it was still
 *	actively attempting to solicit input, so that
 *	the normal console driver may regain control.
 *
 *	Restores a saved copy of the console's read CB from
 * 	the static structure "saved_cons_rcb".
 *
 * Description:
 *	In order to coordinate the use of console read CB with
 *	its use by the debugger input hooks, it was necessary to 
 *	cancel any currently outstanding rcb request on the console
 *	at the time the debugger is entered.  At that time 
 *	ssm_cons_suspend saved a copy of that read CB, which this 
 *	function restores.  Prior to restoring it, ensure that
 *	the debuggers use of the read CB is also terminated.
 *
 * Remarks:
 *	Always use the CB's for the local port on SSM.
 */
/*ARGSUSED*/
STATIC void 
ssm_cons_resume(minor_t minor)
{
	volatile struct cons_rcb *rcb = (struct cons_rcb *) 
			(CONS_BASE_CB(cb, CCB_LOCAL) + CCB_RECV_CB);

	if (rcb->rcb_status == CCB_BUSY) {
		/* This request must be cancelled */
		rcb->rcb_status = CCB_BUSY;
		slic_mIntr(console_slic_id, CONS_BIN,
			CONS_FLUSH(CCB_LOCAL, CCB_RECV_CB));
		while (rcb->rcb_status == CCB_BUSY)
			continue;
	}
	ssm_rcb_sent = 0;
	ssm_rcb_rcount = 0;

	bcopy(&saved_cons_rcb, (void *)rcb, sizeof(struct cons_rcb));
}
