/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/svc_auth_sys.c	1.1.11.4"
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
 * svc_auth_sys.c
 *
 * Handles UNIX flavor authentication parameters on the service side of rpc.
 * There are two svc auth implementations here: AUTH_SYS and AUTH_SHORT.
 * _svcauth_sys does full blown unix style uid, gid+gids auth,
 * _svcauth_short uses a shorthand auth to index into a cache of
 *	longhand auths.
 * Note: the shorthand has been gutted for efficiency.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include <sys/syslog.h>
#include <sys/types.h>
#include "trace.h"

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  256
#endif

/*
 * System (Unix) longhand authenticator
 */
enum auth_stat
_svcauth_sys(rqst, msg)
	register struct svc_req *rqst;
	register struct rpc_msg *msg;
{
	register enum auth_stat stat;
	XDR xdrs;
	register struct authsys_parms *aup;
	register long *buf;
	struct area {
		struct authsys_parms area_aup;
		char area_machname[MAXHOSTNAMELEN+1];
		gid_t area_gids[NGRPS];
	} *area;
	u_int auth_len;
	int str_len, gid_len;
	register int i;

	trace1(TR___svcauth_sys, 0);
	area = (struct area *) rqst->rq_clntcred;
	aup = &area->area_aup;
	aup->aup_machname = area->area_machname;
	aup->aup_gids = area->area_gids;
	auth_len = (u_int)msg->rm_call.cb_cred.oa_length;
	xdrmem_create(&xdrs, msg->rm_call.cb_cred.oa_base, auth_len,
			XDR_DECODE);
	buf = XDR_INLINE(&xdrs, auth_len);
	if (buf != NULL) {
		aup->aup_time = IXDR_GET_LONG(buf);
		str_len = IXDR_GET_U_LONG(buf);
		if (str_len > MAXHOSTNAMELEN) {
			stat = AUTH_BADCRED;
			goto done;
		}
		(void) memcpy(aup->aup_machname, (caddr_t)buf, str_len);
		aup->aup_machname[str_len] = 0;
		str_len = RNDUP(str_len);
		buf += str_len / sizeof (long);
		aup->aup_uid = IXDR_GET_LONG(buf);
		aup->aup_gid = IXDR_GET_LONG(buf);
		gid_len = IXDR_GET_U_LONG(buf);
		if (gid_len > NGRPS) {
			stat = AUTH_BADCRED;
			goto done;
		}
		aup->aup_len = gid_len;
		for (i = 0; i < gid_len; i++) {
			aup->aup_gids[i] = (gid_t) IXDR_GET_LONG(buf);
		}
		/*
		 * five is the smallest unix credentials structure -
		 * timestamp, hostname len (0), uid, gid, and gids len (0).
		 */
		if ((5 + gid_len) * BYTES_PER_XDR_UNIT + str_len > auth_len) {
			(void) syslog(LOG_ERR,
			    gettxt("uxnsl:91",
			    "_svcauth_sys: bad auth_len gid=%d str=%d auth=%d"),
			    gid_len, str_len, auth_len);
			stat = AUTH_BADCRED;
			goto done;
		}
	} else if (! xdr_authsys_parms(&xdrs, aup)) {
		xdrs.x_op = XDR_FREE;
		(void) xdr_authsys_parms(&xdrs, aup);
		stat = AUTH_BADCRED;
		goto done;
	}
	rqst->rq_xprt->xp_verf.oa_flavor = AUTH_NULL;
	rqst->rq_xprt->xp_verf.oa_length = 0;
	stat = AUTH_OK;
done:
	XDR_DESTROY(&xdrs);
	trace1(TR___svcauth_sys, 1);
	return (stat);
}

/*
 * Shorthand unix authenticator
 * Looks up longhand in a cache.
 */
/*ARGSUSED*/
enum auth_stat
_svcauth_short(rqst, msg)
	struct svc_req *rqst;
	struct rpc_msg *msg;
{
	trace1(TR___svcauth_short, 0);
	trace1(TR___svcauth_short, 1);
	return (AUTH_REJECTEDCRED);
}

/*
 * Unix longhand authenticator. Will be obsoleted
 */
enum auth_stat
_svcauth_unix(rqst, msg)
	register struct svc_req *rqst;
	register struct rpc_msg *msg;
{
	enum auth_stat dummy;

	trace1(TR___svcauth_unix, 0);
	dummy = _svcauth_sys(rqst, msg);
	trace1(TR___svcauth_unix, 1);
	return (dummy);
}
