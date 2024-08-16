/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:proc/usync.c	1.26"
#ident	"$Header: $"

#include <mem/as.h>
#include <proc/class.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/proc_hier.h>
#include <proc/mman.h>
#include <proc/usync.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/types.h>
#include <util/var.h>

/*
 * Forward references to some sync-queue management routines.
 */
STATIC int sq_hold(sq_t *, char *);
STATIC int sq_rele(sq_t *, int , char *);
STATIC int semasq_hold(sq_t *);
STATIC int semasq_rele(sq_t *, int);
STATIC void sq_timedsetrun(lwp_t *);
STATIC sq_t *sqfind(void *, void *);
STATIC sq_t *lsqfind(vaddr_t);
STATIC sq_t *sqget(void *, void *);
STATIC sq_t *lsqget(vaddr_t);

list_t sq_hash[SQ_NHASH];
lock_t sq_update;
sleep_t sema_locks[SEMA_HASHSIZE];

/*
 *+ This is the global sync queue hash update spin lock.
 */
LKINFO_DECL(sq_update_info, "PS::sq_update", 0);

/*
 *+ This is semaphore sleep lock.
 */
LKINFO_DECL(sema_locks_buf, "SCHED:USYNC:lwp semaphore sleep lock", 0);


struct prepblock_args {
	vaddr_t mutexp;
	char *waiterp;
	int	rdwr;
};

/*
 *
 * int
 * prepblock(struct prepblock_args *uqp, rval_t *rvp)
 *
 *	Prepare the caller for blocking.
 *
 * Calling/Exit State:
 *
 *	Called from the system call entry point.
 *
 * Description:
 *
 *	Queue the current lwp against the sync-queue identified by "waiterp".
 *	If sync-queue does not exist, allocate one.
 *
 *	Prepblock is called as part of a prepblock/block pair.	It allows the
 *	caller to return to user-mode after enqueueing on a sync queue
 *	and thus eliminating missed wakeup.
 *
 *	The lwp is queued against the sync-queue using l_sfwd and l_sbak.  It
 *	may not use l_link/l_rlink, as it is possible for an lwp to be blocked
 *	against a user-mode synchronization primitive and runnable or blocked
 *	against a kernel-mode synchronization primitive.
 *
 *	An lwp queues against the sync-queue as a member of the class "rdwr".
 *	This class is either "reader", "writer" or "any".  Class is only used
 *	by reader-writer sleep locks.
 *
 */
/* ARGSUSED */
int
prepblock(struct prepblock_args *uap, rval_t *rvp)
{
	sq_t *sq = NULL;
	lwp_t *lwp = u.u_lwpp;
	int error;
	boolean_t addrvalid;
	uint_t	maptype;
	void	*vp;
	void	*off;
	pl_t	pl;
	int ret;
	lock_t *lockp;

	/*
	 * No need to hold l_mutex since the lwp checks its own
	 * l_sq and no one else changes that field.
	 */
	if (lwp->l_sq != NULL) {
		/*
		 * We've already prepblocked.
		 */
		return(EINVAL);
	}

	as_rdlock(u.u_procp->p_as);
	addrvalid = as_getwchan((vaddr_t)uap->waiterp, &maptype, &vp, &off);
	if (addrvalid == B_FALSE) {  /* address of waiter flag not mapped in */
		as_unlock(u.u_procp->p_as);
		return (EFAULT);
	}
	addrvalid = as_getwchan(uap->mutexp, &maptype, &vp, &off);
	if (addrvalid == B_FALSE) {  /* address of mutex not mapped in */
		as_unlock(u.u_procp->p_as);
		return (EFAULT);
	}

	as_unlock(u.u_procp->p_as);

	/*
	 * Find or allocate a sync queue.
	 * If a sync queue is not found, one is allocated
	 * and appended to the hash/linked queue.
	 */
	if (maptype == MAP_PRIVATE) {
		lockp = &u.u_procp->p_squpdate;
		LOCK(lockp, PLHI);
		sq = (sq_t *)lsqget(uap->mutexp);
	} else {
		lockp = &sq_update;
		LOCK(lockp, PLHI);
		sq = (sq_t *)sqget(vp, off);
	}

	ret = sq_hold(sq, uap->waiterp);
	if (ret != 0) {	 /* sq_hold failed, back out of prepblock */
		sq_rele(sq, 1, uap->waiterp);
		return (EFAULT);
	}

	pl = LOCK(&lwp->l_mutex, PLHI);
	/*
	 * CL_PREPBLOCK places the lwp on the sync queue.
	 * The order of queueing depends on scheduling class.
	 */
	error = CL_PREPBLOCK(lwp, sq, uap->rdwr);
	lwp->l_sq = sq;
	lwp->l_waiterp = uap->waiterp;
	lwp->l_flag |= L_ONSQ;
	if (maptype != MAP_PRIVATE)
		lwp->l_flag |= L_ONGLOBSQ;
	UNLOCK(&lwp->l_mutex, pl);

	UNLOCK(lockp, PLBASE);

	return(error);
}

struct block_args {
	timestruc_t *abstime;
};

/*
 * int block(struct block_args *uap, rval_t *rvp)
 *
 *	Block against the sync-queue originally identified by
 *	the previous prepblock.
 *
 * Calling/Exit State:
 *
 *	This is a system call.
 *
 * Description:
 *
 *	Actually block against the user-mode synchronization object we've
 *	previously queued against using prepblock.
 *
 *	If "abstime" is non-NULL, the timeout value is validated and
 *	itimeout is called for the time remaining (after converting from
 *	absolute to relative, i.e., time left).
 *
 *	If CL_BLOCK returns an error, this implies the lwp received an
 *	interrupt.  The lwp will be left on the sync-queue if it was
 *	woken up by a signal (if woken up by timeout, it will be
 *	dequeued by block system call) and is capable of calling block
 *	again. For this reason, we maintain our reference to the sync-queue.
 */
