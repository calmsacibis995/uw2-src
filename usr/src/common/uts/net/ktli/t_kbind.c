/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kbind.c	1.6"
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
 *	t_kbind.c, kernel TLI function to bind a transport endpoint
 *	to an address.
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
 * t_kbind(tiptr, req, ret);
 *	Bind a transport endpoint to an address.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success or a positive error value.
 *
 * Description:
 *	This function binds a transport endpoint to an address.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	req			# requested bind address if any
 *	ret			# bound address
 *	
 */
int 
t_kbind(TIUSER *tiptr, struct t_bind *req, struct t_bind *ret)
{
	struct T_bind_req	*bind_req;
	struct T_bind_ack	*bind_ack;
	int 			bindsz;
	struct vnode 		*vp;
	struct file 		*fp;
	char 			*buf;
	struct	 strioctl 	strioc;
	int	 		retval;
	int			error;

	error = 0;
	retval = 0;
	fp = tiptr->tp_fp;
	vp = fp->f_vnode;

	/*
	 * send the ioctl request downstream and wait
	 * for a reply.
	 */
	bindsz = max(TBINDREQSZ, TBINDACKSZ);
	bindsz += max(req == NULL ? 0 : req->addr.len , tiptr->tp_info.addr);
	buf = (char *)kmem_alloc(bindsz, KM_SLEEP);

	/* LINTED pointer alignment */
	bind_req = (struct T_bind_req *)buf;
	bind_req->PRIM_type = T_BIND_REQ;
	bind_req->ADDR_length = (req == NULL ? 0 : req->addr.len);
	bind_req->ADDR_offset = TBINDREQSZ;
	bind_req->CONIND_number = (req == NULL ? 0 : req->qlen);

	if (bind_req->ADDR_length)
		bcopy(req->addr.buf, buf+bind_req->ADDR_offset,
			bind_req->ADDR_length);


	strioc.ic_cmd = TI_BIND;
	strioc.ic_timout = 0;
	strioc.ic_dp = buf;
	strioc.ic_len = TBINDREQSZ+bind_req->ADDR_length;

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
	bind_ack = (struct T_bind_ack *)strioc.ic_dp;
	if (strioc.ic_len < TBINDACKSZ || bind_ack->ADDR_length == 0) {
		error = EIO;
		goto badbind;
	}

	/*
	 * copy bind data into users buffer
	 */
	if (ret) {
		if (ret->addr.maxlen > bind_ack->ADDR_length)
			ret->addr.len = bind_ack->ADDR_length;
		else	ret->addr.len = ret->addr.maxlen;

		bcopy(buf+bind_ack->ADDR_offset, ret->addr.buf,
			 ret->addr.len);

		ret->qlen = bind_ack->CONIND_number;
	}

badbind:
	kmem_free(buf, (u_int)bindsz);
	return error;
}
