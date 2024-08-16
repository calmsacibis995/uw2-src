/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/svc_gen.c	1.10"
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
 *	svc_generic.c, server side for rpc in the kernel.
 */

#include <util/param.h>
#include <util/types.h>
#include <net/rpc/types.h>
#include <net/inet/in.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/xti.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <fs/file.h>
#include <proc/user.h>
#include <proc/lwp.h>
#include <io/stream.h>
#include <net/tihdr.h>
#include <fs/fcntl.h>
#include <svc/errno.h>
 
extern void	poptimod(struct vnode *);

/*
 * svc_tli_create(fp, sendsz, nxprt)
 * 	Generic interface for creating a server side rpc
 *	transport handle.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success, errno of failure.
 *
 * Description:
 * 	Generic interface for creating a server side rpc
 *	transport handle. It opens the endpoint and calls
 *	the transport specific create function. Only
 *	connectionless transport is supported.
 *
 * Parameters:
 *
 *	fp			# file pointer for transport endpoint
 *	sendsz			# maximum sendsize
 *	nxprt			# 
 *	
 */
int
svc_tli_kcreate(struct file *fp, u_int sendsz, SVCXPRT **nxprt)
{
	SVCXPRT			*xprt = NULL;
	TIUSER			*tiptr = NULL;
	int			error;

	error = 0;

	if (fp == NULL || nxprt == NULL)
		return EINVAL;

	if ((error = t_kopen(fp, -1, FREAD|FWRITE|FNDELAY, &tiptr,
			u.u_lwpp->l_cred)) != 0) {

		RPCLOG(0x40, "svc_tli_kcreate: t_kopen: %d\n", error);

	 	return error;
	}

	/*
	 * call transport specific function.
	 */
	switch(tiptr->tp_info.servtype) {
		case T_CLTS:
			error = svc_clts_kcreate(tiptr, sendsz, &xprt);
			break;
		default:

			RPCLOG(0x40, "svc_tli_kcreate: Bad service type %d\n", 
				tiptr->tp_info.servtype);

			error = EINVAL;
			break;
	}

	if (error != 0)
		goto freedata;

	/*
	 * pop timod off of the stream, we apparently do not need it
	 */
	(void)poptimod(tiptr->tp_fp->f_vnode);

	/*
	 * this is needed to show that it is tli based. Switch
	 */
	xprt->xp_port = (u_short)-1;

	*nxprt = xprt;
	return 0;

freedata:
	if (xprt)
		SVC_DESTROY(xprt);
	return error;
}
