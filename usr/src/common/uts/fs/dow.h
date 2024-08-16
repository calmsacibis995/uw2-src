/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow.h	1.7"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

#ifndef	_FS_DOW_H
#define	_FS_DOW_H

#ifdef	_KERNEL

#include <fs/buf.h>		/* REQUIRED */
#include <fs/vnode.h>		/* REQUIRED */
#include <util/ksynch.h>	/* REQUIRED */
#include <util/types.h>		/* REQUIRED */

typedef	short dowid_t;
typedef short dowlinkid_t;

/*
 * A note on DOW structures and struncture linkages.
 *
 * Each dow structure is used to represent one unit of deferred action:
 * a write or a deferred function. When not in use, unless a hold is
 * active on a dow structure, the structure is placed on a free list
 * for allocation to a new use.
 *
 * When in use, a dow structure is placed on some hash chain and one of
 * three flush chains -- with one exception. The exception is when a dow
 * structure is "aborted", but cannot be freed due to an existing hold
 * by some context. In this case, the dow structure is marked as having
 * been abolished ("DOW_GONE"), and is not placed on any hash chains.
 * This fact is exploited to union the freelist linkage with the hash
 * chain linkage in the dow structure.
 *
 * There are three flush chains: (a) leaf flush chain, (b) non-leaf, 
 * "pruned" flushed chain, and (c) an aged flush chain. The flush
 * chain membership determines how a dow structure is handled through
 * various operations, and is described in some detail in dow_prune.c
 * In brief, flush chains are used to divide work among different 
 * dow functions by isolating those actions that need to be performed
 * quickly from those that do not have an associated urgency.
 *
 */

typedef struct	dow {
	union {
		struct  {
			dowid_t	dowfreenext;
			dowid_t dowfreeprev;
		} dowfreelinks;
		struct {
			dowid_t dowhashnext;
			dowid_t dowhashprev;
		} dowhashlinks;
	} dow_links;
	dowid_t dow_flushnext;
	dowid_t	dow_flushprev;
	uchar_t dow_spare;	/* free space */
	uchar_t	dow_level;	/* level depth: currently unused */
	uchar_t	dow_dep_cnt;	/* number of immediate dependents */
	uchar_t	dow_ant_cnt;	/* number of immediate antecedents */
	/*
	 * sync variable to signal waiters on
	 * the DOW_INTRANS state.
	 *
	 * The hash bucket SVs are used, on ther other hand,
	 * to implement a pseudo sleep lock for signalling 
	 * the modify-busy exchange between dow_setmod and
	 * dow_strategy.
	 */
	sv_t	dow_sv;		
	 
	uchar_t	dow_state;
	/*
	 * dow_type specifies whether it is a page, a buffer,
	 * a general function, or a non-blocking function 
 	 */
	uchar_t	dow_type;
	short	dow_hold;
	clock_t dow_timestamp;	
	union {
		dev_t dowdev;		/* "dev" for a buffer dow */
		vnode_t *dowvp; 	/* "vp" for a page dow */
		void (*dowfunc)(); 	/* "func ptr" for a function dow */
		long dowident1;		
	} dow_u1;
	union {
		int dowblkno;		/* "blkno" for a buffer dow */
		off_t dowoffset;	/* "offset" for a page dow */
		void *dowargp;		/* "argp" for a function dow */
		long dowident2;
	} dow_u2;

	buf_t	*dow_bp;		/* pageio/buffer header pointer */
	/*
	 * we should not really need to remember
	 * size for a delayed write buffer, since we
	 * expect that a waiting variant of blookup
	 * must find the buffer if the DOW is marked
	 * modified. however, we remember it anyway,
	 * for now -- for convenience.
	 */
	union {
		long	dowbsize;	/* block size for a buffer dow */
		long	dowlen;		/* length of unit of a "page" dow */
	} dow_u3;
	/*
	 * dow_mod_lock count: for pages only. We could union it
	 * with the dow_bsize field, above.
	 */
	short	dow_mod_lock;
	/* 
	 * b_iodone/b_iochain linkage 
	 */
	void	(*dow_b_iodone)();		
	void	*dow_b_iochain;
#ifdef	DEBUG
	dowid_t dow_debug_hashid;
	dowid_t	dow_debug_flushid;
	short	dow_debug_cnt;
#endif
} dow_t;
/*
 * dow_state values
 */
#define	DOW_MODIFIED	0x01
#define	DOW_INTRANS	0x02
#define	DOW_FLUSH	0x04
#define	DOW_IOSETUP	0x08
#define	DOW_PRUNE	0x10
#define	DOW_GONE	0x20


/*
 * common compound tests of dow structure state: following are useful.
 */
#define	DOW_MODINTRANS		(DOW_MODIFIED | DOW_INTRANS)
#define	DOW_FLUSHINTRANS	(DOW_FLUSH | DOW_INTRANS)
#define	DOW_MODPRUNE		(DOW_MODIFIED | DOW_PRUNE)



/*
 * dow_type values
 */
#define	DOW_ISBUFFER	0x01
#define	DOW_ISPAGE	0x02
#define	DOW_ISFUNC	0x04
#define DOW_ISFUNC_NBLK	0x0C	/* includes DOW_ISFUNC */

/*
 * dow_order: flag values and return codes from dow_order_nowait
 */

/* wait flags */
#define	DOW_NO_RESWAIT	0x01 
#define	DOW_NO_WAIT	0x02 
#define	DOW_ANY_WAIT	0x04

