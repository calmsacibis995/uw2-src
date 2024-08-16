/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/pgrp.c	1.11"

#include <util/param.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <proc/signal.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/proc_hier.h>
#include <proc/lwp.h>
#include <proc/session.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <mem/kmem.h>

/*
 * Remove the given process group 'pgidp' from the given session 'sp'.
 * The containing session must be held locked by the caller.  The session
 * remains locked upon return.
 */
#define	RM_PGRP_FROM_SESS(pgidp, sp) \
{ \
	if ((sp)->s_pgrps == (pgidp)) { \
		(sp)->s_pgrps = (pgidp)->pid_pgrpf; \
		if ((pgidp)->pid_pgrpf != NULL) \
			(pgidp)->pid_pgrpf->pid_pgrpb = (pgidp)->pid_pgrpb; \
	} else { \
		if ((pgidp)->pid_pgrpf != NULL) \
			(pgidp)->pid_pgrpf->pid_pgrpb = (pgidp)->pid_pgrpb; \
		else \
			(sp)->s_pgrps->pid_pgrpb = (pgidp)->pid_pgrpb; \
		(pgidp)->pid_pgrpb->pid_pgrpf = (pgidp)->pid_pgrpf; \
	} \
	(pgidp)->pid_pgprocs = NULL; \
}


/*
 * Add the given process group 'pgidp' to the session 'sp'.
 * The containing session must be held locked by the caller.
 * The session remains locked upon return.
 */
#define ADD_PGRP_TO_SESS(pgidp, sp) \
{ \
	if (((pgidp)->pid_pgrpf = (sp)->s_pgrps) == NULL) \
		(pgidp)->pid_pgrpb = (pgidp); \
	else { \
		(pgidp)->pid_pgrpb = (sp)->s_pgrps->pid_pgrpb; \
		(sp)->s_pgrps->pid_pgrpb = (pgidp); \
	} \
	(sp)->s_pgrps = (pgidp); \
}


/*
 * Remove the given non-zombie process 'p' from the process group 'pgidp'.
 * The containing session must be held locked by the caller.  The session
 * remains locked upon return.
 *
 * NOTE: It is assumed that the process being removed from the process group
 *	 is NOT the last process in the group.
 */
#define	RM_PROC_FROM_PGRP(p, pgidp) \
{ \
	if ((pgidp)->pid_pgprocs == (p)) { \
		(pgidp)->pid_pgprocs = (p)->p_pglinkf; \
		ASSERT((p)->p_pglinkf != NULL); \
		(p)->p_pglinkf->p_pglinkb = (p)->p_pglinkb; \
	} else { \
		if ((p)->p_pglinkf != NULL) \
			(p)->p_pglinkf->p_pglinkb = (p)->p_pglinkb; \
		else \
			(pgidp)->pid_pgprocs->p_pglinkb = (p)->p_pglinkb; \
		(p)->p_pglinkb->p_pglinkf = (p)->p_pglinkf; \
	} \
}

/*
 *
 * void pgsess_fork(proc_t *cp, proc_t *pp)
 *	Add the child process 'cp' to the process group and session of
 *	its parent 'pp'.
 *
 * Calling/Exit State:
 *	The session lock of the parent process must be held at
 *	entry. The session lock remains held at exit.
 *
 * Remarks:
 *	This function is called from fork() only, and must be called before
 *	the child process becomes visible to the rest of the system.
 *
 */
void
pgsess_fork(proc_t *cp, proc_t *pp)
{
	ASSERT(LOCK_OWNED(&pp->p_sessp->s_mutex));
	cp->p_pgidp = pp->p_pgidp;
	cp->p_pglinkf = pp->p_pgidp->pid_pgprocs;
	cp->p_pglinkb = pp->p_pgidp->pid_pgprocs->p_pglinkb;
	pp->p_pgidp->pid_pgprocs->p_pglinkb = cp;
	pp->p_pgidp->pid_pgprocs = cp;
	cp->p_pgid = pp->p_pgid;
	if (pp->p_flag & P_PGORPH)
		cp->p_flag |= P_PGORPH;
	cp->p_sessp = pp->p_sessp;
	cp->p_parentinsess = 1;
	cp->p_cttydev = pp->p_cttydev;
	cp->p_sid = pp->p_sid;
	pp->p_sessp->s_ref++;	/* One more process in session */
}


