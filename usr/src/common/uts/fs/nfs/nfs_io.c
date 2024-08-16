/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/nfs/nfs_io.c	1.39"
#ident	"$Header: $"

/*
 *	nfs_io.c, routines pertaining to actual nfs io.
 */

#include <fs/buf.h>
#include <fs/nfs/nfs_clnt.h>
#include <fs/nfs/rnode.h>
#include <fs/nfs/nfslk.h>
#include <fs/vfs.h>
#include <fs/fs_hier.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/pvn.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <mem/seg_vn.h>
#include <mem/vmmeter.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/exec.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <proc/resource.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <svc/time.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/ksynch.h>

/*
 * struct to pass to itimeout in nfs_async_start.
 */
struct nfs_argtotimeout {
	int		tm_timeup;	/* indicates timeup */
	struct lwp	*tm_lwpp;	/* pointer to lwp */
};

extern	lock_t		nfs_async_lock;
extern	lock_t		nfs_mnt_lock;
extern	sv_t		nfs_asynclwp_sv;
extern	sv_t		nfs_mmaplwp_sv;
extern	sv_t		nfs_asyncd_sv;
extern	struct mntinfo	*nfs_mnt_list;
extern	int		nfs_async_max;
extern	int		nfs_async_timeout;
extern	int		nfs_mmap_timeout;

extern	int		nfs_async_total;
extern	int		nfs_async_currmax;
extern	int		nfs_mmap_alive;

extern	void		printfhandle(caddr_t);
extern	void		nfs_mmap_sync();
extern	void		nfs_lwp_exit(int, pl_t);
extern	void		itimeout_free(void *);

void			nfs_async_start(void *);
int			do_bio(struct buf *);
void			nfs_async_signal(struct nfs_argtotimeout *);
void			relvm_noswap();
void			nfs_mmaplwp_start(void *);
void			nfs_mmap_signal(void *);
int			nfswrite(vnode_t *, caddr_t, off_t, long, cred_t *);
int			nfsread(vnode_t *, caddr_t, off_t, long, long *,
					cred_t *, vattr_t *);

int			nfs_async_sum;
int			nfs_async_sleeping;
int			nfs_async_waking;
int			nfs_async_sigcnt;

/*
 * nfs async_stats, protected by nfs_async_lock
 */
struct nfs_async_stats {
	int	st_total;		/* async reqs */
	int	st_orgsync;		/* sync reqs */
	int	st_syncedmax;		/* async reqs made sync, max lwps */
	int	st_sigwake;		/* total wakeups issued for lwps */
	int	st_sigasyncd;		/* # of times asyncd signalled */
	int	st_synceddef;		/* async reqs made sync, default */
	int	st_asyncdsleep;		/* times asyncd found asleep lwps */
	int	st_created;		/* times lwps created */
	int	st_creatfail;		/* times lwp create failed */
	int	st_exited;		/* times lwps exited */
} nfs_async_stats;

/*
 * macro used by biod to wait or to exit. nfs_async_lock must be held
 * before this.
 */
#define	NFSBIOD_WAIT()							\
{									\
	if (nfs_async_sigcnt == 0) {					\
		/*							\
		 * no more signals to nfsbiod. wait.			\
		 */							\
		if (SV_WAIT_SIG(&nfs_asyncd_sv, PRIMED,			\
				&nfs_async_lock) == B_FALSE) {		\
			/*						\
			 * sleep was interrupted, cleanup and exit.	\
			 */						\
			opl = LOCK(&nfs_async_lock, PLMIN);		\
									\
			/*						\
			 * we will set the max lwps to zero		\
			 * so that no new ones are created.		\
			 */						\
			nfs_async_currmax = 0;				\
			nfs_lwp_exit(NFS_BIOD, opl);			\
			/* NOTREACHED */				\
		}							\
	} else {							\
		UNLOCK(&nfs_async_lock, opl);				\
	}								\
}									\


/*
 * nfs_strategy(struct buf *bp)
 *	Start async or sync io for caller.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Starts async or sync io for caller. If async,
 *	and sleeping async lwps, then wake one up, otherwise
 *	if can create a new async lwp, signal nfsbiod.
 *	Is called when any actual io is desired, in
 *	user context or pageout context.
 *
 * Parameters:
 *
 *	bp			# buffer containing io info.
 *
 */
