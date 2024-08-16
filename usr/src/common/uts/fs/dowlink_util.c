/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dowlink_util.c	1.3"
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

dowlink_t	dow_link_tab[DOWLINK_ARRAYSIZE];

/*
 * void
 * dowlink_arrayinit(void)
 *	Initialize the array of dow link structures.
 *
 * Calling/Exit State:
 *	None.
 */

void
dowlink_arrayinit(void)
{
	dowlinkid_t i;

	for (i = 0; i < (DOWLINK_ARRAYSIZE); i++) {
		dowlink_init(i, i, DOW_BADID);
		DOWLINK_DEBUG_INIT(i, i);
	}
	DOWLINK_FREE_SV_INIT();
	dowlink_freecount = 0;
	for (i = DOWLINK_TABLE_BASE; 
	     i < (DOWLINK_TABLE_BASE + DOWLINK_MAXFREE); i++) {
		DOWLINK_DEBUG_INIT(i, DOWLINK_BADID);
		dowlink_free(i);
	}
	ASSERT(dowlink_freecount == DOWLINK_MAXFREE);
}

/*
 * void
 * dowlink_init(dowlinkid_t link, dowlinkid_t inverselink, dowid_t id)
 *	Initialize the dow link upon allocation. If link X were to
 *	be used to specify an antecedence/dependence relationship
 *	between two dow_ids A and B, such that X is on the appropriate
 *	link chain for A, then,
 *		inverselink identifies the corresponding link on B's
 *			link chain that identifies the dowid A,
 *		id identifies the dowid B.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */

void
dowlink_init(dowlinkid_t link, dowlinkid_t inverselink, dowid_t id)
{
	DOWLINK_NEXT(link) = link;
	DOWLINK_PREV(link) = link;
	DOWLINK_INVERSE(link) = inverselink;
	DOWLINK_DOWID(link) = id;
}

#ifdef DEBUG

/*
 * void
 * dowlink_debug_init(dowlinkid_t link, dowlinkid_t head)
 *	Initialize debugging support fields in a link that will
 *	be inserted on a linkchain headed by "head".
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */

void
dowlink_debug_init(dowlinkid_t link, dowlinkid_t head)
{
	DOWLINK_DEBUG_LHEAD(link) = head;
	DOWLINK_DEBUG_CNT(link) = 0;
}

/*
 * void
 * dowlink_free_badinit(dowlinkid_t link)
 *	Festoon this link with artful material to lure insects.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */

void
dowlink_free_badinit(dowlinkid_t link)
{
	DOWLINK_INVERSE(link) = DOWLINK_BADID;
	DOWLINK_DOWID(link) = DOW_BADID;
}
#endif

/*
 * dowlinkid_t
 * dowlink_alloc(void)
 *	Allocate a link structure.
 *
 * Calling/Exit State:
 *	dow_mutex is held by caller.
 *
 * Remark:
 *	The freelist is maintained doubly linked only under DEBUG
 *	compile option. Therefore, the following assert is not valid
 *	except in DEBUG compile:
 *		ASSERT(DOWLINK_LINKS_SANE(DOWLINK_FREEHEAD))
 */
dowlinkid_t
dowlink_alloc(void)
{
	dowlinkid_t freefirst;
	
	ASSERT(DOWLINK_LINKS_SANE(DOWLINK_FREEHEAD)); 
	if (dowlink_freecount == 0) {
		ASSERT(EMPTY_DOWLINK_LIST(DOWLINK_FREEHEAD));
		return(DOWLINK_BADID);
	}
	ASSERT(!EMPTY_DOWLINK_LIST(DOWLINK_FREEHEAD));
	--(dowlink_freecount);
	freefirst = DOWLINK_NEXT(DOWLINK_FREEHEAD);
	ASSERT(VALID_DOWLINK_ID(freefirst));
	DOWLINK_NEXT(DOWLINK_FREEHEAD) = DOWLINK_NEXT(freefirst);
#ifdef	DEBUG 
	DOWLINK_PREV(DOWLINK_NEXT(freefirst)) = DOWLINK_FREEHEAD;
#endif 
	DOWLINK_DEBUG_FREELEAVE(freefirst);

#ifdef	DEBUG
	DOWLINK_PREV(freefirst) = (-8877);	
	DOWLINK_NEXT(freefirst) = (-7788);	
	DOWLINK_INVERSE(freefirst) = (-6655);	
	DOWLINK_DOWID(freefirst) = (-5566);	
#endif
	DOWLINK_DEBUG_INIT(freefirst, DOWLINK_BADID);

	return(freefirst);
}

