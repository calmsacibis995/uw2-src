/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:mem/seg_wrap.c	1.2"
#ident	"$Header: $"

/***************************************************************************

       @(-)seg_wrap.c	1.15 LCC) Modified 03:48:14 3/3/94

       Copyright (c) 1990-91 Locus Computing Corporation.
       All rights reserved.
       This is an unpublished work containing CONFIDENTIAL INFORMATION
       that is the property of Locus Computing Corporation.
       Any unauthorized use, duplication or disclosure is prohibited.

***************************************************************************/

/*
**	seg_wrap.c	- doubly maps an address space
**
**	routines included:
**		segwrap_create - create the double mapping
**
**	This segment driver performs a double mapping of a segvn PRIVATE
**	address space.  It's primary purpose is to provide virtual addresses
**	for the DOS 64K wrap around region (i.e. the address 1Mb to 1Mb + 64K
**	map into the same physical memory as 0 to 64K).
**	Basically, this driver simply sets up a reference to the incore page
**	corresponding to the "real" region of memory.  This driver is only
**	necessary to share the mapping to a private region.  Most applications
**	(in DOS) won't ever access the segment so in general, the only
**	code excersized is the mapping and unmapping code.
*/

#include <sys/types.h>
#include <util/param.h>
#include <util/sysmacros.h>
#include <proc/mman.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>
#include <svc/systm.h>

#include <mem/page.h>
#include <mem/mem_hier.h>
#include <mem/hat.h>
#include <mem/as.h>
#include <mem/seg.h>

#include <mem/seg_wrap.h>

/*
 * Private seg op routines.
 */
STATIC	int segwrap_nop(void);
STATIC	int segwrap_dup(struct seg *seg, struct seg *newsegp);
STATIC	int segwrap_unmap(struct seg *seg, vaddr_t addr, uint_t len);
STATIC	void segwrap_free(struct seg *seg);
STATIC	faultcode_t segwrap_fault(struct seg *seg, vaddr_t addr, uint_t len,
				  enum fault_type type, enum seg_rw rw);
STATIC	int segwrap_getprot(struct seg *seg, vaddr_t addr, uint_t *prot);
STATIC	off_t segwrap_getoffset(struct seg *seg, vaddr_t addr);
STATIC	int segwrap_gettype(struct seg *seg, vaddr_t addr);
STATIC	int segwrap_getvp(struct seg *seg, vaddr_t addr, vnode_t **vpp);
STATIC	int segwrap_kluster(struct seg *seg, vaddr_t addr, int delta);
STATIC	int segwrap_sync(struct seg *seg, vaddr_t addr, uint_t len,
			 int attr, uint_t flags);
STATIC	int segwrap_incore(struct seg *seg, vaddr_t addr, uint_t len,
			   char *vec);
STATIC void segwrap_childload(struct seg *pseg, struct seg *cseg);
STATIC	int segwrap_memory(struct seg *seg, vaddr_t *basep, uint_t *lenp);

struct	seg_ops segwrap_ops = {
	segwrap_unmap,
	segwrap_free,
	segwrap_fault,
	(int (*)()) segwrap_nop,	/* setprot */
	(int (*)()) segwrap_nop,	/* checkprot */
	segwrap_kluster,
	segwrap_sync,
	segwrap_incore,
	(int (*)()) segwrap_nop,	/* lockop */
	segwrap_dup,
	segwrap_childload,
	segwrap_getprot,
	segwrap_getoffset,
	segwrap_gettype,
	segwrap_getvp,
	(void (*)()) NULL,		/* age */
	(boolean_t (*)()) NULL,		/* lazy_shootdown */
	segwrap_memory
};

/*
 * Common zfod structures, provided as a shorthand for others to use.
 */
STATIC
struct	segwrap_crargs zfod_segwrap_crargs = {
	(vaddr_t) NULL,
};

STATIC
struct	segwrap_crargs kzfod_segwrap_crargs = {
	(vaddr_t) NULL,
};

caddr_t	segwrap_args = (caddr_t)&zfod_segwrap_crargs;	/* user zfod argsp */
caddr_t	ksegwrap_args = (caddr_t)&kzfod_segwrap_crargs;	/* kernel zfod argsp */

/*
 * int
 * segwrap_create(struct seg *seg, vaddr_t arg_list)
 *
 * Calling/Exit State:
 *
 *	input:
 *		seg - pointer to generic segment structure describing this
 *			particular mapping
 *		arg_list - segment driver specific arguement list
 *
 *	tasks:
 *		set up the private data structure associated with the segment.
 *		The actual mappings for the page table entries are done when
 *		a fault occurs.
 *
 *		We do not check to see if the segment can be concatenated with
 *		any other segments since we don't expect many of these segments
 *		in a processes virtual address space.  In fact, currently we
 *		expect only one.
 *
 *	output:	0 on SUCCESS
 *		non zero on FAIL
 */

