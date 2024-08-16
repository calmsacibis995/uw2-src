/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_util.c	1.11"
#ident	"$Header: $"

#include	<fs/dow.h>
#include	<fs/fs_hier.h>
#include	<io/conf.h>
#include	<mem/kmem.h>
#include	<mem/page.h>
#include	<proc/lwp.h>
#include	<proc/proc.h>
#include	<proc/user.h>
#include	<svc/clock.h>
#include	<util/cmn_err.h>
#include	<util/debug.h>
#include	<util/ghier.h>
#include	<util/inline.h>
#include	<util/sysmacros_f.h>
#include	<util/var.h>

#define	SWAP_DISABLE()	{						\
			ASSERT(u.u_lwpp != NULL);			\
			(++(u.u_lwpp->l_keepcnt));			\
			}

#define	SWAP_ENABLE()	{						\
			ASSERT(u.u_lwpp != NULL);			\
			ASSERT((u.u_lwpp->l_keepcnt) > 0);		\
			(--(u.u_lwpp->l_keepcnt));			\
			}

dow_t		dow_tab[DOW_ARRAYSIZE];
int		dow_freecount;
int		dowlink_freecount;
int		untracked_pageio;
dow_io_utp_t	dow_io_utp_tab[MAX_UNTRACKED_PAGES];
sv_t		dow_io_utp_sv; /* to control dow_io_utp_t alloc */
lock_t		dow_mutex;
sv_t		dow_free_sv;
sv_t		dowlink_free_sv;
event_t		dow_flush_event;

LKINFO_DECL(dow_mutex_lkinfo, "FS:dow_mutex spin lock global", 0);

/*
 * void
 * dow_io_utp_setup(buf_t *bp)
 *	Setup the biodone processing chain for executing the untracked
 *	pageio protocol. Basically, we need to adjust the untracked
 *	pageio counter for each pageio that completes, where such pages
 *	would have normally been associated with dow structures but were
 *	not, due to dow resource exhaustion.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, and may need to be dropped and reacquired
 *	if the system exhausts the frames for tracking untracked pageio.
 */
void
dow_io_utp_setup(buf_t *bp)
{
	int i;
	dow_io_utp_t *dow_io_utpp;

	ASSERT(DOW_MUTEX_OWNED());
	for(;;) {
		for (i=0; i < MAX_UNTRACKED_PAGES; i++) {
			dow_io_utpp = &(dow_io_utp_tab[i]); 
			if (dow_io_utpp->diu_bp == NULL) {
				dow_io_utpp->diu_bp = bp;
				dow_io_utpp->diu_func = bp->b_iodone;
				dow_io_utpp->diu_chain = bp->b_misc;
				bp->b_iodone = dow_untracked_pageio_done;
				bp->b_misc = dow_io_utpp;
				return;
			}
		}
		DOW_IO_UTP_SV_WAIT();
		DOW_MUTEX_LOCK();
	}
}

/*
 * void
 * dow_io_utp_rele(buf_t *bp)
 *	Release the untracked pageio frame acquired in dow_io_utp_setup, and
 *	signal any blocked waiters.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, who is at interrupt level.
 */
void
dow_io_utp_rele(buf_t *bp)
{
	dow_io_utp_t *dow_io_utpp;

	ASSERT(DOW_MUTEX_OWNED());
	dow_io_utpp = bp->b_misc;
	bp->b_misc = dow_io_utpp->diu_chain;
	bp->b_iodone = dow_io_utpp->diu_func;
	dow_io_utpp->diu_bp = NULL;
	dow_io_utpp->diu_func = NULL;
	dow_io_utpp->diu_chain = NULL;
	if (DOW_IO_UTP_SV_BLKD()) {
		DOW_IO_UTP_SV_BROADCAST();
	}
}

/*
 * void
 * dow_arrayinit(void)
 *	Initialize all dow structures (including hash buckets and
 *	chain anchors for flushchains and freelist).
 *
 * Calling/Exit State:
 *	None. This is a system initialization routine.
 *
 * Remarks:
 * 	Must call dowlink_arrayinit before calling dow_arrayinit. Else
 * 	assertions in dow_init (for the dependent and antecdent 
 *	linkheads) would trip.
 */
