/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/scalls.c	1.34"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <util/types.h>
#include <proc/proc_hier.h>
#include <proc/exec.h>
#include <proc/resource.h>
#include <proc/ulimit.h>
#include <proc/times.h>
#include <proc/signal.h>
#include <proc/cred.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/session.h>
#include <util/param.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <svc/clock.h>
#include <svc/sco.h>
#include <acc/priv/privilege.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/as.h>
#include <mem/page.h>
#include <mem/vmparam.h>
#include <acc/mac/mac.h>
#include <acc/audit/audit.h>

/*
 * This file contains entry points for several system calls found in
 * Section 2 of the "Programmer's Reference Manual". All system calls
 * in this file are related to process attributes.
 */


STATIC void nudgeus(void *);
extern lkinfo_t	proflkinfo;


struct setuida {
	uid_t	uid;
};

/*
 * int setuid(struct setuida *uap, rval_t *rvp)
 *	Setuid(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
/* ARGSUSED */
int
setuid(register struct setuida *uap, rval_t *rvp)
{
	register cred_t *newcredp, *credp = u.u_lwpp->l_cred;
	register uid_t uid;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if ((uid = uap->uid) > MAXUID || uid < (uid_t) 0) {
		error = EINVAL;
	} else if ((error = pm_denied(credp, P_SETUID)) == 0) {
		/*
		 * Do the work only if you need to.
		 */
		if ((uid != credp->cr_uid) || (uid != credp->cr_ruid) ||
		    (uid != credp->cr_suid)) {
			newcredp = crdup2(credp);
			newcredp->cr_uid = uid;
			newcredp->cr_ruid = uid;
			newcredp->cr_suid = uid;
			pm_recalc(newcredp);	/* MUST do before crinstall() */
			crinstall(newcredp);
		}
	} else if (uid == credp->cr_ruid || uid == credp->cr_suid) {
		/*
		 * Do the work only if you need to.
		 */
		if (uid != credp->cr_uid) {
			newcredp = crdup2(credp);
			newcredp->cr_uid = uid;
			pm_recalc(newcredp);	/* MUST do before crinstall() */
			crinstall(newcredp);
		}
		error = 0;
	}
	return error;
}


/*
 * int getuid(char *uap, rval_t *rvp)
 *	getuid(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
/* ARGSUSED */
int
getuid(char *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	rvp->r_val1 = (int) u.u_lwpp->l_cred->cr_ruid;
	rvp->r_val2 = (int) u.u_lwpp->l_cred->cr_uid;
	return 0;
}


struct setgida {
	gid_t	gid;
};

/*
 * int setgid(struct setgida *uap, rval_t *rvp)
 *	Setgid(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
/* ARGSUSED */
int
setgid(register struct setgida *uap, rval_t *rvp)
{
	register cred_t *newcredp, *credp = u.u_lwpp->l_cred;
	register gid_t gid;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if ((gid = uap->gid) > MAXUID || gid < (gid_t) 0) {
		error = EINVAL;
	} else if ((error = pm_denied(credp, P_SETUID)) == 0) {
		if ((gid != credp->cr_gid) || (gid != credp->cr_rgid) ||
		    (gid != credp->cr_sgid)) {
			newcredp = crdup2(credp);
			newcredp->cr_gid = gid;
			newcredp->cr_rgid = gid;
			newcredp->cr_sgid = gid;
			crinstall(newcredp);
		}
	} else if (gid == credp->cr_rgid || gid == credp->cr_sgid) {
		if (gid != credp->cr_gid) {
			newcredp = crdup2(credp);
			newcredp->cr_gid = gid;
			crinstall(newcredp);
		}
		error = 0;
	}
	return error;
}


/*
 * int getgid(char *uap, rval_t *rvp)
 *	Getgid(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
/* ARGSUSED */
int
getgid(char *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	rvp->r_val1 = u.u_lwpp->l_cred->cr_rgid;
	rvp->r_val2 = u.u_lwpp->l_cred->cr_gid;
	return 0;
}


/*
 * int getpid(char *uap, rval_t *rvp)
 *	Getpid(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
/* ARGSUSED */
int
getpid(char *uap, rval_t *rvp)
{
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	rvp->r_val1 = u.u_procp->p_pidp->pid_id;
	rvp->r_val2 = u.u_procp->p_ppid;
	return 0;
}


