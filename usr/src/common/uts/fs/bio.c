/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/bio.c	1.94"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <fs/buf.h>
#include <fs/fs_hier.h>
#include <io/conf.h>
#include <mem/kmem.h>
#include <mem/memresv.h>
#include <mem/pvn.h>
#include <proc/disp.h>
#include <proc/lwp.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/autotune.h>
#include <svc/clock.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/metrics.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <util/types.h>	
#include <util/var.h>

/*
 * Global Synchronization Objects
 * A spin lock maintains the integrity of the free list and the hash list.
 * A thread of execution may not block while holding the spin lock;
 * the lock must be held when modifying or examining the state of either of
 * the lists. A LWP wishing to allocate a buffer via getblk() or
 * bread() first obtains this lock and searches for the buffer.
 * If the buffer is on the hash list, the LWP atomically releases the
 * hash/spin list spin lock and enqueues for the target buffer's allocation
 * lock, bp->b_avail. When a thread of execution releases a buffer via brelse(),
 * the hash/free list spin lock is obtained. If there are any LWPs blocked on
 * this buffer becoming available then the spin lock is released followed by
 * the allocation lock of the buffer. This two-step process to obtain and
 * release a buffer insures that a buffer will never be put on the free list

 * if there exists an LWP waiting for it. In addition, once a thread of
 * execution determines that there does not exist waiters for this buffer then
 * no new references may be made to the buffer until the spin lock is released.
 * The buffer may be safely put on the free list at this point.
 *
 * The event object is used when there is not enough memory available to satisfy
 * additional buffer requests. A LWP requiring more buffer space than is
 * currently available will block on this event being signaled.
 * During the release of a buffer, this event is signaled when there are LWPs
 * blocked on it and the buffer is not contended for, i.e., no processes are
 * blocked on the buffer's allocation lock. An event object is used since it
 * is expected that multiple LWPs will be blocked on the event occurring when
 * there is a shortage of buffer space. Since the event object maintains state
 * there is no need to use a lock to test the event state.
 *
 * There are two other synchronization objects to collectively maintain the
 * number of outstanding asynchronous I/O requests. It's used by callers of
 * bdwait() (re-mounting a file system after fsck of root, or the unmounting
 * of the root file system). There exists a spin lock to maintain the
 * integrity of the count of outstanding asynchronous I/O requests and a
 * synchronization variable to await asynchronous I/O drain.
 */

/*
 * LWPs block on this event when there's not enough buffer space
 * to satisfy the LWP's request. brelse() notices blocked LWPs
 * and signals the event.
 */
STATIC event_t	buf_free_space;

/* 
 * Spin lock to protect buffer lists - hash and free. A two-step
 * process is used to obtain and release the buffer. First, this
 * lock is obtained. If releasing a buffer, can check the buffer's 
 * allocation lock (bp->b_avail) to see if there are LWP's blocked
 * on the buffer becoming available. If trying to get a buffer, first
 * get the buf_lists lock, and then atomically dropping buf_lists
 * while enqueuing on bp->b_avail.
 */
STATIC lock_t	buf_lists;
LKINFO_DECL(buf_lists_lkinfo, "FS: buf_lists: hash/free lists (global)", 0);

/* 
 * Fast spin lock for managing B_ASYNC buffer completion lists.
 */
STATIC fspin_t	buf_async_lock;

/*
 * Event used to trigger the bdelay daemon.
 */
STATIC event_t	bdelay_event;

/*
 * Lockinfo structure for per-buffer sleep lock.
 */
LKINFO_DECL(buf_avail_lkinfo, "FS: b_avail: allocation lock (per buffer)", 0);

STATIC buf_t	bfreelist;	/* Head of the free list of buffers */

STATIC buf_t	bhdrlist;	/* free buf header list              */
STATIC int	nbuf;		/* # of buffer headers allocated     */

extern struct hbuf hbuf[];	/* hash buffer list	*/

STATIC buf_t *bufchain;    	/* chain of allocated buffers */

buf_t *bclnlist;		/* completed B_ASYNC buffers which need
				 * further processing by clean_list() */
STATIC buf_t *bdelay_list;	/* B_ASYNC buffers waiting for delayed
				 * iodone processing */

STATIC void getblk_release_private(void);
STATIC void getblk_private(void);
STATIC int pboverlap(buf_t *, long);

buf_t getblk_private_buf;
boolean_t getblk_private_inuse;
sv_t getblk_private_sv;
LKINFO_DECL(getblk_private_lkinfo, "FS: getblk_private_buf: getblk_private_buf for pageout (global)", 0);

int buf_hash_cookie = 0;

/*
 * The following several routines allocate and free buffers with various
 * side effects.  In general the arguments to an allocate routine are a
 * device and a block number, and the value is a pointer to to the buffer
 * header; the returned buffer is locked so that no one else can touch it.
 * If the block was already in core, no I/O need be done; if it is already
 * locked, the LWP waits until it becomes free. The following routines
 * allocate a buffer:
 *	getblk
 *	bread
 *	breada
 *
 * Eventually the buffer must be released, possibly with the side effect of
 * writing it out, by using one of
 *	bwrite
 *	bdwrite
 *	brelse
 *
 * The buffer usage model is that once a buffer is allocated via
 * getblk()/bread()/etc. the owner of the buffer has complete control over
 * the buffer header information. Other LWPs will block on the buffer
 * header's allocation lock (bp->b_avail). When a LWP hands a buffer to
 * a driver via bdevsw[], the fields of the buffer header must not be
 * manipulated until the I/O has completed. This condition is noted by B_DONE
 * being set in the flags field of the buffer header.
 */


/*
 * STATIC void
 * buf_start_read(dev, bp, flags, bsize)
 *	Start a read operation to fill a buffer.
 *
 * Calling/Exit State:
 *	The buffer's allocation lock is held by the calling LWP
 *	on entry and remains held on exit.
 *
 *	No return value.
 *
 * Description:
 *	The I/O completion event is cleared before starting the
 *	read so that the caller may wait on it (biowait()). After
 *	starting the read (strategy routine for <dev>), the read
 *	is charged to the system (MET_BREAD()) and the caller
 *	(u.u_ior).
 */
STATIC void
buf_start_read(dev_t dev, buf_t *bp, int flags, long bsize)
{
	EVENT_CLEAR(&bp->b_iowait);
	bp->b_flags |= flags;
	bp->b_bcount = bsize;
	(*bdevsw[getmajor(dev)].d_strategy)(bp);
	ldladd(&u.u_ior, (ulong) 1);
	MET_BREAD();
}

/*
 * buf_t *
 * buf_search_hashlist(buf_t *bh, dev_t dev, daddr_t blkno)
 *	Search a buffer hash list for a specific buffer.
 *
 * Calling/Exit State:
 *	Caller holds <buf_lists> on entry; remains held
 *	on exit.
 *
 *	Returns NULL if a valid buffer matching <dev, blkno>
 *	could not be found; else a buffer pointer is returned.
 *
 * Description:
 *	Each hash bucket in <bh> is examined to determine
 *	whether a valid (!B_STALE) buffer matching <dev, blkno>
 *	exists. If there is a match, then a pointer to the
 *	buffer is returned.
 *
 *	Return NULL if we couldn't find the buffer on the
 *	hash list.
 *
 * Remarks:
 *	It's sufficient to search a single hash list since
 *	<bh> is the hash bucket for <dev, blkno>.
 *
 */
buf_t *
buf_search_hashlist(buf_t *bh, dev_t dev, daddr_t blkno)
{
	buf_t	*bp;

	/*
	 * Search the hash list for a matching buffer (using <blkno, dev>)
	 * that is not stale (B_STALE)
	 */
	for (bp = bh->b_forw; bp != bh; bp = bp->b_forw) {
		if (bp->b_blkno == blkno && bp->b_edev == dev &&
		    ((bp->b_flags & B_STALE) == 0)) {
			return (bp);
		}
	}

	/*
	 * Couldn't find one.
	 */
	return (NULL);
}

/*
 * buf_t *
 * bread(dev_t dev, daddr_t blkno, long bsize)
 *	Read in, if necessary, the block and return a buffer pointer
 *	to it.
 *
 * Calling/Exit State:
 *	The buffer's allocation lock is return held by the calling LWP.
 *	Other LWPs that want this buffer will block until the
 *	lock is released (brelse()).
 *
 *	The returned buffer must be checked for B_ERROR, etc.
 *
 * Description:
 *	The read is charged to the system (MET_LREAD()). Getblk()
 *	is used to locate the buffer; it may find the buffer is being
 *	used by another LWP, in this case, getblk() will enqueue
 *	the LWP on the buffer's allocation lock. When the buffer
 *	is released, the thread of execution releasing the buffer
 *	will hand the buffer to an LWP blocked on the buffer's
 *	allocation lock. There may be multiple LWPs blocked for
 *	the buffer but only 1 will be awakened at each buffer
 *	release (brelse()).
 *
 *	When the caller has ownership of the buffer (b_avail held
 *	by calling LWP) it will start a read operation on the buffer
 *	(if B_DONE is not set) and synchronously wait for completion. 
 */
buf_t *
bread(dev_t dev, daddr_t blkno, long bsize)
{
	buf_t	*bp;

	MET_LREAD();

	/*
	 * First, locate the requested block.
	 */
	bp = getblk(dev, blkno, bsize, 0);

	/*
	 * The calling LWP now 'owns' the buffer so its ok
	 * to inspect flags without racing. If B_DONE is not set,
	 * then start the I/O and wait for it.
	 */
	ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail) && SLEEP_LOCKOWNED(&bp->b_avail));
	if ((bp->b_flags & B_DONE) == 0) {
		buf_start_read(dev, bp, B_READ, bsize);
		(void) biowait(bp);
	}

	/*
	 * Return the buffer.
	 */
	ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail) && SLEEP_LOCKOWNED(&bp->b_avail));
	return (bp);
}

/*
 * buf_t *
 * breada(dev_t dev, daddr_t blkno, daddr_t rablkno, long bsize)
 *	Read in the block, like bread, but also start I/O on the
 *	read-ahead block (which is not allocated to the caller).
 *
 * Calling/Exit State:
 *	A buffer pointer for the non-read-ahead block is returned.
 *	This buffer's allocation lock is return held by the calling LWP.
 *	Other LWPs that want this buffer will block until the
 *	lock is released (brelse()).
 *
 *	The returned buffer must be checked for B_ERROR, etc.
 *
 * Description:
 *	The read is charged to the system (MET_LREAD()). Getblk()
 *	is used to locate the buffer; it may find the buffer is being
 *	used by another LWP, in this case, getblk() will enqueue
 *	the LWP on the buffer's allocation lock. When the buffer
 *	is released, the thread of execution releasing the buffer
 *	will hand the buffer to an LWP blocked on the buffer's
 *	allocation lock. There may be multiple LWPs blocked for
 *	the buffer but only 1 will be awakened at each buffer
 *	release (brelse()).
 *
 *	When the caller has ownership of the buffer (b_avail held
 *	by calling LWP) it will start a read operation on the buffer
 *	(if B_DONE is not set) and synchronously wait for completion. 
 *	The read is not awaited until after any requested read-ahead
 *	I/O is started. Since read-ahead is an optimization and
 *	a seldom-used (read no current usages) feature in SVR4, we
 *	choose not to block the caller if the read-ahead block
 *	is not in the buffer cache *and* there's not enough free
 *	space to allocate the read-ahead buffer. If we do get
 *	a read-ahead buffer, an asynchronous read is started for
 *	it if necessary (B_DONE is clear).
 *
 *	After any read-ahead is started, the non-read-ahead buffer
 *	is synchronously waited on. The read-ahead buffer may be
 *	picked up through a subsequent bread() (or other buffer
 *	cache operation that returns a buffer).
 */
