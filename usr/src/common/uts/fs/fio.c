/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/fio.c	1.35"
#ident	"$Header: $"

#include <util/types.h>
#include <util/ksynch.h>
#include <util/list.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <proc/resource.h>
#include <proc/cred.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/proc_hier.h>
#include <fs/vnode.h>
#include <fs/file.h>
#include <mem/kmem.h>
#include <util/metrics.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/plocal.h>
#include <io/poll.h>

/*
 * Prototypes for internal functions.
 */
STATIC int fdroundup(int);
STATIC int fdalloc_l(int, int *, fd_table_t *);
STATIC boolean_t fdtaballoc(fd_table_t *, int, int *, fd_entry_t **);
STATIC void fdtabinstall(fd_entry_t *, int, fd_table_t *);

/*
 * XENIX Support
 */
extern void closesem(file_t *, vnode_t *);

/*
 * File list maintenance variables and macros.
 */
fspin_t file_list_mutex;		/* lock for file list access */

#define	FTL_LOCK()	FSPIN_LOCK(&file_list_mutex)
#define	FTL_UNLOCK()	FSPIN_UNLOCK(&file_list_mutex)

/*
 * f_mutex:	Per file structure lock, protects fields within
 *		a file structure.
 */
LKINFO_DECL(f_mutex_lkinfo,   "FF::f_mutex", 0);

/*
 * fdt_mutex:	Per file descriptor table lock, protects fields
 *		within a file descriptor table.
 */
STATIC LKINFO_DECL(fdt_mutex_lkinfo, "FF::fdt_mutex", 0);

#define	MIN_FD_COUNT	10

/*
 * void fdtinit(proc_t *procp)
 *	Initialize the fd-table management routines.
 *
 * Calling/Exit State:
 *	Called at system initialization time.
 */
void
fdtinit(proc_t *procp)
{
	register fd_table_t *fdtp;

	ASSERT(KS_HOLD0LOCKS());

	fdtp = GET_FDT(procp);
	fdtp->fdt_size = 0;
	fdtp->fdt_sizeused = 0;
	fdtp->fdt_entrytab = NULL;
	LOCK_INIT(&fdtp->fdt_mutex, FD_HIER, FD_MINIPL,
			&fdt_mutex_lkinfo, KM_NOSLEEP);
}

/*
 * STATIC int fdroundup(int num_fd)
 *	Determines an optimal number of file descriptor entries
 *	for allocation via kmem_alloc.
 *	
 * Calling/Exit State:
 *	Given an input parameter which is a lower bound on the number
 *	of file descriptor entries required by the caller, a value
 *	is returned which is the number of file descriptor entries
 *	that will fit into memory obtained via kmem_alloc().
 *
 * Remarks:
 *	This routine knows that kmem_alloc() allocates memory in chunks
 *	which are a power of 2.  The variable 'min_size' is set to the
 *	minimum size to allocate from kmem_alloc.  'Min_size' is set to
 *	accommodate MIN_FD_COUNT file descriptors.
 */
STATIC int
fdroundup(int num_fd)
{
	static int min_size = 0;
	register int size;
	register int request_size;

	request_size = num_fd * sizeof(fd_entry_t);

	if ((size = min_size) == 0) {
		/* Initialize min_size. */
		register int i;

		for (i = 16; i < MIN_FD_COUNT * sizeof(fd_entry_t); i <<= 1)
			;
		size = min_size = i;
	}

	while (size < request_size)
		size <<= 1;
	return (size / sizeof(fd_entry_t));
}

/*
 * int fdalloc(int start, int *fdp)
 *	Allocate the first available file descriptor numbered 'start' or
 *	higher.  On success, '*fdp' contains the allocated file descriptor
 *	number.
 *
 * Calling/Exit State:
 *	This function can block, no spin locks can be held on entry.
 *	Returns 0 if successful, non-zero errno otherwise.
 */
int
fdalloc(int start, int *fdp)
{
	register fd_table_t *fdtp;
	int error;

	ASSERT(KS_HOLD0LOCKS());

	fdtp = GET_FDT(u.u_procp);
	(void)FDT_LOCK(fdtp);
	if ((error = fdalloc_l(start, fdp, fdtp)) == 0) {
		register int nfd;
		/*
		 * Adjust fdt_sizeused here atomically.  Otherwise,
		 * a subsequent fd-allocation operation that required
		 * the fd-table to be extended (and copied) could miss
		 * the record of our allocation.
		 */
		if ((nfd = *fdp) >= fdtp->fdt_sizeused)
			fdtp->fdt_sizeused = nfd + 1;
	}
	FDT_UNLOCK(fdtp);
	return (error);
}

/*
 * STATIC int fdalloc_l(int start, int *fdp, fd_table_t *fdtp)
 *	Allocate the first available file descriptor numbered 'start' or
 *	higher from the file descriptor table given by 'fdtp'.  On success,
 *	'*fdp' will contain the allocated file descriptor number.
 *
 * Calling/Exit State:
 *	This function can block.
 *	The caller must hold the fdt_mutex of the given fd-table locked
 *	upon entry.  Fdt_mutex is held upon return, though it may be
 *	dropped and reacquired.
 *	Returns 0 if successful, non-zero errno if unsuccessful.
 *
 * Remarks:
 *	This function does not update fdt_sizeused.  It is the
 *	responsibility of the caller to update fdt_sizeused.
 */
