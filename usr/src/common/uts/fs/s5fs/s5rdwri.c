/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/s5fs/s5rdwri.c	1.11"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#include <util/types.h>
#include <fs/buf.h>
#include <io/conf.h>
#include <util/debug.h>
#include <svc/errno.h>
#include <fs/file.h>
#include <util/param.h>
#include <mem/swap.h>
#include <util/sysmacros.h>
#include <proc/resource.h>
#include <svc/systm.h>
#include <io/uio.h>
#include <fs/vfs.h>
#include <fs/vnode.h>
#include <proc/disp.h>
#include <fs/s5fs/s5param.h>
#include <fs/s5fs/s5inode.h>
#include <fs/s5fs/s5data.h>
#include <fs/s5fs/s5macros.h>
#include <fs/s5fs/s5filsys.h>
#include <mem/seg_kmem.h>
#include <mem/seg_map.h>
#include <mem/seg.h>
#include <mem/page.h>
#include <util/cmn_err.h>
#include <mem/kmem.h>
#include <proc/user.h>
#include <proc/cred.h>
#include <acc/mac/mac.h>

extern int s5_itruncup(inode_t *, size_t);

/*
 * int
 * s5_readi(inode_t *ip, uio_t *uio, int ioflag)
 * 	Read the file corresponding to the supplied inode.
 *
 * Calling/Exit State:
 *	The calling LWP must hold the inode's rwlock in at least *shared*
 *	mode. This lock is must be acquired from above the VOP interface
 *	via VOP_RWRDLOCK() (below the VOP interface use s5rwlock).
 *
 *	A return value of 0 indicates success; othwerise a valid errno
 *	is returned. Errnos returned directly by this routine are:
 *		EINVAL	offset in <uio> is negative.
 *
 * Description:
 *	The VM segmap driver is used to establish a mapping for the
 *	vnode and offset to a kernel address space managed by the segmap
 *	driver. It then calls uiomove() to move data from the kernel
 *	address space to the calling process's buffer. Accessing the
 *	kernel address space causes the a fault which is handled
 *	by the segmap driver. The segmap driver calls VOP_GETPAGE in
 *	response to the fault where the pages are sought first in the
 *	page cache, and if necessary, reads them in from the file's
 *	backing store.
 *
 */
/* ARGSUSED */
int
s5_readi(inode_t *ip, uio_t *uiop, int ioflag)
{
	uint_t	mapon, n;
	int	error;
	off_t	off;
	vnode_t	*vp = ITOV(ip);
	long	oresid = uiop->uio_resid;
	int	diff;
	uint_t	flags;
	addr_t  base;
	pl_t	s;

        ASSERT((int)vp->v_type == VREG || (int)vp->v_type == VDIR ||
                (int)vp->v_type == VLNK);

	if (uiop->uio_resid == 0)
		return 0;

	off = uiop->uio_offset;

	if (off < 0)
		return EINVAL;

	if (WRITEALLOWED(vp, u.u_lwpp->l_cred)) {
		s = S5_ILOCK(ip);
		IMARK(ip, IACC);
		S5_IUNLOCK(ip, s);
	}

	error = 0;

	do {
		diff = ip->i_size - off;
		if (diff <= 0)
			break;

		mapon = off & MAXBOFFSET;
		n = MIN(MAXBSIZE - mapon, uiop->uio_resid);

		if (diff < n)
			n = diff;

		base = segmap_getmap(segkmap, vp, off, n, S_READ,
				     B_FALSE, NULL);

		if ((error = uiomove(base+mapon, n, UIO_READ, uiop)) == 0) {
			off = uiop->uio_offset;
			/*
			 * If we read to the end of the mapping or to
			 * EOF, we won't need these pages again soon.
			 */
			if (mapon + n == MAXBSIZE || off == ip->i_size)
				flags = SM_DONTNEED;
			else
				flags = 0;
			error = segmap_release(segkmap, base, flags);
		} else
			(void) segmap_release(segkmap, base, 0);
	} while (error == 0 && uiop->uio_resid > 0);

	/* check if it's a partial read, terminate without error */
  
	if (oresid != uiop->uio_resid)
		error = 0;
	
	return error;
}

/*
 * int
 * s5_writei(inode_t *ip, uio_t *uiop, int ioflag)
 *	 Write the file corresponding to the specified inode.
 *
 * Calling/Exit State:
 *	The caller holds the inode's rwlock *exclusive* to
 *	PREparation to change the file's contents/allocation
 *	information.
 *
 * Description:
 *
 *	The VM segmap driver is used to establish a mapping
 *	of the vnode, offset to a kernel address space managed
 *	by the segmap driver. It uses uiomove() to move data
 *	from the user's buffer to the kernel address space.
 */