/* ARGSUSED */
int
block(struct block_args *uap, rval_t *rvp)
{
	timestruc_t time;
	long timeoutval;	/* in ticks */
	long temp1, temp2;
	lwp_t *lwp = u.u_lwpp;
	sq_t *sq;
	int error = 0;
	toid_t	id;
	lock_t *lockp;


	/*
	 * No need to hold l_mutex.
	 */
	if ((sq = lwp->l_sq) == NULL) {
		/*
		 * We haven't prepblocked.
		 */
		return(ENOENT);
	}

	if (uap->abstime != NULL) {
		if (copyin((caddr_t)uap->abstime, (caddr_t)&time,
			sizeof(timestruc_t)))
			return (EFAULT);
		/* Perform sanity checking on values supplied */
		if (time.tv_sec <0 || time.tv_nsec < 0 ||
			time.tv_nsec > NANOSEC)
			return (EINVAL);
		/* Convert timeout values from absolute to relative */
		temp1 = time.tv_sec - hrestime.tv_sec;
		temp2 = time.tv_nsec - hrestime.tv_nsec;
		if (temp2 < 0) {
			temp1 -= 1;
			temp2 += NANOSEC;
		}
		if (temp1 < 0 || (temp1 == 0 && temp2 == 0)) 
			return (ETIME);
		/* Perform sanity checking on timeout values */
		if (temp1 > MAXTIMEOUT)
			return (EINVAL);
			
		/* Convert timeout value (sec, nsec) -> ticks */
		temp1 = temp1 * HZ;
		temp2 = temp2 / NSEC_PER_HZ;
		timeoutval = temp1 + temp2;

		id = itimeout(sq_timedsetrun, u.u_lwpp, timeoutval, PL1);
	}

	LOCK(&lwp->l_mutex, PLHI);
	error = CL_BLOCK(lwp, sq);
	UNLOCK(&lwp->l_mutex, PLBASE);

	if (uap->abstime != NULL) {
		untimeout(id);
	}

	if (error == 0) {
		LOCK(&lwp->l_mutex, PLHI);
		ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
		lwp->l_sq = NULL;
		lwp->l_flag &= ~(L_ONGLOBSQ|L_SQTIMEDOUT);
		UNLOCK(&lwp->l_mutex, PLBASE);
	} else if (error == ETIME) {
		if (sq->sq_flags & SQGLOBAL)
			lockp = &sq_update;
		else
			lockp = &u.u_procp->p_squpdate;
		LOCK(lockp, PLHI);
		LOCK(&lwp->l_mutex, PLHI);
		/*
		 * Need to check if we are still on the sync queue.
		 * If we are not on the queue, we were awarded the
		 * resource. Simply release all locks and return 0.
		 */
		if (!(SQ_ONQUEUE(sq, lwp))) {
			UNLOCK(&lwp->l_mutex, PLHI);
			UNLOCK(lockp, PLBASE);
			ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
			lwp->l_sq = NULL;
			lwp->l_flag &= ~L_ONGLOBSQ;
			return (0);
		}
		sq_delqueue(lwp, sq);
		ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
		lwp->l_sq = NULL;
		lwp->l_flag &= ~L_ONGLOBSQ;
		UNLOCK(&lwp->l_mutex, PLHI);
		sq_rele(sq, 1, lwp->l_waiterp);
				/* returns with update spin lock released.  */
	} else if (error == EINTR) {
		if (lwp->l_flag & L_SQTIMEDOUT) {
			/*
			 * timer expired after CL_BLOCK returned EINTR;
			 * clear the flag to prevent errors later 
			 */
			LOCK(&lwp->l_mutex, PLHI);
			lwp->l_flag &= ~L_SQTIMEDOUT;
			UNLOCK(&lwp->l_mutex, PLBASE);
		}
	}

	return(error);
}

struct rdblock_args {
	vaddr_t mutexp;
};

/*
 * int
 * rdblock(struct rdblock_args *, rval_t *rvp)
 *
 *	Determine if we should block against uap->sqid.
 *
 * Calling/Exit State:
 *
 *	This is a system call.
 *
 * Description:
 *
 *	Rdblock is called by a potential reader, when there are known to be
 *	writers waiting on the sync-queue.
 *
 *	Lwp's are queued against a reader/writer lock by their scheduling
 *	class.	When writers are waiting, a potential reader must call into
 *	the scheduling class in order to determine if they would be queued
 *	behind a writer.
 */
int
rdblock(struct rdblock_args *uap, rval_t *rvp)
{
	lwp_t *lwp = u.u_lwpp;
	int error;
	sq_t *sq = NULL;
	boolean_t addrvalid;
	uint_t	maptype;
	void	*vp;
	void	*off;
	pl_t	pl;
	lock_t *lockp;

	as_rdlock(u.u_procp->p_as);
	addrvalid = as_getwchan(uap->mutexp, &maptype, &vp, &off);
	if (addrvalid == B_FALSE) {  /* address of mutex not mapped in */
		as_unlock(u.u_procp->p_as);
		return (EFAULT);
	}
	as_unlock(u.u_procp->p_as);

	/*
	 * Find the sync queue.
	 * It returns with update lock
	 * still held.
	 */
	if (maptype == MAP_PRIVATE) {
		lockp = &u.u_procp->p_squpdate;
		LOCK(lockp, PLHI);
		sq = lsqfind(uap->mutexp);
	} else {
		lockp = &sq_update;
		LOCK(lockp, PLHI);
		sq = sqfind(vp, off);
	}

	if (sq == NULL) {
		/*
		 * No sync queue to block on, must have raced with
		 * a signal. Just return. Let the caller reenter.
		 */
		UNLOCK(lockp, PLBASE);
		rvp->r_val1 = 0;
		return (0);
	}

	/*
	 * Let the scheduling class decide if the caller should block.
	 */
	pl = LOCK(&lwp->l_mutex, PLHI);
	error = CL_URDBLOCK(lwp, sq, &rvp->r_val1);
	UNLOCK(&lwp->l_mutex, pl);

	UNLOCK(lockp, PLBASE);

	return(error);
}

struct unblock_args {
	vaddr_t	mutexp;
	char * waiterp;
	int flag;
};

/*
 * int
 * unblock(struct unblock_args *uap, rval_t *rvp)
 *
 *	Unblock one or all waiters blocked on the sync queue.
 *
 * Calling/Exit State:
 *
 *	This is a system call.
 *
 * Description:
 *
 *	This system call arranges for lwp's blocked on a sync queue
 *	to be activated.  The lwp's described by "uap->flag"
 *	are activated.	uap->flag must contain exactly one of:
 *
 *		UNBLOCK_ANY	When only one lwp of any class is
 *				to be activated.
 *		UNBLOCK_ALL	When all lwp's of any class are to be
 *				activated.
 *		UNBLOCK_READER	When only one lwp of the reader class
 *				is to be activated.
 *		UNBLOCK_WRITER	When only one lwp of the writer class
 *				is to be activated.
 *
 *	When uap->flag is or'd with UNBLOCK_RESET, it indicates that
 *	the user level flag that indicates whether the queue is empty
 *	has been blindly set to zero.  In this case, unblock must set
 *	the user level flag to non-zero if the queue is not empty;
 *	if uap->flag was not or'd with UNBLOCK_RESET, unblock must only
 *	set the user level flag to zero if the queue is empty.
 *
 *	The class specific unblock code returns an indication of the
 *	number of lwps woken up. If one or more lwps are woken up,
 *	unblock will call sq_rele() to decrement the count of the
 *	the sleepers on that sync queue and if the count drops to
 *	zero, sq_rele will deallocate the sync queue structure, if
 *	there is no racing lookup operation.
 */
