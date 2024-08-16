/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/dow_check.c	1.2"
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

#ifdef	DEBUG

/*
 * void
 * check_antecdents_pruned(dowid_t id)
 * 	Verify that all antecdents of id are pruned.
 *
 * Calling/Exit State:
 *	Dow mutex held by caller.
 */

void
check_antecdents_pruned(dowid_t id)
{

	dowlinkid_t ant_lhead, link;
	dowid_t ant_id;
	
	link = ant_lhead = DOWLINK_ANT_LINKHEAD(id);

	ASSERT(VALID_DOWLINK_HEAD(link));
	ASSERT(DOWLINK_LINKS_SANE(link));

	while ((link = DOWLINK_NEXT(link)) != ant_lhead) {
		ASSERT(VALID_DOWLINK_ID(link));
		ant_id = DOWLINK_DOWID(link);
		ASSERT(VALID_DOWID(ant_id));
		ASSERT(DOW_STATE(ant_id) & (DOW_PRUNE|DOW_INTRANS));
	}
	return;
}

/*
 * void
 * check_flush_chain(dowid_t flush_header)
 *	Debugging support to verify that indicated flush chain 
 *	(headed by "flush_header") is sane and has the right 
 *	number of items on it.
 *
 * Calling/Exit State:
 *	dow_mutex is held by caller.
 */

void
check_flush_chain(dowid_t flush_header)
{
	int count = 0;
	uchar_t prune_state = 0;
	dowid_t id;

	ASSERT(DOW_MUTEX_OWNED());

	ASSERT( (flush_header == DOW_LEAF_FLUSHHEAD) ||
		(flush_header == DOW_PRUN_FLUSHHEAD) ||
		(flush_header == DOW_AGED_FLUSHHEAD) );

	count = DOW_DEBUG_CNT(flush_header);

	if (flush_header != DOW_AGED_FLUSHHEAD) {
		prune_state = DOW_PRUNE;
	}

	for (id = DOW_FLUSHNEXT(flush_header); id != flush_header;
		id = DOW_FLUSHNEXT(id)) {
		ASSERT((--count) >= 0);
		ASSERT(DOW_FLUSHLINKS_SANE(id));
		ASSERT((DOW_STATE(id) & DOW_PRUNE) == prune_state);
		ASSERT(DOW_DEBUG_FLUSHID(id) == flush_header);
	}
	ASSERT(count == 0);
}

/*
 * void
 * check_on_flushchain(dowid_t x, dowid_t flush_header) 
 *	Debugging support to verify that indicated dow, x, is on
 *	the specified flush chain (headed by "flush_header").
 *
 * Calling/Exit State:
 *	dow_mutex is held by caller.
 */

void
check_on_flushchain(dowid_t x, dowid_t flush_header)
{
	dowid_t id;

	ASSERT(	(flush_header == DOW_LEAF_FLUSHHEAD) ||
		(flush_header == DOW_PRUN_FLUSHHEAD) ||
		(flush_header == DOW_AGED_FLUSHHEAD) );

	if (flush_header == DOW_AGED_FLUSHHEAD) {
		ASSERT((DOW_STATE(x) & DOW_PRUNE) == 0);
	} else {
		ASSERT((DOW_STATE(x) & DOW_PRUNE) != 0);
	}
	ASSERT(DOW_DEBUG_FLUSHID(x) == flush_header);
	for (id = DOW_FLUSHNEXT(flush_header); id != flush_header;
		id = DOW_FLUSHNEXT(id)) {
		ASSERT(VALID_DOWID(id));
		if (id == x) {
			return;
		}
	}
	/*
	 *+ a dow structure was not found on the proper flush chain.
	 *+ kernel software error.
	 */
	if (flush_header == DOW_LEAF_FLUSHHEAD) {
		cmn_err(CE_PANIC, 
			"dowid %d not found on leaf flush chain\n", x);
	} else if (flush_header == DOW_PRUN_FLUSHHEAD) {
		cmn_err(CE_PANIC, 
			"dowid %d not found on pruned flush chain\n", x);
	} else {
		cmn_err(CE_PANIC, 
			"dowid %d not found on aged flush chain\n", x);
	}
	
}

