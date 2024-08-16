/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386:mem/pse_drv.c	1.5"
#ident	"$Header: $"

#include <fs/file.h>
#include <fs/specfs/snode.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/immu.h>
#include <mem/kmem.h>
#include <mem/pse.h>
#include <mem/seg_kmem.h>
#include <mem/seg_dev.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/disp.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <svc/cpu.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ghier.h>
#include <util/ksynch.h>
#include <util/mod/ksym.h>
#include <util/mod/mod_obj.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/types.h>
#include <util/ipl.h>

/*
 * Pseudo driver supporting PSE mappings
 *
 * The driver consists of three parts:
 *
 *	(1) Allocator for PSE_PAGESIZE physical pages, used internally.
 *		PSE_PAGESIZE pages are reserved at startup time, based
 *		on the variable pse_physmem, initialized from the
 *		tunable PSE_PHYSMEM.
 *
 *	(2) Allocator for device numbers (dev_t) and vnodes
 *		corresponding to the pse pseudo-device.  Minor
 *		device numbers are allocated dynamically, and
 *		physical pages are associated with them.
 *
 *	(3) The device driver proper, including routines pse_segmap
 *		and pse_mmap, required to implement PSE mappings of
 *		the PSE_PAGESIZE physical pages.
 */

/*
 * Declarations for memory allocator:
 *
 *	pse_physmem	size of memory, in bytes, to reserve for PSE
 *			physical pages.  (imported from space.c)
 *
 *	pse_freepool	dynamically allocated array of ppids of available
 *			PSE physical pages
 *
 *	pse_nfree	count of pages available in pse_freepool
 *
 *	pse_npgs	total number of PSE physical pages
 *
 *	pse_palloc()	startup allocator for PSE_PAGESIZE-aligned memory
 */

extern int pse_physmem;
STATIC ppid_t *pse_freepool = NULL;
STATIC int pse_nfree = 0;
STATIC int pse_npgs = 0;

extern paddr_t pse_palloc(void);
extern void *calloc(ulong_t);
extern caddr_t physmap(paddr_t, ulong_t, uint_t);

#define PALLOC_FAIL	((paddr_t)-1)

/*
 * STATIC void
 * pse_pagepool_create(void)
 *	Create pool of physical pages of size PSE_PAGESIZE.
 *
 * Calling/Exit State:
 *	Called during startup, while calloc is still available
 *	and startup code is still mapped in.
 *
 * Remarks:
 *	Calls pse_palloc, which is part of the pre-sysinit
 *	startup code, i.e., in mmu.c
 */
STATIC void
pse_pagepool_create(void)
{
	paddr_t paddr;
	size_t size = btopser(pse_physmem);

	/*
	 * if PSE is not supported, or if no physical memory was requested
	 *	don't create the pool
	 */
	if (!PSE_SUPPORTED() || (size == 0))
		return;

	/*
	 * Allocate an array of ppid_t's for keeping track of pages.
	 */
	pse_freepool = calloc(size * sizeof(ppid_t));

	/*
	 * Allocate PSE pages until either get as many as requested
	 *	(via pse_physmem) or can't get any more.
	 */
	for (pse_nfree = 0 ; pse_nfree < size ; ++pse_nfree) {
		paddr = (paddr_t)pse_palloc();
		if (paddr == PALLOC_FAIL)
			break;
		ASSERT((paddr & PSE_PAGEOFFSET) == 0);
		pse_freepool[pse_nfree] = phystopfn(paddr);
		ASSERT((pse_freepool[pse_nfree] & PSE_NPGOFFSET) == 0);
	}
	pse_npgs = pse_nfree;
}

/*
 * STATIC void
 * pse_pagezero(ppid)
 *	Zero out a PSE_PAGESIZE chunk of physical memory whose first
 *	physical page has the specified ppid.
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	The routine zeros out the memory by calling pzero for
 *	each physical page in the PSE_PAGESIZE chunk.
 */
