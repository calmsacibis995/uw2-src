/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/pid.c	1.42"
#ident	"$Header: $"

#include <acc/mac/mac.h>
#include <acc/priv/privilege.h>
#include <util/types.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <proc/pid.h>
#include <proc/signal.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/uidquota.h>
#include <svc/systm.h>
#include <svc/errno.h>
#include <util/debug.h>
#include <util/var.h>
#include <mem/kmem.h>
#include <util/inline.h>
#include <util/debug.h>
#include <util/cmn_err.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <proc/session.h>
#include <util/plocal.h>
#include <util/metrics.h>
#include <proc/proc_hier.h>
#include <fs/file.h>
#include <fs/procfs/prdata.h>

extern void proc_cleanup_f(proc_t *);

int pid_hashsize = 64;	/* Should be configurable and must be a power of 2 */

/*
 * Macro to perform basic initialization of a process-ID (pid) structure.
 */
#define	PID_INIT(pidp, p) \
{ \
	(pidp)->pid_procref = 0; \
	(pidp)->pid_ref = 1; \
	(pidp)->pid_zpgref = 0; \
	(pidp)->pid_pgprocs = NULL; \
	(pidp)->pid_pgrpf = NULL; \
	(pidp)->pid_pgrpb = NULL; \
	(pidp)->pid_procp = (p); \
}


rwlock_t proc_list_mutex;	/* lock to protect the active chain */
proc_t	*practive;		/* active process chain */
pidhash_t	*pidhash;
struct pid *pid0p;	/* process 0 pid structure */

STATIC	pid_t		minpid;
STATIC	pid_t		nextpid;
union	procent		*procdir;
STATIC	union procent	*procentfree;
STATIC	uidres_t	exit_uidres;	/* used by pid_exit() */

/*
 *+ Lock protecting the pid hash.
 */
STATIC  LKINFO_DECL(pidlkinfo, "PP::pidhash_mutex", 0);

/*
 *+ Lock protecting the active process list.
 */
STATIC  LKINFO_DECL(proclistinfo, "PP::proc_list_mutex", 0);


/*
 *
 * void pid_alloc(struct pid *pidp_toinit, proc_t *p)
 * 	Allocate the next available process-ID, and initialize the pid
 * 	structure 'pidp_toinit' to reference the process: 'p'.
 *
 * Calling/Exit State:
 *	This function returns with the proc_list_mutex and s_mutex held.
 *	The proc_list_mutex is held in the "write" mode.
 *
 */
void
pid_alloc(struct pid *pidp_toinit, proc_t *p)
{
	register struct pid *pidp;
	register pidhash_t *pidhashp;
	register pid_t pid;
	union procent *pep;
	register pl_t pl;

	/* Perform basic initialization of the pid structure.  */

	PID_INIT(pidp_toinit, p);

	/* Allocate a pid */
	pid = nextpid;		/* taking advantage of atomic pid_t reads */

	pidhashp = HASHPID(pid);
	pl = LOCK(&pidhashp->pidhash_mutex, PL_SESS);
	for (pidp = pidhashp->pidhash_link; pidp != NULL; ) {
		if (pidp->pid_id == pid) {
			ASSERT(pidp->pid_ref > 0);
			UNLOCK(&pidhashp->pidhash_mutex, pl);
			if (++pid == MAXPID) 
				pid = minpid;
			pidhashp = HASHPID(pid);
			pl = LOCK(&pidhashp->pidhash_mutex, PL_SESS);
			pidp = pidhashp->pidhash_link;
		} else 
			pidp = pidp->pid_link;
	}

	pidp_toinit->pid_id = pid;
	pidp_toinit->pid_link = pidhashp->pidhash_link;
	pidhashp->pidhash_link = pidp_toinit;
	UNLOCK(&pidhashp->pidhash_mutex, pl);

	/* Update nextpid appropriately. */
	if (++pid == MAXPID)
		pid = minpid;
	nextpid = pid;		/* taking advantage of atomic pid_t writes */

	/*
	 * Add the process to the active list and do other initialization.
	 * Prior to acquiring the proc_list_mutex, we need to acquire the 
	 * s_mutex of the calling context's session.
	 */
	(void)LOCK(&u.u_procp->p_sess_mutex, PL_SESS);
        (void)LOCK(&u.u_procp->p_sessp->s_mutex, PL_SESS);
	UNLOCK(&u.u_procp->p_sess_mutex, PL_SESS);

	(void)RW_WRLOCK(&proc_list_mutex, PL_PROCLIST);
	ASSERT(procentfree != NULL);
	pep = procentfree;
	procentfree = procentfree->pe_next;
	pep->pe_proc = p;
	p->p_slot = pep - procdir;
	p->p_next = practive;
	p->p_prev = NULL;
	practive->p_prev = p;
	practive = p;
}


