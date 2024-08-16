/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/svcauthesv.c	1.10"
#ident 	"$Header: $"

/*
 *	svcauthesv.c, esv flavor authentication parameters
 *	on the service side of rpc.
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
#include <net/rpc/auth_esv.h>
#include <net/rpc/svc_auth.h>
#include <net/rpc/token.h>

#ifdef RPCESV

extern void	bcopy(void *, void *, size_t);

/*
 * _svcauth_esv(rqst, msg)
 *	ESV authenticator for a rpc request.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns auth status in a auth_stat.
 *
 * Description:
 *	ESV authenticator for a rpc request.
 *
 * Parameters:
 *
 *	rqst			# service request
 *	msg			# rpc message
 */
enum auth_stat
_svcauth_esv(struct svc_req *rqst, struct rpc_msg *msg)
{
	enum	auth_stat	stat;
	XDR			xdrs;
	struct	authesv_parms	*auc;
	long			*buf;
	u_int			auth_len;
	int			str_len, gid_len;
	int			i;
	struct area {
		struct authesv_parms area_auc;
		char area_machname[MAX_ESVMACH_NAME+1];
		u_int area_gids[ESV_NGRPS];
	}			*area;

	/* LINTED pointer alignment */
	area = (struct area *) rqst->rq_clntcred;
	auc = &area->area_auc;
	auc->auc_machname = area->area_machname;
	auc->auc_gids = (gid_t *)area->area_gids;
	auth_len = (u_int)msg->rm_call.cb_cred.oa_length;

	/*
	 * first try the fast path
	 */
	xdrmem_create(&xdrs, msg->rm_call.cb_cred.oa_base, auth_len,XDR_DECODE);
	buf = XDR_INLINE(&xdrs, auth_len);
	if (buf != NULL) {
		/*
		 * we can do the fast path
		 */
		auc->auc_stamp = IXDR_GET_LONG(buf);	/* timestamp */
		str_len = IXDR_GET_U_LONG(buf);		/* machine name len */
		if (str_len > MAX_ESVMACH_NAME) {
			stat = AUTH_BADCRED;
			goto done;
		}
		bcopy((caddr_t)buf, auc->auc_machname,
					(u_int)str_len); /* machine name */
		auc->auc_machname[str_len] = 0;
		str_len = RNDUP(str_len);
		buf += str_len / sizeof (long);
		auc->auc_uid = IXDR_GET_LONG(buf);	/* user id */
		auc->auc_gid = IXDR_GET_LONG(buf);	/* group id */
		gid_len = IXDR_GET_U_LONG(buf);		/* num of groups */
		if (gid_len > ESV_NGRPS) {
			stat = AUTH_BADCRED;
			goto done;
		}
		auc->auc_len = gid_len;
		for (i = 0; i < gid_len; i++) {
			auc->auc_gids[i] = IXDR_GET_LONG(buf);
		}
		auc->auc_aid = IXDR_GET_LONG(buf);	/* additional id, pid */
		auc->auc_privs = (s_token)IXDR_GET_LONG(buf);
							/* priv vector */
		auc->auc_sens = (s_token)IXDR_GET_LONG(buf);
							/* sensitivity */
		auc->auc_info = (s_token)IXDR_GET_LONG(buf);
							/* info token */
		auc->auc_integ = (s_token)IXDR_GET_LONG(buf);
							/* integrity */
		auc->auc_ncs = (s_token)IXDR_GET_LONG(buf);
							/* nationality */
		/*
		 * 11 is the smallest unix credentials structure -
		 * timestamp, hostname len (0), uid, gid, gids len (0), and
		 * the 6 secure fields: auditid, privs, sens, info, integ, ncs.
		 */
		if ((11 + gid_len) * BYTES_PER_XDR_UNIT + str_len > auth_len) {

			cmn_err(CE_CONT, "bad auth_len gid %d str %d auth %d",
				gid_len, str_len, auth_len);

			stat = AUTH_BADCRED;
			goto done;
		}
	} else if (! xdr_authesv_parms(&xdrs, auc)) {
		xdrs.x_op = XDR_FREE;
		(void)xdr_authesv_parms(&xdrs, auc);
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

#endif /* RPCESV */