struct seteuida {
	int	uid;
};

/*
 * int seteuid(struct seteuida *uap, rval_t *rvp)
 *	Seteuid(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
/* ARGSUSED */
int
seteuid(register struct seteuida *uap, rval_t *rvp)
{
	register cred_t *newcredp, *credp = u.u_lwpp->l_cred;
	register unsigned uid;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if ((uid = uap->uid) > MAXUID || uid < (uid_t) 0) {
		error = EINVAL;
	} else if (uid == credp->cr_ruid || uid == credp->cr_uid ||
		   uid == credp->cr_suid || !pm_denied(credp, P_SETUID)) {
		newcredp = crdup2(credp);
		newcredp->cr_uid = uid;
		pm_recalc(newcredp);	/* MUST be done before crinstall() */
		crinstall(newcredp);
		error = 0;
	} else
		error = EPERM;

	return error;
}


struct setegida {
	int	gid;
};

/*
 * int setegid(struct setegida *uap, rval_t *rvp)
 *	Setegid(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
/* ARGSUSED */
int
setegid(register struct setegida *uap, rval_t *rvp)
{
	register cred_t *newcredp, *credp = u.u_lwpp->l_cred;
	register unsigned gid;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if ((gid = uap->gid) > MAXUID || gid < (gid_t) 0) {
		error = EINVAL;
	} else if (gid == credp->cr_rgid || gid == credp->cr_gid ||
		   gid == credp->cr_sgid || !pm_denied(credp, P_SETUID)) {
		newcredp = crdup2(credp);
		newcredp->cr_gid = gid;
		crinstall(newcredp);
		error = 0;
	} else
		error = EPERM;

	return error;
}


struct setgroupsa {
	u_int	gidsetsize;
	gid_t	*gidset;
};

/*
 * int setgroups(struct setgroupsa *uap, rval_t *rvp)
 *	Setgroups(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
/* ARGSUSED */
int
setgroups(register struct setgroupsa *uap, rval_t *rvp)
{
	register cred_t *newcredp;
	register u_short i, n;

	if (pm_denied(u.u_lwpp->l_cred, P_SETUID))
		return EPERM;

	n = uap->gidsetsize;
	if (n > (u_short)ngroups_max)
		return EINVAL;

	newcredp = crdup2(u.u_lwpp->l_cred);

	if (n != 0) {
		if (copyin((void *)uap->gidset, newcredp->cr_groups,
			   (size_t)n * sizeof (gid_t))) {
			crfreen(newcredp, 2);
			return EFAULT;
		}
		for (i = 0; i < n; i++) {
			if (newcredp->cr_groups[i] < 0 ||
			    newcredp->cr_groups[i] > MAXUID) {
				crfreen(newcredp, 2);
				return EINVAL;
			}
		}
	}
	newcredp->cr_ngroups = n;
	crinstall(newcredp);


	return 0;
}


struct getgroupsa {
	int	gidsetsize;
	gid_t	*gidset;
};

/*
 * int getgroups(struct getgroups *uap, rval_t *rvp)
 *	Getgroups(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry or exit.
 *
 */
int
getgroups(register struct getgroupsa *uap, rval_t *rvp)
{
	register struct cred *current_credp = u.u_lwpp->l_cred;
	register gid_t n = current_credp->cr_ngroups;

	if (uap->gidsetsize != 0) {
		if ((gid_t)uap->gidsetsize < n)
			return EINVAL;
		if (copyout(current_credp->cr_groups,
			    (void *)uap->gidset, (size_t)n * sizeof (gid_t))) {
			return EFAULT;
		}
	}

	rvp->r_val1 = n;
	return 0;
}


struct setpgrpa {
	int	flag;
	int	pid;
	int	pgid;
};

/* Enhanced Application Compatibility Support */
struct setpgrpa_sco {
	int	flag;
	int	(*retaddr)();
	int	pid;
	int	pgid;
};
/* End Enhanced Application Compatibility Support */

