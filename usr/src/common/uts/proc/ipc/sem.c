/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/ipc/sem.c	1.19"

/*
 * SystemV IPC Semaphore Facility.
 */

#include <util/types.h>
#include <acc/audit/audit.h>
#include <acc/priv/privilege.h>
#include <acc/mac/mac.h>
#include <util/ksynch.h>
#include <util/debug.h>
#include <util/param.h>
#include <svc/systm.h>
#include <svc/clock.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/time.h>
#include <acc/dac/acl.h>
#include <proc/ipc/ipcsec.h>
#include <proc/ipc/ipc.h>
#include <proc/ipc/sem.h>
#include <proc/user.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <util/cmn_err.h>
#include <util/list.h>
#include <util/ghier.h>
#include <util/metrics.h>
#include <util/plocal.h>
#include <proc/proc_hier.h>
#include <mem/kmem.h>
#ifdef CC_PARTIAL
#include <acc/mac/cca.h>
#endif

/*
 * Lock Ordering (in ascending order):
 *
 *	semexit_mutex		Reader/Writer sleep lock which serializes
 *				semexit() with the SETVAL/SETALL options
 *				to semctl().  Semexit() acquires this lock
 *				in shared mode, semctl() SETVAL/SETALL acquires
 *				this lock in exclusive mode.
 *
 *	semdir_mutex		Locks IPC lookups and additions(removals)
 *				to(from) the semaphore ipc directory.
 *
 *	ksem_mutex		Per semid_ds structure lock to mediate
 *				access to a semaphore set and associated
 *				state information.
 *
 *	semunp_mutex		Locks additions(removals) to(from) the linked
 *				list of active sem_undo structures (semunp).
 *
 *	un_mutex		Per sem_undo structure lock to mediate
 *				access to the per process undo list.
 *
 * Nesting of lock acquistions is rarely more than two deep and
 * is as follows:
 *
 *	Nesting:			Functions:
 *	-------				---------
 *	semdir_mutex			semconv()
 *	  ksem_mutex
 *
 *	ksem_mutex			semaoe()
 *	  un_mutex
 *
 *	semexit_mutex(exclusive)	semctl() SETVAL/SETALL options
 *	  ksem_mutex
 *	    semunp_mutex
 *	      un_mutex
 *
 *	semexit_mutex(shared)		semexit()
 *	  ksem_mutex
 */	

STATIC LKINFO_DECL(semexit_lkinfo, "PI::semexit_mutex", 0);
STATIC rwsleep_t	semexit_mutex;

STATIC LKINFO_DECL(semdir_lkinfo, "PI::semdir_mutex", 0);
STATIC rwlock_t		semdir_mutex;
ipcdir_t		semdir;

#define	SEMDIR_RDLOCK()		RW_RDLOCK_PLMIN(&semdir_mutex)
#define	SEMDIR_WRLOCK()		RW_WRLOCK_PLMIN(&semdir_mutex)
#define	SEMDIR_UNLOCK(pl)	RW_UNLOCK_PLMIN(&semdir_mutex, (pl))

STATIC int		semalloc(ipc_perm_t **);
STATIC void		semdealloc(ipc_perm_t *);

STATIC ipcops_t		semops = {semalloc, semdealloc};
STATIC ipcdata_t	semdata = {&semdir, &semops};


/*
 * Lock info structure for per semid_ds ksem_mutex.
 */
STATIC LKINFO_DECL(ksem_lkinfo, "PI::ksem_mutex", 0);

/*
 * List of active undo structures, and associated mutex.
 */
STATIC	LKINFO_DECL(semunp_lkinfo, "PI::semunp_mutex", 0);
STATIC	list_t	semunp;			/* head of active undo list */
STATIC	lock_t	semunp_mutex;		/* lock for active undo list */
STATIC	size_t	semundosz;		/* size of sem_undo structure */

/* seminfo is defined in proc/ipc/sem.cf */
extern struct seminfo seminfo;		/* configuration info structure */


/* Convienient macros: */
#define	SEMID_TO_SLOT(semid)	IPCID_TO_SLOT((semid), seminfo.semmni)
#define	SEMID_TO_SEQ(semid)	IPCID_TO_SEQ((semid),  seminfo.semmni)
#define	SLOT_TO_SEMID(slot, seq) SLOT_TO_IPCID((slot), (seq), seminfo.semmni)


/* Argument vectors for the various flavors of semsys(). */
#define	SEMCTL	0
#define	SEMGET	1
#define	SEMOP	2

/*
 * Format of arguments to the semaphore IPC system calls.
 */
union semsysa {
	int			opcode;
	struct semctla {
		int		opcode;		/* SEMCTL */
		int		semid;
		uint		semnum;
		int		cmd;
		int		arg;
	} semctla;
	struct semgeta {
		int		opcode;		/* SEMGET */
		key_t		key;
		int		nsems;
		int		semflg;
	} semgeta;
	struct semopa {
		int		opcode;		/* SEMOP */
		int		semid;
		struct sembuf	*sops;
		uint		nsops;
	} semopa;
};


/*
 * Support functions.
 */

/*
 * int semaoe(short val, int semid, ushort num)
 *	Create or update Adjust On Exit entry.
 *
 * Calling/Exit State:
 *	The semaphore descriptor associated with 'semid' must be
 *	locked by the caller, it remains locked on return.
 *
 *	Arguments:
 *		val	operation value to be adjusted on exit
 *		semid	semaphore id
 *		num	semaphore number
 */
STATIC int
semaoe(short val, int semid, ushort num)
{
	struct sem_undo *sup = u.u_procp->p_semundo;
	struct undo	*unp;		/* ptr to entry to update */
	int		i;		/* loop control */
	int		found = 0;	/* matching entry found flag */

	/* The sem_undo structure for the process
	 * must have been previously allocated!
	 */
	ASSERT(u.u_procp->p_semundo != NULL);

	if (val == 0)
		return 0;
	if (val > seminfo.semaem || val < -seminfo.semaem)
		return ERANGE;

	FSPIN_LOCK(&sup->un_mutex);

	/*
	 * Search for an existing undo entry for this semaphore.
	 * Undo entries are sorted by semaphore id and number.
	 */
	for (unp = sup->un_ent, i = 0; i < sup->un_cnt; i++) {
		if (unp->un_id < semid
		  || (unp->un_id == semid && unp->un_num < num)) {
			unp++;
			continue;
		}
		if (unp->un_id == semid && unp->un_num == num)
			found = 1;
		break;
	}

	/* Need to create a new undo entry for this semaphore. */
	if (!found) {
		struct	undo	 *unp2;

		if (sup->un_cnt >= seminfo.semume) {
			FSPIN_UNLOCK(&sup->un_mutex);
			return EINVAL;		/* exceeded per-process limit */
		}

		/*
		 * Bump the count of undo entries, and
		 * push down the entries after 'unp'.
		 */
		unp2 = &sup->un_ent[sup->un_cnt++];
		while (unp2-- > unp)
			*(unp2 + 1) = *unp2;		/* struct copy */
		unp->un_id = semid;
		unp->un_num = num;
		unp->un_aoe = -val;
		FSPIN_UNLOCK(&sup->un_mutex);
		return 0;
	}

	/* Use an existing undo entry. */
	unp->un_aoe -= val;
	if (unp->un_aoe > seminfo.semaem || unp->un_aoe < -seminfo.semaem) {
		unp->un_aoe += val;			/* undo the undo! */
		FSPIN_UNLOCK(&sup->un_mutex);
		return ERANGE;
	}

	if (unp->un_aoe == 0) {
		/*
		 * The adjust on exit value is now zero,
		 * so the undo entry is not needed.
		 * Decrement the count of undo entries and
		 * pull up any undo entries after this one.
		 */
		struct undo *unp2 = &sup->un_ent[--(sup->un_cnt)];
		while (unp++ < unp2)
			*(unp - 1) = *unp;		/* struct copy */
	}
	FSPIN_UNLOCK(&sup->un_mutex);
	return 0;
}

