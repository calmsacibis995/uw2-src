/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:util/malloc.c	1.6"
#ident	"$Header: $"

#include <mem/kmem.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ghier.h>
#include <util/inline.h>
#include <util/ipl.h>
#include <util/ksynch.h>
#include <util/map.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Control and synchronization structure for a map.
 */
struct mapctl {
	lock_t	mc_lock;
	uchar_t	mc_flags;
	sv_t	mc_wait;
	size_t	mc_allocsz;
	ulong_t	mc_nempty;
	ulong_t mc_want;	/* count of old sleep style waiters */
};

/*
 * Flag values for mc_flags.
 */
#define MAPALLOC	0x01	/* map was initialized via rmallocmap */

static LKINFO_DECL(mc_lkinfo, "KU:rmalloc:mc_lock", 0);

/*
 * Macros used to access internal fields.
 *	mapctl(struct map *mapp) -- from map ptr, get mapctl ptr
 *	MAPOFFSET -- offset, in struct map units, from map base to first
 *		     useable slot
 *	mapstart(struct map *mapp) -- from map ptr, get ptr to first slot
 *	MAPCTLOFF -- offset, in struct map units, from mapctl to map base;
 *		     this is sizeof(struct mapctl) rounded up and converted
 *		     to struct map units to ensure proper alignment
 */
#define mapctl(mapp)	(*(struct mapctl **)(mapp))
#define MAPOFFSET	((sizeof(struct mapctl *) + sizeof(struct map) - 1) / \
			  sizeof(struct map))
#define mapstart(mapp)	(&(mapp)[MAPOFFSET]) /* first slot after header */
#define MAPCTLOFF	((sizeof(struct mapctl) + sizeof(struct map) - 1) / \
			  sizeof(struct map))

/*
 * struct map *
 * rmallocmap(ulong_t mapsiz)
 *	Allocate and initialize a resource map.
 *
 * Calling/Exit State:
 *	No locks are required on entry and none are held on return.
 *	If possible without blocking, a map table is allocated that is
 *	big enough to hold mapsiz map slots (not including the NULL
 *	terminator), and a pointer to the map table is returned.
 *	If the map structure could not be allocated, NULL is returned.
 *	The map table is initialized to an "empty" state; in order to
 *	use it, rmfree() must be called to provide the initial resource.
 *
 *	The resource map should only be accessed via routines in this
 *	file; the caller should not make any assumptions about the
 *	layout of the data structure.
 */
struct map *
rmallocmap(ulong_t mapsiz)
{
	struct map *mp;
	struct mapctl *mcp;
	size_t size;

	if (mapsiz == 0)
		return NULL;
	/*
	 * Compute needed size; MAPOFFSET slots for the header,
	 * and 1 for the null terminator.  Also add in the mapctl structure
	 * for a single allocation.
	 */
	size = sizeof(struct map) * (mapsiz + MAPOFFSET + 1 + MAPCTLOFF);
	if ((mcp = kmem_zalloc(size, KM_NOSLEEP)) == NULL)
		return NULL;
	mp = (struct map *)mcp + MAPCTLOFF;

	LOCK_INIT(&mcp->mc_lock, HIER_RMALLOC, PLHI, &mc_lkinfo, KM_NOSLEEP);
	SV_INIT(&mcp->mc_wait);
	mcp->mc_flags = MAPALLOC;
	mcp->mc_allocsz = size;
	mcp->mc_nempty = mapsiz;

	mapctl(mp) = mcp;

	return mp;
}
	
/*
 * void
 * _Compat_rminit(struct map *mp, ulong_t mapsiz)
 *	Initialize a resource map allocated by a driver.
 *
 * Calling/Exit State:
 *	No locks are required on entry and none are held on return.
 *	The map table pointed to by mp, which is mapsiz * sizeof(struct map)
 *	bytes long, is initialized to an "empty" state; in order to
 *	use it, rmfree() must be called to provide the initial resource.
 *
 *	The actual number of useable slots will be less than mapsiz,
 *	since some of the space will be used for accounting structures
 *	and one slot will be used for a NULL terminator.
 *
 *	The resource map should only be accessed via routines in this
 *	file; the caller should not make any assumptions about the
 *	layout of the data structure.
 *
 * Remarks:
 * 	This routine is provided for compatibility with the 4.0 DDI/DKI,
 *	and should not be used by any new code.
 */