/*
 *
 * void pgnotify(proc_t *p, struct pid *pgidp, boolean_t orphaned)
 *	Record the process 'p' as being in the process group 'pgidp'.
 *	The process group is an orphaned process group if 'orphaned'
 *	is B_TRUE.  Also inform any LWPs in the parent
 *	process of 'p' waiting against a specific process group-ID.
 *
 * Calling/Exit State:
 *	The session (s_mutex) lock of the session containing the process
 *	'p' must be held by the caller upon entry,unless pgnotify() is being
 *	called as part of a setsid(2) or setpgrp(2) operation.  If the
 *	session is locked upon entry, it remains locked upon return.
 *
 */
STATIC void
pgnotify(register proc_t *p, struct pid *pgidp, boolean_t orphaned)
{
	register proc_t *pp;
	pl_t pl;



	/*
	 * The P_PGORPH state is only valid for non-zombies; we might
	 * set this state for zombies as called from pgzmove(),
	 * but this is harmless.
	 */
	pl = LOCK(&p->p_mutex, PLHI);
	if (orphaned)
		p->p_flag |=  P_PGORPH;
	else
		p->p_flag &= ~P_PGORPH;
	p->p_pgid = pgidp->pid_id;

	/*
	 * Notify the parent of the new process group membership (even if the
	 * process is not in a waitable state), since we might have been the
	 * last child of the parent in the given process group.  Otherwise,
	 * the parent could wait forever for a non-existent process group....
	 */
	pp = p->p_parent;
	(void)LOCK_SH(&pp->p_mutex, PLHI);
	pp->p_flag |= P_CLDEVT;
	if (SV_BLKD(&pp->p_waitsv))
		SV_BROADCAST(&pp->p_waitsv, 0);
	UNLOCK(&pp->p_mutex, PLHI);
	UNLOCK(&p->p_mutex, pl);
}


/*
 *
 * boolean_t pgorphaned(struct pid *pgidp, boolean_t sigcont)
 *	Check if the specified process group 'pgidp' is to be orphaned.
 *	If 'pgidp' is to be orphaned, it orphans the process group
 *	and sends SIGHUP/SIGCONT to the entire process group if any
 *	process is found to be stopped for job control and 'sigcont'
 *	is B_TRUE.
 *
 * Calling/Exit State:
 *	The session lock (s_mutex) of the specified process group 'pgidp'
 *	must be held locked by the caller.  This lock remains held upon
 *	return.  This function returns B_TRUE, if the specified group
 *	has been orphaned.  Otherwise, B_FALSE is returned.
 *
 * Remarks:
 *	This function contains three loops in the logic that marks the group
 *	as orphaned (with some replicated code), so that the number of p_mutex
 *	lock acquisitions is kept to the smallest amount possible.
 *
 */
boolean_t
pgorphaned(struct pid *pgidp, boolean_t sigcont)
{
	register proc_t *np;		/* used to walk pgrp members */
	register pl_t pl;

	ASSERT(LOCK_OWNED(&pgidp->pid_pgprocs->p_sessp->s_mutex));

	for (np = pgidp->pid_pgprocs; np != NULL; np = np->p_pglinkf) {
		if (np->p_parentinsess && np->p_parent->p_pgidp != pgidp)
			return B_FALSE;
	}

	/*
	 * The process group is to be orphaned.
	 */
	np = pgidp->pid_pgprocs;	/* start at the beginning */
	do {
		pl = LOCK(&np->p_mutex, PLHI);
		np->p_flag |= P_PGORPH;
		if (np->p_sigjobstop > 0) {
			/*
			 * We just encountered a process stopped for job
			 * control.  If 'sigcont' is B_TRUE, then send
			 * SIGHUP/SIGCONT to the process, and all previous
			 * processes in the process group.
			 */
			register proc_t *startp;

			if (sigcont) {
				(void)sigtoproc_l(np, SIGHUP, NULL);
				(void)sigtoproc_l(np, SIGCONT, NULL);
				UNLOCK(&np->p_mutex, pl);

				for (startp = pgidp->pid_pgprocs; startp != np;
				     startp = startp->p_pglinkf) {
					pl = LOCK(&startp->p_mutex, PLHI);
					(void)sigtoproc_l(startp, SIGHUP, NULL);
					(void)sigtoproc_l(startp, SIGCONT,
							  NULL);
					UNLOCK(&startp->p_mutex, pl);
				}
			} else
				UNLOCK(&np->p_mutex, pl);

			/*
			 * Mark all remaining processes in the process group
			 * as orphaned, and also post SIGHUP/SIGCONT to all
			 * such processes while p_mutex is held (optimization)
			 * if 'sigcont' is B_TRUE.
			 */
			while ((np = np->p_pglinkf) != NULL) {
				pl = LOCK(&np->p_mutex, PLHI);
				np->p_flag |= P_PGORPH;
				if (sigcont) {
					(void)sigtoproc_l(np, SIGHUP, NULL);
					(void)sigtoproc_l(np, SIGCONT, NULL);
				}
				UNLOCK(&np->p_mutex, pl);
			}
			return B_TRUE;
		}
		UNLOCK(&np->p_mutex, pl);
	} while ((np = np->p_pglinkf) != NULL);

	return B_TRUE;
}