void
dow_arrayinit(void)
{
	dowid_t i;

	dowlink_arrayinit();
	CHECK_DOWLINK_FREELIST();

	DOW_FREE_SV_INIT();
	DOW_IO_UTP_SV_INIT();
	DOW_MUTEX_INIT();
	DOW_FLUSH_EVENT_INIT();

	for (i = 0; i < DOW_ARRAYSIZE; i++) {
		dow_init(i, 0, NULL, DOW_ISFUNC_NBLK); 
	}
	dow_freecount = 0;
	untracked_pageio = 0;
	for (i = 0; i < DOW_TABLE_SIZE; i++) {
		dow_free(i);
	}

	for (i = 0; i < MAX_UNTRACKED_PAGES; i++) {
		dow_io_utp_tab[i].diu_bp = NULL;
		dow_io_utp_tab[i].diu_func = NULL;
		dow_io_utp_tab[i].diu_chain = NULL;
	}

	ASSERT(dow_freecount == DOW_TABLE_SIZE);

#ifdef	DEBUG
	for (i = 0; i < DOW_TABLE_SIZE; i++) {
		ASSERT(DOW_DEBUG_FLUSHID(i) == DOW_FREEHEAD);
		ASSERT(EMPTY_DOWLINK_LIST(DOWLINK_ANT_LINKHEAD(i)));
		ASSERT(EMPTY_DOWLINK_LIST(DOWLINK_DEP_LINKHEAD(i)));
		CHECK_DOWLINK_CHAIN(DOWLINK_DEP_LINKHEAD(i));
		CHECK_DOWLINK_CHAIN(DOWLINK_ANT_LINKHEAD(i));
	}

	for (i = DOW_TABLE_SIZE; i < (DOW_TABLE_SIZE + DOW_HASHWIDTH); i++) {
		ASSERT(DOW_DEBUG_CNT(i) == 0);
		ASSERT(EMPTY_DOW_HASHLIST(i));
	}

	for (i = (DOW_TABLE_SIZE + DOW_HASHWIDTH); 
	     i <  (DOW_TABLE_SIZE + DOW_HASHWIDTH + 3); i++) {
		ASSERT(DOW_DEBUG_CNT(i) == 0);
		ASSERT(EMPTY_DOW_FLUSHLIST(i));
	}

#endif
	CHECK_DOW_FREELIST();
}

/*
 * void
 * dow_init(dowid_t x, long ident1, long ident2, uchar_t type)
 *	Initialize a dow structure. ident1 and ident2 provide the
 *	identity; type specifies whether this is to be a buffer,
 *	page, or function dow.
 *
 * Calling/Exit State: 
 *	Normally the dow_mutex should be held by caller, except when
 *	called during system initialization. It is the caller's
 *	responsibility to insert the dow structure on the correct
 *	hash chain.
 *
 * Remarks:
 *	dow_init() should only be used as an internal interface. It
 *	does not require that a dow_hold be placed on x, nor does it
 *	place the dow_hold. That is left to the caller(s). The validity
 *	of x is guaranteed by the dow_mutex held by caller.
 */
void
dow_init(dowid_t x, long ident1, long ident2, uchar_t type)
{
	DOW_FREENEXT(x)	= x;
	DOW_FREEPREV(x) = x;
	DOW_FLUSHNEXT(x) = x;
	DOW_FLUSHPREV(x) = x;
	DOW_HOLD(x) = 0;
	DOW_LEVEL(x) = 0;
	DOW_DEP_CNT(x) = 0;
	DOW_ANT_CNT(x) = 0;
	DOW_TIMESTAMP(x) = INT_MAX;
	DOW_SV_INIT(x);
	DOW_STATE(x) = 0;
	DOW_TYPE(x) = type;
	if (type == DOW_ISBUFFER) {
		DOW_DEV(x) = (dev_t)ident1;
		DOW_BLKNO(x) = (int)ident2;
		DOW_BSIZE(x) = 0;
		DOW_MOD_LOCK(x) = 0;

	} else if (type == DOW_ISPAGE) {
		DOW_VP(x) = (vnode_t *)ident1;
		DOW_OFFSET(x) = (off_t)ident2;
		DOW_MOD_LOCK(x) = 0;

	} else {
		DOW_FUNC(x) = (void (*)())ident1;
		DOW_ARGP(x) = (void *)ident2;
		DOW_MOD_LOCK(x) = 0;
	}
	DOW_BP(x) = NULL;
	DOW_DEBUG_IODONE_INIT(x);	
	/* 
	 * This assertion would trip unless we called
	 * dowlink_array init before dow_arrayinit was called,
	 * during initialization. We expect the assertion
	 * to hold subsequently, when dow_t's are recycled.
	 */
#ifdef	DEBUG
	if (x < DOW_TABLE_SIZE) {
	ASSERT(EMPTY_DOWLINK_LIST(DOWLINK_ANT_LINKHEAD(x)));
	ASSERT(EMPTY_DOWLINK_LIST(DOWLINK_DEP_LINKHEAD(x)));
	ASSERT(DOWLINK_LINKS_SANE(DOWLINK_ANT_LINKHEAD(x)));
	ASSERT(DOWLINK_LINKS_SANE(DOWLINK_DEP_LINKHEAD(x)));
	}
#endif

	DOW_DEBUG_INIT(x);
}

#ifdef	DEBUG

/*
 * void
 * dow_free_badinit(dowid_t id)
 *	Fill up a freed dow structure with creative material 
 *	for enhanced entomological purposes.
 *
 * Calling/Exit State:
 *	dow mutex held by caller.
 */

void
dow_free_badinit(dowid_t id)
{
	DOW_FLUSHNEXT(id) = DOW_BADID;
	DOW_FLUSHPREV(id) = DOW_BADID;
	DOW_HOLD(id) = (-1799);
	DOW_LEVEL(id) = 170;
	DOW_DEP_CNT(id) = 187;
	DOW_ANT_CNT(id) = 204;
	DOW_TIMESTAMP(id) = INT_MAX;
	DOW_STATE(id) = 255; 
	DOW_TYPE(id) = 255;
	DOW_DEV(id) = 1600085856;
	DOW_BLKNO(id) = 168430090;
	DOW_BP(id) = (buf_t *)1515870810;
	DOW_BSIZE(id) = 1600085856;
	DOW_MOD_LOCK(id) = (-6475);
	DOW_B_IOCHAIN(id) = (void *)(0xffffffff);
}	 