/*
 *
 * void pid_hold(struct pid *pidp)
 * 	Increment the non-driver reference count on the ID defined by the
 * 	given pid structure.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex.
 *
 */
void
pid_hold(register struct pid *pidp)
{
	register pidhash_t *pidhashp;
	register pl_t pl;

	pidhashp = HASHPID(pidp->pid_id);
	pl = LOCK(&pidhashp->pidhash_mutex, PL_SESS);
	pidp->pid_ref++;
	UNLOCK(&pidhashp->pidhash_mutex, pl);
}


/*
 *
 * void pid_rele(struct pid *pidp)
 * 	Decrement the non-driver reference count on the ID defined by the
 * 	given pid structure.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex.
 *
 */
void
pid_rele(register struct pid *pidp)
{
	register pidhash_t *pidhashp;
	register struct pid **pidpp;
	register pl_t pl;

	ASSERT(pidp != pid0p);
	ASSERT(pidp->pid_ref > 0);
	pidhashp = HASHPID(pidp->pid_id);
	pl = LOCK(&pidhashp->pidhash_mutex, PL_SESS);
	if (--pidp->pid_ref <= 0) {
		/*
		 * No non-driver references to this ID, remove the pid 
		 * object from its pid hash chain.
		 */
		for (pidpp = &pidhashp->pidhash_link; *pidpp != pidp;
		     pidpp = &(*pidpp)->pid_link) 
			;
		*pidpp = pidp->pid_link;
		/*
		 * If there are no driver references to the process 
		 * then release the pid structure.
		 */
		if (pidp->pid_procref == 0) {
			UNLOCK(&pidhashp->pidhash_mutex, pl);
			kmem_free((void *)pidp, sizeof(struct pid));
			return;
		}
	}
	UNLOCK(&pidhashp->pidhash_mutex, pl);
}


/*
 *
 * void pid_exit(proc_t *p)
 * 	Free the given process, and its associated IDs (process-ID, process
 * 	group-ID, and session-ID).  Also remove the process from its next-of-
 * 	kin resource orphan chain, and free its credentials.
 *
 * Calling/Exit State:
 *	No locks can be held on entry (must be at PLBASE).
 *	No locks are held on exit (at PLBASE).
 *
 * Remarks:
 *	The pid_procp pointer of the pid structure corresponding to the
 *	process must have already been cleared while holding the p_mutex
 *	lock of the process.
 *
 */
