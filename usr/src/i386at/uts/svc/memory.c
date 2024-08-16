/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:svc/memory.c	1.11"
#ident	"$Header: $"

/*
 * Architecture dependent memory handling routines
 * to deal with configration, initialization, and
 * error polling.
 */

#include <mem/hatstatic.h>
#include <mem/immu.h>
#include <svc/bootinfo.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

size_t		totalmem = 0;		/* total real memory */

/*
 * Any memnotused chunk with size less than or equal to SMALL_EXTENT
 * is considered a small chunk for allocation purposes (we try to use small
 * chunks before big ones).
 */
#define SMALL_EXTENT	(64 * 1024)

/*
 * The following two functions are located in the pstart module.
 * It is not valid to call them after pstart is unmapped.
 */
extern paddr_t	phys_palloc_dma(size_t);
#ifndef NO_RDMA
extern paddr_t	phys_palloc_nodma(size_t);
#endif /* NO_RDMA */

/*
 * Special return value from phys_palloc_dma() and phys_palloc_nodma.
 */
#define PALLOC_FAIL	((paddr_t)-1)

/*
 * void
 * conf_mem(void)
 *
 *	Configure memory.
 *
 * Calling/Exit State:
 *
 *	Figure top of memory.
 */

void
conf_mem(void)
{
	register int	avail;

	/*
	 * Determine total free physical memory.
	 */
	for (avail = 0; avail < bootinfo.memavailcnt; avail++)
		totalmem += bootinfo.memavail[avail].extent;
}

/*
 * paddr_t
 * getnextpaddr(uint_t size, uint_t flag)
 *
 *	Return (and consume) size bytes of pages aligned physical memory.
 *	Intended for use by palloc().
 *
 * Calling/Exit State:
 *
 *	Returns the physical address of size bytes of page aligned
 *	physically contiguous memory.
 *
 *	If flag is PMEM_ANY then non-DMAable memory is preferentially
 *	allocated, unless the DMAable list has a small extent (which
 *	we try to use up first).
 *
 *	If flag is PMEM_PHYSIO then only DMAable memory is allocated.
 *
 *	Thus, preference is given to small extents, and to non-DMAable
 *	memory (when permitted).
 */
/* ARGSUSED */
paddr_t
getnextpaddr(uint_t size, uint_t flag)
{
	paddr_t ret_paddr = PALLOC_FAIL;
	extern int pages_in_use;

	ASSERT(flag == PMEM_ANY || flag == PMEM_PHYSIO);

#ifndef NO_RDMA

	if (flag == PMEM_PHYSIO || memNOTused->extent < SMALL_EXTENT)
		ret_paddr = phys_palloc_dma(size);

	if (ret_paddr == PALLOC_FAIL && flag == PMEM_ANY)
		ret_paddr = phys_palloc_nodma(size);

#endif /* NO_RDMA */

	if (ret_paddr == PALLOC_FAIL)
		ret_paddr = phys_palloc_dma(size);

	if (ret_paddr == PALLOC_FAIL) {
		/*
		 *+ During kernel initialization (boot), there
		 *+ was insufficient physical memory to allocate
		 *+ kernel data structures.  (Note that some
		 *+ physical memory may have been left unused in order
		 *+ to satisfy alignment and contiguity requirements.)
		 *+ Corrective action:  Add more physical memory.
		 */
		cmn_err(CE_PANIC, "palloc: ran out of physical memory");
	}
	pages_in_use += btop(size);

	return ret_paddr;
}