STATIC void
pse_pagezero(ppid)
{
	caddr_t va;
	int i;

	ASSERT(ppid != NOPAGE);
	ASSERT((ppid & PSE_NPGOFFSET) == 0);
	for (i = 0 ; i < PSE_NPAGES ; ++i)
		pzero(ppid + i);
}

/*
 * STATIC ppid_t
 * pse_page_get(void)
 *	Allocate a PSE page (physical)
 *
 * Calling/Exit State:
 *	The pse_lock is held on entry, since the only caller of
 *	this currently is pse_allocdev.
 *
 *	Returns a ppid for the page, or NOPAGE if none available.
 *	The page is zeroed out.
 */
STATIC ppid_t
pse_page_get(void)
{
	ppid_t ppid;

	if (pse_nfree == 0)
		return NOPAGE;
	ASSERT(pse_nfree <= pse_npgs);
	ppid = pse_freepool[--pse_nfree];
#ifdef	DEBUG
	pse_freepool[pse_nfree] = NOPAGE;
#endif
	ASSERT((ppid != NOPAGE) && (pse_nfree >= 0));
	pse_pagezero(ppid);
	return ppid;
}

/*
 * STATIC void
 * pse_page_free(ppid_t ppid)
 *	Free a PSE page (physical)
 *
 * Calling/Exit State:
 *	On entry, the pse_lock is held, and it remains locked
 *	on exit.
 *
 *	The specified ppid must have been previously allocated by
 *	pse_page_get.
 */
STATIC void
pse_page_free(ppid_t ppid)
{

	if (ppid == NOPAGE)
		return;
	ASSERT(pse_nfree < pse_npgs);
#ifdef	DEBUG
	{
	int i;

	for (i = 0 ; i < pse_npgs ; ++i)
		if (pse_freepool[i] == ppid)
			cmn_err(CE_PANIC,
				"pse_page_free: returning free ppid %x\n",
					ppid);
	}
#endif
	pse_freepool[pse_nfree++] = ppid;
}

/*
 * Declarations for device and vnode allocator:
 *
 *	psedevinfo	table of per minor device information:
 *			number of pse pages and list of ppid's for
 *			each minor device
 *
 *	pse_nminors	number of usable minor device numbers
 *
 *	pse_major	major device number (from space.c)
 *
 *	pse_drv_lock	lock guarding the driver.  It in effect
 *			also guards the memory allocator, since
 *			the driver is the only client of the
 *			memory allocator at present.
 */
STATIC struct psedevinfo {
	uint_t psedev_npse;
	ppid_t *psedev_ppidp;
} *psedevinfo;

STATIC int pse_nminors;

extern int pse_major;

STATIC lock_t pse_drv_lock;
LKINFO_DECL(pse_lkinfo, "VM:PSE:pse device driver mutex", 0);

#define	PSE_LOCK()	LOCK_PLMIN(&pse_drv_lock)
#define	PSE_UNLOCK(pl)	UNLOCK_PLMIN(&pse_drv_lock, pl)

/*
 * STATIC void
 * pse_initdev(void)
 *	Initialize device-related information for allocating minor
 *	devices.
 *
 * Calling/Exit State:
 *	Called during startup, by pse_init (Driver init routine).
 */
STATIC void
pse_initdev(void)
{
	int i;

	/*
	 * Maximum number of devices is number of PSE pages.  If
 	 *	no pages, then no devices - return.
	 */
	if ((pse_nminors = pse_npgs) == 0)
		return;

	/*
	 * if there are pse pages, this platform must support PSE.
	 */
	ASSERT(PSE_SUPPORTED());

	/*
	 * Allocate space for pse device table, and initialize
	 *	each entry as having no pages assigned =>
	 *	it's available.
	 */
	psedevinfo = (struct psedevinfo *)kmem_zalloc(pse_nminors *
			sizeof(struct psedevinfo), KM_NOSLEEP);

	/*
	 * If space allocation fails, then act as if we have no devices
	 */
	if (psedevinfo == NULL) {
		pse_nminors = 0;
		return;
	}
	LOCK_INIT(&pse_drv_lock, KERNEL_HIER_BASE, PLMIN, &pse_lkinfo,
		KM_NOSLEEP);
}