/*
 * int setpgrp( struct setpgrpa *uap, rval_t *rvp)
 * 	Implement the setpgrp(2), setsid(2), setpgid(2), getpgrp(2), 
 *	getsid(2), and getpgid(2) system calls.
 *
 * Calling/Exit State:
 *	No locks should be held on entry and none held on return.
 *	The caller must be at PLBASE.
 *
 * Description:
 * 	We always acquire two spin locks in the code below in the following
 *	order:
 *	  p_sess_mutex -- This lock prevents multiple LWPs in the same
 *			  process from changing the process' session
 *			  membership simultaneously.
 *	  s_mutex -- This lock interlocks updates by multiple processes
 *		     in the same session (e.g. fork(2), _exit(2), setpgid(2)
 *		     activity).
 *
 * 	Two locks are necessary.  Without the p_sess_mutex lock, p->p_sessp 
 *	could be changing at the same time we acquired the session pointer and 
 *	tried to lock s_mutex, due to another LWP in the process performing a
 *	concurrent setsid(2) call.  Even worse, the session structure
 *	referenced by the stale pointer could be freed up by the LWP
 *	performing the setsid(2) call.
 *	
 */
/* ARGSUSED */
int
setpgrp(struct setpgrpa *uap, rval_t *rvp)
{
	register proc_t *p;
	register struct pid *pgidp;
	register sess_t *sp;
	register sess_t *nsp;
	struct setpgrpa_sco *scoap;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/* Enhanced Application Compatibility Support */
	if (IS_SCOEXEC) {
		/* Must be careful to overlay the first argument first */
		/* to avoid clobbering the next arugment in the array*/
		scoap = (struct setpgrpa_sco *)uap;
		uap->pid = scoap->pid;
		uap->pgid = scoap->pgid;

		/* Translate SCO flag to SVR4 flag */
		switch (uap->flag) {
		case	0:	/* SCO_GETPGRP */
		case	1:	/* SCO_SETPGRP */
		case	3:	/* SCO_SETSID */
			break;
		case	2:	/* SCO_SETPGID	*/
			uap->flag = 5;
			break;
		default:
			return (ENOSYS);
		}
	}
	/* End Enhanced Application Compatibility Support */

	p = u.u_procp;
	switch (uap->flag) {

	case 1: /* setpgrp() */
		nsp = kmem_alloc(sizeof *nsp, KM_SLEEP);
		(void)LOCK(&p->p_sess_mutex, PL_SESS);	/* interlock LWPs */
		sp = p->p_sessp;
		(void)LOCK(&sp->s_mutex, PL_SESS);	/* interlock procs */
		if (sp->s_sidp != p->p_pidp &&
		    ((pgidp = pgfind_sess(p->p_pidp->pid_id)) == NULL || 
		     (pgidp->pid_pgprocs == p && p->p_pglinkf == NULL 
		      && pgidp->pid_zpgref == 0))) {
			/*
			 * We allow a process to perform setpgrp() provided 
			 * that the process is not a session leader, and that
			 * its process-ID is not use as a process group-ID 
			 * by any other processes in the system.
			 */
			sp = sess_create(nsp);	/* old session unlocked */
			UNLOCK(&p->p_sess_mutex, PLBASE);
			if (sp != NULL)
				kmem_free(sp, sizeof *sp);
		} else {
			UNLOCK(&sp->s_mutex, PL_SESS);
			UNLOCK(&p->p_sess_mutex, PLBASE);
			kmem_free(nsp, sizeof *nsp);
		}
		rvp->r_val1 = p->p_sid;
		return 0;

	case 3: /* setsid() */
		/*
		 * Create a new session if the calling process is not a process
		 * group leader, or there are no processes whose process group-
		 * ID equals the process ID of the calling process. 
		 */
		nsp = kmem_alloc(sizeof *nsp, KM_SLEEP);
		(void)LOCK(&p->p_sess_mutex, PL_SESS);	/* interlock LWPs */
		sp = p->p_sessp;
		(void)LOCK(&sp->s_mutex, PL_SESS);	/* interlock procs */
		if (pgfind_sess(p->p_pidp->pid_id) != NULL) {
			/*
			 * The calling process is a process group leader, or
			 * there are other process(es) with a process group ID
			 * equal to the process ID of the calling process.
			 */
			UNLOCK(&sp->s_mutex, PL_SESS);
			UNLOCK(&p->p_sess_mutex, PLBASE);
			kmem_free(nsp, sizeof *nsp);
			return EPERM;
		}
		sp = sess_create(nsp);	/* old session unlocked */
		UNLOCK(&p->p_sess_mutex, PLBASE);
		if (sp != NULL)
			kmem_free(sp, sizeof *sp);
		rvp->r_val1 = p->p_sid;
		return 0;

	case 5: /* setpgid() */
	{
		/*
		 * The setpgid() function is used to either join an existing
		 * process group or create a new process group within the
		 * session of the calling process.  The calling process must 
		 * not be a session leader.
		 */
		pid_t pgid;
		pid_t pid;
		boolean_t exit;  	/* zombie process */
		boolean_t in_sess;  

		pid = uap->pid;
		pgid = uap->pgid;
		if (pid < 0 || pid >= MAXPID) {
			/*
			 * IEEE POSIX std 1003.1-1988 (and 1003.1-1990) 
			 * doesn't state that EINVAL should be returned for 
			 * invalid process-IDs.  We have chosen to be dogmatic 
			 * in our adherence to the standard, even though 
			 * EINVAL seems preferable.
			 */
			return ESRCH;
		}
		if (pgid < 0 || pgid >= MAXPID)
			return EINVAL;
		if (pid == 0)
			pid = p->p_pidp->pid_id;
		if (pgid == 0)
			pgid = pid;

		(void)LOCK(&p->p_sess_mutex, PL_SESS);	/* interlock LWPs */
		sp = p->p_sessp;
		(void)LOCK(&sp->s_mutex, PL_SESS);	/* interlock procs */
		UNLOCK(&p->p_sess_mutex, PL_SESS);

		exit = B_FALSE;
		in_sess = B_TRUE;
		/* 
		 * The MAC checks are not performed here because the only 
		 * way a process within a session can have a different LID is 
		 * if that process changes its LID via executing a privileged 
		 * system call lvlproc().  Once a process has the privilege 
		 * to change its LID, MAC can not prevent a process
		 * group from having processes with different LIDs
		 * because a process can change its LID after it joins
		 * the process group.
	 	 */
		if (pid != p->p_pidp->pid_id) {
			/*
			 * There are usually a small number of processes
			 * in the session, and thus the linear search
			 * through the session is faster than the hash
			 * lock round trip incurred when using prfind().
			 */
			if ((p = prfind_sess(pid)) == NULL) {
				/* 
				 * The target process may be zombie or
				 * outside of the caller's session. 
				 */
				if ((p = prfind(pid)) == NULL) {
					UNLOCK(&sp->s_mutex, PLBASE);
					return ESRCH;
				}

				if (p->p_sessp == sp)
					UNLOCK(&p->p_mutex, PL_SESS);
				else
					in_sess = B_FALSE;
				exit = B_TRUE;
			}
			if (p->p_parent != u.u_procp) {
				/*
				 * The value of the pid argument does
				 * not match the process-ID of the
				 * calling process, or of a child
				 * process of the calling process.
				 */
				if (!in_sess)
					UNLOCK(&p->p_mutex, PL_SESS);
				UNLOCK(&sp->s_mutex, PLBASE);
				return ESRCH;
			}
			/*
			 * Check to see if the target process has performed 
			 * an exec(2) system call.
			 *
			 *  NOTE:
			 *	The P_EXECED flag is being checked without
			 *	any interlock whatsoever here with exec(2).
			 *	This lack of interlock is harmless however,
			 *	since from the time of this check to the
			 *	completion of the setpgid(2) system call,
			 *	no exec(2) operation could beat us to
			 *	completion.
			 */
			if (p->p_flag & P_EXECED) {
				/*
				 * The child process has successfully 
				 * executed one of the exec functions.
				 */
				if (!in_sess)
					UNLOCK(&p->p_mutex, PL_SESS);
				UNLOCK(&sp->s_mutex, PLBASE);
				return EACCES;
			}

			/* 
			 * Release the p_mutex.  The process cannot exit
			 * because we hold the session lock.  If we are
			 * ignoring children, the child will call freeproc()
			 * and block trying to accquire the session lock to 
			 * release the process group reference of the child.
			 * Therefore race is harmless at this point.
			 */
			if (!in_sess) {
				UNLOCK(&p->p_mutex, PL_SESS);
				UNLOCK(&sp->s_mutex, PLBASE);
				return EPERM;
			}
		}
		if (p->p_sid == pid) {
			/* The process is a session leader. */
			UNLOCK(&sp->s_mutex, PLBASE);
			return EPERM;
		}

		if (p->p_pgid == pgid) {
			UNLOCK(&sp->s_mutex, PLBASE);
			break;
		}
		if (p->p_pidp->pid_id == pgid)
			pgidp = p->p_pidp;
		else if ((pgidp = pgfind_sess(pgid)) == NULL) {
			/*
			 * There is no process with a process group ID that
			 * matches the value of the pgid argument in the 
			 * same session as the calling process.
			 */
			UNLOCK(&sp->s_mutex, PLBASE);
			return EPERM;
		}
		if (exit)
			pgzmove(p, pgidp);
		else {
			pgexit(p, B_FALSE);
			pgjoin(p, pgidp);
		}
		UNLOCK(&sp->s_mutex, PLBASE);
		break;
	}

	case 0: /* getpgrp() */
		rvp->r_val1 = p->p_pgid;
		break;

	case 2: /* getsid() */
	case 4: /* getpgid() */
		if (uap->pid < 0 || uap->pid >= MAXPID) 
			return EINVAL;
		if (uap->pid != 0 && p->p_pidp->pid_id != uap->pid &&
		    (p = prfind(uap->pid)) == NULL)
			return ESRCH;
		/* Check MAC access if the target process is not ourself */
		if (p != u.u_procp &&
		    MAC_ACCESS(MACDOM, CRED()->cr_lid, p->p_cred->cr_lid) && 
		    pm_denied(CRED(), P_MACREAD)) {
			UNLOCK(&p->p_mutex, PLBASE);
			return ESRCH;
		}
		if (uap->flag == 2)
			rvp->r_val1 = p->p_sid;
		else
			rvp->r_val1 = p->p_pgid;
		if (p != u.u_procp)
			UNLOCK(&p->p_mutex, PLBASE);
		break;
	}
	return 0;
}