buf_t *
breada(dev_t dev, daddr_t blkno, daddr_t rablkno, long bsize)
{
	buf_t	*bp;
	buf_t	*rabp;

	MET_LREAD();

	/*
	 * First, locate the requested block.
	 */
	bp = getblk(dev, blkno, bsize, 0);

	/*
	 * The calling LWP now 'owns' the buffer so its ok to inspect
	 * b_flags in a non-racy way. If B_DONE is not set, then start
	 * the I/O.
	 */
	if ((bp->b_flags & B_DONE) == 0) {
		buf_start_read(dev, bp, B_READ, bsize);
		(void) biowait(bp);
	}

	/*
	 * Now, do the read-ahead block, if any. Getblk() will return
	 * 0 if the block is already in an encached buffer (BG_NOHIT)
	 * or if the LWP would have to block for free space to become
	 * available to get a read-ahead buffer (BG_NOFREESPACE).
	 */
	if (rablkno) {
		rabp = getblk(dev, rablkno, bsize, BG_NOFREESPACE|BG_NOHIT);
		if (rabp) {
			if (rabp->b_flags & B_DONE) {
				brelse(rabp);
			} else {
				SLEEP_DISOWN(&rabp->b_avail);
				buf_start_read(dev,rabp, B_ASYNC|B_READ, bsize);
			}
		}
	}

	/*
	 * Return the buffer pointer (non-read-ahead buffer).
	 */
	ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail) && SLEEP_LOCKOWNED(&bp->b_avail));
	return (bp);
}

/*
 * buf_t *
 * blookup(dev_t dev, daddr_t blkno, long bsize)
 * 	See if there is a buffer associated with the specified block and return
 * 	it. This routine is like a variant of getblk which doesn't wait. If the
 * 	buffer exists, and it's not busy, then mark the buffer busy and
 * 	return a pointer to it; otherwise, return NULL.
 *
 * Calling/Exit State:
 * 	No locks held on entry; buffer's allocation lock is held on exit
 *	if it's in the cache.
 */
/* ARGSUSED */
buf_t *
blookup(dev_t dev, daddr_t blkno, long bsize)
{
        buf_t *bp, *dp;
        pl_t	 s;

        dp = bhash(dev, blkno);
	ASSERT(dp != NULL);
	s = LOCK(&buf_lists, FS_BUFLISTPL);
	bp = buf_search_hashlist(dp, dev, blkno);
	if (bp != NULL) {
        	if (!SLEEP_TRYLOCK(&bp->b_avail)) {
			UNLOCK(&buf_lists, s);
		       	return (NULL);
		}
        	if (!(bp->b_flags & B_DONE)) {
			SLEEP_UNLOCK(&bp->b_avail);
			UNLOCK(&buf_lists, s);
		       	return (NULL);
		}
     	   	bp->b_flags = (bp->b_flags & ~B_AGE) | B_BUSY;
		if (!SLEEP_LOCKBLKD(&bp->b_avail)) {
			bremfree(bp);
		}
		MET_LREAD();
	}
	UNLOCK(&buf_lists, s);
        return (bp);
}

/*
 * int
 * bwrite(buf_t *bp)
 *	Write the buffer to its backing store. The caller
 *	may choose to write the buffer synchronously or
 *	asynchrously.
 *
 * Calling/Exit State:
 *	The buffer's allocation lock is held by the calling LWP
 *	on entry; on exit, the buffer may not be referenced
 *	by the calling LWP since the buffer's ownership has
 *	been relinquished (if synchronous), or transferred
 *	(if asynchronous).
 *
 *	For synchronous writes, the completion status (from geterror)
 *	is returned. For asynchronous writes, 0 is always returned.
 *
 * Description:
 *	Buffer writes are charged to the system (MET_LWRITE(),
 *	MET_BWRITE()) and the calling process (u.u_iow). The
 *	buffer's flags must be set to indicate a non-delayed
 *	(!B_DELWRI) write (!B_READ) that hasn't completed (!B_DONE).
 *	B_ERROR is cleared now since it will be set if required.
 *	If the write is to be synchronous (B_ASYNC is clear), the
 *	I/O completion event (b_iowait) is cleared before starting
 *	the I/O so the calling LWP can determine when the I/O
 *	is complete.
 *
 *	The write is started on the buffer, and if synchronous, the
 *	calling I/O waits for it to complete. When the write
 *	completes, the buffer is released (brelse()).
 *	
 *	Allows ordered writes on asynchronous I/O.
 */
int
bwrite(buf_t *bp)
{
	int	flags;
	int	error;

	ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail));

	MET_LWRITE();
	MET_BWRITE();
	ldladd(&u.u_iow, (ulong) 1);

	flags = bp->b_flags;
	bp->b_flags &= ~(B_READ | B_DONE | B_ERROR | B_DELWRI);
	if ((flags & B_ASYNC) == 0) {
		EVENT_CLEAR(&bp->b_iowait);
	} else {
		SLEEP_DISOWN(&bp->b_avail);
	}

	if (bp->b_writestrat) {
		(*(bp->b_writestrat))(bp);
	} else {
		(*bdevsw[getmajor(bp->b_edev)].d_strategy)(bp);
	}

	if ((flags & B_ASYNC) == 0) {
		error = biowait(bp);
		brelse(bp);
	} else {
		error = 0;
	}

	return (error);
}

/*
 * void
 * bdwrite(buf_t *bp)
 *	Release the buffer, marking it so that if it is grabbed
 *	for another purpose it will be written out before being
 *	given up.
 *
 * Calling/Exit State:
 *	The buffer's allocation lock is held by the calling LWP
 *	on entry; on exit, the buffer may not be referenced
 *	by the calling LWP since the buffer's ownership has
 *	been relinquished (if synchronous), or transferred
 *	(if asynchronous).
 *
 *	No return value.
 *
 * Description:
 *	Buffers that are not written synchronously are marked with
 *	the time given to the system. This allows buffer aging
 *	to insure that the buffer doesn't get too old.
 *
 *	If the buffer was not already marked delayed write, the current time
 *	is placed in the buffer (b_start). The buffer's flags are marked
 *	to indicate delayed write (B_DELWRI) and mark the buffer as
 *	containing valid data (B_DONE).
 *
 *	Bdwrite() is often used when writing a partial block and it is
 *	assumed that another write for the same block will soon follow.
 *	Thus, there's no benefit to immediately writing the buffer to
 *	disk.
 *
 * Remarks:
 *	Buffers marked delayed write will be written to disk before
 *	reusing them. 
 */
void
bdwrite(buf_t *bp)
{
	MET_LWRITE();

	ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail));

	SLEEP_DISOWN(&bp->b_avail);

	/*
	 * Mark the time for buffer aging.
	 */
	if ((bp->b_flags & B_DELWRI) == 0) {
		bp->b_start = lbolt;
	}

	bp->b_flags |= B_DELWRI | B_DONE;
	bp->b_resid  = 0;

	brelse(bp);

	return;
}

/*
 * btwrite(buf_t *bp, clock_t start)
 * 	Variation on bdwrite, which sets the start time of the request to
 * 	the start time passed in unless the start time of the buffer is
 * 	older than the new start time.
 *
 * Calling/Exit State:
 *	The buffer's allocation lock is held by the calling LWP
 *	on entry; on exit, the buffer may not be referenced
 *	by the calling LWP since the buffer's ownership has
 *	been relinquished (if synchronous), or transferred
 *	(if asynchronous).
 *
 *	No return value.
 * Description:
 *      Buffers that are not written synchronously are marked with
 *      the time given to the system. This allows buffer aging
 *      to insure that the buffer doesn't get too old.
 *
 *      If the buffer was not already marked delayed write, the current time
 *      is placed in the buffer (b_start). The buffer's flags are marked
 *      to indicate delayed write (B_DELWRI) and mark the buffer as
 *      containing valid data (B_DONE).
 *
 */
void
btwrite(buf_t *bp, clock_t start)
{

	MET_LWRITE();
	ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail));

        SLEEP_DISOWN(&bp->b_avail);

        if (((bp->b_flags & B_DELWRI) == 0) || (bp->b_start > start))
                bp->b_start = start;
        bp->b_flags |= B_DELWRI | B_DONE;
        bp->b_resid = 0;
        brelse(bp);
}

/*
 * void
 * brelse(buf_t *bp)
 *	Release the buffer, with no I/O implied.
 *
 * Calling/Exit State:
 *	This can be called from an interrupt handler
 *	indirectly through biodone().
 *
 *	<bp> is owned exclusively by the calling LWP if not at
 *	interrupt level. If at interrupt level, then the
 *	brelse() is due to an asynchronous I/O. In such cases, the
 *	buffer is not "owned" by any LWP. The buffer's allocation
 *	lock is not available, i.e., the lock was obtained during
 *	getblk() and the thread of execution will release the buffer's
 *	allocation lock. If other LWPs are waiting for this buffer
 *	to become available, then one such LWP will be awakened here.
 *
 * Description:
 *	Releasing a buffer can only happen while holding
 *	<buf_lists> since holding the lock allows an LWP
 *	to decide to block on a buffer becoming available.
 *
 *	If there's a LWP blocked on this buffer becoming
 *	available, then release <buf_lists> and <bp->b_avail> and
 *	return. Releasing <bp->b_avail> will cause a single LWP blocked
 *	on <bp> becoming available to awaken. Before releasing
 *	<bp->b_avail> the buffer may be invalidated (if B_ERROR
 *	is set) by setting B_STALE. Before handing the buffer
 *	to another LWP, B_AGE, B_ASYNC, and B_DELWRI are cleared.
 *
 *	If no other LWP currently wants the buffer, the buffer is
 *	invalidated if necessary (B_ERROR is set). If it is 
 *	invalidated, then it is marked with B_AGE to place
 *	on the head of the freelist. Marking the buffer with
 *	B_STALE makes it non-findable in hash list searches.
 *
 *	The buffer is put on the head of the free list if B_AGE is set;
 *	otherwise, it's added to the back. Increment the free list buffer
 *	count and mark bp->b_reltime with the current time.
 *
 *	Release <buf_lists> and <bp->b_avail> and wake up any LWPs
 *	blocked on free space becoming available by signaling 
 *	<buf_free_space>.
 */