/*
 * dowlinkid_t
 * dowlink_free(void)
 *	Free a link structure.
 *
 * Calling/Exit State:
 *	dow_mutex is held by caller.
 *
 * Remark:
 *	The freelist is maintained doubly linked only under DEBUG
 *	compile option. Therefore, the following assert is not valid
 *	except in DEBUG compile:
 *		ASSERT(DOWLINK_LINKS_SANE(DOWLINK_FREEHEAD))
 */

void
dowlink_free(dowlinkid_t link)
{
	ASSERT(VALID_DOWLINK_ID(link));
	DOWLINK_NEXT(link) = DOWLINK_NEXT(DOWLINK_FREEHEAD);

#ifdef DEBUG 
	DOWLINK_PREV(link) = DOWLINK_FREEHEAD;
	DOWLINK_PREV(DOWLINK_NEXT(link)) = link;
#endif 

	DOWLINK_NEXT(DOWLINK_FREEHEAD) = link;
	++dowlink_freecount;
	if (DOWLINK_FREE_SV_BLKD()) {
		DOWLINK_FREE_SV_BROADCAST();
	}
	DOWLINK_FREE_BADINIT(link);
	DOWLINK_DEBUG_FREEENTER(link);
}

/*
 * void
 * dowlink_addlink(dowlinkid_t newlink, dowlinkid_t headlink)
 *	Add the link, newlink, to the dow link chain anchored
 *	at headlink.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */
void
dowlink_addlink(dowlinkid_t newlink, dowlinkid_t headlink)
{
	ASSERT(VALID_DOWLINK_ID(newlink));
	ASSERT(VALID_DOWLINK_HEAD(headlink));
	DOWLINK_NEXT(newlink) = DOWLINK_NEXT(headlink);
	DOWLINK_PREV(newlink) = headlink;
	DOWLINK_NEXT(headlink) = newlink;
	DOWLINK_PREV(DOWLINK_NEXT(newlink)) = newlink;
	DOWLINK_DEBUG_ENTER(newlink, headlink);
}

/*
 * void
 * dowlink_sublink(dowlinkid_t link)
 * 	Remove the specified link from its link chain.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */
void
dowlink_sublink(dowlinkid_t link)
{
	DOWLINK_NEXT(DOWLINK_PREV(link)) = DOWLINK_NEXT(link);
	DOWLINK_PREV(DOWLINK_NEXT(link)) = DOWLINK_PREV(link);
	DOWLINK_DEBUG_LEAVE(link);
}


/*
 * int
 * dowlink_makelink(dowid_t dep_id, dowid_t ant_id)
 *	Build two linkages: one on the dep_id's antecedent
 *	link chain to represent the dependency on ant_id; the
 *	other, on ant_id's dependent link chain to represent its
 *	antecedence over dep_id. Also adjust the dependent and
 *	antecedent counts for ant_id and dep_id respectively.
 *
 *	Return appropriate error code if the creation of the dependence
 *	link causes maximum dependent/antecdent counts to be exceeded.
 *	Otherwise return 0.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */
int
dowlink_makelink(dowid_t dep_id, dowid_t ant_id)
{
	dowlinkid_t dlink, alink, dep_head, ant_head;

	ASSERT(DOWLINK_AVAIL(2));
	ASSERT(VALID_DOWID(dep_id));
	ASSERT(VALID_DOWID(ant_id));

	if (DOW_DEP_CNT(ant_id) >= DOW_DEP_MAX) {
		return (DOW_MAX_DEPCNT);
	} else if (DOW_ANT_CNT(dep_id) >= DOW_ANT_MAX) {
		return (DOW_MAX_ANTCNT);
	}

	dep_head = DOWLINK_ANT_LINKHEAD(dep_id);
	ant_head = DOWLINK_DEP_LINKHEAD(ant_id);

	dlink = dowlink_alloc();
	alink = dowlink_alloc();

	dowlink_init(alink, dlink, dep_id);
	dowlink_init(dlink, alink, ant_id);

	dowlink_addlink(alink, ant_head);
	dowlink_addlink(dlink, dep_head);
	
	++DOW_DEP_CNT(ant_id);
	++DOW_ANT_CNT(dep_id);

	ASSERT(DOWLINK_ILINK_SANE(dlink));	

#ifdef	DEBUG
	{
	ASSERT(	(dep_id == DOWLINK_DEBUG_MYDOW(dlink)) &&
	       	(ant_id == DOWLINK_DEBUG_MYDOW(alink)) &&
	       	(ant_head == DOWLINK_DEBUG_LHEAD(alink)) &&	
	       	(dep_head == DOWLINK_DEBUG_LHEAD(dlink)) &&
	       	(DOWLINK_DEBUG_CNT(ant_head) == DOW_DEP_CNT(ant_id)) &&
		(DOWLINK_DEBUG_CNT(dep_head) == DOW_ANT_CNT(dep_id)) );
	}
#endif
	return(0);
}

