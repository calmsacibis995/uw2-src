/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:mem/rdma_p.c	1.2"
#ident	"$Header: $"

#ifndef NO_RDMA

#include <fs/buf.h>
#include <fs/file.h>
#include <fs/specfs/snode.h>
#include <fs/vnode.h>
#include <io/conf.h>
#include <proc/cred.h>
#include <svc/errno.h>
#include <svc/systm.h>
#include <sys/types.h>
#include <util/param.h>

/*
 * boolean_t
 * rdma_root(void)
 *	Determine if the root device has restricted DMA capabilities. If so,
 *	return B_TRUE.
 *
 * Calling/Exit State:
 *	Called from rdma_convert(), which is called from main() just
 *	before the vfs_mountroot().
 */
boolean_t
rdma_root(void)
{
	bcb_t *bcbp;
	vnode_t *devvp;
	dev_t rdev;
	int error;
	boolean_t ret = B_FALSE;

	/*
	 * If no root device is present, then report that the root device
	 * does not require restricted DMA.
	 */
	if (rootdev == NODEV)
		return B_FALSE;

	/*
	 * Open the device.
	 */
	devvp = makespecvp(rootdev, VBLK);
	error = VOP_OPEN(&devvp, FREAD, sys_cred);
	if (error) {
		VN_RELE(devvp);
		return B_FALSE;
	}
	rdev = devvp->v_rdev;

	/*
	 * If the driver doesn't supply a bcb, then we assume that the device
	 * requires 24-bit DMA support.  Otherwise, we decide based on the
	 * phys_dmasize from the bcb.
	 */
	if (bdevsw[getmajor(rdev)].d_devinfo(rdev, DI_BCBP, &bcbp) ||
	    bcbp == NULL || bcbp->bcb_physreqp->phys_dmasize < 32)
		ret = B_TRUE;

	/*
	 * Close the device.
	 */
	(void) VOP_CLOSE(devvp, FREAD, 1, 0, sys_cred);
	VN_RELE(devvp);

	return ret;
}
#endif /* NO_RDMA */
