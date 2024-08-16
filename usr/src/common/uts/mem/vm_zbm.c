/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/vm_zbm.c	1.3"
#ident	"$Header: $"

/*
 * VM - Zoned Bit Map (ZBM) allocation routines
 */

#include <mem/hat.h>
#include <mem/hatstatic.h>
#include <mem/kmem.h>
#include <mem/mem_hier.h>
#include <mem/zbm.h>
#include <proc/user.h>
#include <util/bitmasks.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/types.h>

/*
 * miscellaneous constants
 *
 *	ZBM_BIG_PRCNT and ZBM_FREE_PRCNT are allocation control thresholds,
 *	given as a percentage of the free space in a cell.
 *
 *	request size > ZBM_BIG_PRCNT of cell => attempt to allocate from the
 *						cells is skipped (go directly
 *						to big bitmap)
 *	cell free space > ZBM_FREE_PRCNT => cell is a candidate for the
 *					    reserve list
 */

#define	ZBM_BIG_PRCNT		25		/* Classify request as "big" */
#define	ZBM_FREE_PRCNT		50		/* Classify cell as "free" */
#define ZBM_MINCELLS		16		/* min # of cells required */
#define ZBM_CELLSIZE		(32 * NBITPW)	/* recommended cellsize */

/*
 * ZBM allocates space from a bitmap. The bitmap is divided up into
 * a number of equal sized zones called "cells". Consequently,
 * there is a linear relationship between (i) cell address,
 * (ii) bitmap address, and (iii) virtual allocation address.
 *
 * Each cell carries a TLB cookie, an allocation cursor, plus
 * a free count.
 *
 * Cells with sufficiently large free space are placed on the reserve
 * list.  Of these, those with the oldest shootdown cookies can get
 * shotdown, and make their way into the free pool.  It is from
 * the free pool that allocation typically takes places.
 */

#ifdef DEBUG

/*
 * macros to increment/decrement stats counters
 */
#define	ZBM_INC_STAT(field)	{ ++zbmp->zbm_stats.field; }
#define	ZBM_DEC_STAT(field)	{ --zbmp->zbm_stats.field; }
#define	ZBM_ADD_STAT(field, n)	{ zbmp->zbm_stats.field += (n); }

#define ZBM AUDIT_DEBUG	1			/* audit on */

#ifdef ZBM_AUDIT_DEBUG
#define	ZBM_AUDIT()	zbm_audit()
#else
#define	ZBM_AUDIT()
#endif

#else	/* !DEBUG */

#define	ZBM_INC_STAT(field)
#define	ZBM_ADD_STAT(field, value)
#define	ZBM_DEC_STAT(field)
#define	ZBM_AUDIT()

#endif	/* !DEBUG */

/*
 *+ Per-zbm structure spin lock protecting all Zoned Bit Map data.
 */
STATIC LKINFO_DECL(zbm_lkinfo, "mem: zbm_lock: zbm structure lock", 0);

/*
 * Declarations: internal routines
 */

STATIC void zbm_add_reserve(zbm_t *, struct zbm_cell *, int);
STATIC void zbm_del_reserve(struct zbm_cell *);
STATIC vaddr_t zbm_alloc_from(zbm_t *, struct zbm_cell *, ulong_t);

#ifdef DEBUG
static int zbm_nbits(uint_t *, int);
static void zbm_print_num(int, const char *);
static void zbm_print_decimal(ulong_t, ulong_t, const char *);
#endif /* DEBUG */

/*
 * macros for manipulating bit numbers
 *
 *	ZBM_WDOFF	Bit number in map to bit number in a word
 *	ZBM_NWORDS	Number of bits to number of words (truncating)
 *	ZBM_NBITS	Number of words to number of bits
 */
#define	ZBM_WDOFF(bitno)	((bitno) % NBITPW)
#define	ZBM_NWORDS(bitno)	((bitno) / NBITPW)
#define	ZBM_NBITS(words)	((words) * NBITPW)

/*
 *  macros for converting to/from virtual address
 *
 *	ZBM_CELLTOVA(i, cr, bn)	cell to virtual address
 *		i	= cell index
 *		cr	= cell cursor
 *		bn	= bit number in word
 *	ZBM_MAPTOVA(mi, wbn)	map to virtual address
 *		mi	= map index
 *		wbn	= bit number in word
 *	ZBM_VATOBN	virtual address to bitmap bit number
 */
#define	ZBM_CELLTOVA(i, cr, bn)	(zbmp->zbm_base + ptob(ZBM_NBITS(cr + \
				((i) << zbmp->zbm_cwshft)) + (bn)))
#define	ZBM_MAPTOVA(mi, wbn)	(zbmp->zbm_base + ptob(ZBM_NBITS(mi) + (wbn)))
#define	ZBM_VATOBN(va)		(btop((va) - zbmp->zbm_base))

/*
 * macros for converting among cell pointer, cell indicies, bitmap
 * indicies, and bitmap pointer
 *
 *	ZBM_CPTOCI		cell pointer to cell index
 * 	ZBM_MITOMP		map index to map pointer
 * 	ZBM_MITOCI		map index to cell index
 *	ZBM_CITOCP		cell index to cell pointer
 *	ZBM_MITOCP		map index to cell pointer
 *	ZBM_CITOMP		cell index to map pointer
 *	ZBM_MITOCURSOR		map index to cell cursor
 */
#define	ZBM_CPTOCI(cp)		((cp) - zbmp->zbm_cell)
#define	ZBM_MITOMP(mi)		(zbmp->zbm_bitmap + (mi))
#define	ZBM_MITOCI(mi)		((mi) >> zbmp->zbm_cwshft)
#define	ZBM_CITOCP(ci)		(zbmp->zbm_cell + (ci))
#define	ZBM_MITOCP(mi)		ZBM_CITOCP(ZBM_MITOCI(mi))
#define	ZBM_CITOMP(ci)		ZBM_MITOMP((ci) << zbmp->zbm_cwshft)
#define	ZBM_MITOCURSOR(mi)	((mi) & zbmp->zbm_cellmask)


#ifdef DEBUG

/*
 * AUDIT style DEBUG code
 */