int
nfs_strategy(struct buf *bp)
{
	struct	vnode	*vp;
	struct	mntinfo	*mi;
	pl_t		opl1;
	pl_t		opl2;

	vp = (struct vnode *)bp->b_priv.un_ptr;
	mi = vtomi(vp);

	NFSLOG(0x100, "nfs_strategy bp %x lbn %d\n", bp, bp->b_blkno);
	NFSLOG(0x100, "nfs_strategy vp %x mi %x\n", vp, mi);
	NFSLOG(0x100, "nfs_strategy mi->mi_bufhead %x\n", mi->mi_bufhead, 0);
	NFSLOG(0x100, "nfs_strategy bp->b_pages %x\n", bp->b_pages, 0);

	if (bp->b_flags & B_ASYNC) {
		NFSLOG(0x100, "nfs_strategy async req\n", 0, 0);

		/*
		 * grab the global async and mount specific locks.
		 */
		opl1 = LOCK(&nfs_async_lock, PLMIN);
		opl2 = LOCK(&mi->mi_async_lock, PLMIN);

		ASSERT((nfs_async_currmax == 0) ||
				(nfs_async_total <= nfs_async_currmax));
		ASSERT((nfs_async_currmax == 0) ||
				(nfs_async_sleeping <= nfs_async_currmax));
		ASSERT(nfs_async_waking <= nfs_async_sleeping);
		ASSERT(nfs_async_sleeping >= 0);
		ASSERT(nfs_async_total >= 0);
		ASSERT(nfs_async_waking >= 0);
		ASSERT(mi->mi_rlwps <= mi->mi_lwpsmax);

		nfs_async_stats.st_total++;

		if (mi->mi_rlwps == mi->mi_lwpsmax) {
			nfs_async_stats.st_syncedmax++;
			/*
			 * already max lwps are running on this mount.
			 * release locks and change req to sync.
			 *
			 * there is a benign race here with lwps waiting
			 * for the mi_async_lock, to get off of this
			 * mount point.
			 *
			 * XXX: this is needed to limit queing for this
			 * mount and avoid memory depletion in the kernel.
			 */
			UNLOCK(&mi->mi_async_lock, opl2);
			UNLOCK(&nfs_async_lock, opl1);

			NFSLOG(0x100, "nfs_strategy: change to sync\n", 0, 0);

			return (do_bio(bp));
		}

		if (nfs_async_waking < nfs_async_sleeping) {
			nfs_async_stats.st_sigwake++;
			/*
			 * there are sleeping lwps which have not been
			 * signalled yet. queue the request, release the
			 * mi lock, signal one lwp and THEN release the
			 * nfs_async_lock so as to avoid racing with the lwp.
			 * 
			 */
			bbackfree(bp, mi->mi_bufhead);
			mi->mi_asyncreq_count++;
			nfs_async_waking++;
			UNLOCK(&mi->mi_async_lock, opl2);
			SV_SIGNAL(&nfs_asynclwp_sv, 0);
			UNLOCK(&nfs_async_lock, opl1);

			NFSLOG(0x100, "nfs_strategy: sleeping lwp\n", 0, 0);

			return (0);
		}

		/*
		 * now see if we can create one more.
		 */
		if ((nfs_async_total < nfs_async_currmax)
				&& (nfs_async_total < nfs_async_sum)) {
			nfs_async_stats.st_sigasyncd++;
			/*
			 * we can create another lwp. incr signal count
			 * for nfsbiod so that a signal is not missed if
			 * it is already running.
			 */
			nfs_async_sigcnt++;

			/*
			 * incr total lwps count here so as to avoid racing
			 * with another lwp queing I/O. asyncd will decr
			 * this if it is unable to create.
			 */
			nfs_async_total++;

			/*
			 * queue the request, release the mi lock signal
			 * asyncd to create another, and then release the
			 * nfs_async_lock, to avoid racing with the nfsbiod.
			 */
			bbackfree(bp, mi->mi_bufhead);
			mi->mi_asyncreq_count++;
			UNLOCK(&mi->mi_async_lock, opl2);
			SV_SIGNAL(&nfs_asyncd_sv, 0);
			UNLOCK(&nfs_async_lock, opl1);

			NFSLOG(0x100, "nfs_strategy: creating lwp\n", 0, 0);

			return (0);
		}

		nfs_async_stats.st_synceddef++;

		/*
		 * there was room on this mount point, but everyone's busy
		 * and cannot create any more. do the I/O synchronously now. 
		 *
		 * XXX: this is needed to limit queing for this mount and
		 * avoid memory depletion in the kernel.
		 */
		UNLOCK(&mi->mi_async_lock, opl2);
		UNLOCK(&nfs_async_lock, opl1);

		NFSLOG(0x100, "nfs_strategy: change to sync\n", 0, 0);

		return (do_bio(bp));
	} else {
		/*
		 * sync request
		 */
		NFSLOG(0x100, "nfs_strategy sync\n", 0, 0);

		nfs_async_stats.st_orgsync++;

		return (do_bio(bp));
	}
}

/*
 * nfsbiod()
 *	The async daemon is started here.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	The async daemon is started here.
 *
 * Parameters:
 *
 *	None.
 *
 */
