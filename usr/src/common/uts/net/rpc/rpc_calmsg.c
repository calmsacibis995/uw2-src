/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/rpc_calmsg.c	1.9"
#ident 	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	rpc_callmsg.c, xdr routine to serialize or de-serialize
 *	a rpc call message.
 */

#include <util/param.h>
#include <util/types.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc_msg.h>
#include <net/inet/in.h>

extern void		bcopy(void *, void *, size_t);
extern bool_t		xdr_opaque_auth(XDR *, struct opaque_auth *);

/*
 * xdr_calmsg(xdrs, cmsg)
 *	XDR a call message
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	This routine XDRs a rpc call message. It basically
 *	serializes or deserializes all the stuff in the
 *	call message according to there order. See the
 *	definition of a call message and follow the order.
 *
 * Parameters:
 *
 *	xdrs			# stream to serialize into or deserialize from
 *	cmsg			# the call message
 *
 */
bool_t
xdr_callmsg(XDR *xdrs, struct rpc_msg *cmsg)
{
	long			*buf;
	struct	opaque_auth	*oa;

	if (xdrs->x_op == XDR_ENCODE) {
		if (cmsg->rm_call.cb_cred.oa_length > MAX_AUTH_BYTES) {
			return (FALSE);
		}
		if (cmsg->rm_call.cb_verf.oa_length > MAX_AUTH_BYTES) {
			return (FALSE);
		}
		buf = XDR_INLINE(xdrs, 8 * BYTES_PER_XDR_UNIT
			+ RNDUP(cmsg->rm_call.cb_cred.oa_length)
			+ 2 * BYTES_PER_XDR_UNIT
			+ RNDUP(cmsg->rm_call.cb_verf.oa_length));
		if (buf != NULL) {
			IXDR_PUT_LONG(buf, cmsg->rm_xid);
			IXDR_PUT_ENUM(buf, cmsg->rm_direction);
			if (cmsg->rm_direction != CALL) {
				return (FALSE);
			}
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_rpcvers);
			if (cmsg->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
				return (FALSE);
			}
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_prog);
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_vers);
			IXDR_PUT_LONG(buf, cmsg->rm_call.cb_proc);
			oa = &cmsg->rm_call.cb_cred;
			IXDR_PUT_ENUM(buf, oa->oa_flavor);
			IXDR_PUT_LONG(buf, oa->oa_length);
			if (oa->oa_length) {
				bcopy(oa->oa_base, (caddr_t)buf, oa->oa_length);
				buf += RNDUP(oa->oa_length) / sizeof (long);
			}
			oa = &cmsg->rm_call.cb_verf;
			IXDR_PUT_ENUM(buf, oa->oa_flavor);
			IXDR_PUT_LONG(buf, oa->oa_length);
			if (oa->oa_length) {
				bcopy(oa->oa_base, (caddr_t)buf, oa->oa_length);
			}
			return (TRUE);
		}
	}
	if (xdrs->x_op == XDR_DECODE) {
		buf = XDR_INLINE(xdrs, 8 * BYTES_PER_XDR_UNIT);
		if (buf != NULL) {
			cmsg->rm_xid = IXDR_GET_LONG(buf);
			cmsg->rm_direction = IXDR_GET_ENUM(buf, enum msg_type);
			if (cmsg->rm_direction != CALL) {
				return (FALSE);
			}
			cmsg->rm_call.cb_rpcvers = IXDR_GET_LONG(buf);
			if (cmsg->rm_call.cb_rpcvers != RPC_MSG_VERSION) {
				return (FALSE);
			}
			cmsg->rm_call.cb_prog = IXDR_GET_LONG(buf);
			cmsg->rm_call.cb_vers = IXDR_GET_LONG(buf);
			cmsg->rm_call.cb_proc = IXDR_GET_LONG(buf);
			oa = &cmsg->rm_call.cb_cred;
			oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
			oa->oa_length = IXDR_GET_LONG(buf);
			if (oa->oa_length) {
				if (oa->oa_length > MAX_AUTH_BYTES) {
					return (FALSE);
				}
				if (oa->oa_base == NULL) {
					oa->oa_base = (caddr_t)
					  kmem_alloc(oa->oa_length, KM_SLEEP);
				}
				buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
				if (buf == NULL) {
					if (xdr_opaque(xdrs, oa->oa_base,
						oa->oa_length) == FALSE) {
						return (FALSE);
					}
				} else {
					bcopy((caddr_t)buf, oa->oa_base,
						oa->oa_length);
				}
			}
			oa = &cmsg->rm_call.cb_verf;
			buf = XDR_INLINE(xdrs, 2 * BYTES_PER_XDR_UNIT);
			if (buf == NULL) {
				if (xdr_enum(xdrs, &oa->oa_flavor) == FALSE ||
					xdr_u_int(xdrs, &oa->oa_length)
						== FALSE) {
					return (FALSE);
				}
			} else {
				oa->oa_flavor = IXDR_GET_ENUM(buf, enum_t);
				oa->oa_length = IXDR_GET_LONG(buf);
			}
			if (oa->oa_length) {
				if (oa->oa_length > MAX_AUTH_BYTES) {
					return (FALSE);
				}
				if (oa->oa_base == NULL) {
					oa->oa_base = (caddr_t)
					  kmem_alloc(oa->oa_length, KM_SLEEP);
				}
				buf = XDR_INLINE(xdrs, RNDUP(oa->oa_length));
				if (buf == NULL) {
					if (xdr_opaque(xdrs, oa->oa_base,
						oa->oa_length) == FALSE) {
						return (FALSE);
					}
				} else {
					bcopy((caddr_t)buf, oa->oa_base,
						oa->oa_length);
				}
			}
			return (TRUE);
		}
	}
	if ( xdr_u_long(xdrs, &(cmsg->rm_xid)) &&
		xdr_enum(xdrs, (enum_t *)&(cmsg->rm_direction)) &&
		(cmsg->rm_direction == CALL) &&
		xdr_u_long(xdrs, &(cmsg->rm_call.cb_rpcvers)) &&
		(cmsg->rm_call.cb_rpcvers == RPC_MSG_VERSION) &&
		xdr_u_long(xdrs, &(cmsg->rm_call.cb_prog)) &&
		xdr_u_long(xdrs, &(cmsg->rm_call.cb_vers)) &&
		xdr_u_long(xdrs, &(cmsg->rm_call.cb_proc)) &&
		xdr_opaque_auth(xdrs, &(cmsg->rm_call.cb_cred)) ) {

		return (xdr_opaque_auth(xdrs, &(cmsg->rm_call.cb_verf)));
	}

	return (FALSE);
}
