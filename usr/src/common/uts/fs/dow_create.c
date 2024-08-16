/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_create.c	1.4"
#ident	"$Header: $"

#include	<fs/dow.h>
#include	<fs/fs_hier.h>
#include	<io/conf.h>
#include	<mem/kmem.h>
#include	<mem/page.h>
#include	<svc/clock.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/ghier.h>
#include	<util/sysmacros_f.h>
#include	<util/var.h>

/*
 * dowid_t
 * dow_create_buf(dev_t dev, daddr_t blkno, long bsize, uint_t flag)
 *	Lookup the specified <dev, blkno> to find an associated dow,
 *	if one exists, and return it held. If a dow does not exist,
 *	create one. Return with a hold on the dow. flag specifies
 *	whether blocking is permitted for dow structures.
 *
 * Calling/Exit State:
 *	Caller does not hold dow_mutex. Even if the flag specifies no wait,
 *	the caller is not expected to call this function on an interrupt
 *	path or while holding other locks; while this is not a requirement
 *	for this functionality, it is implied in the current implementation
 *	(because DOW_MUTEX_LOCK() and DOW_MUTEX_UNLOCK() assume that the
 *	caller was at basepl.
 *
 * Remarks:
 *
 * PERF:
 *	We could acquire the dow_mutex inside the function and release
 *	it before returning. It is a performance issue whether we can
 *	hope to cover several calls to dow_interface with one lock
 *	round trip.
 */

#ifdef	DEBUG
	/* Remove after funcs are tested */
short	func_creats = 0;
#endif

dowid_t
dow_create_buf(dev_t dev, daddr_t blkno, long bsize, uint_t flag)
{
	dowid_t id;
	dowid_t hashbucket;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	DOW_MUTEX_LOCK();

	while ((id = dow_lookup(dev, blkno)) == (dowid_t)DOW_BADID) {

		if ((id = dow_alloc_l()) == DOW_BADID) {
			if (flag & (DOW_NO_WAIT | DOW_NO_RESWAIT)) {
				DOW_MUTEX_UNLOCK();
				return (DOW_BADID);
			} else {
				DOW_FREE_SV_WAIT();
				DOW_MUTEX_LOCK();
				continue;
			}
		}

		ASSERT(VALID_DOWID(id));
		dow_init(id, (long)dev, (long)blkno, DOW_ISBUFFER);
		DOW_CHEAP_HOLD(id);
		DOW_BSIZE(id) = bsize;
		DOW_BP(id) = NULL;


		hashbucket = DOW_HASHBUCKET(dev, blkno);
		dow_inshash(id, hashbucket);
		dow_flush_tailins(id, DOW_AGED_FLUSHHEAD);

		CHECK_HASHDUP(id, hashbucket, dev, blkno);
		DOW_MUTEX_UNLOCK();
		return(id);
	}

	/* 
	 * Successful lookup. There is no need to wait for INTRANS or 
	 * MOD states to clear. Note that dow_lookup() created a hold on 
	 * id before returning it to us.
	 */
#ifdef	DEBUG
	ASSERT(VALID_DOWID(id));
	ASSERT(DOW_TYPE(id) == DOW_ISBUFFER);
	ASSERT(DOW_BSIZE(id) == bsize);
	hashbucket = DOW_HASHBUCKET(dev, blkno);
	CHECK_HASHDUP(id, hashbucket, dev, blkno);
#endif
	DOW_MUTEX_UNLOCK();
	return(id);
}
/*
 * dowid_t
 * dow_create_function(void (*func)(), void *argp, uint_t flag,
 *		boolean_t func_can_block)
 *
 *	Lookup the specified <func, argp> to find an associated dow;
 *	if one exists, and return it held. If a dow does not exist,
 *	create one. Return with a hold on the dow. flag specifies
 *	whether blocking is permitted for dow structures.
 *
 *	func_can_block: B_FALSE if it is known that the function is
 *	a non-blocking function in all invocations; B_TRUE otherwise.
 *
 * Calling/Exit State:
 *	Caller does not hold dow_mutex. It may be acquired, and released 
 *	and reacquired if allocation is a blocking allocation. The mutex
 *	will be dropped before return.
 *
 *	Eventhough the caller may specify non-blocking allocation, it will
 *	be assumed that the caller is at plbase. (convenience).
 *
 * Remarks:
 *
 * PERF:
 *	We could acquire the dow_mutex inside the function and release
 *	it before returning. It is a performance issue whether we can
 *	hope to cover several calls to dow_interface with one lock
 *	round trip.
 */