/*
 * Forward reference
 */
STATIC void pse_freedev(dev_t);

/*
 * STATIC dev_t
 * pse_allocdev(size_t nbytes)
 *	Allocate a new minor device, and associate nbytes worth
 *	of PSE pages with it.
 *
 * Calling/Exit State:
 *	Return value is either ENODEV or a device number which
 *	can be used to map in nbytes worth of PSE pages.
 */
STATIC dev_t
pse_allocdev(size_t nbytes)
{
	int minor, i;
	size_t npse;
	ppid_t *ppid;
	struct psedevinfo *psep;
	pl_t opl;

	/*
	 * Make a quick test; if not enough free pages, no need to try
	 *	to allocate a device
	 */
	npse = btopser(nbytes);
	if (pse_nfree < npse)
		return NODEV;
	/*
	 * Allocate an array of ppid's to store allocated pages.
	 */
	ppid = (ppid_t *)kmem_alloc(npse * sizeof(ppid_t), KM_SLEEP);
	opl = PSE_LOCK();
	i = 0;
	while ((i < npse) && ((ppid[i] = pse_page_get()) != NOPAGE))
		++i;
	if (i < npse) {
		while (i >= 0)
			pse_page_free(ppid[--i]);
		PSE_UNLOCK(opl);
		kmem_free((char *)ppid, npse * sizeof(ppid_t));
		return NODEV;
	}
	/*
	 * Now, find a minor device.  We're guaranteed to find one
	 * since the number of minor devices is equal to the total
	 * number of PSE pages
	 */
	minor = 0;
	psep = psedevinfo;
	while (psep->psedev_npse != 0) {
		++minor;
		++psep;
		ASSERT(minor < pse_nminors);
	}
	/*
	 * Fill in the data for the minor device, and release the lock
	 */
	psep->psedev_ppidp = ppid;
	psep->psedev_npse = i;
	PSE_UNLOCK(opl);

	return makedevice(pse_major, minor);
}

/*
 * STATIC dev_t
 * pse_freedev(size_t nbytes)
 *	Free a previously allocated minor device, and release the pages
 *	associated with it.
 *
 * Calling/Exit State:
 *	pse_lock is held neither on entry nor on exit.
 *
 *	On exit, the device may not be used again until it is
 *	returned by another call to pse_allocdev.  The pages
 *	formerly associated with the device are returned to
 *	the PSE page pool.
 */
STATIC void
pse_freedev(dev_t dev)
{
	int minor, i, npse;
	struct psedevinfo *psep;
	pl_t opl;
	ppid_t *ppid;

	ASSERT(dev != ENODEV);
	ASSERT(getemajor(dev) == pse_major);
	minor = geteminor(dev);
	ASSERT((0 <= minor) && (minor < pse_nminors));
	psep = &psedevinfo[minor];
	ASSERT((psep->psedev_npse > 0) && (psep->psedev_ppidp != NULL));
	ppid = psep->psedev_ppidp;
	npse = psep->psedev_npse;
	opl = PSE_LOCK();
	psep->psedev_ppidp = NULL;
	psep->psedev_npse = 0;
	for (i = 0 ; i < npse ; ++i) {
		ASSERT(ppid[i] != NOPAGE);
		pse_page_free(ppid[i]);
	}
	PSE_UNLOCK(opl);
	kmem_free((char *)ppid, npse * sizeof(ppid_t));
}

/*
 * vnode_t *
 * pse_makevp(size_t nbytes, size_t *actual)
 *	Allocate a device and a corresponding vnode, with nbytes worth of
 *	PSE pages, if nbytes is greater than or equal to PSE_PAGESIZE.
 *
 * Calling/Exit State:
 *	Return value is either NULL or a pointer to a vnode which
 *	can be used to map in nbytes worth of PSE pages.
 */