/*
 * void
 * check_noton_flushchain(dowid_t flush_header, dowid_t x) 
 *	Debugging support to verify that indicated dow, x, is not on
 *	the specified flush chain (headed by "flush_header").
 *
 * Calling/Exit State:
 *	dow_mutex is held by caller.
 */

void
check_noton_flushchain(dowid_t x, dowid_t flush_header)
{
	dowid_t id;

	ASSERT(	(flush_header == DOW_LEAF_FLUSHHEAD) ||
		(flush_header == DOW_PRUN_FLUSHHEAD) ||
		(flush_header == DOW_AGED_FLUSHHEAD) );

	ASSERT(DOW_DEBUG_FLUSHID(x) != flush_header);

	for (id = DOW_FLUSHNEXT(flush_header); id != flush_header;
		id = DOW_FLUSHNEXT(id)) {
		ASSERT(id != x);
	}
}

/*
 * void
 * check_noton_any_flushchain(dowid_t x) 
 *	Debugging support to verify that indicated dow, x, is not on
 *	any flush chain.
 *
 * Calling/Exit State:
 *	dow_mutex is held by caller.
 */

void
check_noton_any_flushchain(dowid_t x)
{
	ASSERT(DOW_DEBUG_FLUSHID(x) == DOW_BADID);
}

/*
 * boolean_t
 * on_some_flushchain(dowid_t id)
 *	return true if id is on one of the flush chains, false otherwise.
 *
 * Calling/Exit State:
 *	dow_mutex is held by caller.
 */
boolean_t
on_some_flushchain(dowid_t id)
{
	dowid_t fh;

	fh = DOW_DEBUG_FLUSHID(id);

	if ((fh == DOW_AGED_FLUSHHEAD) ||
	    (fh == DOW_PRUN_FLUSHHEAD) ||
	    (fh == DOW_LEAF_FLUSHHEAD)) {
		CHECK_ON_FLUSHCHAIN(id, fh);
		return(B_TRUE);
	}

	ASSERT(!VALID_DOWID(fh));
	return(B_FALSE);
}

/*
 * boolean_t
 * dow_exists(long ident1, long ident2)
 *	Return B_TRUE if the dow of the specified identity exists,
 *	B_FALSE otherwise.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Answer stale unless higher level serialization ensures
 *	otherwise.
 */
boolean_t
dow_exists(long ident1, long ident2)
{
	dowid_t	hashid;
	dowid_t i;
	boolean_t retval = B_FALSE;
	
	DOW_MUTEX_LOCK();
	i = hashid = DOW_HASHBUCKET(ident1, ident2);
	ASSERT(DOW_HASHLINKS_SANE(hashid));
	ASSERT(VALID_HASH_BUCKET(hashid));
	while ((i = DOW_HASHNEXT(i)) != hashid) {
		if ((DOW_IDENT1(i) == ident1) && (DOW_IDENT2(i) == ident2)) {
			ASSERT((DOW_STATE(i) & DOW_GONE) == 0);
			retval = B_TRUE;
			break;
		}
	}
	DOW_MUTEX_UNLOCK();
	return (retval);
}


/*
 * dowid_t
 * dow_vp_any(vnode_t *vp)
 *	Return a dowid if the dow has a page identity in the specified vp.
 *	Return DOW_BADID otherwise.
 *
 * Calling/Exit State:
 *	None.
 *
 * Remarks:
 *	Answer stale unless higher level serialization ensures
 *	otherwise.  NO HOLD IS PLACED ON RETURNED id.
 */
