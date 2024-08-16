/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/lkstat.c	1.19"
#ident	"$Header: $"

/*
 * allocation and deallocation of lkstat_t's.
 */

#include <util/types.h>
#include <util/cmn_err.h>
#include <util/ksynch.h>
#include <util/ipl.h>
#include <util/debug.h>
#include <util/plocal.h>
#include <mem/kmem.h>
#include <svc/systm.h>

/*
 * If stat collection is enabled on all locks (spin and sleep), then
 * we need to configure many stat buffers up front, since the system
 * cannot allocate anything from the dynamic allocator until after
 * KMA is initialized. If stat collection is enabled only on sleep locks
 * then few stat blocks need to be provided initially.
 */



/*
 * It may be nicer to convert EXTRA_LSBS into a tunable, and set extra_lsbs 
 * to be the variable carrying that value. The tunable values for EXTRA_LSBS
 * should be picked on the basis of which of the following are
 * defined: DEBUG, SPINDEBUG, _MPSTATS.
 */


#ifdef	_MPSTATS
#if (defined DEBUG || defined SPINDEBUG)
#define	EXTRA_LSBS	400
#define	CHECK_FREE_CHAIN(x, y) \
	{if (do_lsbp_free_check) { check_free_chain((x), (y)); }}
int do_lsbp_free_check = 1;
STATIC	void	check_free_chain(lksblk_t *, lkstat_t *);
#else
#define	EXTRA_LSBS	5
#define	CHECK_FREE_CHAIN(x, y) 
#endif
#else
#define	CHECK_FREE_CHAIN(x, y) 
#define	EXTRA_LSBS	0
STATIC	lksblk_t *extra_lsbs_first = NULL;
STATIC	lksblk_t *extra_lsbs_last  = NULL;
#endif

#ifdef _MPSTATS
STATIC lksblk_t	lsbs_ext[EXTRA_LSBS];
STATIC	lksblk_t *extra_lsbs_first = &lsbs_ext[0];
STATIC	lksblk_t *extra_lsbs_last  = &lsbs_ext[EXTRA_LSBS -1];
#endif /* _MPSTATS */


STATIC int extra_lsbs = EXTRA_LSBS;	/* so we can peek at it from kdb */
STATIC void lksblk_init(lksblk_t *);
STATIC lksblk_t *block_alloc(lksblk_t *, int km_sleep, pl_t s);
STATIC lksblk_t lsb_first;	/* statically allocated to prevent problems
				 * with initializing the locks that protect
				 * this code
				 */
STATIC lksblk_t *lksblk_head = &lsb_first;	/* head of block list */
STATIC lock_t lksblk_mutex; 		/* protect list of lksblk's */

#if (defined DEBUG || defined SPINDEBUG)
STATIC lkinfo_t lksblk_info = { "KU:LKSTAT:lksblk_mutex", LK_BASIC };
#endif /* (DEBUG || SPINDEBUG) */

/*
 * void
 * lkstat_init(void)
 *	Called from startup code to initialize the first block of
 *	stat buffers.
 *
 * Calling/Exit State:
 *	Called from startup code.  Returns:  None.
 *
 * Remarks:
 *	This is to eliminate the inifinite recursion that results when
 *	this code tries to initialize its first lock.  Because we don't call
 *	kmem_alloc, we aren't concerned about whether or not it uses spin
 *	locks.  (If we did, there would be additional infinite recursion
 *	concerns.)
 */