/* ARGSUSED */
int
nfsbiod()
{
	pl_t	opl;

	if (nfs_async_max == 0) {
		/*
		 *+ nfs_async_max is not initialized.
		 */
		cmn_err(CE_NOTE,
		"nfsbiod(): nfs_async_max is 0, check configuration files");
		return(EINVAL);
	}

	/*
	 * release address space, and mark the process P_NOSEIZE,
	 * which will keep the swapper away.
	 */
	relvm_noswap();

	ASSERT(nfs_async_currmax == 0);
	ASSERT(nfs_async_total == 0);
	ASSERT(nfs_async_waking == 0);
	ASSERT(nfs_async_sleeping == 0);

	/*
	 * create a lwp which keeps mmap working as before by
	 * waking up periodically and updating attributes of
	 * rnodes which have been mapped.
	 */
	if (spawn_lwp(NP_FAILOK, NULL, LWP_DETACHED, NULL,
			      nfs_mmaplwp_start, NULL)) {
		/*
		 *+ nfsbiod failed to create the mmap lwp.
		 */
		cmn_err(CE_NOTE, "nfsbiod(): could not create mmap lwp");

		opl = LOCK(&nfs_async_lock, PLMIN);
		nfs_lwp_exit(NFS_BIOD, opl);
		/* NOTREACHED */
	}

	/*
	 * spawn the first async lwp so that all nfs activity does
	 * not come to a halt when I/O is queued up, but the nfsbiod
	 * is unable to create an async lwp. if this fails, all I/O
	 * will be sync and nothing will be queued.
	 */
	if (spawn_lwp(NP_FAILOK, NULL, LWP_DETACHED, NULL,
			      nfs_async_start, NULL)) {
		/*
		 *+ nfsbiod failed to create the first async lwp.
		 *+ this means that all nfs I/O will be synchronous.
		 */
		cmn_err(CE_NOTE, "nfsbiod(): could not create mmap lwp");

		opl = LOCK(&nfs_async_lock, PLMIN);
		nfs_lwp_exit(NFS_BIOD, opl);
		/* NOTREACHED */
	}

	/*
	 * lock and initialize nfs_async_total and nfs_async_currmax.
	 */
	opl = LOCK(&nfs_async_lock, PLMIN);
	nfs_async_total = 1;
	nfs_async_currmax = nfs_async_max;

	ASSERT(nfs_async_sigcnt == 0);

	/*
	 * simply wait to get woken up
	 */
	NFSBIOD_WAIT();

	for(;;) {
		/*
		 * grab the lock again
		 */
		opl = LOCK(&nfs_async_lock, PLMIN);

		ASSERT(nfs_async_currmax != 0);
		ASSERT(nfs_async_total <= nfs_async_currmax);
		ASSERT(nfs_async_sleeping <= nfs_async_currmax);
		ASSERT(nfs_async_waking <= nfs_async_sleeping);
		ASSERT(nfs_async_total >= 1);
		ASSERT(nfs_async_sleeping >= 0);
		ASSERT(nfs_async_waking >= 0);
		ASSERT(nfs_async_sigcnt > 0);

		/*
		 * decr sigcnt
		 */
		nfs_async_sigcnt--;

		if (nfs_async_waking < nfs_async_sleeping) {

			NFSLOG(0x100, "nfsbiod: sleeping lwps\n", 0, 0);

			/*
			 * there are sleeping lwps, just wake one up.
			 * this may happen when an lwp gets freed up while
			 * asynd was being woken up, not the common path
			 */
			nfs_async_waking++;
			SV_SIGNAL(&nfs_asynclwp_sv, 0);

			/*
			 * need to decr total lwp count as it was
			 * incr in nfs_strategy()
			 */
			nfs_async_total--;
			nfs_async_stats.st_asyncdsleep++;

			NFSBIOD_WAIT();

			continue;
		}

		/*
		 * create lwp, drop the lock first
		 */
		UNLOCK(&nfs_async_lock, opl);
		if (spawn_lwp(NP_FAILOK, NULL, LWP_DETACHED, NULL,
			      nfs_async_start, NULL)) {

			/*
			 *+ failed to create an lwp, warn and 
			 *+ go back to waiting. note that there
			 *+ is at least one lwp all the time.
			 */
			cmn_err(CE_WARN,
			"nfsbiod(): could not create async lwp");

			/*
			 * need to decr total lwp count as it was
			 * incr in nfs_strategy()
			 */
			opl = LOCK(&nfs_async_lock, PLMIN);
			nfs_async_total--;
			nfs_async_stats.st_creatfail++;

			NFSBIOD_WAIT();

			continue;
		}

		NFSLOG(0x100, "nfsbiod: created lwp\n", 0, 0);

		/*
		 * created lwp successfully, go back to
		 * waiting if no more signals.
		 */
		opl = LOCK(&nfs_async_lock, PLMIN);
		nfs_async_stats.st_created++;

		NFSBIOD_WAIT();

		continue;
	}
}

/*
 * void nfs_mmaplwp_start(void *nfs_argp)
 *	This is where the mmap lwp starts.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 *	No lock is held on entry.
 *
 * Description:
 *	This is where the mmap lwp starts. It basically
 *	wakes up every 30 seconds, finds all the rnodes
 *	which are currently mapped, and updates their caches.
 *
 * Parameters:
 *
 *	None.
 */