void
_Compat_rminit(struct map *mp, ulong_t mapsiz)
{
	struct mapctl *mcp;

	/*
	 * Make sure the map is at least the minimum necessary size.
	 */
	if (mapsiz - MAPOFFSET - 1 < 1) {
		/*
		 *+ An old driver called rminit() with a resource map which
		 *+ is too small to hold any resources.  This indicates a
		 *+ bug in the driver.
		 */
		cmn_err(CE_PANIC, "rminit: map too small");
		/* NOTREACHED */
	}
	/*
	 * First try to allocate the map control structure separately,
	 * so we can use as many of the passed-in slots as possible.
	 */
	mcp = kmem_zalloc(sizeof(struct mapctl), KM_NOSLEEP);
	if (mcp == NULL) {
		/*
		 * Couldn't allocate map control structure; try to stick it
		 * inside of the map table (at the end) and sacrifice some
		 * slots.
		 */
		if (mapsiz - MAPOFFSET - MAPCTLOFF - 1 < 1) {
			/*
			 *+ There is not enough space in a driver-allocated
			 *+ resource map to allow room for the necessary
			 *+ accounting/lock structures.  This is a temporary
			 *+ condition caused by lack of immediately available
			 *+ memory.
			 */
			cmn_err(CE_PANIC, "rminit: not enough space in map");
			/* NOTREACHED */
		}
		mcp = (struct mapctl *)(mp + mapsiz - MAPCTLOFF);
		mcp->mc_nempty = mapsiz - MAPOFFSET - MAPCTLOFF - 1;
	} else
		mcp->mc_nempty = mapsiz - MAPOFFSET - 1;

	LOCK_INIT(&mcp->mc_lock, HIER_RMALLOC, PLHI, &mc_lkinfo, KM_NOSLEEP);
	SV_INIT(&mcp->mc_wait);

	mapctl(mp) = mcp;
}

/*
 * void
 * rmfreemap(struct map *mp)
 *	Free a map structure previously allocated via rmallocmap().
 *
 * Calling/Exit State:
 *	No locks are required on entry and none are held on return.
 *	Nobody should currently be blocked on this map.
 */
void
rmfreemap(struct map *mp)
{
	struct mapctl *mcp = mapctl(mp);

	ASSERT(mcp->mc_flags & MAPALLOC);
	ASSERT(!SV_BLKD(&mcp->mc_wait));
	ASSERT(mcp->mc_want == 0);

	LOCK_DEINIT(&mcp->mc_lock);
	kmem_free(mcp, mcp->mc_allocsz);
}

/*
 * ulong_t
 * rmalloc(struct map *mp, size_t size)
 *	Allocate 'size' units from the given map.
 *
 * Calling/Exit State:
 *	No locks are required on entry and none are held on exit.
 *	This routine does not block.
 * 	Returns the base of the allocated space, or 0 if the allocation fails.
 *
 * Description:
 * 	In a map, the addresses are increasing and the
 * 	list is terminated by a 0 size.
 * 	Algorithm is first-fit.
 */
ulong_t
rmalloc(struct map *mp, size_t size)
{
	struct mapctl *mcp = mapctl(mp);
	uint_t a;
	struct map *bp;
	pl_t oldpri;

	if (size == 0)
		return 0;
	oldpri = LOCK(&mcp->mc_lock, PLHI);
	for (bp = mapstart(mp); bp->m_size; bp++) {
		if (bp->m_size >= size) {
			a = bp->m_addr;
			bp->m_addr += size;
			if ((bp->m_size -= size) == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while (((bp-1)->m_size = bp->m_size) != 0);
				mcp->mc_nempty++;
			}
			UNLOCK(&mcp->mc_lock, oldpri);
			return a;
		}
	}
	UNLOCK(&mcp->mc_lock, oldpri);
	return 0;
}

/*
 * ulong_t
 * rmalloc_wait(struct map *mp, size_t size)
 *	Allocate 'size' units from the given map, blocking if necessary.
 *
 * Calling/Exit State:
 *	This routine may block, so no locks may be held on entry.
 * 	Returns the base of the allocated space.
 *
 * Description:
 *	Works the same as rmalloc(), but waits for space if none is available.
 */
ulong_t
rmalloc_wait(struct map *mp, size_t size)
{
	struct mapctl *mcp = mapctl(mp);
	uint_t a;
	struct map *bp;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(getpl() == PLBASE);

	if (size == 0)
		return 0;
	(void) LOCK(&mcp->mc_lock, PLHI);
again:
	for (bp = mapstart(mp); bp->m_size; bp++) {
		if (bp->m_size >= size) {
			a = bp->m_addr;
			bp->m_addr += size;
			if ((bp->m_size -= size) == 0) {
				do {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
				} while (((bp-1)->m_size = bp->m_size) != 0);
				mcp->mc_nempty++;
			}
			UNLOCK(&mcp->mc_lock, PLBASE);
			return a;
		}
	}
	SV_WAIT(&mcp->mc_wait, PRIMED, &mcp->mc_lock);
	(void) LOCK(&mcp->mc_lock, PLHI);
	goto again;
	/* NOTREACHED */
}

/*
 * void
 * rmfree(struct map *mp, size_t size, ulong_t a)
 *	Free space into a resource map.
 *
 * Calling/Exit State:
 *	'Size' units of space starting at "address" 'a' are given back to
 *	the resource map, mp.  This should either be space given out by a
 *	previous rmalloc() or rmalloc_wait(), or should be the initial
 *	resource insertion after the map is initialized.
 *	No locks are required on entry and none are held on exit.
 *	This routine does not block.
 *
 * Description:
 * 	The segment ['a', 'a'+'size') is sorted into the map and coalesced
 * 	with adjacent segments on one or both ends, if possible.
 */