dowid_t
dow_vp_any(vnode_t *vp)
{
	dowid_t	hashid;
	dowid_t id;
	
	DOW_MUTEX_LOCK();

	for (hashid = DOW_TABLE_SIZE;
	     hashid < (DOW_TABLE_SIZE + DOW_HASHWIDTH); hashid++) {

		ASSERT(DOW_HASHLINKS_SANE(hashid));
		for (id = DOW_HASHNEXT(hashid); id != hashid;
			id = DOW_HASHNEXT(id)) {

			if ((DOW_IDENT1(id) == (long)vp) &&
			    (DOW_TYPE(id) == DOW_ISPAGE)) {
				DOW_MUTEX_UNLOCK();
				return(id);
			}
		}
	}
	DOW_MUTEX_UNLOCK();
	return (DOW_BADID);
}



#ifndef	DOW_KLUDGE
void
mark_vp_offset_dows(vnode_t *vp, off_t offset, uchar_t marker)
{
	dowid_t	hashid;
	dowid_t id;
	
	DOW_MUTEX_LOCK();

	for (hashid = DOW_TABLE_SIZE;
	     hashid < (DOW_TABLE_SIZE + DOW_HASHWIDTH); hashid++) {

		ASSERT(DOW_HASHLINKS_SANE(hashid));
		for (id = DOW_HASHNEXT(hashid); id != hashid;
			id = DOW_HASHNEXT(id)) {

			if ((DOW_IDENT1(id) == (long)vp) &&
			    (DOW_TYPE(id) == DOW_ISPAGE) &&
			    (DOW_IDENT2(id) >= offset)) {
				DOW_SPARE(id) = marker;
			}
		}
	}
	DOW_MUTEX_UNLOCK();
	return;
}

void
check_vp_offset_dows(vnode_t *vp, off_t offset)
{
	dowid_t	hashid;
	dowid_t id;
	
	DOW_MUTEX_LOCK();
	for (hashid = DOW_TABLE_SIZE;
	     hashid < (DOW_TABLE_SIZE + DOW_HASHWIDTH); hashid++) {

		ASSERT(DOW_HASHLINKS_SANE(hashid));
		for (id = DOW_HASHNEXT(hashid); id != hashid;
			id = DOW_HASHNEXT(id)) {
			if ((DOW_IDENT1(id) == (long)vp) &&
			    (DOW_IDENT2(id) >= offset)) {
				if (DOW_SPARE(id) & DOW_TRACKED) {
					cmn_err(CE_CONT,
					"CHECK_VP_OFFSET_DOWS: found tracked dow 0x%x\n", id);
				} else {
					cmn_err(CE_CONT,
					"CHECK_VP_OFFSET_DOWS: found untracked dow 0x%x\n", id);
				}
				call_demon();
			}
		}
	}
	DOW_MUTEX_UNLOCK();
	return;
}

#endif

/*
 * void
 * check_dow_freelist()
 *	Verify that the freelist of DOW structures is consistent.
 *
 * Calling/Exit State:
 *	DOW mutex held by caller.
 */

void
check_dow_freelist()
{
	dowid_t x = DOW_FREEHEAD;
	int count = 0;

	ASSERT(DOW_FREENEXT(DOW_FREEPREV(x)) == x);
	while ((x = DOW_FREENEXT(x)) != DOW_FREEHEAD) {
		ASSERT(DOW_FREEPREV(DOW_FREENEXT(x)) == x);
		++count;
		ASSERT(count <= dow_freecount);
	}
	ASSERT(DOW_FREEPREV(DOW_FREENEXT(x)) == x);
	ASSERT(count == dow_freecount);

}

/*
 * void
 * check_notfree(dowid_t id)
 *	Verify that id is not on the freelist.
 *
 * Calling/Exit State:
 *	DOW mutex held by caller.
 */

void
check_notfree(dowid_t id)
{
	ASSERT(DOW_DEBUG_HASHID(id) != DOW_FREEHEAD);
	ASSERT(DOW_DEBUG_FLUSHID(id) != DOW_FREEHEAD);
}