/*
 * int
 * zbm_nbits(uint_t *mp, int n)
 *	Debug routine to count the number of bits set in a bitmap.
 *
 * Calling/Exit State:
 *	Returns number of set bits.
 */

static int
zbm_nbits(uint_t *mp, int n)
{
	int count = 0;
	int i;

	while (n > 0) {
		for (i = 0; i < NBITPW; ++i) {
			if (BITMASK1_TEST1(mp, i)) {
				++count;
			}
		}
		++mp;
		--n;
	}

	return (count);
}

/*
 * void
 * zbm_audit(zbm_t *zbmp)
 *	This heavy duty routine should be run in a debug mode in which
 *	performance is of no consequence.
 *
 * Calling/Exit State:
 *	- called with zbmp->zbm_lock unlocked
 *	- returns with zbmp->zbm_lock unlocked
 *
 * Description:
 *	The following assertions are checked:
 *
 * 		1. The number of the set bits == zbmp->zbm_mapsize - the sum of
 *		   zbc_nfree.
 * 		2. For each cell, the number of set bits in the map is
 * 		   equal to zbmp->zbm_cellsize - zbc_nfree.
 * 		3. For each cell on the reserve list, zbc_state is
 * 		   ZBC_RESERVE, ZBC_AGING, or ZBC_FREE_AGING.
 * 		4. For each cell not on the reserve list, the state is
 * 		   ZBC_TLBC, ZBC_TLBI, ZBC_FREE_TLBC, or ZBC_FREE_TLBI.
 * 		5. Every cell in the free pool is in state
 *		   ZBC_FREE_TLBC, ZBC_FREE_TLBI, or ZBC_FREE_AGING.
 * 		6. The count of cells on the reserve list + the count of
 * 		   all cells not on the reserve list == zbmp->zbm_ncell.
 * 		7. The count of all cells in the free pool + the count of
 * 		   all cells not in the free pool == zbmp->zbm_ncell.
 *		8. Each cell cursor is constrained to the cell.
 *		9. The bitmap cursor is constrained to the bitmap.
 *		10. The free pool cursor is constrained to the free pool
 */

void
zbm_audit(zbm_t *zbmp)
{
	pl_t ospl;
	int totalcount, cellcount, i;
	struct zbm_cell *cellp, *lastp;
	int rvcount, nrvcount;
	int freecount, nfreecount;

	ospl = LOCK(&zbmp->zbm_lock, VM_ZBM_IPL);

	/*
	 *	1. The number of the set bits == zbmp->zbm_mapsize - the sum of
	 *	   zbc_nfree.
	 */
	totalcount = zbm_nbits(zbmp->zbm_bitmap, zbmp->zbm_mapwidth);
	cellcount = 0;
	cellp = zbmp->zbm_cell;
	for (i = 0; i < zbmp->zbm_ncell; ++i) {
		cellcount += cellp->zbc_nfree;
		++cellp;
	}
	ASSERT(zbmp->zbm_mapsize - cellcount == totalcount);

	/*
	 *	3. For each cell on the reserve list, zbc_state is
	 *	   ZBC_RESERVE, ZBC_AGING, or ZBC_FREE_AGING
	 */
	lastp = &zbmp->zbm_reserve;
	cellp = zbmp->zbm_reserve.zbc_flink;
	rvcount = 0;
	while (cellp != &zbmp->zbm_reserve) {
		++rvcount;
		ASSERT(rvcount <= zbmp->zbm_ncell);
		switch(cellp->zbc_state) {
		case ZBC_RESERVE:
		case ZBC_AGING:
		case ZBC_FREE_AGING:
			break;
		default:
			cmn_err(CE_PANIC, "zbm_audit: reserve state");
		}
		ASSERT(cellp->zbc_blink == lastp);
		lastp = cellp;
		cellp = cellp->zbc_flink;
	}
	ASSERT(zbmp->zbm_reserve.zbc_blink == lastp);

	/*
	 *	2. For each cell, the number of set bits in the map is
	 *	   equal to zbmp->zbm_cellsize - zbc_nfree.
	 *	4. For each cell not on the reserve list, the state is
	 *	   ZBC_TLBC, ZBC_TLBI, ZBC_FREE_TLBC, or ZBC_FREE_TLBI.
 	 *	8. Each cell cursor is constrained to the cell.
	 */
	cellp = zbmp->zbm_cell;
	nrvcount = nfreecount = 0;
	for (i = 0; i < zbmp->zbm_ncell; ++i) {
		cellcount = zbm_nbits(ZBM_CITOMP(i), zbmp->zbm_cellwidth);
		ASSERT(cellcount == zbmp->zbm_cellsize - cellp->zbc_nfree);

		switch (cellp->zbc_state) {
		case ZBC_TLBC:
		case ZBC_TLBI:
		case ZBC_FREE_TLBC:
		case ZBC_FREE_TLBI:
			++nrvcount;
			ASSERT(cellp->zbc_flink == cellp);
			ASSERT(cellp->zbc_blink == cellp);
			break;

		case ZBC_RESERVE:
		case ZBC_AGING:
		case ZBC_FREE_AGING:
			ASSERT(cellp->zbc_flink != cellp);
			ASSERT(cellp->zbc_blink != cellp);
			break;
		default:
			cmn_err(CE_PANIC, "zbm_audit: cell state");
		}

		switch (cellp->zbc_state) {
		case ZBC_TLBC:
		case ZBC_TLBI:
		case ZBC_RESERVE:
		case ZBC_AGING:
			++nfreecount;
			break;
		default:
			break;
		}
		ASSERT((int)cellp->zbc_cursor <= zbmp->zbm_cellwidth);

		++cellp;
	}

	/*
	 *	5. Every cell in the free pool is in state
	 *	   ZBC_FREE_TLBC, ZBC_FREE_TLBI, or ZBC_FREE_AGING.
	 */
	freecount = 0;
	for (i = 0;  i < ZBM_NPOOL; ++i) {
		if ((cellp = zbmp->zbm_pool[i]) != (struct zbm_cell *)NULL) {
			switch (cellp->zbc_state) {
			case ZBC_FREE_TLBC:
			case ZBC_FREE_TLBI:
			case ZBC_FREE_AGING:
				++freecount;
				break;
			default:
				cmn_err(CE_PANIC, "zbm_audit: free state");
			}
		}
	}

	/*
	 *	6. The count of cells on the reserve list + the count of
	 *	   all cells not on the reserve list == zbmp->zbm_ncell.
	 */
	ASSERT(rvcount + nrvcount== zbmp->zbm_ncell);

	/*
	 *	7. The count of all cells in the free pool + the count of
	 *	   all cells not in the free pool == zbmp->zbm_ncell.
	 */
	ASSERT(freecount + nfreecount == zbmp->zbm_ncell);

	/*
 	 *	9. The bitmap cursor is constrained to the bitmap.
	 */
	ASSERT(zbmp->zbm_bit_cur < zbmp->zbm_mapwidth);

	/*
 	 *	10. The free pool cursor is constrained to the free pool
	 */
	ASSERT(zbmp->zbm_pool_cursor < ZBM_NPOOL);

	UNLOCK(&zbmp->zbm_lock, ospl);
}