STATIC int
fdalloc_l(int start, int *fdp, fd_table_t *fdtp)
{
	register fd_entry_t *fdep;	/* current fd entry pointer */
	register int fd;		/* current fd */
	register int size;		/* size of current fd array */
	fd_entry_t *nfdep;		/* new fd array */
	int nsize;			/* size of new fd array */
	int maxfds;			/* max fds allowed (rlimit) */

	ASSERT(LOCK_OWNED(&fdtp->fdt_mutex));

	if (start < 0)
		return (EINVAL);

	maxfds = u.u_rlimits->rl_limits[RLIMIT_NOFILE].rlim_cur;
	nfdep = NULL;			/* No new fd-entries allocated */
	nsize = 0;			/* make lint happy */

	for (;;) {
		/*
		 * Check for rlimit reduced below size of
		 * allocated fd_table.
		 */
		size = (fdtp->fdt_size > maxfds) ? maxfds : fdtp->fdt_size;
		fdep = &fdtp->fdt_entrytab[start];
		for (fd = start; fd < size; fd++, fdep++) {
			if (fdep->fd_status == FD_UNUSED) {
				/*
				 * Found an unused fd-entry.
				 * Mark it as allocated and return the
				 * corresponding fd.  This is the most
				 * frequent allocation case.
				 */
				fdep->fd_file = NULL;
				fdep->fd_lwpid = u.u_lwpp->l_lwpid;
				fdep->fd_flag = 0;	/* start clean */
				fdep->fd_status = FD_ALLOC;
				*fdp = fd;
				if (nfdep != NULL)
					kmem_free((void *)nfdep,
					    nsize * sizeof(fd_entry_t));
				return (0);
			}
		}

		/*
		 * No entries found, we need to increase the size of the
		 * fd table.  We may have already allocated the necessary
		 * space below if this is not the first time through the loop.
		 *
		 * Reallocation should be uncommon, as the minimum
		 * amount of space allocated per-process should
		 * be adequate for most processes.
		 */
		if (fd >= maxfds) {
			/*
			 * Too many open file descriptors.
			 * Make sure that if we raced with another LWP to
			 * grow the file descriptor table and lost, that
			 * we free up the extra file descriptors.
			 */
			if (nfdep != NULL)
				kmem_free((void *)nfdep,
					  nsize * sizeof(fd_entry_t));
			return (EMFILE);
		}

		if (nfdep != NULL) {
			/*
			 * Previously attempted to grow the file
			 * descriptor table before.  Check to see if
			 * we raced with another LWP in the process
			 * that grew the file descriptor table ahead of
			 * us.
			 */
			if (nsize > size) {
				/*
				 * We won the race:
				 * Nfdep is the right size, copy over existing
				 * entries and free the old space.
				 */
				fdtabinstall(nfdep, nsize, fdtp);
				fdep = &fdtp->fdt_entrytab[fd];
				fdep->fd_file = NULL;
				fdep->fd_lwpid = u.u_lwpp->l_lwpid;
				fdep->fd_flag = 0;	/* start clean */
				fdep->fd_status = FD_ALLOC;
				*fdp = fd;
				return (0);
			} else
				/*
				 * We raced with another LWP in the process
				 * and lost.
				 */
				kmem_free((void *)nfdep,
					  nsize * sizeof(fd_entry_t));
		}

		if (fdtaballoc(fdtp, fd + 1, &nsize, &nfdep)) {
			/*
			 * Found the memory without having to release the lock.
			 * As far as reallocation is concerned, this is the
			 * most likely case of reallocation.
			 */
			fdtabinstall(nfdep, nsize, fdtp);
			fdep = &fdtp->fdt_entrytab[fd];
			fdep->fd_file = NULL;
			fdep->fd_lwpid = u.u_lwpp->l_lwpid;
			fdep->fd_flag = 0;		/* start clean */
			fdep->fd_status = FD_ALLOC;
			*fdp = fd;
			return (0);
		}

		/*
		 * An fd-entry may have been freed while we were sleeping.
		 * Scan again before installing nfdep.
		 */
	}
}

/*
 * STATIC boolean_t
 * fdtaballoc(fd_table_t *fdtp, int size, int *sizep, fd_entry_t **fdepp)
 *	Allocate a new file descriptor entry table of at least 'size'
 *	elements.  A pointer to the new table and its size are returned
 *	via '*fdepp' and '*sizep', respectively.
 *
 * Calling/Exit State:
 *	The fdt_mutex lock locking the designated file descriptor table
 *	must be held upon entry.  Fdt_mutex is held upon return, though
 *	it may have been dropped and reacquired.
 *	Returns B_TRUE if fdt_mutex was not dropped, B_FALSE otherwise.
 *
 * Remarks:
 *	This function can block.
 *	This function does not update fdt_sizeused.  It is the
 *	responsibility of the caller to update fdt_sizeused.
 */
/* ARGSUSED */
STATIC boolean_t
fdtaballoc(fd_table_t *fdtp, int size, int *sizep, fd_entry_t **fdepp)
{
	register fd_entry_t *nfdep;
	register int nsize;

	ASSERT(LOCK_OWNED(&fdtp->fdt_mutex));

	nsize = fdroundup(size);	/* get the next increment */
	nfdep = (fd_entry_t *)
			kmem_alloc(nsize * sizeof(fd_entry_t), KM_NOSLEEP);
	if (nfdep != NULL) {
		*fdepp = nfdep;
		*sizep = nsize;
		return (B_TRUE);
	}

	/* Need to sleep to allocate memory.  Drop fdt_mutex. */
	FDT_UNLOCK(fdtp);
	nfdep = (fd_entry_t *)kmem_alloc(nsize * sizeof(fd_entry_t), KM_SLEEP);
	*fdepp = nfdep;
	*sizep = nsize;
	(void)FDT_LOCK(fdtp);		/* reacquire fdt_mutex */
	return (B_FALSE);
}

/*
 * STATIC void fdtabinstall(fd_entry_t *nfdep, int nsize, fd_table_t *fdtp)
 *	Install the array of file descriptor entries given by 'nfdep' in the
 *	file descriptor table given by 'fdtp'.
 *
 * Calling/Exit State:
 *	The fdt_mutex lock of the given file descriptor table must be
 *	held on entry, and remains held on return.  The old file
 *	descriptor table is freed.
 */
STATIC void
fdtabinstall(fd_entry_t *nfdep, int nsize, fd_table_t *fdtp)
{
	register fd_entry_t *fdep;
	register int sizeused;

	ASSERT(LOCK_OWNED(&fdtp->fdt_mutex));

	fdep = fdtp->fdt_entrytab;	/* existing fd entry array */
	sizeused = fdtp->fdt_sizeused;	/* max fd used + 1 */

	ASSERT(nsize >= sizeused);

	/* Copy existing fd entries to the new fd entry array. */
	if (fdep != NULL) {
		bcopy((void *)fdep, (void *)nfdep,
			sizeused * sizeof(fd_entry_t));
		kmem_free((void *)fdep, fdtp->fdt_size * sizeof(fd_entry_t));
	}

	/* Clear unused portion of the new fd entry array. */
	if (nsize > sizeused)
		bzero((void *)(nfdep + sizeused),
			(nsize - sizeused) * sizeof(fd_entry_t));

	fdtp->fdt_entrytab = nfdep;
	fdtp->fdt_size = nsize;
}

