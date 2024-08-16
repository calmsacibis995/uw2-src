/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:io/async/async.c	1.2"

#include <svc/errno.h>
#include <sys/types.h>
#include <sys/cred.h>

/*
 * The async I/O driver is unsupported on this platform.
 */

int
aio_open(dev_t *dev_p, int flags, int otype, struct cred *cred_p)
{
	return ENOSYS;
}

int
aio_close(dev_t dev, int flag, int otyp, cred_t *cred_p)
{
	return ENOSYS;
}

int
aio_ioctl(dev_t dev, int cmd, int arg, int mode, struct cred *cred_p,
	  int *rval_p)
{
	return ENOSYS;
}

void aio_intersect() { }
void aio_as_free() { }