/*
 * void
 * dowlink_breaklink(alink, dependent, antecdent)
 *	Dismantle the linkage between dependent and antecedent ids.
 *		- link exists on antecdent's list
 *		- remove link and its inverse, and free them both
 *	Adjust the dependent and antecedent counts of the dow ids
 *	appropriately.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */
void
dowlink_breaklink(dowlinkid_t alink, dowid_t dep_id, dowid_t ant_id)
{
	dowlinkid_t dlink;

	ASSERT(VALID_DOWID(dep_id));
	ASSERT(VALID_DOWID(ant_id));


	ASSERT(VALID_DOWLINK_ID(alink));
	dlink = DOWLINK_INVERSE(alink);
	ASSERT(VALID_DOWLINK_ID(dlink));
	ASSERT(DOWLINK_ILINK_SANE(dlink));	
	ASSERT(DOWLINK_DOWID(dlink) == ant_id);
	ASSERT(DOWLINK_DOWID(alink) == dep_id);

#ifdef	DEBUG
	{
	dowlinkid_t dep_head = DOWLINK_ANT_LINKHEAD(dep_id);
	dowlinkid_t ant_head = DOWLINK_DEP_LINKHEAD(ant_id);

	ASSERT(	(dep_id == DOWLINK_DEBUG_MYDOW(dlink)) &&
	       	(ant_id == DOWLINK_DEBUG_MYDOW(alink)) &&
	       	(ant_head == DOWLINK_DEBUG_LHEAD(alink)) &&	
	       	(dep_head == DOWLINK_DEBUG_LHEAD(dlink)) &&
	       	(DOWLINK_DEBUG_CNT(ant_head) == DOW_DEP_CNT(ant_id)) &&
		(DOWLINK_DEBUG_CNT(dep_head) == DOW_ANT_CNT(dep_id)) );
	}
#endif
	dowlink_sublink(alink);
	dowlink_sublink(dlink);
	--(DOW_ANT_CNT(dep_id));
	--(DOW_DEP_CNT(ant_id));
	dowlink_free(alink);
	dowlink_free(dlink);
}

/*
 * int
 * linkage_detect(dowid_t dep_id, dowid_t ant_id)
 *	Return indication of whether: (a) a dependence link exists between
 *	dep_id and ant_id, or (b) a dependence link exists the other way,
 *	or, (c) there exists the possibility of a transitive dependence
 *	between the ant_id and dep_id (i.e., whether the introduction of
 *	the proposed link potentially results in a cycle), or (d) no link
 *	exists and no possibility of a cycle exists either, so the proposed
 *	link can be created.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */
 
int
linkage_detect(dowid_t dep_id, dowid_t ant_id)
{
	dowlinkid_t link, dep_ant_lhead, dep_dep_lhead;

	/*
	 * first, we search the antecdent list of the dependent to see
	 * whether the desired link exists already (if so, return 0).
	 */

	link = dep_ant_lhead = DOWLINK_ANT_LINKHEAD(dep_id);

	ASSERT((DOW_ANT_CNT(dep_id) == 0) || 
	       !EMPTY_DOWLINK_LIST(dep_ant_lhead));
	ASSERT((DOW_ANT_CNT(dep_id) >  0) || 
	        EMPTY_DOWLINK_LIST(dep_ant_lhead));
	CHECK_DOWLINK_CHAIN(dep_ant_lhead);

	while ((link = DOWLINK_NEXT(link)) != dep_ant_lhead) {

		if (DOWLINK_DOWID(link) == ant_id) {
			CHECK_DOW_LINKAGE(link);
			return 0;
		}
	}

	/* 
	 * the desired link does not yet exist. check whether it can be
	 * created.
	 */
	if (DOW_DEP_CNT(dep_id) == 0 || DOW_ANT_CNT(ant_id) == 0) {
		/*
		 * there is no possibility of a cycle.
		 */
		return (DOW_CREATE_OK);
	}

	/*
	 * At this point, we can either return an indication that a cycle
	 * is possible; or, check a little further to see whether a reverse
	 * link exists (in which case a cycle would be definite). This is
	 * largely a performance issue: we want to decide which of the two
	 * to flush, and knowing that ant_id definitely depends on dep_id
	 * tilts the bias towards flushing dep_id.
	 */

	link = dep_dep_lhead = DOWLINK_DEP_LINKHEAD(dep_id);
	ASSERT(!EMPTY_DOWLINK_LIST(dep_dep_lhead));
	CHECK_DOWLINK_CHAIN(dep_dep_lhead);

	while ((link = DOWLINK_NEXT(link)) != dep_dep_lhead) {

		if (DOWLINK_DOWID(link) == ant_id) {
			CHECK_DOW_LINKAGE(link);
			return DOW_DEF_CYCLE;
		}
	}

	return DOW_POS_CYCLE;
}