int
unblock(struct unblock_args *uap, rval_t *rvalp)
{
	lwp_t *lwp = u.u_lwpp;
	int error = 0;
	sq_t *sq;
	boolean_t addrvalid;
	int ret, lwptypeflag;
	uint_t	maptype;
	void	*vp;
	void	*off;
	int count = 0;
	int extra = 0;
	lock_t *lockp;

	as_rdlock(u.u_procp->p_as);


	addrvalid = as_getwchan((vaddr_t)uap->waiterp, &maptype, &vp, &off);
	if (addrvalid == B_FALSE) {  /* address of waiter flag not mapped in */
		as_unlock(u.u_procp->p_as);
		return (EFAULT);
	}
	addrvalid = as_getwchan(uap->mutexp, &maptype, &vp, &off);
	if (addrvalid == B_FALSE) {  /* address of mutex not mapped in */
		as_unlock(u.u_procp->p_as);
		return (EFAULT);
	}

	as_unlock(u.u_procp->p_as);

	/*
	 * Find the sync queue.
	 * The 'find' functions return with the update lock still held,
	 * whether or not the sync queue is found.
	 */
	if (maptype == MAP_PRIVATE) {
		lockp = &u.u_procp->p_squpdate;
		LOCK(lockp, PLHI);
		sq = lsqfind(uap->mutexp);
	} else {
		lockp = &sq_update;
		LOCK(lockp, PLHI);
		sq = sqfind(vp, off);
	}

	if (sq == NULL) {
		UNLOCK(lockp, PLBASE);
		return (EINVAL);
	}

	if (uap->flag & UNBLOCK_RESET) {
		ASSERT(uap->flag == (UNBLOCK_ANY | UNBLOCK_RESET));
		/*
		 * If we get here, we were called by _lwp_mutex_unlock.  
		 * In this case, we must reset the user level flag 
		 * since this flag was cleared at user level to prevent a 
		 * possible memory fault.  _lwp_mutex_unlock clears the flag 
		 * in an atomic operation while clearing its lock to avoid 
		 * subsequently accessing memory associated with the mutex 
		 * because libthread uses _lwp_mutex_unlock on memory that may
		 * subsequently be unmapped.
		 *
		 * We would prefer to reset this flag in sq_rele() only if 
		 * there will be LWPs on the sleep queue after CL_UNBLOCK,
		 * but we must do it before CL_UNBLOCK to avoid losing the
		 * race with an awakened LWP which could otherwise find the 
		 * wanted flag unset and not call unblock, consequently 
		 * stranding any remaining LWPs on the sleep queue.
		 *
		 * Increment sq_refcnt to prevent deallocation of the
		 * queue by another LWP while we're waiting but not
		 * holding the spin lock.
		 */
		sq->sq_refcnt++;
		extra = 1;
		while (sq->sq_uflag) {
			if (sq->sq_flags & SQGLOBAL)
				SV_WAIT_SIG(&sq->sq_sv, PRINOD, 
			   	&sq_update);
			else
				SV_WAIT_SIG(&sq->sq_sv, PRINOD, 
			   	&u.u_procp->p_squpdate);
			/*
		  	* SV_WAIT_SIG automatically drops the spin lock.
		  	* Re-acquire the spin lock after coming
		  	* out of SV_WAIT_SIG.
		  	*/
			LOCK(lockp, PLHI);
		}
		sq->sq_uflag = 1;  /* set flag to "busy" */
		/* drop spin lock */
		UNLOCK(lockp, PLBASE);
		error = subyte(uap->waiterp, 1);
		/* re-acquire spin lock */
		LOCK(lockp, PLHI);
		/* now clear flag */
		sq->sq_uflag = 0;
		SV_BROADCAST(&sq->sq_sv, 0);
		lwptypeflag = (uap->flag & ~UNBLOCK_RESET);
	} else {
		lwptypeflag = uap->flag;
	}
	error = CL_UNBLOCK(lwp, sq, &count, lwptypeflag);
	count += extra;

	/*
	 * call sq_rele to update the count of the sleep queue if any
	 * LWPs were awakened or to deallocate the sleep queue if the 
	 * count is already zero.
	 */
	if (count >=1) {
		ret = sq_rele(sq, count, uap->waiterp);
		if (ret == -1)
			return (EFAULT);
	} else {
		UNLOCK(lockp, PLBASE);
	}

	return(error);
}

/*
 * int
 * cancelblock(void)
 *
 *	Unqueue an lwp from its user-mode sync-queue.
 *
 * Calling/Exit State:
 *
 *	This is a system call.
 *
 * Description:
 *
 *	Remove the user from its current sync-queue.
 *
 */
int
cancelblock()
{
	lwp_t *lwp = u.u_lwpp;
	sq_t *sq;
	int deqp = 0;
	lock_t *lockp;

	if ((sq = lwp->l_sq) == NULL)
		/*
		 * We haven't prepblocked. Simply return.
		 */
		return(0);

	if (lwp->l_flag & L_ONGLOBSQ) {
		lockp = &sq_update;
		LOCK(lockp, PLHI);
	} else {
		lockp = &u.u_procp->p_squpdate;
		LOCK(lockp, PLHI);
	}

	LOCK(&lwp->l_mutex, PLHI);
	CL_CANCELBLOCK(lwp, sq, &deqp);
	ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
	lwp->l_sq = NULL;
	lwp->l_flag &= ~L_ONGLOBSQ;
	UNLOCK(&lwp->l_mutex, PLHI);

	if (deqp == 1) { /* only if the lwp was actually dequeued */
		sq_rele(sq, 1, lwp->l_waiterp);
			/* returns with update lock released */
	} else { /* awarded resource, simply unlock update lock, and return */
		UNLOCK(lockp, PLBASE);
	}

	return(0);
}


/*
 * void sq_init(void)
 *
 *	Called to initialize global update lock and hash list.
 *
 * Calling/Exit State:
 *
 *	No locking required on entry.
 *
 * Description:
 *
 *	Called at startup. Initializes the global update spin lock
 *	and the global hash buckets.
 *
 */
void
sq_init(void)
{
	list_t *cur, *list_end;
	int i;

	LOCK_INIT(&sq_update, SQ_HIER, SQ_MINIPL, &sq_update_info, KM_NOSLEEP);

	/*
	 * Initialize the hash list.
	 */
	for (cur = sq_hash, list_end = cur + SQ_NHASH; cur < list_end; cur++) {
		INITQUE(cur);
	}
	/*
	 * Initialize semaphore sleep locks.
	 */
	for (i = 0; i < SEMA_HASHSIZE; i++)
		SLEEP_INIT(&sema_locks[i], (uchar_t) 0,
			&sema_locks_buf, KM_SLEEP);

	return;
}


/*
 * void sq_exit(lwp_t *lwp)
 *
 *	Perform necessary exit processing relative to user-mode synchronization.
 *
 * Calling/Exit State:
 *
 *	Called without holding any locks.
 *
 * Description:
 *
 *	The lwp in question must be "invisible" (i.e. not able to be located
 *	using any of the normal lists).
 *
 *	Each exiting lwp calls this function.
 */
void
sq_exit(lwp_t *lwp)
{
	sq_t *sq;
	pl_t pl;
	lock_t *lockp;

	ASSERT(KS_HOLD0LOCKS());
	if ((sq = lwp->l_sq) != NULL) {
		/*
		 * We may be "queued" on a sync-queue.	Need to release
		 * our claim to this sync-queue.  
		 */
		if (lwp->l_flag & L_ONGLOBSQ) {
			lockp = &sq_update;
			LOCK(lockp, PLHI);
		} else {
			lockp = &(lwp->l_procp->p_squpdate);
			LOCK(lockp, PLHI);
		}

		pl = LOCK(&lwp->l_mutex, PLHI);

		if (SQ_ONQUEUE(sq, lwp)) {
			/*
			 * The lwp is still on the queue, dequeue the
			 * lwp.
			 */
			sq_delqueue(lwp, sq);
			ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
			lwp->l_sq = NULL;
			lwp->l_flag &= ~L_ONGLOBSQ;
			UNLOCK(&lwp->l_mutex, pl);
			if (lwp->l_flag & L_ONSEMAQ)
				semasq_rele(sq, 1);
			else
				sq_rele(sq, 1, lwp->l_waiterp);
		} else {
			/*
			 * We were awarded the resource.
			 */
			UNLOCK(&lwp->l_mutex, PLHI);
			UNLOCK(lockp, PLBASE);
			
		}
	}

	return;
}


/*
 * void
 * sq_cancelblock(lwp_t *lwp, sq_t *sq, int *ret, int *deqp)
 *
 *	Remove the caller from its current sync-queue.
 *
 * Calling/Exit State:
 *
 *	Must be called with the caller's l_mutex and the update lock
 *	held.
 *
 * Description:
 *
 *	Remove "lwp" from, "sq".
 *
 *	This is a generic implementation of CL_CANCELBLOCK for most
 *	scheduling classes.
 */
void
sq_cancelblock(lwp_t *lwp, sq_t *sq, int *deqp)
{
	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	if (SQ_ONQUEUE(sq, lwp)) {
		sq_delqueue(lwp, sq);
		*deqp = 1;
	}

	return;
}

