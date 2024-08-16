/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/svc_authu.c	1.10"
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
 *	svc_authu.c, unix flavor authentication parameters
 *	on the service side of rpc.
 *
 *	There are two svc auth implementations here: AUTH_UNIX and
 *	AUTH_SHORT.
 *
 *	_svcauth_unix does full blown unix style uid, gid+gids auth,
 *	_svcauth_short uses a shorthand auth to index into a cache
 *	of longhand auths.
 *
 *	NOTE: the shorthand auth has been gutted for efficiency.
 */

#include <util/param.h>
#include <util/types.h>
#include <util/cmn_err.h>
#include <svc/time.h>
#include <net/inet/in.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc_msg.h>
#include <net/xti.h>
#include <net/tihdr.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <net/rpc/auth_unix.h>
#include <net/rpc/svc_auth.h>

extern void	bcopy(void *, void *, size_t);

/*
 * _svcauth_unix(rqst, msg)
 *	Unix longhand authenticator for a rpc request.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns auth status in a auth_stat.
 *
 * Description:
 *	Unix longhand authenticator for a rpc request.
 *
 * Parameters:
 *
 *	rqst			# service request
 *	msg			# rpc message
 */
enum auth_stat
_svcauth_unix(struct svc_req *rqst, struct rpc_msg *msg)
{
	enum	auth_stat	stat;
	XDR			xdrs;
	struct	authunix_parms	*aup;
	long			*buf;
	u_int			auth_len;
	int			str_len, gid_len;
	int			i;
	struct area {
		struct authunix_parms area_aup;
		char area_machname[MAX_MACHINE_NAME+1];
		u_int area_gids[NGRPS];
	}			*area;

	/* LINTED pointer alignment */
	area = (struct area *) rqst->rq_clntcred;
	aup = &area->area_aup;
	aup->aup_machname = area->area_machname;
	aup->aup_gids = (gid_t *)area->area_gids;
	auth_len = (u_int)msg->rm_call.cb_cred.oa_length;
	xdrmem_create(&xdrs, msg->rm_call.cb_cred.oa_base, auth_len,XDR_DECODE);

	/*
 	 * first try the fast path for xdr
	 */
	buf = XDR_INLINE(&xdrs, auth_len);
	if (buf != NULL) {
		/*
		 * we can do the fast path
		 */
		aup->aup_time = IXDR_GET_LONG(buf);	/* timestamp */
		str_len = IXDR_GET_U_LONG(buf);		/* machine name len */
		if (str_len > MAX_MACHINE_NAME) {
			stat = AUTH_BADCRED;
			goto done;
		}
		bcopy((caddr_t)buf, aup->aup_machname,
					(u_int)str_len);/* machine name */
		aup->aup_machname[str_len] = 0;
		str_len = RNDUP(str_len);
		buf += str_len / sizeof (long);
		aup->aup_uid = IXDR_GET_LONG(buf);	/* user id */
		aup->aup_gid = IXDR_GET_LONG(buf);	/* group id */
		gid_len = IXDR_GET_U_LONG(buf);		/* # of groups */
		if (gid_len > NGRPS) {
			stat = AUTH_BADCRED;
			goto done;
		}
		aup->aup_len = gid_len;
		for (i = 0; i < gid_len; i++) {		/* all the groups */
			aup->aup_gids[i] = IXDR_GET_LONG(buf);
		}

		/*
		 * five is the smallest unix credentials structure -
		 * timestamp, hostname len (0), uid, gid, and gids len (0).
		 */
		if ((BASE_CREDSZ + gid_len) * BYTES_PER_XDR_UNIT + str_len
						> auth_len) {

			cmn_err(CE_CONT, "bad auth_len gid %d str %d auth %d",
				gid_len, str_len, auth_len);

			stat = AUTH_BADCRED;
			goto done;
		}
	} else if (! xdr_authunix_parms(&xdrs, aup)) {
		xdrs.x_op = XDR_FREE;
		(void)xdr_authunix_parms(&xdrs, aup);
		stat = AUTH_BADCRED;
		goto done;
	}

	rqst->rq_xprt->xp_verf.oa_flavor = AUTH_NULL;
	rqst->rq_xprt->xp_verf.oa_length = 0;
	stat = AUTH_OK;
done:
	XDR_DESTROY(&xdrs);

	return (stat);
}

/*
 * _svcauth_short(rqst, msg)
 *	Unix shorthand authenticator for a rpc request.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns auth status in a auth_stat.
 *
 * Description:
 *	Unix shorthand authenticator for a rpc request.
 *	Looks up longhand in a cache. has been gutted
 *	for efficiency, always returns rejected cred.
 *
 * Parameters:
 *
 *	rqst			# service request
 *	msg			# rpc message
 */
/*ARGSUSED*/
enum auth_stat 
_svcauth_short(struct svc_req *rqst, struct rpc_msg *msg)
{
	return (AUTH_REJECTEDCRED);
}