/*
 * void semunrm(int semid, ushort low, ushort high)
 *	Undo entry remover.
 *
 * Calling/Exit State:
 *	The corresponding semid_ds structure must be locked by
 *	the caller of this function to assure that no new undo
 *	entries for the semaphore set are created while this
 *	function is executing.
 *
 * Arguments:
 *	semid	semaphore id
 *	low	lowest semaphore being changed in set
 *	high	highest semaphore being changed in set
 *
 * Notes:
 *	This routine is called to clear all undo entries for a set of
 *	semaphores that are being removed from the system (IPC_RMID),
 *	or are being reset by SETVAL or SETALL commands to semctl().
 */
STATIC void
semunrm(int semid, ushort low, ushort high)
{
	struct	sem_undo	*sup;	/* ptr to current entry */
	struct	undo		*unp;	/* ptr to undo entry */
	int			i, j;	/* loop control */

	ASSERT(KS_HOLD1LOCK());

	(void)LOCK_PLMIN(&semunp_mutex);
	sup = (struct sem_undo *)semunp.flink;		/* active undo list */
	while (sup != (struct sem_undo *)&semunp) {
		/* Search through current structure for matching entries. */
		FSPIN_LOCK(&sup->un_mutex);
		for (unp = sup->un_ent, i = 0; i < sup->un_cnt; ) {
			if (semid < unp->un_id)
				break;
			if (semid > unp->un_id || low > unp->un_num) {
				unp++;
				i++;
				continue;
			}
			if (high < unp->un_num)
				break;
			/*
			 * Remove this entry, pull up any
			 * following entries.
			 */
			for (j = i; ++j < sup->un_cnt; )
				sup->un_ent[j - 1] = sup->un_ent[j];
			sup->un_cnt--;
		}

		/*
		 * Move on to the next sem_undo structure.
		 * Note that the linked list of structures
		 * is held stable by semunp_mutex.
		 */
		FSPIN_UNLOCK(&sup->un_mutex);
		sup = (struct sem_undo *)sup->un_np.flink;
	}
	UNLOCK_PLMIN(&semunp_mutex, PLMIN);
}

/*
 * void semundo(const struct sembuf *op, int n, int semid, struct ksemid_ds *sp)
 *	Undo work done up to finding an operation that can't be done.
 *
 * Calling/Exit State:
 *	The semaphore descriptor 'sp' must be locked by the caller, it
 *	remains locked on return.
 *
 * Arguments:
 *	op	pointer to first operation that was done
 *	n	of operations that were done
 *	semid	semaphore id
 *	sp	pointer to semaphore descriptor
 *
 * Notes:
 *	This function is only called from semop().
 */
STATIC void
semundo(const struct sembuf *op, int n, int semid, struct ksemid_ds *sp)
{
	struct	sem	* const semp = sp->ksem_ds.sem_base;

	ASSERT(n >= 0);
	ASSERT(LOCK_OWNED(&sp->ksem_mutex) && KS_HOLD1LOCK());

	for (op += n - 1; n--; op--) {
		if (op->sem_op != 0) {
			(semp + op->sem_num)->semval -= op->sem_op;
			if (op->sem_flg & SEM_UNDO)	/* undo the undo! */
				semaoe(-op->sem_op, semid, op->sem_num);
		}
	}
}

/*
 * int semconv(const int semid, struct ksemid_ds **spp, const int hold)
 * 	Convert user supplied semid into a pointer to the associated
 *	semaphore descriptor.
 *
 * Calling/Exit State:
 *	No spin locks can be held on entry.  This function can
 *	block.
 *	On success, this function returns 0 with the semaphore
 *	descriptor lock held.
 *	On failure, a non-zero errno is returned, with no locks held.
 *
 *	Arguments:
 *		semid	semaphore identifier
 *		spp	out arg: corresponding semaphore descriptor
 *		hold	If true, acquire the directory lock in
 *			exclusive mode and keep the directory lock
 *			held on return.
 *			This is only used by IPC_RMID.  Unfortunate,
 *			but resolves a race when removing a descriptor.
 * Notes:
 *	This function cannot be static since it is called from ipcdac.c.
 */
int
semconv(const int semid, struct ksemid_ds **spp, const int hold)
{
	struct	ksemid_ds *sp;		/* ptr to semaphore descriptor */
	ipcdirent_t *dp;		/* ptr to directory entry */
	int	slot;			/* directory slot number */
	int	seq;			/* slot re-use sequence number */

	ASSERT(KS_HOLD0LOCKS());

	if (semid < 0 || semdir.ipcdir_nents == 0)
		return EINVAL;

	slot = SEMID_TO_SLOT(semid);
	seq  = SEMID_TO_SEQ(semid);
	dp = semdir.ipcdir_entries + slot;

again:
	if (hold)
		(void)SEMDIR_WRLOCK();
	else
		(void)SEMDIR_RDLOCK();

	if ((sp = IPC_TO_SEMDS(dp->ipcd_ent)) == NULL || seq != dp->ipcd_seq) {
		SEMDIR_UNLOCK(PLBASE);
		return EINVAL;
	}

	(void)SEMID_LOCK(sp);
	/* Wait if semid is marked busy. */
	if (sp->ksem_flag & SEMID_BUSY) {
		SEMDIR_UNLOCK(PLMIN);
		SV_WAIT(&sp->ksem_sv, PRIMED, &sp->ksem_mutex);
		goto again;
	}

	if (!hold)
		SEMDIR_UNLOCK(PLMIN);

	/* Must be allocated and not busy. */
	ASSERT((sp->ksem_ds.sem_perm.mode & IPC_ALLOC) != 0);
	ASSERT((sp->ksem_flag & SEMID_BUSY) == 0);

	*spp = sp;
	return 0;
}

/*
 * void semremove(const int semid)
 *	Remove semid from the semaphore directory.
 *
 * Calling/Exit State:
 *	Both the semaphore directory lock and the corresponding
 *	semid_ds structure must be locked by the caller.
 *	This function does not explicitly acquire or release any
 *	locks.
 */