/*
 * void sq_wakeup(lwp_t *lwp, sq_t *sq)
 *
 *	Wake up an lwp from a sync-queue.
 *
 * Calling/Exit State:
 *
 *	Called with both lwp->l_mutex and update spin lock held.
 *
 * Description:
 *
 *	We remove the lwp from the appropriate sync-queue and
 *	if the lwp is blocked on a user-synchronization, we
 *	wake it up.
 */
void
sq_wakeup(lwp_t *lwp, sq_t *sq)
{
	ASSERT(LOCK_OWNED(&lwp->l_mutex));
	ASSERT(lwp->l_sq != NULL);
	ASSERT(lwp->l_sq == sq);

	/*
	 * Dequeue the lwp.
	 */
	sq_delqueue(lwp, sq);

	/*
	 * If the lwp is blocked on the synchronization, activate it.
	 */
	if (lwp->l_stat == SSLEEP && lwp->l_stype == ST_USYNC) {
		/*
		 * If not ST_USYNC, we're somewheres after the
		 * prepblock and before the block.
		 */
		lwp->l_stat = SRUN;
		lwp->l_syncp = NULL;
		lwp->l_flag &= ~L_SIGWOKE;
		lwp->l_stype = ST_NONE;

		CL_WAKEUP(lwp, lwp->l_cllwpp, 0);
	}

	return;
}

/*
 * int usync_unsleep(sq_t *sq, lwp_t *lwp)
 *
 *	Abnormally activate an lwp.
 *
 * Calling/Exit State:
 *
 *	Called with lwp->l_mutex held.
 *
 * Description:
 *
 *	We don't need to do anything here, as the lwp will be
 *	awakened by our caller (setrun).  We do not dequeue
 *	the lwp from the sync-object.  This is not necessary, as the
 *	lwp will determine if it should be dequeued or not.
 */
/* ARGSUSED */
int
usync_unsleep(sq_t *sq, lwp_t *lwp)
{
	ASSERT(LOCK_OWNED(&lwp->l_mutex));
	return(1);
}


/*
 * void sq_delqueue(lwp_t *lwp, sq_t *sq)
 *
 *	Do class indepent remove from the sync queue.
 *
 * Calling/Exit State:
 *
 *	Must be called with both update lock and lwp->l_mutex held.
 */
void
sq_delqueue(lwp_t *lwp, sq_t *sq)
{
	lwp_t *lp;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	if ((lp = lwp->l_sfwd) != NULL)
		lp->l_sbak = lwp->l_sbak;
	else
		sq->sq_tail = lwp->l_sbak;

	if ((lp = lwp->l_sbak) != NULL)
		lp->l_sfwd = lwp->l_sfwd;
	else
		sq->sq_head = lwp->l_sfwd;

	lwp->l_sfwd = NULL;
	lwp->l_sbak = NULL;

	lwp->l_flag &= ~L_ONSQ;
	lwp->l_flag &= ~L_ONSEMAQ;
}

/*
 * int sq_checksigs(lwp_t *lwp, sq_t *sq)
 *
 *	Determine if we've got a signal we need to process.
 *
 * Calling/Exit State:
 *
 *	Look at any signals pending against this lwp and return
 *	non-zero if the lwp should remove itself from the synchronization.
 */
/* ARGSUSED */
int
sq_checksigs(lwp_t *lwp, sq_t *sq)
{

	if (!QUEUEDSIG(lwp))
		return(0);

	/*
	 * Need to call into issig.  This may suspend us, thus, we
	 * must release all locks.
	 */
	UNLOCK(&lwp->l_mutex, PLBASE);
loop:
	switch (issig((lock_t *)0)) {
	case ISSIG_NONE:
		/* returns with l_mutex held, simply return 0 */
		return(0);
	case ISSIG_STOPPED:
		/* We were stopped and may have new reasons to stop. */
		goto loop;
	case ISSIG_SIGNALLED:
		break;
	default:
		/*
		 *+ The issig function returned an unexpected value.
		 *+ This indicates a kernel software problem.
		 */
		cmn_err(CE_PANIC, "unexpected return from issig");
		break;
	}

	LOCK(&lwp->l_mutex, PLHI);

	return(1);
}


/*
 * sq_t *
 * sqget(void **key1, void **key2)
 *
 *	Find or get a sync queue.
 *
 * Calling/Exit State:
 *
 *	Called with the update spin lock held.
 *	Returns with the update spin lock still held.
 *
 * Description:
 *
 *	This function first finds if the sync queue already exists.
 *	If it exists, a pointer to it is returned. If it does not
 *	exist, it allocates memory for the structure with KM_SLEEP
 *	option. After returning from kmem_alloc(), it is possible
 *	that the caller slept inside kmem_alloc() and in the 
 *	meantime, someone else allocated a sync queue for the same
 *	mutex. Therefore, it goes back and checks again if a sync
 *	queue exists. If it does, we lost the race and thus we simply
 *	return with the memory freed and with the pointer to sync
 *	queue set. If a sync queue still does not exist, it goes ahead
 *	and initializes the sync queue it allocated and return with the
 *	the update lock still held.
 */
STATIC sq_t *
sqget(void *key1, void *key2)
{
	sq_t *sqp;
	sq_t *sq;

	sq = sqfind(key1, key2);

	if (sq != NULL)	 /* sync queue exists */
		return (sq);
	UNLOCK(&sq_update, PLBASE);
	sqp = (sq_t *)kmem_zalloc(sizeof(sq_t), KM_SLEEP);
	LOCK(&sq_update, PLHI);
	sq = sqfind(key1, key2); /* check again */
	if (sq != NULL) {  /* lost the race */
		kmem_free(sqp, sizeof(sq_t));
		return (sq);
	}

	/*
	 * Now we add the new sync-queue to the hash list.
	 * We are still holding the sq_update spin lock.
	 */
	insque((list_t *)sqp, sq_hash[SQ_HASHFUNC(key1, key2)].rlink);

	/* Initialize sync variable */
	SV_INIT(&sqp->sq_sv);

	/*
	 * Initialize the sync queue.
	 */
	sqp->sq_key1 = key1;
	sqp->sq_key2 = key2;
	sqp->sq_flags |= SQGLOBAL;
	return (sqp);
}


/*
 * sq_t *
 * lsqget(vaddr_t mutexp)
 *
 *	Find or get a sync queue on the local list.
 *
 * Calling/Exit State:
 *
 *	Called with the local update spin lock held.
 *	Returns with the lock still held.
 *
 * Description:
 *
 *	This function first finds if the hash buckets exist. If not,
 *	memory for this is allocated with KM_SLEEP option. As it is
 *	possible that someone else allocated this while this caller
 *	is sleeping for allocating memory, we go back and check if
 *	that is indeed the case. If it is, the memory just allocated
 *	is released and the function continues. Next, it tries to
 *	find if the sync queue already exists. If it does,
 *	a pointer to it is returned. If it does not exist,
 *	it allocates memory for the structure, again with KM_SLEEP
 *	option. As before, there is the possibility of a race while
 *	caller sleeps in kmem_alloc(). Therefore, it goes back and
 *	checks if someone else allocated a sync queue. Again, if the 
 *	sync queue is found, the memory just allocated is released
 *	and a pointer to the sync queue is returned. If not, the
 *	memory allocated is initialized before returning.
 */
