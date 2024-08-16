/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/acct.c	1.5"

#include <util/types.h>
#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <util/param.h>
#include <svc/systm.h>
#include <proc/acct.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/resource.h>
#include <proc/exec.h>
#include <svc/errno.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <util/debug.h>
#include <io/uio.h>
#include <util/ksynch.h>
#include <svc/clock.h>
#include <mem/kmem.h>

STATIC vnode_t	*acctvp;
STATIC sleep_t	aclock;
STATIC LKINFO_DECL(aclock_info, "PP::aclock", 0);

/*
 * Macros to lock/unlock the lock which protects acctvp.
 */
#define ACLOCK()	SLEEP_LOCK(&aclock, PRIMED)

#define ACUNLOCK() { \
        ASSERT(SLEEP_LOCKOWNED(&aclock)); \
	SLEEP_UNLOCK(&aclock); \
}

/*
 * void acctinit(void)
 *	Initialize accounting subsystem.
 *
 * Calling/Exit State:
 *	One time initialization routine which is called early
 *	during system initialization.  Needed to init the
 *	sleep lock which protects the state of process accounting.
 */
void
acctinit(void)
{
	SLEEP_INIT(&aclock, (uchar_t)0, &aclock_info, KM_SLEEP);
}


/* Perform process accounting functions. */
struct accta {
	char	*fname;
};

/*
 * int sysacct(struct accta *uap, rval_t *rvp)
 *	Acct(2) system call.  Enable or disable process accounting.
 *
 * Calling/Exit State:
 *	No locks can be held on entry, none are held on return.
 */
/* ARGSUSED */
int
sysacct(struct accta *uap, rval_t *rvp)
{
	vnode_t *vp;
	int error = 0;
	cred_t *crp = CRED();

	ASSERT(KS_HOLD0LOCKS());

	if (pm_denied(crp, P_SYSOPS))
		return EPERM;

	ACLOCK();
	if (uap->fname == NULL) {		/* disable accounting */
		if (acctvp != NULL) {
			if (error = VOP_CLOSE(acctvp, FWRITE, 1, 0, crp))
				goto out;
			VN_RELE(acctvp);
			acctvp = NULL;
		}
	} else {				/* enable accounting */
		ADT_ACCT(acctvp);
		if (error = vn_open(uap->fname, UIO_USERSPACE, FWRITE,
		  0, &vp, (enum create)0)) {
			if (error == EISDIR)		/* SVID  compliance */
				error = EACCES;
			goto out;
		}
		if (acctvp != NULL && VN_CMP(acctvp, vp)) {
			error = EBUSY;
			goto closevp;
		}
		if (vp->v_type != VREG) {
			error = EACCES;
			goto closevp;
		}
		if (acctvp != NULL) {
			if (error = VOP_CLOSE(acctvp, FWRITE, B_TRUE,
						(off_t)0, crp)) {
				goto closevp;
			}
			VN_RELE(acctvp);
		}
		acctvp = vp;
	}
	goto out;

closevp:
	(void)VOP_CLOSE(vp, FWRITE, B_TRUE, (off_t)0, crp);
	VN_RELE(vp);
out:
	ACUNLOCK();
	return error;
}

/*
 * STATIC int compress(long t)
 *	Produce a pseudo-floating point representation
 *	with 3 bits base-8 exponent, 13 bits fraction.
 *
 * Calling/Exit State:
 *	Nothing special here.
 */
STATIC int
compress(u_long t)
{
	register int exp = 0, round = 0;

	while (t >= 8192) {
		exp++;
		round = t&04;
		t >>= 3;
	}
	if (round) {
		t++;
		if (t >= 8192) {
			t >>= 3;
			exp++;
		}
	}
	return (exp << 13) + t;
}