STATIC void
semremove(const int semid)
{
	ipcdirent_t *dp;			/* ptr to directory entry */
	int	slot = SEMID_TO_SLOT(semid);	/* directory slot */

	ASSERT(RW_OWNED(&semdir_mutex));	/* held in exclusive mode */
	ASSERT(semid >= 0 && semdir.ipcdir_nents > slot);
	ASSERT(semdir.ipcdir_nactive > 0);

	semdir.ipcdir_nactive--;
	dp = semdir.ipcdir_entries + slot;
	dp->ipcd_ent = NULL;

	/* Bump the sequence number, don't allow negative semid's. */
	dp->ipcd_seq = (semid + seminfo.semmni < 0) ? 0 : dp->ipcd_seq + 1;
}

/*
 * void semexit(void)
 *	Called by exit() to clean up on process exit.
 *
 * Calling/Exit State:
 *	No locks can be held on entry to this function.
 *	The process is single threaded upon entry.
 */
void
semexit(void)
{
	struct sem_undo	* const sup = u.u_procp->p_semundo;
	struct ksemid_ds *sp;		/* semid being undone ptr */
	int		i;		/* loop control */
	long		v;		/* adjusted semaphore value */
	struct sem	*semp;		/* semaphore ptr */
	struct undo	*unp;		/* undo struct ptr */

	ASSERT(u.u_procp->p_nlwp == 1);
	ASSERT(KS_HOLD0LOCKS());

	if (sup == NULL)
		return;

	/*
	 * Acquiring the 'semexit_mutex' here serializes access to
	 * our sem_undo structure by other processes (actually, all
	 * out of context accesses to any sem_undo structures).
	 * Such acesses are made by the SETVAL, SETALL, and IPC_RMID
	 * commands to semctl() (callers of semunrm()).
	 * Since the calling process is known to be singly threaded
	 * at this point, we do not protect against other LWPs in the
	 * process modifying the sem_undo structure (i.e. do not acquire
	 * 'un_mutex').
	 *
	 * This is done for two reasons:
	 *	1) To avoid a hierarchy violation when acquiring
	 *	   the locks on the semid_ds structures (via semconv()).
	 *	   Since semop() is what we wish to optimize for, the
	 *	   dominant lock ordering is the lock on the semid_ds
	 *	   structure ('ksem_mutex'), followed by the lock on the
	 *	   per-process sem_undo structure ('un_mutex').
	 *
	 *	2) The SETVAL/SETALL commands to semctl() must do
	 *	   an atomic update to the semaphore value and clear
	 *	   all undo entries for the semaphore.  If this criteria
	 *	   is not met, the semaphore could end up with a wrong
	 *	   value.  This precludes removing the process sem_undo
	 *	   structure from the active undo list ('semunp') to
	 *	   cure the lock ordering problem.
	 *	   Note that IPC_RMID does not need to serialize with
	 *	   semexit().  This is because the sem_undo structure
	 *	   is removed from the active list before it is accessed
	 *	   here, and since the semaphore is going away we don't
	 *	   care about its value.
	 */
	RWSLEEP_RDLOCK(&semexit_mutex, PRIMED+2);
	(void)LOCK_PLMIN(&semunp_mutex);
	remque((void *)sup);			/* remove from active list */
	u.u_procp->p_semundo = NULL;
	UNLOCK(&semunp_mutex, PLBASE);

	if (sup->un_cnt == 0)
		goto cleanup;				/* no undo entries */
	for (i = sup->un_cnt, unp = &sup->un_ent[i-1]; i--; unp--) {
		if (semconv(unp->un_id, &sp, 0) != 0)
			continue;			/* semaphore is gone! */

		semp = sp->ksem_ds.sem_base + unp->un_num;
		v = semp->semval + unp->un_aoe;
		if (v < 0 || v > seminfo.semvmx) {
			SEMID_UNLOCK(sp, PLBASE);
			continue;
		}
		semp->semval = (ushort) v;		/* adjust semaphore */
		if (v == 0 && semp->semzcnt != 0)
			SV_BROADCAST(&semp->semz_sv, 0);
		if (unp->un_aoe > 0 && semp->semncnt != 0)
			SV_BROADCAST(&semp->semn_sv, 0);
		SEMID_UNLOCK(sp, PLBASE);
	}

cleanup:
	RWSLEEP_UNLOCK(&semexit_mutex);
	kmem_free((void *)sup, semundosz);	/* Free our sem_undo struct. */
}

/*
 * System call interfaces (semctl, semget, semop).
 */

#define	STACK_NSEMS 10
/*
 * int semctl(struct semctla *uap, rval_t *rvp)
 *	Semctl system call handler.
 *
 * Calling/Exit State:
 *	No locks are held on entry, none are held on return.
 */
STATIC int
semctl(struct semctla *uap, rval_t *rvp)
{
	struct	ksemid_ds	*sp;		/* ptr to semaphore descriptor*/
	struct sem		*semp;		/* ptr to semaphore */
	unsigned int		i;		/* loop control */
	int			error = 0;
	unsigned int		nsems = 0;
	struct semid_ds		ds;		/* application view */
	struct o_semid_ds	ods;		/* old application view */
	ushort			*semvals;
	ushort			usemvals[STACK_NSEMS];
	cred_t			*crp = CRED();	/* caller's credentials */
	pid_t			pid;
	lid_t			lid = 0;

	semvals = usemvals;
	rvp->r_val1 = 0;

	switch (uap->cmd) {

	/* Remove semaphore set. */
	case IPC_O_RMID:
	case IPC_RMID:
		if (error = semconv(uap->semid, &sp, 1))
			goto nolock;
		
		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R|SEM_A,
		    IPC_MAC, crp)) {
			SEMDIR_UNLOCK(PLMIN);
			break;
		}

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_SAME);
#endif
		
		/* must have ownership */
		if (crp->cr_uid != sp->ksem_ds.sem_perm.uid
		 && crp->cr_uid != sp->ksem_ds.sem_perm.cuid
		 && pm_denied(crp, P_OWNER)) {
			SEMDIR_UNLOCK(PLMIN);
			error = EPERM;
			break;
		}

		/*
		 * Committed to removing the entry.
		 * The caller holds an exclusive lock on the directory
		 * and on the semaphore descriptor.
		 * Semremove() makes the semaphore descriptor
		 * invisible to lookup channels (semget(), semconv()).
		 */
		semremove(uap->semid);

		sp->ksem_ds.sem_perm.mode &= ~IPC_ALLOC;

		SEMDIR_UNLOCK(PLMIN);

		/*
		 * Wake up all contexts which are blocked on
		 * the synch variables associated with this
		 * semaphore set.  These synch variables are
		 * going away real soon...
		 */
		nsems = sp->ksem_ds.sem_nsems;
		semp = sp->ksem_ds.sem_base;
		for (i = 0; i < nsems; i++, semp++) {
			if (semp->semncnt)
				SV_BROADCAST(&semp->semn_sv, 0);
			if (semp->semzcnt)
				SV_BROADCAST(&semp->semz_sv, 0);
		}


		/*
		 * Remove all undo entries for this semaphore set.
		 * Since the semaphore set is effectively gone (not
		 * in the directory), we do not bother to serialize
		 * with semexit().
		 */
		semunrm(uap->semid, 0, nsems);

		SEMID_UNLOCK(sp, PLBASE);

                /*
                 * Decrement mac levels reference.
                 * This must be done before deallocation since the
                 * ipc_secp referenced structure is also freed
                 * as part of the deallocation.
                 *
                 */
                mac_rele(lid);

		/* Free up all memory associated with this semaphore set. */
		semdealloc(&sp->ksem_ds.sem_perm);
		goto nolock;

	/* Set ownership and permissions. */
	case IPC_O_SET:
		if (copyin((void *)uap->arg, (void *)&ods, sizeof(ods))) {
			error = EFAULT;
			goto nolock;
		}
		if (ods.sem_perm.uid > MAXUID || ods.sem_perm.gid > MAXUID) {
			error = EINVAL;
			goto nolock;
		}
		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R|SEM_A,
		    IPC_MAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_SAME);