void
brelse(buf_t *bp)
{
	pl_t		s;

	ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail));

	s = LOCK(&buf_lists, FS_BUFLISTPL);

	if (bp->b_flags & B_PRIVBLK) {
		bremhash(bp);
		ASSERT(!SLEEP_LOCKBLKD(&bp->b_avail));
		SLEEP_UNLOCK(&bp->b_avail);

		(void) getblk_release_private();
		UNLOCK(&buf_lists, s);
		return;
	}

	/*
	 * First - if there are LWPs blocked waiting for this buffer to
	 * become available, then wake up 1 after updating b_flags.
	 */

	if (SLEEP_LOCKBLKD(&bp->b_avail)) {
		/*
		 * It's OK to release <buf_lists> first since we already
		 * decided that there is a LWP waiting. If it's invalid
		 * then set B_STALE so others don't see it and queue more
		 * waiters. Leave on the hash list since it will be removed
		 * when the last LWP blocked on the buffer releases it.
		 *
		 * Turn off B_AGE and B_ASYNC if they were set since they
		 * don't apply any more, i.e., the LWP receiving the buffer
		 * will set and/or modify the flags according to it's 
		 * usage.
		 */
		bp->b_flags &= ~(B_AGE | B_ASYNC | B_BUSY);
		if (bp->b_flags & B_ERROR) {
			bp->b_flags |= B_STALE | B_AGE;
			bp->b_flags &= ~B_ERROR;
		}
		SLEEP_UNLOCK(&bp->b_avail);
		UNLOCK(&buf_lists, s);
		return;
	}

	/*
	 * No other LWP is waiting for this buffer.
	 *
	 * If there was an error on this buffer, than destroy
	 * it's identity by setting B_STALE.
	 *
	 * Cancel any deferred I/O by clearing B_DELWRI and mark
	 * the buffer so it's put on the head of the free list (B_AGE).
	 */
	if (bp->b_flags & B_ERROR) {
		bp->b_flags |= B_STALE | B_AGE;
		bp->b_flags &= ~(B_ERROR | B_DELWRI);
		bp->b_error  = 0;
	}

	bp->b_reltime = lbolt;

	/*
	 * Put the buffer back on the free list. 
	 */
	if (bp->b_flags & B_AGE) {
		/*
		 * Put the buffer at the head of free list.
		 */
		bfrontfree(bp, &bfreelist);
	} else {
		/*
		 * Put the buffer at the tail of free list.
		 */
		bbackfree(bp, &bfreelist);
	}

	bfreelist.b_bcount++;

	/*
	 * Release alloc lock before buf_lists so that a racing getfreeblk
	 * won't see the buffer until we're done with it.
	 * Clear the B_ASYNC flag at this point.
	 */
	bp->b_flags &= ~(B_BUSY|B_ASYNC);
	SLEEP_UNLOCK(&bp->b_avail);
	UNLOCK(&buf_lists, s);

	/*
	 * Wake up all waiting LWPs, if there is any, to avoid
	 * starving any of them. The wake up is done outside of the lock
	 * since each LWP will immediately go for the lock again.
	 */
	EVENT_BROADCAST(&buf_free_space, 0);


	return;
}

/*
 * STATIC void
 * getblk_private(void)
 *	Mark the <getblk_private_buf> buffer in use.
 *	Called only for a pageout or memory scheduler process.
 *
 * Calling/Exit State:
 *	No locks are held on entry or exit.
 *
 * Description:
 *	Obtain the <buf_lists> lock. If the <getblk_private_buf>
 *	is already in use, wait for <getblk_private_sv> to be signaled.
 *	When it is, set <getblk_private_inuse> to B_TRUE, unlock the
 *	<buf_lists> lock, and return.
 */
STATIC void
getblk_private(void)
{

	ASSERT(getpl() == PLBASE);

	(void) LOCK(&buf_lists, PLHI);
	while (getblk_private_inuse) {
		SV_WAIT(&getblk_private_sv, PRIMEM, &buf_lists);
		(void) LOCK(&buf_lists, PLHI);
	}
	getblk_private_inuse = B_TRUE;
	UNLOCK(&buf_lists, PLBASE);
}

/*
 * STATIC void
 * getblk_release_private(void)
 *	Mark the <getblk_private_buf> buffer available.
 *
 * Calling/Exit State:
 *	The <buf_lists> spin lock is held on entry and exit.
 *
 * Description:
 *	Set <getblk_private_inuse> to B_FALSE, and signal the
 *	<getblk_private_sv> synchronization variable.
 */
STATIC void
getblk_release_private(void)
{

	getblk_private_inuse = B_FALSE;
	SV_SIGNAL(&getblk_private_sv, 0);
}

/*
 * buf_t *
 * getfreeblk(long bsize, int option, int lists_locked, pl_t *s)
 *	Release space from the free list. If a buffer with <bsize> is
 *	found on the free list, then that buffer is returned; otherwise,
 *	enough space is released to satisfy the caller's request.
 *
 * Calling/Exit State:
 *	If <lists_locked> is non-zero, than the calling LWP holds the
 *	<buf_lists> lock. Otherwise, there are no locks held on entry.
 *	<buf_lists> is never held on exit.
 *
 *	NULL is returned if there's not enough free space on the free list,
 *	i.e., bsize > free list size *and* we've freed all eligible buffers
 *	from the list.
 *
 *	Otherwise, a pointer to a buffer is returned. The buffer is
 *	removed from the hash list and returned locked.
 *
 * Description:
 *	<buf_lists> is held while searching for free space. Loop through
 *	the free list until 1) we find a buffer with <bsize>, 2) we have
 *	freed all eligible buffer and there's still not enough space for
 *	this buffer or 3) we've recycled enough buffers from the freelist
 *	to be able to kmem_alloc a new one without going over BUFHWM.
 *
 *	If we find an available buffer on the free list with <bsize>,
 *	we claim that buffer and return it.
 *
 *	If there's not enough space on the free list after our efforts,
 *	block on the free space event. The free space event will be
 *	signaled from brelse() when a buffer is placed on the free list.
 *	When awakened, return to caller to search the hash list again
 *	since it's possible that the buffer was placed in the hash list
 *	while blocked.
 *
 *	If there's enough space on the free list, i.e., bsize < free list size,
 *	than we take a header off the free list and allocate a buffer for it.
 */
buf_t *
getfreeblk(long bsize, int option, int lists_locked, pl_t *sp)
{
	buf_t	*bp;
	buf_t	*dp;
	buf_t	*tdp;
	int		size;
	caddr_t		addr;
	boolean_t tried = B_FALSE;
	
	if (!lists_locked) {
		*sp = LOCK(&buf_lists, FS_BUFLISTPL);
	}
loop:
	bp = bfreelist.av_forw;
	while(bp->b_flags == B_MARKER) {
		bp = bp->av_forw;
		ASSERT(bp != NULL);
	}
	if (bp != &bfreelist &&
	    (bfreelist.b_bufsize < bsize ||
	     bp->b_flags & B_AGE || !mem_disc_check())) {
		ASSERT(bp != NULL);
		notavail(bp);			

		/*
		 * This buffer hasn't been written to disk yet.
		 */
		if (bp->b_flags & B_DELWRI) {
			/*
			 * If the buffer is not marked STALE and the caller
			 * does not specify NOFLUSH, start the I/O on it now
			 * and free it later.
			 * B_AGE is set since the buffer has already migrated
			 * to the head of the freelist, we want it to return
			 * to the head of the list when the I/O has completed.
			 */
			if ((bp->b_flags & B_STALE) == 0 &&
						(option & BG_NOFLUSH) == 0) {
				UNLOCK(&buf_lists, *sp);
				bp->b_flags |= B_ASYNC | B_AGE;
				(void)bwrite(bp);
				*sp = LOCK(&buf_lists, FS_BUFLISTPL);
				goto loop;
			} else if ((option & BG_NOFLUSH) != 0) {
				/*
				 * Put the buffer at the tail of free list.
				 */
				bbackfree(bp, &bfreelist);
				SLEEP_UNLOCK(&bp->b_avail);
				/*
				 * If we have come through this path and
				 * was not able to recycle a free buffer,
				 * return.
				 */
				if (tried == B_TRUE) {
					UNLOCK(&buf_lists, *sp);
					return (NULL);
				} else {
					tried = B_TRUE;
					goto loop;
				}
			}
				
		}

		/*
		 * Remove this buffer from the hash lists. If
		 * it's the right size, then return that. Otherwise
		 * free the buffer and return the header for the
		 * free list. Take care to update the amount
		 * of space on the free list.
		 */
		bremhash(bp);

		UNLOCK(&buf_lists, *sp);

		if (bp->b_bufsize == bsize) {
			return (bp);
		}

		/*
		 * If size doesn't match, free buffer
		 * and add buffer header to bhdrlist.
		 *
		 * Save addr, size since we can't free memory
		 * while holding the spin lock.
		 */
		ASSERT((bp->b_bufsize > 0));
		addr = bp->b_un.b_addr;
		size = bp->b_bufsize;

		SLEEP_UNLOCK(&bp->b_avail);
		BUF_DEINIT(bp);
		struct_zero(bp, sizeof(buf_t));
		BUF_INIT(bp);

		kmem_free(addr, size);

		*sp = LOCK(&buf_lists, FS_BUFLISTPL);
		bp->av_forw = bhdrlist.av_forw;
		bhdrlist.av_forw = bp;

		bfreelist.b_bufsize += size;

		goto loop;
	}

	/*
	 * If there's not enough memory on the free list to satisfy
	 * the request, then block on <buf_free_space>. When we're
	 * awakened, return to the caller and check the hash queue again.
	 */	
	if (bfreelist.b_bufsize < bsize) {
		UNLOCK(&buf_lists, *sp);
		if ((option & BG_NOFREESPACE) == 0) {
			EVENT_WAIT(&buf_free_space, PRIBUF+1);
		}
		return (NULL);
	}

	/*
	 * Must allocate a new buffer which requires a buffer header.
	 * If we're out of buffer headers, drop the buf_lists lock and
	 * allocate a chunk; re-acquire the spin lock and add in the
	 * chunk we just allocated (if we didn't race with anybody).
	 */	
	bfreelist.b_bufsize -= bsize;
	if (bhdrlist.av_forw == NULL) {
		int i;

		UNLOCK(&buf_lists, *sp);
		dp = (buf_t *)kmem_zalloc(sizeof(buf_t) * (v.v_buf + 1),
				KM_SLEEP);
		ASSERT(dp != NULL);

		/*
		 * Initialize the newly allocated buffer
		 * headers.
		 */
		tdp = dp;
		dp++;
		for (i = 0; i < v.v_buf ; i++,dp++) {
			BUF_INIT(dp);
			dp->av_forw = dp + 1;
		}
		*sp = LOCK(&buf_lists, FS_BUFLISTPL);
		(--dp)->av_forw  = bhdrlist.av_forw;
		bhdrlist.av_forw = tdp + 1;
		nbuf += v.v_buf;
		tdp->av_forw = bufchain;
		bufchain = tdp;
	
	}


	/*
	 * Take a buffer header 
	 */
	bp = bhdrlist.av_forw;
	ASSERT(bp != NULL);
	bhdrlist.av_forw = bp->av_forw;
	UNLOCK(&buf_lists, *sp);
	bp->av_forw = bp->av_back = NULL;

	ASSERT(SLEEP_LOCKAVAIL(&bp->b_avail));
	(void) SLEEP_TRYLOCK(&bp->b_avail);
	ASSERT(!SLEEP_LOCKBLKD(&bp->b_avail));
	bp->b_flags |= B_BUSY;

	bp->b_un.b_addr  = (caddr_t)kmem_zalloc(bsize, KM_SLEEP|KM_DMA);
	bp->b_bufsize    = bsize;


	return (bp);
}		