/*
 *
 * void pgexit(proc_t *p, boolean_t chgsess)
 * 	Remove the non-zombie process 'p' from its current process group.
 *
 * Calling/Exit State:
 *	The session lock containing the process group must be held
 *	by the caller at entry.  This lock remains held upon exit.  If
 *	the "chgsess" parameter is B_TRUE, then the current process is
 *	performing a setsid(2) or setpgrp(2) request.
 *
 */
void
pgexit(register proc_t *p,  boolean_t chgsess)
{
	register struct pid *pgidp;
	register proc_t *np;
	register pl_t pl;

	ASSERT(LOCK_OWNED(&p->p_sessp->s_mutex));

	/*
	 * Remove the departing non-zombie process from its process group.
	 */
	pgidp = p->p_pgidp;
	if (p->p_pglinkb == p) {
		sess_t *sp;
		/*
		 * There are no non-zombie processes left in the process group.
		 * Remove the process group from the list of process groups
		 * in the session.
		 */
		sp = p->p_sessp;
		RM_PGRP_FROM_SESS(pgidp, sp);
		if (pgidp->pid_zpgref == 0) {
			/*
			 * No zombie (or non-zombie) processes are referring
			 * to the ID as a process group-ID.
			 */
			pid_rele(pgidp);
		}
		return;
	}
	/*
	 * One or more non-zombie processes remain in the process group.
	 */
	RM_PROC_FROM_PGRP(p, pgidp);

	if ((p->p_flag & P_PGORPH) == 0) {
		/*
		 * The process group is not orphaned; check if the
		 * process group becomes orphaned by the departure
		 * of the process 'p' from the group.
		 */
		if (p->p_parentinsess && p->p_parent->p_pgidp != pgidp) {
			/*
			 * Set p_pgidp to NULL to avoid falsely
			 * orphaning the pgrp for setpgid(2).  For
			 * setsid and setpgrp, the p_parentinsess
			 * has been already cleared for all childern
			 * of process 'p'.
			 */
			p->p_pgidp = NULL;
			(void) pgorphaned(pgidp, B_FALSE);
		}
	} else if (!chgsess) {
		/*
		 * Check if the process group became unorphaned due to
		 * the process 'p' departing from the group.
		 */
		np = pgidp->pid_pgprocs;
		do {
			if (np->p_parent == p) {
				/*
				 * Unorphan the process group.
				 */
				np = pgidp->pid_pgprocs;
				do {
					pl = LOCK(&np->p_mutex, PLHI);
					np->p_flag &= ~P_PGORPH;
					UNLOCK(&np->p_mutex, pl);
					np = np->p_pglinkf;
				} while (np != NULL);
				break;
			}
		} while (np = np->p_pglinkf);
	}
}


/*
 *
 * void pgjoin(proc_t *p, struct pid *pgidp)
 *	Add the non-zombie process 'p' to process group 'pgidp'.
 *
 * Calling/Exit State:
 *	The session containing the process group must be locked
 *	by the caller, unless pgjoin() is being called as part of
 *	a setsid(2) or setpgrp(2) operation.  If the session is
 *	locked upon entry, it remains locked upon return.
 *
 */
