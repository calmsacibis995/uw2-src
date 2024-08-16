/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/rpc_subr.c	1.10"
#ident 	"$Header: $"

/*
 *	rpc_subr.c, miscellaneous support routines for kernel
 *	implementation of rpc.
 */

#include	<util/param.h>
#include	<util/types.h>
#include	<util/cmn_err.h>
#include	<net/rpc/types.h>
#include	<fs/vnode.h>
#include	<io/stream.h>
#include	<io/stropts.h>
#include	<io/strsubr.h>
#include	<proc/cred.h>
#include	<proc/proc.h>
#include	<proc/user.h>

extern int	strioctl(struct vnode *, int, int, int, int, cred_t *, int *);

/*
 * poptimod(vp)
 *	 Pop TIMOD off the stream.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns a void.
 *
 * Description:
 *	This routine pops the timod streams module off
 *	of the stream vp.
 *
 * Parameters:
 *
 *	vp			# vnode of stream to pop timod off of
 *
 */
void
poptimod(struct vnode *vp)
{
	int error, isfound, ret;

	error = strioctl(vp, I_FIND, (int)"timod", 0, K_TO_K,
					u.u_lwpp->l_cred, &isfound);
	if (error) {
		RPCLOG((ulong)1, "poptimod: I_FIND strioctl error %d\n", error);
		return;
	}
	if (isfound != 0) {
		error = strioctl(vp, I_POP, (int)"timod", 0, K_TO_K,
					u.u_lwpp->l_cred, &ret);
		if (error)
			RPCLOG((ulong)1,
			"poptimod: I_POP strioctl error %d\n", error);
	}
}

#ifdef DEBUG

int		rpclog = 0;

/*
 * rpc_log(level, str, a1)
 *	RPC Kernel debugging aid.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Always returns 0.
 *
 * Description:
 *	The global variable "rpclog" is a bit mask which
 *	allows various types of debugging messages to be
 *	printed out.
 *
 *	rpclog & 0x1
 *	rpclog & 0x2
 *	rpclog & 0x4
 *	rpclog & 0x8		will print stuff from key_call.c
 *	rpclog & 0x10		will print stuff from des client and server
 *	rpclog & 0x20		will print stuff from clnt_gen.c
 *	rpclog & 0x40		will print stuff from svc_gen.c
 *	rpclog & 0x80		will print stuff from clnt_clts.c
 *	rpclog & 0x100		for future use in clnt_clts.c
 *	rpclog & 0x200		for future use in clnt_clts.c
 *	rpclog & 0x400		will print stuff from svc_clts.c
 *	rpclog & 0x800		will print errors from svc_clts.c
 *	rpclog & 0x1000		for future use in svc_clts.c
 *	rpclog & 0x2000		will print stuff from svc.c
 *	rpclog & 0x4000
 *	rpclog & 0x8000
 *	rpclog & 0x10000
 *
 * Parameters:
 *	
 *	level			# what all to print
 *	str			# string to print
 *	a1			# value to print
 */
int
rpc_log(u_long level, char *str, int a1)
{
	if (level & rpclog)
		cmn_err(CE_CONT, str, a1);

	return(0);
}

#endif /* DEBUG */