#endif /* DEBUG */

/*
 * void
 * zbm_calloc(zbm_t *zbmp, uint_t size, int cellsize)
 *	calloc the zbmp->zbm_cell(s) and the bitmap
 *
 * Calling/Exit State:
 *	- called only once while calloc is still alive during
 *	  system initialization
 *	- size will have been set to an upper bound on the eventual size
 *	  of the space form which we will be allocating
 */

void
zbm_calloc(zbm_t *zbmp, uint_t size, int cellsize)
{
	/*
	 * Record the cell size. This will not change between now and
	 * zbm_init() time. However, segment size may change.
	 */
	zbmp->zbm_cellsize = cellsize;

	/*
	 * allocate the cells
	 *	N.B. Calculation assumes that size has been
	 *	     trimmed back to a multiple of zbmp->zbm_cellsize.
	 */
	zbmp->zbm_cell = calloc(btop(size) / (ulong_t)cellsize *
			 sizeof(struct zbm_cell));

	/*
	 * allocate the bit maps
	 *	N.B. Calculation assumes that zbmp->zbm_cellsize size has been
	 *	     trimmed back to a multiple of NBITPW.
	 */
	zbmp->zbm_bitmap = calloc(ZBM_NWORDS(btop(size)) * sizeof(uint_t));
}

/*
 * void
 * zbm_init(zbm_t *zbmp, vaddr_t base, uint_t size, void (*fail_func)(void))
 *	Final initialization for the zbm_t.
 *	Initializes all fields not already initialized.
 *
 * Calling/Exit State:
 *	- called without any locks held.
 */

void
zbm_init(zbm_t *zbmp, vaddr_t base, uint_t size, void (*fail_func)(void))
{
	struct zbm_cell *cellp;
	int i;

	/*
	 * Record segment base and size.
	 * These values are now permanently fixed.
	 */
	zbmp->zbm_base = base;
	zbmp->zbm_size = size;

	/*
	 * initialize static variables
	 */
	zbmp->zbm_cellwidth = ZBM_NWORDS(zbmp->zbm_cellsize);
	zbmp->zbm_mapsize = btop(zbmp->zbm_size);
	zbmp->zbm_ncell = zbmp->zbm_mapsize / zbmp->zbm_cellsize;
	zbmp->zbm_big = zbmp->zbm_cellsize * ZBM_BIG_PRCNT / 100;
	zbmp->zbm_reservesz = zbmp->zbm_cellsize * ZBM_FREE_PRCNT / 100;
	zbmp->zbm_climit = zbmp->zbm_cellwidth - 1;
	zbmp->zbm_mapwidth = ZBM_NWORDS(zbmp->zbm_mapsize);
	zbmp->zbm_cellmask = zbmp->zbm_cellwidth - 1;
	zbmp->zbm_fail_func = fail_func;

	/*
	 * zbmp->zbm_cwshft = log2(zbmp->zbm_cellwidth)
	 */
	i = 1;
	while (i < zbmp->zbm_cellwidth) {
		++zbmp->zbm_cwshft;
		i <<= 1;
	}

	/*
	 * place all cells on the reserve list
	 *
	 * N.B. The highest address cell ends up on the head of the
	 *	reserve list.  Thus, large request allocation will commence
	 *	at low addresses of the segment, and small request allocation
	 *	will commence at high addresses.
	 */
	zbmp->zbm_reserve.zbc_flink =
		zbmp->zbm_reserve.zbc_blink = &zbmp->zbm_reserve;
	for (i = 0; i < zbmp->zbm_ncell; ++i) {
		cellp = &zbmp->zbm_cell[i];
		cellp->zbc_nfree = (ushort_t)zbmp->zbm_cellsize;
		cellp->zbc_flink = cellp->zbc_blink = cellp;
		zbm_add_reserve(zbmp, cellp, ZBC_RESERVE);
	}

	/*
	 * initialize the bitmap to all clear (all free)
	 *
	 *	N.B. even though calloc clears memory, we can make no
	 *	     assumption about how bitmaps are implemented on any given
	 *	     machine
	 */
	BITMASKN_CLRALL(zbmp->zbm_bitmap, zbmp->zbm_mapwidth);

	/*
	 * initialize zbmp->zbm_lock
	 */
	LOCK_INIT(&zbmp->zbm_lock, VM_ZBM_HIER, VM_ZBM_IPL, &zbm_lkinfo,
		  KM_NOSLEEP);
	SV_INIT(&zbmp->zbm_sv);
}

/*
 * void
 * zbm_add_reserve(zbm_t *zbmp, struct zbm_cell *cellp, int state)
 *	Add a cell to the reserve list (either to the head or the tail
 *	depending upon state).  Set the new state of the cell.
 *
 * Calling/Exit State:
 *	called with zbmp->zbm_lock held
 *	returns with zbmp->zbm_lock held
 *	never blocks
 */