/* ARGSUSED */
void
nfs_mmaplwp_start(void *nfs_argp)
{
	toid_t		timeid;
	pl_t		opl;

	ASSERT(nfs_mmap_alive == 0);

	/*
	 * start a periodic timer to kick this daemon off on a regular basis.
	 * use a blocking allocation so it is guaranteed to succeed.
	 */
	timeid = itimeout_a(nfs_mmap_signal, NULL,
			    (nfs_mmap_timeout * HZ) | TO_PERIODIC, PLMIN,
			    itimeout_allocate(KM_SLEEP));

	/*
	 * advertise that mmap lwp is running.
	 */
	nfs_mmap_alive = 1;

	for(;;) {
		/*
		 * wait for timeout.
		 */
		opl = LOCK(&nfs_async_lock, PLMIN);
		if (!SV_WAIT_SIG(&nfs_mmaplwp_sv, PRIMED, &nfs_async_lock)) {
			/*
			 * we got signalled, cleanup and exit.
			 */
			untimeout(timeid);
			opl = LOCK(&nfs_async_lock, PLMIN);
			nfs_lwp_exit(NFS_MMAP_LWP, opl);
			/* NOTREACHED */
		}

		/*
		 * update attributes of mmaped files, if needed.
		 */
		nfs_mmap_sync();
	}
}

/*
 * nfs_mmap_signal()
 *	Timeout routine for mmap lwp.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Signal the mmap lwp to do its routine.
 *
 * Parameters:
 *
 *	arg			# always NULL
 *
 */
/* ARGSUSED */
void
nfs_mmap_signal(void *arg)
{
	NFSLOG(0x100, "nfs_mmap_signal: entered\n", 0, 0);

	SV_SIGNAL(&nfs_mmaplwp_sv, 0);
}

/*
 * void nfs_async_start(void *nfs_argp)
 *	This is where a new async lwp starts.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 *	No lock is held on entry.
 *
 * Description:
 *	This is where a new async lwp starts when a new
 *	io request is made and the asyncd is able to create
 *	a new lwp.
 *
 *	This new lwp basically loops around util there is
 *	no more work and time runs out for its existense.
 *
 * Parameters:
 *
 *	None.
 */
