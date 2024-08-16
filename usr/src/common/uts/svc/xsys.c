/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1987, 1988 Microsoft Corporation	*/
/*	  All Rights Reserved	*/

/*	This Module contains Proprietary Information of Microsoft  */
/*	Corporation and should be treated as Confidential.	   */

#ident	"@(#)kern:svc/xsys.c	1.4"

/* XENIX source support */

#include <util/types.h>
#include <proc/proc.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <proc/pid.h>
#include <proc/lwp.h>
#include <svc/time.h>
#include <svc/timeb.h>
#include <svc/proctl.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <util/inline.h>
#include <util/debug.h>

extern int Dstflag;	/* daylight savings time flag, defined in kernel.cf */
extern int Timezone;	/* local time zone, defined in kernel.cf */

/*
 * STATIC void
 * napwkup(void *arg)
 *	Execute a wakeup on the event structure pointed to by arg.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 */
STATIC void
napwkup(void *arg)
{
	EVENT_SIGNAL((event_t *)arg, 0);
}

struct napa {
	time_t msec;
};

/*
 * int
 * nap(struct napa *uap, rval_t *rvp)
 *	Suspends execution for a short interval.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Nap for the specified number of milliseconds. On success, a
 *	long integer indicating the number of milliseconds actually
 *	slept is returned. If the LWP received a signal while napping,
 *	the return value will be -1 and errno is set to EINTR.
 */
int
nap(struct napa *uap, rval_t *rvp)
{
	lwp_t	*lwpp;
	clock_t	fst;
	long	togo;
	toid_t	tmout_id;

	ASSERT(KS_HOLD0LOCKS());
	lwpp = u.u_lwpp;

	fst = lbolt;
	/* preclude overflow */
	if (uap->msec >= LONG_MAX / HZ || uap->msec < 0) {
		return (EINVAL);
	}

	/* togo gets time to nap in ticks */
	togo = (uap->msec*HZ + 999) / 1000;
	
	if (togo > 0) {
		EVENT_CLEAR(&lwpp->l_slpevent);
		tmout_id = itimeout(napwkup, &lwpp->l_slpevent, togo, PLBASE);
		/* nap, return time napped */
		if (!EVENT_WAIT_SIG(&lwpp->l_slpevent, PRISLEP)) {
			(void)untimeout(tmout_id);
			rvp->r_time = -1;
			return (EINTR);
		}
	}
	rvp->r_time = (lbolt - fst) * 1000L / HZ;
	return (0);
}

struct ftimea {
	struct timeb *tp;
};

/*
 * int
 * ftime(struct ftimea *uap, rval_t *rvp)
 *	Return TOD with milliseconds, timezone, DST flag
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
/* ARGSUSED */
int
ftime(struct ftimea *uap, rval_t *rvp)
{
	struct timeb	t;
	timestruc_t	ltime;

	ASSERT(KS_HOLD0LOCKS());
	/* 
	 * Millisecond resolution used to be obtained by combining
	 * the low resolution time with a syncronized lbolt. Since
	 * the hrestime now includes nanoseconds, this is no longer
	 * needed.
	 */
	GET_HRESTIME(&ltime);
	t.time = ltime.tv_sec;
	t.millitm = ltime.tv_nsec / (NANOSEC / MILLISEC);
	t.timezone = (short) Timezone;
	t.dstflag = (short) Dstflag;
	if (copyout(&t, uap->tp, sizeof(t)) == -1) {
		return (EFAULT);
	}
	return (0);
}

struct proctla {
	pid_t	pid;
	int	cmd;
	char	*arg;
};

/*
 * int
 * proctl(struct proctla *uap, rval_t *rvp)
 *	XENIX process control system call.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 */
/* ARGSUSED */
int
proctl(struct proctla *uap, rval_t *rvp)
{
	proc_t	*p;
	pid_t	pid;
	int	found;
	int	error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	error = found = 0;
	pid = uap->pid;

	(void)RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (p = practive; p != NULL; UNLOCK(&p->p_mutex, PL_PROCLIST),
				      p = p->p_next) {
		(void)LOCK(&p->p_mutex, PLHI);
		if (pid > 0) {
			if (p->p_pidp->pid_id != pid) {
				continue;
			}
		} else if (p == proc_init) {
			continue;
		}
		if (pid == 0 && p->p_pgidp != u.u_procp->p_pgidp) {
			continue;
		}
		if (pid < -1 && p->p_pgid != -pid) {
			continue;
		}
		if (!hasprocperm(p->p_cred, u.u_lwpp->l_cred)) {
			if (pid > 0) {
				error = EPERM;
				break;
			} else {
				continue;
			}
		}
		found++;
		switch (uap->cmd) {
			case PRHUGEX:
			case PRNORMEX:
				/*
				 * These values are to preserve the
				 * functionality of this system call.
				 * They are implemented as no-ops to
				 * maintain backwards compatibility.
				 */
				break;
			default:
				error = EINVAL;
				break;
		}
		if (pid > 0 || error) {
			break;
		}
	}
	UNLOCK(&p->p_mutex, PL_PROCLIST);
	RW_UNLOCK(&proc_list_mutex, PLBASE);
	if (found == 0) {
		error = ESRCH;
	}
	return (error);
}

/* End XENIX Support */