STATIC void
zbm_add_reserve(zbm_t *zbmp, struct zbm_cell *cellp, int state)
{
	struct zbm_cell *next, *last;

	/*
	 * Delete from reserve list if already on it
	 * Note that this operation has no effect if the cell is not
	 * on the reserve list, as the links are always "wrapped back"
	 * when the cell is not on the reserve list.
	 */
	next = cellp->zbc_flink;
	last = cellp->zbc_blink;
	next->zbc_blink = last;
	last->zbc_flink = next;

	/*
	 * Depending upon the state, add to the head or tail of the
	 * reserve list
	 */
	switch (state) {
	case ZBC_RESERVE:
		last = &zbmp->zbm_reserve;
		next = zbmp->zbm_reserve.zbc_flink;
		break;
	case ZBC_AGING:
	case ZBC_FREE_AGING:
		next = &zbmp->zbm_reserve;
		last = zbmp->zbm_reserve.zbc_blink;
		break;
	default:
		/*
		 *+ Internal error in the operating system.  The basic
		 *+ integrity of the operating system may have been
		 *+ compromised by malfunctioning software or hardware.
		 */
		cmn_err(CE_PANIC, "zbm_add_reserve");
		break;
	}
	next->zbc_blink = cellp;
	last->zbc_flink = cellp;
	cellp->zbc_flink = next;
	cellp->zbc_blink = last;

	/*
	 * set the state
	 */
	cellp->zbc_state = (uchar_t)state;

	return;
}

/*
 * void
 * zbm_del_reserve(struct zbm_cell * cellp)
 *	Remove a cell from the reserve list.
 *
 * Calling/Exit State:
 *	called with zbmp->zbm_lock held
 *	returns with zbmp->zbm_lock held
 *	never blocks
 */

STATIC void
zbm_del_reserve(struct zbm_cell * cellp)
{
	struct zbm_cell *next, *last;

	/*
	 * delete the cell from the list
	 */
	next = cellp->zbc_flink;
	last = cellp->zbc_blink;
	next->zbc_blink = last;
	last->zbc_flink = next;

	/*
	 * wrap back the pointers (not for courtesy - this is actually
	 * required for zbm_add_reserve to work)
	 */
	cellp->zbc_flink = cellp;
	cellp->zbc_blink = cellp;

	return;
}

/*
 * vaddr_t
 * zbm_alloc_from(zbm_t *zbmp, struct zbm_cell *cellp, ulong_t npages)
 *	This routine allocates contiguous kernel virtual address of size
 *	ptob(npages) from the requested cell.
 *
 * Calling/Exit State:
 *	called with zbmp->zbm_lock held
 *	returns with zbmp->zbm_lock held
 *	returns the virtual address if sucessful and NULL otherwise
 *	never blocks
 */

STATIC vaddr_t
zbm_alloc_from(zbm_t *zbmp, struct zbm_cell *cellp, ulong_t npages)
{
	vaddr_t vaddr;
	int cursor, celli, remaining, n;
	uint_t *mp;

	ZBM_INC_STAT(zbs_ncells);

	/*
	 * convert cellp to bitmap address
	 */
	celli = ZBM_CPTOCI(cellp);
	cursor = cellp->zbc_cursor;
	mp = ZBM_CITOMP(celli) + cursor;

	/*
	 * scan the bitmap, optimizing for 1 page allocations
	 * if success
	 *	decrement free count
	 *	advance cursor
	 */
	if (npages == 1) {
		if (cursor == zbmp->zbm_cellwidth) {
			return ((vaddr_t)NULL);
		}
		n = BITMASKN_FFCSET(mp, zbmp->zbm_cellwidth - cursor);
		if (n == -1) {
			return ((vaddr_t)NULL);
		}
		ASSERT(cellp->zbc_nfree >= 1);
		--cellp->zbc_nfree;
		cellp->zbc_cursor = cursor + ZBM_NWORDS(n + 1);
	} else {
		remaining = ZBM_NBITS(zbmp->zbm_cellwidth - cursor);
		if (npages > remaining) {
			return ((vaddr_t)NULL);
		}
		n = BITMASKN_ALLOCRANGE(mp, remaining, npages);
		if (n == -1) {
			return ((vaddr_t)NULL);
		}
		ASSERT(cellp->zbc_nfree >= npages);
		cellp->zbc_nfree -= npages;
		cellp->zbc_cursor = cursor + ZBM_NWORDS(n + npages);
	}

	/*
	 * convert to virtual address
	 */
	vaddr = ZBM_CELLTOVA(celli, cursor, n);
	ASSERT(vaddr >= zbmp->zbm_base);
	ASSERT(vaddr - zbmp->zbm_base + ptob(npages) <= zbmp->zbm_size);

	/*
	 * adjust cell state
	 */
	switch (cellp->zbc_state) {
	case ZBC_FREE_TLBC:
	case ZBC_FREE_TLBI:
		break;
	case ZBC_FREE_AGING:
		if ((int)cellp->zbc_nfree <= zbmp->zbm_reservesz) {
			zbm_del_reserve(cellp);
			cellp->zbc_state = ZBC_FREE_TLBI;
		}
		break;
	default:
		/*
		 *+ Internal error in the operating system.  The basic
		 *+ integrity of the operating system may have been
		 *+ compromised by malfunctioning software or hardware.
		 */
		cmn_err(CE_PANIC, "zbm_alloc_from");
	}

	ZBM_ADD_STAT(zbs_nallocated, npages);

	return (vaddr);
}

/*
 * vaddr_t
 * zbm_alloc(zbm_t *zbmp, ulong_t npages, uint_t flag)
 * 	This routine allocates contiguous kernel address of "size"
 *	bytes, but no physical allocation or mapping setup yet.
 *	The address returned is page-aligned kernel vaddr.
 *
 * Calling/Exit State:
 * 	- called with zbmp->zbm_lock unlocked
 *	- callers prepare to block if !NOSLEEP
 * 	- returns with zbmp->zbm_lock unlocked
 *
 * Description:
 *	A three attempt approach is used:
 *		attempt 1: Try to allocate from the free pool.
 *		attempt 2: Try to rebuild the free pool and allocate.
 *		attempt 3: Try to allocate from the entire bitmap.
 *	The first two attempts are skipped for large requests.
 */