/*
 * void
 * dow_debug_init(dowid_t id)
 *	Initialize debugging information for a dow structure.
 *
 * Calling/Exit State:
 *	dow mutex held by caller.
 */

void
dow_debug_init(dowid_t id)
{
	DOW_DEBUG_HASHID(id) = DOW_BADID;
	DOW_DEBUG_FLUSHID(id) = DOW_BADID;
	DOW_DEBUG_CNT(id) = 0;
}
#endif

/*
 * void
 * dow_inshash(dowid_t id, dowid_t hbucket)
 *	Insert dow "id" on a hash chain anchored at dow table index 
 *	of hbucket
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, and held on return. The function
 *	does not block or drop the lock. 
 *
 * PERF:
 *	Convert to a macro. Preferrably assembler macro.
 *	Collect stats on how good a hash function we really have.
 */
void
dow_inshash(dowid_t id, dowid_t hbucket)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(id));
	ASSERT(VALID_HASH_BUCKET(hbucket));
	DOW_HASHNEXT(id) = DOW_HASHNEXT(hbucket);
	DOW_HASHPREV(id) = hbucket;
	DOW_HASHPREV(DOW_HASHNEXT(id)) = id;
	DOW_HASHNEXT(hbucket) = id;
	DOW_DEBUG_HASHENTER(id, hbucket);
}
/*
 * void
 * dow_remhash(dowid_t id, dowid_t hbucket)
 *	Remove dow "id" from its hash chain.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, and held on return. The function
 *	does not block or drop the lock. 
 *
 * PERF:
 *	Convert to a macro. Preferrably assembler macro.
 */
void
dow_remhash(dowid_t id)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(VALID_DOWID(id));
	CHECK_HASH(id);
	DOW_HASHNEXT(DOW_HASHPREV(id)) = DOW_HASHNEXT(id);
	DOW_HASHPREV(DOW_HASHNEXT(id)) = DOW_HASHPREV(id);
	DOW_DEBUG_HASHLEAVE(id);
	DOW_HASHINVAL(id);
}
/*
 * dowid_t
 * dow_alloc(void)
 *	Allocate a dow structure from the freelist. 
 *
 * Calling/Exit State:
 *	The dow_mutex is acquired and released within. The function does
 *	not block. 
 *
 * Remarks:
 *	If later we implement dow_reservations, then a flag could be
 *	used to specify reservation. All the real work is done by
 *	dow_alloc_l.
 *
 * PERF: both dow_alloc and dow_alloc_l are good candidates for inlining.
 */

dowid_t
dow_alloc(void)
{
	dowid_t id;

	DOW_MUTEX_LOCK();
	id = dow_alloc_l();
	DOW_MUTEX_UNLOCK();

	return(id);
}

/*
 * dowid_t
 * dow_alloc_l(void)
 *	Allocate a dow structure from the freelist. 
 *
 * Calling/Exit State:
 *	The dow_mutex is held at entry and exit, and is not reacquired. 
 *	The function does not block.
 *
 * Remarks:
 *	If later we implement dow_reservations, then a flag could be
 *	used to specify reservation. Once dow structures are reserved,
 *	the caller can expect successful yet non-blocking return.
 */
dowid_t
dow_alloc_l(void)
{
	dowid_t freefirst;

	ASSERT(DOW_MUTEX_OWNED());
	if (dow_freecount == 0) {
		ASSERT(EMPTY_DOW_FREELIST(DOW_FREEHEAD));
		return ((dowid_t)DOW_BADID);
	}
	ASSERT(!EMPTY_DOW_FREELIST(DOW_FREEHEAD));
	--(dow_freecount);
	freefirst = DOW_FREENEXT(DOW_FREEHEAD);
	DOW_FREENEXT(DOW_FREEHEAD) = DOW_FREENEXT(freefirst);

#ifdef	DEBUG 
	/*
	 * in DEBUG mode, we maintain the freelist doubly linked.
	 */
	ASSERT(DOW_FREEPREV(DOW_FREENEXT(freefirst)) == freefirst);
	DOW_FREEPREV(DOW_FREENEXT(freefirst)) = DOW_FREEHEAD;
#endif 

	DOW_DEBUG_FREELEAVE(freefirst);
	return(freefirst);
}

/*
 * void
 * dow_free(dowid_t x)
 *	Insert dowid "x" at the head of the dow free list. Bump
 *	up the dow_freecount.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller, returned held. Non-blocking function.
 *
 * Remarks:
 *	Caller has the responsibility of dequeuing x from its hash and
 *	flush chains.
 *
 * PERF:
 *	Convert to a macro.
 */