STATIC sq_t *
lsqget(vaddr_t mutexp)
{
	sq_t *sqp;
	sq_t *sq;
	proc_t *pp = u.u_procp;
	list_t *lhashp;
	list_t *cur, *list_end;

	if (pp->p_sqhashp == NULL) {  /* hash buckets don't exist */
		UNLOCK(&pp->p_squpdate, PLBASE);
		lhashp = (list_t *)kmem_zalloc(sizeof(list_t) * LSQ_NHASH,
				KM_SLEEP);
		LOCK(&pp->p_squpdate, PLHI);
		if (pp->p_sqhashp != NULL) { /* lost the race */
			kmem_free(lhashp, sizeof(list_t) * LSQ_NHASH);
		} else { /* did not lose race though we may have slept */
			pp->p_sqhashp = lhashp;
			/* initialize hash buckets */
			for (cur = lhashp, list_end = cur + LSQ_NHASH;
				cur < list_end; cur++)
				INITQUE(cur);
		}
	}

	sq = lsqfind(mutexp);

	if (sq != NULL)	 /* sync queue exists */
		return (sq);
	UNLOCK(&pp->p_squpdate, PLBASE);
	sqp = (sq_t *)kmem_zalloc(sizeof(sq_t), KM_SLEEP);
	LOCK(&pp->p_squpdate, PLHI);
	sq = lsqfind(mutexp);  /* check again */
	if (sq != NULL) {  /* lost the race */
		kmem_free(sqp, sizeof(sq_t));
		return (sq);
	}

	/*
	 * Now we add the new sync-queue to the linked list.
	 * We are still holding the sq_update spin lock.
	 */
	insque((list_t *)sqp,
		pp->p_sqhashp[LSQ_HASHFUNC(mutexp)].rlink);

	/* Initialize sync variable */
	SV_INIT(&sqp->sq_sv);

	/*
	 * Initialize the sync queue.
	 * Lock is already initialized.
	 */
	sqp->sq_mutexp = mutexp;
	return (sqp);
}


/*
 * void
 * sq_timedsetrun(lwp_t *lwp)
 *
 *	Set a flag in the lwp structure and setrun the named lwp.
 *
 * Calling/Exit State:
 *
 *	Called without any locks held.
 */
STATIC void
sq_timedsetrun(lwp_t *lwp)
{
        pl_t pl;

        pl = LOCK(&lwp->l_mutex, PLHI);
        if (lwp->l_stat == SSLEEP) {
		/* normal wakeup */
        	lwp->l_flag |= L_SQTIMEDOUT;
        	setrun(lwp);
        } else if (lwp->l_flag & L_ONSQ) {
			/*
			 * lwp is still on sleep queue but timer has 
			 * expired.  We must show that the timer has
			 * expired or the lwp may block forever if 
			 * it is transitioning from the stop state
			 * back to the sleep state.
			 */
			lwp->l_flag |= L_SQTIMEDOUT;
	}
	/* else raced with a normal wakeup or signal and lost the race */

        UNLOCK(&lwp->l_mutex, pl);
}

/*
 * sq_t *
 * lsqfind(vaddr_t mutexp)
 *
 *	Find the sync queue in the process-local list.
 *
 * Calling/Exit State:
 *
 *	Called with the process-private update spin lock held.
 *	Returns with the lock still held.
 */
STATIC sq_t *
lsqfind(vaddr_t mutexp)
{
	proc_t *procp = u.u_procp;
	sq_t *sq;
	list_t *lp;
	int i, index;

	lp = procp->p_sqhashp;
	if (lp == NULL)	 /* even hash queues don't exist */
		return (NULL);
	index = LSQ_HASHFUNC(mutexp);
	for (i = 0; i < index; i++)
		lp++;
	if (EMPTYQUE(lp)) {
		return (NULL);
	}
	sq = (sq_t *)lp->flink;
	while (sq != (sq_t *)lp) {
		if (sq->sq_mutexp == mutexp) { /* found it */
			return (sq);
		}
		sq = (sq_t *)sq->sq_hnext;
	}
	/*
	 * Could not find it.
	 */
	return (NULL);
}

/*
 * sq_t *
 * sqfind(void **key1, void **key2)
 *
 *	Find the sync queue in the global hash list.
 *
 * Calling/Exit State:
 *
 *	Called with the global update spin lock held.
 *	Returns with the lock still held.
 */
STATIC sq_t *
sqfind(void *key1, void *key2)
{
	sq_t *sq;
	list_t *lp;

	lp = &sq_hash[SQ_HASHFUNC(key1, key2)];
	if (EMPTYQUE(lp)) {
		return (NULL);
	}
	sq = (sq_t *)lp->flink;
	while (sq != (sq_t *)lp) {
		if (sq->sq_key1 == key1 &&
			sq->sq_key2 == key2) {	/* found it */
			return (sq);
		}
		sq = (sq_t *)sq->sq_hnext;
	}
	/*
	 * Could not find it.
	 */
	return (NULL);
}


/*
 * int
 * sq_hold(sq_t *sq, char *waiterp)
 *
 *	increment the reference count of the sync queue
 *
 * Calling/Exit State:
 *
 *	Called with the update spin lock held.
 *	Returns with the lock still held.
 *
 * Description:
 *
 *	This function increments the reference count for the
 *	sync queue. If the reference count is 1 (first caller)
 *	then the "waiter" flag in the user address space is set
 *	to 1. While writing to user space, a flag in sync queue
 *	(sq_uflag) is set, indicating that if someone else is
 *	trying to write to the user flag, the other writer must
 *	wait till this one is done writing.
 */
STATIC int
sq_hold(sq_t *sq, char *waiterp)
{
	int error = 0;
	lock_t *lockp;

	if (sq->sq_flags & SQGLOBAL)
		lockp = &sq_update;
	else
		lockp = &u.u_procp->p_squpdate;
	ASSERT(LOCK_OWNED(lockp));
		

	/* increment the reference count */
	sq->sq_refcnt++;

	if ((sq->sq_refcnt == 1) || QUEUEISEMPTY(sq)) {
		while (sq->sq_uflag) {
			if (sq->sq_flags & SQGLOBAL)
				SV_WAIT_SIG(&sq->sq_sv, PRINOD, &sq_update);
			else
				SV_WAIT_SIG(&sq->sq_sv, PRINOD, &u.u_procp->p_squpdate);
			/*
			 * SV_WAIT_SIG automatically drops the spin lock.
			 * Re-acquire the spin lock after coming
			 * out of SV_WAIT_SIG.
			 */
			LOCK(lockp, PLHI);
		}
		sq->sq_uflag = 1;  /* set flag to "busy" */
		UNLOCK(lockp, PLBASE);
		error = subyte(waiterp, 1);
		/* re-acquire spin lock */
		LOCK(lockp, PLHI);
		sq->sq_uflag = 0;  /* clear flag - "not busy" */
		SV_BROADCAST(&sq->sq_sv, 0);
	}
	return (error);
}

/*
 * int
 * sq_rele(sq_t *sq, int count, char *waiterp)
 *
 *	decrement the reference count of the sync queue,
 *	deallocate if reference count drops to zero.
 *
 * Calling/Exit State:
 *
 *	Called with the update spin lock held.
 *	Returns with the update spin lock released at PLBASE.
 *
 * Description:
 *
 *	As with sq_hold, sq_uflag is set/cleared to single-thread
 *	writing to user flag (waiter flag).
 */