vaddr_t
zbm_alloc(zbm_t *zbmp, ulong_t npages, uint_t flag)
{
	pl_t ospl;
	vaddr_t vaddr;
	struct zbm_cell *cellp;
	int i, j, n, remaining, limit;
	TLBScookie_t cookie;
	boolean_t must_shoot;
	int mi;			/* global map index */
	int cursor;		/* cell word number */
	int wbn;		/* word bit number */
	int nbits;		/* pages freed in the current cell */

	ospl = LOCK(&zbmp->zbm_lock, VM_ZBM_IPL);

	ZBM_INC_STAT(zbs_nvm_allocs);
	ZBM_ADD_STAT(zbs_npages, npages);

	ASSERT(npages != 0);

	/*
	 * skip the free pool for large allocations - go directly to the
	 * entire bitmap
	 */
	if (npages < zbmp->zbm_big) {
		ZBM_INC_STAT(zbs_nsmall);
		/*
		 * Attempt 1: try to allocate from the free pool
		 *
		 *	for cell in the free pool
		 *		attempt to allocate
		 *		if the cell might be out of space
		 *			recycle it
		 */
		i = zbmp->zbm_pool_cursor;
		do
		{
			cellp = zbmp->zbm_pool[i];
			if (cellp == (struct zbm_cell *)NULL) {
				if (++i == ZBM_NPOOL) {
					i = 0;
				}
				continue;
			}
			if ((vaddr = zbm_alloc_from(zbmp, cellp, npages)) !=
			    (vaddr_t)NULL) {
				zbmp->zbm_pool_cursor = i;
				UNLOCK(&zbmp->zbm_lock, ospl);
				ZBM_AUDIT();
				return (vaddr);
			}
			if (npages == 1 ||
			    (int)cellp->zbc_cursor >= zbmp->zbm_climit) {
				zbmp->zbm_pool[i] = (struct zbm_cell *)NULL;
				switch (cellp->zbc_state) {
				case ZBC_FREE_TLBC:
					if ((int)cellp->zbc_nfree >=
							zbmp->zbm_reservesz) {
						zbm_add_reserve(zbmp, cellp,
								ZBC_RESERVE);
					} else {
						cellp->zbc_state = ZBC_TLBC;
					}
					break;
				case ZBC_FREE_TLBI:
					cellp->zbc_state = ZBC_TLBI;
					break;
				case ZBC_FREE_AGING:
					cellp->zbc_state = ZBC_AGING;
					break;
				default:
					/*
					 *+ Internal error in the operating
					 *+ system. The basic integrity of the
					 *+ operating system may have been
					 *+ compromised by malfunctioning
					 *+ software or hardware.
					 */
					cmn_err(CE_PANIC,
						"zbm_alloc: free state");
				}
			}
			if (++i == ZBM_NPOOL) {
				i = 0;
			}
		} while (i != zbmp->zbm_pool_cursor);
		ZBM_INC_STAT(zbs_n1fail);

		/*
		 * Attempt 2: try to allocate from a rebuilt free list
		 *	      get new cells from the reserve list
		 */
		must_shoot = B_FALSE;
		i = 0;
		do {
			if (zbmp->zbm_pool[i] != (struct zbm_cell *)NULL) {
				++i;
				continue;
			}
			if ((cellp = zbmp->zbm_reserve.zbc_flink) == &zbmp->zbm_reserve) {
				break;
			}
			j = i; /* remember the current pool cursor */
			switch (cellp->zbc_state) {
			case ZBC_AGING:
				zbmp->zbm_pool[i++] = cellp;
				cookie = cellp->zbc_tlbcookie;
				must_shoot = B_TRUE;
				break;
			case ZBC_RESERVE:
				zbmp->zbm_pool[i++] = cellp;
				break;
			case ZBC_FREE_AGING:
				cookie = cellp->zbc_tlbcookie;
				must_shoot = B_TRUE;
				break;
			default:
				/*
				 *+ Internal error in the operating system.
				 *+ The basic integrity of the operating
				 *+ system may have been compromised by
				 *+ malfunctioning software or hardware.
				 */
				cmn_err(CE_PANIC,
					"zbm_alloc: reserve state");
			}
			zbm_del_reserve(cellp);
			cellp->zbc_state = ZBC_FREE_TLBC;
			cellp->zbc_cursor = 0;
			if ((vaddr = zbm_alloc_from(zbmp, cellp, npages)) !=
			    (vaddr_t)NULL) {
				/*
				 * shoots down all members of the
				 * free list together
				 */
				if (must_shoot == B_TRUE) {
					hat_shootdown(cookie, HAT_HASCOOKIE);
					ZBM_INC_STAT(zbs_nshootdowns);
				}
				zbmp->zbm_pool_cursor = j;
				UNLOCK(&zbmp->zbm_lock, ospl);
				ZBM_AUDIT();
				return (vaddr);
			}
		} while (i < ZBM_NPOOL);
		if (must_shoot) {
			hat_shootdown(cookie, HAT_HASCOOKIE);
			ZBM_INC_STAT(zbs_nshootdowns);
		}
		ZBM_INC_STAT(zbs_n2fail);
	}

	if (npages >= zbmp->zbm_mapsize) {
		/*
		 *+ An attempt occurred to allocate an extent of virtual
		 *+ space from kernel segment which exceeds the total segment
		 *+ size. This may indicate that it is necessary to
		 *+ reconfigure the operating system with a larger segment
		 *+ size (e.g. by increasing SEGKMEM_BYTES or SEGKZF_BYTES).
		 */
		cmn_err(CE_PANIC, "zbm_alloc: ultra-large request");
	}

	/*
	 * Attempt 3: try to allocate from the entire map
	 *
	 *	if successful
	 *		advance the bitmap cursor to the end of the allocation
	 *	else if the client can block
	 *		wait
	 *		try again
	 */
	for (;;) {
		/*
		 * Scan the bitmap from the cursor to the end - but only
		 * enough space remains to satisfy the request.
		 */
		remaining = ZBM_NBITS(zbmp->zbm_mapwidth -
				      zbmp->zbm_bit_cur);
		if (npages <= remaining) {
			n = BITMASKN_ALLOCRANGE(ZBM_MITOMP(zbmp->zbm_bit_cur),
				remaining, npages);
			if (n != -1) {
				mi = zbmp->zbm_bit_cur + ZBM_NWORDS(n);
				zbmp->zbm_bit_cur += ZBM_NWORDS(n + npages);
				break;
			}
		}
		/*
		 * Now scan the map from the beginning to the cursor - but
		 * include enough space following the cursor so as not to
		 * miss any holes. However, be  sure not to scan beyond
		 * the end of the map.
		 */
		if (zbmp->zbm_bit_cur != 0) {
			limit = ZBM_NBITS(zbmp->zbm_bit_cur) + npages - 1;
			if (limit > zbmp->zbm_mapsize) {
				limit = zbmp->zbm_mapsize;
			}
			n = BITMASKN_ALLOCRANGE(zbmp->zbm_bitmap,
						limit, npages);
			if (n != -1) {
				mi = ZBM_NWORDS(n);
				zbmp->zbm_bit_cur = ZBM_NWORDS(n + npages);
				break;
			}
		}
		/*
		 * We're out of virtual addresses.  Call the client specific
		 * failure function.
		 */
		(*zbmp->zbm_fail_func)();
		if (flag & NOSLEEP) {
			ZBM_INC_STAT(zbs_nfail);
			UNLOCK(&zbmp->zbm_lock, ospl);
			ZBM_AUDIT();
			return ((vaddr_t)NULL);
		}
		SV_WAIT(&zbmp->zbm_sv, PRIMEM, &zbmp->zbm_lock);
		(void) LOCK(&zbmp->zbm_lock, VM_ZBM_IPL);
	}
	ZBM_ADD_STAT(zbs_nallocated, npages);

	/*
	 *	Don't leave zbmp->zbm_bit_cur at the map end.
	 */
	if (zbmp->zbm_bit_cur == zbmp->zbm_mapwidth) {
		zbmp->zbm_bit_cur = 0;
	}

	/*
	 *	Compute vaddr.
	 */
	wbn = ZBM_WDOFF(n);
	vaddr = ZBM_MAPTOVA(mi, wbn);
	ASSERT(vaddr >= zbmp->zbm_base);
	ASSERT(vaddr + ptob(npages) <= zbmp->zbm_base + zbmp->zbm_size);

	/*
	 *	Take care of each cell hit by the allocation.
	 */
	cellp = ZBM_MITOCP(mi);
	cursor = ZBM_MITOCURSOR(mi);
	nbits = zbmp->zbm_cellsize - ZBM_NBITS(cursor) - wbn;
	if (nbits > npages) {
		nbits = npages;
	}

	for (;;) {
		/*
		 * adjust cell free count
		 */
		ASSERT((int)cellp->zbc_nfree >= nbits);
		cellp->zbc_nfree -= nbits;

		/*
		 * shootdown cell if required
		 * remove from reserve list if free space has been gobbled
		 * adjust cell state
		 */
		switch (cellp->zbc_state) {
		case ZBC_TLBC:
		case ZBC_FREE_TLBC:
			break;
		case ZBC_TLBI:
			if (cursor < (int)cellp->zbc_cursor) {
				hat_shootdown(cellp->zbc_tlbcookie,
					      HAT_HASCOOKIE);
				ZBM_INC_STAT(zbs_nshootdowns);
				cellp->zbc_state = ZBC_TLBC;
			}
			break;
		case ZBC_RESERVE:
			if ((int)cellp->zbc_nfree <= zbmp->zbm_reservesz) {
				zbm_del_reserve(cellp);
				cellp->zbc_state = ZBC_TLBC;
			}
			break;
		case ZBC_AGING:
			if (cursor < (int)cellp->zbc_cursor) {
				hat_shootdown(cellp->zbc_tlbcookie,
					      HAT_HASCOOKIE);
				ZBM_INC_STAT(zbs_nshootdowns);
				if ((int)cellp->zbc_nfree <=
				    zbmp->zbm_reservesz) {
					zbm_del_reserve(cellp);
					cellp->zbc_state = ZBC_TLBC;
				} else {
					/*
					 * effectively deletes and then
					 * adds cell to reserve list
					 */
					zbm_add_reserve(zbmp, cellp,
							ZBC_RESERVE);
				}
			} else {
				if ((int)cellp->zbc_nfree <=
				    zbmp->zbm_reservesz) {
					zbm_del_reserve(cellp);
					cellp->zbc_state = ZBC_TLBI;
				}
			}
			break;
		case ZBC_FREE_TLBI:
			if (cursor < (int)cellp->zbc_cursor) {
				hat_shootdown(cellp->zbc_tlbcookie,
					      HAT_HASCOOKIE);
				ZBM_INC_STAT(zbs_nshootdowns);
				cellp->zbc_state = ZBC_FREE_TLBC;
			}
			break;
		case ZBC_FREE_AGING:
			if (cursor < (int)cellp->zbc_cursor) {
				hat_shootdown(cellp->zbc_tlbcookie,
					      HAT_HASCOOKIE);
				ZBM_INC_STAT(zbs_nshootdowns);
				zbm_del_reserve(cellp);
				cellp->zbc_state = ZBC_FREE_TLBC;
			} else {
				if ((int)cellp->zbc_nfree <=
				    zbmp->zbm_reservesz) {
					zbm_del_reserve(cellp);
					cellp->zbc_state = ZBC_FREE_TLBI;
				}
			}
			break;
		default:
			/*
			 *+ Internal error in the operating system.  The basic
			 *+ integrity of the operating system may have been
			 *+ compromised by malfunctioning software or hardware.
			 */
			cmn_err(CE_PANIC, "zbm_alloc: allocating state");
		}

                /*
                 * optimize loop exit condition for single cell case
                 */
                if (npages == nbits) {
                        break;
                }

                ++cellp;
                npages -= nbits;
                nbits = (npages < zbmp->zbm_cellsize) ? npages :
							zbmp->zbm_cellsize;
		cursor = 0;
	}

	UNLOCK(&zbmp->zbm_lock, ospl);
	ZBM_AUDIT();
	return (vaddr);
}