void 
dow_free(dowid_t x)
{
	ASSERT(DOW_HOLD(x) == 0);
	ASSERT(DOW_DEP_CNT(x) == 0);
	ASSERT(DOW_ANT_CNT(x) == 0);
	ASSERT(DOW_MOD_LOCK(x) == 0);
	ASSERT(!DOW_SV_BLKD(x));

	DOW_FREENEXT(x) = DOW_FREENEXT(DOW_FREEHEAD);

#ifdef	DEBUG 
	DOW_FREEPREV(x) = DOW_FREEHEAD;
	DOW_FREEPREV(DOW_FREENEXT(x)) = x;
#endif 

	DOW_FREENEXT(DOW_FREEHEAD) = x;
	++dow_freecount;
	if (DOW_FREE_SV_BLKD()) {
		DOW_FREE_SV_BROADCAST();
	}
	DOW_FREE_BADINIT(x);
	DOW_DEBUG_FREEENTER(x);
}

/*
 * dowid_t
 * dow_lookup(long ident1, long ident2)
 *	Search for a dow corresponding to the specified
 *	identity tuple, <ident1, ident2>, which may identify
 *	a buffer or a function dow. If one is found, return with 
 * 	a hold on the dowid. If none is found, return an invalid 
 *	dowid (i.e., DOW_BADID).
 *
 * Calling/Exit State:
 *	Called and returns with dow_mutex held.
 *
 * PERF:
 *	May be converted to a macro? We should put in several debug
 *	checks for now, and convert it to a macro with these checks
 *	removed, after the code is shown to hold up well.
 */

dowid_t
dow_lookup(long ident1, long ident2)
{
	dowid_t hash_bucketid;
	dowid_t i;

	ASSERT(DOW_MUTEX_OWNED());

	hash_bucketid = DOW_HASHBUCKET(ident1, ident2);

	ASSERT(DOW_HASHLINKS_SANE(hash_bucketid));
	ASSERT(VALID_HASH_BUCKET(hash_bucketid));

	i = DOW_HASHNEXT(hash_bucketid);
	while (i != hash_bucketid) {

		ASSERT(VALID_DOWID(i));
		ASSERT(DOW_DEBUG_HASHID(i) == hash_bucketid);
		ASSERT(DOW_HASHLINKS_SANE(i));
		ASSERT((DOW_STATE(i) & DOW_GONE) == 0);
		if ((DOW_IDENT1(i) == ident1) && (DOW_IDENT2(i) == ident2)) {
			DOW_CHEAP_HOLD(i);
			return(i);
		}
		i = DOW_HASHNEXT(i);
	}; 
	return(DOW_BADID);
}

/*
 * dowid_t
 * dow_lookup_page(vnode_t *vp, off_t offset)
 *	Search for a page dow corresponding to the specified
 *	identity <vp, offset>. If one is found, return with 
 * 	a hold on the dowid. If none is found, return an invalid 
 *	dowid (i.e., DOW_BADID).
 *
 * Calling/Exit State:
 *	Called and returns with dow_mutex held.
 *
 * PERF:
 *	May be converted to a macro? We should put in several debug
 *	checks for now, and convert it to a macro with these checks
 *	removed, after the code is shown to hold up well.
 */

dowid_t
dow_lookup_page(vnode_t *vp, off_t offset)
{
	dowid_t hash_bucketid;
	dowid_t i;

	ASSERT(DOW_MUTEX_OWNED());

	hash_bucketid = DOW_HASHBUCKET((long)vp, offset);

	ASSERT(DOW_HASHLINKS_SANE(hash_bucketid));
	ASSERT(VALID_HASH_BUCKET(hash_bucketid));

	i = DOW_HASHNEXT(hash_bucketid);
	while (i != hash_bucketid) {

		ASSERT(VALID_DOWID(i));
		ASSERT(DOW_DEBUG_HASHID(i) == hash_bucketid);
		ASSERT(DOW_HASHLINKS_SANE(i));
		ASSERT((DOW_STATE(i) & DOW_GONE) == 0);
		if ((DOW_IDENT1(i) == (long)vp) && 
		    (DOW_IDENT2(i) == offset)) {
			DOW_CHEAP_HOLD(i);
			return(i);
		}
		i = DOW_HASHNEXT(i);
	}; 
	return(DOW_BADID);
}

/*
 * void
 * dow_startmod(dowid_t id)
 *	Begin modification of the data associated with the specified dow.
 *
 * Calling/Exit State:
 *	None. The caller should be prepeared to block.
 *
 * Description:
 *	Inhibit swapping, and if the dow is a page dow, acquire the
 *	modification interlock.
 *
 * Remarks:
 *	Swapping is inhibited as a deadlock avoidance measure, since
 *	while modifying the data the lwp is either holding a buffer,
 *	if the dow corresponds to a buffer, or it's holding the dow
 *	modification interlock, if the dow corresponds to a page.
 */
void
dow_startmod(dowid_t id)
{
	if (!VALID_DOWID(id))
		return;

	SWAP_DISABLE();
	ASSERT(DOW_TYPE(id) == DOW_ISPAGE || DOW_TYPE(id) == DOW_ISBUFFER);
	if (DOW_TYPE(id) == DOW_ISBUFFER)
		return;
	dow_startmod_rdlock(id);
}