int
segwrap_create(struct seg *seg, vaddr_t arg_list)
{
	struct segwrap_crargs *user_args;
	struct segwrap_data *local_seg_data;
	struct as *our_as;
	int retError;

	user_args = (struct segwrap_crargs *) arg_list;
	our_as = seg->s_as;

	/* verify that the range of addresses to be doubly mapped are */
	/* actually there */
	retError = as_checkprot(our_as, user_args->base, seg->s_size,
				PROT_READ);
	if (retError != 0)
		return retError;

	/* set up our private data structure */
	local_seg_data = (struct segwrap_data *)
			kmem_alloc(sizeof (struct segwrap_data), KM_SLEEP);

	local_seg_data->base = user_args->base;
	seg->s_ops = &segwrap_ops;
	seg->s_data = (char *) local_seg_data;

	return (0);
}

/*
 *
 * STATIC int
 * segwrap_dup(struct seg *oldseg, struct seg *newseg)
 *
 * Calling/Exit State:
 *
 *	input:	oldseg - generic segment desription for existing segment
 *		newseg - generic segment desription for segment to be created
 *
 *	tasks:	Simply duplicate the privately held data
 *
 *	output: always successful, returns 0
 */

STATIC int
segwrap_dup(struct seg *oldseg, struct seg *newseg)
{
	newseg->s_data = kmem_alloc(sizeof (struct segwrap_data), KM_SLEEP);

	*(struct segwrap_data *) newseg->s_data =
					*(struct segwrap_data *) oldseg->s_data;
	newseg->s_ops = &segwrap_ops;

	return 0;
}

/*
 * STATIC int
 * segwrap_unmap(struct seg *seg, vaddr_t start_addr, uint_t len)
 *
 * Calling/Exit State:
 *
 *	input:	seg - pointer to generic segment structure
 *		start_addr - starting address to initiate the unmapping
 *		len - amount of the address space to unmap
 *
 *	tasks:	Call the hat layer to take care of the page table entries.
 *		Remove the segment if the entire segment is being unmapped
 *		(most common case), otherwise if just the beginning
 *		or the ending is being unmapped, trim the segment.
 *		Finally, if the middle of the segment is targeted,
 *		then split the segment into two new segments
 *		(each trimmmed appropriately).
 *
 *	output:	always successful and returns 0
 */

STATIC int
segwrap_unmap(struct seg *seg, vaddr_t start_addr, uint_t len)
{
	struct segwrap_data *local_data;
	struct seg *new_seg;
	vaddr_t newseg_base;
	uint_t newseg_size;

	local_data = (struct segwrap_data *) seg->s_data;

	/* Unload any hardware translations in the range to be taken out. */
	hat_unload(seg, start_addr, len, HAT_NOFLAGS);

	/* If the entire segment is being unmapped, then free it */
	if (start_addr == seg->s_base && len == seg->s_size)
	{
		seg_free(seg);
		return (0);
	}

	/* If the beginning, then simply trim the segment */
	if (start_addr == seg->s_base)
	{
		local_data->base += len;
		seg->s_base += len;
		seg->s_size -= len;
		return 0;
	}

	/* If the end, then simply trim the length */
	if (start_addr + len == seg->s_base + seg->s_size)
	{
		seg->s_size -= len;
		return 0;
	}

	/* Need to unmap the middle of this segment, so split the segment */
	/* effectively creating two new segments */
	newseg_base = start_addr + len;
	newseg_size = (seg->s_base + seg->s_size) - newseg_base;
	seg->s_size = start_addr - seg->s_base;
	new_seg = seg_alloc(seg->s_as, newseg_base, newseg_size);
	if (new_seg == NULL) {
		/*
		 *+ A call to seg_alloc() failed during an unmap
		 *+ attempt. This is an unrecoverable situation and
		 *+ probably indicates a more profound software or
		 *+ hardware problem elsewhere in the system.
		 */
		cmn_err(CE_PANIC, "segwrap_unmap seg_alloc");
	}

	new_seg->s_data =  kmem_alloc(sizeof (struct segwrap_data), KM_SLEEP);

	new_seg->s_ops = seg->s_ops;

	/* blatently copy the private data structure */
	/* and update the offset into the real virtual memory */
	*(struct segwrap_data *)new_seg->s_data = *local_data;
	((struct segwrap_data *)new_seg->s_data)->base +=
				(start_addr - local_data->base) + len;

	return 0;
}

