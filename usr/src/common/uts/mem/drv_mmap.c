/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:mem/drv_mmap.c	1.7"
#ident	"$Header: $"

#include <fs/pathname.h>
#include <fs/vnode.h>
#include <io/uio.h>
#include <io/conf.h>
#include <mem/as.h>
#include <mem/vmparam.h>
#include <proc/cred.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/ksynch.h>
#include <util/param.h>
#include <util/types.h>

/* Cached vnodes for /dev/kmem and /dev/mem */
STATIC vnode_t *devkmem_vp, *devmem_vp;

/*
 * int
 * drv_mmap(vaddr_t vaddr, paddr_t paddr, size_t len, vaddr_t *uvaddrp,
 *	    uint_t prot, uint_t maxprot, uint_t flags)
 *
 * drv_mmap() provides a convenient interface for setting up a user mapping
 * to a range of kernel virtual or physical addresses.
 *
 * One major use for this is the X/win queue data object that is shared
 * between the driver for a virtual terminal and the X server for that VT
 * (see code in io/ws/xque.c).
 *
 * Another use is for video frame buffers mapped via ioctls (instead of mmap)
 * for compatibility with pre-SVR4 applications.
 *
 * Calling/Exit State:
 *
 *	If the MAP_FIXED flag is given, (*uvaddrp) will be used as the user
 *	address at which to map the object; otherwise, an address will be
 *	chosen and placed into (*uvaddrp).
 *
 *	If vaddr is non-zero, it is the kernel address of the object to be
 *	mapped (and paddr is ignored); otherwise, paddr is the physical
 *	address of the object.
 *
 *	Beware that the mapping is at a page granularity.  You can pass in
 *	addresses and lengths which are not page-aligned, but the user will
 *	have access to entire pages.
 *
 *	This routine may block, so callers may not hold any spinlocks,
 *	and it may not be called from interrupt level.
 */
int
drv_mmap(vaddr_t vaddr, paddr_t paddr, size_t len, vaddr_t *uvaddrp,
	 uint_t prot, uint_t maxprot, uint_t flags)
{
	struct as *as = u.u_procp->p_as;
	vnode_t *vp;
	off_t off;
	int err;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(!servicing_interrupt());

	if (vaddr == 0) {
		/* physical mode */
		if (devmem_vp == (vnode_t *)NULL) {
			err = lookupname("/dev/mem", UIO_SYSSPACE, FOLLOW,
					 (vnode_t **)NULL, &devmem_vp);
			if (err)
				return err;
		}
		vp = devmem_vp;
		off = (off_t)paddr;
	} else {
		/* virtual mode */
		if (devkmem_vp == (vnode_t *)NULL) {
			err = lookupname("/dev/kmem", UIO_SYSSPACE, FOLLOW,
					 (vnode_t **)NULL, &devkmem_vp);
			if (err)
				return err;
		}
		vp = devkmem_vp;
		off = (off_t)vaddr;
	}

	/*
	 * Bind all the LWPs of the running process to a processor
	 * to which the driver is bound. This is only necessary on
	 * asymmetric systems where the device memory can only be
	 * accessed from a particular processor and a user process
	 * requests to map the device memory to its address space.
	 */
	if ((u.u_lwpp->l_cdevswp != NULL) &&
	    (u.u_lwpp->l_cdevswp->d_cpu != -1)) {
		if (err = bindproc(u.u_lwpp->l_cdevswp->d_cpu)) {
			/* Note: lookupname returns with a hold on the vnode. */
			VN_RELE(vp);
			return(err);
		}
	}

	ASSERT(!(flags & MAP_FIXED) || VALID_USR_RANGE(*uvaddrp, len));
	ASSERT(!(flags & MAP_FIXED) || \
		(*uvaddrp & PAGEOFFSET) == (off & PAGEOFFSET));
	ASSERT((prot & maxprot) == prot);

	/* Round to page boundaries */
	len = ptob(btopr(off + len) - btop(off));
	*uvaddrp &= PAGEMASK;	/* in case MAP_FIXED */

	err = VOP_MAP(vp, off & PAGEMASK, as, uvaddrp, len, prot, maxprot,
		      flags | MAP_SHARED, sys_cred);

	*uvaddrp += (off & PAGEOFFSET);

	return err;
}

/*
 * void
 * drv_munmap(vaddr_t uvaddr, size_t len)
 *	Release a mapping acquired by drv_mmap().
 *
 * Calling/Exit State:
 *	uvaddr is the address returned from drv_mmap() (in *uvaddrp).
 *	len is the length originally passed to drv_mmap().
 *
 *	This routine may block, so callers may not hold any spinlocks,
 *	and it may not be called from interrupt level.
 */
void
drv_munmap(vaddr_t uvaddr, size_t len)
{
	struct as *as = u.u_procp->p_as;

	ASSERT(KS_HOLD0LOCKS());
	ASSERT(!servicing_interrupt());

	as_wrlock(as);
	as_unmap(as, uvaddr & PAGEMASK,
		 ptob(btopr(uvaddr + len) - btop(uvaddr)));
	as_unlock(as);
	/*
	 * Unbind all the LWPs of a process which had earlier mapped
	 * the device memory to its address space.
	 */
	if (u.u_procp->p_bindnum != 0)
	        unbindproc();
}
