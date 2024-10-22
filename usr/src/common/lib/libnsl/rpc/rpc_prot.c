/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpc_prot.c	1.2.10.2"
#ident	"$Header: $"

/*
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * rpc_prot.c
 *
 * This set of routines implements the rpc message definition,
 * its serializer and some common rpc utility routines.
 * The routines are meant for various implementations of rpc -
 * they are NOT for the rpc client or rpc service implementations!
 * Because authentication stuff is easy and is part of rpc, the opaque
 * routines are also in this program.
 */

#include <sys/param.h>
#include "trace.h"
#include <rpc/rpc.h>

/* * * * * * * * * * * * * * XDR Authentication * * * * * * * * * * * */

struct opaque_auth _null_auth;

/*
 * XDR an opaque authentication struct
 * (see auth.h)
 */
bool_t
xdr_opaque_auth(xdrs, ap)
	register XDR *xdrs;
	register struct opaque_auth *ap;
{
	bool_t dummy;

	trace1(TR_xdr_opaque_auth, 0);
	if (xdr_enum(xdrs, &(ap->oa_flavor))) {
		dummy = xdr_bytes(xdrs, &ap->oa_base,
			&ap->oa_length, MAX_AUTH_BYTES);
		trace1(TR_xdr_opaque_auth, 1);
		return (dummy);
	}
	trace1(TR_xdr_opaque_auth, 1);
	return (FALSE);
}

/*
 * XDR a DES block
 */
bool_t
xdr_des_block(xdrs, blkp)
	register XDR *xdrs;
	register des_block *blkp;
{
	bool_t dummy;

	trace1(TR_xdr_des_block, 0);
	dummy = xdr_opaque(xdrs, (caddr_t)blkp, sizeof (des_block));
	trace1(TR_xdr_des_block, 1);
	return (dummy);
}

/* * * * * * * * * * * * * * XDR RPC MESSAGE * * * * * * * * * * * * * * * */

/*
 * XDR the MSG_ACCEPTED part of a reply message union
 */
bool_t
xdr_accepted_reply(xdrs, ar)
	register XDR *xdrs;
	register struct accepted_reply *ar;
{
	bool_t dummy;

	/* personalized union, rather than calling xdr_union */
	trace1(TR_xdr_accepted_reply, 0);
	if (! xdr_opaque_auth(xdrs, &(ar->ar_verf))) {
		trace1(TR_xdr_accepted_reply, 1);
		return (FALSE);
	}
	if (! xdr_enum(xdrs, (enum_t *)&(ar->ar_stat))) {
		trace1(TR_xdr_accepted_reply, 1);
		return (FALSE);
	}

	switch (ar->ar_stat) {
	case SUCCESS:
		dummy = (*(ar->ar_results.proc))(xdrs, ar->ar_results.where);
		trace1(TR_xdr_accepted_reply, 1);
		return (dummy);

	case PROG_MISMATCH:
		if (! xdr_u_long(xdrs, &(ar->ar_vers.low))) {
			trace1(TR_xdr_accepted_reply, 1);
			return (FALSE);
		}
		dummy = xdr_u_long(xdrs, &(ar->ar_vers.high));
		trace1(TR_xdr_accepted_reply, 1);
		return (dummy);
	}
	trace1(TR_xdr_accepted_reply, 1);
	return (TRUE);  /* TRUE => open ended set of problems */
}

/*
 * XDR the MSG_DENIED part of a reply message union
 */
