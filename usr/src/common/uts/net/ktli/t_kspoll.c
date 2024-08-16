/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kspoll.c	1.15"
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
 *	t_kspoll.c, kernel TLI function to poll on a transport
 *	endpoint.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <proc/proc.h>
#include <fs/file.h>
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
#include <util/debug.h>

extern	toid_t	itimeout(void (*)(), void *, long, pl_t);
extern	void	untimeout(toid_t);

void		ktli_poll();

/*
 * t_kspoll(tiptr, timo, sigflg, events)
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *	On success, ptr is set the structure required.
 *
 * Description:
 * 	This function does a poll for a transport endpoint.
 *	It waits for timo clock ticks for something to arrive
 *	on the specified endpoint. If more than one client is
 *	hanging off of a single endpoint, and at least one has
 *	specified a non-zero timeout, then all will be woken.
 *
 *	Returns 0 on success or positive error code. On success,
 *	"events" is set to
 *
 *		 0	on timeout or no events (timeout = 0),
 *
 *		 1	if desired event has occurred
 *
 *	Most of the code is from strwaitq().
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	timo			# timeout for wait
 *	sigflg			# to sleep interruptibly or not
 *				# (nfs needs this for retries)
 *	events			# return event happened indication
 *	
 */
int 
t_kspoll(TIUSER *tiptr, int timo, int sigflg, int *events)
{
	struct file	*fp;
	struct vnode	*vp;
	struct stdata	*stp;
	toid_t		timeid;
	pl_t		opl;
	int		retval;

	fp = tiptr->tp_fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	ASSERT((sigflg == POLL_SIG_IGNORE) || (sigflg == POLL_SIG_CATCH));
	ASSERT(events != NULL);

	/*
	 * call runqueues to put stuff through streams
	 * better for nfs servers.
	 */
	spl1();
	runqueues();
	spl0();

again:
	opl = LOCK(stp->sd_mutex, PLKTLI);

	/*
	 * check state of stream
	 */
	if (stp->sd_flag & (STRDERR|STPLEX)) {
		retval = (stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror;
		UNLOCK(stp->sd_mutex, opl);

		KTLILOG(0x20000, "t_kspoll: stream error on entry\n", 0);

		return (retval);
	}

	if ((RD(stp->sd_wrq))->q_first != NULL) {
		/*
		 * some data already there 
		 */
		UNLOCK(stp->sd_mutex, opl);
		*events = 1;

		KTLILOG(0x2000, "t_kspoll: data was already present\n", 0);

		return (0);
	}

	/*
	 * drop sd_mutex before calling itimeout
	 */
	UNLOCK(stp->sd_mutex, opl);

	if (timo == 0) {
		*events = 0;

		KTLILOG(0x2000, "t_kspoll: timeout 0\n", 0);

		return (0);
	}

	/*
	 * set timer
	 */
	if (timo > 0) {

		KTLILOG(0x2000, "t_kspoll: calling itimeout, timo %x\n", timo);

		timeid = itimeout_a(ktli_poll, (caddr_t)tiptr, (long)timo,
				    PLKTLI, itimeout_allocate(KM_SLEEP));
	}

	/*
	 * check state of stream again
	 */
	opl = LOCK(stp->sd_mutex, PLKTLI);
	if (stp->sd_flag & (STRDERR|STPLEX)) {
		retval = (stp->sd_flag & STPLEX) ? EINVAL : stp->sd_rerror;
		UNLOCK(stp->sd_mutex, opl);
		if ((timo > 0) && timeid)
			untimeout(timeid);

		KTLILOG(0x20000, "t_kspoll: stream error after itimeout\n", 0);

		return (retval);
	}

	if ((RD(stp->sd_wrq))->q_first != NULL) {
		/*
		 * some data got there while we were in itimeout()
		 */
		UNLOCK(stp->sd_mutex, opl);
		if ((timo > 0) && timeid)
			untimeout(timeid);
		*events = 1;

		KTLILOG(0x2000, "t_kspoll: data present before wait\n", 0);

		return (0);
	}

	stp->sd_flag |= RSLEEP;

	switch(sigflg) {

	case POLL_SIG_IGNORE:

		KTLILOG(0x2000, "t_kspoll: proc %x sleep uninterruptibly\n",
			u.u_procp->p_pidp->pid_id);
		KTLILOG(0x2000, "t_kspoll: lwp %x sleep uninterruptibly\n",
			u.u_lwpp->l_lwpid);

		
		/*
		 * ignore all signals.
		 */
		SV_WAIT(stp->sd_read, PRITLI, stp->sd_mutex);

		break;

	case POLL_SIG_CATCH:

		KTLILOG(0x2000, "t_kspoll: proc %x sleep interruptibly\n",
			u.u_procp->p_pidp->pid_id);
		KTLILOG(0x2000, "t_kspoll: lwp %x sleep interruptibly\n",
			u.u_lwpp->l_lwpid);

		/*
		 * sleep interrutibly.
		 */
		if (SV_WAIT_SIG(stp->sd_read, PRITLI, stp->sd_mutex)
							== B_FALSE) {
			if (timo > 0)
				untimeout(timeid);

			/*
			 * only unset RSLEEP if no other lwps
			 * are sleeping on this stream. this count
			 * should never be less than one.
			 *
			 * have to acquire sd_mutex first as it is
			 * acquired at a lower ipl than fp->f_mutex.
			 *
			 */
			opl = LOCK(stp->sd_mutex, PLKTLI);
			FTE_LOCK(fp);
			if (fp->f_count <= 1)
				stp->sd_flag &= ~RSLEEP;
			FTE_UNLOCK(fp, PLKTLI);
			UNLOCK(stp->sd_mutex, opl);
	
			KTLILOG(0x20000, "t_kspoll: proc %x interrupted\n",
					u.u_procp->p_pidp->pid_id);
			KTLILOG(0x20000, "t_kspoll: lwp %x interrupted\n",
					u.u_lwpp->l_lwpid);
	
			return (EINTR);
		}

		break;

	default:

		/*
		 *+ Wrong sigflg value passed. this points
		 *+ to an inconsistent programming model.
		 */
		cmn_err(CE_PANIC, "sigflg value (%d) wrong in t_kspoll\n",
					sigflg);
	}

	KTLILOG(0x2000, "t_kspoll: proc %x after switch\n",
			u.u_procp->p_pidp->pid_id);
	KTLILOG(0x2000, "t_kspoll: lwp %x after switch\n",
			u.u_lwpp->l_lwpid);

	if (timo > 0)
		untimeout(timeid);

	/*
	 * only unset RSLEEP if no other lwps
	 * are sleeping on this stream. this count
	 * should never be less than one.
	 *
	 * have to acquire sd_mutex first as it is
	 * acquired at a lower ipl than fp->f_mutex.
	 *
	 */
	opl = LOCK(stp->sd_mutex, PLKTLI);
	FTE_LOCK(fp);
	if (fp->f_count <= 1)
		stp->sd_flag &= ~RSLEEP;
	FTE_UNLOCK(fp, PLKTLI);
	UNLOCK(stp->sd_mutex, opl);

	/*
	 * see if the timer expired. do not need lock
	 * protection for flags because ktli_poll()
	 * is definitely not on the timeout callout list
	 * at this point.
	 */
	if (tiptr->tp_flags & TIME_UP) {
		tiptr->tp_flags &= ~TIME_UP;
		*events = 0;
	
		KTLILOG(0x2000, "t_kspoll: timer expired\n", 0);

		return (0);
	}

	goto again;
}


/*
 * ktli_poll(tiptr)
 *	Handler for timeout.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns a void.
 *
 * Description:
 *	This is the handler for timeout. Just marks the
 *	tiptr as timed out.
 *
 * Parameters
 *
 *	tiptr			# open tli pointer
 *
 */
void
ktli_poll(TIUSER *tiptr)
{
	struct vnode	*vp;
	struct file	*fp;
	struct stdata	*stp;
	pl_t		opl;

	fp = tiptr->tp_fp;
	vp = fp->f_vnode;
	stp = vp->v_stream;

	KTLILOG(0x2000, "ktli_poll: called\n", 0);

	/*
	 * set the time up flag.
	 *
	 * do not need protection for flags because any base
	 * kernel thread which cares about the value of these
	 * flags is either sleeping at this time or
	 * not checking their value.
	 */
	tiptr->tp_flags |= TIME_UP;
	opl = LOCK(stp->sd_mutex, PLKTLI);
	SV_BROADCAST(stp->sd_read, 0);
	UNLOCK(stp->sd_mutex, opl);
}