/*
 * void
 * dow_startmod_rdlock(dowid_t id)
 *	Acquire the modification interlock on a dow structure, in read
 *	mode. This is a pseudo reader-writer lock, whose intent is
 *	to ensure that if the dow structure carries the DOW_MODIFIED
 *	state, then the real page is modified as well.
 * 	Called only for page dows. Not needed for buffer dows, because
 *	buffers are uniquely owned, and writes on buffers always complete.
 *	Read mode lockers are those that intend to set the MOD state.
 *
 * Calling/Exit State:
 *	None. The caller should be prepeared to block.
 */
void
dow_startmod_rdlock(dowid_t id)
{
	dowid_t hbucket;
	long ident1, ident2;

	if (!VALID_DOWID(id))
		return;

	DOW_MUTEX_LOCK();        

 	if (DOW_STATE(id) & DOW_GONE) {
		DOW_MUTEX_UNLOCK();
		return;
	}

	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);
	ident1 = DOW_IDENT1(id);
	ident2 = DOW_IDENT2(id);
	hbucket = DOW_HASHBUCKET(ident1, ident2);
#ifdef	DEBUG
	ASSERT(DOW_DEBUG_HASHID(id) == hbucket);
#endif
retry:
	while (DOW_MOD_LOCK(id) == (-1)) {
		DOW_SV_WAIT(hbucket);
		DOW_MUTEX_LOCK();
		if (DOW_STATE(id) & DOW_GONE) {
			DOW_MUTEX_UNLOCK();
			return;
		}
	}

	if ((DOW_STATE(id) & (DOW_GONE|DOW_INTRANS)) == DOW_INTRANS) {
		DOW_SV_WAIT(id);
		DOW_MUTEX_LOCK();
		goto retry;
	}

	ASSERT(DOW_MOD_LOCK(id) >= 0);
	(++(DOW_MOD_LOCK(id)));
	DOW_MUTEX_UNLOCK();
	return;
}

/*
 * void
 * dow_startmod_wrlock_l(dowid_t id)
 *	Acquire the modification interlock on a dow structure in write.
 *	mode. This is a pseudo reader-writer lock, whose intent is
 *	to ensure that if the dow structure carries the DOW_MODIFIED
 *	state, then the real page is modified as well.
 * 	Called only for page dows. Not needed for buffer dows, because
 *	buffers are uniquely owned, and writes on buffers always complete.
 *	Write mode locking is done by functions that clear the 
 *	MODIFY state.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller. The caller should be prepeared 
 *	to block, however.
 */
void
dow_startmod_wrlock_l(dowid_t id)
{
	dowid_t hbucket;
	long ident1, ident2;

	ASSERT(VALID_DOWID(id));

	if (DOW_STATE(id) & DOW_GONE)
		return;

	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);
	ident1 = DOW_IDENT1(id);
	ident2 = DOW_IDENT2(id);
	hbucket = DOW_HASHBUCKET(ident1, ident2);
#ifdef	DEBUG
	ASSERT(DOW_DEBUG_HASHID(id) == hbucket);
#endif
	while (DOW_MOD_LOCK(id) != 0) {
		DOW_SV_WAIT(hbucket);
		DOW_MUTEX_LOCK();
		if (DOW_STATE(id) & DOW_GONE) {
			return;
		}
	}
	ASSERT(DOW_MOD_LOCK(id) == 0);
	(--(DOW_MOD_LOCK(id)));
	return;
}


/*
 * boolean_t
 * dow_startmod_tryrdlock(dowid_t id)
 *	Try acquiring the modification interlock on a dow structure, in read
 *	mode. This is a pseudo reader-writer lock, whose intent is
 *	to ensure that if the dow structure carries the DOW_MODIFIED
 *	state, then the real page is modified as well.
 * 	Called only for page dows. Not needed for buffer dows, because
 *	buffers are uniquely owned, and writes on buffers always complete.
 *	Read mode lockers are those that intend to set the MOD state.
 *
 * Calling/Exit State:
 *	None. 
 */
boolean_t
dow_startmod_tryrdlock(dowid_t id)
{
	dowid_t hbucket;
	long ident1, ident2;

	if (!VALID_DOWID(id)) 
		return (B_TRUE);
	DOW_MUTEX_LOCK();
	if (DOW_STATE(id) & DOW_GONE) {
		DOW_MUTEX_UNLOCK();
		return (B_TRUE);
	}
	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);
	ident1 = DOW_IDENT1(id);
	ident2 = DOW_IDENT2(id);
	hbucket = DOW_HASHBUCKET(ident1, ident2);
#ifdef	DEBUG
	ASSERT(DOW_DEBUG_HASHID(id) == hbucket);
#endif
	if ((DOW_MOD_LOCK(id) == (-1)) || (DOW_STATE(id) & DOW_INTRANS)) {
		/*
		 * refuse the lock -- either it is not available for write
		 * mode, or the page is potentially intransit and cannot be
		 * permitted to be modified at the moment.
		 */
		DOW_MUTEX_UNLOCK();
		return(B_FALSE);
	}
	ASSERT(DOW_MOD_LOCK(id) >= 0);
	(++(DOW_MOD_LOCK(id)));
	DOW_MUTEX_UNLOCK();
	return (B_TRUE);
}