/*
 * STATIC void
 * segwrap_free(struct seg *seg)
 *
 * Calling/Exit State:
 *
 *	input:	seg - generic segment structuer
 *
 *	tasks:	free up our STATIC data
 *
 *	output:	none
 */

STATIC void
segwrap_free(struct seg *seg)
{
	kmem_free(seg->s_data, sizeof(struct segwrap_data));
}

/*
 * STATIC faultcode_t
 * segwrap_fault(struct seg *seg, vaddr_t fault_addr, uint_t len,
 *	         enum fault_type fault_type, enum seg_rw access_type)
 *
 * Calling/Exit State:
 *
 *	input:	seg - generic segment description
 *		fault_addr - address which caused the fault
 *		len - number of bytes to fault in
 *		fault_type - whether the fault was a protection fault or a
 *			software locking or unlocking
 *		access_type - whether this fault occured via a read or write
 *			or something else.
 *
 *	tasks:	since we are only maintaining a mirror mapping of some other
 *		region within our virtual address space, we simply need
 *		to verify that the "real" memory is available.  If it is not,
 *		fault it in.  Then, we simply set up the PTE tables to refer
 *		to the physical pages.  We always cause a write fault to the
 *		"real" memory region since we want to make sure that both
 *		double mapped virtual addresses always refer to the same
 *		page.  If the "real" memory region is PRIVATE (our assumption),
 *		and if we simply mapped in the PTE to refer to current
 *		(read only) page in memory, then any write to the "real" address
 *		would cause a fault in the "real" segment driver which would
 *		replace the object page with an anonymous page.  Our virtual
 *		address that this segment driver manages would then refer
 *		to a stale page.
 *
 *		Efficiency Note:
 *
 *		Since the area that we are currently double mapping is simply
 *		the DOS wrap around (e.g. 64K), both loops therefore only
 *		step through a maximum of 16 times.  Also, the first loop
 *		will run to completion iff there are no pages to fault in.
 *		Actually, most faults are only on a single byte so that the
 *		the common case involves no looping.
 *
 *	output:	0 on SUCCESS
 *		non zero on FAILURE
 */

STATIC faultcode_t
segwrap_fault(struct seg *seg, vaddr_t fault_addr, uint_t len,
	      enum fault_type fault_type, enum seg_rw access_type)
{
	struct segwrap_data *seg_info = (struct segwrap_data *) seg->s_data;
	vaddr_t real_addr;
	vaddr_t double_addr;
	hat_t *hatp;
	pte_t *cur_PTE;
	page_t *cur_page;
	uint_t len_left;
	hatpt_t *dummy_ptap;
	faultcode_t ret_error;
	extern pte_t *hat_vtopte_l();;

	/* compute the address to index into the seg_vn driver */
	real_addr = fault_addr - seg->s_base + seg_info->base;

	/* this driver does not support locking/unlocking of pages */
	/* anybody that needs it should be using the real virtual addresses */
	/* instead, i.e. the addresses that we map to */
	if (fault_type == F_SOFTLOCK || fault_type == F_SOFTUNLOCK)
		return 0;

	hatp = &seg->s_as->a_hat;
	HATRL_LOCK_SVPL(hatp);
	HAT_RDONLY_LOCK(hatp);

	/* Check to see if all the "real" address pages are in memory. */
	/* If there are any missing, fault them in now.  For our purposes, */
	/* a page must be writeable, it it is not it may get copied on write */
	/* and leave us with an inconsistent view of the double mapped memory */
	for (len_left = len; len_left > 0;
	     real_addr += PAGESIZE, len_left -= PAGESIZE)
	{
		/* See if the page directory is there */
		cur_PTE = hat_vtopte_l(hatp, real_addr, &dummy_ptap);
		if (cur_PTE != NULL && PG_ISVALID_WRITEABLE(cur_PTE))
			continue;

		HATRL_UNLOCK_SVDPL(hatp);
		ret_error = as_fault(seg->s_as, real_addr, len_left, F_PROT,
								S_WRITE);
		if (ret_error != 0)
		{
			HAT_RDONLY_UNLOCK(hatp);
			return ret_error;
		}
		HATRL_LOCK_SVPL(hatp);
	}

	/* reset real_addr for the second pass */
	real_addr = fault_addr - seg->s_base + seg_info->base;

