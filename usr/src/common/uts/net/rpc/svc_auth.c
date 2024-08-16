/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/svc_auth.c	1.8"
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
 *	svc_auth.c, server-side rpc authenticator interface.
 *
 *	Server side authenticators are called from authenticate by
 *	using the client auth struct flavor field as the indicator
 *	of auth flavor. The server auth flavors must implement a
 *	routine that looks like: 
 * 
 *		enum auth_stat 
 *		_svcauth_<flavor>(rqst, msg)
 *			struct svc_req *rqst; 
 *			struct rpc_msg *msg;
 */

#include <util/param.h>
#include <util/types.h>
#include <net/rpc/types.h>
#include <net/inet/in.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/rpc_msg.h>
#include <net/xti.h>
#include <net/tihdr.h>
#include <net/ktli/t_kuser.h>
#include <net/rpc/svc.h>
#include <net/rpc/svc_auth.h>


enum auth_stat _svcauth_null();		/* no authentication */
enum auth_stat _svcauth_unix();		/* unix style (uid, gids) */
enum auth_stat _svcauth_short();	/* short hand unix style */
enum auth_stat _svcauth_des();		/* des style */

#ifdef RPCESV

enum auth_stat _svcauth_esv();		/* esv style */

#endif

/*
 * _authenticate(rqst, msg)
 *	Authenticate a rpc request from a client.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns auth status in a auth_stat.
 *
 * Description:
 *	This routine authenticates an rpc request from a client.
 *	The rpc message, msg, has been obtained from the wire. 
 *	The msg contains the raw form of credentials and verifiers.
 *	Authenticate returns AUTH_OK if the msg is successfully
 *	authenticated. If AUTH_OK then the routine also does the
 *	following things:
 *
 *	a) sets rqst->rq_xprt->verf to the appropriate response verifier.
 *	b) sets rqst->rq_client_cred to the "cooked" form of the
 *	   credentials.
 *
 *	NB: rqst->rq_cxprt->verf must be pre-allocated. Its length is
 *	set appropriately.
 *
 *	The caller still owns and is responsible for msg->u.cmb.cred and
 *	msg->u.cmb.verf. The authentication system retains ownership of
 *	rqst->rq_client_cred, the cooked credentials.
 *
 *	There is an assumption that any flavour less than AUTH_NULL is
 *	invalid.
 *
 * Parameters:
 *
 *	rqst			# service request
 *	msg			# rpc message
 */
enum auth_stat
_authenticate(struct svc_req *rqst, struct rpc_msg *msg)
{
	int	cred_flavor;

	rqst->rq_cred = msg->rm_call.cb_cred;
	rqst->rq_xprt->xp_verf.oa_flavor = _null_auth.oa_flavor;
	rqst->rq_xprt->xp_verf.oa_length = 0;
	cred_flavor = rqst->rq_cred.oa_flavor;

	switch (cred_flavor) {

		case AUTH_NULL:

			return (_svcauth_null(rqst, msg));

		case AUTH_UNIX:

			return (_svcauth_unix(rqst, msg));

		case AUTH_SHORT:

			return (_svcauth_short(rqst, msg));

		case AUTH_DES:

			return (_svcauth_des(rqst, msg));

#ifdef RPCSVC
		case AUTH_ESV:

			return (_svcauth_esv(rqst, msg));
#endif

	}

	return (AUTH_REJECTEDCRED);
}

/*
 * _svcauth_null(rqst, msg)
 *	Null authenticator for a rpc request from a client.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns auth status in a auth_stat.
 *
 * Description:
 *	Null authenticator for a rpc request from a client.
 *	Always returns AUTH_OK.
 *
 * Parameters:
 *
 *	rqst			# service request
 *	msg			# rpc message
 */
/*ARGSUSED*/
enum auth_stat
_svcauth_null(struct svc_req *rqst, struct rpc_msg *msg)
{
	return (AUTH_OK);
}