/*
 * int pause(void)
 * 	Indefinite wait
 *
 * Calling/Exit State:
 * 	No locks are held at entry and none held at exit.
 * 	It is assumed that pause_event was initialized as boot time
 * 	via: EVENT_INIT(&pause_event).
 *
 */
int
pause(void)
{
	ASSERT(KS_HOLD0LOCKS());

	/*
	 * This code loops in case someone performs an EVENT_SIGNAL() 
	 * or EVENT_BROADCAST() operation on pause_event.
	 *
	 * NOTE: The pause() system call no longer relies on setjmp()/longjmp()
	 *	 support from systrap().
	 */
	while (EVENT_WAIT_SIG(&pause_event, PRIWAIT) == B_TRUE) 
		;
	return EINTR;
}


/*
 * Mode mask for creation of files.
 */
struct umaska {
	int     mask;
};

/*
 * int umask(register struct umaska *uap, rval_t *rvp)
 *	Implement the umask(2) system call.
 *
 * Calling/Exit State:
 *	No locks held upon entry.  None held upon exit.
 *
 */
int
umask(register struct umaska *uap, rval_t *rvp)
{
	register proc_t *pp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	/*
	 * No locking is done here.
	 * We simply rely on atomic integer-sized memory operations to
	 * be provided by the hardware.
	 */
	pp = u.u_procp;
	rvp->r_val1 = (int) pp->p_cmask;
	pp->p_cmask = (mode_t) (uap->mask & PERMMASK);
	return 0;
}