/*
 * void acct(char st)
 *	On process exit, write a record to the accounting file.
 *
 * Calling/Exit State:
 *	No spin locks can be held by the caller, this function
 *	may sleep.  The process is single threaded when this
 *	function is called.
 *
 * Remarks:
 *	The fields p_utime and p_stime are updated by the clock
 *	interrupt handler whenever an LWP in the process is interrupted
 *	by the clock --- so it is not necessary to sum the LWPs l_utime,
 *	l_stime into this field.
 *	The fields p_ioch, p_ior, and p_iow are not updated when an LWP
 *	does I/O; the corresponding LWP fields (l_ioch, l_ior, l_iow) are
 *	summed into the process level fields in lwp_exit() after the call
 *	to exit().  Since we are the lone survivor, and called from
 *	exit(), these fields in the calling LWP are summed to the process
 *	level here.
 */
void
acct(char st)
{
	vnode_t	*vp;
	proc_t	*p;
	cred_t	*crp;
	timestruc_t hrt;
	clock_t	ticks;
	struct	acct	acctbuf, *bp;

	ASSERT(u.u_procp->p_nlwp == 1);		/* single threaded */
	ASSERT(KS_HOLD0LOCKS());		/* no spin locks held */

	/*
	 * Quickly check whether accounting is on without acquiring
	 * any locks.  This function is called on every exit, this
	 * is done to avoid lock acquisition overhead when accounting
	 * is off.
	 */
	if (acctvp == NULL)
		return;

	ACLOCK();
	if ((vp = acctvp) == NULL){
		ACUNLOCK();
		return;
	}

	p = u.u_procp;
	crp = CRED();
	bp = &acctbuf;

	bcopy(p->p_execinfo->ei_comm, bp->ac_comm, sizeof(bp->ac_comm));
	bp->ac_btime = p->p_start.tv_sec;

	bp->ac_utime = compress(p->p_utime.dl_lop);
	bp->ac_stime = compress(p->p_stime.dl_lop);

	/* Subtract p_start from hrestime and convert to ticks. */
	GET_HRESTIME(&hrt);
	hrt.tv_nsec -= p->p_start.tv_nsec;
	hrt.tv_sec -= p->p_start.tv_sec;
	ticks = (hrt.tv_sec * HZ) + (hrt.tv_nsec / (NANOSEC/HZ));
	bp->ac_etime = compress(ticks);

	bp->ac_mem = compress(p->p_mem);

	/*
	 * Deposit I/O counts.  Note that CPU usage
	 * (p_utime, p_stime) is updated by the clock
	 * handler and we do not need to add in the
	 * values from the LWP level.
	 */
	p->p_ior = ladd(p->p_ior, u.u_ior);
	p->p_iow = ladd(p->p_iow, u.u_iow); 
	p->p_ioch = ladd(p->p_ioch, u.u_ioch);

	bp->ac_io = compress(p->p_ioch.dl_lop);
	bp->ac_rw = compress(p->p_ior.dl_lop + p->p_iow.dl_lop);

	bp->ac_uid = crp->cr_ruid;
	bp->ac_gid = crp->cr_rgid;
	bp->ac_tty = p->p_cttydev;
	bp->ac_stat = st;

	p->p_acflag |= u.u_acflag;
	bp->ac_flag = p->p_acflag | AEXPND;

	(void)vn_rdwr(UIO_WRITE, vp, (caddr_t)bp, sizeof(struct acct), 0,
		UIO_SYSSPACE, IO_APPEND, RLIM_INFINITY, crp, (int *)NULL);
	ACUNLOCK();
}

/*
 * int accton(void)
 * 	Determine if process accounting is on.
 *
 * Calling/Exit State:
 *	No special requirements.
 *
 * Remarks:
 *	As a result of making acctvp STATIC, other modules (e.g. AUDIT)
 *	need a functional interface to determine when process accounting
 *	is enabled.
 *	NOTE: The information returned from this function is stale.
 *	That is, by the time this function returns, the state of
 *	process	accounting may have changed.
 */
int
accton(void)
{
 	return(acctvp != NULL);
}