/* ARGSUSED */
void
nfs_async_start(void *nfs_argp)
{
	struct	nfs_argtotimeout	*arg;
	struct	mntinfo			*mi;
	struct	mntinfo			*savedmi;
	struct	buf			*bp;
	struct	vnode			*vp;
	void				*timo;
	toid_t				timeid;
	int				kill_sig = 0;
	int				nfsbiod_exited = 0;
	pl_t				opl1, opl2;

	NFSLOG(0x100, "nfs_async_start: entered\n", 0, 0);

	ASSERT(nfs_async_total >= 0);

	/*
	 * setup arg to timeout
	 */
	arg = (struct nfs_argtotimeout *)
		kmem_alloc(sizeof(struct nfs_argtotimeout), KM_SLEEP);
	arg->tm_timeup = 0;
	arg->tm_lwpp = u.u_lwpp;

	/*
	 * enter loop with mnt lock held
	 */
	opl1 = LOCK(&nfs_mnt_lock, PLMIN);
	for (;;) {
		/*
		 * remember which mount we started from.
		 * also move the head of the mount list
		 * so that the next lwp starts at the next
		 * mount.
		 */
		mi = savedmi = nfs_mnt_list;
		if (nfs_mnt_list)
			nfs_mnt_list = nfs_mnt_list->mi_forw;

		do {
			if (mi != NULL) {
				/*
				 * grab the mount specific lock
				 */
				opl2 = LOCK(&mi->mi_async_lock, PLMIN);
				ASSERT(mi->mi_rlwps <= mi->mi_lwpsmax);

				if ((mi->mi_rlwps < mi->mi_lwpsmax) &&
					(mi->mi_bufhead->av_forw
						!= mi->mi_bufhead)) {
					/*
					 * the server is not down, max
					 * lwps are not running on this
					 * mount AND there is work to
					 * do, get on it !
					 */
					mi->mi_rlwps++;

					/*
					 * since we are now working on this
					 * mount, save it so that next time
					 * we search the mount list, we do
					 * a full circle back to this
					 */
					savedmi = mi;

					/*
					 * we have found work after we may have
					 * timed out. reset the timeup flag
					 * so that if no more work, we sleep
					 * again for the timeout period.
					 */
					arg->tm_timeup = 0;

					while ((bp = mi->mi_bufhead->av_forw)
							!= mi->mi_bufhead) {
						/*
						 * work till there is work
						 */
						bremfree(bp);
						mi->mi_asyncreq_count--;

						/*
						 * release the locks now
						 */
						UNLOCK(&mi->mi_async_lock,
							opl2);
						UNLOCK(&nfs_mnt_lock, opl1);

						vp = (struct vnode *)
							bp->b_priv.un_ptr;
						vtor(vp)->r_error = do_bio(bp);

						/*
						 * need to grab the locks again
						 */
						opl1 = LOCK(&nfs_mnt_lock,
							PLMIN);
						opl2 = LOCK(&mi->mi_async_lock,
							PLMIN);
					}

					/*
					 * no more work on this mount
					 */
					mi->mi_rlwps--;
				}
				UNLOCK(&mi->mi_async_lock, opl2);

				ASSERT(mi != NULL);

				/*
				 * move on
				 */
				mi = mi->mi_forw;
			}
		/*
		 * loop until we have not covered the entire mount
		 * list and not found any work.
		 */
		} while (mi != savedmi);

		/*
		 * let go off the list, and get the async lock.
		 * in between, while we're holding no locks,
		 * allocate a timeout struct to use below.
		 */
		UNLOCK(&nfs_mnt_lock, opl1);

		timo = itimeout_allocate(KM_SLEEP);

		opl1 = LOCK(&nfs_async_lock, PLMIN);

		if (nfsbiod_exited || ((nfs_async_total > 1) &&
				     (kill_sig || arg->tm_timeup))) {
			/*
			 * we are the last async lwp, the biod has exited
			 * so no more I/O could have been queued, and we
			 * made one last pass.
			 *		OR
			 * we are not the last async lwp, and either we
			 * got killed, or our time is up and we did not
			 * find work in our last run.
			 */
			kmem_free(arg, sizeof(struct nfs_argtotimeout));
			itimeout_free(timo);
			nfs_lwp_exit(NFS_ASYNC_LWP, opl1);
			/* NOTREACHED */
		}

		if (kill_sig && (nfs_async_total == 1) &&
				(nfs_async_currmax == 0)) {
			/*
			 * biod has exited, so there will be no more
			 * queueing of I/O. make one last pass.
			 */
			nfsbiod_exited = 1;
		}

		arg->tm_timeup = 0;
		timeid = itimeout_a(nfs_async_signal, arg,
				    (nfs_async_timeout * HZ), PLMIN, timo);

		/*
		 * incr sleep count and sleep.
		 */
		nfs_async_sleeping++;
		if (!SV_WAIT_SIG(&nfs_asynclwp_sv, PRIMED, &nfs_async_lock))
			kill_sig = 1;

		untimeout(timeid);

		/*
		 * decrement sleeping count. also decrement the
		 * waking count if not woken as a result of
		 * timeout or kill_sig.
		 *
		 * XXX: there is a race between SV_SIGNAL() in
		 * nfs_strategy() and nfs_async_signal() called
		 * from the timeout, so that even when the lwp
		 * was signalled, the timeout set the timeup flag.
		 * this race can cause the waking count to get bigger
		 * than the sleeping count. we fix this here by
		 * reducing the waking count when it gets bigger.
		 */
		opl1 = LOCK(&nfs_async_lock, PLMIN);
		nfs_async_sleeping--;
		if ((arg->tm_timeup == 0) && !kill_sig) {
			nfs_async_waking--;
		}
		if (nfs_async_waking > nfs_async_sleeping)
			nfs_async_waking = nfs_async_sleeping;

		ASSERT((nfs_async_currmax == 0) ||
				(nfs_async_total <= nfs_async_currmax));
		ASSERT((nfs_async_currmax == 0) ||
				(nfs_async_sleeping <= nfs_async_currmax));
		ASSERT(nfs_async_total >= 0);
		ASSERT(nfs_async_sleeping >= 0);
		ASSERT(nfs_async_waking >= 0);

		UNLOCK(&nfs_async_lock, opl1);

		/*
		 * grab the mount list lock and repeat
		 */
		opl1 = LOCK(&nfs_mnt_lock, PLMIN);
	}
}

/*
 * nfs_async_signal()
 *	Timeout routine for async lwps.
 *
 * Calling/Exit State:
 *	Returns a void.
 *
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Simply sets the timeup flag for the lwp
 *	sets it runnning.
 *
 * Parameters:
 *
 *	arg			# pointer to struct containing timeup 
 *				# flag and pointer to lwp struct
 *
 */
void
nfs_async_signal(struct nfs_argtotimeout *arg)
{
	pl_t	plold;

	NFSLOG(0x100, "nfs_async_signal: entered\n", 0, 0);

	/*
	 * Call setrun() only if this lwp is still sleeping,
	 * as it may already have been made runnable by
	 * SV_SIGNAL() in nfs_strategy().
	 *
	 * Also set the timeup flag to either indicate that
	 * the time is up or that the race to wakeup this
	 * lwp was lost. no locking needed for the timeup flag
	 * as only owning lwp sees it
	 */
	plold = LOCK(&arg->tm_lwpp->l_mutex, PLHI);
	if (arg->tm_lwpp->l_stat == SSLEEP) {
		arg->tm_timeup = 1;
		setrun(arg->tm_lwpp);
	} else {
		/*
		 * race to wakeup lwp was lost
		 */
		arg->tm_timeup = -1;
	}
	UNLOCK(&arg->tm_lwpp->l_mutex, plold);
}