#endif
		
		/* must have ownership */
		if (crp->cr_uid != sp->ksem_ds.sem_perm.uid
		 && crp->cr_uid != sp->ksem_ds.sem_perm.cuid
		 && pm_denied(crp, P_OWNER)) {
			error = EPERM;
			break;
		}

		sp->ksem_ds.sem_perm.uid = ods.sem_perm.uid;
		sp->ksem_ds.sem_perm.gid = ods.sem_perm.gid;
		sp->ksem_ds.sem_perm.mode =
					(ods.sem_perm.mode & IPC_PERM) | IPC_ALLOC;
		sp->ksem_ds.sem_ctime = hrestime.tv_sec;
		break;

	case IPC_SET:
		ASSERT(KS_HOLD0LOCKS());

		if (copyin((caddr_t)uap->arg, (caddr_t)&ds, sizeof(ds))) {
			error = EFAULT;
			goto nolock;
		}
		if (ds.sem_perm.uid < (uid_t)0 || ds.sem_perm.uid > MAXUID
		  || ds.sem_perm.gid < (gid_t)0 || ds.sem_perm.gid > MAXUID) {
			error = EINVAL;
			goto nolock;
		}

		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R|SEM_A,
		    IPC_MAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_SAME);
#endif
		
		/* must have ownership */
		if (crp->cr_uid != sp->ksem_ds.sem_perm.uid
		 && crp->cr_uid != sp->ksem_ds.sem_perm.cuid
		 && pm_denied(crp, P_OWNER)) {
			error = EPERM;
			break;
		}

		sp->ksem_ds.sem_perm.uid = ds.sem_perm.uid;
		sp->ksem_ds.sem_perm.gid = ds.sem_perm.gid;
		sp->ksem_ds.sem_perm.mode =
				(ds.sem_perm.mode & IPC_PERM) | IPC_ALLOC;
		sp->ksem_ds.sem_ctime = hrestime.tv_sec;
		break;

	/* Get semaphore data structure. */
	case IPC_O_STAT:

		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R,
		    IPC_MAC|IPC_DAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_DOMINATES);
#endif
		
		/*
		 * Give an "old" (pre SVR4) view of the semid_ds structure to
		 * the application.
		 * Check whether SVR4 values are to large to store into an 
		 * "old" semid_ds structure.  If they are too large, then give
		 * the application an error code which it does not expect...
		 */

		if (sp->ksem_ds.sem_perm.uid  > USHRT_MAX
		 || sp->ksem_ds.sem_perm.gid  > USHRT_MAX
		 || sp->ksem_ds.sem_perm.cuid > USHRT_MAX
		 || sp->ksem_ds.sem_perm.cgid > USHRT_MAX
		 || sp->ksem_ds.sem_perm.seq  > USHRT_MAX) {
			error = EOVERFLOW;
			break;
		}

		ods.sem_perm.uid  = (o_uid_t) sp->ksem_ds.sem_perm.uid;
		ods.sem_perm.gid  = (o_gid_t) sp->ksem_ds.sem_perm.gid;
		ods.sem_perm.cuid = (o_uid_t) sp->ksem_ds.sem_perm.cuid;
		ods.sem_perm.cgid = (o_gid_t) sp->ksem_ds.sem_perm.cgid;
		ods.sem_perm.mode = (o_mode_t) sp->ksem_ds.sem_perm.mode;
		ods.sem_perm.seq  = (ushort) sp->ksem_ds.sem_perm.seq;
		ods.sem_perm.key  = sp->ksem_ds.sem_perm.key;

		ods.sem_base = NULL;		/* NULL out kernel addr */
		ods.sem_nsems = sp->ksem_ds.sem_nsems;
		ods.sem_otime = sp->ksem_ds.sem_otime;
		ods.sem_ctime = sp->ksem_ds.sem_ctime;

		SEMID_UNLOCK(sp, PLBASE);
		if (copyout((void *)&ods, (void *)uap->arg, sizeof(ods)))
			error = EFAULT;
		goto nolock;

	case IPC_STAT:

		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R,
		    IPC_MAC|IPC_DAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_DOMINATES);
#endif
		
		ds = sp->ksem_ds;		/* structure assignment */
		SEMID_UNLOCK(sp, PLBASE);

		ds.sem_base = NULL;		/* NULL out kernel addrs */
		ds.sem_perm.ipc_secp = NULL;

		if (copyout((void *)&ds, (void *)uap->arg, sizeof(ds)))
			error = EFAULT;
		goto nolock;

	/* Get # of processes sleeping for greater semval. */
	case GETNCNT:

		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R,
		    IPC_MAC|IPC_DAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_DOMINATES);
#endif
		if (uap->semnum >= sp->ksem_ds.sem_nsems)
			error = EINVAL;
		else
			rvp->r_val1 =
				(sp->ksem_ds.sem_base + uap->semnum)->semncnt;
		break;

	/* Get pid of last process to operate on semaphore. */
	case GETPID:

		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R,
		    IPC_MAC|IPC_DAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_DOMINATES);
#endif

		if (uap->semnum >= sp->ksem_ds.sem_nsems)
			error = EINVAL;
		else
			rvp->r_val1 =
				(sp->ksem_ds.sem_base + uap->semnum)->sempid;
		break;

	/* Get semval of one semaphore. */
	case GETVAL:

		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R,
		    IPC_MAC|IPC_DAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_DOMINATES);
#endif

		if (uap->semnum >= sp->ksem_ds.sem_nsems)
			error = EINVAL;
		else
			rvp->r_val1 =
				(sp->ksem_ds.sem_base + uap->semnum)->semval;
		break;

	/* Get all semvals in set. */
	case GETALL:

		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R,
		    IPC_MAC|IPC_DAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_DOMINATES);
