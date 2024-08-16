/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/ktli/t_kgtstate.c	1.6"
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
 *
 *	t_kgtstate.c, kernel TLI function to get the state of a
 *	transport endpoint. 
 *
 */

#include <util/param.h>
#include <util/types.h>
#include <proc/proc.h>
#include <fs/file.h>
#include <proc/user.h>
#include <fs/vnode.h>
#include <svc/errno.h>
#include <io/stream.h>
#include <io/ioctl.h>
#include <io/stropts.h>
#include <io/strsubr.h>
#include <net/tihdr.h>
#include <net/timod.h>
#include <net/tiuser.h>
#include <net/ktli/t_kuser.h>

extern	int	strioctl(struct vnode *, int, int, int, int, cred_t *, int *);
extern	int	tlitosyserr(int);

/*
 * t_kgetstate(tiptr, state)
 *	Get the state of a transport endpoint.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns 0 on success and "state" is set to the current
 *	state or a positive error code.
 *
 * Description:
 *	This routine gets the state of a transport endpoint.
 *
 * Parameters:
 *
 *	tiptr			# open TLI descriptor
 *	state			# state is returned in this
 *	
 */
int
t_kgetstate(TIUSER *tiptr, int *state)
{
	struct T_info_ack	inforeq;
	struct strioctl		strioc;
	int 			retval;
	struct vnode		*vp;
	struct file		*fp;
	int			error;

	error = 0;
	retval = 0;
	fp = tiptr->tp_fp;
	vp = fp->f_vnode;

	if (state == NULL)
		return EINVAL;

	inforeq.PRIM_type = T_INFO_REQ;
	strioc.ic_cmd = TI_GETINFO;
	strioc.ic_timout = 0;
	strioc.ic_dp = (char *)&inforeq;
	strioc.ic_len = sizeof(struct T_info_req);

	error = strioctl(vp, I_STR, (int)&strioc, 0, K_TO_K,
				u.u_lwpp->l_cred, &retval);
	if (error) 
		return error;

	if (retval) {
		if ((retval & 0xff) == TSYSERR)
			error = (retval >> 8) & 0xff;
		else
			error = tlitosyserr(retval & 0xff);

		return error;
	}

	if (strioc.ic_len != sizeof(struct T_info_ack))
		return EPROTO;

	switch (inforeq.CURRENT_state) {

		case TS_UNBND:
			*state = T_UNBND;
			break;

		case TS_IDLE:
			*state = T_IDLE;
			break;

		case TS_WRES_CIND:
			*state = T_INCON;
			break;

		case TS_WCON_CREQ:
			*state = T_OUTCON;
			break;

		case TS_DATA_XFER:
			*state = T_DATAXFER;
			break;

		case TS_WIND_ORDREL:
			*state = T_OUTREL;
			break;

		case TS_WREQ_ORDREL:
			*state = T_INREL;
			break;

		default:
			error = EPROTO;
			break;
	}

	return error;
}