/*
 * do_bio(struct buf *bp)
 *	Do block I/O over nfs.
 *
 * Calling/Exit State:
 *	Returns 0 on success, error on failure.
 *
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	This routine does block I/O over nfs. It is common
 *	between sync and async I/O.
 *
 * Parameters:
 *
 *	bp			# buffer pointer
 *
 */
int
do_bio(struct buf *bp)
{
	struct	rnode		*rp;
	struct	vnode		*vp;
	struct vattr		va;
	cred_t			*cred;
	long			count;
	u_long			offset;
	u_long			diff;
	pl_t			opl;
	int			error, read, async;

	ASSERT(bp->b_flags & B_REMAPPED);

	vp = (struct vnode *)bp->b_priv.un_ptr;
	rp = vtor(vp);

	read = bp->b_flags & B_READ;
	async = bp->b_flags & B_ASYNC;

	NFSLOG(0x200, "do_bio: addr %x,blk %d, ", bp->b_un.b_addr, bp->b_blkno);
	NFSLOG(0x200, "offset %d, size %d, ", dbtob(bp->b_blkno), bp->b_bcount);
	NFSLOG(0x200, "bp->v_pages = %x\n", bp->b_pages, 0);
	NFSLOG(0x200, "B_READ %x B_ASYNC %x\n", read, async);

	/*
	 * use the credentials from rnode, but allow for
	 * them to be freed by another lwp
	 */
	opl = LOCK(&rp->r_statelock, PLMIN);
	ASSERT(rp->r_cred != NULL);
	cred = rp->r_cred;
	crhold(cred);
	UNLOCK(&rp->r_statelock, opl);

	/*
	 * evaluate the offset once
	 */
	offset = dbtob(bp->b_blkno);

	if (read) {
		error = bp->b_error = nfsread(vp, bp->b_un.b_addr,
			(u_int)offset, (long)bp->b_bcount,
			(long *)&bp->b_resid, cred, &va);
		if (!error) {
			if (bp->b_resid) {
				/*
				 * Didn't get it all because we hit EOF,
				 * zero all the memory beyond the EOF.
				 */
				bzero(bp->b_un.b_addr +
					(bp->b_bcount - bp->b_resid),
					(u_int)bp->b_resid);
			}

			if (bp->b_resid == bp->b_bcount) {
				/*
				 * check if we are beyond EOF. do not need
				 * statelock here as we are just peeking.
				 */
				if (offset >= rp->r_size) {
					/*
					 * we didn't read anything at all as
					 * we are past EOF. return an error
					 * indicator back but don't destroy
					 * the pages. nfs_getapage() will
					 * decide what to do with them.
					 */
					error = NFS_EOF;
				}
			}
		}
	} else {
		/*
		 * if the write fails and it was asynchronous
		 * all future writes will get an error.
		 */
		if (rp->r_error == 0) {
			opl = LOCK(&rp->r_statelock, PLMIN);
			diff = rp->r_size - offset;
			UNLOCK(&rp->r_statelock, opl);
			count = MIN(bp->b_bcount, diff);

			ASSERT(count >= 0);

			error = bp->b_error = nfswrite(vp, bp->b_un.b_addr,
						(u_int)offset, count, cred);
		} else {
			error = bp->b_error = rp->r_error;
		}
	}

	if (!error && read) {
		opl = LOCK(&rp->r_statelock, PLMIN);
		if (!CACHE_VALID(rp, va.va_mtime, va.va_size)) {
			UNLOCK(&rp->r_statelock, opl);
			/*
			 * read, if cache is not valid mark this bp
			 * with an error so it will be freed by pvn_done
			 * and return a special error, NFS_CACHEINVALERR,
			 * so caller can flush caches and re-do the operation.
			 */
			error = NFS_CACHEINVALERR;
			bp->b_error = EINVAL;
		} else {
			UNLOCK(&rp->r_statelock, opl);
			nfs_cache_attr(vp, &va);
		}
	}

	/*
	 * on NFS_EOF, nfs_getapage() will take care of the pages.
	 */
	if (error != 0 && error != NFS_EOF)
		bp->b_flags |= B_ERROR;

	/*
	 * let go of the credentials
	 */
	crfree(cred);

	/*
	 * mapout the buffer header
	 */
	bp_mapout(bp);

	/*
	 * call pvn_done() to free the bp and pages. If not ASYNC
	 * then we have to call pageio_done() to free the bp.
	 */
	pvn_done(bp);
	if (!async) 
		pageio_done(bp);

	NFSLOG(0x200, "do_bio: error %d, bp %x ", error, bp);
	NFSLOG(0x200, "B_READ %x B_ASYNC %d\n", read, async);

	return (error);
}