#endif

		if ((nsems = sp->ksem_ds.sem_nsems) > STACK_NSEMS) {
			sp->ksem_flag |= SEMID_BUSY;
			SEMID_UNLOCK(sp, PLBASE);
			semvals = (ushort *)
				kmem_alloc(sizeof(ushort) * nsems, KM_SLEEP);
			SEMID_LOCK(sp);
			sp->ksem_flag &= ~SEMID_BUSY;
			if (SV_BLKD(&sp->ksem_sv))
				SV_BROADCAST(&sp->ksem_sv, 0);
		}

		for (i = 0, semp = sp->ksem_ds.sem_base; i < nsems; i++, semp++)
			semvals[i] = semp->semval;

		SEMID_UNLOCK(sp, PLBASE);

		if (copyout((void *)semvals, (void *)uap->arg,
		    sizeof(ushort) * nsems))
			error = EFAULT;

		goto nolock;

	/* Get # of contexts sleeping for semval to become zero. */
	case GETZCNT:

		if (error = semconv(uap->semid, &sp, 0))
			goto nolock;

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_R,
		    IPC_MAC|IPC_DAC, crp))
			break;

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_DOMINATES);
#endif

		if (uap->semnum >= sp->ksem_ds.sem_nsems)
			error = EINVAL;
		else
			rvp->r_val1 =
				(sp->ksem_ds.sem_base + uap->semnum)->semzcnt;
		break;

	/* Set semval of one semaphore. */
	case SETVAL:

		if ((unsigned)uap->arg > seminfo.semvmx) {
			error = ERANGE;
			goto nolock;
		}

		RWSLEEP_WRLOCK(&semexit_mutex, PRIMED+2);
		if (error = semconv(uap->semid, &sp, 0)) {
			RWSLEEP_UNLOCK(&semexit_mutex);
			goto nolock;
		}

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_A,
		    IPC_MAC|IPC_DAC, crp)) {
			RWSLEEP_UNLOCK(&semexit_mutex);
			break;
		}

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_SAME);
#endif
		if (uap->semnum >= sp->ksem_ds.sem_nsems) {
			RWSLEEP_UNLOCK(&semexit_mutex);
			error = EINVAL;
			break;
		}

		/*
		 * Remove all undo entries for this semaphore.
		 */
		semunrm(uap->semid, uap->semnum, uap->semnum);
		RWSLEEP_UNLOCK(&semexit_mutex);

		semp = sp->ksem_ds.sem_base + uap->semnum;
		if ((semp->semval = uap->arg) != 0) {
			if (semp->semncnt)
				SV_BROADCAST(&semp->semn_sv, 0);
		} else if (semp->semzcnt)
			SV_BROADCAST(&semp->semz_sv, 0);
		semp->sempid = u.u_procp->p_pidp->pid_id;

		break;

	/* Set semvals of all semaphores in set. */
	case SETALL:

		RWSLEEP_WRLOCK(&semexit_mutex, PRIMED+2);

		if (error = semconv(uap->semid, &sp, 0)) {
			RWSLEEP_UNLOCK(&semexit_mutex);
			goto nolock;
		}

		lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

		if (error = ipcaccess(&sp->ksem_ds.sem_perm, SEM_A,
		    IPC_MAC|IPC_DAC, crp)) {
			RWSLEEP_UNLOCK(&semexit_mutex);
			break;
		}

#ifdef CC_PARTIAL
                MAC_ASSERT(sp, MAC_SAME);
#endif

		/*
		 * Mark the semphore set  as BUSY and drop the
		 * lock.  This is done since we may block for
		 * memory allocation, or pagefault on the copyin().
		 */
		sp->ksem_flag |= SEMID_BUSY;
		SEMID_UNLOCK(sp, PLBASE);
		if ((nsems = sp->ksem_ds.sem_nsems) > STACK_NSEMS)
			semvals = (ushort *)
				kmem_alloc(sizeof(ushort) * nsems, KM_SLEEP);

		if (copyin((void *)uap->arg, (void *)semvals,
		  sizeof(ushort) * nsems))
			error = EFAULT;
		
		SEMID_LOCK(sp);
		sp->ksem_flag &= ~SEMID_BUSY;
		if (SV_BLKD(&sp->ksem_sv))
			SV_BROADCAST(&sp->ksem_sv, 0);

		if (error)
			break;

		for (i = nsems; i-- != 0;)
			if (semvals[i] > (unsigned)seminfo.semvmx) {
				error = ERANGE;
				break;
			}


		/*
		 * Remove all undo entries for this semaphore set.
		 * Once this is complete, we can safely drop the
		 * semexit_mutex which serializes this code path
		 * w.r.t. semexit().
		 * Since this code path is serialized, semexit()
		 * sees a stable view of its sem_undo structure
		 * without acquiring its table lock (un_mutex).
		 */
		semunrm(uap->semid, 0, nsems);
		RWSLEEP_UNLOCK(&semexit_mutex);

		/*
		 * Update the semaphore values, and signal any waiters.
		 * Since we hold the lock on the semaphore set (ksem_mutex)
		 * it is not possible for any other context to create
		 * new undo entries for this semaphore set.
		 */
		pid = u.u_procp->p_pidp->pid_id;
		for (i=0, semp = sp->ksem_ds.sem_base; i < nsems; i++, semp++) {
			semp->sempid = pid;
			if ((semp->semval = semvals[i]) != 0) {
				if (semp->semncnt)
					SV_BROADCAST(&semp->semn_sv, 0);
			} else if (semp->semzcnt)
				SV_BROADCAST(&semp->semz_sv, 0);
		}

		break;

	default:
		error = EINVAL;
		goto nolock;
	}

	ASSERT(LOCK_OWNED(&sp->ksem_mutex));
	SEMID_UNLOCK(sp, PLBASE);

nolock:
	ASSERT(KS_HOLD0LOCKS());
	if (lid)
		ADT_LIDCHECK(lid);
	if (semvals != usemvals) {
		ASSERT(nsems > STACK_NSEMS);
		ASSERT(uap->cmd == SETALL || uap->cmd == GETALL);
		kmem_free((void *)semvals, sizeof(ushort) * nsems);
	}

	return error;
}

/*
 * int semget(struct semgeta *uap, rval_t *rvp)
 *	Semget system call handler.
 *
 * Calling/Exit State:
 *	No locks are held on entry, no locks are held on return.
 */