/*
 * int fddup(int fd, int startfd, int inherit_mask, int *fdp)
 *	Dup the file descriptor given by 'fd' onto the first free file
 *	descriptor entry which is >= 'startfd'. The mask of descriptor 
 *	lags to be inherited is given by 'inherit_mask'.  On success,
 *	'*fdp' contains the new file descriptor.
 *
 * Calling/Exit State:
 *	This function can block.  No locks should be held by the
 *	caller.  Returns 0 on success, non-zero errno otherwise.
 *
 * Remarks:
 *	It is the responsibility of the caller to validate 'startfd'
 *	before calling this function.  This includes checking that
 *	'startfd' is >= 0, and within current rlimit values.
 */
int
fddup(int fd, int startfd, int inherit_mask, int *fdp)
{
	register fd_table_t *fdtp;
	register fd_entry_t *nfdep;
	fd_entry_t *fdep;
	file_t *fp;
	int nfd;
	int error;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(startfd >= 0);

	if (fd < 0)
		return (EBADF);

	fdtp = GET_FDT(u.u_procp);
	(void)FDT_LOCK(fdtp);
	if ((error = fdalloc_l(startfd, &nfd, fdtp)) != 0) {
		FDT_UNLOCK(fdtp);
		return (error);
	}

	/*
	 * Check to see that the original 'fd' is valid here
	 * (and not before).  This is because fdalloc_l() may drop
	 * fdt_mutex, during which time 'fd' may have been closed.
	 */
	if (fd >= fdtp->fdt_size ||
	    (fdep = &fdtp->fdt_entrytab[fd])->fd_status != FD_INUSE) {
		/*
		 * Inline version of setf() to let go of 'nfd'.
		 */
		nfdep = &fdtp->fdt_entrytab[nfd];
		nfdep->fd_file = NULL;
		nfdep->fd_status = FD_UNUSED;
		FDT_UNLOCK(fdtp);
		return (EBADF);
	}

	fp = fdep->fd_file;
	/*
	 * FTE_HOLD() must be done while holding the fdt_mutex of the
	 * containing file descriptor table to ensure that the file
	 * table entry does not go away before we put our hold on it.
	 * This could happen if another LWP were to close the file descriptor.
	 */
	FTE_HOLD(fp);

	nfdep = &fdtp->fdt_entrytab[nfd];
	nfdep->fd_file = fp;
	nfdep->fd_status = FD_INUSE;
	nfdep->fd_flag = (fdep->fd_flag & inherit_mask); 

	if (nfd >= fdtp->fdt_sizeused)
		fdtp->fdt_sizeused = nfd + 1;

	FDT_UNLOCK(fdtp);

	*fdp = nfd;
	return (0);
}

/*
 * int fddup2(int fd, int inherit_mask, int nfd)
 *	Dup 'fd' onto 'nfd', closing 'nfd' first if necessary.
 *	The mask of file descriptor flags to be inherited is given
 *	by 'inherit_mask'.
 *
 * Calling/Exit State:
 *	This function may block.  No locks should be held
 *	by the caller on entry.
 *
 * Return value:
 *	Returns 0 if successful.
 *	Returns non-zero errno if unsuccessful.
 */
int
fddup2(int fd, int inherit_mask, int nfd)
{
	fd_table_t *fdtp;
	fd_entry_t *fdep, *nfdep;
	file_t *fp, *oldfp;
	int nsize;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(nfd >= 0);

	if (fd < 0)
		return (EBADF);

	nfdep = NULL;
	fdtp = GET_FDT(u.u_procp);

	/* Catch the case of dup'ing onto the same file descriptor. */
	if (fd == nfd) {
		(void)FDT_LOCK(fdtp);
		if (fd >= fdtp->fdt_size ||
		    fdtp->fdt_entrytab[fd].fd_status != FD_INUSE) {
			FDT_UNLOCK(fdtp);
			return (EBADF);
		}
		FDT_UNLOCK(fdtp);
		return (0);			/* expensive no-op */
	}

	(void)FDT_LOCK(fdtp);
	if (nfd >= fdtp->fdt_size) {
		/*
		 * Fdtaballoc may drop and subsequently reacquire
		 * fdt_mutex to allocate memory.
		 */
		if (!fdtaballoc(fdtp, nfd + 1, &nsize, &nfdep) &&
		    nfd < fdtp->fdt_size) {
			/*
			 * We slept while allocating a larger fd-entry
			 * array, and another LWP allocated and installed
			 * a larger array ahead of us.  It's OK to continue
			 * as the new space includes nfd.
			 */
			kmem_free((void *)nfdep, nsize * sizeof(fd_entry_t));
			nfdep = NULL;
		}
	}

	/*
	 * Note that we check to see that the original 'fd' is valid here
	 * (and not before), because fdtaballoc() may have dropped fdt_mutex,
	 * during which time 'fd' may have been closed.
	 */
	if (fd >= fdtp->fdt_size ||
	    (fdep = &fdtp->fdt_entrytab[fd])->fd_status != FD_INUSE) {
		FDT_UNLOCK(fdtp);
		if (nfdep != NULL)
			kmem_free((void *)nfdep, nsize * sizeof(fd_entry_t));

		return (EBADF);
	}

	if (nfdep != NULL) {
		/*
		 * OK to install the new table, as the dup2 is going
		 * to succeed.
		 */
		fdtabinstall(nfdep, nsize, fdtp);
		fdep = &fdtp->fdt_entrytab[fd];
	}

	ASSERT(nfd < fdtp->fdt_size);

	fp = fdep->fd_file;
	FTE_HOLD(fp);		/* Must be done holding fdt_mutex */

	oldfp = NULL;
	nfdep = &fdtp->fdt_entrytab[nfd];
	if (nfdep->fd_status == FD_INUSE)
		oldfp = nfdep->fd_file;

	nfdep->fd_file = fp;
	nfdep->fd_status = FD_INUSE;
	nfdep->fd_flag = fdep->fd_flag & inherit_mask;
	if (nfd >= fdtp->fdt_sizeused)
		fdtp->fdt_sizeused = nfd + 1;

	FDT_UNLOCK(fdtp);

	/* Need to close the old file originally open on nfd. */
	if (oldfp != NULL)
		(void)closef(oldfp);

	return (0);
}