STATIC int
sq_rele(sq_t *sq, int count, char *waiterp)
{
	int error = 0;
	lock_t *lockp;

	ASSERT(sq->sq_refcnt >= count);

	if (sq->sq_flags & SQGLOBAL)
		lockp = &sq_update;
	else
		lockp = &u.u_procp->p_squpdate;
	ASSERT(LOCK_OWNED(lockp));

	sq->sq_refcnt -= count;
	if (sq->sq_refcnt == 0) {
		ASSERT (QUEUEISEMPTY(sq));
		while (sq->sq_uflag) {
			if (sq->sq_flags & SQGLOBAL)
				SV_WAIT_SIG(&sq->sq_sv, PRINOD, 
				   &sq_update);
			else
				SV_WAIT_SIG(&sq->sq_sv, PRINOD, 
				   &u.u_procp->p_squpdate);
			/*
		 	* SV_WAIT_SIG automatically drops the spin lock.
		 	* Re-acquire the spin lock after coming
		 	* out of SV_WAIT_SIG.
		 	*/
			LOCK(lockp, PLHI);
		}

		sq->sq_uflag = 1;  /* set flag to "busy" */
		if (sq->sq_refcnt == 0) {
			ASSERT (QUEUEISEMPTY(sq));
			/* drop spin lock */
			UNLOCK(lockp, PLBASE);
			error = subyte(waiterp, 0);
			/* re-acquire spin lock */
			LOCK(lockp, PLHI);
		}
		/* now clear flag */
		sq->sq_uflag = 0;

		if (sq->sq_refcnt == 0) {
			ASSERT (QUEUEISEMPTY(sq));
			remque((list_t *)sq);
			kmem_free(sq, sizeof(sq_t));
		} else {
			SV_BROADCAST(&sq->sq_sv, 0);
		}
	}
	/*
	 * drop the update lock before returning.
	 */
	UNLOCK(lockp, PLBASE);

	return (error);
}


/*
 * void
 * sq_hashfree(proc_t *procp)
 *
 *	Deallocate hash bucket memory, if allocated.
 *
 * Calling/Exit State:
 *
 *	Called without holding any locks.
 *	Only one lwp left in the process when this is called
 *	and therefore, there is no need to hold any locks.
 */
void
sq_hashfree(proc_t *procp)
{
	list_t *cur, *list_end;
	list_t *psp = procp->p_sqhashp;

	if (psp == NULL) { 
		return;
	}

#ifdef DEBUG
	for (cur = psp, list_end = cur + LSQ_NHASH; cur < list_end; cur++) {
		ASSERT(EMPTYQUE(cur));
	}
#endif

	/* deallocate memory associated with hash buckets */
	kmem_free(psp, sizeof(list_t) * LSQ_NHASH);
	return;
}


/*
 * int sq_unblock(lwp_t *lwp, sq_t *sq, int *count, int flag)
 *
 *	Unblock lwp's waiting on "*sq".
 *
 * Calling/Exit State:
 *
 *	Called with update lock held.
 *
 * Description:
 *
 *	Wakeup somebody, depending on "flag".  If "flag" is ALL, we wake
 *	up everybody.  If "flag" is ANY, we wake up one lwp of any type,
 *	if "flag" is READER or WRITER, we wakeup one lwp if they're of
 *	that type.
 */
/* ARGSUSED */
int
sq_unblock(lwp_t *lwp, sq_t *sq, int *count, int flag)
{
	lwp_t *wake;
	pl_t pl;
	int any;
	int error = 0;
	int wakecnt = 0;  /* number of lwps woken up */

	/*
	 * See if anybody's there.
	 */
	if ((wake = sq->sq_head) == NULL) {
		/*
		 * Nobody there to wake up.  Must have raced with a
		 * signal.  We don't signal this as an error, just indicate
		 * the queue must be deallocated.
		 */
		*count = 0;
		return (0);
	}

	/*
	 * Wake the first lwp up.  The values for flag are:  all (unblock
	 * everybody), any (unblock 1 of anybody), reader (unblock 1 reader)
	 * and writer (unblock 1 writer).  Thus, only "all" implies multiple
	 * waiters could be unblocked.
	 */
	any = flag == UNBLOCK_ALL || flag == UNBLOCK_ANY;

	pl = LOCK(&wake->l_mutex, PLHI);

	if (!any && wake->l_ublocktype != flag) {
		/*
		 * Flag specifies 1 lwp of a particular class should
		 * be awoken and the head of the queue isn't one of these.
		 * Let the lwp go and signal this with an error.
		 */
		*count = 0;
		UNLOCK(&wake->l_mutex, pl);
		return (ENOENT);
	}

	/*
	 * Wake up the first lwp.
	 */
	sq_wakeup(wake, sq);
	wakecnt++;
	UNLOCK(&wake->l_mutex, pl);

	if (flag == UNBLOCK_ALL) {
		/*
		 * Flag specifies that all waiters should be activated.
		 */
		while ((wake = sq->sq_head) != NULL) {
			pl = LOCK(&wake->l_mutex, PLHI);
			sq_wakeup(wake, sq);
			wakecnt++;
			UNLOCK(&wake->l_mutex, pl);
		}
	}
	*count = wakecnt;

	return (error);
}


/*
 * int sq_block(lwp_t *lwp, sq_t *sq)
 *
 *	Block an lwp.
 *
 * Calling/Exit State:
 *
 *	Called and returns with the l_mutex held, but
 *	will most-likely release and regain this lock during processing.
 *
 *	Block the current lwp ("lwp") against the sync-queue pointed to by
 *	"sq".  We have already been placed upon the sync-queue by the
 *	previous prepblock.
 *
 * Description:
 *
 *	sq_block simply checks for signals, timeout or wakeup before
 *	switching out, as we may have been activated before we had
 *	a chance to sleep. These conditions are checked after coming
 *	out of sleep to check what caused us to wake up.
 */
int
sq_block(lwp_t *lwp, sq_t *sq)
{
	int error = 0;
	proc_t *p = u.u_procp;

	ASSERT(LOCK_OWNED(&lwp->l_mutex));

	for(;;) {
		if (sq_checksigs(lwp, sq)) {
			/*
			 * We've got a signal.
			 */
			error = EINTR;
			lwp->l_flag &= ~L_SQTIMEDOUT; /* in case of race
						       * with timeout */
			break;
		}
		/*
		 * Check if we were setrun by timeout set by block().
		 */
		if ((lwp->l_flag & L_SQTIMEDOUT) != 0) {
			lwp->l_flag &= ~L_SQTIMEDOUT;
			error = ETIME;
			break;
		}

		if (!SQ_ONQUEUE(sq, lwp))
			/*
			 * We've been bumped off the sync queue.
			 */
			break;

		/* 
		 * No signal is pending, so block the calling context.
		 * If we are the last context in the process to block
		 * interruptibly, send SIGWAITING if the process is
		 * catching that signal and it is not already pending.
		 */

		FSPIN_LOCK(&p->p_niblk_mutex);
		++p->p_niblked;
		if ((p->p_niblked >= p->p_nlwp) && p->p_sigwait &&
		    !sigismember(&p->p_sigignore, SIGWAITING) &&
		    !sigismember(&p->p_sigs, SIGWAITING)) {
			/*
			 * There is a possibility that we may be the last
			 * context in the process to block interruptibly.  Send
			 * SIGWAITING.  It is possible that we may not block or
			 * that a different context in the process could wake
			 * up even as we are preparing to send
			 * SIGWAITING.	This is a harmless race and cannot be
			 * closed; the worst that can happen is that a signal
			 * will be sent when none should have been.
			 */
			--p->p_niblked;
			FSPIN_UNLOCK(&p->p_niblk_mutex);
			UNLOCK(&lwp->l_mutex, PLBASE);
			sigtoproc(p, SIGWAITING, (sigqueue_t *)NULL);
			LOCK(&lwp->l_mutex, PLHI);
			continue; /* go to beginning of 'for' loop */
		}

		FSPIN_UNLOCK(&p->p_niblk_mutex);

		lwp->l_stype = ST_USYNC;
		lwp->l_syncp = (void *)sq;
		lwp->l_stat = SSLEEP;
		lwp->l_flag &= ~L_NWAKE;
		lwp->l_slptime = 0;

		swtch(lwp);

		LOCK(&lwp->l_mutex, PLHI);

		FSPIN_LOCK(&p->p_niblk_mutex);
		--p->p_niblked;
		FSPIN_UNLOCK(&p->p_niblk_mutex);	
	}

	return (error);
}



