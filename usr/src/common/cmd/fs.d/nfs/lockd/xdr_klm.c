/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)nfs.cmds:lockd/xdr_klm.c	1.3"
#ident  "$Header: $"

/*
 *  		PROPRIETARY NOTICE (Combined)
 *  
 *  This source code is unpublished proprietary information
 *  constituting, or derived under license from AT&T's Unix(r) System V.
 *  In addition, portions of such source code were derived from Berkeley
 *  4.3 BSD under license from the Regents of the University of
 *  California.
 *  
 *  
 *  
 *  		Copyright Notice 
 *  
 *  Notice of copyright on this source code product does not indicate 
 *  publication.
 *  
 *  	(c) 1986,1987,1988,1989  Sun Microsystems, Inc.
 *  	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 *  	          All rights reserved.
 */

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <rpc/rpc.h>
#include "klm_prot.h"

bool_t
xdr_klm_stats(xdrs, objp)
	XDR *xdrs;
	klm_stats *objp;
{

	register long *buf;

	if (!xdr_enum(xdrs, (enum_t *)objp)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_klm_lock(xdrs, objp)
	XDR *xdrs;
	klm_lock *objp;
{

	register long *buf;


	if (xdrs->x_op == XDR_ENCODE) {
		if (!xdr_string(xdrs, &objp->server_name, LM_MAXSTRLEN)) {
			return (FALSE);
		}
		if (!xdr_netobj(xdrs, &objp->fh)) {
			return (FALSE);
		}
		buf = XDR_INLINE(xdrs,10 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			if (!xdr_int(xdrs, &objp->base)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->length)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->type)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->granted)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->color)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->LockID)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->pid)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->class)) {
				return (FALSE);
			}
			if (!xdr_long(xdrs, &objp->rsys)) {
				return (FALSE);
			}
			if (!xdr_long(xdrs, &objp->rpid)) {
				return (FALSE);
			}
		} else {
			IXDR_PUT_LONG(buf,objp->base);
			IXDR_PUT_LONG(buf,objp->length);
			IXDR_PUT_LONG(buf,objp->type);
			IXDR_PUT_LONG(buf,objp->granted);
			IXDR_PUT_LONG(buf,objp->color);
			IXDR_PUT_LONG(buf,objp->LockID);
			IXDR_PUT_LONG(buf,objp->pid);
			IXDR_PUT_LONG(buf,objp->class);
			IXDR_PUT_LONG(buf,objp->rsys);
			IXDR_PUT_LONG(buf,objp->rpid);
		}

		return (TRUE);
	} else if (xdrs->x_op == XDR_DECODE) {
		if (!xdr_string(xdrs, &objp->server_name, LM_MAXSTRLEN)) {
			return (FALSE);
		}
		if (!xdr_netobj(xdrs, &objp->fh)) {
			return (FALSE);
		}
		buf = XDR_INLINE(xdrs,10 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			if (!xdr_int(xdrs, &objp->base)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->length)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->type)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->granted)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->color)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->LockID)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->pid)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->class)) {
				return (FALSE);
			}
			if (!xdr_long(xdrs, &objp->rsys)) {
				return (FALSE);
			}
			if (!xdr_long(xdrs, &objp->rpid)) {
				return (FALSE);
			}
		} else {
			objp->base = IXDR_GET_LONG(buf);
			objp->length = IXDR_GET_LONG(buf);
			objp->type = IXDR_GET_LONG(buf);
			objp->granted = IXDR_GET_LONG(buf);
			objp->color = IXDR_GET_LONG(buf);
			objp->LockID = IXDR_GET_LONG(buf);
			objp->pid = IXDR_GET_LONG(buf);
			objp->class = IXDR_GET_LONG(buf);
			objp->rsys = IXDR_GET_LONG(buf);
			objp->rpid = IXDR_GET_LONG(buf);
		}
		return(TRUE);
	}

	if (!xdr_string(xdrs, &objp->server_name, LM_MAXSTRLEN)) {
		return (FALSE);
	}
	if (!xdr_netobj(xdrs, &objp->fh)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->base)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->length)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->type)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->granted)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->color)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->LockID)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->pid)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->class)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->rsys)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->rpid)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_klm_holder(xdrs, objp)
	XDR *xdrs;
	klm_holder *objp;
{

	 register long *buf;


	if (xdrs->x_op == XDR_ENCODE) {
		buf = XDR_INLINE(xdrs,11 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			if (!xdr_bool(xdrs, &objp->exclusive)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->base)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->length)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->type)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->granted)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->color)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->LockID)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->pid)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->class)) {
				return (FALSE);
			}
			if (!xdr_long(xdrs, &objp->rsys)) {
				return (FALSE);
			}
			if (!xdr_long(xdrs, &objp->rpid)) {
				return (FALSE);
			}
		} else {
			IXDR_PUT_BOOL(buf,objp->exclusive);
			IXDR_PUT_LONG(buf,objp->base);
			IXDR_PUT_LONG(buf,objp->length);
			IXDR_PUT_LONG(buf,objp->type);
			IXDR_PUT_LONG(buf,objp->granted);
			IXDR_PUT_LONG(buf,objp->color);
			IXDR_PUT_LONG(buf,objp->LockID);
			IXDR_PUT_LONG(buf,objp->pid);
			IXDR_PUT_LONG(buf,objp->class);
			IXDR_PUT_LONG(buf,objp->rsys);
			IXDR_PUT_LONG(buf,objp->rpid);
		}

		return (TRUE);
	} else if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE(xdrs,11 * BYTES_PER_XDR_UNIT);
		if (buf == NULL) {
			if (!xdr_bool(xdrs, &objp->exclusive)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->base)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->length)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->type)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->granted)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->color)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->LockID)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->pid)) {
				return (FALSE);
			}
			if (!xdr_int(xdrs, &objp->class)) {
				return (FALSE);
			}
			if (!xdr_long(xdrs, &objp->rsys)) {
				return (FALSE);
			}
			if (!xdr_long(xdrs, &objp->rpid)) {
				return (FALSE);
			}

		} else {
			objp->exclusive = IXDR_GET_BOOL(buf);
			objp->base = IXDR_GET_LONG(buf);
			objp->length = IXDR_GET_LONG(buf);
			objp->type = IXDR_GET_LONG(buf);
			objp->granted = IXDR_GET_LONG(buf);
			objp->color = IXDR_GET_LONG(buf);
			objp->LockID = IXDR_GET_LONG(buf);
			objp->pid = IXDR_GET_LONG(buf);
			objp->class = IXDR_GET_LONG(buf);
			objp->rsys = IXDR_GET_LONG(buf);
			objp->rpid = IXDR_GET_LONG(buf);
		}
		return (TRUE);
	}

	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->base)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->length)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->type)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->granted)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->color)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->LockID)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->pid)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, &objp->class)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->rsys)) {
		return (FALSE);
	}
	if (!xdr_long(xdrs, &objp->rpid)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_klm_stat(xdrs, objp)
	XDR *xdrs;
	klm_stat *objp;
{

	register long *buf;

	if (!xdr_klm_stats(xdrs, &objp->stat)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_klm_testrply(xdrs, objp)
	XDR *xdrs;
	klm_testrply *objp;
{

	register long *buf;

	if (!xdr_klm_stats(xdrs, &objp->stat)) {
		return (FALSE);
	}
	switch (objp->stat) {
	case klm_denied:
		 if (!xdr_klm_holder(xdrs, &objp->klm_testrply_u.holder)) {
			 return (FALSE);
		 }
		break;
	}
	return (TRUE);
}

bool_t
xdr_klm_lockargs(xdrs, objp)
	XDR *xdrs;
	klm_lockargs *objp;
{

	register long *buf;

	if (!xdr_bool(xdrs, &objp->block)) {
		return (FALSE);
	}
	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_klm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_klm_testargs(xdrs, objp)
	XDR *xdrs;
	klm_testargs *objp;
{

	register long *buf;

	if (!xdr_bool(xdrs, &objp->exclusive)) {
		return (FALSE);
	}
	if (!xdr_klm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_klm_unlockargs(xdrs, objp)
	XDR *xdrs;
	klm_unlockargs *objp;
{

	register long *buf;

	if (!xdr_klm_lock(xdrs, &objp->alock)) {
		return (FALSE);
	}
	return (TRUE);
}