/*
 * void fdtfork(proc_t *cp)
 *	Create the file descriptor table for the child process given
 *	by 'cp'.   The file descriptor table of the calling process
 *	(LWP) is copied to the child process.
 *
 * Calling/Exit State:
 *	This function can block.  No locks should be held on entry,
 *	no locks are held on return.
 *
 * Remarks:
 *	The process denoted by 'cp' must not be visible on the
 *	list of active processes (practive).  This is necessary
 *	to prevent an out of context access to a partially
 *	initialized file descriptor table via dofusers().
 */
void
fdtfork(proc_t *cp)
{
	fd_table_t *fdtp;
	fd_table_t *nfdtp;
	register fd_entry_t *fdep;
	register fd_entry_t *lastfdep;
	register fd_entry_t *nfdep;
	boolean_t locked;
	int nsize;
	register int i;

	ASSERT(KS_HOLD0LOCKS());

	/*
	 * Allocate a fd-table of the appropriate size (fdt_sizeused is
	 * large enough).  The child's fd-entry array may be smaller than
	 * that of the calling process.
	 */
	fdtp = GET_FDT(u.u_procp);
	nfdtp = GET_FDT(cp);
	locked = B_FALSE;
	if (!SINGLE_THREADED()) {	/* No need to lock for comparison */
		locked = B_TRUE;
		(void)FDT_LOCK(fdtp);
	}

	/* Only allocate an fd table if it is needed. */
	if (fdtp->fdt_entrytab == NULL || fdtp->fdt_sizeused == 0) {
		nfdep = NULL;
		nsize = 0;
		goto init_fdt;
	}

	nsize = fdroundup(fdtp->fdt_sizeused);
	if (locked) {
		/*
		 * The caller is multithreaded; try the allocation once
		 * without sleeping.  Then unlock to sleep if necessary.
		 */
		for (;;) {
			nfdep = (fd_entry_t *)
				 kmem_alloc(nsize * sizeof(fd_entry_t),
					    KM_NOSLEEP);
			if (nfdep != NULL)
				break;

			FDT_UNLOCK(fdtp);
			nfdep = (fd_entry_t *)
				 kmem_alloc(nsize * sizeof(fd_entry_t),
					    KM_SLEEP);
			(void)FDT_LOCK(fdtp);
			if (nsize >= fdtp->fdt_sizeused)
				break;
			/*
			 * Can only happen in the multithreaded case in
			 * which other LWP(s) in the process extend the
			 * process file descriptor table.
			 */
			kmem_free((void *)nfdep, nsize * sizeof(fd_entry_t));
			nsize = fdroundup(fdtp->fdt_sizeused);
		}
	} else {
		/* The caller is single-threaded.  Allow kmem_alloc to sleep. */
		nfdep = (fd_entry_t *)kmem_alloc(nsize * sizeof(fd_entry_t),
						KM_SLEEP);
	}
	ASSERT(nfdep != NULL);

init_fdt:
	/* Initialize the new fd-table. */
	LOCK_INIT(&nfdtp->fdt_mutex, FD_HIER, FD_MINIPL,
			&fdt_mutex_lkinfo, KM_NOSLEEP);
	nfdtp->fdt_size = nsize;
	nfdtp->fdt_sizeused = fdtp->fdt_sizeused;
	nfdtp->fdt_entrytab = nfdep;

	/* Copy each fd-entry and reference each file table entry, if any. */
	fdep = fdtp->fdt_entrytab;
	lastfdep = fdep + fdtp->fdt_sizeused;
	for (; fdep < lastfdep; fdep++, nfdep++) {
		*nfdep = *fdep;				/* struct assignment */
		if (fdep->fd_status == FD_INUSE) {
			/*
			 * FTE_HOLD() must be done while holding the
			 * fdt_mutex of the containing file descriptor
			 * table if the process contains multiple LWPs.
			 */
			FTE_HOLD(fdep->fd_file);
		} else if (fdep->fd_status == FD_ALLOC) {
			/*
			 * This is an fd in-transit. Since it's not fully
			 * initialized, we have to skip it. Shrink the
			 * fdt_sizeused if this is the highest fd.
			 * Scan backwards for the new highest fd being used.
			 */
			nfdep->fd_status = FD_UNUSED;

			if (fdep == lastfdep - 1) {
				register int sizeused;
				register fd_entry_t *fdep0;

				fdep0 = nfdtp->fdt_entrytab;
				sizeused = nfdtp->fdt_sizeused;
				do {
					if (nfdep->fd_status != FD_UNUSED)
						break;
					sizeused--;
				} while (nfdep-- > fdep0);
				nfdtp->fdt_sizeused = sizeused;
			}
		}
	}
	if ((i = nsize - nfdtp->fdt_sizeused) > 0)
		bzero((void *)nfdep, i * sizeof(fd_entry_t));

	if (locked)
		FDT_UNLOCK(fdtp);
}

/*
 * void fdtexec(void)
 *	Perform "close-on-exec" processing semantics for the calling LWP
 *	that has already destroyed all other LWPs in the process as part
 *	of an exec(2) system call.  Trim the size of the fd-table back
 *	if possible.
 *
 * Calling/Exit State:
 *	This function can block.  No locks should be held by the caller.
 *	The caller must be single-threaded.
 *
 * Remarks:
 *	This function must protect against out of context accesses to
 *	the file descriptor table via dofusers().
 */
void
fdtexec(void)
{
	register fd_table_t *fdtp;
	register fd_entry_t *fdep;
	file_t *fp;
	register int fd;
	register int new_size, new_sizeused;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(SINGLE_THREADED());

	new_sizeused = -1;
	fdtp = GET_FDT(u.u_procp);
	fdep = fdtp->fdt_entrytab;
#ifdef DEBUG
	if (fdep == NULL)			/* sanity check */
		ASSERT(fdtp->fdt_size == 0 && fdtp->fdt_sizeused == 0);
#endif /*DEBUG*/

	for (fd = 0; fd < fdtp->fdt_sizeused; fd++, fdep++) {
		if (fdep->fd_status == FD_INUSE) {
			if (fdep->fd_flag & FCLOSEXEC) {
				/*
				 * Even though the process is single threaded,
				 * must acquire the lock on the file descriptor
				 * table to protect against out of context
				 * access via dofusers().
				 */
				(void)FDT_LOCK(fdtp);
				fp = fdep->fd_file;
				fdep->fd_file = NULL;
				fdep->fd_lwpid = 0;
				fdep->fd_flag = 0;
				fdep->fd_status = FD_UNUSED;
				FDT_UNLOCK(fdtp);
				(void)closef(fp);
			} else
				new_sizeused = fd;
		} else {
			ASSERT (fdep->fd_status == FD_UNUSED);
		}
	}

	new_sizeused++;
	fdep = fdtp->fdt_entrytab;
	if ((new_size = fdroundup(new_sizeused)) < fdtp->fdt_size) {

		/* Trim off the unused space. */
		fdep = (fd_entry_t *)
			kmem_alloc(new_size * sizeof(fd_entry_t), KM_SLEEP);

		(void)FDT_LOCK(fdtp);
		fdtp->fdt_sizeused = new_sizeused;
		fdtabinstall(fdep, new_size, fdtp);
		FDT_UNLOCK(fdtp);
	} else if (fdtp->fdt_sizeused != new_sizeused) {
		ASSERT(fdtp->fdt_sizeused > new_sizeused);
		(void)FDT_LOCK(fdtp);
		fdtp->fdt_sizeused = new_sizeused;
		FDT_UNLOCK(fdtp);
	}
}

