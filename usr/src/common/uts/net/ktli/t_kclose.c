/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kclose.c	1.6"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */


/*
 *	t_kclose.c, kernel TLI function to close a transport
 *	endpoint.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/ioctl.h>
#include <io/stropts.h>
#include <mem/kmem.h>
#include <fs/file.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>

/*
 * t_kclose(tiptr, callclosef)
 *	Close a transport endpoint.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0
 *
 * Description:
 *	This routine closes a transport endpoint. It simply
 *	frees the tiptr and calls closef if specified.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	callclosef		# to call closef() or not
 *	
 */
int
t_kclose(TIUSER *tiptr, int callclosef)
{
	struct	file	*fp;

	fp = tiptr->tp_fp;

	KTLILOG(0x100, "t_kclose: entered\n", 0);

	kmem_free((caddr_t)tiptr, (u_int)TIUSERSZ);
	if (callclosef) {
		/*
		 * closef() does not assume any locks held on entry
		 */
		(void)closef(fp);
	}

	return 0;
}
