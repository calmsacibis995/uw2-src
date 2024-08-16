/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/auth_sys.c	1.2.10.6"
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
 * auth_sys.c, Implements UNIX (system) style authentication parameters.
 *
 * The system is very weak.  The client uses no encryption for its
 * credentials and only sends null verifiers.  The server sends backs
 * null verifiers or optionally a verifier that suggests a new short hand
 * for the credentials.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/syslog.h>

#include <rpc/types.h>
#include "trace.h"
#include <rpc/xdr.h>
#include <rpc/auth.h>
#include <rpc/auth_sys.h>

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN  256
#endif

static struct auth_ops *authsys_ops();

/*
 * This struct is pointed to by the ah_private field of an auth_handle.
 */
struct audata {
	struct opaque_auth	au_origcred;	/* original credentials */
	struct opaque_auth	au_shcred;	/* short hand cred */
	u_long			au_shfaults;	/* short hand cache faults */
	char			au_marshed[MAX_AUTH_BYTES];
	u_int			au_mpos;	/* xdr pos at end of marshed */
};
#define	AUTH_PRIVATE(auth)	((struct audata *)auth->ah_private)

static void marshal_new_auth();

/*
 * Create a (sys) unix style authenticator.
 * Returns an auth handle with the given stuff in it.
 */
AUTH *
authsys_create(machname, uid, gid, len, aup_gids)
	char *machname;
	uid_t uid;
	gid_t gid;
	register int len;
	gid_t *aup_gids;
{
	struct authsys_parms aup;
	char mymem[MAX_AUTH_BYTES];
	struct timeval now;
	XDR xdrs;
	register AUTH *auth;
	register struct audata *au;

	trace2(TR_authsys_create, 0, len);
	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)mem_alloc(sizeof (*auth));
	if (auth == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32",
			"%s: out of memory"),
			"authsys_create");
		trace1(TR_authsys_create, 1);
		return (NULL);
	}
	au = (struct audata *)mem_alloc(sizeof (*au));
	if (au == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32",
			"%s: out of memory"),
			"authsys_create");
		trace1(TR_authsys_create, 1);
		return (NULL);
	}
	auth->ah_ops = authsys_ops();
	auth->ah_private = (caddr_t)au;
	auth->ah_verf = au->au_shcred = _null_auth;
	au->au_shfaults = 0;

	/*
	 * fill in param struct from the given params
	 */
	(void) gettimeofday(&now, (struct timezone *) NULL);
	aup.aup_time = now.tv_sec;
	aup.aup_machname = machname;
	aup.aup_uid = uid;
	aup.aup_gid = gid;
	aup.aup_len = (u_int)len;
	aup.aup_gids = aup_gids;

	/*
	 * Serialize the parameters into origcred
	 */
	xdrmem_create(&xdrs, mymem, MAX_AUTH_BYTES, XDR_ENCODE);
	if (! xdr_authsys_parms(&xdrs, &aup)) {
		(void) syslog(LOG_ERR, 
			gettxt("uxnsl:40",
			    "authsys_create: xdr_authsys_parms failed"));
		trace1(TR_authsys_create, 1);
		return(NULL);
	}
	au->au_origcred.oa_length = len = XDR_GETPOS(&xdrs);
	au->au_origcred.oa_flavor = AUTH_SYS;
	if ((au->au_origcred.oa_base =
		(caddr_t) mem_alloc((u_int) len)) == NULL) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:32",
			"%s: out of memory"),
			"authsys_create");
		(void) mem_free((char *) au, sizeof (*au));
		(void) mem_free((char *) auth, sizeof (*auth));
		trace1(TR_authsys_create, 1);
		return (NULL);
	}
	(void) memcpy(au->au_origcred.oa_base, mymem, (u_int)len);

	/*
	 * set auth handle to reflect new cred.
	 */
	auth->ah_cred = au->au_origcred;
	(void) marshal_new_auth(auth);
	trace1(TR_authsys_create, 1);
	return (auth);
}

/*
 * Returns an auth handle with parameters determined by doing lots of
 * syscalls.
 */

AUTH *
authsys_create_default()
{
	register int len;
	char machname[MAXHOSTNAMELEN + 1];
	register uid_t uid;
	register gid_t gid;
	gid_t gids[NGRPS];
	AUTH *dummy;

	trace1(TR_authsys_create_default, 0);
	if (gethostname(machname, MAXHOSTNAMELEN) == -1) {
		(void) syslog(LOG_ERR, 
			gettxt("uxnsl:41",
			    "authsys_create_default:  gethostname failed:  %m"));
		trace1(TR_authsys_create_default, 1);
		return(NULL);
	}
	machname[MAXHOSTNAMELEN] = 0;
	uid = geteuid();
	gid = getegid();
	if ((len = getgroups(NGRPS, gids)) < 0) {
		(void) syslog(LOG_ERR, 
			gettxt("uxnsl:42",
			    "authsys_create_default:  getgroups failed:  %m"));

		trace2(TR_authsys_create_default, 1, len);
		return(NULL);
	}
	dummy = authsys_create(machname, uid, gid, len, gids);
	trace2(TR_authsys_create_default, 1, len);
	return (dummy);
}

/*
 * authsys operations
 */

static void
authsys_nextverf(auth)
	AUTH *auth;
{
	trace1(TR_authsys_nextverf, 0);
	/* no action necessary */
	trace1(TR_authsys_nextverf, 1);
}