void
lkstat_init(void)
{

#ifdef	_MPSTATS
	lksblk_t *lsbp, *lsbp_prev ;
	int i;
#endif

	lsb_first.lsb_prev = lsb_first.lsb_next = NULL;
	lksblk_init(& lsb_first);
	lksblk_mutex.sp_lock = SP_UNLOCKED;
#if (defined DEBUG || SPINDEBUG) && ! defined UNIPROC
	lksblk_mutex.sp_flags = 0;
#ifdef _LOCKTEST
	lksblk_mutex.sp_hier = 0;
	lksblk_mutex.sp_minipl = PL0;
#endif /* _LOCKTEST */
	lksblk_mutex.sp_lkstatp = NULL;
#endif /* (DEBUG || SPINDEBUG) && ! UNIPROC */

#if (defined DEBUG || SPINDEBUG)
	lksblk_mutex.sp_lkinfop = &lksblk_info;
#endif /* (DEBUG || SPINDEBUG) */

	/* also initialize all the extra blocks that are available, */
	/* and link them on the chain beginning at lsb_first.	    */

#ifdef _MPSTATS
	lsbp_prev = &lsb_first;

	for (i=0; i < extra_lsbs; i++) {
		lsbp = &lsbs_ext[i];
		lksblk_init(lsbp);
		lsbp->lsb_prev = lsbp_prev;
		lsbp->lsb_next = lsbp_prev->lsb_next;
		lsbp_prev->lsb_next = lsbp;
		lsbp_prev = lsbp;
	}
#endif
}

/*
 * lkstat_t
 * lkstat_alloc(lkinfo_t *lkinfop, int km_sleep)
 *	Allocates and initializes a statistics buffer
 *	from the pool.
 *
 * Calling/Exit State:
 *	km_sleep should be KM_SLEEP or KM_NOSLEEP.  Returns:  a pointer
 *	to the statistics buffer.  Returns a pointer to the newly-allocated
 *	lkstat_t, or NULL if no memory was available.
 *
 * Description:
 *	The lock lksblk_mutex is the only one acquired by this code, and
 *	it's held for the duration.
 *
 */
lkstat_t *
lkstat_alloc(lkinfo_t *lkinfop, int km_sleep)
{
	lksblk_t
		*lsbp,		/* to traverse the linked list */
		*lsbpmax,	/* pointer to fullest node */
		*lsbplast;	/* pointer to tail of list */
	lkstat_t *lkstatp;
	pl_t s;

	s = LOCK(& lksblk_mutex, PLHI);
	/* 
	 * find the block with the minimum number > 0 of free lkstat_t's.
	 */
	lsbp = lksblk_head;
	lsbpmax = NULL;

	lsbplast = NULL;

	while (lsbp != NULL) {

		ASSERT(lsbp->lsb_prev == lsbplast);

		if (lsbp->lsb_nfree > 0) {
			ASSERT(lsbp->lsb_free != NULL);
			ASSERT(lsbp->lsb_free >= (&(lsbp->lsb_bufs[0])));
			ASSERT(lsbp->lsb_free <= 
				(&(lsbp->lsb_bufs[LSB_NLKDS - 1])));
			if (NULL == lsbpmax)
				lsbpmax = lsbp;
			else if (lsbp->lsb_nfree < lsbpmax->lsb_nfree)
				lsbpmax = lsbp;
		} else {
			/* No free lkstat buffers in this block */
			ASSERT(lsbp->lsb_free == NULL);
		}
		lsbplast = lsbp;
		lsbp = lsbp->lsb_next;
	}

	if (lsbpmax == NULL) {
		lsbpmax = block_alloc(lsbplast, km_sleep, s);
		if (lsbpmax != NULL) {
			ASSERT((lsbpmax == lksblk_head) ||
				(lsbpmax->lsb_prev->lsb_next == lsbpmax));
			ASSERT((lsbpmax->lsb_next == NULL) || 
				(lsbpmax->lsb_next->lsb_prev == lsbpmax));
		}
	}

	/* if no memory could be allocated, return NULL */
	if (lsbpmax == NULL) {
		UNLOCK(& lksblk_mutex, s);
		return (NULL);
	}

	ASSERT(lsbpmax->lsb_free != NULL);
	ASSERT(lsbpmax->lsb_free >= (&(lsbpmax->lsb_bufs[0])));
	ASSERT(lsbpmax->lsb_free <= (&(lsbpmax->lsb_bufs[LSB_NLKDS - 1])));

	ASSERT(lsbpmax->lsb_nfree > 0);
	CHECK_FREE_CHAIN(lsbpmax, NULL);
	lkstatp = lsbpmax->lsb_free;
	lsbpmax->lsb_free = lkstatp->lks_next;
	--lsbpmax->lsb_nfree;


	ASSERT((lsbpmax->lsb_nfree == 0) || (lsbpmax->lsb_free != NULL));	
	ASSERT((lsbpmax->lsb_nfree != 0) || (lsbpmax->lsb_free == NULL));

	UNLOCK(&lksblk_mutex, s);

	/* zero the statistics fields */
	(void) bzero(lkstatp, sizeof(lkstat_t));

	lkstatp->lks_infop = lkinfop;
	return (lkstatp);
}

