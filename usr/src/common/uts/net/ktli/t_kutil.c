/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kutil.c	1.19"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */


/*
 *	t_kutil.c, utility functions for kernel TLI.
 *
 */

#include <util/param.h>
#include <util/ksynch.h>
#include <util/types.h>
#include <util/engine.h>
#include <util/plocal.h>
#include <proc/proc.h>
#include <proc/bind.h>
#include <fs/file.h>
#include <util/cmn_err.h>
#include <util/mod/moddefs.h>
#include <proc/user.h>
#include <fs/vnode.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/ioctl.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>
#include <net/netsubr.h>

extern	int	canput_l(queue_t *);

int		ktli_load(void);
int		ktli_unload(void);

MOD_MISC_WRAPPER(ktli, ktli_load, ktli_unload, "Kernel TLI");

/*
 * tli_send(tiptr, bp, fmode)
 *	Send a message block downstream.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *
 * Description:
 *	This routine sends a message block on the
 *	transport endpoint tiptr.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	bp			# message block to send
 *	fmode			# to block or not to block
 *	
 */
int
tli_send(TIUSER *tiptr, mblk_t *bp, int fmode)
{
	struct file	*fp;
	struct vnode	*vp;
	struct stdata	*stp;
	pl_t		opl;
	struct engine	*engp;
	int		unbind;
	int		error;
	int		retval;

	KTLILOG(0x1000, "tli_send: entered\n", 0);

	retval = 0;
	error = 0;
	unbind = 0;
	fp = tiptr->tp_fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	/*
	 * strwaitq() assumes sd_mutex locked so get
	 * it now and call canput_l() instead of locking
	 * and unlocking in the loop.
	 */
	opl = LOCK(stp->sd_mutex, PLKTLI);
	while ((stp->sd_wrq->q_flag & QFREEZE) ||
	       !canput_l(stp->sd_wrq->q_next)) {
		if ((error = strwaitq(stp, WRITEWAIT, (off_t)0, fmode, 
						&retval)) != 0 || retval) {
			UNLOCK(stp->sd_mutex, opl);

			KTLILOG(0x10000,
			  "tli_send: strwaitq returned error %d\n", error);

			return error;
		}
	}

	/*
	 * Put message downstream. sd_wrq is invariant and needs no lock.
	 * need to worry about uniproc support here.
	 */
	stp->sd_upbcnt++;
	UNLOCK(stp->sd_mutex, opl);
	if (stp->sd_cpu) {
		engp = kbind(stp->sd_cpu);
		unbind = 1;
	}
	putnext(stp->sd_wrq, bp);
	opl = LOCK(stp->sd_mutex, PLKTLI);
	if ((--stp->sd_upbcnt == 0) && (stp->sd_flag & UPBLOCK))
		SV_BROADCAST(stp->sd_upblock, 0);
	UNLOCK(stp->sd_mutex, opl);
	if (unbind)
		kunbind(engp);

	KTLILOG(0x1000, "tli_send: returning no error\n", 0);

	return 0;
}

/*
 * tli_recv(tiptr, bp, fmode)
 *	Receive a message block from a stream.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *
 * Description:
 *	This routine receives a message block from the
 *	transport endpoint tiptr.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	bp			# to get message block in
 *	fmode			# to block or not to block
 *	
 */
/*ARGSUSED*/
int 
tli_recv(TIUSER *tiptr, mblk_t **bp, int fmode)
{
	struct file	*fp;
	struct vnode	*vp;
	struct stdata	*stp;
	pl_t		opl;
	int		error;

	KTLILOG(0x1000, "tli_recv: entered\n", 0);

	error = 0;
	fp = tiptr->tp_fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	opl = LOCK(stp->sd_mutex, PLKTLI);
	if (stp->sd_flag & (STRDERR|STPLEX)) {
		error = (stp->sd_flag&STPLEX) ? EINVAL : stp->sd_rerror;
		UNLOCK(stp->sd_mutex, opl);

		KTLILOG(0x10000, "tli_recv: streams error %d\n", error);

		return error;
	}

	if ( !(*bp = getq_l(RD(stp->sd_wrq)))) {

		KTLILOG(0x10000, "tli_recv: nothing to recv\n", 0);

		error = EINTR;
	}

	if (stp->sd_flag)
		stp->sd_flag &= ~STRPRI;
	UNLOCK(stp->sd_mutex, opl);

	KTLILOG(0x10000, "tli_recv: retuning error %d\n", error);

	return error;
}