/*
 * void closeall(proc_t *p)
 *	Close all file descriptors for the process given by 'p'.
 *
 * Calling/Exit State:
 *	This function can block (via closef()).  No locks should be held
 *	by the caller.
 *
 * Remarks:
 *	This function is called by exit, in which case 'p == u.u_procp'
 *	and the calling process is single threaded.
 *	This function is also called by fork (proc_setup) on a failed
 *	fork attempt (for cleanup purposes), in which case 'p' is the
 *	the problem child.
 *
 *	This function protects itself against out of context accesses
 *	to the file descriptor table via dofusers().
 *	The fdt_mutex is not de-initialized here.  It is the responsibility
 *	of the caller to invoke, or arrange to have invoked, the FDT_DEINIT()
 *	macro when the process given by 'p' is no longer on the list of
 *	active processes.  This is due to a race with dofusers().
 */
void
closeall(proc_t *p)
{
	fd_table_t *fdtp;
	register fd_entry_t *fdep;
	fd_entry_t *save_fdep;
	int size, sizeused;

	ASSERT(KS_HOLD0LOCKS());

	fdtp = GET_FDT(p);

	/* Make the file descriptor table invisible to any other context. */
	(void)FDT_LOCK(fdtp);
	save_fdep = fdep = fdtp->fdt_entrytab;
	size = fdtp->fdt_size;
	sizeused = fdtp->fdt_sizeused;
	fdtp->fdt_entrytab = NULL;
	fdtp->fdt_size = fdtp->fdt_sizeused = 0;
	FDT_UNLOCK(fdtp);

	if (fdep != NULL) {
		register fd_entry_t *lastfdep;

		lastfdep = fdep + sizeused;
		for (; fdep < lastfdep; fdep++) {
			if (fdep->fd_status == FD_INUSE)
				(void)closef(fdep->fd_file);
			else
				ASSERT(fdep->fd_status == FD_UNUSED);
		}
		kmem_free((void *)save_fdep, size * sizeof(fd_entry_t));
	}
}

/*
 * int closefd(int fd)
 *	Close the file associated with 'fd' and destroy the
 *	fd-entry associated with 'fd'.
 *
 * Calling/Exit State:
 *	This function can block (via closef).  No locks should
 *	be held by the caller.
 *	Returns	0 if successful; non-zero errno otherwise.
 */
int
closefd(int fd)
{
	register fd_table_t *fdtp;
	register fd_entry_t *fdep;
	file_t *fp;

	ASSERT(KS_HOLD0LOCKS());

	fdtp = GET_FDT(u.u_procp);
	(void)FDT_LOCK(fdtp);
	if ((u_int)fd >= fdtp->fdt_size ||
	    ((fdep = &fdtp->fdt_entrytab[fd])->fd_status) != FD_INUSE) {

		/* Either not used or being allocated. */
		FDT_UNLOCK(fdtp);
		return (EBADF);
	}

	fp = fdep->fd_file;
	fdep->fd_file = NULL;
	fdep->fd_lwpid = 0;
	fdep->fd_flag = 0;
	fdep->fd_status = FD_UNUSED;

	/*
	 * Shrink fdt_sizeused.  If we are closing the highest fd in
	 * use, scan backwards for the new highest fd being used.
	 */
	if (fd == (fdtp->fdt_sizeused - 1)) {
		register int sizeused;
		register fd_entry_t *fdep0;

		fdep0 = fdtp->fdt_entrytab;
		sizeused = fdtp->fdt_sizeused;
		do {
			if (fdep->fd_status != FD_UNUSED)
				break;
			sizeused--;
		} while (fdep-- > fdep0);
		fdtp->fdt_sizeused = sizeused;
	}

	FDT_UNLOCK(fdtp);

	return (closef(fp));
}


/*
 * int getf_mhold(int fd, file_t **fpp)
 *	
 *	Get the file pointer associated with 'fd'.  On success, return
 *	the file pointer via '*fpp'.
 *
 * Calling/Exit State:
 *	This function does not block.
 *	This function acquires fdt_mutex, and releases it before returning.
 *	Returns 0  on success, '*fpp' will contain the held file table entry
 *	associated with 'fd'; non-zero errno value if unsuccessful.
 *
 * Remarks:
 *	An optimized version of getf().  It only places a "HOLD" on the file-
 *	table if the application was never multi-threaded.  The macro
 *	GETF_MRELE() should be used to release any hold placed on the
 *	file-table.
 *
 *	If the process was ever multi-threaded, an execution reference is
 *	attached to the file-table entry and must be released by the caller
 *	using either FTE_RELE() or GETF_MRELE(). 
 */
int
getf_mhold(register int fd, file_t **fpp)
{
	register fd_table_t *fdtp;
	fd_entry_t *fdep;
	file_t *fp;

	fdtp = GET_FDT(uprocp);
	if (!WAS_MT()) {
		if (fd >= 0 && fd < fdtp->fdt_size &&
	    	(fdep = &fdtp->fdt_entrytab[fd])->fd_status == FD_INUSE) {
			*fpp = fdep->fd_file;
			return (0);
		}
		return (EBADF);
	}
	(void)FDT_LOCK(fdtp);
	if (fd < 0 || fd >= fdtp->fdt_size ||
    	(fdep = &fdtp->fdt_entrytab[fd])->fd_status != FD_INUSE) {
		FDT_UNLOCK(fdtp);
		return (EBADF);
	}

	/*
	 * Attach an execution reference. If the process contains more
	 * than one LWP, then FTE_HOLD must be done while holding fdt_mutex.
	 */
	fp = fdep->fd_file;
	FTE_HOLD(fp);
	FDT_UNLOCK(fdtp);
	*fpp = fp;
	return (0);
}