dowid_t
dow_create_func(void (*func)(), void *argp, uint_t flag, 
	boolean_t func_can_block)
{
	dowid_t id;
	dowid_t hashbucket;

	DOW_MUTEX_LOCK();

	while ((id = dow_lookup((long)func, (long)argp)) == (dowid_t)DOW_BADID){

		if ((id = dow_alloc_l()) == DOW_BADID) {
			if (flag & (DOW_NO_WAIT | DOW_NO_RESWAIT)) {
				DOW_MUTEX_UNLOCK();
				return (DOW_BADID);
			} else {
				DOW_FREE_SV_WAIT();
				DOW_MUTEX_LOCK();
				continue;
			}
		}

		ASSERT(VALID_DOWID(id));
		dow_init(id, (long)func, (long)argp, 
			(func_can_block ? DOW_ISFUNC : DOW_ISFUNC_NBLK));
		DOW_CHEAP_HOLD(id);
		DOW_BP(id) = NULL;

		hashbucket = DOW_HASHBUCKET((long)func, (long)argp);
		dow_inshash(id, hashbucket);
		dow_flush_tailins(id, DOW_AGED_FLUSHHEAD);
		CHECK_HASHDUP(id, hashbucket, (long)func, (long)argp);
		DOW_MUTEX_UNLOCK();
#ifdef	DEBUG
		++func_creats;
#endif
		return(id);
	}

	/* 
	 * Successful lookup. There is no need to wait for INTRANS or 
	 * MOD states to clear. Note that dow_lookup() created a hold on 
	 * id before returning it to us.
	 */
#ifdef	DEBUG
	ASSERT(VALID_DOWID(id));
	ASSERT(DOW_TYPE(id) & DOW_ISFUNC);
	hashbucket = DOW_HASHBUCKET((long)func, (long)argp);
	CHECK_HASHDUP(id, hashbucket, (long)func, (long)argp);
#endif
	DOW_MUTEX_UNLOCK();
	return(id);
}
/*
TODO:
accept a len arguement
return badid if off+len crosses pageboundary
match offset to offset & PAGEMASK

*/

/*
 * dowid_t
 * dow_create_page(vnode_t *vp, off_t offset, int len, uint_t flag)
 *	Lookup the specified <vp, offset> to find an associated dow,
 *	if one exists, and return it held. If a dow does not exist,
 *	create one. Return with a hold on the dow. flag specifies
 *	whether:
 *	(1) blocking is permitted for dow structures and
 *	(2) dow_create is being called from dow_strategy in which case, 
 *	    (a)	blocking is not permitted, and 
 *	    (b)	if dow resource is not available, other dow_create() calls 
 *		must be disabled (failed or blocked) in order to disable 
 *		the DOW optimization until the count of un-tracked page 
 *		pushes drops to 0. (This should be extremely improbable!)
 *
 * Calling/Exit State:
 *	Caller holds dow_mutex. It may be released and reacquired if
 *	allocation is a blocking allocation.
 *
 * Remarks:
 *
 * PERF:
 *	We could acquire the dow_mutex inside the function and release
 *	it before returning. It is a performance issue whether we can
 *	hope to cover several calls to dow_interface with one lock
 *	round trip.
 */
dowid_t
dow_create_page(vnode_t *vp, off_t offset, int len, uint_t flag)
{
	dowid_t id;

	DOW_MUTEX_LOCK();
	id = dow_create_page_l(vp, offset, len, flag);
	DOW_MUTEX_UNLOCK();
	return(id);	
}

/*
 * dowid_t
 * dow_create_page_l(vnode_t *vp, off_t offset, int len, uint_t flag)
 *	Internal interface to do the actual creation of a dow corresponding
 *	to the specified identity. Does the real work of dow_create_page.
 *
 * Calling/Exit State:
 *	dow_mutex held by the caller. The dow_mutex is held on return.
 *	It may be dropped and reacquired if flag specifies waiting is
 *	tolerable.
 */

dowid_t
dow_create_page_l(vnode_t *vp, off_t offset, int len, uint_t flag)
{
	dowid_t id;
	dowid_t hashbucket;

	ASSERT(DOW_MUTEX_OWNED());

	while ((id = dow_lookup_page(vp, offset)) == (dowid_t)DOW_BADID){

		if (untracked_pageio > 0) {
			return (DOW_BADID);
		}

		if ((id = dow_alloc_l()) == DOW_BADID) {
			if (flag & (DOW_NO_WAIT | DOW_NO_RESWAIT)) {
				return (DOW_BADID);
			} else {
				DOW_FREE_SV_WAIT();
				DOW_MUTEX_LOCK();
				continue;
			}
		}

		ASSERT(VALID_DOWID(id));
		dow_init(id, (long)vp, (long)offset, DOW_ISPAGE);
		VN_SOFTHOLD(vp);
		DOW_CHEAP_HOLD(id);
		DOW_LEN(id) = len;
		/*
		 * XXX:
		 *	OTHER dow INITIALIZATIONS FOR PAGE TYPE?
		 */

		hashbucket = DOW_HASHBUCKET((long)vp, (long)offset);
		dow_inshash(id, hashbucket);
		dow_flush_tailins(id, DOW_AGED_FLUSHHEAD);
		/*
		 * Verify (in DEBUG mode) that there is no synonym
		 */
		CHECK_HASHDUP(id, hashbucket, (long)vp, (long)offset);
		return(id);
	}
#ifdef	DEBUG
	hashbucket = DOW_HASHBUCKET((long)vp, (long)offset);
	/* 
	 * Successful lookup. There is no need to wait for INTRANS or 
	 * MOD states to clear. Note that dow_lookup() created a hold on 
	 * id before returning it to us.
	 */
	ASSERT(VALID_DOWID(id));
	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);
	CHECK_HASHDUP(id, hashbucket, (long)vp, (long)offset);
#endif
	return(id);
}