void
pid_exit(register proc_t *p)
{
	proc_t *nextp;
	proc_t *prevp;
	struct pid *pidp;
	sess_t *sp;
	
	ASSERT(KS_HOLD0LOCKS());
	ASSERT(p->p_holdcnt == 0);

	crfree(p->p_cred);		/* release the process credentials */

	pidp = p->p_pidp;

	ASSERT(pidp->pid_procp == NULL);
	ASSERT(p != proc_sys && p != proc_init);

	/* Release the process-ID reference. */
	pid_rele(pidp);

	/*
	 * Remove the process from its process group and session.
	 * Note that it is not necessary to acquire p_sess_mutex,
	 * since no other LWPs exist to change the session state.
	 */
	pidp = p->p_pgidp;
	sp = p->p_sessp;

	(void)LOCK(&sp->s_mutex, PL_SESS);

	if (--pidp->pid_zpgref == 0 && pidp->pid_pgprocs == NULL) {
		/*
		 * No processes (zombie or non-zombie) remain in the
		 * process group.
		 */
		pid_rele(pidp);
	}
	if (--sp->s_ref == 0) {
		/*
		 * Free the session-ID.
		 * Is any LWP is performing syXXX op using s_vp?
		 */
		pidp = sp->s_sidp;
		if (sp->s_cttyref == 0) {
			/*  Free the session.  */
			UNLOCK(&sp->s_mutex, PLBASE);
			LOCK_DEINIT(&sp->s_mutex);
			kmem_free((void *)sp, sizeof(sess_t));
		} else
			UNLOCK(&sp->s_mutex, PLBASE);
		pid_rele(pidp);			/* release session-ID ref */
	} else
		UNLOCK(&sp->s_mutex, PLBASE);

	/*
	 *  Remove process from of the process list, and 
	 *  from the next-of-kin list.
	 */
	(void)RW_WRLOCK(&proc_list_mutex, PL_PROCLIST);

	/* Remove the process from the active list */

	nextp = p->p_next;
	prevp = p->p_prev;
	if (prevp == NULL)		/* first process in chain */
		practive = nextp;
	else 				/* not first process in chain */
		prevp->p_next = nextp;

	/* Since process 0 cannot exit, nextp cannot be NULL. */
	ASSERT(nextp != NULL);
	nextp->p_prev = prevp;

	/*  Return the procdir slot. */
	procdir[p->p_slot].pe_next = procentfree;
	procentfree = &procdir[p->p_slot];

	/* Remove the process from the next-of-kin orphan chain. */
	nextp = p->p_nextorph;
	prevp = p->p_prevorph;
	if (prevp == NULL) 
		p->p_nextofkin->p_orphan = nextp;
	else 
		prevp->p_nextorph = nextp;
	if (nextp != NULL)
		nextp->p_prevorph = prevp;

	/*
	 * While still holding the lock, call MET_PROC_INUSE to
	 * decrement MET_PROC_CNT and update any other proc metrics.
	 */
	MET_PROC_INUSE(-1);
	ASSERT(MET_PROC_CNT >= (long)0);

	RW_UNLOCK(&proc_list_mutex, PLBASE);

	/*
	 * At this point, the process has only one LWP.
	 */
	MET_LWP_INUSE(-1);

	uidquota_decr(p->p_uidquotap, &exit_uidres);

	LOCK_DEINIT(&p->p_mutex);
	LOCK_DEINIT(&p->p_sess_mutex);
	LOCK_DEINIT(&p->p_seize_mutex);
	LOCK_DEINIT(&p->p_dir_mutex);
	LOCK_DEINIT(&p->p_squpdate);
	FDT_DEINIT(p);			/* de-init proc fd-table lock */

	proc_cleanup_f(p);

	kmem_free((void *)p, sizeof(struct proc));
}


/*
 *
 * proc_t *prfind(pid_t pid)
 *	Find process given a process-ID.
 *
 * Calling/Exit State:
 *	The function returns a pointer to the process with p_mutex
 *	locked if found.  Otherwise, NULL is returned.
 *	It is assumed that the given process-ID is within [0..PID_MAX-1].
 *
 * Remarks:
 *	This function assumes that the caller knows the appropriate pl to
 *	UNLOCK the p_mutex at when pointer to a process is returned to the
 *	caller.
 *
 */
proc_t *
prfind(register pid_t pid)
{
	register pidhash_t *pidhashp;
	register struct pid *pidp;
	register struct proc *p;
	register pl_t pl;

	ASSERT(pid >= 0);
	pidhashp = HASHPID(pid);
	pl = LOCK(&pidhashp->pidhash_mutex, PL_SESS);
	/*
	 * Search the appropriate hash chain for the pid and
	 * return a pointer to its pid structure, if successful.
	 */
	for (pidp = pidhashp->pidhash_link; pidp; pidp = pidp->pid_link) {
		if (pidp->pid_id == pid) {
			ASSERT(pidp->pid_ref > 0);
			if ((p = (struct proc *)pidp->pid_procp) == NULL) {
				UNLOCK(&pidhashp->pidhash_mutex, pl);
				return ((proc_t *)NULL); 
			}
			/*
			 * While still holding pidhash_mutex, acquire
			 * the p_mutex lock of the process to close the
			 * race in which a process is being freeproc()ed.
			 */
			(void)LOCK(&p->p_mutex, PLHI);
			if (pidp->pid_procp != NULL) {
				/* The process still exists. */
				UNLOCK(&pidhashp->pidhash_mutex, PLHI);
				return (p);
			}
			/*
			 * We lost the race.  The process is freeproc()ed.
			 */
			UNLOCK(&p->p_mutex, PL_SESS);
			UNLOCK(&pidhashp->pidhash_mutex, pl);
			return ((proc_t *)NULL); 
		}
	}
	UNLOCK(&pidhashp->pidhash_mutex, pl);
	return ((proc_t *)NULL); 
}