/*
 * void getf_mrele(file_t *fp)
 *	Releases the file table hold placed by a FTE_HOLD
 *	The function is called via the GETF_RELE macro only if the process is
 *	multi-threaded.
 *
 * Calling/Exit State:
 *	This function can block.
 *	This function acquires fp_mutex, and releases it before returning.
 *	Returns nothing.  Should the last reference be cleared, the file will
 *	will be closed.
 */
void
getf_mrele(file_t *fp)
{
	(void)FTE_LOCK(fp);
	if ((fp)->f_count > 1) {
		(fp)->f_count--;
		FTE_UNLOCK(fp, PLBASE);
	} else {
		(void)closef_l(fp);
		/* unlocked on ret */
	}
}
/*
 * int getf(int fd, file_t **fpp)
 *	Get the file pointer associated with 'fd'.  On success, return
 *	the file pointer via '*fpp'.
 *
 * Calling/Exit State:
 *	This function does not block.
 *	This function acquires fdt_mutex, and releases it before returning.
 *	Returns 0  on success, '*fpp' will contain the held file table entry
 *	associated with 'fd'; non-zero errno value if unsuccessful.
 *
 * Remarks:
 *	The execution reference established against the returned file table
 *	entry pointer must be released by the caller using FTE_RELE().
 */
int
getf(register int fd, file_t **fpp)
{
	register fd_table_t *fdtp;
	fd_entry_t *fdep;
	file_t *fp;
	boolean_t locked;

	fdtp = GET_FDT(u.u_procp);
	locked = B_FALSE;
	if (!SINGLE_THREADED()) {
		locked = B_TRUE;
		(void)FDT_LOCK(fdtp);
	}

	if (fd < 0 || fd >= fdtp->fdt_size ||
	    (fdep = &fdtp->fdt_entrytab[fd])->fd_status != FD_INUSE) {

		/* Either invalid, not used, or being allocated. */
		if (locked)
			FDT_UNLOCK(fdtp);

		return (EBADF);
	}

	/*
	 * Attach an execution reference. If the process contains more
	 * than one LWP, then FTE_HOLD must be done while holding fdt_mutex.
	 */
	fp = fdep->fd_file;
	FTE_HOLD(fp);

	if (locked)
		FDT_UNLOCK(fdtp);

	*fpp = fp;
	return (0);
}

/*
 * void setf(int fd, file_t *fp)
 *	Install (set) the file table entry given by 'fp', into the
 *	fd-entry given by 'fd'.
 *
 * Calling/Exit State:
 *	This function can block (via closef).  No locks should be
 *	held by the caller on entry.
 *
 * Remarks:
 *	If the fp is NULL, simply mark the fd entry as UNUSED.
 *
 *	If the fd-table has shrunk below 'fd', or the corresponding
 *	fd-entry is not FD_ALLOC with the calling LWP as the LWP
 *	which performed the allocation, then the passed in file table
 *	entry ('fp') is silently closed.  This occurs when the fd-entry
 *	the caller allocated was destroyed by a racing fd operation.
 *
 *	If the fd-entry is valid, then the fd-entry is set to point
 *	to the file table entry given by 'fp'.  The fd-entry is marked
 *	as in use.
 */
void
setf(int fd, file_t *fp)
{
	register fd_entry_t *fdep;
	fd_table_t *fdtp;

	ASSERT(fd >= 0);
	fdtp = GET_FDT(u.u_procp);
	(void)FDT_LOCK(fdtp);

	fdep = &fdtp->fdt_entrytab[fd];

	/* 
	 * Handle the NULL fp case first.
	 */
	if (fp == NULLFP) {
		fdep->fd_status = FD_UNUSED;
		if (fd == (fdtp->fdt_sizeused - 1)) {
			/* Shrink fdt_sizeused. */
			register int sizeused;
			register fd_entry_t *fdep0;

			fdep0 =	fdtp->fdt_entrytab;
			sizeused = fdtp->fdt_sizeused;
			do {
				if (fdep->fd_status != FD_UNUSED)
					break;
				sizeused--;
			} while (fdep-- > fdep0);
			ASSERT(sizeused >= 0);
			fdtp->fdt_sizeused = sizeused;
		}
		FDT_UNLOCK(fdtp);
		return;
	}

	if (fd >= fdtp->fdt_size ||
	    (fdep = &fdtp->fdt_entrytab[fd])->fd_status != FD_ALLOC ||
	      fdep->fd_lwpid != u.u_lwpp->l_lwpid) {
		/*
		 * The entry we were allocating was destroyed due to a
		 * dup2/close/open race.  Silently close the file table
		 * entry being assigned.  This can happen if another LWP
		 * in the process does a dup2 which dups onto 'fd'.
		 */
		FDT_UNLOCK(fdtp);
		ASSERT(fp != NULL);
		(void)closef(fp);
	} else {
		/* Install the fp into the fd-entry. */

		fdep->fd_file = fp;
		fdep->fd_status = FD_INUSE;
		FDT_UNLOCK(fdtp);
	}
}

/*
 * int getpof(int fd, char *flagsp)
 *	Given a file descriptor, return the users file flags.
 *	The file descriptor flags are returned via '*flagsp'.
 *
 * Calling/Exit State:
 *	This function does not block.  The fdt_mutex of the
 *	calling process's file descriptor table is obtained
 *	and released by this function.
 *	Returns	0 if successful; non-zero errno otherwise.
 */
int
getpof(int fd, char *flagsp)
{
	fd_table_t *fdtp;
	fd_entry_t *fdep;
	boolean_t locked;

	if (fd < 0)
		return (EBADF);

	fdtp = GET_FDT(u.u_procp);
	locked = B_FALSE;
	if (!SINGLE_THREADED()) {
		locked = B_TRUE;
		(void)FDT_LOCK(fdtp);
	}

	if (fd >= fdtp->fdt_size ||
	    (fdep = &fdtp->fdt_entrytab[fd])->fd_status != FD_INUSE) {

		/* Either not used or being allocated. */
		if (locked)
			FDT_UNLOCK(fdtp);

		return (EBADF);
	}

	*flagsp = fdep->fd_flag;

	if (locked)
		FDT_UNLOCK(fdtp);

	return (0);
}