/*
 * lksblk_t *
 * block_alloc(lksblk_t *lsbp_tail, int km_sleep, pl_t s)
 *	Allocates and initializes a block of statistics buffers,
 *	appending it after the given block in the list.
 *
 * Calling/Exit State:
 *	`sleep' should be KM_SLEEP or KM_NOSLEEP, depending on whether
 *	we can sleep or not.  's' is the level to unlock the lksblk_mutex
 *	to, if necessary.  Should be called with lksblk_mutex held, returns
 *	with it held.  Returns: a pointer to the new block.
 */
STATIC lksblk_t *
block_alloc(lksblk_t *lsbp_tail,	/* the last in the list */
	int km_sleep,			/* allowed to sleep or not? */
	pl_t s)				/* the level to unlock to */
{
	lksblk_t *lsbp;
	extern boolean_t kma_initialized;

	/*
	 * first we try to get the memory without sleeping
	 */
	if (kma_initialized) {
		lsbp = (lksblk_t *)kmem_zalloc(sizeof (lksblk_t), KM_NOSLEEP);
		if (lsbp != NULL) {
			lsbp->lsb_next = NULL;
			lsbp->lsb_prev = lsbp_tail;
			ASSERT(lsbp_tail != NULL);
			ASSERT(lsbp_tail->lsb_next == NULL);
			lsbp_tail->lsb_next = lsbp;
			lksblk_init(lsbp);
			CHECK_FREE_CHAIN(lsbp, NULL);
			return (lsbp);
		}
	}
	if (km_sleep == KM_NOSLEEP || !kma_initialized) {
		static boolean_t warned = B_FALSE;

		if (!warned) {
			/*
			 *+ The EXTRA_LSBS tunable controls the amount of
			 *+ space statically allocated for _MPSTATS lock
			 *+ statistics structures, which are used before
			 *+ KMA has been initialized.  This space ran out.
			 *+ Correct by increasing the EXTRA_LSBS tunable.
			 */
			cmn_err(CE_WARN,
			  "EXTRA_LSBS is not big enough to handle space for\n"
			  "\tthe statistics structures for all early locks.\n"
			  "\tIncrease EXTRA_LSBS.");
			warned = B_TRUE;
		}
		return (NULL);
	}

	/*
	 * next we're going to try to get some memory by sleeping.
	 * since we may block in kmem_alloc, we need to drop the
	 * spin lock we're holding and then re-acquire it.  This
	 * means we may race with other processes doing the same
	 * allocation, or freeing lkstat_t's.
	 */
	UNLOCK(&lksblk_mutex, s);		/* open the window */
	lsbp = (lksblk_t *)kmem_zalloc(sizeof (lksblk_t), KM_SLEEP);
	ASSERT(lsbp != NULL);
	LOCK(&lksblk_mutex, PLHI);		/* close the window */

	/*
	 * Recompute lsbp_tail, since we dropped the lock and it may
	 * now be a different one.
	 */
	lsbp_tail = lksblk_head;

	ASSERT(lsbp_tail->lsb_prev == NULL);

	while (lsbp_tail->lsb_next != NULL) {
		ASSERT(lsbp_tail->lsb_next->lsb_prev == lsbp_tail);
		lsbp_tail = lsbp_tail->lsb_next;
	}

	lsbp->lsb_next = NULL;
	lsbp->lsb_prev = lsbp_tail;
	ASSERT(lsbp_tail->lsb_next == NULL);
	lsbp_tail->lsb_next = lsbp;
	lksblk_init(lsbp);
	CHECK_FREE_CHAIN(lsbp, NULL);
	return (lsbp);
}