/*
 * nfswrite(vnode_t *vp, caddr_t base, off_t offset, long count, cred_t *cred)
 *	Actual write to a file.
 *
 * Calling/Exit State:
 * 	No locking assumptions made on entry or exit.
 *
 * Description:
 * 	Write to file. Writes to remote server in largest size
 * 	chunks that the server can handle. Write is synchronous.
 *
 * Parameters:
 *
 *	vp			# vnode to write to
 *	base			# base addr of data to write
 *	offset			# offset in file to write
 *	count			# number of bytes to write
 *	cred			# cred to use for write
 *
 */
int
nfswrite(vnode_t *vp, caddr_t base, off_t offset, long count, cred_t *cred)
{
	struct	mntinfo		*mi = vtomi(vp);
	struct	vattr		va;
	struct	nfswriteargs	wa;
	struct	nfsattrstat	*ns;
#ifdef NFSESV
	struct	nfsesvattrstat	*cns;
#endif
	int			tsize;
	u_long			xid;
	int			error;

	NFSLOG(0x10000, "nfswrite %s %x ", mi->mi_hostname, vp);
	NFSLOG(0x10000, "offset = %d, count = %d\n", offset, count);

	ns = (struct nfsattrstat *)kmem_zalloc(sizeof (*ns), KM_SLEEP);

#ifdef NFSESV
	cns = (struct nfsesvattrstat *)kmem_alloc(sizeof (*cns), KM_SLEEP);
#endif

	do {
		/*
		 * rpc xid assigned here because we want it to
		 * be unique for each rpc call (rfscall). assigning
		 * it in rfscall or below will cause different xid's
		 * for retransmissions of the same call.
		 */
		xid = alloc_xid();
		tsize = MIN(mi->mi_curwrite, count);
		wa.wa_data = base;
		wa.wa_fhandle = *vtofh(vp);
		wa.wa_begoff = offset;
		wa.wa_totcount = tsize;
		wa.wa_count = tsize;
		wa.wa_offset = offset;
		if (mi->mi_protocol == NFS_V2)
			error = rfscall(mi, RFS_WRITE, xid, xdr_writeargs,
				(caddr_t)&wa, xdr_attrstat, (caddr_t)ns, cred);

#ifdef NFSESV
		else
			error = rfscall(mi, RFS_WRITE, xid, xdr_writeargs,
				(caddr_t)&wa, xdr_esvattrstat, (caddr_t)cns,
								cred);
#endif

		if (error == ENFS_TRYAGAIN) {
			NFSLOG(0x40000, "nfswrite: ENFS_TRYAGAIN\n", 0, 0);

			error = 0;
			continue;
		}

		if (!error) {
			if (mi->mi_protocol == NFS_V2)
				error = geterrno(ns->ns_status);
#ifdef NFSESV
			else
				error = geterrno(cns->ns_status);
#endif
			/*
			 * can't check for stale fhandle and purge caches
			 * here because pages are held by nfs_getpage.
			 */
		}

		NFSLOG(0x10000, "nfswrite: sent %d of %d, ", tsize, count);
		NFSLOG(0x10000, "error %d\n", error, 0);

		count -= tsize;
		base += tsize;
		offset += tsize;
	} while (!error && count);

	if (!error) {
		if (mi->mi_protocol == NFS_V2) {
			/*
			 * convert attributes to vnode format and cache them
			 */
			nattr_to_vattr(vp, &ns->ns_attr, &va);
			nfs_cache_attr(vp, &va);
		}
#ifdef NFSESV
		else
			nfs_esvattrcache(vp, &cns->ns_attr);
#endif
	} else {
		NFSLOG(0x40000, "nfswrite: error %d\n", error, 0);

		PURGE_ATTRCACHE(vp);
	}

	kmem_free((caddr_t)ns, sizeof (*ns));

#ifdef NFSESV
	kmem_free((caddr_t)cns, sizeof (*cns));
#endif

	switch (error) {

	case 0:
#ifndef	SYSV
	case EDQUOT:
#endif
		break;

	case ENOSPC:
		/*
		 *+ Remote nfs file system full.
		 *+ Print a notice.
		 */
		cmn_err(CE_CONT,
	"NFS write error: on host %s remote file system full\n",
		mi->mi_hostname);

		break;

	default:
		/*
		 * Write error on nfs server.
		 * Print notice and file handle.
		 */
		cmn_err(CE_CONT, "NFS write error %d on host %s fh ",
			error, mi->mi_hostname);
		printfhandle((caddr_t)vtofh(vp));
		cmn_err(CE_CONT, "\n");

		break;
	}

	NFSLOG(0x40000, "nfswrite: returns %d\n", error, 0);

	return (error);
}

/*
 * nfsread(vnode_t *vp, caddr_t base, off_t offset, long count, 
 *		long *residp, cred_t *cred, vattr_t *vap)
 *	Actual read from a file.
 *
 * Calling/Exit State:
 * 	No locking assumptions made on entry or exit.
 *
 * Description:
 * 	Read from a file. Reads data in largest chunks our interface 
 * 	can handle.
 *
 * Parameters:
 *
 *	vp			# vnode to read
 *	base			# base addr to get data in
 *	offset			# offset to read
 *	count			# numbre of bytes to read
 *	residp			# pointer to numbre of bytes unread
 *	cred			# creds to use for read
 *	vap			# vnode attributes are returned in this
 */
