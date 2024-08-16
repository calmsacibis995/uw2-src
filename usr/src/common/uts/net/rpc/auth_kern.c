/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/auth_kern.c	1.14"
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
 *	auth_kern.c, implements client side UNIX style authentication
 *	parameters in the kernel. Interfaces with svc_auth_unix on the
 *	server.
 *
 */

#include <util/param.h>
#include <util/cmn_err.h>
#include <svc/time.h>
#include <util/types.h>
#include <net/rpc/types.h>
#include <proc/user.h>
#include <proc/proc.h>
#include <net/xti.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_unix.h>
#include <net/rpc/clnt.h>
#include <svc/utsname.h>
#include <proc/cred.h>
#include <mem/kmem.h>
#include <util/sysmacros.h>
#include <util/debug.h>

extern	bool_t		xdr_authkern(XDR *);
extern	bool_t		xdr_opaque_auth(XDR *, struct opaque_auth *);
extern	int		strlen(char *);
extern	void		bcopy(void *, void *, size_t);

extern	int		sizeof_long;
extern	timestruc_t	hrestime;

/*
 * Unix authenticator operations vector
 */
void			authkern_nextverf();
bool_t			authkern_marshal();
bool_t			authkern_validate();
bool_t			authkern_refresh();
void			authkern_destroy();

static struct auth_ops auth_kern_ops = {
	authkern_nextverf,
	authkern_marshal,
	authkern_validate,
	authkern_refresh,
	authkern_destroy
};

/*
 * authkern_create()
 *	Create a kernel unix style authenticator.
 *
 * Calling/Exit State:
 *	No locking assumptions.
 *
 *	Returns an auth handle.
 *
 * Description:
 *	Create a kernel unix style authenticator.
 *
 * Parameters:
 *
 */
AUTH *
authkern_create()
{
	AUTH	*auth;

	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)kmem_alloc((u_int)sizeof(*auth), KM_SLEEP);
	auth->ah_ops = &auth_kern_ops;
	auth->ah_cred.oa_flavor = AUTH_UNIX;
	auth->ah_verf = _null_auth;

	return (auth);
}

/*
 * authkern_nextverf(auth)
 *	Verify kern style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	No return value.
 *
 * Description:
 *	Does nothing.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *
 */
/*ARGSUSED*/
void
authkern_nextverf(AUTH *auth)
{
}

/*
 * authkern_marshal(auth, xdrs, cr, addr, pre4dot0)
 *	Serialize kern style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Called from client side.
 *
 *	Serializes the kern style credentials obtained from
 *	cr and also from utsname.nodename into xdrs. addr is
 *	not used for kern style but is needed for esv.
 *
 *	If pre4dot0 is set, the maximum number of groups which
 *	will be serialized from the creds will be 8. This is
 *	for compatibility with pre4dot0 server rpc which only
 *	allowed 8 groups. The world has since revolved.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *	xdrs			# stream to serialize into
 *	cr			# credentials
 *	addr			# address of remote host, not used here
 *	pre4dot0			# pre 4.0 rpc/nfs compat
 *	
 */
/* ARGSUSED */
bool_t
authkern_marshal(AUTH *auth, XDR *xdrs, struct cred *cr,
		 struct netbuf *addr, u_int pre4dot0)
{
	struct	opaque_auth	*cred;
	char			*sercred;
	XDR			xdrm;
	bool_t			ret = FALSE;
	gid_t			*gp;
	gid_t			*gpend;
	int			gidlen;
	int			credsize;
	char			myutsname[SYS_NMLN];
	int			namelen;
	int			rounded_namelen;
	long			*ptr;

	gp = cr->cr_groups;

	/*
	 * only serialize 8 groups when talking with pre4dot0
	 * server rpc.
	 */
	if (pre4dot0)
		gidlen = MIN(cr->cr_ngroups, 8);
	else
		gidlen = cr->cr_ngroups;

	gpend = &gp[gidlen-1];