/*
 * void
 * zbm_free(zbm_t *zbmp, vaddr_t vaddr, ulong_t npages)
 * 	This function frees a range of kernel virtual heap space
 *
 * Calling/Exit State:
 *	- called and returns with zbmp->zbm_lock unlocked
 *
 * Description:
 *	clear the bits in the bitmap
 *	adjust cell state for the affected cells
 */

void
zbm_free(zbm_t *zbmp, vaddr_t vaddr, ulong_t npages)
{
	pl_t ospl;
	struct zbm_cell *cellp;
	TLBScookie_t cookie;
	int mi;			/* global map index */
	int mbn;		/* global map bit number */
	int cursor;		/* cell cursor following last free word */
	int wbn;		/* word bit number */
	int nbits;		/* pages freed in the current cell */

	ASSERT((vaddr & POFFMASK) == 0);
	ASSERT(vaddr >= zbmp->zbm_base);
	ASSERT(vaddr + ptob(npages) <= zbmp->zbm_base + zbmp->zbm_size);
	ASSERT(npages != 0);

	cookie = hat_getshootcookie();
	hat_statpt_unload(vaddr, npages);

	ospl = LOCK(&zbmp->zbm_lock, VM_ZBM_IPL);

	ZBM_INC_STAT(zbs_nvm_free);

	/*
	 * clear the bits in the bitmap
	 */
	mbn = ZBM_VATOBN(vaddr);
	mi = ZBM_NWORDS(mbn);
	wbn = ZBM_WDOFF(mbn);
	cursor = ZBM_MITOCURSOR(mi);
	if (npages == 1) {
		BITMASK1_CLR1(ZBM_MITOMP(mi), wbn);
		nbits = 1;
		cursor += BITMASK_NWORDS(wbn + 1);
	} else {
		BITMASKN_FREERANGE(zbmp->zbm_bitmap, mbn, npages);
		nbits = zbmp->zbm_cellsize - ZBM_NBITS(cursor) - wbn;
		if (nbits > npages) {
			nbits = npages;
		}
		cursor += BITMASK_NWORDS(wbn + nbits);
	}
	ZBM_ADD_STAT(zbs_nfreed, npages);

	/*
	 * for each cell overlapping the freed address range
	 */
	cellp = ZBM_MITOCP(mi);
	for (;;) {
		/*
		 * adjust cell free count
		 */
		cellp->zbc_nfree += (ushort_t)nbits;

		/*
		 * advance cell cursor
		 */
		if (cursor > (int)cellp->zbc_cursor) {
			cellp->zbc_cursor = (uchar_t)cursor;
		}

		/*
		 * save the TLB shootdown cookie
		 */
		cellp->zbc_tlbcookie = cookie;

		/*
		 * adjust cell state
		 */
		switch (cellp->zbc_state) {
		case ZBC_TLBC:
			if ((int)cellp->zbc_nfree > zbmp->zbm_reservesz) {
				zbm_add_reserve(zbmp, cellp, ZBC_AGING);
			} else {
				cellp->zbc_state = ZBC_TLBI;
			}
			break;
		case ZBC_TLBI:
			if ((int)cellp->zbc_nfree > zbmp->zbm_reservesz) {
				zbm_add_reserve(zbmp, cellp, ZBC_AGING);
			}
			break;
		case ZBC_AGING:
		case ZBC_RESERVE:
			/*
			 * effectively deletes and then
			 * adds cell to reserve list
			 */
			zbm_add_reserve(zbmp, cellp, ZBC_AGING);
			break;
		case ZBC_FREE_TLBI:
		case ZBC_FREE_TLBC:
			if ((int)cellp->zbc_nfree > zbmp->zbm_reservesz) {
				zbm_add_reserve(zbmp, cellp, ZBC_FREE_AGING);
			} else {
				cellp->zbc_state = ZBC_FREE_TLBI;
			}
			break;
		case ZBC_FREE_AGING:
			/*
			 * effectively deletes and then
			 * adds cell to reserve list
			 */
			zbm_add_reserve(zbmp, cellp, ZBC_FREE_AGING);
			break;
		default:
			/*
			 *+ Internal error in the operating system.  The basic
			 *+ integrity of the operating system may have been
			 *+ compromised by malfunctioning software or hardware.
			 */
			cmn_err(CE_PANIC, "zbm_free");
		}

		/*
		 * optimize loop exit condition for single cell case
		 */
		if (npages == nbits) {
			break;
		}

		++cellp;
		npages -= nbits;
		nbits = (npages < zbmp->zbm_cellsize) ? npages :
							zbmp->zbm_cellsize;
		cursor = BITMASK_NWORDS(nbits);
	}

	UNLOCK(&zbmp->zbm_lock, ospl);

	/*
	 * wake up anybody waiting for space
	 */
	if (SV_BLKD(&zbmp->zbm_sv)) {
		SV_BROADCAST(&zbmp->zbm_sv, 0);
	}

	ZBM_AUDIT();

	return;
}