/*
 *
 * proc_t *prfind_sess(pid_t pid)
 * 	Locate the given non-zombie process in the same session as the
 * 	caller.
 *
 * Calling/Exit State:
 *	The session in which the non-zombie process is to be located must
 *	be locked by the caller.  The session remains locked upon return.
 *	The function returns a pointer to the process if the 
 *	process exists within the session.  Otherwise, NULL is returned.
 *
 * Remarks:
 *	This function could be implemented to use the ID hash chains
 *	for speedy lookup.  Such an implementation is clearly a win
 *	if there are enough processes in the session to warrant the
 *	cost of the extra lock round trip.
 *
 */
proc_t *
prfind_sess(register pid_t pid)
{
	register struct pid *pgidp;
	register proc_t *p;

	/*
	 * Start the search with the first non-zombie process group in
	 * the session.
	 */
	ASSERT(LOCK_OWNED(&u.u_procp->p_sessp->s_mutex));
	pgidp = u.u_procp->p_sessp->s_pgrps;
	do {
		p = pgidp->pid_pgprocs;
		do {
			if (p->p_pidp->pid_id == pid)
				return (p);
		} while ((p = p->p_pglinkf) != NULL);
	} while ((pgidp = pgidp->pid_pgrpf) != NULL);
	return ((proc_t *)NULL);
}

/*
 *
 * int pid_next_entries(void *prcp, int start, int count, idbuf_t *list,
 *	  int *nextp)
 *	Return a set of process ids and process directory slot numbers
 *	found in the proc table starting at a given slot.
 *
 * Calling/Exit State:
 *	No locks are held on entry and none on return; the
 *	proc_list_mutex lock is acquired and released during execution.
 *	Beginning at slot number "start", up to "count" pid/slot pairs
 *	are returned to the caller in the array of "idbuf_t" structures
 *	to which "list" refers.  The next unused slot is returned in the
 *	int to which "nextp" refers.  The function returns the number of
 *	entries that were returned in "list"; if zero, "*nextp" is set to
 *	"start".  If the search ran to the end of the proc table, *nextp
 * 	will contain v.v_proc.
 *
 * Remarks:
 *	proc_list_mutex is the only lock we need to acquire.  In
 *	particular the p_mutex locks of the target processes need
 *	not be acquired because the only datum we require from each
 *	proc structure is the pid, and this cannot change during the
 *	lifetime of the process (nor can the process itself disappear
 *	as long as p_mutex is held).
 *
 *	The argument "prcp" should be NULL but is ignored in any case.
 *	It exists only so that this routine and lwpid_next_entries()
 *	can have a common interface.
 */
/* ARGSUSED */
int
pid_next_entries(void *prcp, int start, int count, idbuf_t *list, int *nextp)
{
	register int i, n;
	register proc_t *p;
	register union procent *pep;
	register pl_t pl;

	ASSERT(start >= 0);
	pl = RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (i = start, n = 0; i < v.v_proc && n < count; i++) {
		if ((pep = procdir[i].pe_next) == NULL
		  || (pep > &procdir[0] && pep < &procdir[v.v_proc]))
			continue;
		p = procdir[i].pe_proc;
		if (mac_installed) {
			(void) LOCK(&p->p_mutex, PLHI);
			/* The process is being freeproc()ed. */
			if (p->p_pidp->pid_procp != NULL) {
				if (!(MAC_ACCESS(MACDOM, CRED()->cr_lid, 
				     p->p_cred->cr_lid) && 
				     pm_denied(CRED(), P_MACREAD))) {
					list[n].id_id = p->p_pidp->pid_id;
					list[n].id_nodeid = i;
					list[n].id_slot = i;
					n++;
				}
			}
			UNLOCK(&p->p_mutex, PL_PROCLIST);
		} else {
			list[n].id_id = p->p_pidp->pid_id;
			list[n].id_nodeid = i;
			list[n].id_slot = i;
			n++;
		}
	}
	*nextp = i;		
	RW_UNLOCK(&proc_list_mutex, pl);
	return n;
}

