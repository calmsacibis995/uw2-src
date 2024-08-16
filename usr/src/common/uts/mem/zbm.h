/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _MEM_ZBM_H	/* wrapper symbol for kernel use */
#define _MEM_ZBM_H	/* subject to change without notice */

#ident	"@(#)kern:mem/zbm.h	1.3"
#ident	"$Header: $"

#if defined(__cplusplus)
extern "C" {
#endif

/*
 * VM - Zoned Bit Map (ZBM) Allocation for Kernel Segment Drivers
 */

#ifdef _KERNEL_HEADERS

#include <mem/hat.h> /* REQUIRED */
#include <util/types.h> /* REQUIRED */

#elif defined(_KERNEL) || defined(_KMEMUSER)

#include <sys/types.h> /* REQUIRED */
#include <vm/hat.h> /* REQUIRED */

#endif /* _KERNEL_HEADERS */

#if defined(_KERNEL) || defined(_KMEMUSER)

/*
 * Statistical Information for a Zoned Bit Map
 */
struct zbm_stats {
	ulong_t		zbs_nvm_allocs;		/* # of zbm_allocs */
	ulong_t		zbs_nvm_free;		/* # of zbm_free */
	ulong_t		zbs_ncells;		/* # of cells searched */
	ulong_t		zbs_nshootdowns;	/* # of shootdowns */
	ulong_t		zbs_npages;		/* # of pages requested */
	ulong_t		zbs_nallocated;		/* # of pages allocated */
	ulong_t		zbs_nfreed;		/* # of pages freed */
	ulong_t		zbs_n1fail;		/* # of free pool failures */
	ulong_t		zbs_n2fail;		/* # of restore pool fails */
	ulong_t		zbs_nfail;		/* # of allocation failures */
	ulong_t		zbs_nsmall;		/* # of small requests */
} zbm_stats;

/*
 * bit map to control allocation of a cell of zbm_cellsize pages
 * within the address range
 */
struct zbm_cell {
	ushort_t	zbc_nfree;		/* free page count */
	uchar_t		zbc_cursor;		/* allocation scan pointer */
	uchar_t		zbc_state;		/* cell state */

	TLBScookie_t	zbc_tlbcookie;		/* TLB cookie for last unmap */

	/*
	 * list links
	 */
	struct zbm_cell	*zbc_flink;		/* forward link */
	struct zbm_cell	*zbc_blink;		/* backward link */
};

/*
 * values for zbc_state
 */
#define	ZBC_TLBC	0			/* TLB consistent */
#define	ZBC_TLBI	1			/* TLB inconsistent */
#define	ZBC_RESERVE	2			/* on the reserve list */
						/* TLB consistent */
#define	ZBC_AGING	3			/* on the reserve list */
						/* TLB inconsistent */
#define	ZBC_FREE_TLBC	4			/* in the free pool */
						/* TLB consistent */
#define	ZBC_FREE_TLBI	5			/* in the free pool */
						/* TLB inconsistent */
#define	ZBC_FREE_AGING	6			/* in the free pool */
						/* on the reserve list */
						/* TLB inconsistent */

/* 
 * The states have the following significance:
 * 
 *	state			significance
 *	-----			------------
 *	ZBC_TLBC		<= 50% of the space is free
 *				cell is off the reserve list
 *				cell is out of the free pool
 *				TLB is consistent (no shootdown required)
 * 
 *	ZBC_TLBI		<= 50% of the space is free
 *				cell is off the reserve list
 *				cell is out of the free pool
 *				TLB is inconsistent (shootdown required)
 * 
 *	ZBC_RESERVE		> 50% of the space is free
 *				cell is on the reserve list
 *				cell is out of the free pool
 *				TLB is consistent (no shootdown required)
 * 
 *	ZBC_AGING		> 50% of the space is free
 *				cell is on the reserve list
 *				cell is out of the free pool
 *				TLB is inconsistent (shootdown required)
 *	
 *	ZBC_FREE_TLBC		cell is off the reserve list
 *				cell is in the free pool
 *				TLB is consistent (no shootdown required)
 *	
 *	ZBC_FREE_TLBI		<= 50% of the space is free
 *				cell is off the reserve list
 *				cell is in the free pool
 *				TLB is inconsistent (shootdown required)
 *	
 *	ZBC_FREE_AGING		> 50% of the space is free
 *				cell is on the reserve list
 *				cell is in the free pool
 *				TLB is inconsistent (shootdown required)
 * 
 * Reserve List Structure:
 * 
 *	Doubly linked list containing cells in state ZBC_RESERVE and
 *	ZBC_AGING.
 * 
 *	All ZBC_RESERVE cells on the reserve list precede all ZBC_AGING
 *	cells. Thus, the list contains two sections: (i) the ZBC_RESERVE
 *	cells (if any) beginning at the list head, followed by (ii) the
 *	ZBC_AGING cells (if any) extending to the list tail.
 * 
 *	The ZBC_AGING cells on the reserve list are in cookie age order
 *	(youngest cookie at the tail).
 * 
 *	All cells on the reserve list must have > 50% of the space free.
 * 
 * Free Pool:
 * 
 *	A set of upto ZBM_NPOOL cells, from which "small" allocations are
 *	currently taking place.
 * 
 *	Implemented as an array of pointers to cells. Non-NULL pointers
 *	indicate cells present in the pool.
 * 
 * TLB Consistency Rules:
 * 
 *	If a cell is in state ZBM_TLBC, ZBC_RESERVE, or ZBC_FREE_TLBC,
 *	then all free space in the cell may be allocated without a
 *	TLB shootdown.
 * 
 *	All free space "ahead" of the cell cursor can always be allocated
 *	without the need for a TLB shootdown.
 * 
 *	If a cell is in state ZBC_TLBI, ZBC_AGING, ZBC_FREE_TLBI, or
 *	ZBC_FREE_AGING, then a TLB shootdown is required to allocate
 *	space from "behind" the cell cursor.
 * 
 *	Consequently, whenever space is freed from a cell:
 * 
 *		(a) the cell cusor is advanced beyond the highest
 *		    address freed, and,
 * 
 *		(b) the cell is transitioned (if necessary) into one of
 *		    states ZBC_TLBI, ZBC_AGING, ZBC_FREE_TLBI, or
 *		    ZBC_FREE_AGING.
 *		
 *		(c) A new TLBScookie_t shootdown cookie is obtained for
 *		    the cell.
 *	
 *	Normally, when a TLB shootdown is generated for the cell, the cookie
 *	obtained in step (c) is used.  However, when multiple cells are
 *	shot down together, the cookie of the one obtained last (youngest
 *	cookie) may be used.
 * 
 *	Also, when space is allocated from a cell, the cell cursor may
 *	only be advanced.  The cell cursor can be reset to the beginning of
 *	the cell only when:
 * 
 *		(a) a TLB shootdown is generated for the cell, or
 *		(b) the cell is in state ZBC_RESERVE.
 * 
 * Initial State:
 * 
 *	For all cells:
 * 
 *		(a) the state is ZBC_RESERVE,
 *		(b) the cell is on the reserve list,
 *		(c) the cell cursor is at the beginning of the cell, and
 *		(d) all space in the cell is free.
 * 
 *	The free pool is empty.
 * 
 * State Transition Rules:
 * 
 *	The TLB rules:
 * 
 *		Whnever space is freed from a cell, the cell is
 *		transitioned (if necessary) into one of the states
 *		ZBC_TLBI, ZBC_AGING, ZBC_FREE_TLBI, or ZBC_FREE_AGING.
 * 
 *		Whenever a TLB shootdown is generated for a cell, the cell
 *		is transitioned into one of the states ZBM_TLBC,
 *		ZBC_RESERVE, or ZBC_FREE_TLBC.
 * 
 *	The FREE pool rules:
 * 
 *		Whenever a cell is added to the free pool, it is transitioned
 *		into state ZBC_FREE_TLBC.
 * 
 *		Whenever a state transition occurs for a cell in the free
 *		pool, and the cell remains in the free pool, then the
 *		resulting state must be one of ZBC_FREE_TLBC,
 *		ZBC_FREE_TLBI, or ZBC_FREE_AGING.
 * 
 *		Whenever a cell is removed from the free pool, it is
 *		transitioned into one of the states ZBC_TLBC, ZBC_TLBI,
 *		ZBC_RESERVE, or ZBC_AGING.
 *	
 *	Reserve List Rules:
 * 
 *		Whenever a cell is added to the reserve list head, it is
 *		transitioned into state ZBC_RESERVE.
 *		
 *		Whenever a cell is added to the reserve list tail, it is
 *		transitioned into one of the states ZBC_AGING or
 *		ZBC_FREE_AGING.
 * 
 *		Whenever a state transition occurs for a cell on the
 *		reserve list, and the cell remains on the reserve list,
 *		then the resulting state must be one of ZBC_RESERVE,
 *		ZBC_AGING, or ZBC_FREE_AGING.
 * 
 *		Whenever a cell is removed from the reserve list, it is
 *		transitiioned into one of the states ZBC_TLBC, ZBC_TLBI,
 *		ZBC_FREE_TLBC, or ZBC_FREE_TLBI.
 * 
 *	The Space rule:
 * 
 *		Whenever more than 50% of the space in a cell is free, it
 *		is one of the states ZBC_RESERVE, ZBC_AGING, ZBC_FREE_TLBC,
 *		or ZBC_FREE_AGING.
 * 
 *		Whenever less than 50% of the space in a cell is free, it
 *		is in one of the states ZBC_TLBC, ZBC_TLBI,
 * 
 * Transitions:
 * 
 * Source State	Target State	Reason (Side Effects)
 * ------------	------------	---------------------
 * ZBC_TLBC	ZBC_TLBI	Space is freed from the cell, but the
 *				cell still has <= 50% free. (The cell cursor
 *				is advanced, if necessary.)
 *		ZBC_AGING	Space is freed from the cell, resulting
 *				in > 50% free space in the cell. (The
 *				cell is added to the tail of the reserve
 *				list. The cell cursor is advanced if
 *				necessary.)
 * ZBC_TLBI	ZBC_TLBC	A "big map" allocation caused a TLB
 *				shootdown to be generated for the cell.
 *		ZBC_AGING	Space is freed from the cell, resulting
 *				in > 50% free space in the cell. (The cell
 *				is added to the tail of the reserve list.
 *				The cell cursor is advanced if necessary.)
 * ZBC_RESERVE	ZBC_FREE_TLBC	An allocation demand caused the cell to be
 *				added to the free pool. (The cell is removed
 *				from the reserve list. The cell cursor is
 *				reset to the front of the cell.)
 *		ZBC_AGING	Space is freed from the cell. (The cell is
 *				moved to the tail of the reserve list. The
 *				cell cursor is advanced, if necessary.)
 *		ZBC_TLBC	A "big map" allocation caused free space
 *				to drop to <= 50%. (The cell is removed
 *				from the reserve list.)
 * ZBC_AGING	ZBC_RESERVE	A "big map" allocation caused a TLB
 *				shootdown to be generated for the cell, but
 *				> 50% of the cell remains free. (The cell is
 *				moved to the head of the reserve list.)
 *		ZBC_TLBC	A "big map" allocation caused a TLB
 *				shootdown to be generated for the cell, and
 *				<= 50% of the cell remains free. (The cell
 *				is removed from the reserve list.)
 *		ZBC_FREE_TLBC	An allocation demand caused the cell to be
 *				added to the free pool. (A TLB shootdown is
 *				generated. The cell is removed from the
 *				reserve list. The cell cursor is reset to
 *				the front of the cell.)
 *		ZBC_TLBI	A "big map" allocation resulted in free
 *				space of <= 50% of the cell. However, no
 *				TLB shootdown was required. (The cell is
 *				removed from the reserve list.)
 *		ZBC_AGING	Space is freed from the cell. (The cell is
 *				moved to the tail of the free list. The
 *				cell cursor is advanced, if necessary.)
 * ZBC_FREE_TLBC
 *		ZBC_RESERVE	The cell was removed from the free pool
 *				due to an allocation failure, but > 50%
 *				of the cell remains free. (The cell is
 *				added to the head of the reserve list.)
 *		ZBC_FREE_TLBI	Space is freed from the cell, but <= 50%
 *				of the space is now free. (The cell cursor
 *				is advanced, if necessary.)
 *		ZBC_FREE_AGING	Space is freed from the cell, and > 50%
 *				of the space is now free. (The cell is
 *				added to the tail of the reserve list. The
 *				cell cursor is advanced, if necessary.)
 *		ZBC_TLBC	The cell was removed from the free pool
 *				due to an allocation failure, and <= 50% of
 *				the cell is now free.
 * ZBC_FREE_TLBI
 *		ZBC_FREE_TLBC	A "big map" allocation caused a TLB
 *				shootdown to be generated for the cell.
 *		ZBC_AGING	Space is freed from the cell, resulting
 *				in > 50% of the cell being free. (The cell
 *				is added to the tail of the reserve list.
 *				The cell cursor is advanced, if necessary.)
 * ZBC_FREE_AGING
 *		ZBC_FREE_TLBC	A "big map" allocation caused a TLB
 *				shootdown to be generated for the cell.
 *				(The cell is removed from the reserve list.)
 *		ZBC_FREE_TLBC	An allocation demand caused the cursor to
 *				be reset to the front of the cell. (A TLB
 *				shootdown is generated. The cell is removed
 *				from the reserve list.)
 *		ZBC_FREE_TLBI	Space was allocated from the cell, causing
 *				free space to drop to <= 50% of the cell,
 *				but no TLB shootdown was generated. (The
 *				cell is removed from the reserve list.)
 */

/*
 * Number of cells in the free pool
 */
#define ZBM_NPOOL		4

/*
 * The zbm structure: locus of activity for a ZONED BIT MAP
 */
typedef struct zbm {
	/*
	 * pointers to calloced storage
	 */
	uint_t			*zbm_bitmap;	/* allocation map */
	struct zbm_cell		*zbm_cell;	/* cells */

	/*
	 * The reserve list anchor.
	 */
	struct zbm_cell		zbm_reserve;

	/*
	 * the free pool
	 */
	struct zbm_cell		*zbm_pool[ZBM_NPOOL];
	int 			zbm_pool_cursor;

	/*
	 * miscellaneous data
	 */
	int			zbm_bit_cur;	/* alloc cursor for bitmap */
	int			zbm_cellwidth;	/* words of bitmap per cell */
	int			zbm_ncell;	/* number of cells */
	int			zbm_reservesz;	/* free size threshold */
	int			zbm_big;	/* alloc algorithm threshold */
	int			zbm_climit;	/* cursor near cell end */
	int			zbm_mapsize;	/* # bits in the bit map */
	int			zbm_mapwidth;	/* # words in the bitmap */
	int			zbm_cwshft;	/* log2 zbm_cellwidth */
	int			zbm_cellmask;	/* for % by zbm_cellwidth */
	int 			zbm_cellsize;	/* number of pages per cell */

	/*
	 * Virtual address range from which we are allocating
	 */
	vaddr_t			zbm_base;	/* base address */
	uint_t			zbm_size;	/* size in bytes */

	/*
	 * zbm_lock mutexes all non-constant fields in this structure
	 */
	lock_t			zbm_lock;	/* spin lock */
	sv_t			zbm_sv; 	/* for waiting */

	/*
	 * function to call to report allocation failures
	 */
	void			(*zbm_fail_func)(void);

	/*
	 * statistics
	 */
#ifdef DEBUG
	struct zbm_stats	zbm_stats;
#endif /* DEBUG */
} zbm_t;

#endif /* defined(_KERNEL) || defined(_KMEMUSER) */

#ifdef _KERNEL

/*
 * Exported Functions
 */
extern void zbm_calloc(zbm_t *, uint_t, int);
extern void zbm_init(zbm_t *, vaddr_t, uint_t, void (*)(void));
extern vaddr_t zbm_alloc(zbm_t *zbmp, ulong_t npages, uint_t flag);
void zbm_free(zbm_t *zbmp, vaddr_t vaddr, ulong_t npages);
int zbm_cell_size(uint_t);

/*
 * Exported Data
 */
extern int kzfod_cellsize;

#endif /* _KERNEL */

#if defined(__cplusplus)
	}
#endif

#endif /* _MEM_ZBM_H */
