/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:io/lockstat/lockstat.c	1.1"

#include <util/types.h>
#include <util/ksynch.h>
#include <proc/cred.h>
#include <io/uio.h>
#include <io/conf.h>
#include <svc/errno.h>

/*
 * General notes:
 *	- 	Not a bound driver.
 *	- 	Only the ioctl does anything useful.
 *	-	Optionally installed along with lockstat command.
 *	-	Too small to make it worth the effort of unloading?
 *	-	Do not envision any use for it in a production system,
 *		with _MPSTATS undefined. 
 */

int	lkstdevflag = D_MP;

/*
 * int lkstopen(dev_t *devp, int flag, int type, struct cred *cr)
 *	Open routine for lockstat. Does nothing.
 * 
 * Calling/Exit State:
 *	Standard args to all driver open()/close() entry points.
 */

/* ARGSUSED */
int
lkstopen(dev_t devp, int flag, int type, struct cred *cr)
{
	return 0;
}

/*
 * int lkstclose(dev_t *devp, int flag, struct cred *cr)
 *	Open routine for lockstat. Does nothing.
 * 
 * Calling/Exit State:
 *	Standard args to all driver open()/close() entry points.
 */

/* ARGSUSED */
int
lkstclose(dev_t dev, int flag, struct cred *cr)
{
	return 0;
}

/*
 * int lkstioctl(dev_t dev,int cmd, int arg, int flag, struct cred *cr,
 * 		int *rvalp)
 * 	The only reason for existence. Called to initialize the statistics
 *	blocks. May expand to optionally clear other areas where statistics
 *	for locks are collected -- for example, in plocal structure, we're
 *	currently collecting peak information for lock hold times.
 *
 * Calling/Exit State:
 *	Standard arguments to all driver ioctl entry points. Caller
 *	should not hold lks_mutex.
 */

/* ARGSUSED */
int
lkstioctl(dev_t dev, int cmd, int arg, int flag, struct cred *cr, int *rvalp)
{

	if (cmd == 0) {
		blk_statzero_all();
	}
	return 0;
}

/*
 * int lkstread(dev_t dev,  struct uio *uiop, struct cred *cr)
 *	Nothing to read. May change to read some structures that cannot
 *	be read conveniently through /dev/kmem.
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
lkstread(dev_t dev, struct uio *uiop, struct cred *cr)
{
	return ENXIO; 
}

/*
 * int lkstwrite(dev_t dev,  struct uio *uiop, struct cred *cr)
 *	Nothing to write, so write nothing.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
lkstwrite(dev_t dev, struct uio *uiop, struct cred *cr)
{
	return ENXIO;
}