/*
 * buf_t *
 * getblk(dev, blkno, bsize, option)
 *	Assign a buffer for the given block.  If the appropriate block is
 *	already associated, return it; otherwise search for the oldest
 *	non-busy buffer and reassign it.
 *
 * Calling/Exit State:
 *	No locks held on entry; buffer's allocation lock is held
 *	by the calling LWP on exit.
 *
 *	<Option> is used to handle special cases if the block is found
 *	or not found in the cache. BG_NOHIT instructs getblk()
 *	to return 0 if the requested block is in the buffer cache; if
 *	not found in the cache, a buffer header may be returned 
 *	to allow the caller to start an asynchronous I/O. BG_NOMISS
 *	instructs getblk() to return 0 if a buffer matching the requested
 *	block is not in the buffer cache; if a buffer for the requested
 *	block exists, a pointer to it is returned. BG_NOFREESPACE
 *	instructs getblk() to not block if the requested block is
 *	not in the cache *and* there's not enough space on the free
 *	list to satisfy the caller's request. BG_NOWAIT instructs
 *	getblk() to return 0 if the requested buffer is in the cache
 *	and in use by another LWP.
 *
 * Description:
 *	After computing the hash value for <dev, blkno>,
 *	<buf_lists> is obtained to maintain the state of the
 *	hash/free lists. If the requested block is found and it's
 *	allocation lock is held by another LWP, <buf_lists> is
 *	dropped after enqueuing on the requested buffer's allocation
 *	lock. Brelse() will notice this an give the buffer to the
 *	LWP. If the buffer is available, it's taken and then
 *	buf_lists is released.
 *
 *	If the buffer is not in the hash list, then try to get one from
 *	the free list. 	Since <buf_lists> may be dropped, must re-verify
 *	that the buffer was entered into the hash list. If it was
 *	re-entered, release the buffer obtained from the free list. If
 *	it wasn't re-entered, than finish the allocation process.
 */
buf_t *
getblk(dev_t dev, daddr_t blkno, long bsize, int option)
{
	buf_t	*bp;
	buf_t	*dp;
	buf_t	*nbp;
	pl_t		s;
	int save_cookie;


	nbp = NULL; 

	/*
	 * Calculate the hash bucket for the <dev,blkno> pair.
	 * 
	 * Search the cache for the block. If we find it, and
	 * the buffer is in use, then wait until it's available.
	 *
	 * Since we have the <buf_lists> lock, there are no other
	 * LWPs searching or modifying the hash/free lists.
	 *
	 * Note that if we wait on a buffer becoming available,
	 * there is no need to loop when we get it since brelse()
	 * will give it to us instead of putting it on the
	 * free list since we're waiting.
	 */

	dp = bhash(dev, blkno);

	ASSERT(dp != NULL);

	s = LOCK(&buf_lists, FS_BUFLISTPL);

loop:
	bp = buf_search_hashlist(dp, dev, blkno);
	if (bp != NULL) {
		/*
		 * Found the requested buffer. If it's not
		 * immediately available, wait for it. Otherwise,
		 * pull it off the free list and in either case,
		 * use it. Brelse() notices whether any LWPs are
		 * waiting for the buffer; if there are, it gives
		 * the buffer to the blocked LWP even if an error
		 * occurred on it.
		 */

		/*
		 * If the caller doesn't want it from the cache, return.
		 * (breada() uses this option).
		 */

		if (option & BG_NOHIT) {
			UNLOCK(&buf_lists, s);
			return ((buf_t *)NULL);
		}

		/*
		 * If the buffer found is a private buffer for pageout or
		 * some other critical process, wait until the buffer is
		 * released.
		 */
		if (bp->b_flags & B_PRIVBLK)  {

			if (option & BG_NOWAIT) {
				UNLOCK(&buf_lists, s);
				return ((buf_t *)NULL);
			}

			SV_WAIT(&getblk_private_sv, PRIMEM, &buf_lists);
			s = LOCK(&buf_lists, FS_BUFLISTPL);
			goto loop;
		}

		if ((!SLEEP_LOCKAVAIL(&bp->b_avail)) 
			|| SLEEP_LOCKBLKD(&bp->b_avail)) {
			/*
			 * Return NULL if the caller doesn't want
			 * the buffer if it's in the cache but in
			 * use by another LWP.
			 */
			if (option & BG_NOWAIT) {
				UNLOCK(&buf_lists, s);
				return ((buf_t *)NULL);
			}
			MET_IOWAIT(1);
			SLEEP_LOCK_RELLOCK(&bp->b_avail, PRIBUF+1, &buf_lists);
			MET_IOWAIT(-1);

			if (bp->b_bcount != bsize) {
				pboverlap(bp, bsize);
				/*
				 * overlapping I/O has not been completed.
				 * Loop back and research hashlist.
				 */
				brelse(bp);
				s = LOCK(&buf_lists, FS_BUFLISTPL);
				goto loop;
			}

			bp->b_flags |= B_BUSY;
			if (bp->b_flags & (B_ERROR | B_STALE)) {
				brelse(bp);
				s = LOCK(&buf_lists, FS_BUFLISTPL);
				goto loop;
			}
		} else {
			notavail(bp);
			bp->b_flags &= ~B_AGE;
			UNLOCK(&buf_lists, s);
		}

		return (bp);
	}

	/*
	 * The buffer's not in the hash list. Try to re-use
	 * one from the free list.
	 */

	/*
	 * Return NULL if the caller doesn't want the buffer
	 * if it's not in the cache.
	 */
	if (option & BG_NOMISS) {
		UNLOCK(&buf_lists, s);
		return ((buf_t *)NULL);
	}
	save_cookie = buf_hash_cookie;

	if (NOMEMWAIT())
		option |= BG_NOFREESPACE;
	bp = getfreeblk(bsize, option, 1, &s); /* buf lists locked on entry */

	/*
	 * bp will be NULL if we blocked on free space to become
	 * available. BG_NOFREESPACE is specified by breada()
	 * so it doesn't block on free space on read-ahead buffer.
	 */
	if (bp == NULL) {
		if (NOMEMWAIT()) {
			getblk_private();
			bp = &getblk_private_buf;
			(void) SLEEP_TRYLOCK(&bp->b_avail);
			bp->b_flags |= B_PRIVBLK;
		} else {
			if (option & BG_NOFREESPACE)
				return ((buf_t *)NULL);
			s = LOCK(&buf_lists, FS_BUFLISTPL);
			goto loop;
		}
	}

	s = LOCK(&buf_lists, FS_BUFLISTPL);
	/*
	 * Verify that the buffer we're interested in hasn't been
	 * added to the hash list while we weren't holding buf_lists.
	 */
	if (save_cookie != buf_hash_cookie) {
		nbp = buf_search_hashlist(dp, dev, blkno);
	}
	if (nbp != NULL) {
		/* duplicate -> release this buffer */
		bhashself(bp);
		UNLOCK(&buf_lists, s);
		bp->b_edev   = (dev_t)NODEV;
		/* B_KERNBUF and B_BUSY should already be set; clear the rest */
		ASSERT((bp->b_flags & (B_KERNBUF|B_BUSY)) ==
			(B_KERNBUF|B_BUSY));
		if ((bp->b_flags & B_PRIVBLK) == B_PRIVBLK)
			bp->b_flags  = B_KERNBUF | B_BUSY | B_PRIVBLK;
		else 
			bp->b_flags  = B_KERNBUF | B_BUSY;
		bp->b_bcount = bsize;
		brelse(bp);
		s = LOCK(&buf_lists, FS_BUFLISTPL);
		goto loop;
	}

	/* 
	 * Insert into the hash list. Release the lock
	 * after adding the buffer to the hash list
	 * and assigning the buffer it's identity
	 * (currently: blockno, edev).
	 */
	bp->b_blkno  = blkno;
	bp->b_edev   = dev;
	/* B_KERNBUF and B_BUSY should already be set; clear the rest */
	ASSERT((bp->b_flags & (B_KERNBUF|B_BUSY)) == (B_KERNBUF|B_BUSY));
	if ((bp->b_flags & B_PRIVBLK) == B_PRIVBLK)
		bp->b_flags  = B_KERNBUF | B_BUSY | B_PRIVBLK;
	else
		bp->b_flags  = B_KERNBUF | B_BUSY;
	bp->b_bcount = bsize;
	bp->b_resid = 0;
	bp->b_iodone = NULL;
	bp->b_writestrat = NULL;
	bp->av_forw = bp->av_back = bp;
	buf_hash_cookie++;
	binshash(bp, dp);
	UNLOCK(&buf_lists, s);
	return (bp);
}

/*
 * buf_t *
 * ngeteblk(bsize)
 *	Get an empty block, not assigned to any particular device.
 *
 * Calling/Exit State:
 *	No locks held on entry; buffer returned exclusively locked and
 *	on the free list.
 *
 * Description:
 *	Use getfreeblk() to return a buffer with size <bsize>.
 *	When that returns a non-NULL buffer pointer, initialize
 *	the buffer and return.
 */
buf_t *
ngeteblk(long bsize)
{
	buf_t	*bp;
	pl_t		s;

	/*
	 * Take block from free list. No lock
	 * held on entry; no BG_xxxx options
	 * specified.
	 */

	do {
		bp = getfreeblk(bsize, 0, 0, &s);
	} while (bp == NULL);

	/* 
	 * Currently this block is not on any list so there is no locking 
	 * required. 
	 *
	 * B_KERNBUF and B_BUSY should already be set; clear the rest
	 * and turn on B_AGE.
	 */
	ASSERT((bp->b_flags & (B_KERNBUF|B_BUSY)) == (B_KERNBUF|B_BUSY));
	bp->b_flags  = B_KERNBUF | B_BUSY | B_AGE;
	bp->b_edev   = (dev_t)NODEV;
	bp->b_bcount = bsize;
	bp->b_iodone = NULL;
	bp->b_writestrat = NULL;
	return (bp);
}

/*
 * buf_t *
 * geteblk(void)
 *	Get an empty 1024 byte block, not assigned to any particular device.
 *	Interface of geteblk() is kept intact to maintain driver compatibility.
 *
 * Calling/Exit State:
 *	No locks held on entry; buffer returned exclusively locked
 *	by the calling LWP.
 *
 * Description:
 *	Use ngeteblk() to allocate block size other than 1 KB.
 */
buf_t *
geteblk(void)
{
	return (ngeteblk((long)1024));
}