static bool_t
authsys_marshal(auth, xdrs)
	AUTH *auth;
	XDR *xdrs;
{
	register struct audata *au = AUTH_PRIVATE(auth);
	bool_t dummy;

	trace1(TR_authsys_marshal, 0);
	dummy  = XDR_PUTBYTES(xdrs, au->au_marshed, au->au_mpos);
	trace1(TR_authsys_marshal, 1);
	return (dummy);
}

static bool_t
authsys_validate(auth, verf)
	register AUTH *auth;
	struct opaque_auth *verf;
{
	register struct audata *au;
	XDR xdrs;

	trace1(TR_authsys_validate, 0);
	if (verf->oa_flavor == AUTH_SHORT) {
		au = AUTH_PRIVATE(auth);
		xdrmem_create(&xdrs, verf->oa_base,
			verf->oa_length, XDR_DECODE);

		if (au->au_shcred.oa_base != NULL) {
			mem_free(au->au_shcred.oa_base,
			    au->au_shcred.oa_length);
			au->au_shcred.oa_base = NULL;
		}
		if (xdr_opaque_auth(&xdrs, &au->au_shcred)) {
			auth->ah_cred = au->au_shcred;
		} else {
			xdrs.x_op = XDR_FREE;
			(void) xdr_opaque_auth(&xdrs, &au->au_shcred);
			au->au_shcred.oa_base = NULL;
			auth->ah_cred = au->au_origcred;
		}
		(void) marshal_new_auth(auth);
	}
	trace1(TR_authsys_validate, 1);
	return (TRUE);
}

static bool_t
authsys_refresh(auth)
	register AUTH *auth;
{
	register struct audata *au = AUTH_PRIVATE(auth);
	struct authsys_parms aup;
	struct timeval now;
	XDR xdrs;
	register int stat;

	trace1(TR_authsys_refresh, 0);
	if (auth->ah_cred.oa_base == au->au_origcred.oa_base) {
		/* there is no hope.  Punt */
		trace1(TR_authsys_refresh, 1);
		return (FALSE);
	}
	au->au_shfaults ++;

	/* first deserialize the creds back into a struct authsys_parms */
	aup.aup_machname = NULL;
	aup.aup_gids = (gid_t *)NULL;
	xdrmem_create(&xdrs, au->au_origcred.oa_base,
	    au->au_origcred.oa_length, XDR_DECODE);
	stat = xdr_authsys_parms(&xdrs, &aup);
	if (! stat)
		goto done;

	/* update the time and serialize in place */
	(void) gettimeofday(&now, (struct timezone *) NULL);
	aup.aup_time = now.tv_sec;
	xdrs.x_op = XDR_ENCODE;
	XDR_SETPOS(&xdrs, 0);
	stat = xdr_authsys_parms(&xdrs, &aup);
	if (! stat)
		goto done;
	auth->ah_cred = au->au_origcred;
	(void) marshal_new_auth(auth);
done:
	/* free the struct authsys_parms created by deserializing */
	xdrs.x_op = XDR_FREE;
	(void) xdr_authsys_parms(&xdrs, &aup);
	XDR_DESTROY(&xdrs);
	trace1(TR_authsys_refresh, 1);
	return (stat);
}

static void
authsys_destroy(auth)
	register AUTH *auth;
{
	register struct audata *au = AUTH_PRIVATE(auth);

	trace1(TR_authsys_destroy, 0);
	mem_free(au->au_origcred.oa_base, au->au_origcred.oa_length);
	if (au->au_shcred.oa_base != NULL)
		mem_free(au->au_shcred.oa_base, au->au_shcred.oa_length);
	mem_free(auth->ah_private, sizeof (struct audata));
	if (auth->ah_verf.oa_base != NULL)
		mem_free(auth->ah_verf.oa_base, auth->ah_verf.oa_length);
	mem_free((caddr_t)auth, sizeof (*auth));
	trace1(TR_authsys_destroy, 1);
}

/*
 * Marshals (pre-serializes) an auth struct.
 * sets private data, au_marshed and au_mpos
 */

static void
marshal_new_auth(auth)
	register AUTH *auth;
{
	XDR		xdr_stream;
	register XDR	*xdrs = &xdr_stream;
	register struct audata *au = AUTH_PRIVATE(auth);

	trace1(TR_marshal_new_auth, 0);
	xdrmem_create(xdrs, au->au_marshed, MAX_AUTH_BYTES, XDR_ENCODE);
	if ((! xdr_opaque_auth(xdrs, &(auth->ah_cred))) ||
	    (! xdr_opaque_auth(xdrs, &(auth->ah_verf)))) {
		(void) syslog(LOG_ERR,
		    gettxt("uxnsl:43",
			"marshal_new_auth - Fatal marshalling problem"));

	} else {
		au->au_mpos = XDR_GETPOS(xdrs);
	}
	XDR_DESTROY(xdrs);
	trace1(TR_marshal_new_auth, 1);
}

static struct auth_ops *
authsys_ops()
{
	static struct auth_ops ops;

	trace1(TR_authsys_ops, 0);
	if (ops.ah_destroy == NULL) {
		ops.ah_nextverf = authsys_nextverf;
		ops.ah_marshal = authsys_marshal;
		ops.ah_validate = authsys_validate;
		ops.ah_refresh = authsys_refresh;
		ops.ah_destroy = authsys_destroy;
	}
	trace1(TR_authsys_ops, 1);
	return (&ops);
}
