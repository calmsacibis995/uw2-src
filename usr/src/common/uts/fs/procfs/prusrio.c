/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/procfs/prusrio.c	1.6"

#include <util/types.h>
#include <util/param.h>
#include <svc/errno.h>
#include <util/inline.h>
#include <proc/mman.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <fs/procfs/prdata.h>

#define	PRNIO	512

/*
 * int prusrio(proc_t *p, enum uio_rw rw, uio_t *uiop)
 * 	Perform I/O to/from a process.  Returns an errno or 0.
 *
 * Calling/Exit State:
 *	If the process is not stopped on an event of interest, the
 *	caller must have acquired the process reader-writer lock (via
 *	a call to pr_p_rdwr()); it remains held on exit.
 */
int
prusrio(proc_t *p, enum uio_rw rw, uio_t *uiop)
{
	vaddr_t addr, maxaddr;
	addr_t iaddr;
	register struct seg *seg;
	struct as *as;
	int count, asize, n, error = 0, didalloc = 0;
	enum seg_rw srw = (rw == UIO_WRITE) ? S_WRITE : S_READ;
	char ibuf[PRNIO];

	if ((as = p->p_as) == NULL)
		return EIO;

	as_rdlock(as);

	addr = uiop->uio_offset;

	/*
	 * Locate segment containing address of interest.  If the address
	 * is invalid, treat a write as an error and a read as a no-op so
	 * that zero (EOF) will be returned by the system call.
	 */
	if ((seg = as_segat(as, addr)) == NULL) {
		as_unlock(as);
		return (rw == UIO_WRITE) ? EIO : 0;
	}
more:
	maxaddr = seg->s_base + seg->s_size;

	/*
	 * Map in the necessary pages one at a time and copy in or out.
	 * Unfortunately read and write aren't symmetric because of the
	 * need to use an intermediate kernel buffer when the data is
	 * moved (since page faults are illegal when as_prmapin() is in
	 * effect).
	 *
	 * If more than PRNIO bytes are being moved, we kmem_alloc the
	 * space; otherwise we use a buffer on the stack.
	 */
	count = min(uiop->uio_resid, maxaddr - addr);
	if (count <= PRNIO) {
		asize = PRNIO;
		iaddr = ibuf;
		didalloc = 0;
	} else {
		asize = min(count, PAGESIZE);
		iaddr = kmem_alloc(asize, KM_SLEEP);
		didalloc = 1;
	}

	while (count > 0) {
		vaddr_t kvaddr;
		as_prmapcookie_t cookie;

		n = min(min(count, asize), PAGESIZE - (addr & PAGEOFFSET));
		if (rw == UIO_WRITE && (error = uiomove(iaddr, n, rw, uiop)))
			break;
		if (as_prmapin(as, addr, srw, &kvaddr, &cookie)) {
			error = EIO;
			break;
		}
		if (rw == UIO_WRITE)
			bcopy(iaddr, (addr_t)kvaddr, n);
		else
			bcopy((addr_t)kvaddr, iaddr, n);
		(void) as_prmapout(as, addr, kvaddr, cookie);
		if (rw == UIO_READ && (error = uiomove(iaddr, n, rw, uiop)))
			break;
		count -= n;
		addr += n;
	}

	if (didalloc) {
		kmem_free(iaddr, asize);
		didalloc = 0;
	}

	/*
	 * If we reached the end of the segment without exhausting
	 * the I/O count, see if there is another segment abutting
	 * the end of the previous segment.  Continue if so.
	 */
	if (error == 0 && uiop->uio_resid != 0) {
		addr = uiop->uio_offset;
		if ((seg = as_segat(as, addr)) != NULL)
			goto more;
	}

	as_unlock(as);
	if (didalloc)
		kmem_free(iaddr, asize);
	return error;
}		