/* XENIX Support */
#define NIND		((long) (512 / sizeof (daddr_t)))
#define MAXBLOCKS	(10 + NIND + NIND*NIND + NIND*NIND*NIND)
/* End XENIX Support */

struct ulimita {
	int	cmd;
	long	arg;
};

/*
 * int ulimit(struct ulimita *uap, rval_t *rvp)
 *	Ulimit(2) system call.
 *
 * Calling/Exit State:
 *	No spin locks should be held on entry; none are held on return.
 */
int
ulimit(struct ulimita *uap, rval_t *rvp)
{
	rlimits_t	*rlimitsp;

	ASSERT(KS_HOLD0LOCKS());

	rlimitsp = u.u_rlimits;	/* LWP's view of resource limits */

	switch (uap->cmd) {

	case UL_SFILLIM:	/* Set new file size limit. */
	{
		long arg = uap->arg;
		rlim_t lim;
		int error;

		/* This should be done differently --- XXX */
		/* XENIX Support */
		/* For XENIX compatibility, artificially limit the 
		 * maximum number of 512-byte blocks in a file to the 
		 * maximum number of blocks that are representable in 
		 * a 512-byte block file system.  MAXBLOCKS is in 
		 * units of 512 bytes.
 		 */
		if (VIRTUAL_XOUT && uap->arg > MAXBLOCKS)
			return EINVAL;
		/* End XENIX Support */

		if (arg < 0)
			return EINVAL;
		else if (arg >= (RLIM_INFINITY >> SCTRSHFT))
			lim = RLIM_INFINITY;
		else
			lim = arg << SCTRSHFT;

		if (lim > u.u_rlimits->rl_limits[RLIMIT_FSIZE].rlim_cur &&
		    pm_denied(CRED(), P_SYSOPS))
			return EPERM;

		if (error = rlimit(RLIMIT_FSIZE, lim, lim))
			return error;

		rlimitsp = u.u_rlimits;
	}
	/* FALLTHROUGH */

	case UL_GFILLIM:	/* Return current file size limit. */
		rvp->r_off = (rlimitsp->rl_limits[RLIMIT_FSIZE].rlim_cur >>
						SCTRSHFT);
		break;

	case UL_GMEMLIM:	/* Return maximum possible break value. */
	{
		struct seg *seg, *sseg;
		struct seg *nextseg;
		proc_t *p = u.u_procp;
		struct as *as = p->p_as;
		vaddr_t brkend;
		size_t size, datalim;

		as_rdlock(as);
		brkend = pageroundup(p->p_brkbase + p->p_brksize);

		/*
		 * Find the segment that starts at a virtual address
		 * greater than or equal to the end of the current break.
		 */
		nextseg = NULL;
		sseg = seg = as->a_segs;
		if (seg != NULL) {
			do {
				if (seg->s_base >= brkend) {
					nextseg = seg;
					break;
				}
				seg = seg->s_next;
			} while (seg != sseg);
		}

		/*
		 * We reduce the max break value (base+rlimit[DATA])
		 * if we run into another segment, or the end of a valid
		 * user address range.  We also are limited by rlimit[VMEM].
		 */
		rvp->r_off = VALID_USR_END(brkend);
		if (nextseg != NULL && (vaddr_t)rvp->r_off > nextseg->s_base)
			rvp->r_off = (off_t)nextseg->s_base;
		datalim = rlimitsp->rl_limits[RLIMIT_DATA].rlim_cur & PAGEMASK;
		if ((vaddr_t)rvp->r_off - p->p_brkbase > datalim)
			rvp->r_off = (off_t)(p->p_brkbase + datalim);

		/*
		 * Also handle case where rlimit[VMEM] has been 
		 * lowered below the current address space size.
		 */
		size = rlimitsp->rl_limits[RLIMIT_VMEM].rlim_cur & PAGEMASK;
		if (as->a_size < size)
			size -= as->a_size;
		else
			size = 0;

		as_unlock(as);

		rvp->r_off = (off_t)min((vaddr_t)rvp->r_off, brkend + size);
		break;
	}

	case UL_GDESLIM:	/* Return approximate number of open files */
		rvp->r_off = rlimitsp->rl_limits[RLIMIT_NOFILE].rlim_cur;
		break;

	case UL_GTXTOFF:	/* XENIX compat */
		rvp->r_off = uap->arg;
		break;

	default:
		return EINVAL;
	}

	return 0;
}