vnode_t *
pse_makevp(size_t nbytes, size_t *actual)
{
	dev_t dev;
	vnode_t *vp;

	if (!PSE_SUPPORTED() || (nbytes < (PSE_PAGESIZE / 4)))
		return (vnode_t *)NULL;
	dev = pse_allocdev(nbytes);
	if (dev == NODEV)
		return (vnode_t *)NULL;
	vp = makespecvp(dev, VCHR);
	if (vp == NULL)
		pse_freedev(dev);
	else
		*actual = roundup(nbytes, PSE_PAGESIZE);
	return vp;
}

/*
 * Declarations for the device driver:
 *
 *	pse_devflags		device flags for driver
 *
 *	pse_major		major number (from space.c)
 */
extern int pse_major;	/* major number */

int pse_devflag = D_MP;

extern int segpse_create(struct seg *, void *);
extern int segdev_create(struct seg *, void *);
extern void map_addr(vaddr_t *, uint_t, off_t, int);
extern void kpse_init(void);

/*
 * void
 * pse_init(void)
 *	Init routine for PSE pseudo device
 *
 * Calling/Exit State:
 *	None. Called during system startup.
 *
 * Description:
 *	Initializes the device allocator
 */
void
pse_init(void)
{

	pse_initdev();
	kpse_init();
}

/*
 * int pse_open(dev_t *, int, int, cred_t *)
 *	Open routine for PSE pseudo device.
 *
 * Calling/Exit State: 
 *	Return value is 0 if this is an existing device, ENODEV
 *	if not.
 *
 * Remarks:
 *	The expectation is that we get here through VOP_OPEN of
 *	a vnode created by pse_makevp.
 */
/*ARGSUSED*/
int
pse_open(dev_t *devp, int flag, int type, cred_t *cr)
{
	int minor;
	pl_t opl;

	ASSERT(getemajor(*devp) == pse_major);
	minor = geteminor(*devp);
	opl = PSE_LOCK();
	if ((0 <= minor) && (minor < pse_nminors) &&
			(psedevinfo[minor].psedev_npse != 0)) {
		ASSERT(psedevinfo[minor].psedev_ppidp != NULL);
		PSE_UNLOCK(opl);
		return 0;
	}
	PSE_UNLOCK(opl);
	return ENODEV;
}

/*
 * int
 * pse_close(dev_t, int, cred_t *)
 *      Close routine for PSE pseudo-device
 *
 * Calling/Exit State:
 *	None.
 *
 * Description:
 *	Calls freedev to free the device, and its associated pages.
 */
/*ARGSUSED1*/
int
pse_close(dev_t dev, int flag, cred_t *cr)
{

	pse_freedev(dev);
	return 0;
}

/*
 * int
 * psemmap(dev_t, off_t, uint_t)
 *      mmap routine for PSE psuedo-device
 *
 * Calling/Exit State:
 *	Returns ppid for mapping the requested offset for the
 *	device, or NOPAGE if no such ppid.
 *
 * Description:
 *	Split the byte offset into two pieces: btopse(off)
 *	gives the index into an array of 4MB-aligned ppid's
 *	associated with the device, and pnum(off) is added
 *	to the ppid to get the ppid for the specific page.
 */
/*ARGSUSED2*/
int
pse_mmap(dev_t dev, off_t off, uint_t prot)
{
	struct psedevinfo *psep;
	int minor = geteminor(dev);

	ASSERT((0 <= minor) && (minor < pse_nminors));
	psep = &psedevinfo[minor];
	if ((0 <= off) && (off < psetob(psep->psedev_npse))) {
		ASSERT(psep->psedev_ppidp != NULL);
		ASSERT(psep->psedev_ppidp[btopse(off)] != NOPAGE);
		return psep->psedev_ppidp[btopse(off)] + pnum(off);
	}
	return (NOPAGE);
}