void
pgjoin(proc_t *p, struct pid *pgidp)
{
	register proc_t *np;
	register sess_t *sp;
	register pl_t pl;


	/*
	 * Insert the process into its new process group.
	 */
	np = pgidp->pid_pgprocs;
	p->p_pgidp = pgidp;
	p->p_pglinkf = np;
	pgidp->pid_pgprocs = p;

	if (np == NULL) {
		/*
		 * We are the first non-zombie process to join (or rejoin)
		 * the process group.
		 */
		p->p_pglinkb = p;
		if (pgidp->pid_zpgref == 0) 	/* creating a new pgrp */
			pid_hold(pgidp);

		/*
	 	 * Add the process group to the list of process groups
		 * containing one or more non-zombie processes within the
		 * session.
		 */
		sp = p->p_sessp;
		ADD_PGRP_TO_SESS(pgidp, sp);

		/*
		 * Update p_pgid and orphaned state.
		 */
		pgnotify(p, pgidp, p->p_parentinsess ? B_FALSE : B_TRUE);
	} else {
		/* Join the existing process group. */
		p->p_pglinkb = np->p_pglinkb;
		np->p_pglinkb = p;
		if (np->p_flag & P_PGORPH) {
			/*
			 * Our addition to the process group may have
			 * unorphaned the process group.
			 */
			if (p->p_parentinsess &&
			    p->p_parent->p_pgidp != pgidp) {	/* unorphaned */
				pgnotify(p, pgidp, B_FALSE);
				do {
					pl = LOCK(&np->p_mutex, PLHI);
					np->p_flag &= ~P_PGORPH;
					UNLOCK(&np->p_mutex, pl);
				} while ((np = np->p_pglinkf) != NULL);
			} else 				/* still orphaned */
				pgnotify(p, pgidp, B_TRUE);
		} else {
			/*
			 * We might have been the only parent outside
			 * the process group that was keeping the
			 * process group from being orphaned.
			 *
			 * Thus, our addition to the process group may
			 * have orphaned the process group.
			 */
			pgnotify(p, pgidp, pgorphaned(pgidp, B_FALSE));
		}
	}
}


/*
 *
 * void pgzmove(proc_t *p, register struct pid *pgidp)
 * 	Remove the zombie (or soon to become a zombie) process 'p' from its
 *	process group, where the process has already removed itself from the
 *	process group linkages within the group, and then add it to the
 *	process group 'pgidp'.
 *
 * Calling/Exit State:
 *	The session containing the process group must be locked
 *	by the caller.  The session remains locked upon return.
 *
 * Remarks:
 *	This function is used by setpgid(2), when it is known that the
 *	target process is a zombie, or is currently in exit() and has
 *	already removed itself from the process group chains.
 *
 */
void
pgzmove(register proc_t *p, register struct pid *pgidp)
{
	register struct pid *opgidp;	/* process group being left */

	ASSERT(LOCK_OWNED(&p->p_sessp->s_mutex));

	/*
	 * Remove from current process group.
	 */
	opgidp = p->p_pgidp;
	if (--opgidp->pid_zpgref == 0 && opgidp->pid_pgprocs == NULL) {
		/*
		 * No zombie (or non-zombie) processes remain in the
		 * old process group.
		 */
		pid_rele(opgidp);
	}

	/*
	 * Add to new process group.
	 */
	p->p_pgidp = pgidp;
	if (pgidp->pid_zpgref++ == 0 && pgidp->pid_pgprocs == NULL) {
		/*
		 * Since there are no non-zombie processes in
		 * the process group, we have no way to determine
		 * the session in which the process group resides.
		 * Establish a pointer via pid_zpgsess (since the
		 * pid_pgrpf and pid_pgrpb fields are now unused).
		 */
		pgidp->pid_zpgsess = p->p_sessp;
	}

	/*
	 * Update p_pgid, and alert parent waiters if waiting on a specific
	 * process group-ID.
	 */
	pgnotify(p, pgidp, B_FALSE);
}


/*
 *
 * void pgdetach(void)
 * 	Remove the calling process from its process group, and handle the
 *	orphaning of any child process groups by the exit(2) of the calling
 *	parent process.
 *
 * Calling/Exit State:
 *	The session containing the process group must be locked by the
 *	caller.  The session remains locked upon return.
 *
 *	The calling process must also either not be ignoring exiting child-
 *	ren, or else P_CLDWAIT must be set in its p_flag field to prevent
 *	corruption of its parent-child-sibling chain by child processes
 *	exiting concurrently with this function.  In addition,
 *	the calling process must be single-threaded (one LWP).
 *
 * Description:
 * 	A SIGHUP/SIGCONT sequence is posted to all process groups orphaned
 * 	by the exit of the calling process.
 *
 */
