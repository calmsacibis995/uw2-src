/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/physio.c	1.43"
#ident	"$Header: $"

#include <fs/buf.h>
#include <io/conf.h>
#include <io/uio.h>
#include <mem/as.h>
#include <mem/faultcode.h>
#include <mem/hat.h>
#include <mem/kmem.h>
#include <mem/page.h>
#include <mem/seg.h>
#include <mem/seg_kmem.h>
#include <mem/seg_vn.h>
#include <mem/tuneable.h>
#include <mem/uas.h>
#include <proc/mman.h>
#include <proc/proc.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/inline.h>
#include <util/param.h>
#include <util/plocal.h>
#include <util/metrics.h>
#include <util/sysmacros.h>
#include <util/types.h>
#include <util/var.h>

#ifndef NO_RDMA
#include <mem/rdma.h>
#endif

extern int uiophysio(void (*strat)(), buf_t *, dev_t, int, uio_t *);
extern int buscheck(struct buf *);

/*
 * int physiock(void (*strat)(), buf_t *bp, dev_t dev, int rw, daddr_t devsize,
 *		uio_t *uiop)
 * 	DDI function - performs raw device I/O on block devices.
 *
 * Calling/Exit State:
 *	The arguments are
 *		- the strategy routine for the device
 *		- a buffer, which is usually NULL, or else a special buffer
 *		  header owned exclusively by the device for this purpose
 *		- the device number
 *		- read/write flag (B_READ or B_WRITE).
 *		  B_READ indicates reading from the device and B_WRITE
 *		  indicates writing to the device.
 *		- size of the device (in blocks); 0 if unlimited.
 *		- uio structure containing the I/O parameters
 *
 *	No spin locks held by the caller.
 *	Returns 0 on success, or a non-zero errno on failure.
 *
 * Description:
 *	Calls uiophysio() to do the real work after verifying the offsets
 *	against the device size limit.
 */
int
physiock(void (*strat)(), buf_t *bp, dev_t dev, int rw, daddr_t devsize,
	 uio_t *uiop) 
{
	if (devsize != 0) {
		struct iovec *iov;
		size_t cnt;
		ulong_t limit;
		struct a {
			int	fdes;
			char	*cbuf;
			unsigned count;
		} *uap; /* arg list for read/write system call */

		/*
		 * Determine if offset is at or past end of device 
		 * if past end of device return ENXIO. Also, if at end 
		 * of device and writing, return ENXIO. If at end of device
		 * and reading, nothing more to read -- return 0
		 * If past end of device and reading -- return ENXIO
		 */
		limit = devsize << SCTRSHFT;
		if ((ulong_t)uiop->uio_offset >= limit) {
			if ((ulong_t)uiop->uio_offset > limit || rw == B_WRITE)
				return ENXIO;
			return 0;
		}

		/*
		 * Adjust count of request so that it does not take I/O past
		 * end of device.
		 */
		limit -= (ulong_t)uiop->uio_offset;
	
		for (cnt = 0, iov = uiop->uio_iov;
		     cnt++ < uiop->uio_iovcnt;
		     iov++) {
			if ((ulong_t)iov->iov_len > limit) {
				iov->iov_len = limit;
				uiop->uio_iovcnt = cnt;
				break;
			}
			limit -= (ulong_t)iov->iov_len;
		}
	}

	/*
	 * Request in uio has been adjusted to fit device size, if necessary.
	 * Now perform a uiophysio() and return the error number it returns.
	 */

	return uiophysio(strat, bp, dev, rw, uiop);
}

/*
 * int uiophysio(void (*strat)(), buf_t *bp, dev_t dev, int rw, uio_t *uio)
 * 	Raw I/O. If the user has the proper access privileges, the pages of the
 *	user address space involved in the I/O faulted in and softlocked.
 *	After the completion of the I/O, the above pages are unlocked.
 *
 *
 * Calling/Exit State:
 * 	The arguments are:
 *		The strategy routine for the device,
 *		a buffer, which is usually NULL, or else a special buffer
 *		  header owned exclusively by the device for this purpose
 *		the device number, Read/write flag and uio struct.
 *	No Spin locks should be held by the caller.
 */
