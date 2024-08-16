/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:fs/fbio.c	1.5"
#ident	"$Header: $"

#include <fs/fbuf.h>
#include <fs/vnode.h>
#include <mem/kmem.h>
#include <mem/seg.h>
#include <mem/seg_map.h>
#include <svc/errno.h>
#include <util/cmn_err.h>
#include <util/debug.h>
#include <util/param.h>
#include <util/types.h>

/*
 * Pseudo-bio routines which use a segmap mapping to address file data.
 */

/*
 * int
 * fbread(vnode_t *vp, off_t off, size_t len, enum seg_rw rw,
 *	  struct fbuf **fbpp)
 *
 * Calling/Exit State:
 *	Set (*fbpp)->fb_addr to a pointer to locked kernel virtual address for
 *	the given <vp, off>, for len bytes.  rw must be set to S_WRITE if the
 *	range is going to be written into; in that case, the entire range
 *	must be written.  The given range is not allowed to cross a MAXBSIZE
 *	boundary.  The fbread() must be matched by a subsequent fbrelse()
 *	when the mapping is no longer needed.
 *
 *	If rw == S_WRITE, the caller must ensure that there are no other
 *	accesses to this MAXBSIZE chunk.
 *
 *	Since fbread() calls segmap_getmap(), and the corresponding call to
 *	segmap_release() is in fbrelse(), the calling LWP may not call
 *	fbread() again until fbrelse() has been called.
 */
int
fbread(vnode_t *vp, off_t off, size_t len, enum seg_rw rw, struct fbuf **fbpp)
{
	void *smbuf;
	struct fbuf *fbp;
	int error;

	ASSERT((off & MAXBOFFSET) + len <= MAXBSIZE);

	smbuf = segmap_getmap(segkmap, vp, off, len, rw, B_TRUE, &error);
	if (smbuf == NULL)
		return error;

	fbp = kmem_alloc(sizeof(struct fbuf), KM_SLEEP);
	fbp->fb_addr = (char *)smbuf + (off & MAXBOFFSET);
	fbp->fb_count = len;
	*fbpp = fbp;
	return 0;
}

/*
 * int
 * fbrelse(struct fbuf *fbp, uint_t sm_flags)
 *	Release the fbp (obtained from fbread()).
 *
 * Calling/Exit State:
 *	sm_flags are passed to segmap_release(), and are used to control the
 *	disposition of the released pages.
 */
int
fbrelse(struct fbuf *fbp, uint_t sm_flags)
{
	vaddr_t addr = (vaddr_t)fbp->fb_addr;
	int err;

	err = segmap_release(segkmap, (void *)(addr & MAXBMASK), sm_flags);
	kmem_free(fbp, sizeof(struct fbuf));
	return err;
}