/*
 * void
 * clrbuf(buf_t *bp)
 *	Zero the buffer associated with a buffer header.
 *
 * Calling/Exit State:
 *	The calling LWP must own the buffer.
 *
 * Description:
 *	Use bzero() to fill the buffer with zeros.
 */
void
clrbuf(buf_t *bp)
{
	bzero(bp->b_un.b_addr, bp->b_bcount);
	bp->b_resid = 0;
}

#undef bioerror

/*
 * void
 * bioerror(buf_t *bp, int errno)
 *	Set or clear the error flag and error field in a buffer header.
 *
 * Calling/Exit State:
 *	The calling LWP must own the buffer.
 *
 * Description
 *	If <errno> is non-zero, set <bp->b_error> and B_ERROR. If
 *	<errno> is zero, clear <bp->b_error> and B_ERROR.
 *
 */
void
bioerror(buf_t *bp, int errno)
{

	bp->b_error = errno;
	if (errno) {
		bp->b_flags |= B_ERROR;
	} else {
		bp->b_flags &= ~B_ERROR;
	}

	return;
}

/*
 * void
 * bflush(dev_t dev)
 *	Make sure all write-behind blocks on dev (or NODEV for all)
 *	are flushed out.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *
 * Description:
 *	The free list is searched for buffers belonging to <dev>
 *	or NODEV while holding <buf_lists>. If one is found and
 *	it's available and marked for delayed write (B_DELWRI),
 *	an asynchronous write is started for it.
 *
 *	After each I/O is started, <buf_lists> is reacquired and
 *	the procedure is repeated.
 *
 *	Note that other LWPs can be placing buffer headers into
 *	the free list on windows when buf_lists is unlocked. This
 *	could conceiveably result in this procedure never returning.
 *	This "cannot" happen since it requires processors to produce
 *	buffers at a rate faster than this can start I/O on them
 *	for a continued period of time. Also note that callers using
 *	NODEV are heuristic (from an update operation); callers passing
 *	<dev> are doing so on the last close on the device and thus,
 *	there's no concurrent activity that could place more buffers
 *	for <dev>.
 *
 */
void
bflush(dev_t dev)
{
	buf_t *bp;
	pl_t		pri;
	buf_t *bmarkerp;
	buf_t *bpnext;

        /*
         *      Initialize marker buffer
         *
         *      Use the marker buffer to traverse the
         *      list and flush all eligble buffers.
         */
	bmarkerp = kmem_zalloc(sizeof(buf_t), KM_SLEEP);
        init_bmarker(bmarkerp);

        /* Set the pointer to the first buffer on the list */

	pri = LOCK(&buf_lists, FS_BUFLISTPL);
	bpnext = bfreelist.av_forw;
loop:
	for (bp = bpnext; bp != &bfreelist; bp = bp->av_forw) {

		ASSERT(bp != NULL);

		/*
		 * Find a matching buffer. Must make sure buffer isn't in
		 * use before checking flags.
		 */
		if ((dev == NODEV || dev == bp->b_edev) &&
		    (bp->b_flags & B_DELWRI)) {

			ASSERT((bp->b_flags & B_MARKER) == 0);
			ASSERT(SLEEP_LOCKAVAIL(&bp->b_avail));
			ASSERT(!SLEEP_LOCKBLKD(&bp->b_avail));
			
			bfrontfree(bmarkerp, bp);
			notavail(bp);
			UNLOCK(&buf_lists, pri);
			bp->b_flags |= B_ASYNC;
			(void)bwrite(bp);
			(void) LOCK(&buf_lists, FS_BUFLISTPL);
			bpnext = bmarkerp->av_forw;
			bremfree(bmarkerp);
			goto loop;
		}
	}
	UNLOCK(&buf_lists, pri);
	kmem_free(bmarkerp, sizeof(buf_t));
}

/*
 * void
 * bdflush(clock_t max_age)
 *	As a part of file system hardening walk down the free list
 *	and flush all write-behind buffers in the buffer cache older
 *	than max_age ticks.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *
 * Description:
 *	The free list is searched for buffers while holding <buf_lists>.
 *	If one is found and it's available and marked for delayed 
 *	write (B_DELWRI) and old enough, a dummy buffer as a marker
 *	is inserted  *	*after* it and an asynchronous write is started
 *	for the buffer.
 *
 *	After each I/O is started, <buf_lists> is reacquired, the
 *	marker buffer is removed from the list and search is continued
 *	from the point where the last buffer was found.
 *
 *	Note that other LWPs can be placing buffer headers into
 *	the free list on windows when buf_lists is unlocked. The
 *	buffers placed at the end of the list will be written this time;
 *	the buffers placed at the head of the list will be written
 *	during the next "sweep" through the list.
 *	In either case, with each invocation this function will make
 *	a single, complete pass through the list from the head to
 *	the tail.
 *
 */
void
bdflush(clock_t max_age)
{
	buf_t *bp;
	pl_t	pri;
	buf_t *bmarkerp;
	buf_t *bpnext;

	/*
	 * 	Initialize marker buffer
	 *
	 *	Use the marker buffer to traverse the
	 *	list and flush all eligble buffers.
	 */
	bmarkerp = kmem_zalloc(sizeof(buf_t), KM_SLEEP);
	init_bmarker(bmarkerp);

	/* Set the pointer to the first buffer on the list */

	pri = LOCK(&buf_lists, FS_BUFLISTPL);
	bpnext = bfreelist.av_forw;
loop:
	for (bp = bpnext; bp != &bfreelist; bp = bp->av_forw) {

		ASSERT(bp != NULL);

		if ((bp->b_flags & (B_DELWRI|B_MARKER|B_STALE)) == B_DELWRI && 
		    lbolt - bp->b_start >= max_age) {

			ASSERT(SLEEP_LOCKAVAIL(&bp->b_avail));
			ASSERT(!SLEEP_LOCKBLKD(&bp->b_avail));

			bfrontfree(bmarkerp, bp);
			notavail(bp);
			UNLOCK(&buf_lists, pri);
			bp->b_flags |= B_ASYNC;
			bwrite(bp);
			(void) LOCK(&buf_lists, FS_BUFLISTPL);
			bpnext = bmarkerp->av_forw;
			bremfree(bmarkerp);
			goto loop;
		}
	}
	UNLOCK(&buf_lists, pri);
	kmem_free(bmarkerp, sizeof(buf_t));
}

/*
 * void
 * blkflush(dev_t dev, daddr_t blkno)
 *	Ensure that a specified block is up-to-date on disk.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *	Loop through the buffer lists while holding <buf_lists>. If a match
 *	is found, "take" the buffer by grabbing it's alloc lock (bp->b_avail).
 *	Start a write if the buffer is dirty (bp->b_flags & B_DELWRI and no
 *	errors); otherwise, just brelse() it.
 */
void
blkflush(dev_t dev, daddr_t blkno)
{
	buf_t	*bp;
	buf_t	*dp;
	pl_t		s;

	dp = bhash(dev, blkno);
	s = LOCK(&buf_lists, FS_BUFLISTPL);
	bp = buf_search_hashlist(dp, dev, blkno);
	if (bp != NULL) {
		/*
		 * Found one. 'Allocate' the buffer and start
		 * I/O on it if it's dirty, i.e., B_DELWRI is set.
		 */
		if ((!SLEEP_LOCKAVAIL(&bp->b_avail)) || 
	             SLEEP_LOCKBLKD(&bp->b_avail)) {
			MET_IOWAIT(1);
			SLEEP_LOCK_RELLOCK(&bp->b_avail, PRIBUF+1,  &buf_lists);
			MET_IOWAIT(-1);
			bp->b_flags |= B_BUSY;
			if ((bp->b_flags & (B_STALE|B_ERROR)) == 0 &&
			    (bp->b_flags & B_DELWRI)) {
				(void)bwrite(bp);	/* synchronous */
			} else {
				brelse(bp);
			}
			return;

		} else if (bp->b_flags & B_DELWRI) {
			/*
			 * It's available and dirty. "Allocate" it,
			 * release the list lock and synchronously update
			 * it and return.
			 */
			notavail(bp);
			UNLOCK(&buf_lists, s);
			bp->b_flags &= ~B_ASYNC;
			(void)bwrite(bp);
			return;
		} else {
			/*
			 * It's available and not dirty, so just return
			 */
			UNLOCK(&buf_lists, s);
			return;
		}
	}
	UNLOCK(&buf_lists, s);

	return;
}

/*
 * void
 * bdwait(dev_t)
 *	Wait for asynchronous writes to finish.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *    Wait for all asynchronous scheduled i/o to complete
 *    for a particular device (or any device if NODEV).
 *    Normally called when unmounting/remounting a file system.
 *
 * Remarks:
 *	The way this is handled is correct, but much too granular. The
 *	count should be maintained on a per-dev data structure. It's
 *	sufficient, however, given it's current use from xxx_mountroot
 *	and re-mounts of root.
 *
 * 	Replaces original bdwait() which used basyncnt
 *      to keep track of asynchronous i/o.
 *	This way it solved the unmounting of a persistent file system.
 */
void
bdwait(dev_t dev)
{
	buf_t   *bp;
	buf_t	*bc;
	pl_t	s;

	s = LOCK(&buf_lists, FS_BUFLISTPL);
bloop:
	bc = bufchain;
	while (bc) {
               for (bp = bc + v.v_buf; bp != bc; bp--) {
                       if ((bp->b_flags & (B_DONE|B_ASYNC)) == B_ASYNC &&
                               (dev == NODEV || dev == bp->b_edev)) {
				UNLOCK(&buf_lists, s);
				LBOLT_WAIT(PRIBUF);
				(void) LOCK(&buf_lists, FS_BUFLISTPL);
				goto bloop;
                       }
               }
               bc = bc->av_forw;
	}
	UNLOCK(&buf_lists, s);
}

/*
 * void
 * binval(dev_t dev)
 *	Invalidate blocks for a device after last close.
 *
 * Calling/Exit State:
 *	No locks held on entry; no locks held on exit.
 *
 * Description:
 *	This is done by marking each buffer associated with <dev> invalid.
 *	and is called when closing a device to insure that future references
 *	to it will cause the data to be fetched from disk and not from
 *	the buffer cache. This should be preceded by bflush() to flush any
 *	modified blocks to disk.
 *
 *	We loop on each hash bucket until the hash bucket no longer contains
 *	relevant buffer headers. This is OK since there is no concurrent
 *	activity on the same device. Thus, the algorithm will terminate.
 */