/*
 * int setpof(int fd, char flags)
 *	Given a file descriptor and file flags, set the user's file flags.
 *
 * Calling/Exit State:
 *	This function does not block.  The fdt_mutex of the
 *	calling process's file descriptor table is obtained
 *	and released by this function.
 *	Returns 0 if successful; non-zero errno otherwise.
 */
int
setpof(int fd, char flags)
{
	fd_table_t *fdtp;
	fd_entry_t *fdep;
	boolean_t locked;

	if (fd < 0)
		return (EBADF);

	fdtp = GET_FDT(u.u_procp);
	locked = B_FALSE;
	if (!SINGLE_THREADED()) {
		locked = B_TRUE;
		(void)FDT_LOCK(fdtp);
	}

	if (fd >= fdtp->fdt_size ||
	    (fdep = &fdtp->fdt_entrytab[fd])->fd_status != FD_INUSE) {

		/* Either not used or being allocated. */
		if (locked)
			FDT_UNLOCK(fdtp);

		return (EBADF);
	}

	fdep->fd_flag = flags;

	if (locked)
		FDT_UNLOCK(fdtp);

	return (0);
}

/*
 * void finit(void)
 *
 * Calling/Exit State:
 *	One time initialization for file table management
 *	routines.  Called early during system initialization.
 */
void
finit(void)
{
	FSPIN_INIT(&file_list_mutex);
}

/*
 * int closef(file_t *fp)
 *	Close the given file table entry.
 *
 * Calling/Exit State:
 *	The callers of this function must be prepared to block,
 *	as a VOP_CLOSE() operation is performed upon the
 *	underlying vnode, which could block.
 *	Returns: 0 if successful, non-zero errno otherwise.
 */
int
closef(file_t *fp)
{
	ASSERT(fp != NULL);
	ASSERT(KS_HOLD0LOCKS());

	(void)FTE_LOCK(fp);
	return(closef_l(fp));
}

/*
 * int closef_l(file_t *fp)
 *	Close the given file table entry.
 *	When the last reference to the given file table entry is removed,
 *	the file table entry is discarded.
 *
 * Calling/Exit State:
 *	This function expects the f_mutex lock of the given file table
 *	entry to be held by the caller.  F_mutex is dropped upon return.
 *	The caller must be prepared to block, since VOP_CLOSE() is
 *	invoked upon the vnode referenced by the given file table entry.
 *	Returns 0 if successful; non-zero errno otherwise.
 *
 * Remarks:
 *	On a last close, no further references to the file table entry
 *	may be made since the containing file descriptor entry has been
 *	obliterated.
 */
int
closef_l(register file_t *fp)
{
	vnode_t *vp;
	int flag;
	off_t offset;
	cred_t *credp;
	int error, error2;
	boolean_t lastclose;

	ASSERT(fp != NULL && fp->f_count > 0);
	ASSERT(LOCK_OWNED(&fp->f_mutex));

	vp = fp->f_vnode;
	flag = fp->f_flag;
	offset = fp->f_offset;
	credp = fp->f_cred;

	ASSERT(fp->f_count != 0);
	lastclose = (fp->f_count == 1);
	if (lastclose) {
		FTL_LOCK();
		MET_FILE_INUSE(-1);
		FTL_UNLOCK();
	}

	FTE_UNLOCK(fp, PLBASE);

	error = VOP_CLOSE(vp, flag, lastclose, offset, credp);

	(void)FTE_LOCK(fp);
	ASSERT(fp->f_count != 0);
	if (--fp->f_count == 0 && !lastclose) {
		/*
		 * Multiple closes were racing.  We didn't think we were
		 * going to be the last close when we checked above, but
		 * now we know we really are.  Do the lastclose processing
		 * at this time.  This introduces an extra call to VOP_CLOSE,
		 * but it is harmless.
		 */
		lastclose = B_TRUE;

		FTL_LOCK();
		MET_FILE_INUSE(-1);
		FTL_UNLOCK();

		FTE_UNLOCK(fp, PLBASE);

		error2 = VOP_CLOSE(vp, flag, lastclose, offset, credp);
		if (error == 0)
			error = error2;
	} else
		FTE_UNLOCK(fp, PLBASE);

	if (!lastclose)
		return (error);

	/*
	 * Dropping the last reference to the file table entry,
	 * do not need the lock.  The file table entry is not
	 * visible via a file descriptor at this point.
	 */
	/* XENIX support */
	if (vp->v_type == VXNAM)
		closesem(fp, vp);

	VN_LOCK(vp);
	vp->v_flag &= ~VXLOCKED;
	VN_UNLOCK(vp);
	/* End XENIX support */

	VN_RELE(vp);				/* may block */

	LOCK_DEINIT(&fp->f_mutex);
	crfree(credp);		/* release hold for file table entry */
	kmem_free((void *)fp, sizeof(file_t));
	return (error);
}

/*
 * int falloc(struct vnode *vp, int flag, file_t **fpp, int *fdp)
 *	Allocate a file structure, and allocate a process file descriptor.
 *	Possibly initialize the descriptor to point at the file structure.
 *	If successful, a pointer to the allocated file structure and
 *	file descriptor is returned in '*fpp' and '*fdp', respectively.
 *
 * Calling/Exit State:
 *	This function can block.  No locks should be held upon entry.
 *	Returns 0 on success; non-zero errno otherwise.
 *
 * Remarks:
 *	If 'vp' is non-NULL, then the allocated file descriptor is set
 *	to point at the allocated file structure, and the file
 *	descriptor entry is marked as in use.  Otherwise, the file
 *	descriptor entry is marked as being-allocated.
 *
 *	Callers who pass 'vp' as NULL are responsible for pointing the
 *	file table entry to the vnode they allocate, and then install
 *	the allocated file table entry using setf().  If there is an
 *	error procuring the vnode, the file table entry should be
 *	released using unfalloc(), and the fd-entry should be destroyed
 *	using setf(fd, NULLFP).
 */