void
rmfree(struct map *mp, size_t size, ulong_t a)
{
	struct mapctl *mcp = mapctl(mp);
	struct map *bp;
	uint_t t;
	pl_t oldpri;
	extern void wakeup(caddr_t);

	if (size == 0)
		return;
	oldpri = LOCK(&mcp->mc_lock, PLHI);
	bp = mapstart(mp);
	for (; bp->m_addr <= a && bp->m_size != 0; bp++);
	if (bp > mapstart(mp) && (bp-1)->m_addr + (bp-1)->m_size == a) {
		(bp-1)->m_size += size;
		if (bp->m_addr) {	/* m_addr==0 end of map table */
			ASSERT(a + size <= bp->m_addr);
			if (a + size == bp->m_addr) { 

				/* compress adjacent map addr entries */
				(bp-1)->m_size += bp->m_size;
				while (bp->m_size != 0) {
					bp++;
					(bp-1)->m_addr = bp->m_addr;
					(bp-1)->m_size = bp->m_size;
				}
				mcp->mc_nempty++;
			}
		}
	} else {
		if (a + size == bp->m_addr && bp->m_size != 0) {
			bp->m_addr -= size;
			bp->m_size += size;
			ASSERT(bp == mapstart(mp) ||
			       (bp-1)->m_addr + (bp-1)->m_size < bp->m_addr);
		} else {
			ASSERT(bp == mapstart(mp) ||
			       (bp-1)->m_addr + (bp-1)->m_size < a);
			ASSERT(bp->m_size == 0 || a + size < bp->m_addr);
			if (mcp->mc_nempty == 0) {
				UNLOCK(&mcp->mc_lock, oldpri);
				/*
				 *+ A resource map has overflowed.  Some of
				 *+ the resource covered by the map is now
				 *+ permanently lost (until the next reboot).
				 */
				cmn_err(CE_WARN,
					"rmfree map overflow %x."
					"  Lost %u item(s) at %u\n",
					mp, size, a);
				UNLOCK(&mcp->mc_lock, oldpri);
				return;
			}
			do {
				t = bp->m_addr;
				bp->m_addr = a;
				a = t;
				t = bp->m_size;
				bp->m_size = size;
				bp++;
			} while ((size = t) != 0);
			mcp->mc_nempty--;
		}
	}
	if (mcp->mc_want != 0) {
		mcp->mc_want = 0;
		UNLOCK(&mcp->mc_lock, oldpri);
		wakeup((caddr_t)mp);
	} else
		UNLOCK(&mcp->mc_lock, oldpri);

	if (SV_BLKD(&mcp->mc_wait))
		SV_BROADCAST(&mcp->mc_wait, 0);
}

/*
 * void
 * _Compat_rmsetwant(struct map *mp)
 *	Indicate that space is wanted in the resource map, mp,
 *	(and that the caller will block on it using sleep((caddr_t)mp, X)).
 *
 * Calling/Exit State:
 *	No locks are required on entry and none are held on exit.
 *	This routine does not block.
 *
 * Remarks:
 * 	This routine is provided for compatibility with the 4.0 DDI/DKI,
 *	and should not be used by any new code.
 *
 *	This interface is inherently not MP-safe since there is a window
 *	between the rmsetwant() call and the sleep().  It is assumed that
 *	any map for which rmsetwant() or rmwant() are called will only
 *	have rmsetwant(), rmwant() or rmfree() called from a driver
 *	(or drivers) bound to a single engine, to avoid the race with rmfree().
 */
void
_Compat_rmsetwant(struct map *mp)
{
	struct mapctl *mcp = mapctl(mp);
	pl_t oldpri;

	oldpri = LOCK(&mcp->mc_lock, PLHI);
	mcp->mc_want++;
	UNLOCK(&mcp->mc_lock, oldpri);
}

/*
 * ulong_t
 * _Compat_rmwant(struct map *mp)
 *	Return the number of contexts wanting space from the given
 *	resource map (as indicated by rmsetwant()).
 *
 * Calling/Exit State:
 *	No locks are required on entry and none are held on exit.
 *	This routine does not block.
 *
 * Remarks:
 * 	This routine is provided for compatibility with the 4.0 DDI/DKI,
 *	and should not be used by any new code.
 *
 *	This interface is inherently not MP-safe since there is a window
 *	between the rmsetwant() call and the sleep().  It is assumed that
 *	any map for which rmsetwant() or rmwant() are called will only
 *	have rmsetwant(), rmwant() or rmfree() called from a driver
 *	(or drivers) bound to a single engine, to avoid the race with rmfree().
 */
ulong_t
_Compat_rmwant(struct map *mp)
{
	return mapctl(mp)->mc_want;
}