void
pgdetach(void)
{
	register proc_t *p;		/* calling process */
	register proc_t *cp;		/* child process */
	register proc_t *np;		/* used to walk child pgrp */
	register struct pid *pgidp;	/* pgrp of child */
	register sess_t *sp;


	p = u.u_procp;
	ASSERT(LOCK_OWNED(&p->p_sessp->s_mutex));
	ASSERT(p->p_nlwp == 1);
	ASSERT((p->p_flag & (P_NOWAIT|P_CLDWAIT)) == 0 ||
	       (p->p_flag & (P_NOWAIT|P_CLDWAIT)) == (P_NOWAIT|P_CLDWAIT));

	/*
	 * Remove the exiting process from its process group.
	 */
	pgidp = p->p_pgidp;
	pgidp->pid_zpgref++;		/* one more zombie pgrp ref */
	if (p->p_pglinkb == p) {
		/*
		 * The exiting process is the only process in the process group.
		 * Remove the process group from the list of process groups
		 * in the session.
		 */
		sp = p->p_sessp;
		RM_PGRP_FROM_SESS(pgidp, sp);
		/*
		 * The process group is removed from the session;
		 * establish a pointer identifying the session in which
		 * the process group resides (NOTE: pid_zpgsess overlays
		 * the now unused pid_pgrpf and pid_pgrpb fields).
		 */
		pgidp->pid_zpgsess = sp;
	} else {
		/*
		 * One or more non-zombie processes remain in the process
		 * group.
		 */
		RM_PROC_FROM_PGRP(p, pgidp);

		/*  Handle orphaness checks. */
		if ((p->p_flag & P_PGORPH) == 0 && p->p_parentinsess &&
		    p->p_parent->p_pgidp != pgidp) {
			/*
			 * The process group was not previously orphaned,
			 * and may have become orphaned by the departure
			 * of the process from the process group (since
			 * the parent of the departing process was in the
			 * same session, and in a different process group).
			 */
			(void) pgorphaned(pgidp, B_TRUE);
		}
	}

	/*
	 * Perform orphan process group checks/updates for all child
	 * processes.
	 */
	for (cp = p->p_child; cp != NULL; cp = cp->p_nextsib) {
		if (cp->p_parentinsess && cp->p_nlwp != 0) {
			/*
			 * The child is not zombie and is in the same session
			 * as the parent.
			 *
			 * NOTE #1:
			 *  There is race in that child might be exiting at
			 *  the same time we make p_nlwp != 0 check.  However,
			 *  this race is harmless because child would have
			 *  removed itself from the process group linkage.
			 *
			 * NOTE #2:
			 *  Clearing the "p_parentinsess" flag below for the
			 *  child would not be compliant with POSIX if init(1)
			 *  failed to invoke setsid/setpgrp for each child it
			 *  created, or if a child of init(1) failed to invoke
			 *  setsid/setpgrp itself before creating new children
			 *  in the absence of init(1) doing the setsid/setpgrp.
			 *
			 *  Consider that the disinheritance of the child to
			 *  init(1) could mean that "parentinsess" flag should
			 *  be non-zero, if in fact the child being
			 *  disinherited was in the same session as init(1)....
			 *
			 *  The assumption is that this situation does not
			 *  happen, and even if it does, the kernel assumes
			 *  that init(1) is not prepared to deal with processes
			 *  stopped by job control signals.  Hence,
			 *  "p_parentinsess" is always set to zero.
			 */
			cp->p_parentinsess = 0;

			if ((pgidp = cp->p_pgidp) != p->p_pgidp &&
			    (np = pgidp->pid_pgprocs) != NULL &&
			    (np->p_flag & P_PGORPH) == 0) {
				/*
				 * The exiting process might be preventing the
				 * child pgrp from being orphaned.  Clear
				 * p_parentinsess for all processes in the
				 * group that have the exiting process as the
				 * parent, to prevent multiple loops through
				 * this code.
				 */
				do {
					if (np->p_parent == p)
						np->p_parentinsess = 0;
				} while ((np = np->p_pglinkf) != NULL);
				/*
				 * Call pgorphaned() to perform the
				 * orphan check/action.
				 */
				(void) pgorphaned(pgidp, B_TRUE);
			}
		}
	}
}