bool_t
xdr_rejected_reply(xdrs, rr)
	register XDR *xdrs;
	register struct rejected_reply *rr;
{
	bool_t dummy;

	/* personalized union, rather than calling xdr_union */
	trace1(TR_xdr_rejected_reply, 0);
	if (! xdr_enum(xdrs, (enum_t *)&(rr->rj_stat))) {
		trace1(TR_xdr_rejected_reply, 1);
		return (FALSE);
	}
	switch (rr->rj_stat) {
	case RPC_MISMATCH:
		if (! xdr_u_long(xdrs, &(rr->rj_vers.low))) {
			trace1(TR_xdr_rejected_reply, 1);
			return (FALSE);
		}
		dummy = xdr_u_long(xdrs, &(rr->rj_vers.high));
		trace1(TR_xdr_rejected_reply, 1);
		return (dummy);

	case AUTH_ERROR:
		dummy = xdr_enum(xdrs, (enum_t *)&(rr->rj_why));
		trace1(TR_xdr_rejected_reply, 1);
		return (dummy);
	}
	trace1(TR_xdr_rejected_reply, 1);
	return (FALSE);
}

/*
 * XDR a reply message
 */
bool_t
xdr_replymsg(xdrs, rmsg)
	register XDR *xdrs;
	register struct rpc_msg *rmsg;
{
	struct xdr_discrim reply_dscrm[3];
	register long *buf;
	register struct accepted_reply *ar;
	register struct opaque_auth *oa;
	bool_t	dummy;

	trace1(TR_xdr_replymsg, 0);
	if (xdrs->x_op == XDR_ENCODE &&
	    rmsg->rm_reply.rp_stat == MSG_ACCEPTED &&
	    rmsg->rm_direction == REPLY &&
	    (buf = XDR_INLINE(xdrs, 6 * BYTES_PER_XDR_UNIT +
		    rmsg->rm_reply.rp_acpt.ar_verf.oa_length)) != NULL) {
		IXDR_PUT_LONG(buf, rmsg->rm_xid);
		IXDR_PUT_ENUM(buf, rmsg->rm_direction);
		IXDR_PUT_ENUM(buf, rmsg->rm_reply.rp_stat);
		ar = &rmsg->rm_reply.rp_acpt;
		oa = &ar->ar_verf;
		IXDR_PUT_ENUM(buf, oa->oa_flavor);
		IXDR_PUT_LONG(buf, oa->oa_length);
		if (oa->oa_length) {
			(void) memcpy((caddr_t)buf, oa->oa_base, oa->oa_length);
			buf += (oa->oa_length + BYTES_PER_XDR_UNIT - 1) /
				sizeof (long);
		}
		/*
		 * stat and rest of reply, copied from xdr_accepted_reply
		 */
		IXDR_PUT_ENUM(buf, ar->ar_stat);
		switch (ar->ar_stat) {
		case SUCCESS:
			dummy = (*(ar->ar_results.proc))
				(xdrs, ar->ar_results.where);
			trace1(TR_xdr_replymsg, 1);
			return (dummy);

		case PROG_MISMATCH:
			if (! xdr_u_long(xdrs, &(ar->ar_vers.low))) {
				trace1(TR_xdr_replymsg, 1);
				return (FALSE);
			}
			dummy = xdr_u_long(xdrs, &(ar->ar_vers.high));
			trace1(TR_xdr_replymsg, 1);
			return (dummy);
		}
		trace1(TR_xdr_replymsg, 1);
		return (TRUE);
	}
	if (xdrs->x_op == XDR_DECODE &&
	    (buf = XDR_INLINE(xdrs, 3 * BYTES_PER_XDR_UNIT)) != NULL) {
		rmsg->rm_xid = IXDR_GET_LONG(buf);
		rmsg->rm_direction = IXDR_GET_ENUM(buf, enum msg_type);
		if (rmsg->rm_direction != REPLY) {
			trace1(TR_xdr_replymsg, 1);
			return (FALSE);
		}
		rmsg->rm_reply.rp_stat = IXDR_GET_ENUM(buf, enum reply_stat);
		if (rmsg->rm_reply.rp_stat != MSG_ACCEPTED) {
			if (rmsg->rm_reply.rp_stat == MSG_DENIED) {
				dummy = xdr_rejected_reply(xdrs,
					&rmsg->rm_reply.rp_rjct);
				trace1(TR_xdr_replymsg, 1);
				return (dummy);
			}
			trace1(TR_xdr_replymsg, 1);
			return (FALSE);
		}
		ar = &rmsg->rm_reply.rp_acpt;
		oa = &ar->ar_verf;
		buf = XDR_INLINE(xdrs, 2 * BYTES_PER_XDR_UNIT);
		if (buf != NULL) {
			oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
			oa->oa_length = IXDR_GET_LONG(buf);
		} else {
			if (xdr_enum(xdrs, &oa->oa_flavor) == FALSE ||
			    xdr_u_int(xdrs, &oa->oa_length) == FALSE) {
				trace1(TR_xdr_replymsg, 1);
				return (FALSE);
			}
		}
		if (oa->oa_length) {
			if (oa->oa_length > MAX_AUTH_BYTES) {
				trace1(TR_xdr_replymsg, 1);
				return (FALSE);
			}
			if (oa->oa_base == NULL) {
				oa->oa_base = (caddr_t)
					mem_alloc(oa->oa_length);
			}
			buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
			if (buf == NULL) {
				if (xdr_opaque(xdrs, oa->oa_base,
				    oa->oa_length) == FALSE) {
					trace1(TR_xdr_replymsg, 1);
					return (FALSE);
				}
			} else {
				(void) memcpy(oa->oa_base,
					(caddr_t)buf, oa->oa_length);
			}
		}
		/*
		 * stat and rest of reply, copied from
		 * xdr_accepted_reply
		 */
		if (! xdr_enum(xdrs, (enum_t *)&ar->ar_stat)) {
			trace1(TR_xdr_replymsg, 1);
			return (FALSE);
		}
		switch (ar->ar_stat) {
		case SUCCESS:
			dummy = (*(ar->ar_results.proc))
				(xdrs, ar->ar_results.where);
			trace1(TR_xdr_replymsg, 1);
			return (dummy);

		case PROG_MISMATCH:
			if (! xdr_u_long(xdrs, &(ar->ar_vers.low))) {
				trace1(TR_xdr_replymsg, 1);
				return (FALSE);
			}
			dummy = xdr_u_long(xdrs, &(ar->ar_vers.high));
			trace1(TR_xdr_replymsg, 1);
			return (dummy);
		}
		trace1(TR_xdr_replymsg, 1);
		return (TRUE);
	}

	reply_dscrm[0].value = (int)MSG_ACCEPTED;
	reply_dscrm[0].proc = (xdrproc_t) xdr_accepted_reply;
	reply_dscrm[1].value = (int)MSG_DENIED;
	reply_dscrm[1].proc =  (xdrproc_t) xdr_rejected_reply;
	reply_dscrm[2].value = __dontcare__;
	reply_dscrm[2].proc = NULL_xdrproc_t;
	if (xdr_u_long(xdrs, &(rmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)&(rmsg->rm_direction)) &&
	    (rmsg->rm_direction == REPLY)) {
		dummy = xdr_union(xdrs, (enum_t *)&(rmsg->rm_reply.rp_stat),
				(caddr_t)&(rmsg->rm_reply.ru),
				reply_dscrm, NULL_xdrproc_t);
		trace1(TR_xdr_replymsg, 1);
		return (dummy);
	}
	trace1(TR_xdr_replymsg, 1);
	return (FALSE);
}

