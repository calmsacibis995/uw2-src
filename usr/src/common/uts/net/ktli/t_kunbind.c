/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kunbind.c	1.6"
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
 *	t_kunbind.c, kernel TLI function to unbind a transport endpoint
 *	from an address.
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <proc/proc.h>
#include <fs/file.h>
#include <proc/user.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/strsubr.h>
#include <io/ioctl.h>
#include <io/stropts.h>
#include <fs/vnode.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>
#include <mem/kmem.h>

extern	int	strioctl(struct vnode *, int, int, int, int, cred_t *, int *);
extern	int	tlitosyserr(int);

/*
 * t_kunbind(tiptr)
 *	Unbind an address from a transport endpoint.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success else positive error code.
 *
 * Description:
 *	This routine unbinds an address from a transport endpoint.
 *
 * Parameters:
 *
 *	tiptr			# bound TLI descriptor
 *	
 */
int 
t_kunbind(TIUSER *tiptr)
{
	struct T_unbind_req	*unbind_req;
	struct T_ok_ack		*ok_ack;
	int			unbindsz;
	struct vnode		*vp;
	struct file		*fp;
	char			*buf;
	struct strioctl		strioc;
	int			retval;
	int			error;

	error = 0;
	retval = 0;
	fp = tiptr->tp_fp;
	vp = fp->f_vnode;

	/*
	 * send the ioctl request and wait
	 * for a reply.
	 */
	unbindsz = max(TUNBINDREQSZ, TOKACKSZ);
	buf = (char *)kmem_alloc(unbindsz, KM_SLEEP);
	/* LINTED pointer alignment */
	unbind_req = (struct T_unbind_req *)buf;
	unbind_req->PRIM_type = T_UNBIND_REQ;

	strioc.ic_cmd = TI_BIND;
	strioc.ic_timout = 0;
	strioc.ic_dp = buf;
	strioc.ic_len = TUNBINDREQSZ;

	error = strioctl(vp, I_STR, (int)&strioc, 0, K_TO_K,
				u.u_lwpp->l_cred, &retval);
	if (error)
		goto badbind;

	if (retval) {
		if ((retval & 0xff) == TSYSERR)
			error = (retval >> 8) & 0xff;
		else
			error = tlitosyserr(retval & 0xff);

		goto badbind;
	}

	/* LINTED pointer alignment */
	ok_ack = (struct T_ok_ack *)strioc.ic_dp;
	if (strioc.ic_len < TOKACKSZ || ok_ack->PRIM_type != T_UNBIND)
		error = EIO;

badbind:
	kmem_free(buf, (u_int)unbindsz);
	return error;
}