/* return codes */
#define	DOW_ANT_BUSY		1
#define	DOW_DEP_BUSY		2
#define DOW_POS_CYCLE		3
#define DOW_DEF_CYCLE		4
#define DOW_CREATE_OK		5
#define DOWLINK_NORES		6
#define DOW_DEP_GONE		7
#define DOW_DEP_PRUNE		8 
#define DOW_MAX_ANTCNT		9
#define	DOW_MAX_DEPCNT		10

/* 
 * We maintain the dow and dowlink free lists as doubly linked
 * lists -- anchored at the last elements of each array.
 * However, the freelist management can (and should) be made
 * simpler by making these lists singly linked and having all
 * allocation and freeing proceed directly at the heads of the
 * the freelists. The "prev" pointers can be used at that point
 * only in the DEBUG cases.
 */

#define	dow_freenext	dow_links.dowfreelinks.dowfreenext
#define	dow_freeprev	dow_links.dowfreelinks.dowfreeprev
#define	dow_hashnext	dow_links.dowhashlinks.dowhashnext
#define dow_hashprev	dow_links.dowhashlinks.dowhashprev

#define	dow_dev		dow_u1.dowdev
#define	dow_vp		dow_u1.dowvp
#define	dow_func	dow_u1.dowfunc
#define	dow_ident1	dow_u1.dowident1
#define	dow_blkno	dow_u2.dowblkno
#define	dow_offset	dow_u2.dowoffset
#define	dow_argp	dow_u2.dowargp
#define	dow_ident2	dow_u2.dowident2
#define	dow_bsize	dow_u3.dowbsize
#define	dow_len		dow_u3.dowlen

#define DOW_ID_TO_P(x)		((void *)(&(dow_tab[(x)])))
#define DOW_P_TO_ID(idaddr)	(((dow_t *)(idaddr)) - (&(dow_tab[0])))
#define	DOW_HOLD(x)		((dow_tab[(x)]).dow_hold)
#define	DOW_LEVEL(x)		((dow_tab[(x)]).dow_level)
#define DOW_DEP_CNT(x)		((dow_tab[(x)]).dow_dep_cnt)
#define DOW_ANT_CNT(x)		((dow_tab[(x)]).dow_ant_cnt)
#define	DOW_TIMESTAMP(x)	((dow_tab[(x)]).dow_timestamp)
#define	DOW_FREENEXT(x)		((dow_tab[(x)]).dow_freenext)
#define	DOW_FREEPREV(x)		((dow_tab[(x)]).dow_freeprev)
#define	DOW_HASHNEXT(x)		((dow_tab[(x)]).dow_hashnext)
#define	DOW_HASHPREV(x)		((dow_tab[(x)]).dow_hashprev)
#define	DOW_FLUSHNEXT(x)	((dow_tab[(x)]).dow_flushnext)
#define DOW_FLUSHPREV(x)	((dow_tab[(x)]).dow_flushprev)
#define	DOW_SVP(x)		(&(dow_tab[(x)]).dow_sv)
#define	DOW_STATE(x)		((dow_tab[(x)]).dow_state)
#define	DOW_TYPE(x)		((dow_tab[(x)]).dow_type)
#define	DOW_DEV(x)		((dow_tab[(x)]).dow_dev)
#define	DOW_VP(x)		((dow_tab[(x)]).dow_vp)
#define	DOW_FUNC(x)		((dow_tab[(x)]).dow_func)
#define	DOW_IDENT1(x)		((dow_tab[(x)]).dow_ident1)
#define	DOW_BLKNO(x)		((dow_tab[(x)]).dow_blkno)
#define	DOW_OFFSET(x)		((dow_tab[(x)]).dow_offset)
#define	DOW_ARGP(x)		((dow_tab[(x)]).dow_argp)
#define	DOW_IDENT2(x)		((dow_tab[(x)]).dow_ident2)
#define	DOW_BP(x)		((dow_tab[(x)]).dow_bp)
#define	DOW_BSIZE(x) 		((dow_tab[(x)]).dow_bsize)
#define	DOW_LEN(x) 		((dow_tab[(x)]).dow_len)
#define	DOW_MOD_LOCK(x) 	((dow_tab[(x)]).dow_mod_lock)
#define	DOW_B_IOCHAIN(x)	((dow_tab[(x)]).dow_b_iochain)
#define	DOW_B_IODONE(x)		((dow_tab[(x)]).dow_b_iodone)
#define	DOW_SPARE(x)		((dow_tab[(x)]).dow_spare)

#ifndef	DOW_KLUDGE
#define	DOW_TRACKED	0x01
#define	DOW_PUTPAGED	0x02
#define	DOW_STRATTED	0x04
#define	DOW_WRITTEN	0x08
extern	void mark_vp_offset_dows(vnode_t *, off_t , uchar_t);
extern	void check_vp_offset_dows(vnode_t *, off_t ); 
#define	MARK_VP_OFFSET_DOWS(x, y, z) mark_vp_offset_dows((x), (y), (z))
#define	CHECK_VP_OFFSET_DOWS(x, y) check_vp_offset_dows((x), (y))
#else
#define	DOW_TRACKED	
#define	DOW_PUTPAGED	
#define	DOW_STRATTED	
#define	DOW_WRITTEN	
#define	MARK_VP_OFFSET_DOWS(x, y, z) 
#define	CHECK_VP_OFFSET_DOWS(x, y) 
#endif


#ifdef	DEBUG
#define	DOW_DEBUG_HASHID(x)	((dow_tab[(x)]).dow_debug_hashid)
#define	DOW_DEBUG_FLUSHID(x)	((dow_tab[(x)]).dow_debug_flushid)
#define	DOW_DEBUG_CNT(x)	((dow_tab[(x)]).dow_debug_cnt)
#else
#define	DOW_DEBUG_HASHID(x) 
#define	DOW_DEBUG_FLUSHID(x) 
#define	DOW_DEBUG_CNT(x) 
#endif