/*
 *
 * int lwpid_next_entries(prcommon_t *prcp, int start, int count,
 *	  idbuf *list, int *nextp)
 *	Return a set of LWP ids and LWP node ids found in the p_lwpdir
 *	array of the process associated with the given prcommon structure.
 *
 * Calling/Exit State:
 *	No locks are held on entry and none on return; the p_mutex lock
 *	of the target process is acquired and released during execution.
 *	Beginning at LWP slot "start", up to "count" lwpid/node-id pairs
 *	are returned to the caller in the array of "idbuf_t" structures
 *	to which "list" refers.  The next unused LWP slot is returned in the
 *	int to which "nextp" refers.  The function returns the number of
 *	entries that were returned in "list"; if zero, "*nextp" is set to
 *	"start".  If the search ran to the end of the p_lwpdir array, *nextp
 * 	will contain the first unused LWP slot.
 */
int
lwpid_next_entries(prcommon_t *prcp, int start, int count,
		   idbuf_t *list, int *nextp)
{
	register int i, n, imax;
	register proc_t *p;
	register lwp_t *lwp;

	ASSERT(start >= 0);
	if (!pr_p_mutex(prcp))
		return 0;
	p = prcp->prc_proc;
	ASSERT(p != NULL);
	ASSERT(LOCK_OWNED(&p->p_mutex));
	if  (MAC_ACCESS(MACDOM, CRED()->cr_lid, p->p_cred->cr_lid) 
	     && pm_denied(CRED(), P_MACREAD)) { 
		UNLOCK(&p->p_mutex, PLBASE);
		return 0;
	}
	imax = p->p_nlwpdir;
	for (i = start, n = 0; i < imax && n < count; i++) {
		lwp = p->p_lwpdir[i];
		if (lwp == NULL)
			continue;
		list[n].id_id = i+1;	/* LWP ids start at 1 */
		list[n].id_nodeid = p->p_slot | ((i+1)<<13);
		list[n].id_slot = i;
		n++;
	}
	*nextp = i;		
	UNLOCK(&p->p_mutex, PLBASE);
	return n;
}

/*
 *
 * proc_t *pid_next_entry(int *slotp)
 *	Return a pointer to the next process in the range
 *	procdir[*slotp], procdir[v.v_proc - 1], inclusive.
 *
 * Calling/Exit State:
 *	If a process is found within the given range, a
 *	pointer to the process is returned with its p_mutex
 *	held, and '*slotp' is set to the corresponding slot
 *	number.  Otherwise NULL is returned.
 *
 * Remarks:
 *	The parameter 'slotp' is an in/out argument.
 *
 */
proc_t *
pid_next_entry(int *slotp)
{
	union procent *pep;
	struct proc *p;
	pl_t pl;
	int slot;

	ASSERT(*slotp >= 0);

	pl = RW_RDLOCK(&proc_list_mutex, PL_PROCLIST);
	for (slot = *slotp; slot < v.v_proc; slot++) {
		pep = procdir[slot].pe_next;
		if (pep != NULL &&
		    (pep < &procdir[0] || pep >= &procdir[v.v_proc])) {
			/*
			 * The process directory slot is not on the
			 * freelist of unused process directory slots,
			 * it must be a pointer to a proc structure.
			 *
			 * While holding proc_list_mutex, acquire the
			 * p_mutex lock of the process to close the race
			 * in which a process is being simultaneously
			 * freeproc'ed.
			 */
			p = procdir[slot].pe_proc;
			(void)LOCK(&p->p_mutex, PLHI);
			if (p->p_pidp->pid_procp != NULL) {
				/*
				 * The process has not yet been freeproc'ed.
				 * Return with p_mutex held at PLHI.
				 */
				RW_UNLOCK(&proc_list_mutex, PLHI);
				*slotp = slot;
				return (p); 
			} else {
				/* Lost the race with freeproc(). */
				UNLOCK(&p->p_mutex, PL_PROCLIST);
			}
		}

	}

	RW_UNLOCK(&proc_list_mutex, pl);
	return (NULL); 
}