void
binval(dev_t dev)
{
	buf_t	*dp;
	buf_t	*bp;
	int		i;
	pl_t		s;


	for (i = 0; i < v.v_hbuf; i++) {
		dp = (buf_t *)&hbuf[i];
	loop:	if (dp->b_forw == (buf_t *)&hbuf[i]) {
			/* empty hash bucket */
			continue;
		}
		s = LOCK(&buf_lists, FS_BUFLISTPL);
		for (bp = dp->b_forw; bp != dp; bp = bp->b_forw) {
			if (bp->b_edev == dev) {
				if (SLEEP_LOCKAVAIL(&bp->b_avail)
				    && !SLEEP_LOCKBLKD(&bp->b_avail)) {
					notavail(bp);
					UNLOCK(&buf_lists, s);
				} else {
					SLEEP_LOCK_RELLOCK(&bp->b_avail,
							   PRIBUF+1,
							   &buf_lists);
					bp->b_flags |= B_BUSY;
				}
				/*
				 * Found one. Invalidate it by
				 * setting B_STALE. Use B_AGE
				 * to put on head of free list.
				 * Take it out of the hash list.
				 */
				s = LOCK(&buf_lists, FS_BUFLISTPL);
				bremhash(bp);
				bp->b_flags |= B_STALE | B_AGE;
				UNLOCK(&buf_lists, s);
				brelse(bp);
				goto loop;
			}
		}
		UNLOCK(&buf_lists, s);
	}

	return;
}

extern struct tune_point BUFHWMcurve[];
extern int BUFHWMcurvesz;
/*
 * void
 * binit(void)
 *	Initialize the buffer I/O system by freeing all buffers and setting
 *	all device hash buffer lists to empty.
 *
 * Calling/Exit State:
 *	On exit, all buffer cache data structures are allocated and/or
 *	initialized.
 */
void
binit(void)
{
	buf_t	*bp;
	unsigned	i;
	extern void pageio_init(void);

	v.v_bufhwm = tune_calc(BUFHWMcurve, BUFHWMcurvesz);

	/*
	 * Change buffer memory usage high-water-mark from kbytes
	 * to bytes.
	 */
	bfreelist.b_bufsize = v.v_bufhwm * 1024;
	nbuf = 0;

	/*
	 * Initialize lists
	 */
	bp = &bfreelist;
	bp->b_forw = bp->b_back = bp->av_forw = bp->av_back = bp;
	bhdrlist.av_forw = NULL;
	for (i = 0; i < v.v_hbuf; i++) {
		bp = (buf_t *)&hbuf[i];
		bp->b_forw = bp->b_back = bp;
	}

	/*
	 * Initialize Synchronization Objects
	 */
	LOCK_INIT(&buf_lists, FS_BUFLISTHIER, FS_BUFLISTPL,
		  &buf_lists_lkinfo, KM_SLEEP);

	FSPIN_INIT(&buf_async_lock);
	EVENT_INIT(&bdelay_event);

	EVENT_INIT(&buf_free_space);

	SV_INIT(&getblk_private_sv);
	BUF_INIT(&getblk_private_buf);
	getblk_private_buf.b_un.b_addr = kmem_zalloc(MAXBSIZE,
						     KM_SLEEP|KM_DMA);

	pageio_init();
}

/*
 * int
 * biowait(buf_t *bp)
 *	Wait for I/O completion on the buffer; return error code.
 *
 * Calling/Exit State:
 *	The buffer's allocation lock is held by the calling LWP
 *	on entry; remains held on exit.
 *
 *	If there was an error during I/O, it is returned to
 *	the caller; 0 is returned if no error.
 *
 *	When we return, B_DONE will be set for this buffer.
 *
 * Description:
 *	Await the buffer's completion event (b_iowait) to be
 *	signaled (from biodone()). When the I/O completes,
 *	get any error status (geterror) to return to the caller.
 *
 *	For page-level I/O completions, notify VM (pvn_done()); for
 *	B_REMAPPED I/Os, call bp_mapout().
 */
int
biowait(buf_t *bp)
{
	int	error;

	/*
	 * Could be racing with biodone() here. Always consume event
	 * since it's always posted. Note that even if EVENT_WAIT
	 * happens after the EVENT_SIG, it's remembered.
	 */

	ASSERT(bp->b_flags & B_KERNBUF);
        ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail));
	ASSERT(bp->b_flags & B_BUSY);

wait:
	MET_IOWAIT(1);
	EVENT_WAIT(&bp->b_iowait, PRIBUF);
	MET_IOWAIT(-1);

	/*
	 * Now, only calling context has access to bp.
	 */

	/* Do delayed I/O done processing, if needed. */
	if (bp->b_flags & B_DELAYDONE) {
		bp->b_flags &= ~B_DELAYDONE;
		ASSERT(bp->b_iodone != NULL);
		(*bp->b_iodone)(bp);
		goto wait;
	}

	error = geterror(bp);

	if (bp->b_flags & B_REMAPPED)
		bp_mapout(bp);
	if ((bp->b_flags & (B_PAGEIO|B_PARTIAL)) == B_PAGEIO)
		pvn_done(bp);

	return (error);
}

/*
 * boolean_t
 * biowait_sig(buf_t *bp)
 *	Wait for I/O completion on the buffer; can be awakened by signals.
 *
 * Calling/Exit State:
 *	The buffer's allocation lock is held by the calling LWP
 *	on entry; remains held on exit.
 *
 *	Returns B_TRUE when the I/O has completed (biodone has been called).
 *	If the wait is interrupted by a signal, B_FALSE will be returned
 *	and it is the caller's responsibility to cause the transfer to be
 *	aborted (and biodone to be called) and then call biowait or
 *	biowait_sig again.
 *
 * Description:
 *	Behaves like biowait() unless the wait is interrupted by a signal.
 */
boolean_t
biowait_sig(buf_t *bp)
{
	boolean_t normal_wakeup;

	/*
	 * Could be racing with biodone() here. Always consume event
	 * since it's always posted. Note that even if EVENT_WAIT
	 * happens after the EVENT_SIG, it's remembered.
	 */

	ASSERT(bp->b_flags & B_KERNBUF);
        ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail));
	ASSERT(bp->b_flags & B_BUSY);

wait:
	MET_IOWAIT(1);
	normal_wakeup = EVENT_WAIT_SIG(&bp->b_iowait, PRIBUF);
	MET_IOWAIT(-1);

	if (!normal_wakeup)
		return B_FALSE;

	/*
	 * Now, only calling context has access to bp.
	 */

	/* Do delayed I/O done processing, if needed. */
	if (bp->b_flags & B_DELAYDONE) {
		bp->b_flags &= ~B_DELAYDONE;
		ASSERT(bp->b_iodone != NULL);
		(*bp->b_iodone)(bp);
		goto wait;
	}

	if (bp->b_flags & B_REMAPPED)
		bp_mapout(bp);
	if ((bp->b_flags & (B_PAGEIO|B_PARTIAL)) == B_PAGEIO)
		pvn_done(bp);

	return B_TRUE;
}

/*
 * void
 * biodone(buf_t *bp)
 *	Mark I/O complete on a buffer, release it if I/O is asynchronous,
 *	and wake up anyone waiting for it.
 *
 * Calling/Exit State:
 *	May be called at interrupt level. Generally, no locks are
 *	held on either entry or exit. However, if this is the completion of
 *	an asynchronous I/O, the buffer's allocation lock is held
 *	on exit but will be released via brelse().
 *
 * Description:
 *	Indirect through <bp->biodone> if <bp->biodone> is set.
 *
 *	The I/O is marked complete by setting B_DONE. Call basyncdone()
 *	for asynchronous I/O completions, which decrements the number of
 *	outstanding writes if a write completes and if a page-level
 *	I/O completes puts the buffer header on the bclnlist and does
 *	brelse for other I/O completions.
 *
 *	If this was a synchronous I/O, then signal the <bp->b_iowait> event
 *	to awake the LWP awaiting this buffer's I/O completion in biowait().
 */
void
biodone(buf_t *bp)
{
	ASSERT(bp->b_flags & B_KERNBUF);

	if (bp->b_iodone) {
		(*bp->b_iodone)(bp);
		return;
	}

	ASSERT(!(bp->b_flags & (B_DONE|B_DELAYDONE)));
	bp->b_flags |= B_DONE;

	/* 
	 * There isn't a waiter for asynchronous I/O.
	 *
	 * If async I/O, call basyncdone.
	 */
	if (bp->b_flags & B_ASYNC)
		basyncdone(bp);
	else
		EVENT_SIGNAL(&bp->b_iowait, 0);
}

/*
 * void
 * basyncdone(buf_t *bp)
 *	Note the completion of an asynchronous I/O and if B_PAGEIO flag is
 *	for the bp, put the buffer header on the bclnlist. 
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 *
 * Description:
 *	In case of B_PAGEIO, wake up the waiters on the first
 *	PAGE_WAIT()ed page on the b_pages list. The process PAGE_WAIT()ing will
 *	then call cleanup() thereby causing pvn_done on this buffer header and
 *	waking up all the waiters on the other pages. This is good because
 *	we don't want cleanup() called for every page on the b_pages list.
 */
void
basyncdone(buf_t *bp)
{
	page_t *pp = bp->b_pages;
	uint_t numpages;

	ASSERT(bp->b_flags & B_ASYNC);
	ASSERT(!(bp->b_flags & B_PHYS));
	ASSERT(!(bp->b_flags & B_DELAYDONE));

	if (bp->b_flags & (B_PAGEIO|B_REMAPPED)) {
		FSPIN_LOCK(&buf_async_lock);
		bp->av_forw = bclnlist;
		bclnlist = bp;
		if (!(bp->b_flags & B_WASPHYS)) {
			ASSERT(pp);
			numpages = bp->b_numpages;
			ASSERT(numpages != 0);
			do {
				if (WAITERS_ON_PAGE(pp)) {
					FSPIN_UNLOCK(&buf_async_lock);
					PAGE_BROADCAST_U(pp);
					return;
				}
				pp = pp->p_next;
			} while (--numpages != 0);
		}
		FSPIN_UNLOCK(&buf_async_lock);
	} else {
		brelse(bp);
	}
}

/*
 * STATIC void
 * clean_list(buf_t *clnlist)
 *	Go through the clnlist and call pvn_done() on the buffer
 *	headers with B_PAGEIO set.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
STATIC void
clean_list(buf_t *clnlist)
{
	buf_t *bp;

	while ((bp = clnlist) != NULL) {
		clnlist = bp->av_forw;

		ASSERT(bp->b_flags & B_ASYNC);
		ASSERT(!(bp->b_flags & B_DELAYDONE));

		if (bp->b_flags & B_REMAPPED)
			bp_mapout(bp);
		ASSERT(bp->b_flags & B_PAGEIO);
		if (bp->b_flags & B_PARTIAL) {
			/* "Clone" buffer from pageio_breakup(). */
			pageio_done(bp);
		} else
			pvn_done(bp);
	}
}

/*
 * void
 * cleanup(void)
 *	Go through the bclnlist and call pvn_done() on the buffer
 *	headers with B_PAGEIO set.
 *
 * Calling/Exit State:
 *	No locks held on entry or exit.
 */
void
cleanup(void)
{
	buf_t *clnlist;

	FSPIN_LOCK(&buf_async_lock);
	clnlist = bclnlist;
	bclnlist = NULL;
	FSPIN_UNLOCK(&buf_async_lock);

	clean_list(clnlist);
}