/*
 * get_ok_ack(tiptr, type, fmode)
 *	Receive an acknowledgement on a transport endpoint.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *
 * Description:
 *	This routine receives an acknowledgement on the
 *	transport endpoint tiptr.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	type			# type of ack to receive
 *	fmode			# to block or not to block
 *	
 */
int
get_ok_ack(TIUSER *tiptr, int type, int fmode)
{
	int			msgsz;
	union T_primitives	*pptr;
	mblk_t			*bp;
	int			error;

	error = 0;

	/*
	 * wait for ack
	 */
	if ((error = tli_recv(tiptr, &bp, fmode)) != 0)
		return error;

	if ((msgsz = (bp->b_wptr - bp->b_rptr)) < sizeof(long))
		return EPROTO;

	/* LINTED pointer alignment */
	pptr = (union T_primitives *)bp->b_rptr;
	switch(pptr->type) {

		case T_OK_ACK:
			if (msgsz < TOKACKSZ ||
					pptr->ok_ack.CORRECT_prim != type) 
				error = EPROTO;
			break;

		case T_ERROR_ACK:
			if (msgsz < TERRORACKSZ) {
				error = EPROTO;
				break;
			}

			if (pptr->error_ack.TLI_error == TSYSERR)
				error = pptr->error_ack.UNIX_error;
			else
				error = tlitosyserr(pptr->error_ack.TLI_error);

			break;

		default:
			error = EPROTO;
			break;
	}

	return error;
}

/*
 * ktli_load(void)
 *	Dynamically load kernel tli.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Dynamically load kernel tli.
 *
 * Parameters:
 *
 */
int
ktli_load(void)
{
	/*
	 * nothing to do.
	 */
        return(0);
}

/*
 * ktli_unload(void)
 *	Dynamically unload kernel tli.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 * Description:
 *	Dynamically unload kernel tli.
 *
 * Parameters:
 *
 */
int
ktli_unload(void)
{
	/*
	 * nothing to do.
	 */
        return(0);
}

#ifdef DEBUG

/*
 * switch for type of logging
 */
int	ktlilog;

/*
 * ktli_log()
 *	kernel debugging aid.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Return 0.
 *
 * Description:
 *	Kernel level debugging aid. The global variable
 *	"ktlilog" is a bit mask which allows various
 *	debugging messages to be printed out.
 * 
 *      ktlilog & 0x1
 *      ktlilog & 0x2
 *      ktlilog & 0x4
 *      ktlilog & 0x8		from t_kfree.c
 *      ktlilog & 0x10		from t_kalloc.c
 *      ktlilog & 0x20		from t_kbind.c
 *      ktlilog & 0x40		from t_kunbind.c
 *      ktlilog & 0x80		from t_kopen.c
 *      ktlilog & 0x100		from t_kclose.c
 *      ktlilog & 0x200		from t_krcvudat.c
 *      ktlilog & 0x400		from t_ksndudat.c
 *      ktlilog & 0x800		from t_kgtstate.c
 *      ktlilog & 0x1000	from t_kutil.c
 *      ktlilog & 0x2000	from t_kspoll.c
 *      ktlilog & 0x4000	from t_kconnect.c
 *      ktlilog & 0x8000	error info from t_ksndudat and t_krcvudat
 *      ktlilog & 0x10000	error from t_kutil.c
 *      ktlilog & 0x20000	error from t_kspoll.c, about signals
 *
 * Parameters:
 *
 *	None.
 *
 */
int
ktli_log(int level, char *str, int a1)
{
	if (level & ktlilog)
		cmn_err(CE_CONT, str, a1);

	return(0);
}

#endif /* DEBUG */