/*
 * int
 * psesegmap(dev_t dev, off_t off, struct as *as, vaddr_t *addrp,
 *	 uint_t len, int_t prot, uint_t maxprot, uint_t flags, cred_t *fcred)
 *
 *	Establish a mapping.  Use the pse segment driver if possible
 *	and appropriate; otherwise, use seg_dev.
 *
 * Calling/Exit State:
 *	No lock is held on entry or at exit.
 *
 * Description:
 *	Creates a segpse mapping if certain criteria are met;
 *	otherwise, creates a segdev mapping.
 *
 * Remarks:
 *	This routine has two roles.  First, it is the segmap
 *	routine for the PSE pseudo-device.  Secondly, other
 *	drivers who wish to provide user-level PSE mappings
 *	may provide a segmap function which simply calls
 *	(or jumps to) this routine.
 */
/* ARGSUSED */
int
pse_segmap(dev_t dev, uint_t off, struct as *as, vaddr_t *addrp, uint_t len,
	uint_t prot, uint_t maxprot, uint_t flags, cred_t *cred)
{
	struct segdev_crargs args;
	int (*mapfunc)();
	int i, error = 0, align;
	int (*crfunc)(struct seg *, void *);

	if (PSE_SUPPORTED() && ((off & PSE_PAGEOFFSET) == 0) &&
			((len & PSE_PAGEOFFSET) == 0) &&
			(((flags & MAP_FIXED) == 0) ||
			((*addrp & PSE_PAGEOFFSET) == 0))) {
		crfunc = segpse_create;
		align = PSE_PAGESIZE;
	} else {
		crfunc = segdev_create;
		align = PAGESIZE;
	}

	if ((mapfunc = cdevsw[getmajor(dev)].d_mmap) == nodev)
		return ENODEV;

	/*
	 * Spec_segmap takes steps to ensure driver binding, but this
	 * routines doesn't need to, because this was called by a driver
	 * or as part of a driver.
	 */

	/*
	 * Character devices that support the d_mmap
	 * interface can only be mmap'ed shared.
	 */
	if ((flags & MAP_TYPE) != MAP_SHARED)
		return EINVAL;

	/*
	 * Check to ensure that the entire range is
	 * legal and we are not trying to map in
	 * more than the device will let us.
	 */
	for (i = 0; i < len; i += PAGESIZE) {
		if ((*mapfunc)(dev, off + i, maxprot) == (int)NOPAGE)
			return ENXIO;
	}

	if ((flags & MAP_FIXED) == 0) {
		/*
		 * Pick an address w/o worrying about
		 * any vac alignment contraints; allocate more
		 * than needed, then throw away the excess to ensure
		 * a 4MB aligned space.
		 */
		map_addr(addrp, len + align - PAGESIZE, (off_t)off, 0);
		if (*addrp == NULL) {
			crfunc = segdev_create;
			align = PAGESIZE;
			map_addr(addrp, len, (off_t)off, 0);
			if (*addrp == NULL)
				return ENOMEM;
		}
		*addrp = roundup(*addrp, align);
	} else {
		/*
		 * User-specified address; blow away any previous mappings.
		 */
		(void) as_unmap(as, *addrp, len);
	}

	args.mapfunc = mapfunc;
	args.dev = dev;
	args.offset = off;
	args.prot = (uchar_t)prot;
	args.maxprot = (uchar_t)maxprot;

	if (error = as_map(as, *addrp, len, crfunc, &args))
	        return error;
	return 0;
}

/*
 * void
 * pse_pagepool_init(void)
 *	Initialize physical and virtual allocator for PSE pages.
 *
 * Calling/Exit State:
 *	Called at startup
 */
void
pse_pagepool_init(void)
{
	/*
	 * initialize physical allocator
	 */
	pse_pagepool_create();

	/*
	 * initialize virtual allocator
	 */
	kpse_create();
}