int
falloc(struct vnode *vp, int flag, file_t **fpp, int *fdp)
{
	fd_table_t *fdtp;
	fd_entry_t *fdep;
	file_t *fp;
	int nfd;
	int error;

	ASSERT(KS_HOLD0LOCKS());

	fdtp = GET_FDT(u.u_procp);
	fp = (file_t *)kmem_zalloc(sizeof(file_t), KM_SLEEP);

	LOCK_INIT(&fp->f_mutex, FILE_HIER, FILE_MINIPL,
		&f_mutex_lkinfo, KM_SLEEP);
	fp->f_flag = flag;
	fp->f_count = 1;
	fp->f_vnode = vp;
	fp->f_offset = 0;		/* redundant since kmem_zalloc() */
	crhold(u.u_lwpp->l_cred);
	fp->f_cred = u.u_lwpp->l_cred;

	(void)FDT_LOCK(fdtp);
	if ((error = fdalloc_l(0, &nfd, fdtp)) != 0) {
		FDT_UNLOCK(fdtp);
		LOCK_DEINIT(&fp->f_mutex);
		crfree(fp->f_cred);
		kmem_free((void *)fp, sizeof(file_t));
		MET_FILE_FAIL();
		return (error);
	}

	/*
	 * A file descriptor has been marked as being allocated.
	 * Adjust fdt_sizeused here while the fd-table is still locked.
	 */
	if (nfd >= fdtp->fdt_sizeused)
		fdtp->fdt_sizeused = nfd + 1;

	if (vp != NULL) {
		/* Inline version of setf(). */
		fdep = &fdtp->fdt_entrytab[nfd];
		fdep->fd_file = fp;
		fdep->fd_status = FD_INUSE;
	}

	FDT_UNLOCK(fdtp);

	*fpp = fp;
	*fdp = nfd;

	/* Link the file table entry into the file table entry list. */
	FTL_LOCK();
	MET_FILE_INUSE(1);
	FTL_UNLOCK();
	return (0);
}

/*
 * void unfalloc(file_t *fp)
 *	Release the previously allocated file table entry.
 *
 * Calling/Exit State:
 *	This function does not block.  This function acquires
 *	the f_mutex lock of the passed in file structure, and
 *	it acquires the file list mutex.  Both locks are released
 *	prior to return.
 */
void
unfalloc(register file_t *fp)
{
	pl_t pl;

	pl = FTE_LOCK(fp);
	if (fp->f_count <= 0) {
		FTE_UNLOCK(fp, pl);
		/*
		 *+ Unfalloc function called with an unreferenced
		 *+ file table entry.  Kernel software bug.
		 */
		cmn_err(CE_PANIC,
			"unfalloc: unreferenced file table entry");
	}
	if (--fp->f_count == 0) {
		FTE_UNLOCK(fp, pl);
		FTL_LOCK();
		MET_FILE_INUSE(-1);
		FTL_UNLOCK();
		LOCK_DEINIT(&fp->f_mutex);
		crfree(fp->f_cred);
		kmem_free((void *)fp, sizeof(file_t));
	} else
		FTE_UNLOCK(fp, pl);
}

/*
 * int fassign(struct vnode **vpp, int mode, int *fdp)
 *	Allocate a file descriptor and assign it to the vnode "*vpp",
 *	performing the usual open protocol upon it and returning the
 *	allocated file descriptor.  It is the responsibility of the
 *	caller to dispose of "*vpp" if any error occurs.
 *
 * Calling/Exit State:
 *	This function can block.  No locks should be held upon entry.
 *	Returns 0 on success;  non-zero errno otherwise.
 */
int
fassign(struct vnode **vpp, int mode, int *fdp)
{
	file_t *fp;
	int error;
	int fd;
	pl_t pl;

	ASSERT(KS_HOLD0LOCKS());

	if (error = falloc((struct vnode *)NULL, mode & FMASK, &fp, &fd))
		return (error);
	if (error = VOP_OPEN(vpp, mode, u.u_lwpp->l_cred)) {
		setf(fd, NULLFP);
		unfalloc(fp);
		return (error);
	}
	pl = FTE_LOCK(fp);
	fp->f_vnode = *vpp;
	FTE_UNLOCK(fp, pl);
	setf(fd, fp);
	*fdp = fd;
	return (0);
}

/*
 * void fdgetpollx(struct pollfd *pfd, struct pollx *px, int n)
 *	Atomically map file descriptors to file pointers and create
 *	references to the file pointers.
 *
 * Calling/Exit State:
 *	This function acquires the file descriptor table lock and
 *	file table entry locks.  This function does not block.
 *
 * Remarks:
 *	The caller is responsible for allocating the passed in arrays
 *	of at least 'n' elements.  The caller must release the execution
 *	references on each returned file pointer.
 */
void
fdgetpollx(struct pollfd *pfd, struct pollx *px, int n)
{
	fd_table_t *fdtp;
	fd_entry_t *fdep;
	file_t *fp;
	boolean_t locked;
	int i, fd;

	fdtp = GET_FDT(u.u_procp);
	locked = B_FALSE;
	if (!SINGLE_THREADED()) {
		locked = B_TRUE;
		(void)FDT_LOCK(fdtp);
	}

	for (i = 0; i < n; i++, pfd++, px++) {
		fd = pfd->fd;
		if (fd < 0 || fd >= fdtp->fdt_size ||
		    (fdep = &fdtp->fdt_entrytab[fd])->fd_status != FD_INUSE) {
			/* Either invalid, not used, or being allocated. */
			px->px_fp = NULL;
		} else {
			fp = fdep->fd_file;
			if (WAS_MT())
				FTE_HOLD(fp);
			px->px_fp = fp;
		}
	}

	if (locked)
		FDT_UNLOCK(fdtp);

	return;
}

#if defined(DEBUG) || defined(DEBUG_TOOLS)

#ifdef	DEBUG
extern void print_vnode(const vnode_t *);
#endif	/* DEBUG */

/*
 * void
 * print_file(const file_t *fp)
 *	Print a file structure.
 *
 * Calling/Exit State:
 *	No locking.
 *
 * Remarks:
 *	Intended for use from a kernel debugger.
 */
void
print_file(const file_t *fp)
{
	debug_printf("file struct = %x:\n", fp);
	debug_printf("\tf_mutex.sp_lock: %slocked\n",
		 (fp->f_mutex.sp_lock == SP_LOCKED ? "" : "un"));

	debug_printf("\tf_offset = %8x, f_cred = %8x\n",
		 fp->f_offset, fp->f_cred);
#ifdef	DEBUG
	print_vnode(fp->f_vnode);
#else
	debug_printf("\tvnode = %lx\n", fp->f_vnode);
#endif	/* DEBUG */
}

#endif	/* defined(DEBUG) || defined(DEBUG_TOOLS) */