/*
 * void
 * bdelaydone(buf_t *bp)
 *	Delayed I/O completion.  Put off iodone processing until no longer
 *	at interrupt level.
 *
 * Calling/Exit State:
 *	May be called at interrupt level. Generally, no locks are
 *	held on either entry or exit.
 *
 * Description:
 *	The buffer will be marked such that it will undergo normal biodone()
 *	processing from base level instead of interrupt level. For
 *	asynchronous I/O, this will be in bdelay_daemon(); for synchronous I/O,
 *	it will be done in biowait().
 */
void
bdelaydone(buf_t *bp)
{
	ASSERT(!(bp->b_flags & (B_DONE|B_DELAYDONE)));
	ASSERT(bp->b_iodone != NULL);

	bp->b_flags |= B_DELAYDONE;

	/* 
	 * There isn't a waiter for asynchronous I/O.
	 *
	 * If async I/O, queue it up for the bdelay daemon to handle.
	 */
	if (bp->b_flags & B_ASYNC) {
		FSPIN_LOCK(&buf_async_lock);
		bp->av_forw = bdelay_list;
		bdelay_list = bp;
		FSPIN_UNLOCK(&buf_async_lock);
		EVENT_SIGNAL(&bdelay_event, 0);
	} else
		EVENT_SIGNAL(&bp->b_iowait, 0);
}

/*
 * void
 * bdelay_daemon(void *arg)
 *	Process delayed B_ASYNC buffers.
 *
 * Calling/Exit State:
 *	No locks held on entry; never exits.
 *
 * Description:
 *	Wakes up whenever signalled by bdelay_event that there might be
 *	work to do.  Processes all buffers then on the bdelay_list, calling
 *	each one's iodone routine.  While we're at it, also call clean_list
 *	to process the bclnlist.
 */
/* ARGSUSED */
void
bdelay_daemon(void *arg)
{
	buf_t *dlist, *clnlist, *bp;

	u.u_lwpp->l_name = "bdelay";

	for (;;) {
		EVENT_WAIT(&bdelay_event, PRIBUF);

		FSPIN_LOCK(&buf_async_lock);
		dlist = bdelay_list;
		bdelay_list = NULL;
		clnlist = bclnlist;
		bclnlist = NULL;
		FSPIN_UNLOCK(&buf_async_lock);

		while ((bp = dlist) != NULL) {
			dlist = bp->av_forw;
			ASSERT(bp->b_flags & B_DELAYDONE);
			bp->b_flags &= ~B_DELAYDONE;
			ASSERT(bp->b_iodone != NULL);
			(*bp->b_iodone)(bp);
		}

		clean_list(clnlist);
	}
}


#undef bioreset
/*
 * void
 * bioreset(buf_t *bp)
 *	Reset a buffer after completed I/O so it can be used again.
 *	(DDI/DKI interface.)
 *
 * Calling/Exit State:
 *	The calling LWP must own the buffer.
 */
void
bioreset(buf_t *bp)
{
	ASSERT(bp->b_flags & B_KERNBUF);
	ASSERT(!SLEEP_LOCKAVAIL(&bp->b_avail));
	ASSERT(bp->b_flags & B_BUSY);

	bp->b_flags &= ~(B_DONE|B_ERROR);
	bp->b_error = 0;
}

#undef geterror
/*
 * int
 * geterror(buf_t *bp)
 *	Pick up the device's error number and pass it to the user;
 *	if there is an error but the number is 0 set a generalized code.
 *
 * Calling/Exit State:
 *	The calling LWP must own the buffer.
 *
 *	If there is an error but <bp->b_error> is 0, then return EIO.
 *
 */
int
geterror(buf_t *bp)
{
	int	error;

	ASSERT(bp->b_flags & B_KERNBUF);

	if (bp->b_flags & B_ERROR) {
		if ((error = bp->b_error) == 0)
			error = EIO;
	} else
		error = 0;

	return (error);
}

/*
 * buf_t *
 * getrbuf(int sleep)
 *	Allocate buffer header.
 *	'sleep' can be KM_SLEEP or KM_NOSLEEP.
 *
 * Calling/Exit State:
 *
 *	Returns a buffer header or NULL if memory couldn't be allocated.
 *
 */
buf_t *
getrbuf(int sleep)
{
	buf_t *bp;

	bp = kmem_zalloc(sizeof(buf_t), sleep);
	if (bp == NULL) {
		ASSERT(sleep & KM_NOSLEEP);
		return (NULL);
	}
	BUF_INIT(bp);
	SLEEP_LOCK_PRIVATE(&bp->b_avail);
	bp->b_flags |= B_BUSY;
	return (bp);
}

/*
 * void
 * freerbuf(buf_t *)
 *	free up space allocated by getrbuf()
 *
 * Calling/Exit State:
 *
 *	None
 *
 */
void
freerbuf(buf_t *bp)
{
	if (bp->b_flags & B_REMAPPED)
		bp_mapout(bp);
	SLEEP_UNLOCK(&bp->b_avail);
	BUF_DEINIT(bp);
	kmem_free(bp, sizeof(buf_t));
}

/*
 * buf_t *
 * pbread(dev_t dev, daddr_t blkno, long bsize)
 *	Read in (if necessary) the physical block and return a buffer pointer.
 *	This routine takes the physical block number, and the length to read
 *	as input.  The old bread() routine reads only a logical block
 *	for the logical blocksize.  This new routine allows file systems
 *	that have fragmented blocks to read partial logical blocks containing
 *	control data without mapping the device special file via fbread.
 *
 * Calling/Exit State:
 *	The buffer returned is owned exclusively by the calling LWP.
 *	Other LWPs wanting this buffer will block on it's being brelse()'d.
 *
 *	The returned buffer should be checked for B_ERROR, etc.
 *
 * Description:
 *	Use pgetblk() to locate the buffer. If I/O has completed on the
 *	buffer (indicated by B_DONE being clear), then initiate I/O to read in
 *	the data and synchronously wait for it.
 */
buf_t *
pbread(dev_t dev, daddr_t blkno, long bsize)
{
	buf_t	*bp;

	MET_LREAD();

	/*
	 * First, locate the requested block.
	 */
	bp = pgetblk(dev, blkno, bsize);

	/*
	 * The calling LWP now 'owns' the buffer
	 * so its ok to inspect flags in a non-racy way.
	 * If B_DONE is not set, then start the I/O and wait
	 * for it.
	 */
	if ((bp->b_flags & B_DONE) == 0) {
		buf_start_read(dev, bp, B_READ, bsize);
		(void) biowait(bp);
	}

	/*
	 * Return the buffer.
	 */
	return (bp);
}

/*
 * STATIC int
 * pboverlap(buf_t *bp, long size)
 *	Check for overlap of this buffer with any other allocated buffer.
 *	Similar to old ufs "brealloc" code.
 *
 * Calling/Exit State:
 *	Buffer is locked by the calling LWP.
 *
 *	If 0 is returned, then bp is marked delayed write and the calling
 *	LWP is only interested in a portion of the buffer, size < bp->b_count.
 *	In this case and the buffer is dirty, the buffer is given to bwrite()
 *	- the calling LWP cannot reference bp anymore.
 *
 * Description:
 *	Make sure overlapping I/O on bp has been completed. If size
 *	is less than bp->b_count and it's dirty (B_DELWRI), than
 *	give the buffer to bwrite() and 0 is returned. 
 *
 *	Search the buffer cache for buffers that overlap the one we're
 *	looking at. If there are any such buffers, they're invalidated.
 *	Note that 2 buffers may be held at this point. Deadlock is not
 *	possible since the buffer cache is only used for file system
 *	meta-data, and in the pbread() case, only for accessing ACL
 *	information. Users of block special files use the page cache
 *	for accessing data.
 */
STATIC int
pboverlap(buf_t *bp, long size)
{
	daddr_t	start;
	daddr_t	last;
	buf_t	*ep;
	buf_t	*dp;
	pl_t	s;

	/*
	 * First need to make sure that all overlapping previous
	 * I/O is dispatched.
	 */
	
	if (size == bp->b_bcount) {
		return 1;
	}

	bp->b_flags &= ~B_DONE;
	if (bp->b_edev == (dev_t) 0) {
		return 1;
	}

	if (size < bp->b_bcount) {
		if (bp->b_flags & B_DELWRI) {
			(void)bwrite(bp);
			bp->b_flags |= B_STALE;
			return (0);
		}
		bp->b_flags |= B_STALE;
		return (1);
	}

	/*
	 * Search cache for any buffers that overlap the one that we are
	 * looking at. Overlapping buffers must be marked invalid, after being
	 * written out if they are dirty (B_DELWRI).
	 */

	start = bp->b_blkno;
	last  = start + btodb(size) - 1;
	dp    = bhash(bp->b_edev, start);

	s = LOCK(&buf_lists, FS_BUFLISTPL);
	for (ep = dp->b_forw; ep != dp; ep = ep->b_forw) {
		if ((ep->b_edev != bp->b_edev) ||
		    ((ep->b_flags & B_STALE) != 0)) 
			continue;
		/* look for overlap */
		if ((ep->b_bcount == 0) || (ep->b_blkno > last) ||
			((ep->b_blkno + btodb(ep->b_bcount)) <= start))
			continue;

		/*
		 * Found a buffer that overlaps. Allocate the buffer and
		 * cause I/O to be done. After I/O, invalidate the buffer
		 * by marking B_STALE.
		 */
		if ((!SLEEP_LOCKAVAIL(&ep->b_avail)) || 
		     SLEEP_LOCKBLKD(&ep->b_avail)) {
			MET_IOWAIT(1);
			SLEEP_LOCK_RELLOCK(&ep->b_avail, PRIBUF+1, &buf_lists);
			MET_IOWAIT(-1);
			bp->b_flags |= B_BUSY;
		} else {
			notavail(ep);
			UNLOCK(&buf_lists, s);
		}
		if ((ep->b_flags & B_DELWRI) != 0) {
			(void)bwrite(ep);
			ep->b_flags |= B_STALE;
			s = LOCK(&buf_lists, FS_BUFLISTPL);
			continue;
		}
		ep->b_flags |= B_STALE;
		brelse(ep);
		s = LOCK(&buf_lists, FS_BUFLISTPL);
	}	/* end for */

	UNLOCK(&buf_lists, s);
	return 1;
}

/*
 * buf_t *
 * pgetblk(dev_t dev, daddr_t blkno, long bsize)
 *	Assign a buffer for the given physical block.  If the appropriate
 *	block is already associated, return it; otherwise search
 *	for the oldest non-busy buffer and reassign it.
 *
 * Calling/Exit State:
 *	No locks held on entry; buffer returned exclusively
 *	locked.
 *
 * Description:
 *	Obtain <buf_lists>. Compute the hash value for <dev, blkno>.
 *	If we find it and it's not immediately available, then atomically
 *	release <buf_lists> while enqueuing on the buffer's allocation
 *	lock <bp->b_avail>. If it is immediately available, take it while
 *	also releasing <buf_lists> and obtaining <bp->b_avail>.
 *
 *	If the buffer is not in the hash list, then get one off the free
 *	list. Since <buf_lists> was dropped, must re-verify that the buffer
 *	was entered into the hash list. If it was re-entered, release the
 *	buffer obtained from the free list. If it wasn't re-entered, than
 *	finish the allocation process.
 */