/*
 * int
 * zbm_cell_size(uint_t segsize)
 *	Compute the size of a zbm cell given the size of the virtual
 *	allocation space.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 * 	The priorities for the cell size are:
 *
 *	1. must be a multiple (by a power of 2) of NPITPW
 *	2. should have at least ZBM_MINCELLS allocated
 *	3. should have cells of ZBM_CELLSIZE pages
 */
int
zbm_cell_size(uint_t segsize)
{
	int cellsize, i;

	cellsize = ZBM_CELLSIZE;
	if (btop(segsize) / cellsize < ZBM_MINCELLS) {
		cellsize = btop(segsize) / ZBM_MINCELLS;
	}
	i = NBITPW;
	while (i < cellsize) {
		i *= 2;
	}
	cellsize = (i > cellsize && i > NBITPW) ? i / 2 : i;

	return (cellsize);
}


#if defined(DEBUG) || defined(DEBUG_TOOLS)

/*
 * void
 * print_zbm_cell(const zbm_t *zbmp, const struct zbm_cell *cellp)
 *	Debug routine to print out the contents of a cell
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger or in-kernel test.
 */

void
print_zbm_cell(const zbm_t *zbmp, const struct zbm_cell *cellp)
{
	char *state;
	uint_t *mp;
	int i;

	switch (cellp->zbc_state) {
	case ZBC_TLBC:
		state = "C ";
		break;
	case ZBC_TLBI:
		state = "I ";
		break;
	case ZBC_RESERVE:
		state = "R ";
		break;
	case ZBC_AGING:
		state = "A ";
		break;
	case ZBC_FREE_TLBC:
		state = "F ";
		break;
	case ZBC_FREE_TLBI:
		state = "FC";
		break;
	case ZBC_FREE_AGING:
		state = "FA";
		break;
	default:
		state = "? ";
		break;
	}
	debug_printf("%4d %s%5d%4d", ZBM_CPTOCI(cellp), state,
		     cellp->zbc_nfree, cellp->zbc_cursor);
	mp = ZBM_CITOMP(ZBM_CPTOCI(cellp));
	for (i = 0; i < zbmp->zbm_cellwidth; ++i) {
		if (i > 0 && i % 5 == 0) {
			debug_printf("\n                ");
		}
		debug_printf(" %08x", *mp);
		if (debug_output_aborted())
			return;
		++mp;
	}
	debug_printf("\n");
}