int
uiophysio(void (*strat)(), buf_t *bp, dev_t dev, int rw, uio_t *uio)
{
	void (*cstrat)();
	struct iovec *iov;
	int hpf, error = 0;
	major_t maj;
	int devflag;
	faultcode_t fault_err;
	proc_t *procp;
	struct as *asp;
	vaddr_t	addr;
	u_int len;

#if (B_READ == 0)
#error uiophysio() code assumes B_READ != 0
#endif
#if ((MAXBIOSIZE % NBPSCTR) != 0)
#error uiophysio() code assumes MAXBIOSIZE is a multiple of NBPSCTR
#endif

	ASSERT(rw == B_READ || rw == B_WRITE);

	maj = getmajor(dev);
	ASSERT(maj >= cdevcnt || cdevsw[maj].d_flag != NULL);
	if (maj >= cdevcnt)
		devflag = D_DMA;
	else
		devflag = *cdevsw[maj].d_flag;

	if ((uio->uio_offset % NBPSCTR) != 0 && !(devflag & D_BLKOFF))
		return EINVAL;

	ASSERT(m.mets_wait.msw_physio >= 0);
	MET_PHYSIOWAIT(1);
	if (rw)
		MET_PHREAD();
	else
		MET_PHWRITE();

	hpf = (bp == NULL);	/* Has the caller supplied a buffer header? */
	if (hpf)
		bp = getrbuf(KM_SLEEP);	/* no, get one */

	ASSERT(bp->b_flags & B_KERNBUF);
	bp->b_flags = ((bp->b_flags & ~(B_READ|B_ERROR)) | B_PHYS | rw);
	bp->b_edev = dev;
	bp->b_odev = _cmpdev(dev);

	if (uio->uio_segflg == UIO_USERSPACE) {
		procp = u.u_procp;
		asp = procp->p_as;
	} else {
		asp = &kas;
		procp = NULL;
		bp->b_flags &= ~B_PHYS;
	}

	while (uio->uio_iovcnt > 0) {
		iov = uio->uio_iov;
		bp->b_error = 0;
		bp->b_proc = procp;

		while (iov->iov_len > 0) {
			if (uio->uio_resid == 0)
				break;
			bp->b_blkno = btodt(uio->uio_offset);
			bp->b_blkoff = uio->uio_offset % NBPSCTR;
			/*
			 * Don't count on b_addr remaining untouched by the
			 * code below (it may be reset because someone does
			 * a bp_mapin on the buffer) -- reset from the iov
			 * each time through, updating the iov's base address
			 * instead.
			 */
			bp->b_un.b_addr = iov->iov_base;
			addr = (vaddr_t)iov->iov_base;
			len = MIN(iov->iov_len, uio->uio_resid);
			/*
			 * We breakup RAW I/O using MAXBIOSIZE as a governor.
			 */
			if (len > MAXBIOSIZE)
				len = MAXBIOSIZE;
			bp->b_bufsize = bp->b_bcount = len;
			as_rdlock(asp);		/* get the as read lock */
			fault_err = as_fault(asp, addr, len, F_SOFTLOCK,
					  rw == B_READ? S_WRITE : S_READ);
			if (fault_err != 0) {
				/*
				 * Either  the range of addresses were
				 * invalid and had incorrect permissions,
				 * or we couldn't lock down all the pages for
				 * the access we needed. (e.g. we needed to
				 * allocate filesystem blocks for
				 * rw == B_READ but the file system was full).
				 */
				as_unlock(asp);
				if (FC_CODE(fault_err) == FC_OBJERR)
					error = FC_ERRNO(fault_err);
				else
					error = EFAULT;
				bp->b_flags = (bp->b_flags | B_ERROR) & ~B_PHYS;
				bp->b_error = error;
				break;
			}
			if (buscheck(bp) < 0) {
				/*
				 * The I/O request crossed illegal pages.
				 */
				bp->b_resid = len;
				bioerror(bp, EFAULT);
				biodone(bp);
				error = biowait(bp);
			} else {
				bp->b_flags &= ~B_DONE;
				cstrat = PHYSIO_START(bp, strat);
#ifndef NO_RDMA
				if (rdma_mode != RDMA_DISABLED &&
				    (devflag & D_DMA)) {
					buf_breakup(cstrat, bp, &rdma_dflt_bcb);
				} else
#endif
					(*cstrat)(bp);

				error = biowait(bp);
				PHYSIO_DONE(bp, addr, len);
			}
			if (as_fault(asp, addr, len, F_SOFTUNLOCK,
			  rw == B_READ ? S_WRITE : S_READ) != 0)
				/*
				 *+ Cannot soft unlock the previously locked
				 *+ pages.
				 */
				cmn_err(CE_PANIC, "physio unlock");
			as_unlock(asp);

			/* Advance the iov pointers */
			len -= bp->b_resid;
			if (len != 0) {
				error = 0;  /* No error if any transferred */
				iov->iov_base += len;
				iov->iov_len -= len;
				uio->uio_resid -= len;
				uio->uio_offset += len;
			}
			if (bp->b_resid || error)
				break;
		}
		if (bp->b_resid || error)
			break;
		uio->uio_iov++;
		uio->uio_iovcnt--;
	}

	if (hpf) {
		/* return the buffer header we allocated */ 
		freerbuf(bp);
	} else {
		bp->b_flags &= ~B_PHYS;
		bp->b_proc = NULL;
	}

	ASSERT(m.mets_wait.msw_physio);
	MET_PHYSIOWAIT(-1);
	return error;
}


/*
 * physreq_t *
 * physreq_alloc(int flags)
 *	Allocate a physreq_t structure.
 *
 * Calling/Exit State:
 *	If flags is KM_NOSLEEP, NULL will be returned if allocation fails;
 *	otherwise, this routine may block, so no locks may be held on entry.
 */