/*
 *
 * void *proc_ref(void)
 * 	Establish a reference upon the currently executing process.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex.
 *
 * Remarks:
 * 	This function is to be called only by device drivers.
 *
 */
void *
proc_ref(void)
{
	register struct pid *pidp;
	register pidhash_t *pidhashp;
	register pl_t pl;

	pidp = u.u_procp->p_pidp;
	ASSERT(pidp->pid_ref > 0 && pidp->pid_procp != NULL);

	pidhashp = HASHPID(pidp->pid_id);
	/*
	 * NOTE: pidhash_mutex is normally acquired at PL_SESS.  We
	 *	 acquire it here at PLHI so that device drivers can
	 *	 call proc_ref(D3DK) at any PL level.
	 */
	pl = LOCK(&pidhashp->pidhash_mutex, PLHI);
	pidp->pid_procref++;
	UNLOCK(&pidhashp->pidhash_mutex, pl);

	return ((void *)pidp);
} 


/*
 * boolean_t proc_traced(void *void_pidp)
 *	Determine if the specified process referenced through proc_ref(D3DK)
 *	call is being traced.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex.
 *	The function returns B_TRUE if the specified process
 *	exists. Otherwise, B_FALSE is returned.
 *
 * Remarks:
 *	This function is to be called only by device drivers.
 */
boolean_t
proc_traced(void *void_pidp)
{
#define PTRACED(procp)		((procp)->p_flag &(P_TRC|P_PROCTR|P_PROPEN))

	register struct pid *pidp;
	register pidhash_t *pidhashp;
	register pl_t pl;

	pidp = (struct pid *)void_pidp;
	ASSERT(pidp->pid_procref > 0);
	pidhashp = HASHPID(pidp->pid_id);

	pl = LOCK(&pidhashp->pidhash_mutex, PLHI);
	if (PTRACED((proc_t *)pidp->pid_procp)) {
		UNLOCK(&pidhashp->pidhash_mutex, pl);
		return(B_TRUE);
	}

	UNLOCK(&pidhashp->pidhash_mutex, pl);
	return(B_FALSE);
}


/*
 *
 * void *proc_ref_pp(struct pid* pidp)
 * 	Establish a reference upon the currently executing process.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex.
 *
 * Remarks:
 * 	Used by shutdown() 
 *
 */
void *
proc_ref_pp(struct pid  *pidp)
{
	register pidhash_t *pidhashp;
	register pl_t pl;

	ASSERT(pidp->pid_ref > 0 && pidp->pid_procp != NULL);

	pidhashp = HASHPID(pidp->pid_id);
	pl = LOCK(&pidhashp->pidhash_mutex, PLHI);
	pidp->pid_procref++;
	UNLOCK(&pidhashp->pidhash_mutex, pl);

	return ((void *)pidp);
} 


/*
 *
 * int proc_signal(void *void_pidp, int sig)
 * 	Post the specified signal to the designated process previously
 * 	referenced via a proc_ref(D3DK) call.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex and p_mutex of
 *	the designated process(if exists).  This function returns 0 if 
 *	the designated process exists and we post the signal.  Otherwise, 
 *	-1 is returned.
 *
 * Remarks:
 * 	This function is to be called only by device drivers.
 *
 */