/*
 * DOW_CHEAP_RELE and DOW_CHEAP_HOLD are used to adjust the hold
 * count without doing much else.
 */
#define DOW_CHEAP_HOLD(x)	(++(DOW_HOLD((x))))
#define DOW_CHEAP_RELE(x)	(--(DOW_HOLD((x))))

#define	DOW_DEP_MAX	32
#define	DOW_ANT_MAX	32
#define DOW_AGE_TIX	(HZ * 5) 	/* 5 seconds */
#define	FLUSHDELTAMAX	(HZ * 180) 	/* 180 seconds == "infinite dow delay" */
#define	MAX_RECUR	7

/*
 *	DOWLINK structures:
 *
 * The dowlink structures are used in linking up dows into chains of
 * antecedent or dependent structures. Since these relationships can
 * be one->many in each direction, we cannot use fields within dow
 * structures for encoding them. So we use the dowlink structures;
 * each dowlink structure encodes a single dependence.
 */ 
typedef struct dow_link {
	/*
	 * the dowlink_next/dowlink_prev linkages double as free list
	 * linkages for non-active link structures; furthermore, the
	 * dowlink_prev linkage is used for doubly linking the free
	 * list ONLY in DEBUG mode.
	*/
	dowlinkid_t	dowlink_next;	
	dowlinkid_t	dowlink_prev;
	/*
	 * Suppose this link is on a chain that specifies the antecdents of
	 * a dow, say X. If this link specifies the dependence: (X --> Y),
	 * then the following two fields identify:
	 *	(a) the inverse link: (Y <-- X) -- on Y's list of antecdents
	 *	(b) the dowid, Y.
	 *
	 * Similarly, if the link specified an antecdence relationship,
	 * say, (X <-- Z), then the inverse link would be the one on Z's 
	 * list of antecdents that specifies (Z --> X). 
	 */
	dowlinkid_t	dowlink_inverse;/* the inverse link */
	dowid_t		dowlink_dowid;	/* the antecdent/dependent dowid */
#ifdef	DEBUG
	/*
	 * the dowlink_debug_lhead field identifies the link chain head.
	 */
	dowlinkid_t	dowlink_debug_lhead;
	/*
	 * dowlink_debug_cnt is used only by linkheaders. but since this
	 * is a DEBUG mode field, we don't try to union it away with something
	 * else in order to save space.
	 */
	short		dowlink_debug_cnt;
#endif
} dowlink_t;

#define	DOWLINK_NEXT(i)		((dow_link_tab[(i)]).dowlink_next) 
#define	DOWLINK_PREV(i)		((dow_link_tab[(i)]).dowlink_prev)
#define	DOWLINK_INVERSE(i)	((dow_link_tab[(i)]).dowlink_inverse)
#define DOWLINK_DOWID(i)	((dow_link_tab[(i)]).dowlink_dowid)

#ifdef DEBUG
#define	DOWLINK_DEBUG_MYDOW(i)	\
	((dow_link_tab[(i)]).dowlink_debug_lhead % DOW_TABLE_SIZE)
#define	DOWLINK_DEBUG_LHEAD(i)	((dow_link_tab[(i)]).dowlink_debug_lhead)
#define	DOWLINK_DEBUG_CNT(i)	((dow_link_tab[(i)]).dowlink_debug_cnt)
#else
#define	DOWLINK_DEBUG_MYDOW(i)	
#define	DOWLINK_DEBUG_LHEAD(i) 
#define	DOWLINK_DEBUG_CNT(i)	
#endif


/*
 * structure for iochaining the untracked pageio function. to prevent
 * potential resource deadlocks when tracking pageios for which dow
 * structures could not be allocated.
 */
typedef struct dow_io_utp { /* utp: UnTrackedPages */
	struct buf *diu_bp;
	void (*diu_func)();
	void *diu_chain;
} dow_io_utp_t;

#define	MAX_UNTRACKED_PAGES	64

/*
 * dow_io_utp_tab is an array of dow_io_utp structures, each used to track
 * pages in transit that do not have DOWs associated with them, due to
 * failure to allocate the dow structures.
 */
extern	dow_io_utp_t	dow_io_utp_tab[];
extern	dow_t		dow_tab[];		/* dow structures array */
extern	dowlink_t	dow_link_tab[];		/* dowlink structures array */
extern	int		dow_freecount;		/* # free dows */
extern	int		dowlink_freecount;	/* # free dowlink structures */
extern	int		untracked_pageio;	/* # untracked in-transit pgs */
extern	lock_t		dow_mutex;		/* global spin lock */
extern	lkinfo_t	dow_mutex_lkinfo;
extern	sv_t		dow_free_sv;		/* SV for dow resource wait */
extern	sv_t		dowlink_free_sv;	/* SV for dowlink res. wait */
extern 	event_t 	dow_flush_event;	
extern	sv_t		dow_io_utp_sv; /* to control dow_io_utp_t alloc */
extern	struct vnode	*dow_pagesync_vp;
extern	off_t		dow_pagesync_off;
extern	dowid_t		pagesync_modlock_dowid;	



#define	DOW_IO_UTP_SV_INIT() 	SV_INIT(&dow_io_utp_sv)
#define	DOW_IO_UTP_SV_WAIT() 	SV_WAIT(&dow_io_utp_sv, PRIBUF, &dow_mutex)
#define	DOW_IO_UTP_SV_BLKD() 	SV_BLKD(&dow_io_utp_sv)
#define	DOW_IO_UTP_SV_BROADCAST() 	SV_BROADCAST(&dow_io_utp_sv, 0)