STATIC int
semget(struct semgeta *uap, rval_t *rvp)
{
	struct ksemid_ds *sp;			/* semaphore header ptr */
	struct sem	*semp;
	ipcdirent_t	*dp;
	boolean_t	new;			/* ipcget status return */
	int		error;
	int		slot;
	int		semid;
	int		i;
	lid_t		lid;
	const int	nsems = uap->nsems;

	ASSERT(KS_HOLD0LOCKS());

	(void)SEMDIR_RDLOCK();
	if (error = ipcget(uap->key, uap->semflg, &semdata, &new, &dp)) {
		SEMDIR_UNLOCK(PLBASE);
		return error;
	}

	sp = IPC_TO_SEMDS(dp->ipcd_ent);
#ifdef CC_PARTIAL
        MAC_ASSERT(sp, MAC_SAME);
#endif
	lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

	slot = dp - semdir.ipcdir_entries;
	semid = SLOT_TO_SEMID(slot, dp->ipcd_seq);

	if (new == B_TRUE) {
		/*
		 * This is a new semaphore set.  Finish initialization.
		 * For new semaphore sets, the directory lock (semdir_mutex)
		 * is held in exclusive mode.  See semalloc() to see how
		 * this is done.
		 */
		if (nsems <= 0 || nsems > seminfo.semmsl) {
			ipc_perm_t *ipc = dp->ipcd_ent;

			dp->ipcd_ent = NULL;
			semdir.ipcdir_nactive--;
			SEMDIR_UNLOCK(PLBASE);
                        mac_rele(lid);
			semdealloc(ipc);
			error = EINVAL;
			goto out;
		}

		/*
		 * If we were to enforce an arbitrary system wide
		 * limit on sem structures, this would be the place
		 * to do it.  Currently, we do not, and the limit
		 * is (seminfo.semmni * seminfo.semmsl).
		 * This seems to be reasonable.
		 */

		/*
		 * Mark this semid_ds as busy so that we can
		 * drop the directory lock.  This is done so
		 * that we can complete the initialization of
		 * this data structure.  We may block to allocate
		 * memory.  Any other context attempting to get
		 * a handle on this semid_ds will block in semconv()
		 * until we clear the SEMID_BUSY flag.
		 * Note that this new entry (key) is visible via semget()
		 * and semget() can return a semid to the caller.  It
		 * is the usage of the semid which is blocked.
		 */
		(void)SEMID_LOCK(sp);
		sp->ksem_flag |= SEMID_BUSY;
		SEMID_UNLOCK(sp, PLMIN);
		SEMDIR_UNLOCK(PLBASE);

		/* IPC security structure is allocated in semalloc() */
		ASSERT(sp->ksem_ds.sem_perm.ipc_secp != NULL);

		/*
		 * Allocate semaphore structures, and initialize associated
		 * synch variables.
		 */
		semp = (struct sem *)
				kmem_zalloc(nsems*sizeof(struct sem), KM_SLEEP);
		sp->ksem_ds.sem_base = semp;

		for (i = 0; i < nsems; i++, semp++) {
			SV_INIT(&semp->semn_sv);
			SV_INIT(&semp->semz_sv);
		}

		sp->ksem_ds.sem_nsems = (ushort)nsems;
		sp->ksem_ds.sem_ctime = hrestime.tv_sec;
		sp->ksem_ds.sem_otime = 0;
		sp->ksem_ds.sem_perm.mode |= IPC_ALLOC;	/* init complete */

		/* Clear the busy bit and wakeup all waiters. */
		(void)SEMID_LOCK(sp);
		sp->ksem_flag &= ~SEMID_BUSY;
		if (SV_BLKD(&sp->ksem_sv))
			SV_BROADCAST(&sp->ksem_sv, 0);
		SEMID_UNLOCK(sp, PLBASE);
	} else {
		if (nsems != 0 && sp->ksem_ds.sem_nsems < (unsigned)nsems)
			error = EINVAL;
		SEMDIR_UNLOCK(PLBASE);
	}

out:
	ASSERT(KS_HOLD0LOCKS());
	ADT_LIDCHECK(lid);
	if (!error)
		rvp->r_val1 = semid;
	return error;
}


#define	STACK_SEMOPS	10

/*
 * int semop(struct semopa *uap, rval_t *rvp)
 *	Semop system call handler.
 *
 * Calling/Exit State:
 *	No locks are held on entry, no locks are held on return.
 */