/*
 *
 * struct pid *pgfind_sess(pid_t pgid)
 * 	Locate the given process group.
 *
 * Calling/Exit State:
 *	If the process group exists, a pointer to it group is returned.
 *	Otherwise, NULL is returned.
 *
 * Remarks:
 *	The return value is stale unless the caller has made special
 *	arrangements.
 *
 */
struct pid *
pgfind(pid_t pgid)
{
	struct pid *pgidp;
	pidhash_t *pidhp;
	pl_t pl;

	pidhp = HASHPID(pgid);
	pl = LOCK(&pidhp->pidhash_mutex, PL_SESS);
	for (pgidp = pidhp->pidhash_link; pgidp != NULL;
	     pgidp = pgidp->pid_link) {
		if (pgidp->pid_id == pgid) {
			ASSERT(pgidp->pid_ref > 0);
			if (pgidp->pid_zpgref > 0 ||
			    pgidp->pid_pgprocs != NULL) {
				/* The process group exists. */
				UNLOCK(&pidhp->pidhash_mutex, pl);
				return pgidp;
			}
			UNLOCK(&pidhp->pidhash_mutex, pl);
			return (struct pid *)NULL;
		}
	}
	UNLOCK(&pidhp->pidhash_mutex, pl);
	return (struct pid *)NULL;
}


/*
 *
 * struct pid *pgfind_sess(register pid_t pgid)
 * 	Locate the given process group that must reside in the same session
 * 	as the caller.
 *
 * Calling/Exit State:
 *	The session in which the process group is to be located must be
 *	locked.  The s_mutex remains locked upon return.
 *
 *	If the process group exists within the session, a pointer to the
 *	process group is returned.  Otherwise, NULL is returned.
 *
 * Remarks:
 *	When a process exits, it removes itself from session and process
 *	group management chains.  This policy eliminates the check for zombies
 *	when determining whether or not a process group has been orphaned or
 *	unorphaned.
 *
 *      This function could be implemented to use the ID hash chains
 *      for speedy lookup if there are many process groups in the
 *      session.  However, this is an unusual case.
 *
 */
struct pid *
pgfind_sess(pid_t pgid)
{
	register struct pid *pgidp;
	register pidhash_t *pidhp;
	register sess_t *sp;
	pl_t pl;

	/*
	 * Search for process group assuming at least one non-zombie process.
	 */
	sp = u.u_procp->p_sessp;
	ASSERT(LOCK_OWNED(&sp->s_mutex));

	for (pgidp = sp->s_pgrps; pgidp != NULL; pgidp = pgidp->pid_pgrpf) {
		if (pgidp->pid_id == pgid)
			return pgidp;
	}

	/*
	 * The process group contains only zombie processes or processes
	 * in the midst of becoming zombie processes.  Otherwise, we would
	 * have found it in the previous loop.  Lookup the process group
	 * using the hash chains to see if the process group exists.
	 */
	pidhp = HASHPID(pgid);
	pl = LOCK(&pidhp->pidhash_mutex, PL_SESS);
	for (pgidp = pidhp->pidhash_link; pgidp != NULL;
	     pgidp = pgidp->pid_link) {
		if (pgidp->pid_id == pgid) {
			ASSERT(pgidp->pid_ref > 0);
			if (pgidp->pid_zpgref > 0 && pgidp->pid_zpgsess == sp) {
				/*
				 * The process group exists in the callers
				 * session.
				 */
				UNLOCK(&pidhp->pidhash_mutex, pl);
				ASSERT(pgidp->pid_pgprocs == NULL);
				return pgidp;
			}
			UNLOCK(&pidhp->pidhash_mutex, pl);
			return (struct pid *)NULL;
		}
	}
	UNLOCK(&pidhp->pidhash_mutex, pl);
	return (struct pid *)NULL;
}


/*
 *
 * void pgsignal(struct pid *pgidp, int sig)
 * 	Send the specified signal to all processes in the designated
 * 	process group.
 *
 * Calling/Exit State:
 *	The caller must hold the session containing the process group
 *	locked upon entry.  The session remains locked upon return.
 *
 */
void
pgsignal(struct pid *pgidp, int sig)
{
	register proc_t *p;

	ASSERT(pgidp->pid_pgprocs == NULL ||
	       LOCK_OWNED(&pgidp->pid_pgprocs->p_sessp->s_mutex));
	for (p = pgidp->pid_pgprocs; p != NULL; p = p->p_pglinkf) {
		(void)sigtoproc(p, sig, (sigqueue_t *)NULL);
	}
}