int
nfsread(vnode_t *vp, caddr_t base, off_t offset, long count, 
	long *residp, cred_t *cred, vattr_t *vap)
{
	struct	mntinfo	*mi = vtomi(vp);
	struct	nfsreadargs	ra;
	struct	nfsrdresult	rr;
#ifdef NFSESV
	struct	nfsesvrdresult	crr;
	lid_t			tmplid;
#endif
	int			tsize;
	int			error;
	int			rcount, rstatus;
	u_long			xid;
	pl_t			opl;

	NFSLOG(0x10000, "nfsread %s %x ", mi->mi_hostname, vp);
	NFSLOG(0x10000, "base: %x, offset = %d, ", base, offset);
	NFSLOG(0x10000, "totcount = %d\n", count, 0);

	do {
		do {
			/*
			 * rpc xid assigned here because we want it to
			 * be unique for each rpc call (rfscall). assigning
			 * it in rfscall or below will cause different xid's
			 * for retransmissions of the same call.
			 */
			xid = alloc_xid();
			opl = LOCK(&mi->mi_lock, PLMIN);
			tsize = MIN(mi->mi_curread, count);
			UNLOCK(&mi->mi_lock, opl);
			rr.rr_data = base;

#ifdef NFSESV
			crr.rr_data = base;
#endif

			ra.ra_fhandle = *vtofh(vp);
			ra.ra_offset = offset;
			ra.ra_totcount = tsize;
			ra.ra_count = tsize;
			if (mi->mi_protocol == NFS_V2) {
				error = rfscall(mi, RFS_READ, xid,
					xdr_readargs, (caddr_t)&ra,
					xdr_rdresult, (caddr_t)&rr, cred);
				if (!error) {
					rcount = rr.rr_count;
					rstatus = rr.rr_status;
				}
			}

#ifdef NFSESV
			else {
				error = rfscall(mi, RFS_READ, xid,
					xdr_readargs, (caddr_t)&ra,
					xdr_esvrdresult, (caddr_t)&crr, cred);
				if (!error) {
					rcount = crr.rr_count;
					rstatus = crr.rr_status;
				}
			}
#endif

		} while (error == ENFS_TRYAGAIN);

		if (!error) {
			error = geterrno(rstatus);
			/*
			 * can't purge caches here because pages are held by
			 * nfs_getpage.
			 */
		}

		NFSLOG(0x10000, "nfsread: got %d of %d, ", tsize, count);
		NFSLOG(0x10000, "error %d\n", error, 0);

		if (!error) {
			count -= rcount;
			base += rcount;
			offset += rcount;
		}
	} while (!error && count && rcount == tsize);

	*residp = count;

	if (!error) {
		if (mi->mi_protocol == NFS_V2) {
			/*
			 * convert attributes to vnode format and cache them
			 */
			nattr_to_vattr(vp, &rr.rr_attr, vap);
			nfs_cache_attr(vp, vap);
		}

#ifdef NFSESV
		else {
			nfs_esvattrcache(vp, &crr.rr_attr);
			vtor(vp)->r_aclcnt = acl_getmax();
			nattresv_to_vattr(vp, &crr.rr_attr, vap, &tmplid,
				vtor(vp)->r_acl, &vtor(vp)->r_aclcnt);
		}
#endif
	}

	NFSLOG(0x40000, "nfsread: returning %d, resid %d\n", error, *residp);

	return (error);
}

/*
 * relvm_noswap()
 *	Release address space and mark process with P_NOSWAP.
 *
 * Calling/Exit State:
 * 	The caller must be single threaded, amd must not hold
 *	any spin locks.
 *
 * Description:
 *	Release address space and mark process with P_NOSWAP.
 *
 *	Note: this will be moved to a file in proc.
 *
 * Parameters:
 *
 */
void
relvm_noswap()
{
	proc_t	*p = u.u_procp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(p->p_nlwp == 1);

	/*
	 * get p_rdwrlock first, and then set P_NOSWAP. this will
	 * save us from deadlocking when we do not yield to a sieze
	 * request from the swapper, and the the context holding
	 * p_rdwrlock is waiting for memory.
	 */
	RWSLEEP_WRLOCK(&p->p_rdwrlock, PRIMED);
	(void)LOCK(&p->p_mutex, PLHI);
	p->p_flag |= P_NOSWAP;
	UNLOCK(&p->p_mutex, PLBASE);

	/*
	 * now release the address space.
	 */
	relvm(p);

	/*
	 * set p_nonlockedrss to zero here to keep the time ager at bay.
	 */
	p->p_nonlockedrss = 0;

	RWSLEEP_UNLOCK(&p->p_rdwrlock);
}