STATIC int
semop(struct semopa *uap, rval_t *rvp)
{
	const struct sembuf	*op;		/* ptr to operation */
	struct ksemid_ds	*sp;		/* ptr to associated header */
	struct sem		*semp;		/* ptr to semaphore */
	pid_t			pid;
	int			i, again, error;
	int			nsops;
	int			undo = 0;	/* True if SEM_UNDO specified */
	struct sembuf		uops[STACK_SEMOPS];
	const struct sembuf	*semops = uops;	/* default user semop list */
	cred_t			*crp = CRED();	/* caller's credentials */
	proc_t * const		p = u.u_procp;
	lid_t			lid = 0;
	ushort_t *countp = NULL; /* address of count we need to decrement */

	MET_SEMA();		/* bump semaphore operation count */

	if ((nsops = uap->nsops) > seminfo.semopm)
		return E2BIG;

	if (nsops > STACK_SEMOPS)
		semops = (struct sembuf *)
			    kmem_alloc(nsops * sizeof(struct sembuf), KM_SLEEP);

	if (copyin((void *)uap->sops, (void *)semops,
			nsops * sizeof(struct sembuf))) {
		error = EFAULT;
		goto out;
	}

	/* On success, semconv() returns with the semid_ds locked. */
	if (error = semconv(uap->semid, &sp, 0))
		goto out;

	lid = sp->ksem_ds.sem_perm.ipc_secp->ipc_lid;

	/* Verify that sem #s are in range and permissions are granted. */
	for (i = 0, op = semops; i < nsops; i++, op++) {
		/*
		 * Write MAC access is needed for read since a read
		 * operation modifies the object as well.
		 */
		if ((error = ipcaccess(&sp->ksem_ds.sem_perm,
		    op->sem_op ? SEM_A : SEM_R|SEM_A, IPC_MAC, crp)) != 0
		 || (error = ipcaccess(&sp->ksem_ds.sem_perm,
		    op->sem_op ? SEM_A : SEM_R, IPC_DAC, crp)) != 0) {
			SEMID_UNLOCK(sp, PLBASE);
			goto out;
		}
		if (op->sem_num >= sp->ksem_ds.sem_nsems) {
			error = EFBIG;
			SEMID_UNLOCK(sp, PLBASE);
			goto out;
		}
		if (op->sem_flg & SEM_UNDO)
			undo = 1;
	}

	/*
	 * Allocate a sem_undo structure for the process if we need one.
	 * Once a sem_undo structure is allocated for a process, it
	 * remains allocated until the process exits (free'd in semexit()).
	 * Races with other LWPs in the process are possible here and are
	 * resolved by holding semunp_mutex.  Semunp_mutex is used here
	 * instead of p_mutex since we need to put the newly allocated
	 * sem_undo structure onto the active list of sem_undo structures.
	 *
	 * This deviates slightly from previous versions of the semaphore
	 * IPC in that sem_undo structures appear on the active list even
	 * if there are no undo entries (i.e. un_cnt == 0).  This is done
	 * to prevent a lock ordering problem which would result when
	 * attempting to remove/add a sem_undo structure when un_cnt
	 * transitions to/from zero.
	 */
	if (undo && p->p_semundo == NULL) {
		struct sem_undo *sup;

		sp->ksem_flag |= SEMID_BUSY;
		SEMID_UNLOCK(sp, PLBASE);
		sup = (struct sem_undo *)kmem_zalloc(semundosz, KM_SLEEP);
		FSPIN_INIT(&sup->un_mutex);
		(void)LOCK_PLMIN(&semunp_mutex);
		if (p->p_semundo == NULL) {		/* check for race */
			p->p_semundo = sup;			/* we won */
			insque((void *)sup, (void *)&semunp);
			UNLOCK(&semunp_mutex, PLBASE);
		} else {
			UNLOCK(&semunp_mutex, PLBASE);		/* we lost */
			kmem_free((void *)sup, semundosz);
		}

		SEMID_LOCK(sp);
		sp->ksem_flag &= ~SEMID_BUSY;
		if (SV_BLKD(&sp->ksem_sv))
			SV_BROADCAST(&sp->ksem_sv, 0);

		ASSERT(sp->ksem_ds.sem_perm.mode & IPC_ALLOC);	/* paranoid */
	}

	ASSERT(LOCK_OWNED(&sp->ksem_mutex) && KS_HOLD1LOCK());

#ifdef CC_PARTIAL
        MAC_ASSERT(sp, MAC_SAME);
#endif

	again = 0;
check:
	/*
	 * Loop waiting for the operations to be satisfied atomically.
	 * Actually, do the operations and undo them if a wait is needed
	 * or an error is detected (but do not do any wakeups until we
	 * satisfy all operations).
	 */
	if (again) {
		struct ksemid_ds *sp2;

		ASSERT(KS_HOLD0LOCKS());

		/*
		 * Since the lock on the semid_ds was dropped, we must
		 * verify that the semaphore set has not been removed.
		 */
		if (semconv(uap->semid, &sp2, 0) != 0) {
			error = EIDRM;
			goto out;
		}
		ASSERT(sp == sp2);
		ASSERT(LOCK_OWNED(&sp->ksem_mutex) && KS_HOLD1LOCK());
		if (countp) {
			--(*countp);
			countp = NULL;
		}
	} else
		again = 1;

	for (i = 0, op = semops; i < nsops; i++, op++) {
		semp = sp->ksem_ds.sem_base + op->sem_num;
		if (op->sem_op > 0) {
			if (op->sem_op + (long)semp->semval > seminfo.semvmx
			  || (op->sem_flg & SEM_UNDO
			    && (error = semaoe(op->sem_op, uap->semid,
			      op->sem_num)))) {
				if (i) {
					/* undo previous ops (0 through i-1) */
					semundo(semops, i, uap->semid, sp);
				}
				if (error == 0)
					error = ERANGE;
				SEMID_UNLOCK(sp, PLBASE);
				goto out;
			}
			semp->semval += op->sem_op;
			/*
			 * Wakeups are delayed until all ops complete
			 * since we cannot undo a wakeup.
			 */
			continue;
		}
		if (op->sem_op < 0) {
			if (semp->semval >= (unsigned)(-op->sem_op)) {
				if (op->sem_flg & SEM_UNDO
				  && (error = semaoe(op->sem_op, uap->semid,
				    op->sem_num))) {
					if (i) {
						semundo(semops, i,
						  uap->semid, sp);
					}
					SEMID_UNLOCK(sp, PLBASE);
					goto out;
				}
				semp->semval += op->sem_op;
				/*
				 * Wakeups are delayed until all ops complete
				 * since we cannot undo a wakeup.
				 */
				continue;
			}
			if (i)
				semundo(semops, i, uap->semid, sp);
			if (op->sem_flg & IPC_NOWAIT) {
				SEMID_UNLOCK(sp, PLBASE);
				error = EAGAIN;
				goto out;
			}
			/* Wait for semval to be greater than the op amount. */
			countp = &semp->semncnt;
			++(*countp);
			if (!SV_WAIT_SIG(&semp->semn_sv, SEMNPRI,
			    &sp->ksem_mutex)) {		/* Hit by a signal! */
				struct ksemid_ds *sp2;
				if (semconv(uap->semid, &sp2, 0) == 0) {
					ASSERT(sp == sp2);
					--(*countp);
					SEMID_UNLOCK(sp, PLBASE);
				}
				error = EINTR;
				goto out;
			}
			goto check;		/* re-evaluate */
		}

		ASSERT(op->sem_op == 0);
		if (semp->semval) {
			if (i)
				semundo(semops, i, uap->semid, sp);
			if (op->sem_flg & IPC_NOWAIT) {
				SEMID_UNLOCK(sp, PLBASE);
				error = EAGAIN;
				goto out;
			}

			/* Wait for semval to go to zero. */
			countp = &semp->semzcnt;
			++(*countp);
			if (!SV_WAIT_SIG(&semp->semz_sv, SEMZPRI,
			    &sp->ksem_mutex)) {		/* Hit by a signal! */
				struct	ksemid_ds *sp2;
				if (semconv(uap->semid, &sp2, 0) == 0) {
					ASSERT(sp == sp2);
					--(*countp);
					SEMID_UNLOCK(sp, PLBASE);
				}
				error = EINTR;
				goto out;
			}
			goto check;
		}
	}

	/*
	 * All operations succeeded.  Update sempid for accessed semaphores,
	 * and post necessary wakeups.
	 */
	pid = u.u_procp->p_pidp->pid_id;
	for (i = 0, op = semops; i < nsops; i++, op++) {
		semp = sp->ksem_ds.sem_base + op->sem_num;
		semp->sempid = pid;
		if (op->sem_op > 0) {
			if (semp->semncnt != 0)
				SV_BROADCAST(&semp->semn_sv, 0);
			if (semp->semzcnt != 0 && semp->semval == 0)
				SV_BROADCAST(&semp->semz_sv, 0);
		} else if (op->sem_op < 0) {
			/*
			 * May have decrememented semval to zero.
			 */
			if (semp->semzcnt != 0 && semp->semval == 0)
				SV_BROADCAST(&semp->semz_sv, 0);
		}
	}

	sp->ksem_ds.sem_otime = hrestime.tv_sec;
	SEMID_UNLOCK(sp, PLBASE);
	rvp->r_val1 = 0;
out:
	ASSERT(KS_HOLD0LOCKS());
	if (lid)
		ADT_LIDCHECK(lid);
	if (semops != uops) {
		ASSERT(nsops > STACK_SEMOPS);
		kmem_free((void *)semops, nsops * sizeof(struct sembuf));
	}

	return error;
}

/*
 * int semsys(union semsysa *uap, rval_t *rvp)
 * 	Entry point for semctl, semget, and semop system calls.
 *
 * Calling/Exit State:
 *	No locks are held on entry, none are held on return.
 */
int
semsys(union semsysa *uap, rval_t *rvp)
{
	int error;
	
	ASSERT(KS_HOLD0LOCKS());

	switch (uap->opcode) {
	case SEMCTL:
		error = semctl(&uap->semctla, rvp);
		break;
	case SEMGET:
		error = semget(&uap->semgeta, rvp);
		break;
	case SEMOP:
		error = semop(&uap->semopa, rvp);
		break;
	default:
		error = EINVAL;
		break;
	}

	ASSERT(KS_HOLD0LOCKS());
	return error;
}