/*
 * STATIC void
 * lksblk_init(lksblk_t *lsbp)
 * 	chain the buffers in a block together in a free list, and
 *	initialize the other fields.
 *
 * Calling/Exit State:
 *	lsbp is a pointer to the lksblk to init.  Returns:  None.
 */
STATIC void
lksblk_init(lksblk_t *lsbp)
{
	int i;

	lsbp->lsb_nfree = LSB_NLKDS;

	lsbp->lsb_free = & lsbp->lsb_bufs[0];
	for (i = 0; i < LSB_NLKDS - 1; ++i)
		lsbp->lsb_bufs[i].lks_next = & lsbp->lsb_bufs[i + 1];
	lsbp->lsb_bufs[LSB_NLKDS - 1].lks_next = NULL;
}


/*
 * void
 * lkstat_free(lkstat_t *lsp, boolean_t nullinfop)
 *	Return a statistics buffer to the pool.
 *
 * Calling/Exit State:
 *	lsp is the lkstat_t to free.  'nullinfop' tells whether the
 *	link to the lkinfo_t should be broken or not.  Returns:  None.
 */
void
lkstat_free(lkstat_t *lsp, boolean_t nullinfop)
{
	lksblk_t *lsbp;
	pl_t s;

	/*
	 * if we've been handed a NULL pointer for some reason, ignore
	 * it.
	 */
	if (lsp == NULL) {
		return;
	}
	
	if (nullinfop) {
		lsp->lks_infop = NULL;
	}

	s = LOCK(& lksblk_mutex, PLHI);

	/*
	 * traverse the list of lksblks, looking for the one containing
	 * the given lkstat_t.  
	 */
	for (lsbp = lksblk_head; lsbp != NULL; lsbp = lsbp->lsb_next) {

		if (lsp < (&(lsbp->lsb_bufs[0]))
		    || lsp > (&(lsbp->lsb_bufs[LSB_NLKDS - 1]))) {
			continue;
		}

		ASSERT((lsbp->lsb_nfree == 0) || (lsbp->lsb_free != NULL));	
		ASSERT((lsbp->lsb_nfree != 0) || (lsbp->lsb_free == NULL));

		if (lsbp->lsb_nfree != 0) {
			ASSERT(lsbp->lsb_free >= (&(lsbp->lsb_bufs[0])));
			ASSERT(lsbp->lsb_free <= 
				(&(lsbp->lsb_bufs[LSB_NLKDS - 1])));
		}
		
		CHECK_FREE_CHAIN(lsbp, lsp);	

		lsp->lks_next = lsbp->lsb_free;
		lsbp->lsb_free = lsp;
		lsbp->lsb_nfree++;


		ASSERT(lsbp->lsb_nfree <= LSB_NLKDS);
		/*
		 * free the containing block, if all it's stat buffers are
		 * free and it is not a statically allocated block, i.e.,
		 * it is not the "first" one, nor is it one of the extra
		 * blocks that are statically allocated in order to tide
		 * over the allocation problems before KMA could be initialized.
		 * XXX: this could be cleaned up a little bit, so that
		 * the first block is one of the so called extra blocks.
		 */
		if (lsbp->lsb_nfree == LSB_NLKDS && lsbp != lksblk_head &&
		    (lsbp < extra_lsbs_first || lsbp > extra_lsbs_last) ) {
			ASSERT(lsbp->lsb_prev != NULL);
			ASSERT(lsbp->lsb_prev->lsb_next == lsbp);
			ASSERT((lsbp->lsb_next == NULL) ||
				(lsbp->lsb_next->lsb_prev == lsbp));
			lsbp->lsb_prev->lsb_next = lsbp->lsb_next;
			if (lsbp->lsb_next != NULL) {
				lsbp->lsb_next->lsb_prev = lsbp->lsb_prev;
			}
			kmem_free(lsbp, sizeof(*lsbp));
		}
		UNLOCK(& lksblk_mutex, s);
		return;
	}
	/*
	 *+ The kernel tried to free a lkstat_t for which no enclosing
	 *+ lksblk_t could be found.  This indicates a kernel software
	 *+ problem.
	 */
	cmn_err(CE_PANIC, "couldn't find lksblk_t for lkstat_t");
}