/* Structure used by get/setrlimit(2) system calls. */
struct rlimita {
	int	resource;
	struct rlimit *rlp;
};

/*
 * int getrlimit(struct rlimita *uap, rval_t *rvp)
 *	Getrlimit(2) system call.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry; no locks are held on return.
 */
/*ARGSUSED*/
int
getrlimit(struct rlimita *uap, rval_t *rvp)
{
	int resource = uap->resource;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Determine if the argument passed is within the
	 * valid ranges for the requested resource.
	 */
	if (resource < 0 || resource >= RLIM_NLIMITS)
		return EINVAL;

	if (copyout(&u.u_rlimits->rl_limits[resource], 
		    (caddr_t)uap->rlp, sizeof (struct rlimit)))
		return EFAULT;

	return 0;
}

/*
 * int setrlimit(struct rlimita *uap, rval_t *rvp)
 *	Setrlimit(2) system call.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry; no locks are held on return.
 */
/*ARGSUSED*/
int
setrlimit(struct rlimita *uap, rval_t *rvp)
{
	int resource = uap->resource;
	struct rlimit rlim;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Determine if the argument passed is within the
	 * valid ranges for the requested resource.
	 */
	if (resource < 0 || resource >= RLIM_NLIMITS)
		return EINVAL;

	if (copyin((caddr_t)uap->rlp, &rlim, sizeof rlim))
		return EFAULT;

	return rlimit(resource, rlim.rlim_cur, rlim.rlim_max);
}