STATIC size_t semidsz = (sizeof(struct ksemid_ds) + sizeof(struct ipc_sec)
				+ sizeof(long) - 1)
				& ~(sizeof(long) - 1);

/*
 * int semalloc(ipc_perm_t **ipcpp)
 *	Allocate and partially initialize a ksemid_ds data
 *	structure.  A pointer to the encapsulated ipc_perm
 *	structure is returned via the out argument 'ipcpp'.
 *
 * Calling/Exit State:
 *	Upon entry this function expects to be called with
 *	the semdir_mutex held.  On return, the semdir_mutex
 *	is held in exclusive mode.  However, semdir_mutex is
 *	dropped and re-acquired internally to this function.
 *	This function can block (via kmem_alloc()).
 *	This function must return a nonzero value to indicate
 *	to the caller that the directory lock was dropped.
 *
 * Notes:
 *	Only called from ipcget() via the IPC_ALLOCATE() macro.
 */
STATIC int
semalloc(ipc_perm_t **ipcpp)
{
	vaddr_t	vaddr;
	struct	ksemid_ds *sp;

	ASSERT(RW_OWNED(&semdir_mutex) && KS_HOLD1LOCK());

	SEMDIR_UNLOCK(PLBASE);

	/*
	 * Allocate the ksemid_ds structure and the
	 * ipc_sec structure in one shot.
	 */
	vaddr = (vaddr_t)kmem_zalloc(semidsz, KM_SLEEP);
	sp = (struct ksemid_ds *)vaddr;

	/* Align address for ipc_sec structure; reflected in semidsz. */
	vaddr += (sizeof(struct ksemid_ds) + sizeof(long) - 1)
			& ~(sizeof(long) - 1);
	sp->ksem_ds.sem_perm.ipc_secp = (struct ipc_sec *)vaddr;

	/* Initialize lock and synch variable. */
	LOCK_INIT(&sp->ksem_mutex, SEMDS_HIER, PLMIN, &ksem_lkinfo,
			KM_SLEEP);
	SV_INIT(&sp->ksem_sv);
	*ipcpp = &sp->ksem_ds.sem_perm;	/* out argument */

	/*
	 * Acquire the directory lock in exclusive mode in anticipation
	 * of a write to the directory by ipcget().
	 */
	SEMDIR_WRLOCK();
	return 1;			/* indicate that lock was dropped */
}

/*
 * void semdealloc(ipc_perm_t *ipcp)
 *	Deallocate the passed in ipc data structure and associated
 *	data structures.
 *
 * Calling/Exit State:
 *	This function is called with the semdir_mutex held via ipcget(),
 *	and with no locks held from semget() and semctl(IPC_RMID).
 *	This function does not block.
 *	The ipc data structure must be invisible to any lookup channels
 *	prior to calling this function.
 *
 * Notes:
 *	Called from ipcget() via the IPC_DEALLOCATE() macro.
 *      Above calls should NOT be associated with mac_rele calls
 *      because they represent UNSUCCESSFUL allocation attempts.
 *
 *      Also called directly from semget(), and semctl(IPC_RMID).
 *      Above calls SHOULD be associated with mac_rele calls
 *      since they are bona fide removal of sem object
 *
 *      Until the definition and/or use of the ALLOCATE/DEALLOCATE macros
 *      is changed it's most direct to put the mac_rele calls in the
 *      bona fide callers of semdealloc.  Warrants change to more desirable
 *      robust arrangement.
 */
STATIC void
semdealloc(ipc_perm_t *ipcp)
{
	struct ksemid_ds *sp = IPC_TO_SEMDS(ipcp);
	struct sem	*semp;

	ASSERT(KS_HOLD0LOCKS() || (KS_HOLD1LOCK() && RW_OWNED(&semdir_mutex)));
	ASSERT(!SV_BLKD(&sp->ksem_sv));

	LOCK_DEINIT(&sp->ksem_mutex);

	if ((semp = sp->ksem_ds.sem_base) != NULL) {
#ifdef DEBUG
		ushort i;

		ASSERT(sp->ksem_ds.sem_nsems > 0);
		/*
		 * Paranoid: Make sure that no context is blocked
		 * on a synch variable which is about to go away!
		 */
		for (i = 0; i < sp->ksem_ds.sem_nsems; i++) {
			ASSERT(!SV_BLKD(&(semp + i)->semn_sv));
			ASSERT(!SV_BLKD(&(semp + i)->semz_sv));
		}
#endif
		kmem_free((void *)semp,
			sp->ksem_ds.sem_nsems * sizeof(struct sem));
	}

	/*
	 * Free ACL for object (if any); note that the argument to
	 * FRIPCACL() must be an unevaluated security entry.
	 */
	FRIPCACL(sp->ksem_ds.sem_perm.ipc_secp);

	/* Free up ksemid_ds and ipc_sec structures in one shot. */
	kmem_free((void *)sp, semidsz);
}

/*
 * void seminit(void)
 * 	One time initialization routine for System V semaphore IPC.
 *
 * Calling/Exit State:
 *	Called early during system initialization.
 *	KMA must be available.
 */
void
seminit(void)
{
	const int n = seminfo.semmni;

	RWSLEEP_INIT(&semexit_mutex, (uchar_t)0, &semexit_lkinfo, KM_NOSLEEP);
	RW_INIT(&semdir_mutex, SEMDIR_HIER, PLMIN, &semdir_lkinfo,
			KM_NOSLEEP);
	LOCK_INIT(&semunp_mutex, SEMUNP_HIER, PLMIN, &semunp_lkinfo,
			KM_NOSLEEP);

	INITQUE(&semunp);			/* empty sem_undo list */

	/*
	 * Set semundosz at runtime since "seminfo.semume" is
	 * a configuration parameter.
	 */
	ASSERT(seminfo.semume > 0);
	semundosz = sizeof(struct sem_undo) +
		((seminfo.semume - 1) * sizeof(struct undo));

	/* Initialize the IPC directory for semaphores */
	semdir.ipcdir_nents = n;
	semdir.ipcdir_nactive = 0;
	semdir.ipcdir_entries = (ipcdirent_t *)kmem_zalloc(n *
					sizeof(ipcdirent_t), KM_NOSLEEP);

	if (semdir.ipcdir_entries == NULL && n != 0) {
		/*
		 *+ Could not allocate memory for the SystemV
		 *+ semaphore IPC directory.
		 *+ Instead of PANIC'ing the system, this IPC mechanism
		 *+ is disabled and a warning message is printed.
		 *+ The system configuration parameter seminfo.semmni
		 *+ should be checked to make sure that it is not
		 *+ inordinately large.
		 */
		cmn_err(CE_WARN,
		 "seminit: Can't allocate IPC directory, semaphores disabled");

		/*
		 * Setting ipcdir_nents to zero will cause
		 * ipcget() to always fail with ENOSPC.
		 */
		semdir.ipcdir_nents = 0;
	}
}