/*
 * Serializes the "static part" of a call message header.
 * The fields include: rm_xid, rm_direction, rpcvers, prog, and vers.
 * The rm_xid is not really static, but the user can easily munge on the fly.
 */
bool_t
xdr_callhdr(xdrs, cmsg)
	register XDR *xdrs;
	register struct rpc_msg *cmsg;
{
	bool_t dummy;

	trace1(TR_xdr_callhdr, 0);
	cmsg->rm_direction = CALL;
	cmsg->rm_call.cb_rpcvers = RPC_MSG_VERSION;
	if (xdrs->x_op == XDR_ENCODE &&
	    xdr_u_long(xdrs, &(cmsg->rm_xid)) &&
	    xdr_enum(xdrs, (enum_t *)&(cmsg->rm_direction)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
	    xdr_u_long(xdrs, &(cmsg->rm_call.cb_prog))) {
	    dummy = xdr_u_long(xdrs, &(cmsg->rm_call.cb_vers));
	    trace1(TR_xdr_callhdr, 1);
	    return (dummy);
	}
	trace1(TR_xdr_callhdr, 1);
	return (FALSE);
}

/* ************************** Client utility routine ************* */

static void
accepted(acpt_stat, error)
	register enum accept_stat acpt_stat;
	register struct rpc_err *error;
{
	trace1(TR_accepted, 0);
	switch (acpt_stat) {

	case PROG_UNAVAIL:
		error->re_status = RPC_PROGUNAVAIL;
		trace1(TR_accepted, 1);
		return;

	case PROG_MISMATCH:
		error->re_status = RPC_PROGVERSMISMATCH;
		trace1(TR_accepted, 1);
		return;

	case PROC_UNAVAIL:
		error->re_status = RPC_PROCUNAVAIL;
		trace1(TR_accepted, 1);
		return;

	case GARBAGE_ARGS:
		error->re_status = RPC_CANTDECODEARGS;
		trace1(TR_accepted, 1);
		return;

	case SYSTEM_ERR:
		error->re_status = RPC_SYSTEMERROR;
		trace1(TR_accepted, 1);
		return;

	case SUCCESS:
		error->re_status = RPC_SUCCESS;
		trace1(TR_accepted, 1);
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_ACCEPTED;
	error->re_lb.s2 = (long)acpt_stat;
	trace1(TR_accepted, 1);
}

static void
rejected(rjct_stat, error)
	register enum reject_stat rjct_stat;
	register struct rpc_err *error;
{

	trace1(TR_rejected, 0);
	switch (rjct_stat) {
	case RPC_MISMATCH:
		error->re_status = RPC_VERSMISMATCH;
		trace1(TR_rejected, 1);
		return;

	case AUTH_ERROR:
		error->re_status = RPC_AUTHERROR;
		trace1(TR_rejected, 1);
		return;
	}
	/* something's wrong, but we don't know what ... */
	error->re_status = RPC_FAILED;
	error->re_lb.s1 = (long)MSG_DENIED;
	error->re_lb.s2 = (long)rjct_stat;
	trace1(TR_rejected, 1);
}

/*
 * given a reply message, fills in the error
 */
void
_seterr_reply(msg, error)
	register struct rpc_msg *msg;
	register struct rpc_err *error;
{
	/* optimized for normal, SUCCESSful case */
	trace1(TR___seterr_reply, 0);
	switch (msg->rm_reply.rp_stat) {
	case MSG_ACCEPTED:
		if (msg->acpted_rply.ar_stat == SUCCESS) {
			error->re_status = RPC_SUCCESS;
			trace1(TR___seterr_reply, 1);
			return;
		};
		accepted(msg->acpted_rply.ar_stat, error);
		break;

	case MSG_DENIED:
		rejected(msg->rjcted_rply.rj_stat, error);
		break;

	default:
		error->re_status = RPC_FAILED;
		error->re_lb.s1 = (long)(msg->rm_reply.rp_stat);
		break;
	}

	switch (error->re_status) {
	case RPC_VERSMISMATCH:
		error->re_vers.low = msg->rjcted_rply.rj_vers.low;
		error->re_vers.high = msg->rjcted_rply.rj_vers.high;
		break;

	case RPC_AUTHERROR:
		error->re_why = msg->rjcted_rply.rj_why;
		break;

	case RPC_PROGVERSMISMATCH:
		error->re_vers.low = msg->acpted_rply.ar_vers.low;
		error->re_vers.high = msg->acpted_rply.ar_vers.high;
		break;
	}
	trace1(TR___seterr_reply, 1);
}