struct timesa {
	struct	tms *times;
};

/*
 * int times(struct timesa *uap, rval_t *rvp)
 * 	Times(2) system call, get process and child process aggregate
 *	CPU times (system and user) for all LWPs in the process.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry; no locks are held on return.
 *
 * Remarks:
 *	Internally the kernel tracks the aggregate CPU times as double
 *	long words.  To preserve the existing system call interface, and
 *	to return meaningful information (most of the time), we return
 *	the low order part of each double long word.
 */
int
times(struct timesa *uap, rval_t *rvp)
{
	struct tms tms;

	proc_t *p = u.u_procp;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Sample all the time info. We do not hold any locks
	 * as these are only samples.
	 */
	tms.tms_utime  = (clock_t)p->p_utime.dl_lop;
	tms.tms_stime  = (clock_t)p->p_stime.dl_lop;
	tms.tms_cutime = (clock_t)p->p_cutime.dl_lop;
	tms.tms_cstime = (clock_t)p->p_cstime.dl_lop;

	if (copyout(&tms, (void *)uap->times, sizeof tms))
		return EFAULT;

	/* Note: lbolt is read here without any locks. */
	rvp->r_time = (time_t)lbolt;
	return 0;
}


struct profila {
	short	*bufbase;
	unsigned bufsize;
	unsigned pcoffset;
	unsigned pcscale;
};

/*
 * int profil(struct profila *uap, rval_t *rvp)
 *	Profiling system call.
 *
 * Calling/Exit State:
 *	No locks can be held on entry and no locks will be held on return.
 *
 */