buf_t *
pgetblk(dev_t dev, daddr_t blkno, long bsize)
{
	buf_t	*bp;
	buf_t	*dp; 
	buf_t	*nbp; 
	pl_t		s;

	nbp = NULL;


	dp = bhash(dev, blkno);
	ASSERT(dp != NULL);

loop:
	s = LOCK(&buf_lists, FS_BUFLISTPL);
	bp = buf_search_hashlist(dp, dev, blkno);
	if (bp != NULL) {
		if ((!SLEEP_LOCKAVAIL(&bp->b_avail)) ||
		     SLEEP_LOCKBLKD(&bp->b_avail)) {
			MET_IOWAIT(1);
			SLEEP_LOCK_RELLOCK(&bp->b_avail, PRIBUF+1, &buf_lists);
			MET_IOWAIT(-1);
			bp->b_flags |= B_BUSY;
			if (bp->b_flags & (B_ERROR | B_STALE)) {
				brelse(bp);
				goto loop;
			}
		} else {
			notavail(bp);
			UNLOCK(&buf_lists, s);
		}
		bp->b_flags &= ~B_AGE;
		if ((bp->b_bcount != bsize) && pboverlap(bp, bsize) == 0) {
			/*
			 * overlapping I/O has not been completed. bp
			 * was dropped in pboverlap() so loop and research
			 * hashlist.
			 */
			goto loop;
		}
		return (bp);
	}

	bp = getfreeblk(bsize, 0, 1, &s);
	if (bp == NULL) {
		goto loop;
	}
				
	s = LOCK(&buf_lists, FS_BUFLISTPL);
	nbp = buf_search_hashlist(dp, dev, blkno);
	if (nbp != NULL) {
		/*
		 * buf entered while buf_lists was dropped
		 * release buffer from free list
		 */
		UNLOCK(&buf_lists, s);
		brelse(bp);
		goto loop;
	}


	/*
	 * Insert the buffer into the hash list. Must assign
	 * buffer identity before releasing hash table lock.
	 */
	binshash(bp, dp);
	bp->b_edev   = dev;
	bp->b_blkno  = blkno;
	/* B_KERNBUF and B_BUSY should already be set; clear the rest */
	ASSERT((bp->b_flags & (B_KERNBUF|B_BUSY)) == (B_KERNBUF|B_BUSY));
	bp->b_flags  = B_KERNBUF | B_BUSY;
	UNLOCK(&buf_lists, s);
	if (pboverlap(bp, bsize) == 0) {
		/*
		 * There's still some overlapping I/O that
		 * hasn't finished yet. The buffer has been
		 * dropped in pboverlap(). Must search for
		 * it again.
		 */
		goto loop;
	}

	bp->b_bcount = bsize;
	bp->b_resid = 0;
	bp->b_iodone = NULL;
	bp->b_writestrat = NULL;

	return (bp);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_bfreelist_links(void)
 *	Debugging print of buffer free list.
 *	Can be invoked from kernel debugger.	
 *
 * Calling/Exit State:
 *      No locks held on entry; No locks held on exit.
 */
void
print_bfreelist_links(void)
{
	buf_t *bp;

	for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
		if (bp->b_flags & B_MARKER)
			debug_printf("Marker buffer \n");

		debug_printf("bp = %x, b_forw = %x, b_back = %x\n",
			     bp, bp->b_forw, bp->b_back);
		debug_printf("av_forw = %x, av_back = %x\n",
			     bp->av_forw, bp->av_back);
		if (debug_output_aborted())
			break;
	}
}

/*
 * void
 * print_bufhash_links(void)
 *      Debugging print of the buffer hash list.
 *      Can be invoked from kernel debugger.
 *
 * Calling/Exit State:
 *      No locks held on entry; No locks held on exit.
 */
void
print_bufhash_links(void)
{
	buf_t *bp;
	buf_t *bh;
	int i;
	
	for (i = 0; i < v.v_hbuf; i++) {
		bh = (buf_t *)&hbuf[i];
		for (bp = bh->b_forw; bp != bh; bp = bp->b_forw) {
			debug_printf("bp = %x, b_forw = %x, b_back = %x\n",
				     bp, bp->b_forw, bp->b_back);
			debug_printf("av_forw = %x, av_back = %x\n",
				     bp->av_forw, bp->av_back);
		}
		if (debug_output_aborted())
			return;
	}
}

/*
 * print_bfreelist_id(void)
 *	Debugging dump of buffer free list.
 *	Can be invoked from kernel debugger.	
 * Calling/Exit State:
 *      No locks held on entry; No locks held on exit.
 */
void
print_bfreelist_id(void)
{
	buf_t *bp;

	for (bp = bfreelist.av_forw; bp != &bfreelist; bp = bp->av_forw) {
		if (bp->b_flags & B_MARKER) {
			debug_printf("Marker buffer \n");
			continue;
		}

		debug_printf("bp = %x, dev = %x, blkno = %x, addr = 0x%x\n",
			     bp, bp->b_edev, bp->b_blkno, bp->b_un.b_addr);
		if (debug_output_aborted())
			return;
	}
}

/*
 * print_bufhash_id(void)
 *      Debugging dump of the buffer hash list.
 *      Can be invoked from kernel debugger.
 * Calling/Exit State:
 *      No locks held on entry; No locks held on exit.
 */
void
print_bufhash_id(void)
{
	buf_t *bp;
	buf_t *bh;
	int i;
	
	for (i = 0; i < v.v_hbuf; i++) {
		bh = (buf_t *)&hbuf[i];
		for (bp = bh->b_forw; bp != bh; bp = bp->b_forw) {
			debug_printf("bp = %x, dev = %x,"
				      " blkno = %x, addr = 0x%x\n",
				     bp, bp->b_edev, bp->b_blkno,
				     bp->b_un.b_addr);
			if (debug_output_aborted())
				return;
		}
	}
}

/*
 * static void
 * show_addrtype(uint_t addrtype)
 *	Print buffer address type
 *
 * Calling/Exit State:
 *	No locks held on entry; No locks held on exit.
 */
static void
show_addrtype(uint_t addrtype)
{
	switch (addrtype) {
	case BA_KVIRT:
		debug_printf("BA_KVIRT");
		break;
	case BA_UVIRT:
		debug_printf("BA_UVIRT");
		break;
	case BA_PAGELIST:
		debug_printf("BA_PAGELIST");
		break;
	case BA_PHYS:
		debug_printf("BA_PHYS");
		break;
	case BA_SCGTH:
		debug_printf("BA_SCGTH");
		break;
	default:
		debug_printf("0x%x", addrtype);
	}
}

/*
 * void
 * print_buf(const buf_t *bp)
 * 	Debugging print of buffer headers.
 * 	Can be invoked from kernel debugger.
 *
 * Calling/Exit State:
 *	No locks held on entry; No locks held on exit.
 */
void
print_buf(const buf_t *bp)
{
	debug_printf("bp 0x%x, %s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s%s\n", bp,
		(bp->b_flags & B_READ) ? " B_READ" : "", 
		(bp->b_flags & B_DONE) ? " B_DONE" : "", 
		(bp->b_flags & B_ERROR) ? " B_ERROR" : "", 
		(bp->b_flags & B_BUSY) ? " B_BUSY" : "", 
		(bp->b_flags & B_PHYS) ? " B_PHYS" : "", 
		(bp->b_flags & B_WASPHYS) ? " B_WASPHYS" : "", 
		(bp->b_flags & B_ASYNC) ? " B_ASYNC" : "", 
		(bp->b_flags & B_DELWRI) ? " B_DELWRI" : "", 
		(bp->b_flags & B_STALE) ? " B_STALE" : "", 
		(bp->b_flags & B_PAGEIO) ? " B_PAGEIO" : "", 
		(bp->b_flags & B_DELAYDONE) ? " B_DELAYDONE" : "", 
		(bp->b_flags & B_REMAPPED) ? " B_REMAPPED" : "", 
		(bp->b_flags & B_PRIVADDR) ? " B_PRIVADDR" : "", 
		(bp->b_flags & B_PAGEOUT) ? " B_PAGEOUT" : "", 
		(bp->b_flags & B_PARTIAL) ? " B_PARTIAL" : "", 
		(bp->b_flags & B_CACHE) ? " B_CACHE" : "", 
		(bp->b_flags & B_INVAL) ? " B_INVAL" : "",
		(bp->b_flags & B_PRIVBLK) ? " B_PRIVBLK" : "");
	debug_printf("forw 0x%x, back 0x%x, av_forw 0x%x, av_back 0x%x\n",
		     bp->b_forw, bp->b_back, bp->av_forw, bp->av_back);
	debug_printf("edev (%d,%d), bcount %d, bufsize %ld,"
		      " blkno %ld, blkoff %d, resid %d\n",
		     getemajor(bp->b_edev), geteminor(bp->b_edev),
		     bp->b_bcount, bp->b_bufsize, bp->b_blkno, bp->b_blkoff,
		     bp->b_resid);
	debug_printf("addrtype ");
	show_addrtype(bp->b_addrtype);
	if (bp->b_orig_type != 0) {
		debug_printf(", orig_type ");
		show_addrtype(bp->b_orig_type);
	}
	debug_printf(", childcnt %d\n", bp->b_childcnt);
	debug_printf("b_addr 0x%x, pages 0x%x, numpages %d, proc 0x%x\n",
		     bp->b_un.b_addr, bp->b_pages, bp->b_numpages, bp->b_proc);
	debug_printf("iodone 0x%x, misc 0x%x, priv 0x%x, priv2 0x%lx\n",
		     bp->b_iodone, bp->b_misc, bp->b_priv.un_int,
		     bp->b_priv2.un_long);
	debug_printf("start %ld, reltime %ld, error %d",
		     bp->b_start, bp->b_reltime, bp->b_error);
	if (bp->b_addrtype == BA_SCGTH)
		debug_printf(", scgth_count %d", bp->b_scgth_count);
	debug_printf("\n");
	debug_printf("b_iowait.ev_state %d, ", bp->b_iowait.ev_state);
	if (bp->b_avail.sl_avail)
		debug_printf("available\n");
	else
		debug_printf("lock held\n");
}

/*
 * void
 * print_bufpool(void)
 * 	Debugging print of buffer pool summary info.
 * 	Can be invoked from kernel debugger.
 *
 * Calling/Exit State:
 *	No locks held on entry; No locks held on exit.
 */
void
print_bufpool(void)
{
	buf_t *bp;

	debug_printf("av_f av_b forw back flag count bytes\n");
	bp = &bfreelist;
	debug_printf("Freelist(%x): %d headers available, %d bytes available\n",
		     bp, bp->b_bcount, bp->b_bufsize);
	debug_printf("av_f, av_b: (%x, %x);  forw, back: (%x, %x)\n",
		     bp->av_forw, bp->av_back, bp->b_forw, bp->b_back);
	bp = &bhdrlist;
	debug_printf("bhdrlist(%x): av_f: %x\n", bp, bp->av_forw);

}

#endif /* DEBUG || DEBUG_TOOLS */