/*
 * void
 * check_hashdup(dowid_t x, dowid_t hb, long i1, long i2)
 *	Verify that dowid x is on hash chain hb, and that no 
 *	synonym of x exists on the hash chain. <i1, i2> is the
 *	identity tuple for dowid x.
 *
 * Calling/Exit State:
 *	dow_mutex held by caller.
 */
void
check_hashdup(dowid_t x, dowid_t hb, long i1, long i2)
{
	int count = 0;
	dowid_t	y;

	for (y = DOW_HASHNEXT(hb); y != hb; y = DOW_HASHNEXT(y)) {
		if (y == x) {
			ASSERT(count == 0);
			++count;
		} else {
			ASSERT((i1 != (long)DOW_IDENT1(y)) || 
			       (i2 != (long)DOW_IDENT2(y)));
		}
	}
	ASSERT(count == 1);
}

/*
 * void
 * check_hash(dowid_t id)
 *	Verify that id is on the correct hash chain.
 *
 * Calling/Exit State:
 *	DOW mutex held by caller.
 */
void
check_hash(dowid_t id)
{
	dowid_t hbucket;
	hbucket = DOW_HASHBUCKET(DOW_IDENT1(id), DOW_IDENT2(id));
	ASSERT(DOW_DEBUG_HASHID(id) == hbucket);
	ASSERT((DOW_STATE(id) & DOW_GONE) == 0);
	CHECK_HASHDUP(id, hbucket, DOW_IDENT1(id), DOW_IDENT2(id));
}

/*
 * void
 * check_hash_chain(dowid_t hb)
 *	Verify that the hash chain at bucket hb is consistent.
 *
 * Calling/Exit State:
 *	DOW mutex held by caller.
 */

void
check_hash_chain(dowid_t hb)
{
	/*
	 * count up the number of items and verify that they
	 * are correct, 
	 * check sanity of links,
	 * check that hashids match
	 */

	dowid_t x;
	int count = 0;
	ASSERT(DOW_HASHLINKS_SANE(hb));
	for (x = DOW_HASHNEXT(hb); x != hb ; x = DOW_HASHNEXT(x)) {
		ASSERT(DOW_HASHLINKS_SANE(x));
		++count;
		ASSERT(count <= DOW_DEBUG_CNT(hb));
		ASSERT((DOW_STATE(x) & DOW_GONE) == 0);
		ASSERT(DOW_DEBUG_HASHID(x) == hb);
	}
	ASSERT(count == DOW_DEBUG_CNT(hb));
}

/*
 * void
 * check_hash_removed(dowid_t x)
 *	Verify that dowid x has been unhashed.
 *
 * Calling/Exit State:
 *	DOW mutex held by caller.
 */

void
check_hash_removed(dowid_t x)
{
	ASSERT(DOW_DEBUG_HASHID(x) == DOW_BADID);
}

/*
 * void
 * check_dowlink_chain(dowlinkid_t hd)
 *	Verify that the chain of dowlink structures anchored at
 *	headlink hd is consistent.
 *
 * Calling/Exit State:
 *	Caller holds dow_mutex.
 */

void
check_dowlink_chain(dowlinkid_t hd)
{
	dowlinkid_t link;
	int count = 0;
	ASSERT(VALID_DOWLINK_HEAD(hd));
	ASSERT(DOWLINK_LINKS_SANE(hd));
	for (link = DOWLINK_NEXT(hd); link != hd; link = DOWLINK_NEXT(link)) {
		ASSERT(DOWLINK_LINKS_SANE(link));
		++count;
		ASSERT(count <= DOWLINK_DEBUG_CNT(hd));
	}
	ASSERT(count == DOWLINK_DEBUG_CNT(hd));
}