/*
 * boolean_t
 * dow_startmod_trywrlock_l(dowid_t id)
 *	Try acquiring the modification interlock on a dow structure in write.
 *	mode. This is a pseudo reader-writer lock, whose intent is
 *	to ensure that if the dow structure carries the DOW_MODIFIED
 *	state, then the real page is modified as well.
 * 	Called only for page dows. Not needed for buffer dows, because
 *	buffers are uniquely owned, and writes on buffers always complete.
 *	Write mode locking is done by functions that clear the MODIFY state.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller; function returns without dropping. 
 */
boolean_t
dow_startmod_trywrlock_l(dowid_t id)
{
	dowid_t hbucket;
	long ident1, ident2;

	ASSERT(VALID_DOWID(id));

	if (DOW_STATE(id) & DOW_GONE)
		return(B_TRUE);

	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);
	ident1 = DOW_IDENT1(id);
	ident2 = DOW_IDENT2(id);
	hbucket = DOW_HASHBUCKET(ident1, ident2);
#ifdef	DEBUG
	ASSERT(DOW_DEBUG_HASHID(id) == hbucket);
#endif
	if (DOW_MOD_LOCK(id) != 0) {
		return (B_FALSE);
	}
	(--(DOW_MOD_LOCK(id)));
	return(B_TRUE);
}


/*
 * void
 * dow_drop_modlock_l(dowid_t id)
 *	Release the modification interlock on a page dow.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */
void
dow_drop_modlock_l(dowid_t id)
{
	dowid_t hbucket;
	long ident1, ident2;

	ASSERT(VALID_DOWID(id));

	if (DOW_STATE(id) & DOW_GONE)
		return;

	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);

	if (DOW_MOD_LOCK(id) > 0) {
		(--(DOW_MOD_LOCK(id)));
		if (DOW_MOD_LOCK(id) > 0) {
			return;
		}
	} else {
		ASSERT(DOW_MOD_LOCK(id) == (-1));
		(++(DOW_MOD_LOCK(id)));
	}
	ident1 = DOW_IDENT1(id);
	ident2 = DOW_IDENT2(id);
	hbucket = DOW_HASHBUCKET(ident1, ident2);
#ifdef	DEBUG
	ASSERT(DOW_DEBUG_HASHID(id) == hbucket);
#endif
	if (DOW_SV_BLKD(hbucket))
		DOW_SV_BROADCAST(hbucket);
	return;
}

/*
 * void
 * dow_clear_modlock_l(dowid_t id)
 *	Clear the modification interlock on a page dow and wakeup
 *	all waiters. It is expected that the caller will reset the
 *	DOW_STATE to DOW_GONE before dropping the dow_mutex, which
 *	is held on entry to this function. 
 *	
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */
void
dow_clear_modlock_l(dowid_t id)
{
	dowid_t hbucket;
	long ident1, ident2;

	ASSERT(VALID_DOWID(id));
	if (DOW_STATE(id) & DOW_GONE)
		return;
	ASSERT(DOW_TYPE(id) == DOW_ISPAGE);
	ident1 = DOW_IDENT1(id);
	ident2 = DOW_IDENT2(id);
	hbucket = DOW_HASHBUCKET(ident1, ident2);

#ifdef	DEBUG
	ASSERT(DOW_DEBUG_HASHID(id) == hbucket);
#endif

	DOW_MOD_LOCK(id) = 0; 
	if (DOW_SV_BLKD(hbucket))
		DOW_SV_BROADCAST(hbucket);
	/*
	 * clear the pagesync_modlock_dowid, if we are picking up
	 * a dowid that is modlocked by the pagesync daemon.
	 */
	if (pagesync_modlock_dowid == id)
		pagesync_modlock_dowid = DOW_BADID;
	/*
	 * also, let the pagesync daemon make progress if it
	 * has already stumbled upon this dowid and is attempting
	 * to handle it.
	 */
	if ((dow_pagesync_vp == DOW_VP(id)) &&
			(dow_pagesync_off == DOW_OFFSET(id))) {
		dow_pagesync_vp = NULL;
		dow_pagesync_off = (-1);
	}
	return;
}



/*
 * void
 * dow_setmod(dowid_t id, clock_t flushdelta)
 *	Set the MODIFY state in the dow, and release the modification interlock.
 *	Set the timestamp such that dow_flush_demon flushes out the dow within
 *	the period of clockticks specified by flushdelta.
 *
 * Calling/Exit State:
 *	The caller must have acquired the modification interlock before calling
 * 	this function (this cannot be ASSERT'ed, however, since it is a pseudo
 *	reader-writer sleep lock). 
 */