physreq_t *
physreq_alloc(int flags)
{
	physreq_t *preqp;

	ASSERT((flags & ~KM_NOSLEEP) == 0);

	preqp = kmem_zalloc(sizeof(physreq_t), flags);
	if (preqp != NULL)
		preqp->phys_align = 1;
	return preqp;
}

/*
 * void
 * physreq_free(physreq_t *preqp)
 *	Free a physreq_t structure.
 *
 * Calling/Exit State:
 *	preqp must be a physreq_t structure returned by physreq_alloc
 */
void
physreq_free(physreq_t *preqp)
{
	kmem_free(preqp, sizeof(physreq_t));
}

/*
 * boolean_t
 * physreq_prep(physreq_t *preqp, int flags)
 *	Prepare a physreq_t for use.
 *
 * Calling/Exit State:
 *	Must be called after all fields in preqp have been set but before
 *	this physreq_t is passed to any I/O or allocation routine (e.g.
 *	buf_breakup, kmem_alloc_physcontig).
 *
 *	flags is KM_SLEEP or KM_NOSLEEP.
 *	Return value is B_FALSE if the physreq cannot be successfully
 *	prepped (either due to allocation failure in KM_NOSLEEP case or
 *	due to unsupportable values).
 *
 *	It is legal to call physreq_prep multiple times on the same physreq_t.
 */
/* ARGSUSED */
boolean_t
physreq_prep(physreq_t *preqp, int flags)
{
	extern boolean_t brkup_prep(physreq_t *, int);

	ASSERT((flags & ~KM_NOSLEEP) == 0);

#ifdef DEBUG
	preqp->phys_flags |= PREQ_PREPPED;
#endif

	if (!brkup_prep(preqp, flags)) {
#ifdef DEBUG
		preqp->phys_flags &= ~PREQ_PREPPED;
#endif
		return B_FALSE;
	}

	return B_TRUE;
}

/*
 * boolean_t
 * physreq_met(const void *buf, size_t len, const physreq_t *preqp)
 *	Determine whether or not a range of memory meets the criteria
 *	specified by preqp.
 *
 * Calling/Exit State:
 *	Returns B_TRUE if the range of memory meets the physreq requirements,
 *	else returns B_FALSE.
 */
boolean_t
physreq_met(const void *buf, size_t len, const physreq_t *preqp)
{
	paddr_t boundary, align_mask;
	boolean_t do_physcontig, do_phys;
	const char *addr;
	paddr_t paddr, base_paddr, contig_addr;
	uint_t pgrem;
#ifndef NO_RDMA
	int rdma_requirement;
#endif

	/* the physreq must already be prepped */
	ASSERT(preqp->phys_flags & PREQ_PREPPED);
	/* align must be non-zero and a power of 2 */
	ASSERT(preqp->phys_align != 0);
	ASSERT((preqp->phys_align & (preqp->phys_align - 1)) == 0);
	/* boundary must be zero or a power of 2 */
	ASSERT(preqp->phys_boundary == 0 ||
	       (preqp->phys_boundary & (preqp->phys_boundary - 1)) == 0);

	if (len == 0)
		return B_TRUE;

	boundary = preqp->phys_boundary;
	align_mask = preqp->phys_align - 1;
	do_physcontig = (preqp->phys_flags & PREQ_PHYSCONTIG);

	do_phys = (do_physcontig || (boundary | align_mask));

#ifndef NO_RDMA
	switch (rdma_requirement = RDMA_REQUIREMENT(preqp)) {
	case RDMA_IMPOSSIBLE:
		return B_FALSE;
	case RDMA_REQUIRED:
		do_phys = B_TRUE;
		break;
	default:
		ASSERT(RDMA_REQUIREMENT(preqp) == RDMA_NOTREQUIRED);
		break;
	}
#endif /* !NO_RDMA */

	if (do_phys) {
		addr = buf;
		pgrem = PAGESIZE - ((vaddr_t)addr & PAGEOFFSET);
		contig_addr = ~0;

		for (;;) {
			paddr = kvtophys(addr);
			if (paddr != contig_addr) {
				if ((paddr & align_mask) != 0)
					return B_FALSE;
				if (addr != buf) {
					if (do_physcontig)
						return B_FALSE;
					if (boundary && boundary -
					     (base_paddr & (boundary - 1)) <
					    contig_addr - base_paddr)
					return B_FALSE;
				}
				base_paddr = paddr;
			}
#ifndef NO_RDMA
			if (rdma_requirement == RDMA_REQUIRED &&
			    !DMA_BYTE(paddr))
				return B_FALSE;
#endif /* !NO_RDMA */

			if (len <= pgrem)
				break;

			contig_addr = paddr + pgrem;
			addr += pgrem;
			len -= pgrem;
			pgrem = PAGESIZE;
		}

		if (boundary && boundary - (base_paddr & (boundary - 1)) <
		     (paddr + len) - base_paddr)
			return B_FALSE;
	}

	return B_TRUE;
}