/*
 * void
 * check_dow_linkage(dowlinkid_t my_link)
 *	Verify that the reciprocal linkage between "my_link" and the
 *	inverse thereof is correct. For a link that, for example,
 *	encodes the dependence (X --> Y), the inverse link would
 *	encode the antecdentce (Y <-- X). This function verifies that
 * 	each link correctly identifies both its own dow structure and
 *	the dow structure with which the dependence/antecdence 
 *	relationship.
 *
 * Calling/Exit State:
 *	Caller holds dow_mutex.
 */

void
check_dow_linkage(dowlinkid_t my_link)
{
	dowid_t my_dow, other_dow;
	dowlinkid_t inv_link;

	ASSERT(DOWLINK_ILINK_SANE(my_link));

	inv_link = DOWLINK_INVERSE(my_link);
	my_dow = DOWLINK_DEBUG_MYDOW(my_link);
	other_dow = DOWLINK_DEBUG_MYDOW(inv_link);

	ASSERT(my_dow == DOWLINK_DOWID(inv_link));
	ASSERT(other_dow == DOWLINK_DOWID(my_link));
}

/*
 * void
 * check_dowlink_freelist()
 * 	Verify that the list of free dowlink structures is consistent.
 *
 * Calling/Exit State:
 *	Dow mutex held by caller.
 */

void
check_dowlink_freelist()
{
	dowlinkid_t x = DOWLINK_FREEHEAD;
	int count = 0;

	ASSERT(DOWLINK_LINKS_SANE(x));

	while ((x = DOWLINK_NEXT(x)) != DOWLINK_FREEHEAD) {
		ASSERT(DOWLINK_DEBUG_LHEAD(x) == DOWLINK_FREEHEAD);
		ASSERT(DOWLINK_PREV(DOWLINK_NEXT(x)) == x);
		++count;
		ASSERT(count <= dowlink_freecount);
	}
	ASSERT(DOWLINK_PREV(DOWLINK_NEXT(x)) == x);
	ASSERT(count == dowlink_freecount);

}

/*
 * void
 * print_dow(dowid_t i)
 *	Debugging support routine, expected to be called from the kernel
 * 	debugger (KDB), to print the various fields in the indicated
 *	dow structure. Also prints the antecdent and dependent link
 *	chains, listing the linkid, next-linkid, and the dowid of the
 *	connected antecdent or dependent dows.
 *
 * Calling/Exit State:
 *	None.
 */

