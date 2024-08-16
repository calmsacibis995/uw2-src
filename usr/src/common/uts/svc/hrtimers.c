/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:svc/hrtimers.c	1.2"
#ident	"$Header: $"

#include <util/types.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <svc/time.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <mem/kmem.h>
#include <acc/priv/privilege.h>
#include <acc/audit/audit.h>
#include <svc/hrtcntl.h>


extern int getitimer_k(int, struct itimerval *);
extern int setitimer_k(int, struct itimerval *, int);



STATIC int
hrt_clk_convert(int clk)
{

        switch (clk) {
	case CLK_STD:
		return ITIMER_REAL;

	case CLK_USERVIRT:
		return ITIMER_VIRTUAL;

	case CLK_PROCVIRT:
		return ITIMER_PROF;
	}

	return -1;
}


STATIC void
timeval_to_hrtime(struct timeval *tvp, hrtime_t *hrtp)
{
        hrtp->hrt_secs = (ulong) (tvp->tv_sec);
        hrtp->hrt_rem = tvp->tv_usec;
        hrtp->hrt_res = MICROSEC;
}


/*
 * Convert a hrtime to a timeval.  Assumes that hrtime has proper
 * (MICROSEC) resolution.
 */

STATIC void
hrtime_to_timeval(struct timeval *tvp, hrtime_t *hrtp)
{
        tvp->tv_sec = (long) (hrtp->hrt_secs);
        tvp->tv_usec = hrtp->hrt_rem;
}


/*
 * Argument vectors for the various flavors of hrtsys().
 */

#define	HRTCNTL		0
#define	HRTALARM	1

struct hrtsysa {
	int	opcode;
};

struct  hrtcntla {
	int opcode;
	int cmd;
	int clk;
	int *dummy;		/* interval_t *intp; */
	hrtime_t *hrtp;
};

struct hrtalarma {
	int opcode;
	hrtcmd_t *cmdp;
	int cmds;
};


/* 
 * Hrtcntl (time control) system call.
 */


STATIC int
hrtcntl(struct hrtcntla *uap, rval_t *rvp)
{
	int error = 0;

	switch(uap->cmd) {
	case HRT_TOFD:		/* Get the time of day 		 */
	{
		struct timeval tv;
		timestruc_t atv;
		hrtime_t hrtv;

		if (uap->clk != CLK_STD) {
			error = EINVAL;
			break;
		}

		ASSERT((KS_HOLD0LOCKS()));
		GET_HRESTIME(&atv)

		tv.tv_sec = atv.tv_sec;
		tv.tv_usec = atv.tv_nsec / (NANOSEC/MICROSEC);

		timeval_to_hrtime(&tv, &hrtv);

		if (copyout((caddr_t)&hrtv,
			(caddr_t)uap->hrtp, sizeof(hrtime_t))) {
                                error = EFAULT;
		}
		break;
	}

	case HRT_GETRES:	/* Get the resolution of a clock */
	case HRT_STARTIT:	/* Start timing an activity      */	
	case HRT_GETIT:		/* Get value of interval timer	  */
		error = ENOSYS;
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}			


/*
 * Hrtalarm (start one or more alarms) system call.
 */

STATIC int
hrtalarm(struct hrtalarma *uap, rval_t *rvp)
{
	hrtcmd_t *cp;
	hrtcmd_t *hrcmdp;
	struct itimerval aitv;
	hrtime_t ht;
	int error = 0;
	uint alarm_cnt = 0;
	hrtcmd_t		timecmd;
	int cnt;
	int which;

	/*
	 * Return EINVAL for negative and zero counts.
	 */

	if (uap->cmds <= 0)
		return(EINVAL);

	cp = &timecmd;
	hrcmdp = uap->cmdp;

	/*	Loop through and process each command.
	*/

	for (cnt = 0; cnt < uap->cmds; cnt++, hrcmdp++) {

		if (copyin((caddr_t)hrcmdp, (caddr_t)cp, sizeof(hrtcmd_t))) {
			error = EFAULT;
			return error;
		}

		switch(cp->hrtc_cmd) {
		case HRT_BSD_PEND:
			which = hrt_clk_convert(cp->hrtc_clk);

			if (which < 0) {
				error = EINVAL;
				break;
			}

			if (error = getitimer_k(which, &aitv))
				break;

			timeval_to_hrtime(&aitv.it_value, &ht);

			if (copyout(&ht, &hrcmdp->hrtc_int, sizeof(aitv)))
				error = EFAULT;
			break;

		case HRT_BSD_CANCEL:
			which = hrt_clk_convert(cp->hrtc_clk);

			if (which < 0) {
				error = EINVAL;
				break;
			}

			bzero(&aitv, sizeof(aitv));
			error = setitimer_k(which, &aitv, NULL);
			break;

		case HRT_BSD_REP:
			which = hrt_clk_convert(cp->hrtc_clk);

			if (which < 0) {
				error = EINVAL;
				break;
			}

			hrtime_to_timeval(&aitv.it_interval, &cp->hrtc_int);
			hrtime_to_timeval(&aitv.it_value, &cp->hrtc_tod);

			error = setitimer_k(which, &aitv, NULL);
			break;

		case HRT_BSD:
			which = hrt_clk_convert(cp->hrtc_clk);

			if (which < 0) {
				error = EINVAL;
				break;
			}

			hrtime_to_timeval(&aitv.it_value, &cp->hrtc_int);
			bzero(&aitv.it_interval, sizeof(struct timeval));
			error = setitimer_k(which, &aitv, NULL);
			break;

		case HRT_RBSD:
		default :
			error = EINVAL;
			break;
		}

		if (error) {
			cp->hrtc_flags |= HRTF_ERROR;
			cp->hrtc_error = error;
		} else {
			cp->hrtc_flags |= HRTF_DONE;
			cp->hrtc_error = 0;
			alarm_cnt++;
		}

		if (copyout((caddr_t)&cp->hrtc_flags,
		    (caddr_t)&hrcmdp->hrtc_flags,
		    sizeof(cp->hrtc_flags) + sizeof(cp->hrtc_error))) {
			return EFAULT;
		}
	}
	rvp->r_val1 = alarm_cnt;
	return(0);
}


/*
 * System entry point for hrtcntl and hrtalarm system calls.
 */

int
hrtsys(struct hrtsysa *uap, rval_t *rvp)
{
	int error;

	switch (uap->opcode) {
	case HRTCNTL:
		error = hrtcntl((struct hrtcntla *)uap, rvp);
		break;

	case HRTALARM:
		error = hrtalarm((struct hrtalarma *)uap, rvp);
		break;

	default:
		error = EINVAL;
		break;
	}

	return error;
}