#define	DOW_MUTEX_HIER	FS_HIER_BASE + 4	
#define	DOW_MUTEX_IPL	PLFS
#define	DOW_MUTEX_INIT() \
	LOCK_INIT(&dow_mutex, DOW_MUTEX_HIER, DOW_MUTEX_IPL, \
		  &dow_mutex_lkinfo, KM_NOSLEEP)

/*
 * we will need to modify codepaths that do not want to acquire/release
 * dow_mutex at DOW_MUTEX_IPL and PLBASE respectively. 
 */
#define	DOW_MUTEX_LOCK()		LOCK(&dow_mutex, DOW_MUTEX_IPL)
#define	DOW_MUTEX_UNLOCK()		UNLOCK(&dow_mutex, PLBASE)
#define	DOW_MUTEX_UNLOCK_SAVEDPL(x)	UNLOCK(&dow_mutex, (x))
#define	DOW_MUTEX_OWNED()		LOCK_OWNED(&dow_mutex)

#define	DOW_SV_INIT(x)		SV_INIT(DOW_SVP(x))
#define	DOW_SV_WAIT(x) 		SV_WAIT(DOW_SVP(x), PRIBUF, &dow_mutex)
#define	DOW_SV_BROADCAST(x)	SV_BROADCAST(DOW_SVP(x), 0)
#define	DOW_SV_SIGNAL(x)	SV_SIGNAL(DOW_SVP(x), 0)
#define	DOW_SV_BLKD(x)		SV_BLKD(DOW_SVP(x))

#define	DOW_FREE_SV_INIT()	SV_INIT(&dow_free_sv)
#define	DOW_FREE_SV_WAIT() 	SV_WAIT(&dow_free_sv, PRIBUF, &dow_mutex)
#define	DOW_FREE_SV_BROADCAST()	SV_BROADCAST(&dow_free_sv, 0)
#define	DOW_FREE_SV_SIGNAL()	SV_SIGNAL(&dow_free_sv, 0)
#define	DOW_FREE_SV_BLKD()	SV_BLKD(&dow_free_sv)

#define	DOWLINK_FREE_SV_INIT()	SV_INIT(&dowlink_free_sv)
#define	DOWLINK_FREE_SV_WAIT() 	SV_WAIT(&dowlink_free_sv, PRIBUF, &dow_mutex)
#define	DOWLINK_FREE_SV_BLKD()	SV_BLKD(&dowlink_free_sv)
#define	DOWLINK_FREE_SV_BROADCAST()	SV_BROADCAST(&dowlink_free_sv, 0)
#define	DOWLINK_FREE_SV_SIGNAL()	SV_SIGNAL(&dowlink_free_sv, 0)

#define	DOW_FLUSH_EVENT_CLEAR()	EVENT_CLEAR(&dow_flush_event)
#define	DOW_FLUSH_EVENT_WAIT()	EVENT_WAIT(&dow_flush_event, (PRIBUF + 1))
#define	DOW_FLUSH_EVENT_BROADCAST()	EVENT_BROADCAST(&dow_flush_event, 0)
#define	DOW_FLUSH_EVENT_INIT()	EVENT_INIT(&dow_flush_event)


#define	DOW_AVAIL(x)		(dow_freecount >= (x))
#define DOWLINK_AVAIL(x)	(dowlink_freecount >= (x))

/*
 * The dow structures are organized as a statically allocated array.
 * The array also includes space for a set of special dow structures:
 * these are the dow structures that are used as anchors for the control
 * lists, namely the hash chains, the flush chains, and the freelist.
 *
 *                                             flush
 *                                             chain
 *                                             heads
 *                                            |<--->|
 *	+-+-+-+-+-+-+-+-+-+ ----- +-+-+-+ --- +-+-+-+-+
 *      | | | | | | | | | |       | | | |     | | | | |
 *	+-+-+-+-+-+-+-+-+-+ ----- +-+-+-+ --- +-+-+-+-+
 *      |<- usable dow structs--->|<-- hash ->|       \
 *                                 chain heads       free chain head
 *
 * 			DOW array
 *
 *	The first DOW_TABLE_SIZE elements are the DOW structures
 *	to be used in setting up ordering information.
 *
 *	The next DOW_HASHWIDTH elements are used as heads for
 *	doubly linked DOW hash chains.
 *
 *	The next 3 elements are used as heads for three doubly linked
 *	flush chains: 
 *		leaf chain: dows that have no antecdents and are ready
 *			for completing deferred action,
 *		pruned chain: dows that 
 *			- either have antecdents but have been pruned 
 *			  (i.e., all their antecdents have been either 
 *			  pruned or moved to the leaf chain). 
 *			- or have been processed and/or aborted, and are 
 *			  waiting for their holds to go to 0, so that they
 *			  can be freed. If abort processing has been
 *			  completed, then the dows are not on hash chains.
 *		aged chain: active dows that are not on the other two chains.
 *
 *	Please refer to the notes section for prune() family of functions
 *	for more details on how the flush chains are populated.
 *
 *	The last 1 element is used as head of a free list (singly
 *	linked normally; doubly linked in debug mode).
 */


	