int
proc_signal(void *void_pidp, int sig)
{
	register struct pid *pidp;
	register pidhash_t *pidhashp;
	register proc_t *p;
	register pl_t pl;

	if (sigismember(&sig_jobcontrol, sig))
		return (-1);

	pidp = (struct pid *)void_pidp;
	pidhashp = HASHPID(pidp->pid_id);
	/*
	 * We acquire pidhash_mutex to prevent races with freeproc().
	 * NOTE: pidhash_mutex is normally acquired at PL_SESS.  We
	 *	 acquire it here at PLHI so that device drivers can
	 *	 call proc_signal(D3DK) at any PL level.
	 */
	pl = LOCK(&pidhashp->pidhash_mutex, PLHI);
	ASSERT(pidp->pid_procref > 0);
	if ((p = (proc_t *)pidp->pid_procp) != NULL) {
		/*
		 * While still holding pidhash_mutex, acquire the p_mutex lock
		 * of the process to close the race in which a process is being
		 * freeproc()ed at exactly the same time as we're trying to
		 * signal it here.
		 */
		(void)LOCK(&p->p_mutex, PLHI);
		if (pidp->pid_procp != NULL && p->p_nlwp > 0) {
			/*
			 * The process still exists (and is not a zombie).
			 * Holding p_mutex protects it from being freeproc()ed.
			 */
			UNLOCK(&pidhashp->pidhash_mutex, PLHI);
			(void)sigtoproc_l(p, sig, (sigqueue_t *)NULL);
			UNLOCK(&p->p_mutex, pl);
			return (0);
		}
		/*
		 * We lost the race.  The process is being freeproc()ed, though
		 * the code doing this is now spinning on pidhash_mutex.
		 */
		UNLOCK(&p->p_mutex, PLHI);
	}
	UNLOCK(&pidhashp->pidhash_mutex, pl);
	return (-1);
}


/*
 * boolean_t proc_valid(void *void_pidp)
 *	Determine if the specified process referenced through proc_ref(D3DK)
 *	call has exited.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex.
 *	The function returns B_TRUE if the specified process
 *	exists. Otherwise, B_FALSE is returned.
 *
 * Remarks:
 *	This function is to be called only by device drivers.
 */
boolean_t
proc_valid(void *void_pidp)
{
        register struct pid *pidp;
        register pidhash_t  *pidhashp;
        register pl_t  pl;

        pidp = (struct pid *)void_pidp;
        ASSERT(pidp->pid_procref > 0);
        pidhashp = HASHPID(pidp->pid_id);

        pl = LOCK(&pidhashp->pidhash_mutex, PLHI);
        if ((proc_t *)pidp->pid_procp == NULL) {
                UNLOCK(&pidhashp->pidhash_mutex, pl);
                return(B_FALSE);
        }

        UNLOCK(&pidhashp->pidhash_mutex, pl);
        return(B_TRUE);
}

/*
 * boolean_t proc_valid_pp(void *void_pidp)
 *	Determine if the specified process referenced through proc_ref_pp()
 *	call has exited.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex.
 *	The function returns B_TRUE if the specified process
 *	exists. Otherwise, B_FALSE is returned.
 *
 * Remarks:
 *	This function is to be called only by shutdown()
 */
boolean_t
proc_valid_pp(struct pid* pidp)
{
        register pidhash_t  *pidhashp;
        register pl_t  pl;

        ASSERT(pidp->pid_procref > 0);
        pidhashp = HASHPID(pidp->pid_id);
        pl = LOCK(&pidhashp->pidhash_mutex, PLHI);

        if ((proc_t *)pidp->pid_procp == NULL || 
		((proc_t*)(pidp->pid_procp)->p_nlwp) == 0) {

                UNLOCK(&pidhashp->pidhash_mutex, pl);
                return(B_FALSE);
        }
        UNLOCK(&pidhashp->pidhash_mutex, pl);
        return(B_TRUE);
}



/*
 *
 * void proc_unref(void *void_pidp)
 * 	Remove the reference to the process-ID (and associated process)
 * 	previously established by a proc_ref(D3DK) call.
 *
 * Calling/Exit State:
 *	This function acquires and releases pidhash_mutex.
 *
 * Remarks:
 * 	This function is to be called only by device drivers.
 *
 */