/*
 *    LWP SEMAPHORES
 *    --------------
 */

struct lwp_sema_wait_args {
	_lwp_sema_t *semap;
};

/*
 * int
 * _lwp_sema_wait(struct lwp_sema_wait_args *uap, rval_t *rvp)
 *
 *	Perform P operation on the semaphore.
 *
 * Calling/Exit State:
 *
 *	system call.
 *
 * Description:
 *
 *	This system call attempts to acquire the semaphore pointed
 *	to by semap by decrementing the semaphore count. If the
 *	resulting value is greater than or equal to zero, it returns
 *	to the caller having successfully acquired the semaphore.
 *	If the resulting count is negative, it suspends the execution
 *	of the caller and places it on a sync queue associated with
 *	the semaphore. It remains suspended until awakened by another
 *	LWP performing a _lwp_sema_post on the same semaphore or
 *	by a signal. If interrupted by a signal, it returns the caller
 *	to user level with a return value of EINTR, and the semaphore
 *	count is incremented by 1.
 */
/* ARGSUSED */
int
_lwp_sema_wait(struct lwp_sema_wait_args *uap, rval_t *rvp)
{
	boolean_t addrvalid;
	uint_t	maptype;
	void	*vp;
	void	*off;
	int	error;
	int	ret;
	int	lcount;
	int	hashval;
	lock_t	*lockp;
	lwp_t *lwp = u.u_lwpp;
	sq_t	*sq;

	
	as_rdlock(u.u_procp->p_as);
	addrvalid = as_getwchan((vaddr_t)uap->semap, &maptype, &vp, &off);
	if (addrvalid == B_FALSE) {  /* address of semaphore not mapped in */
		as_unlock(u.u_procp->p_as);
		return (EFAULT);
	}

	as_unlock(u.u_procp->p_as);

	/*
	 * Acquire the semaphore sleep lock and check semaphore count.
	 */

	if (maptype == MAP_PRIVATE)
		hashval = SEMA_HASHFUNC(NULL, uap->semap);
	else
		hashval = SEMA_HASHFUNC(vp, off);

	SLEEP_LOCK(&sema_locks[hashval], PRINOD);

	if (copyin(&uap->semap->count, &lcount, sizeof(int))) {
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (EFAULT);
	}

	/*
	 * Decrement the count and write to user level.
	 */
	lcount--;
	if ((suword((int *)&uap->semap->count, lcount)) == -1) {
		/*
		 * Perhaps semaphore is in read-only page.
		 */
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (EFAULT);
	}

	if (lcount >= 0) { /* got the semaphore */
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (0);
	}

	/*
	 * Could not get the semaphore. Need to block in the kernel.
	 * The semaphore sleep lock will not be released till this
	 * caller is queued on the sync queue where a subsequent
	 * lwp_sema_post could find it.
	 */
	
	/*
	 * Check if the caller is already enqueued on another
	 * sync queue. If so, the caller is dequeued from that 
	 * sync queue before being enqueued on the new one.
	 */

	if (lwp->l_sq != NULL) {
		if (lwp->l_flag & L_ONGLOBSQ) {
			lockp = &sq_update;
			LOCK(lockp, PLHI);
		} else {
			lockp = &u.u_procp->p_squpdate;
			LOCK(lockp, PLHI);
		}
		LOCK(&lwp->l_mutex, PLHI);
		CL_CANCELBLOCK(lwp, lwp->l_sq, &ret);
		lwp->l_flag &= ~L_ONGLOBSQ;
		UNLOCK(&lwp->l_mutex, PLHI);

		if (ret == 1) {	 /* actually dequeued */
			/*
			 * sq_rele will return with update lock
			 * released at PLBASE.
			 */
			sq_rele(lwp->l_sq, 1, lwp->l_waiterp);
		} else
			UNLOCK(lockp, PLBASE);
		ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
		lwp->l_sq = NULL;
	} 


	/*
	 * Find or allocate a sync queue.
	 * If a sync queue is not found, one is allocated
	 * and appended to the hash/linked queue.
	 */
	if (maptype == MAP_PRIVATE) {
		lockp = &u.u_procp->p_squpdate;
		LOCK(lockp, PLHI);
		sq = (sq_t *)lsqget((vaddr_t)uap->semap);
	} else {
		lockp = &sq_update;
		LOCK(lockp, PLHI);
		sq = (sq_t *)sqget(vp, off);
	}

	semasq_hold(sq);

	LOCK(&lwp->l_mutex, PLHI);

	/*
	 * CL_PREPBLOCK places the lwp on the sync queue.
	 * The order of queueing depends on scheduling class.
	 */
	error = CL_PREPBLOCK(lwp, sq, UNBLOCK_ANY);
	lwp->l_sq = sq;
	lwp->l_flag |= L_ONSEMAQ;
	lwp->l_flag |= L_ONSQ;
	if (maptype != MAP_PRIVATE)
		lwp->l_flag |= L_ONGLOBSQ;
	UNLOCK(&lwp->l_mutex, PLHI);
	UNLOCK(lockp, PLBASE);
	
	/*
	 * Now, it is safe to drop the semaphore sleep lock.
	 */
	SLEEP_UNLOCK(&sema_locks[hashval]);

	LOCK(&lwp->l_mutex, PLHI);
	error = CL_BLOCK(lwp, sq);
	UNLOCK(&lwp->l_mutex, PLBASE);

	if (error == 0) {
		LOCK(&lwp->l_mutex, PLHI);
		ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
		lwp->l_sq = NULL;
		lwp->l_flag &= ~L_ONGLOBSQ;
		UNLOCK(&lwp->l_mutex, PLBASE);
	} else {
		/*
		 * Got a signal. Must go to user level to handle it.
		 * There is a race condition here. Someone
		 * could have awakened us from the sleep queue
		 * just as we are backing out of CL_BLOCK (due
		 * to the signal we received). Hence, we must
		 * check again, after acquiring the sleep lock,
		 * to see if we are still enqueued. If still
		 * enqueued, we dequeue ourselves and copyout
		 * incremented count and return EINTR. If we
		 * were awakened by a racing LWP, we simply
		 * return 0.
		 */
		SLEEP_LOCK(&sema_locks[hashval], PRINOD);
		LOCK(lockp, PLHI);
		LOCK(&lwp->l_mutex, PLHI);
		if (!(SQ_ONQUEUE(sq, lwp))) {
			/*
			 * We are awarded the resource.
			 */
			ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
			lwp->l_sq = NULL;
			lwp->l_flag &= ~L_ONGLOBSQ;
			UNLOCK(&lwp->l_mutex, PLHI);
			UNLOCK(lockp, PLBASE);
			SLEEP_UNLOCK(&sema_locks[hashval]);
			return (0);
		}
		UNLOCK(&lwp->l_mutex, PLHI);
		UNLOCK(lockp, PLBASE);

		if ( !(copyin(&uap->semap->count, &lcount, sizeof(int)))) {
			/*
			 * Copyin successful. Increment the count and
			 * write back to user level.
			 *
			 * (if copyin fails, we cannot help keep
			 * the semaphore count consistent. We simply
			 * do not write back incremented count to user
			 * space, but continue with other operations.)
			 */
			lcount++;
			suword((int *)&uap->semap->count, lcount);
		}
		/*
		 * Now, dequeue the lwp.
		 */
		LOCK(lockp, PLHI);
		LOCK(&lwp->l_mutex, PLHI);
		ASSERT(SQ_ONQUEUE(sq, lwp));
		sq_delqueue(lwp, sq);
		ASSERT(!(SQ_ONQUEUE(lwp->l_sq, lwp)));
		lwp->l_sq = NULL;
		lwp->l_flag &= ~L_ONGLOBSQ;
		UNLOCK(&lwp->l_mutex, PLHI);
		semasq_rele(sq, 1); /* releases lockp */
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (EINTR);
	}
	return (error);

}