/*
 * The dowlink structures are organized as a statically allocated array,
 * just as the dow structures are. However, the control lists are different.
 * For convenience, a set of dowlink structures serve as anchors for
 * linked lists of dependents: one anchor for each usable dow structure.
 * Similarly, a second set of dowlink structures are used as anchors of
 * linked lists of antecedents, one per usable dow structure.
 * These contolling structures are placed at the beginning of the dowlink
 * structure array.
 * 
 * Next come all the usable dowlink structures.
 *
 * These are followed by a single dowlink structure that serves as the
 * anchor for the free chain of dowlink structures.
 *
 *                                                           free chain
 *    dependents   antecedents                                     head
 *    chain        chain                                             \
 *   +---------+----------+------------------------------------------+-+
 *   | anchors |  anchors |          usable dowlink structures       | |
 *   +---------+----------+------------------------------------------+-+
 * 			DOWLINK array                             
 *                                                                 
 *	The first (2 * DOW_TABLE_SIZE) entries are used as heads of
 *	link chains: for dependent and antecdent chains for each of the
 *	DOW structure. Entries k and (DOW_TABLE_SIZE + k) respectively
 *	serve as dependent and antecdent link chain heads for the DOW
 *	structure whose index is k.
 *
 *	The next (DOW_LINKS_RATIO * DOW_TABLE_SIZE) entries are the
 *	real link entries that can be allocated and used for setting
 *	up linkages.
 *
 *	Finally, the last entry in the DOWLINK array serves as the
 *	head of a free chain. The chain is singly linked except in
 *	debug mode, when it is doubly linked.
 */

#define	DOW_TABLE_SIZE	500
#define	DOW_HASHWIDTH	31
#define	DOW_LEAF_FLUSHHEAD	(DOW_ARRAYSIZE - 4)
#define	DOW_PRUN_FLUSHHEAD	(DOW_ARRAYSIZE - 3)
#define	DOW_AGED_FLUSHHEAD	(DOW_ARRAYSIZE - 2)
#define	DOW_FREEHEAD	(DOW_ARRAYSIZE - 1)
#define	DOW_ARRAYSIZE	(DOW_TABLE_SIZE + DOW_HASHWIDTH + 3 + 1)
#define	DOW_FIRST_HASHBUCKET	DOW_TABLE_SIZE
#define	DOW_LAST_HASHBUCKET	(DOW_FIRST_HASHBUCKET + DOW_HASHWIDTH - 1)
#define	DOW_HASHBUCKET(is_what, is_which) \
	((((ulong_t)(is_what) + (ulong_t)(is_which)) % DOW_HASHWIDTH) + \
	DOW_TABLE_SIZE)

#define DEPENDENT_LINKS	0	/* from each dow to its dependents */
#define	ANTECDENT_LINKS	1	/* from each dow to its antecedents */
#define	DOW_LINKSRATIO	8
#define	DOWLINK_TABLE_BASE	(2 * DOW_TABLE_SIZE)
#define	DOWLINK_MAXFREE		(DOW_LINKSRATIO * DOW_TABLE_SIZE)
#define	DOWLINK_ARRAYSIZE	(DOWLINK_TABLE_BASE + DOWLINK_MAXFREE + 1)
#define	DOWLINK_FREEHEAD	(DOWLINK_ARRAYSIZE - 1)
#define DOWLINK_DEP_LINKHEAD(dow_id) \
	((dowlinkid_t)((dow_id) + (DEPENDENT_LINKS * DOW_TABLE_SIZE)))
#define	DOWLINK_ANT_LINKHEAD(dow_id) \
	((dowlinkid_t)((dow_id) + (ANTECDENT_LINKS * DOW_TABLE_SIZE)))


/*
 * We use an arbitrary negative dowid_t as an invalid dowid return value,
 * and another arbitrary negative dowlinkid_t as an invalid dowlinkid
 * return value.
 */

#define	DOW_BADID	(-3319)
#define	DOW_NONE	DOW_BADID	/* useful synonym */
#define	DOWLINK_BADID	(-29448)
#define	DOW_BAD_IODONE	((void (*)())(0xffffffff))
#define	DOW_BAD_IOCHAIN ((void *)(0x0c0c0c0c))


/*
 * Following macros permit easy range checks.
 */

#define	VALID_DOWID(i) \
	(((i) >= (dowid_t)0) && ((i) < (dowid_t)DOW_TABLE_SIZE))

#define VALID_HASH_BUCKET(i) \
	(((i) >= (dowid_t)DOW_TABLE_SIZE) && \
	 ((i) <  (dowid_t)(DOW_TABLE_SIZE + DOW_HASHWIDTH)))

#define VALID_FLUSHHEAD(i) \
	(((i) >=  (dowid_t)(DOW_TABLE_SIZE + DOW_HASHWIDTH)) && \
	 ((i) <  (dowid_t)(DOW_TABLE_SIZE + DOW_HASHWIDTH + 3)))

#define VALID_DOWLINK_ID(j) \
	(((j) >= (dowlinkid_t)DOWLINK_TABLE_BASE) && \
	 ((j) <  (dowlinkid_t)(DOWLINK_TABLE_BASE + DOWLINK_MAXFREE)))

#define	VALID_DOWLINK_HEAD(j) \
	(((j) >= (dowlinkid_t)0) && ((j) < (dowlinkid_t)(2 * DOW_TABLE_SIZE)))
/*
 * Following macros permit consistecy checks for various doubly linked lists.
 */
#define	DOW_HASHLINKS_SANE(i) \
	((DOW_HASHPREV(DOW_HASHNEXT(i)) == (i)) && \
	 (DOW_HASHNEXT(DOW_HASHPREV(i)) == (i)))

#define	DOW_FLUSHLINKS_SANE(i) \
	((DOW_FLUSHPREV(DOW_FLUSHNEXT(i)) == (i)) && \
	 (DOW_FLUSHNEXT(DOW_FLUSHPREV(i)) == (i)))

#define	DOWLINK_LINKS_SANE(i) \
	((DOWLINK_NEXT(DOWLINK_PREV(i)) == (i)) && \
	 (DOWLINK_PREV(DOWLINK_NEXT(i)) == (i)))