/* ARGSUSED */
int
s5_writei(inode_t *ip, uio_t *uiop, int ioflag)
{
	vnode_t *vp = ITOV(ip);
	int bsize = VBSIZE(vp);
	int n, mapon;
	rlim_t limit = uiop->uio_limit;
	long oresid = uiop->uio_resid;
	addr_t base;
	uint_t flags;
	off_t off;
	boolean_t mustfault;
	int type, error;
	off_t lastalloc;
	pl_t s;

	ASSERT(vp->v_type == VREG || vp->v_type == VDIR ||
		vp->v_type == VLNK);

	if (uiop->uio_resid == 0)
		return (0);

	off = uiop->uio_offset;

	if (off < 0)
		return (EINVAL);

	type = ip->i_mode & IFMT;

	if (type == IFREG && uiop->uio_offset >= limit)
		return(EFBIG);

	error = 0;

	s = S5_ILOCK(ip);
	/* don't update access time in getpage */
	ip->i_flag |= INOACC;

	if (ioflag & IO_SYNC) {
		ip->i_flag |= ISYNC;
	}
	S5_IUNLOCK(ip, s);

	if (uiop->uio_offset > ip->i_size) {
		error = s5_itruncup(ip, uiop->uio_offset);
		if (error)
			goto out;
	}

	do {
		mapon = off & MAXBOFFSET;
		n = MIN(MAXBSIZE - mapon, uiop->uio_resid);

		/*
		 * If we are exceeding the ulimit or if we are 
		 * overflowing, clip the io size.
		 */
		if (type == IFREG && off + n >= limit || off + n <= 0) {
			if (off >= limit) {
				error = EFBIG;
				goto out;
			}
			n = limit - off;
		}

		if (off + n > ip->i_size) {
				lastalloc = roundup(ip->i_size, bsize);

			mustfault = ((lastalloc & PAGEOFFSET) != 0 &&
				     off + n > lastalloc);
		} else
			mustfault = B_FALSE;

		base = segmap_getmap(segkmap, vp, off, n, S_WRITE,
				     mustfault, NULL);

		error = uiomove(base + mapon, (long)n, UIO_WRITE, uiop);

		if (error) {
			off_t noff;
			ASSERT(uiop->uio_offset < off + n);
			/*
			 * If we had some sort of error during uiomove,
			 * call segmap_abort_create to have the pages
			 * aborted if we created them.
			 */
			noff = segmap_abort_create(segkmap,
					base, uiop->uio_offset,
					(off + n - uiop->uio_offset));

			if (noff != -1 && noff < uiop->uio_offset) {
				/*
				 * Some pages aborted, need to fix
				 * resid.
				 */
				uiop->uio_resid += uiop->uio_offset - noff;
				uiop->uio_offset = noff;
			}

			/*
			 * For synchronous writes, if any data was
			 * written, force the data to be flushed out.
			 */
			if (ioflag & IO_SYNC && uiop->uio_offset != off)
				flags = SM_WRITE;
			else
				flags = 0;
			(void) segmap_release(segkmap, base, flags);
		} else {

			flags = 0;
			/*
			 * Force write back for synchronous write cases.
			 */
			if (ioflag & IO_SYNC) {
				if (ip->i_swapcnt != 0 || IS_STICKY(ip)) {
					flags = SM_WRITE | SM_DONTNEED;
				} else {
					ip->i_flag |= ISYN;
					flags = SM_WRITE;
				}
			} else if (mapon + n == MAXBSIZE) {
				/*
				 * Have written a whole block.
				 * Start an asynchronous write and
				 * mark the buffer to indicate that
				 * it won't be needed again soon.
				 */
				flags = SM_WRITE | SM_ASYNC | SM_DONTNEED;
			}

			s = S5_ILOCK(ip);
			if (off + n > ip->i_size)
				ip->i_size = off + n;
			IMARK(ip, IUPD|ICHG);
			S5_IUNLOCK(ip, s);
			error = segmap_release(segkmap, base, flags);
		}

		off = uiop->uio_offset;

	} while (error == 0 && uiop->uio_resid > 0);

	s = S5_ILOCK(ip);
	ip->i_flag &= ~(INOACC | ISYNC);
	S5_IUNLOCK(ip, s);

out:
	/*
	 * If we've already done a partial-write, terminate
	 * the write but return no error.
	 */
	if (oresid != uiop->uio_resid)
		error = 0;

	return (error);
}

/*
 * int
 * s5_rdwri(enum uio_rw rw, inode_t *ip, caddr_t base, int len, off_t offset,
 *	    enum uio_seg seg, int ioflag, int *aresid)
 *
 * Description:
 * 	Package the arguments into a uio structure and invoke s5_readi()
 * 	or s5_writei(), as appropriate.
 *
 * Calling/Exit State:
 *	The caller must hold the inode's rwlock in at least *shared*
 *	mode if doing a read; *exclusive* mode must be specified
 *	when doing a write.
 */
int
s5_rdwri(enum uio_rw rw, inode_t *ip, caddr_t base, int len, off_t offset,
	enum uio_seg seg, int ioflag, int *aresid)
{
	uio_t	auio;
	iovec_t	aiov;
	int	error;

	aiov.iov_base = base;
	auio.uio_resid = aiov.iov_len = len;
	auio.uio_iov = &aiov;
	auio.uio_iovcnt = 1;
	auio.uio_offset = offset;
	auio.uio_segflg = (short)seg;
	auio.uio_limit = offset + NBPSCTR;
	if (rw == UIO_WRITE) {
		auio.uio_fmode = FWRITE;
		error = s5_writei(ip, &auio, ioflag);
	} else {
		auio.uio_fmode = FREAD;
		error = s5_readi(ip, &auio, ioflag);
	}
	if (aresid)
		*aresid = auio.uio_resid;
	return error;
}