/* ARGSUSED */
int
profil(struct profila *uap, rval_t *rvp)
{
	proc_t	*pp = u.u_procp;
	lwp_t	*lwpp = u.u_lwpp;
	struct prof	*profp;
	boolean_t	holding_locks = B_FALSE;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Allocate a profiling structure if one does not exist. A 
	 * profiling structure is allocated to the process as a 
	 * consequence of the first invocation of the profil(2) system
	 * call. Note that the profiling structure if allocated is not 
	 * deallocated until the process exits.
	 */

	if (pp->p_profp == NULL) {
		profp = kmem_alloc(sizeof *profp, KM_SLEEP);
		SLEEP_INIT(&profp->pr_lock, 0, &proflkinfo, KM_SLEEP);
		(void)LOCK(&pp->p_mutex, PLHI);
		if (pp->p_profp == NULL) {
			pp->p_profp = profp;
			UNLOCK(&pp->p_mutex, PLBASE);
		} else {
			/*
			 * Raced with another LWP. Deallocate the 
			 * structure we just allocated.
			 */
			UNLOCK(&pp->p_mutex, PLBASE);
			SLEEP_DEINIT(&profp->pr_lock);
			kmem_free(profp, sizeof *profp);
		}
	}
	profp = pp->p_profp;
	if (!SINGLE_THREADED()) {
		holding_locks = B_TRUE;
		SLEEP_LOCK(&profp->pr_lock, PRIMED);
	}
	/*
	 * Update the profiling structure
	 * with the system call parameters.
	 * We do this under the protection of
	 * p_mutex to get a consistent
	 * snapshot for a possible concurrent fork() operation.
	 */
	(void)LOCK(&pp->p_mutex, PLHI);
	 
	profp->pr_base = uap->bufbase;
	profp->pr_size = uap->bufsize;
	profp->pr_off = uap->pcoffset;
	profp->pr_scale = uap->pcscale;

	/*
	 * Post the profiling status to all the LWPs if it has
	 * changed.
	 */

	if (profp->pr_scale & ~1) {
		if (!(lwpp->l_trapevf & EVF_PL_PROF)) {
			/* Turn profiling on. */
			trapevproc(pp, EVF_PL_PROF, B_TRUE);
		}
	} else if (lwpp->l_trapevf & EVF_PL_PROF) { 
		/* Turn profiling off. 
		 * Turn off L_CLOCK flag for the current context.
		 * Note that we do not turn off L_CLOCK flag
		 * for other contexts in the process. We believe that
		 * the resulting anomoly (a possible extra tick 
		 * if profiling is turned on again) is considered
		 * harmless.
		 */ 
		trapevunproc(pp, EVF_PL_PROF, B_TRUE);
		if (lwpp->l_flag & L_CLOCK) {
			(void)LOCK(&lwpp->l_mutex, PLHI);
			lwpp->l_flag &= ~L_CLOCK;
			UNLOCK(&lwpp->l_mutex, PLHI);	/* p_mutex still held */
		}
	}

	UNLOCK(&pp->p_mutex, PLBASE);
	if (holding_locks) {
		SLEEP_UNLOCK(&profp->pr_lock);
	}
	return 0;
}


struct	procpriva {
	int	cmd;
	priv_t	*privp;
	int	count;
};

/*
 * int procpriv(struct procpriva *uap, rval_t *rvp)
 *	set, clear, retrieve, or count the privileges of the calling process.
 *
 * Calling/Exit State:
 *	 No locks are held upon entry and none held at exit.
 *
 */
int
procpriv(struct procpriva *uap, rval_t *rvp)
{
	cred_t	*crp;
	int error;

	error = pm_process(uap->cmd, rvp, u.u_lwpp->l_cred,
		           uap->privp, uap->count, &crp);

	/*
	 * If the value of "crp" is non-NULL then it was copied
	 * in pm_process().
	 */
	if (!error  && crp != NULL) 
		crinstall(crp);

	return error;
}


struct sleep_arg {
	unsigned	time;
};

/*
 * int __sleep(unsigned time)
 *	Implements the sleep() system call.
 *
 * Calling/Exit State:
 *	No locks can be held on entry and none will be held on return.
 */
int
__sleep(struct sleep_arg *uap, rval_t *rvp)
{
	lwp_t	*lwpp = u.u_lwpp;
	unsigned curtime = (unsigned)lbolt;
	unsigned timeslept;
	toid_t	 ourid;

	ASSERT(KS_HOLD0LOCKS());
	if (uap->time == 0) {
		rvp->r_val1 = 0;	
		return 0;
	}
	EVENT_CLEAR(&lwpp->l_slpevent);
	ourid = itimeout(nudgeus, &lwpp->l_slpevent, (uap->time)* HZ, PLBASE);
	if (ourid == 0) {
		/*
		 * Could not schedule a timeout.
		 */
		rvp->r_val1 = 0;
		return (0);
	}
	if (!EVENT_WAIT_SIG(&lwpp->l_slpevent, PRIMED)) {
		(void)untimeout(ourid);
		timeslept = ((unsigned)lbolt - curtime)/HZ;
		if (timeslept < uap->time) {
			rvp->r_val1 = uap->time - timeslept;
		} else {
			rvp->r_val1 = 0; 
		}
		return 0;
	}
	/*
	 * Normal wakeup.
	 */
	rvp->r_val1=0;
	return 0;
}

/*
 * void nudgeus(void *arg)
 *	When this executes it will do a wakeup on the event structure
 *	pointed to by arg.
 *
 * Calling/Exit State:
 *	None.
 */
STATIC void
nudgeus(void *arg)
{
	EVENT_SIGNAL((event_t *)arg, 0);
}