#define	DOWLINK_ILINK_SANE(i) (DOWLINK_INVERSE(DOWLINK_INVERSE(i)) == (i))

#define	EMPTY_DOWLINK_LIST(listhead) ((listhead) == DOWLINK_NEXT((listhead)))
#define	EMPTY_DOW_FREELIST(listhead) ((listhead) == DOW_FREENEXT((listhead)))
#define EMPTY_DOW_HASHLIST(listhead) ((listhead) == DOW_HASHNEXT((listhead)))
#define	EMPTY_DOW_FLUSHLIST(lsthead) ((lsthead)  == DOW_FLUSHNEXT((lsthead)))


#ifdef DEBUG
/*
 * Macros for DEBUG field updates.
 */
#define DOW_DEBUG_HASHENTER(id, hbucket) { \
	ASSERT(VALID_DOWID(id)); \
	ASSERT(VALID_HASH_BUCKET(hbucket)); \
	ASSERT(DOW_DEBUG_HASHID(id) == DOW_BADID); \
	DOW_DEBUG_HASHID((id)) = (hbucket); \
	++DOW_DEBUG_CNT((hbucket)); \
	CHECK_HASH_CHAIN(hbucket); \
}

#define DOW_DEBUG_FLUSHENTER(id, flhead) { \
	ASSERT(VALID_DOWID(id)); \
	ASSERT(VALID_FLUSHHEAD(flhead)); \
	ASSERT(DOW_DEBUG_FLUSHID(id) == DOW_BADID); \
	DOW_DEBUG_FLUSHID((id)) = (flhead); \
	++DOW_DEBUG_CNT((flhead)); \
	CHECK_FLUSH_CHAIN((flhead)); \
}

#define	DOW_DEBUG_HASHLEAVE(id) { dowid_t _hbucket; \
	ASSERT(VALID_DOWID(id)); \
	_hbucket = DOW_DEBUG_HASHID((id)); \
	ASSERT(VALID_HASH_BUCKET(_hbucket)); \
	DOW_DEBUG_HASHID((id)) = DOW_BADID; \
	--DOW_DEBUG_CNT(_hbucket); \
	ASSERT(DOW_DEBUG_CNT(_hbucket) >= 0); \
	CHECK_HASH_CHAIN(_hbucket); \
	DOW_HASHINVAL((id)); \
}

#define	DOW_DEBUG_FLUSHLEAVE(id) { dowid_t _flhead; \
	ASSERT(VALID_DOWID(id)); \
	_flhead = DOW_DEBUG_FLUSHID((id)); \
	ASSERT(VALID_FLUSHHEAD(_flhead)); \
	DOW_DEBUG_FLUSHID((id)) = DOW_BADID; \
	--DOW_DEBUG_CNT(_flhead); \
	ASSERT(DOW_DEBUG_CNT(_flhead) >= 0); \
	CHECK_FLUSH_CHAIN(_flhead); \
	DOW_FLUSHINVAL((id)); \
}

#define DOW_DEBUG_FREEENTER(id) { \
	ASSERT(VALID_DOWID(id)); \
	ASSERT(DOW_DEBUG_HASHID(id) == DOW_BADID); \
	ASSERT(DOW_DEBUG_FLUSHID(id) == DOW_BADID); \
	DOW_DEBUG_HASHID((id)) = DOW_FREEHEAD; \
	DOW_DEBUG_FLUSHID((id)) = DOW_FREEHEAD; \
	++DOW_DEBUG_CNT((DOW_FREEHEAD)); \
	ASSERT(DOW_DEBUG_CNT(DOW_FREEHEAD) == dow_freecount); \
}

#define DOW_DEBUG_FREELEAVE(id) { \
	ASSERT(VALID_DOWID(id)); \
	ASSERT(DOW_DEBUG_HASHID((id)) == DOW_FREEHEAD); \
	ASSERT(DOW_DEBUG_FLUSHID((id)) == DOW_FREEHEAD); \
	DOW_DEBUG_HASHID((id)) = DOW_BADID; \
	DOW_DEBUG_FLUSHID((id)) = DOW_BADID; \
	--DOW_DEBUG_CNT((DOW_FREEHEAD)); \
	ASSERT(DOW_DEBUG_CNT(DOW_FREEHEAD) == dow_freecount); \
}

#define	DOW_DEBUG_IODONE_INIT(id) { \
	DOW_B_IODONE(id) = DOW_BAD_IODONE; \
	DOW_B_IOCHAIN(id) = DOW_BAD_IOCHAIN; \
}

#define	EMPTY_IODONE_LINKAGE(id) ((DOW_B_IODONE(id) == DOW_BAD_IODONE) && \
		(DOW_B_IOCHAIN(id) == DOW_BAD_IOCHAIN))

#else
#define DOW_DEBUG_HASHENTER(id, hbucket) 
#define	DOW_DEBUG_HASHLEAVE(id)  
#define DOW_DEBUG_FLUSHENTER(id, flhead)  
#define DOW_DEBUG_FLUSHLEAVE(id)    
#define	DOW_DEBUG_FREEENTER(id) 
#define	DOW_DEBUG_FREELEAVE(id) 
#define	DOW_DEBUG_IODONE_INIT(id) 
#define	EMPTY_IODONE_LINKAGE(id) 	B_TRUE
#endif

#ifdef DEBUG