	/*
	 * locking handled by getutsname().
	 */
	getutsname(utsname.nodename, myutsname);
	namelen = strlen(myutsname);
	rounded_namelen = RNDUP(namelen);
	credsize = rounded_namelen + ((BASE_CREDSZ + gidlen) * sizeof_long);

	/*
	 * First we try a fast path to get through
	 * this very common operation.
	 */
	ptr = XDR_INLINE(xdrs, credsize + 4 * sizeof_long);
	if (ptr) {
		/*
		 * We can do the fast path.
		 */
		IXDR_PUT_LONG(ptr, AUTH_UNIX);		/* cred flavor */
		IXDR_PUT_LONG(ptr, credsize);		/* cred len */
		IXDR_PUT_LONG(ptr, hrestime.tv_sec);	/* time */
		IXDR_PUT_LONG(ptr, namelen);		/* machine name len */
		bcopy(myutsname, (caddr_t)ptr, namelen); /* machine name */
		ptr += rounded_namelen / BYTES_PER_XDR_UNIT;
		IXDR_PUT_LONG(ptr, cr->cr_uid);		/* user id */
		IXDR_PUT_LONG(ptr, cr->cr_gid);		/* group id */
		IXDR_PUT_LONG(ptr, gidlen);		/* number of groups */
		while (gp <= gpend) {
			IXDR_PUT_LONG(ptr, *gp++);	/* groups themselves */
		}
		IXDR_PUT_LONG(ptr, AUTH_NULL);		/* verf flavor, null*/
		IXDR_PUT_LONG(ptr, 0);			/* verf len */
		return (TRUE);
	}

	sercred = (char *)kmem_alloc((u_int)MAX_AUTH_BYTES, KM_SLEEP);

	/*
	 * setup xdrm and sercred for serialization
	 */
	xdrmem_create(&xdrm, sercred, MAX_AUTH_BYTES, XDR_ENCODE);

	/*
	 * serialize u struct stuff into sercred
	 */
	if (! xdr_authkern(&xdrm)) {
		/*
		 *+ XDR of auth params failed.
		 *+ Warn and return error.
		 */
		cmn_err(CE_WARN, "authkern_marshal: xdr_authkern failed\n");

		ret = FALSE;
		goto done;
	}

	/*
	 * Make opaque auth credentials that point at serialized u struct
	 */
	cred = &(auth->ah_cred);
	cred->oa_length = XDR_GETPOS(&xdrm);
	cred->oa_base = sercred;

	/*
	 * serialize credentials and verifiers (null)
	 */
	if ((xdr_opaque_auth(xdrs, &(auth->ah_cred)))
		&& (xdr_opaque_auth(xdrs, &(auth->ah_verf)))) {
		ret = TRUE;
	} else {
		ret = FALSE;
	}

done:

	kmem_free((caddr_t)sercred, (u_int)MAX_AUTH_BYTES);

	return (ret);
}

/*
 * authkern_validate(auth, verf)
 *	Validate servers kern style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Always returns TRUE.
 *
 * Description:
 *	Called from client side.
 *
 *	Validates the server's kern style credentials based on
 *	the verifier provided in verf. Actually does nothing.
 *	Hence there is no validation of server's credentials
 *	for kern style.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *	verf			# server's verifier
 *	
 */
/*ARGSUSED*/
bool_t
authkern_validate(AUTH *auth, struct opaque_auth verf)
{
	return (TRUE);
}

/*
 * authkern_refresh(auth)
 *	Refresh kern style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Always returns FALSE.
 *
 * Description:
 *	This operation is illegal for kern style auth.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *	
 */
/*ARGSUSED*/
bool_t
authkern_refresh(AUTH *auth)
{
	return (FALSE);
}

/*
 * authkern_destroy(auth)
 *	Destroy kern style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	No return value.
 *
 * Description:
 *	Destroys the kern style credentials by calling kmem_free.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *
 */
void
authkern_destroy(AUTH *auth)
{
	kmem_free((caddr_t)auth, (u_int)sizeof(*auth));
}
