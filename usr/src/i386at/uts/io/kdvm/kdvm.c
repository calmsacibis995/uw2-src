/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386at:io/kdvm/kdvm.c	1.8"
#ident	"$Header: $"


#include <util/param.h>
#include <util/types.h>
#include <util/sysmacros.h>
#include <svc/errno.h>
#include <mem/kmem.h>
#include <util/cmn_err.h>
#include <io/uio.h>
#include <io/kd/kd.h>
#include <io/ws/ws.h>
#include <proc/cred.h>
#include <io/ddi.h>
#include <io/conf.h>


int kdvm_devflag = 0;			/* See comments in kdstr.c */


/*
 * int
 * kdvm_open(dev_t *, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
kdvm_open(dev_t *devp, int flag, int otyp, cred_t *cr)
{
	int	dev;
	int	error;

	if ((error = ws_getvtdev((dev_t *)&dev)) != 0)
		return (error);

	return (ws_open(dev, flag, otyp, cr));
}


/*
 * int
 * kdvm_close(dev_t, int, int, cred_t *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
kdvm_close(dev_t dev, int flag, int otyp, cred_t *cr)
{
	return (0);
}


/*
 * int
 * kdvm_ioctl(dev_t, int, int, int, cred_t *, int *)
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
kdvm_ioctl(dev_t dev, int cmd, int arg, int mode, cred_t *cr, int *rvalp)
{
	int	error;

	if ((error = ws_getvtdev(&dev)) != 0)
		return (error);

	return (ws_ioctl(dev, cmd, arg, mode, cr, rvalp));
}