struct lwp_sema_post_args {
	_lwp_sema_t *semap;
};


/*
 * int
 * _lwp_sema_post(struct lwp_sema_post_args *uap, rval_t *rvp)
 *
 *	Release a semaphore (increment count), if already locked.
 *
 * Calling/Exit State:
 *
 *	System Call.
 *
 * Description:
 *
 *	This system call releases the resource under the control
 *	of the semaphore pointed to by semap. The semaphore count
 *	is incremented by 1 and if the new value is less than or
 *	equal to zero, CL_UNBLOCK is called to awaken an appropriate
 *	blocked LWP.
 *
 */
/* ARGSUSED */
int
_lwp_sema_post(struct lwp_sema_post_args *uap, rval_t *rvp)
{
	boolean_t addrvalid;
	uint_t	maptype;
	void	*vp;
	void	*off;
	int	lcount;
	int	wakecount;
	int	hashval;
	lock_t	*lockp;
	sq_t	*sq;
	lwp_t	*lwp = u.u_lwpp;

	as_rdlock(u.u_procp->p_as);
	addrvalid = as_getwchan((vaddr_t)uap->semap, &maptype, &vp, &off);
	if (addrvalid == B_FALSE) {  /* address of semaphore not mapped in */
		as_unlock(u.u_procp->p_as);
		return (EFAULT);
	}
	as_unlock(u.u_procp->p_as);

	/*
	 * Acquire the semaphore sleep lock and get semaphore count.
	 */

	if (maptype == MAP_PRIVATE)
		hashval = SEMA_HASHFUNC(NULL, uap->semap);
	else
		hashval = SEMA_HASHFUNC(vp, off);
	SLEEP_LOCK(&sema_locks[hashval], PRINOD);
	if (copyin(&uap->semap->count, &lcount, sizeof(int))) {
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (EFAULT);
	}
	lcount++;
	if (copyout(&lcount, &uap->semap->count, sizeof(int))) {
		/*
		 * Perhaps semaphore is in read-only page.
		 */
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (EFAULT);
	}

	if (lcount > 0) { /* no sleepers to wake up */
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (0);
	}

	/*
	 * There are sleepers on the semaphore. Need to wakeup one.
	 */

	/*
	 * Find the sync queue.
	 * The 'find' functions return with the update lock still held,
	 * whether or not the sync queue is found.
	 */
	if (maptype == MAP_PRIVATE) {
		lockp = &u.u_procp->p_squpdate;
		LOCK(lockp, PLHI);
		sq = lsqfind((vaddr_t)uap->semap);
	} else {
		lockp = &sq_update;
		LOCK(lockp, PLHI);
		sq = sqfind(vp, off);
	}

	if (sq == NULL) {
		UNLOCK(lockp, PLBASE);
		SLEEP_UNLOCK(&sema_locks[hashval]);
		/*
		 * This should not happen unless the user changes the
		 * semaphore count.
		 */
		return (0);
	}

	CL_UNBLOCK(lwp, sq, &wakecount, UNBLOCK_ANY);
	/*
	 * Now, it is OK to release the semaphore sleep lock.
	 */
	SLEEP_UNLOCK(&sema_locks[hashval]);

	ASSERT(wakecount == 1);

	semasq_rele(sq, wakecount);

	return(0);
}

struct lwp_sema_trywait_args {
	_lwp_sema_t *semap;
};

/*
 * int
 * _lwp_sema_trywait(struct lwp_sema_trywait_args *uap, rval_t *rvp)
 *
 *	Try to get the semaphore.
 *
 * Calling/Exit State:
 *
 *	System call.
 *
 * Description:
 *
 *	This system call makes a single attempt to acquire the
 *	semaphore pointed to by semap, but in case the semaphore
 *	is unavailable, will return EBUSY instead of blocking.
 */
/* ARGSUSED */
int
_lwp_sema_trywait(struct lwp_sema_trywait_args *uap, rval_t *rvp)
{
	boolean_t addrvalid;
	uint_t	maptype;
	void	*vp;
	void	*off;
	int	hashval;
	int	lcount;

	as_rdlock(u.u_procp->p_as);
	addrvalid = as_getwchan((vaddr_t)uap->semap, &maptype, &vp, &off);
	if (addrvalid == B_FALSE) {  /* address of semaphore not mapped in */
		as_unlock(u.u_procp->p_as);
		return (EFAULT);
	}
	as_unlock(u.u_procp->p_as);

	/*
	 * Acquire the semaphore sleep lock and get semaphore count.
	 */

	if (maptype == MAP_PRIVATE)
		hashval = SEMA_HASHFUNC(NULL, uap->semap);
	else
		hashval = SEMA_HASHFUNC(vp, off);
	SLEEP_LOCK(&sema_locks[hashval], PRINOD);
	if (copyin(&uap->semap->count, &lcount, sizeof(int))) {
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (EFAULT);
	}
	lcount--;
	if (lcount >= 0) {
		/*
		 * Got the semaphore.
		 */
		if (copyout(&lcount, &uap->semap->count, sizeof(int))) {
			/*
			 * Perhaps a read-only page.
			 */
			SLEEP_UNLOCK(&sema_locks[hashval]);
			return (EFAULT);
		}
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (0);
	} else {
		SLEEP_UNLOCK(&sema_locks[hashval]);
		return (EBUSY);
	}
}


/*
 * int
 * semasq_hold(sq_t *sq)
 *
 *	increment the reference count of the semaphore sync queue
 *
 * Calling/Exit State:
 *
 *	Called with the update spin lock held.
 *	Returns with the lock still held.
 *
 * Description:
 *
 *	This function increments the reference count for the
 *	sync queue.
 */
STATIC int
semasq_hold(sq_t *sq)
{
	lock_t *lockp;

	if (sq->sq_flags & SQGLOBAL)
		lockp = &sq_update;
	else
		lockp = &u.u_procp->p_squpdate;
	ASSERT(LOCK_OWNED(lockp));
		

	/* increment the reference count */
	sq->sq_refcnt++;
	return (0);
}

/*
 * int
 * semasq_rele(sq_t *sq, int count)
 *
 *	decrement the reference count of the semaphore sync queue,
 *	deallocate if reference count drops to zero.
 *
 * Calling/Exit State:
 *
 *	Called with the update spin lock held.
 *	Returns with the update spin lock released at PLBASE.
 *
 * Description:
 *
 *	This function decrements the reference count by the count
 *	supplied as the second argument. If the reference count
 *	drops to zero, the sync queue structure is dequeued and
 *	deallocated. The function returns with the update lock
 *	released at PLBASE.
 */
STATIC int
semasq_rele(sq_t *sq, int count)
{
	lock_t *lockp;

	ASSERT(sq->sq_refcnt >= count);

	if (sq->sq_flags & SQGLOBAL)
		lockp = &sq_update;
	else
		lockp = &u.u_procp->p_squpdate;
	ASSERT(LOCK_OWNED(lockp));

	sq->sq_refcnt -= count;
	if (sq->sq_refcnt == 0) {
		remque((list_t *)sq);
		kmem_free(sq, sizeof(sq_t));
	}
	/*
	 * drop the update lock before returning.
	 */
	UNLOCK(lockp, PLBASE);

	return (0);
}