void
dow_setmod(dowid_t id, clock_t flushdelta)
{
	uchar_t	dow_state; 
	clock_t flush_stamp;

	if (!VALID_DOWID(id) || (DOW_STATE(id) & DOW_GONE))
		return;

	ASSERT(flushdelta <= FLUSHDELTAMAX);

	DOW_MUTEX_LOCK();
	/*
	 * the following assert rules out setting the MOD bit on
	 * an aborted DOW. not clear if we should just allow the
	 * operation but not set the MOD bit.
	 */

	ASSERT(DOW_HOLD(id) > 0);
	if (DOW_STATE(id) & DOW_MODIFIED) {
		ASSERT((DOW_TYPE(id) & DOW_ISFUNC) == 0);
		if (DOW_TYPE(id) == DOW_ISPAGE) {
			dow_drop_modlock_l(id);
		} 
		flush_stamp = (lbolt - (FLUSHDELTAMAX - flushdelta));
		if (DOW_TIMESTAMP(id) > flush_stamp) {
			DOW_TIMESTAMP(id) = flush_stamp;
		}
		DOW_MUTEX_UNLOCK();
		SWAP_ENABLE();
		return;
	}
	DOW_STATE(id) |= DOW_MODIFIED;
	dow_state = DOW_STATE(id);
	flush_stamp = (lbolt - (FLUSHDELTAMAX - flushdelta));

	/*
	 * Fire the dow if:
	 *	- no antecdents, and, not already being written/executed,
	 *	  and,
	 *		- type == function, or
	 *		- pruned (though how can this be?)
	 */

	if ((DOW_ANT_CNT(id) == 0) && ((dow_state & DOW_FLUSHINTRANS) == 0)) {
		if (DOW_TYPE(id) & DOW_ISFUNC) {

			dow_flush_remove(id);
			DOW_STATE(id) |= (DOW_FLUSH|DOW_PRUNE);
			DOW_TIMESTAMP(id) = INT_MAX;
			dow_process_leaf_func(id, NULL);
			DOW_MUTEX_UNLOCK();
			return;

		} else if (dow_state & DOW_PRUNE) {
			/* Should not this dow already be on the leaf chain? */
			dow_flush_remove(id);
			dow_flush_tailins(id, DOW_LEAF_FLUSHHEAD);
			/*
			 * PERF: instead of leaving the dow on leaf chain
			 *       for flush demon to complete as above, we
			 *	 could just handle it right away, as below?
			 *	 if we do, then, remember to drop the 
			 *	 dow_modlock in page cases!
			 *	
			 * DOW_STATE(id) |= DOW_FLUSH;
			 * if (dow_state & DOW_IOSETUP) {
			 *	dow_ioflush(id); 
			 *	return;
			 * } else {
			 *	dow_flush(id); 
			 *	return;
			 * }
			 */
			DOW_TIMESTAMP(id) = flush_stamp;
			if (DOW_TYPE(id) == DOW_ISPAGE) {
				dow_drop_modlock_l(id);
			}
			DOW_MUTEX_UNLOCK();
			SWAP_ENABLE();
			return;
		}
	}

	DOW_TIMESTAMP(id) = flush_stamp;
	/*
	 * The dowid stays on whichever flush chain it happens to be on!
	 */	
	if (DOW_TYPE(id) == DOW_ISPAGE) {
		dow_drop_modlock_l(id);
	}

	DOW_MUTEX_UNLOCK();
	if ((DOW_TYPE(id) & DOW_ISFUNC) == 0)
		SWAP_ENABLE();
	return;
}

/*
 * void
 * dow_rele(dowid_t id)
 *	Release the hold on a dow structure.
 *
 * Calling/Exit State:
 *	The caller must have previously acquired a hold via a lookup or
 *	create operation.
 *
 * Remarks: 
 *	dow_rele_l does the real work.
 */
void
dow_rele(dowid_t id)
{
	if (!VALID_DOWID(id)) {
		return;
	}
	DOW_MUTEX_LOCK();
	dow_rele_l(id); /* does not drop the dow_mutex */
	DOW_MUTEX_UNLOCK();
	return;
}

/*
 * void
 * dow_rele_l(dowid_t id)
 *	Internal interface to release a hold on the dow structure and to
 *	initiate cleaning/freeing the dow if appropriate.
 *
 * Calling/Exit State:
 * 	Called with dow_mutex held, returns with same held. 
 */