void
proc_unref(void *void_pidp)
{
	register struct pid *pidp;
	register pidhash_t *pidhashp;
	register pl_t pl;

	pidp = (struct pid *)void_pidp;
	pidhashp = HASHPID(pidp->pid_id);
	ASSERT(pidp->pid_procref > 0);
	/*
	 * NOTE: pidhash_mutex is normally acquired at PL_SESS.  We
	 *	 acquire it here at PLHI so that device drivers can
	 *	 call proc_unref(D3DK) at any PL level.
	 */
	pl = LOCK(&pidhashp->pidhash_mutex, PLHI);
	if (--pidp->pid_procref == 0 && pidp->pid_ref == 0) {
		/*
		 * There are no non-driver references to this pid structure,
		 * nor does the pid structure track an active ID (it has
		 * already been removed from the pidhash hash chains).
		 * Now that the last reference is gone, we can return
		 * the pid structure to the free memory pool.
		 */
		UNLOCK(&pidhashp->pidhash_mutex, pl);
		kmem_free((void *)pidp, sizeof(struct pid));
	} else
		UNLOCK(&pidhashp->pidhash_mutex, pl);
}


/*
 *
 * void pid_init(proc_t *procp0p)
 * 	This function initializes the pid management system and pid 
 *	structure for process 0.  This function should only be called 
 *	once during system initialization.
 *
 * Calling/Exit State:
 *	No locks held on entry and no locks held on exit.
 *
 */
void
pid_init(proc_t *proc0p)
{
	register i;

	ASSERT(KS_HOLD0LOCKS());

	pidhash = (struct pidhash *)
	  kmem_zalloc(sizeof(struct pidhash)*pid_hashsize, KM_NOSLEEP);

	procdir = (union procent *)
	  kmem_alloc(sizeof(union procent)*v.v_proc, KM_NOSLEEP);
	pid0p = (struct pid *)kmem_zalloc(sizeof(struct pid), KM_NOSLEEP);

	if (pidhash == NULL || procdir == NULL || pid0p == NULL) {
		/*
		 *+ Out of memory during process management initialization. 
		 *+ Corrective action: Check you configuration.
		 */
		cmn_err(CE_PANIC, "pid_init(): Could not allocate space \n");
	}

	for (i = 0; i < pid_hashsize; i++) {
		LOCK_INIT(&pidhash[i].pidhash_mutex, PIDHASH_HIER, PL_SESS,
			  &pidlkinfo, KM_NOSLEEP);
		pidhash[i].pidhash_link = NULL;
	}

	procentfree = &procdir[1];
	for (i = 1; i < v.v_proc - 1; i++)
		procdir[i].pe_next = &procdir[i+1];
	procdir[i].pe_next = NULL;

	RW_INIT(&proc_list_mutex, PROCLIST_HIER, PROCLIST_MINIPL,
		&proclistinfo, KM_SLEEP);

	PID_INIT(pid0p, proc0p);
	pid0p->pid_ref = 3;	/* 1 ref each for process, session, and pgrp */
	pid0p->pid_pgprocs = proc0p;
	pid0p->pid_pgrpf = NULL;
	pid0p->pid_pgrpb = pid0p;
	proc0p->p_pgid = 0;
	proc0p->p_pglinkf = NULL;
	proc0p->p_pglinkb = proc0p;
	proc0p->p_pgidp = pid0p;
	pid0p->pid_id = 0;
	pid0p->pid_link = NULL;
	HASHPID(0)->pidhash_link = pid0p;

	proc0p->p_pidp = pid0p;
	proc0p->p_prev = NULL;
	proc0p->p_next = NULL;
	proc0p->p_slot = 0;
	proc0p->p_flag |= P_PGORPH;
	procdir[0].pe_proc = proc0p;
	practive = proc0p;

	/*
	 * Call MET_PROC_INUSE() to initialize the count of
	 * processes (MET_PROC_CNT) to one and update any
	 * other proc metrics.  Also call MET_LWP_INUSE() to
	 * initialize the count of lwps that are in use. 
	 */
	MET_PROC_INUSE(1);
	MET_LWP_INUSE(1);
	minpid = 1;
	nextpid = 1;

	/*
	 * Initialize the exit_uidres structure used by pid_exit().
	 * We do this initialization here to be more portable in the
	 * face of new fields being added to uidres_t objects.
	 */
	exit_uidres.ur_prcnt = 1;
	exit_uidres.ur_lwpcnt = 1;
}