#define	DOWLINK_DEBUG_ENTER(link, head)	{ \
	ASSERT(VALID_DOWLINK_ID(link)); \
	ASSERT(VALID_DOWLINK_HEAD(head)); \
	ASSERT(DOWLINK_DEBUG_LHEAD(link) == DOWLINK_BADID); \
	DOWLINK_DEBUG_LHEAD(link) = head; \
	++(DOWLINK_DEBUG_CNT(head)); \
	CHECK_DOWLINK_CHAIN(head); \
}
#define	DOWLINK_DEBUG_LEAVE(link) { dowlinkid_t _linkhead; \
	ASSERT(VALID_DOWLINK_ID(link)); \
	_linkhead = DOWLINK_DEBUG_LHEAD(link); \
	ASSERT(VALID_DOWLINK_HEAD(_linkhead)); \
	DOWLINK_DEBUG_LHEAD(link) = DOWLINK_BADID; \
	--(DOWLINK_DEBUG_CNT(_linkhead)); \
	ASSERT(DOWLINK_DEBUG_CNT(_linkhead) >= 0); \
	CHECK_DOWLINK_CHAIN(_linkhead); \
}

#define	DOWLINK_DEBUG_FREEENTER(link)	{ \
	ASSERT(VALID_DOWLINK_ID(link)); \
	ASSERT(DOWLINK_DEBUG_LHEAD(link) == DOWLINK_BADID); \
	DOWLINK_DEBUG_LHEAD(link) = DOWLINK_FREEHEAD; \
	++(DOWLINK_DEBUG_CNT(DOWLINK_FREEHEAD)); \
	ASSERT(DOWLINK_DEBUG_CNT(DOWLINK_FREEHEAD) == dowlink_freecount); \
}

#define	DOWLINK_DEBUG_FREELEAVE(link) { \
	ASSERT(VALID_DOWLINK_ID(link)); \
	ASSERT(DOWLINK_DEBUG_LHEAD(link) == DOWLINK_FREEHEAD); \
	DOWLINK_DEBUG_LHEAD(link) = DOWLINK_BADID; \
	--(DOWLINK_DEBUG_CNT(DOWLINK_FREEHEAD)); \
	ASSERT(DOWLINK_DEBUG_CNT(DOWLINK_FREEHEAD) == dowlink_freecount); \
}


#else
#define	DOWLINK_DEBUG_ENTER(link, head)	 
#define	DOWLINK_DEBUG_LEAVE(link)  
#define	DOWLINK_DEBUG_FREEENTER(link)	
#define	DOWLINK_DEBUG_FREELEAVE(link)  
#endif


#ifdef DEBUG
#define	DOW_FREE_BADINIT(x)	dow_free_badinit(x)
#define	DOW_DEBUG_INIT(x)	dow_debug_init(x)
#define	DOWLINK_FREE_BADINIT(x)	dowlink_free_badinit(x)
#define	DOWLINK_DEBUG_INIT(a, b)	dowlink_debug_init(a, b)
#else
#define	DOW_FREE_BADINIT(x)	
#define	DOW_DEBUG_INIT(x)	
#define	DOWLINK_FREE_BADINIT(x) 
#define	DOWLINK_DEBUG_INIT(a, b)	
#endif


#ifdef	DEBUG
#define	CHECK_HASHDUP(a, b, c, d) check_hashdup((a), (b), (c), (d))
#define	CHECK_HASH_CHAIN(x) check_hash_chain((x))
#define	CHECK_DOWLINK_CHAIN(x) check_dowlink_chain((x))
#define	CHECK_DOW_LINKAGE(x) check_dow_linkage((x))
#define	CHECK_ANTECDENTS_PRUNED(x) check_antecdents_pruned((x))
#define	CHECK_FLUSH_CHAIN(x) check_flush_chain((x))
#define	CHECK_HASH(x) check_hash((x))
#define	CHECK_ON_FLUSHCHAIN(x, flhead) check_on_flushchain((x), (flhead))
#define CHECK_NOTON_FLUSHCHAIN(x, flhead) check_noton_flushchain((x), (flhead))
#define CHECK_NOTON_ANY_FLUSHCHAIN(x) check_noton_any_flushchain((x))
#define	CHECK_HASH_REMOVED(x) check_hash_removed((x))
#define	CHECK_DOW_FREELIST() check_dow_freelist()
#define	CHECK_NOTFREE(x) check_notfree((x))
#define CHECK_DOWLINK_FREELIST check_dowlink_freelist
#define	DOW_EXISTS(x, y) dow_exists((x), (y))
#define DOW_VP_ANY(vp) dow_vp_any((vp))
#else
#define	CHECK_HASHDUP(a, b, c, d) 
#define	CHECK_HASH_CHAIN(x)
#define	CHECK_DOWLINK_CHAIN(x)
#define	CHECK_DOW_LINKAGE(x)  
#define	CHECK_ANTECDENTS_PRUNED(x)
#define	CHECK_FLUSH_CHAIN(x) 
#define	CHECK_HASH(x)
#define	CHECK_ON_FLUSHCHAIN(x, flhead) 
#define CHECK_NOTON_FLUSHCHAIN(x, flhead)
#define CHECK_NOTON_ANY_FLUSHCHAIN(x)
#define	CHECK_HASH_REMOVED(x) 
#define	CHECK_DOW_FREELIST() 
#define	CHECK_NOTFREE(x)  
#define CHECK_DOWLINK_FREELIST() 
#define	DOW_EXISTS(x, y) 
#define DOW_VP_ANY(vp) 
#endif

#ifdef	DEBUG
#define DOW_HASHINVAL(x) { \
	DOW_HASHNEXT((x)) = DOW_BADID; \
	DOW_HASHPREV((x)) = DOW_BADID; \
}
#define DOW_FLUSHINVAL(x) { \
	DOW_FLUSHNEXT((x)) = DOW_BADID; \
	DOW_FLUSHPREV((x)) = DOW_BADID; \
}
#else
#define DOW_HASHINVAL(x) 
#define	DOW_FLUSHINVAL(x) 
#endif