/*
 * void
 * blk_statzero(lksblk_t *lsbp)
 *	Zero out all relevant statistical information in all of
 * 	the stat buffers of the indicated lock stats block.
 *	Do not munge lkinfop or the list pointers.
 * Calling/Exit State:
 *	lksblk_mutex is held by caller. This function neither
 *	drops nor acquires any locks.
 */
STATIC void
blk_statzero(lksblk_t *lsbp)
{
	lkstat_t *lsb_bufp;
	int i;

	ASSERT(LOCK_OWNED(&lksblk_mutex));	
	lsb_bufp = &lsbp->lsb_bufs[0];
	for (i=0; i < LSB_NLKDS; i++) {

		/* zero out the stats in lsbp->lsb_bufs[i] */
		lsb_bufp->lks_wrcnt = 0;
		lsb_bufp->lks_rdcnt = 0;
		lsb_bufp->lks_solordcnt = 0;
		lsb_bufp->lks_fail = 0;	
		lsb_bufp->lks_wtime.dl_lop = 0;
		lsb_bufp->lks_wtime.dl_hop = 0;
		lsb_bufp->lks_htime.dl_lop = 0;
		lsb_bufp->lks_htime.dl_hop = 0;

		++lsb_bufp;
	}
}


/*
 * void
 * blk_statzero_all(void)
 *	Zero out all relevant statistical information in all of
 * 	the stat buffers of all existing lock stats blocks.
 *	Do not munge lkinfop or the list pointers.
 * Calling/Exit State:
 *	lksblk_mutex is acquired and held over the duration of the
 *	function. It is released at the end.
 */
void
blk_statzero_all(void)
{
	pl_t ospl;
	lksblk_t *lsbp = lksblk_head;
	lksblk_t *prev_lsbp = NULL; 


	ospl = LOCK(&lksblk_mutex, PLHI);

	while (lsbp != NULL) {
		ASSERT(lsbp->lsb_prev == prev_lsbp);
		blk_statzero(lsbp);
		prev_lsbp = lsbp;
		lsbp = lsbp->lsb_next;
	}

	UNLOCK(&lksblk_mutex, ospl);

}


#ifdef	_MPSTATS
#if (defined DEBUG || defined SPINDEBUG)
/*
 * STATIC void
 * check_free_chain(lksblk_t *lsbp, lkstat *lksp)
 *
 *	Verify that the chain of free lock stat buffers in a given
 *	buffer block (pointed to by lsbp) is sane, and that an indicated
 *	lock stat buffer (pointed to by lksp) is not on the free chain.
 *
 * Calling/Exit State:
 *	The lksblk_mutex is held by the caller. The function does not
 *	block. It is assumed further that the free chain has at least one
 *	item on it.
 *
 * Description:
 *	Following sanity checks are done: (1) all buffers on the free
 *	chain are contained in the buffer block (2) the count of free
 *	buffers agrees with the length of the free chain.
 */
STATIC void
check_free_chain(lksblk_t *lsbp, lkstat_t *lksp)
{
	int free_count;
	lkstat_t *lkstatp;
	lkstat_t *lkstat_first; 
	lkstat_t *lkstat_last;

	lkstat_first = (&(lsbp->lsb_bufs[0]));
	lkstat_last = (&(lsbp->lsb_bufs[LSB_NLKDS - 1]));
	lkstatp = lsbp->lsb_free;	
	free_count = lsbp->lsb_nfree;

	if (lksp != NULL) {
		ASSERT((lksp >= lkstat_first) && (lksp <= lkstat_last));
	} 

	ASSERT(free_count <= LSB_NLKDS);

	while (lkstatp != NULL) {
		if (lkstatp == lksp) {
			cmn_err(CE_CONT,
				"Freeing freed lkstatp 0x%x in lsbp 0x%x\n", 
				lksp);
			call_demon();
		} else {
			ASSERT(lkstatp >= lkstat_first);
			ASSERT(lkstatp <= lkstat_last);
			ASSERT(free_count > 0);
			--free_count;
			lkstatp = lkstatp->lks_next;
		}
	}

	ASSERT(free_count == 0);
}
#endif
#endif