void
print_dow(dowid_t i)
{
	dowlinkid_t link, headlink;

	debug_printf("dowid     %6d\taddr   0x%x\n", i, &(dow_tab[i]));
	debug_printf("free/hash:next(prev) %6d(%6d)\t", 
		DOW_HASHNEXT(i),
		DOW_HASHPREV(i));
	debug_printf("flush    :next(prev) %6d(%6d)\n", DOW_FLUSHNEXT(i),
		DOW_FLUSHPREV(i));
	debug_printf("antecdent count      %6d\t", DOW_ANT_CNT(i));
	debug_printf("dependent count      %6d\n", DOW_DEP_CNT(i));
	debug_printf("&dow_sv 0x%x\t", DOW_SVP(i));
	debug_printf("dow state   ");
	if (DOW_STATE(i) & DOW_GONE)
		debug_printf("G");
	else
		debug_printf(" ");
	if (DOW_STATE(i) & DOW_PRUNE)
		debug_printf("P");
	else
		debug_printf(" ");
	if (DOW_STATE(i) & DOW_IOSETUP)
		debug_printf("I");
	else
		debug_printf(" ");
	if (DOW_STATE(i) & DOW_FLUSH)
		debug_printf("F");
	else
		debug_printf(" ");
	if (DOW_STATE(i) & DOW_INTRANS)
		debug_printf("W");
	else
		debug_printf(" ");
	if (DOW_STATE(i) & DOW_MODIFIED)
		debug_printf("M");
	else
		debug_printf(" ");
	debug_printf("\tTYPE: \t");
	if (DOW_TYPE(i) == DOW_ISPAGE)
		debug_printf("page");
	else if (DOW_TYPE(i) == DOW_ISBUFFER)
		debug_printf("buffer");
	else if (DOW_TYPE(i) == DOW_ISFUNC)
		debug_printf("function");
	else if (DOW_TYPE(i) == DOW_ISFUNC_NBLK)
		debug_printf("func+nblk");
	else
		debug_printf("??????");
	debug_printf("\nhold      %6d\t", DOW_HOLD(i));
	debug_printf("IDENT1  0x%8x\t", DOW_IDENT1(i));
	debug_printf("IDENT2  0x%8x\n", DOW_IDENT2(i));
	debug_printf("TimeStamp   %6d\t", DOW_TIMESTAMP(i));
	debug_printf("Buffer ptr 0x%8x\t", DOW_BP(i));
	debug_printf("MOD count   %6d\n", DOW_MOD_LOCK(i));
	debug_printf("DEBUG FLDS: hash(flush) %6d(%6d)\tflush chain: ",
		DOW_DEBUG_HASHID(i),
		DOW_DEBUG_FLUSHID(i));
	if (DOW_DEBUG_FLUSHID(i) == DOW_LEAF_FLUSHHEAD) {
		debug_printf("leaf ");
	}
	else if (DOW_DEBUG_FLUSHID(i) == DOW_PRUN_FLUSHHEAD) {
		debug_printf("prun ");
	}
	else if (DOW_DEBUG_FLUSHID(i) == DOW_AGED_FLUSHHEAD) {
		debug_printf("aged ");
	}
	else if (DOW_DEBUG_FLUSHID(i) == DOW_FREEHEAD) {
		debug_printf("free ");
	}
	else if (DOW_DEBUG_FLUSHID(i) == DOW_BADID) {
		debug_printf("none? ");
	} 
	else {
		debug_printf("???? ");
	}
	debug_printf("\n\n");

	link = headlink = DOWLINK_ANT_LINKHEAD(i);
	debug_printf("antecdents list (curr/next/dow/DLH): %6d %6d %6d %6d\n",
		link,	
		DOWLINK_NEXT(link),
		DOWLINK_DOWID(link),
		DOWLINK_DEBUG_LHEAD(link));
	while ((link = DOWLINK_NEXT(link)) != headlink) {	
	debug_printf("                                    %6d %6d %6d %6d\n",
		link,	
		DOWLINK_NEXT(link),
		DOWLINK_DOWID(link),
		DOWLINK_DEBUG_LHEAD(link));
	} 
	debug_printf("\n");

	link = headlink = DOWLINK_DEP_LINKHEAD(i);
	debug_printf("dependent list (curr/next/dow/DLH): %6d %6d %6d %6d\n",
		link,	
		DOWLINK_NEXT(link),
		DOWLINK_DOWID(link),
		DOWLINK_DEBUG_LHEAD(link));
	while ((link = DOWLINK_NEXT(link)) != headlink) {
	debug_printf("                                    %6d %6d %6d %6d\n",
		link,	
		DOWLINK_NEXT(link),
		DOWLINK_DOWID(link),
		DOWLINK_DEBUG_LHEAD(link));
	}
	debug_printf("\n\n");


}

/*
 * void
 * find_dow(long ident1, long ident2)
 *	Find the hashbucket with the provided hash identity fields,
 *	and print the hashbucket and the dowid (if found on the hash
 *	chain).
 * Calling/Exit State:
 *	None.
 */
void
find_dow(long ident1, long ident2)
{
	dowid_t hb = DOW_HASHBUCKET(ident1, ident2);
	dowid_t i;

	debug_printf("find_dow: hashbucket = %d\t", hb);
	for (i = DOW_HASHNEXT(hb); i != hb; i = DOW_HASHNEXT(i)) {
		if (DOW_IDENT1(i) == ident1 && DOW_IDENT2(i) == ident2) {
			debug_printf("dowid = %d\n", i);
			return;
		}
	}
	debug_printf("\n");
		
}

#endif