#ifdef DEBUG

extern void dow_free_badinit(dowid_t);
extern void dow_debug_init(dowid_t);
extern void dowlink_free_badinit(dowlinkid_t);
extern void dowlink_debug_init(dowlinkid_t , dowlinkid_t );

extern void check_flush_chain(dowid_t );
extern void check_on_flushchain(dowid_t , dowid_t );
extern void check_noton_flushchain(dowid_t , dowid_t );
extern void check_noton_any_flushchain(dowid_t );
extern boolean_t on_some_flushchain(dowid_t );
extern boolean_t dow_exists(long , long );
extern dowid_t dow_vp_any(vnode_t *);
extern void check_antecdents_pruned(dowid_t );
extern void check_dow_freelist();
extern void check_notfree(dowid_t );
extern void check_hashdup(dowid_t , dowid_t , long , long );
extern void check_hash(dowid_t );
extern void check_hash_chain(dowid_t );
extern void check_dowlink_chain(dowlinkid_t);
extern void check_dow_linkage(dowlinkid_t);
extern void check_dowlink_freelist();
extern void check_hash_removed(dowid_t);

#endif

extern buf_t * dow_buffer(dowid_t);
extern void dow_bdwrite(buf_t *);
extern int linkage_detect(dowid_t , dowid_t );

#ifdef _IF_NEEDED_
extern void dow_start_abort(dowid_t );
extern void dow_finish_abort(dowid_t );
#endif

extern void dow_abort(dowid_t );
extern void dow_abort_range(vnode_t *, off_t , int );
extern void dow_abort_rele_l(dowid_t );
extern dowid_t dow_alloc_l(void);
extern dowid_t dow_alloc(void);
extern dowid_t dow_alloc_l(void);
extern void dow_arrayinit(void);
extern void dow_clearmod(dowid_t );
extern dowid_t dow_create_page(vnode_t * , off_t , int, uint_t );
extern dowid_t dow_create_page_l(vnode_t *, off_t , int , uint_t );
extern dowid_t dow_create_buf(dev_t , daddr_t , long , uint_t );
extern dowid_t dow_create_func(void (*)(), void *, uint_t , boolean_t );
extern void dow_flush(dowid_t );
extern void dow_start_flush_timeout(void);
extern void dow_flush_timeout(void);
extern void dow_flush_daemon();
extern void dow_flush_headins(dowid_t , dowid_t);
extern void dow_flush_remove(dowid_t );
extern void dow_flush_tailins(dowid_t , dowid_t );
extern void dow_free(dowid_t );
extern void dow_handle_async(dowid_t );
extern void dow_handle_async_l(dowid_t );
extern void dow_handle_sync(dowid_t );
extern void dow_handle_sync_l(dowid_t);
extern void dow_init(dowid_t , long , long , uchar_t );
extern void dow_inshash(dowid_t , dowid_t );
extern void dow_io_utp_setup(buf_t *);
extern void dow_io_utp_rele(buf_t *);
extern void dow_buf_iodone(buf_t *);
extern void dow_page_iodone(buf_t *);
extern void dow_untracked_pageio_done(buf_t *);
extern void dow_iodone(dowid_t );
extern void dow_iodone_insert(dowid_t , buf_t *, void (*)());
extern void dow_iodone_restore(dowid_t , buf_t *);
extern void dow_ioflush(dowid_t );
extern void dow_intrans_wait(dowid_t );
extern void dow_iowait(dowid_t );
extern void dow_iowait_l(dowid_t );
extern dowid_t dow_lookup(long , long );
extern dowid_t dow_lookup_page(vnode_t *, off_t);
extern int dow_order_no_wait(dowid_t , dowid_t );
extern int dow_order(dowid_t , dowid_t , uint_t );
extern void dow_process_leaf_func(dowid_t, void (*)(dowid_t));
extern void dow_process_leaf_iodone(dowid_t );
extern boolean_t dow_process_leaf_flush(dowid_t );
extern int prune_search(dowid_t , int , boolean_t );
extern void prune(dowid_t );
extern void prune_antecdents(dowid_t );
extern void dow_rele(dowid_t );
extern void dow_rele_l(dowid_t );
extern void dow_remhash(dowid_t );
extern void dow_startmod(dowid_t );
extern void dow_startmod_rdlock(dowid_t );
extern void dow_startmod_wrlock_l(dowid_t );
extern boolean_t dow_startmod_tryrdlock(dowid_t );
extern boolean_t dow_startmod_trywrlock_l(dowid_t );
extern void dow_drop_modlock_l(dowid_t );
extern void dow_clear_modlock_l(dowid_t );
extern void dow_setmod(dowid_t , clock_t );
extern void dow_strategy_buf(buf_t *);
extern void dow_strategy_page(buf_t **, vnode_t *, off_t , int);
extern void dowlink_addlink(dowlinkid_t , dowlinkid_t );
extern dowlinkid_t dowlink_alloc(void);
extern void dowlink_arrayinit(void);
extern void dowlink_breaklink(dowlinkid_t , dowid_t , dowid_t );
extern void dowlink_free(dowlinkid_t );
extern void dowlink_init(dowlinkid_t , dowlinkid_t , dowid_t );
extern int  dowlink_makelink(dowid_t , dowid_t );
extern void dowlink_sublink(dowlinkid_t );

#endif	/* _KERNEL */
#if defined(__cplusplus)
	}
#endif

#endif	/* _FS_DOW_H */