	/* now construct our page table to correspond to the lower page table */
	for (double_addr = fault_addr, len_left = len; len_left > 0;
	     double_addr += PAGESIZE, real_addr += PAGESIZE,
	     len_left -= PAGESIZE)
	{
		cur_PTE = hat_vtopte_l(hatp, double_addr, &dummy_ptap);
		if (cur_PTE != NULL && PG_ISVALID_WRITEABLE(cur_PTE))
			continue;

		HATRL_UNLOCK_SVDPL(hatp);

		/* really wanted to use svtopfn() but that macro contained */
		/* a in appropriate mask, i.e. for pages with high physical */
		/* page numbers the page frame would be incorrect. */
		cur_page = hat_vtopp(seg->s_as, real_addr, access_type);
		if (cur_page == (page_t *) NULL)
		{
			HAT_RDONLY_UNLOCK(hatp);
			return FC_NOMAP;
		}

		hat_memload(seg, double_addr, cur_page, PROT_ALL, 0);
		page_unlock(cur_page);
		HATRL_LOCK_SVPL(hatp);
	}

	HATRL_UNLOCK_SVDPL(hatp);
	HAT_RDONLY_UNLOCK(hatp);
	return (0);
}


/*
 * STATIC int
 * segwrap_nop(void)
 *
 * Calling/Exit State:
 *
 */

STATIC int
segwrap_nop(void)
{
	return (0);
}

/*
 * STATIC int
 * segwrap_getprot(struct seg *seg, vaddr_t addr, uint_t *prot)
 *
 * Calling/Exit State:
 *
 */

/* ARGSUSED */
STATIC int
segwrap_getprot(struct seg *seg, vaddr_t addr, uint_t *prot)
{
	*prot = PROT_ALL;
	return 0;
}

/*
 * STATIC off_t
 * segwrap_getoffset(struct seg *seg, vaddr_t addr)
 *
 * Calling/Exit State:
 *
 */

/* ARGSUSED */
STATIC off_t
segwrap_getoffset(struct seg *seg, vaddr_t addr)
{
	return 0;
}

/*
 * STATIC int
 * segwrap_gettype(struct seg *seg, vaddr_t addr)
 *
 * Calling/Exit State:
 *
 */

/* ARGSUSED */
STATIC int
segwrap_gettype(struct seg *seg, vaddr_t addr)
{
	return MAP_SHARED;
}

/*
 * STATIC int
 * segwrap_getvp(struct seg *seg, vaddr_t addr, struct vnode **vpp)
 *
 * Calling/Exit State:
 *
 */

/* ARGSUSED */
STATIC int
segwrap_getvp(struct seg *seg, vaddr_t addr, struct vnode **vpp)
{
	*vpp = (struct vnode *) NULL;
	return -1;
}

/*
 * STATIC int
 * segwrap_kluster(struct seg *seg, vaddr_t addr, int delta)
 *
 * Calling/Exit State:
 *
 */

/* ARGSUSED */
STATIC int
segwrap_kluster(struct seg *seg, vaddr_t addr, int delta)
{
	return -1;
}

/*
 * STATIC int
 * segwrap_sync(struct seg *seg, vaddr_t addr, uint_t len, int attr,
 *		uint_t flags)
 *
 * Calling/Exit State:
 *
 *	Synchronize primary storage cache with real object in virtual memory.
 */

/* ARGSUSED */
STATIC int
segwrap_sync(struct seg *seg, vaddr_t addr, uint_t len, int attr, uint_t flags)
{
	return (0);	/* all anonymous memory - nothing to do */
}

/*
 * STATIC int
 * segwrap_incore(struct seg *seg, vaddr_t addr, uint_t len, char *vec)
 *
 * Calling/Exit State:
 *
 *	Determine if we have data corresponding to pages in the
 *	primary storage virtual memory cache (i.e., "in core").
 *	N.B. Assumes things are "in core" if page structs exist.
 */
STATIC int
segwrap_incore(struct seg *seg, vaddr_t addr, uint_t len, char *vec)
{
	int ret_error;
	uint_t total_size;
	vaddr_t new_addr;

	new_addr = addr - seg->s_base +
				((struct segwrap_data *) seg->s_data)->base;

	ret_error = as_incore(seg->s_as, new_addr, len, vec, &total_size);

	return (ret_error == 0) ? total_size : -1;
}

/*
 * STATIC void
 * segwrap_childload(struct seg *pseg, struct seg *cseg)
 *
 * Calling/Exit State:
 *
 */

/* ARGSUSED */
STATIC void
segwrap_childload(struct seg *pseg, struct seg *cseg)
{
}

/*
 * STATIC int
 * segwrap_memory(struct seg *seg, vaddr_t *base, uint_t *lenp)
 *
 * Calling/Exit State:
 *
 */

/* ARGSUSED */
STATIC int
segwrap_memory(struct seg *seg, vaddr_t *base, uint_t *lenp)
{
	return 0;
}