/*
 * void
 * print_zbm(const zbm_t *zbmp)
 * 	This routine prints out internal zbm data structures.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger or in-kernel test.
 */

void
print_zbm(const zbm_t *zbmp)
{
	const struct zbm_cell *cellp;
	int i;
	void print_zbm_cell(const zbm_t *zbmp, const struct zbm_cell *cellp);

	debug_printf("cell st free cur map\n");
	debug_printf("---- -- ---- --- ------------------------------\n");

	/*
	 *	1. Print the reserve list.
	 */
	cellp = zbmp->zbm_reserve.zbc_flink;
	while (cellp != &zbmp->zbm_reserve) {
		print_zbm_cell(zbmp, cellp);
		if (debug_output_aborted())
			return;
		cellp = cellp->zbc_flink;
	}

	/*
	 *	2. Print all other cells.
	 */
	cellp = zbmp->zbm_cell;
	for (i = 0; i < zbmp->zbm_ncell; ++i) {
		switch (cellp->zbc_state) {
		case ZBC_TLBC:
		case ZBC_TLBI:
		case ZBC_FREE_TLBC:
		case ZBC_FREE_TLBI:
			print_zbm_cell(zbmp, cellp);
			if (debug_output_aborted())
				return;
			break;

		default:
			break;
		}
		++cellp;
	}
}

#endif /* DEBUG || DEBUG_TOOLS */

#ifdef DEBUG

/*
 * performance measurement and statistics code
 */

/*
 * void
 * zbm_print_num(int num, const char *title)
 *	Statistics audit routine to print a number (in decimal) together
 *	with a description.
 *
 * Calling/Exit State:
 *	none
 */

static void
zbm_print_num(int num, const char *title)
{
	debug_printf("%8d      %s\n", num, title);
}

/*
 * void
 * zbm_print_decimal(ulong_t a, ulong_t b, const char *title)
 *	Statistics audit routine to print a fraction (to 4 decimal digits)
 *	together with a description.
 *
 * Calling/Exit State:
 *	none
 */

static void
zbm_print_decimal(ulong_t a, ulong_t b, const char *title)
{
	ulong_t c;
	char str[5];

	if (b == 0) {
		debug_printf("    ----      %s\n", title);
		return;
	}

	c = ((a % b) * 10000) / b;
	str[4] = '\0';
	str[3] = (c % 10) + '0'; c /= 10;
	str[2] = (c % 10) + '0'; c /= 10;
	str[1] = (c % 10) + '0'; c /= 10;
	str[0] = (c % 10) + '0';

	debug_printf("%8d.%s %s\n", a / b, str, title);
}

/*
 * void
 * print_zbm_stats(const zbm_t *zbmp)
 *	Statistic audit routine to print out a summary of the statistics.
 *
 * Calling/Exit State:
 *	Intended to be called from a kernel debugger or in-kernel test.
 */

void
print_zbm_stats(const zbm_t *zbmp)
{
	
	zbm_print_num(zbmp->zbm_stats.zbs_nvm_allocs,
		      "virtual allocation requests");
	zbm_print_num(zbmp->zbm_stats.zbs_nvm_free, "virtual free requests");
	zbm_print_num(zbmp->zbm_stats.zbs_nfail, "allocation failures");
	zbm_print_decimal(zbmp->zbm_stats.zbs_nvm_allocs -
			  zbmp->zbm_stats.zbs_nsmall,
			  zbmp->zbm_stats.zbs_nvm_allocs,
			  "large allocation requests/all allocation requests");
	zbm_print_decimal(zbmp->zbm_stats.zbs_ncells,
			  zbmp->zbm_stats.zbs_nsmall,
			  "cells searched per small allocation request");
	zbm_print_decimal(zbmp->zbm_stats.zbs_nshootdowns,
			  zbmp->zbm_stats.zbs_nvm_allocs,
			  "shootdowns per allocation requests");
	zbm_print_decimal(zbmp->zbm_stats.zbs_npages,
			  zbmp->zbm_stats.zbs_nvm_allocs,
			  "pages per allocation request");
	zbm_print_decimal(zbmp->zbm_stats.zbs_n1fail,
			  zbmp->zbm_stats.zbs_nsmall,
			  "free pool failures per small request");
	zbm_print_decimal(zbmp->zbm_stats.zbs_n2fail,
			  zbmp->zbm_stats.zbs_nsmall,
			  "free pool restore failures per small request");
	zbm_print_num(zbmp->zbm_stats.zbs_nallocated,
		      "pages have been allocated");
	zbm_print_num(zbmp->zbm_stats.zbs_nfreed, "pages have been freed");
	zbm_print_num(zbmp->zbm_stats.zbs_nallocated -
		      zbmp->zbm_stats.zbs_nfreed, "pages are allocated");
	zbm_print_num(zbmp->zbm_mapsize, "pages in the segment");
	zbm_print_decimal(zbmp->zbm_stats.zbs_nallocated -
			  zbmp->zbm_stats.zbs_nfreed, zbmp->zbm_mapsize,
			  "of the space is allocated");
}

#endif /* DEBUG */