void
dow_rele_l(dowid_t id)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(DOW_HOLD(id) > 0);
	CHECK_NOTFREE(id);
	DOW_CHEAP_RELE(id);

	/*
	 * if the hold is positive, or there are any links
	 * (antecdent/dependent) , don't free the dow.
	 *
	 * PERF: Collapse the three tests into one?
	 */

	if (DOW_HOLD(id) > 0 || DOW_ANT_CNT(id) > 0 || 
	    DOW_DEP_CNT(id) > 0) {
		return;
	}	
	/*
	 * hold == 0, no antecdent/dependent links. Free the dow
	 * if it is not being flushed/written. Alternatively, if
	 * the dow is a function whose execution is deferred to the
	 * flush demon, we cannot remove it.
	 */
	if (DOW_STATE(id) & (DOW_FLUSHINTRANS|DOW_IOSETUP)) {
		return;
	} 
	/*
	 * function types recieve a slightly different treatment:
	 * for a "modified" function dow, we must either absorb the
	 * responsibility of executing the function here itself or
	 * leave it to the flush demon (=> verify that the dow is on
	 * the leaf list and that it is in PRUNE state); the dow, in
	 * this case should not be freed without executing the function.
	 * A function dow that does not have the modified state, on
	 * the other hand, must be assumed to have been "clear-modified"
	 * in which case, dow_rele can treat it basically as free-able.
	 * See discussion in dow_process_leaf_func().
	 */

	if ((DOW_TYPE(id) & DOW_ISFUNC) && 
	    ((DOW_STATE(id) & (DOW_MODIFIED | DOW_GONE)) == DOW_MODIFIED)) {
		CHECK_ON_FLUSHCHAIN(id, DOW_LEAF_FLUSHHEAD);
		return;
	}

	/*
	 * At this point, this dow is a goner! We should be able
	 * to assert that if the GONE state is ON, then the dow
	 * cannot be on any hash chains.
	 */
	
	dow_flush_remove(id);
	if ((DOW_STATE(id) & DOW_GONE) == 0) {
		dow_remhash(id);
		if (DOW_TYPE(id) == DOW_ISPAGE) {
			VN_SOFTRELE(DOW_VP(id));
		}
	} else {
		CHECK_HASH_REMOVED(id);
	}
	dow_free(id);
	return;
}
/*
 * void
 * dow_intrans_wait(dowid_t id)
 *	Wait for any current IO sequence to complete. This may not necessarily
 *	require that an IO must occur -- just that dow_iodone processing
 *	is performed on the id. 
 *
 * Remarks:
 *	No special handling is needed for the DOW_GONE case, assuming that
 *	DOW_FLUSH/DOW_IOSETUP/DOW_INTRANS all guarantee that dow_iodone will
 *	be performed for the id.
 */
void
dow_intrans_wait(dowid_t id)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(DOW_HOLD(id) > 0);

	if (DOW_STATE(id) & (DOW_FLUSHINTRANS | DOW_IOSETUP)) {
		DOW_SV_WAIT(id);
	} else {
		DOW_MUTEX_UNLOCK();
	}
}

/*
 * void
 * dow_iowait_l(dowid_t id)
 *	Wait until the IO initiated by a previous dow_handle_async
 *	completes.
 *
 * Calling/Exit State:
 *	Caller holds the dow mutex, and is prepared to block.
 *
 * Remarks:
 *	If dow_handle_async could not initiate a pruning operation due
 *	to an intransit IO when it got called, then it will set the
 *	state to PRUNE'd -- causing dow_iodone() to resubmit the id for
 *	a quick flush. In that case, dow_iowait may need to wait twice: 
 *	the first time to permit the INTRANS state to clear, and the
 *	second time, for the resubmitted IO to complete. 
 *
 *	No special handling is needed for the DOW_GONE case, assuming that
 *	(a) DOW_FLUSH/DOW_IOSETUP/DOW_INTRANS all guarantee that dow_iodone 
 *	    will be performed for the id. 
 *	(b) If the id is not in one of these states, then DOW_MODIFIED must
 *	    imply !DOW_GONE, so no additional checking is needed.
 *
 */
void
dow_iowait_l(dowid_t id)
{
	ASSERT(DOW_MUTEX_OWNED());
	ASSERT(DOW_HOLD(id) > 0);
	if (DOW_STATE(id) & (DOW_FLUSHINTRANS | DOW_IOSETUP)) {
		if ((DOW_STATE(id) & DOW_MODINTRANS) != DOW_MODINTRANS) {
			DOW_SV_WAIT(id);
			return;
		}
		/* 
		 * else: MODIFIED and INTRANS. In this case, we may need to
		 * 	 wait a second time (depending on whether PRUNE is
		 *	 turned ON or not). 
		 */
		if ((DOW_STATE(id) & DOW_PRUNE) == 0) {
			DOW_SV_WAIT(id);
			return;
		}
		/*
		 * After the current IO completes, we will retest the state
		 * and wait a second time.
		 */
		DOW_SV_WAIT(id);
		DOW_MUTEX_LOCK();
	} 

	if ( (DOW_STATE(id) & (DOW_FLUSHINTRANS | DOW_IOSETUP)) ||
	     ((DOW_STATE(id) & DOW_MODPRUNE) == DOW_MODPRUNE)) {
		DOW_SV_WAIT(id);
		return;
	} 
	/* 
	 * there is no IO that the caller is interested in waiting for.
	 */
	DOW_MUTEX_UNLOCK();
	return;
}

/*
 * void
 * dow_iowait(dowid_t id)
 *	Wait for any initiated IO on the dow to finish, including IO that
 *	may not be currently underway but would happen soon, anyway, due
 *	to the setting of the PRUNE state.
 *
 * Calling/Exit State:
 *	None. The caller must be prepared to block.
 */
void
dow_iowait(dowid_t id)
{
	if (!VALID_DOWID(id)) {
		return;
	}
	ASSERT(KS_HOLD0LOCKS());
	DOW_MUTEX_LOCK();
	dow_iowait_l(id); /* drops the mutex */
	return;
}
