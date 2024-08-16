/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern-i386sym:svc/sysi86_p.c	1.1"
#ident	"$Header: $"

/*
 * Platform-specific sysi86() system call subfunctions.
 */

#include <svc/errno.h>
#include <svc/sysi86.h>
#include <svc/systm.h>


/*
 * int
 * sysi86_p(struct sysi86a *uap, rval_t *rvp)
 *	Platform-specific extensions to sysi86 syscall.
 *
 * Calling/Exit State:
 *	None.
 */
/* ARGSUSED */
int
sysi86_p(struct sysi86a *uap, rval_t *rvp)
{
	/*
	 * No extensions supported for this platform.
	 */
	return EINVAL;
}
