/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/auth_esv.c	1.14"
#ident 	"$Header: $"

/*
 *	auth_esv.c, implements client side secure esv style
 *	authentication in the kernel. It interfaces with svcauthesv()
 *	on the server side.
 */

#include <util/param.h>
#include <util/cmn_err.h>
#include <util/types.h>
#include <svc/time.h>
#include <svc/utsname.h>
#include <proc/user.h>
#include <proc/pid.h>
#include <proc/proc.h>
#include <proc/cred.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/clnt.h>
#include <net/rpc/auth.h>
#include <net/rpc/auth_esv.h>
#include <net/rpc/token.h>
#include <mem/kmem.h>
#include <util/sysmacros.h>
#include <util/debug.h>

#ifdef RPCESV

extern	bool_t		xdr_opaque_auth(XDR *, struct opaque_auth *);
extern	int		strlen(char *);
extern	void		bcopy(void *, void *, size_t);

extern	int		sizeof_long;
extern	timestruc_t	hrestime;

/*
 * esv authenticator operations vector
 */
void			authesv_nextverf();
bool_t			authesv_marshal();
bool_t			authesv_validate();
bool_t			authesv_refresh();
void			authesv_destroy();

static struct auth_ops auth_esv_ops = {
	authesv_nextverf,
	authesv_marshal,
	authesv_validate,
	authesv_refresh,
	authesv_destroy
};

/*
 * authesv_create()
 *	Create a client esv style authenticator.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns an auth handle.
 *
 * Description:
 *	Create a client esv style authenticator.
 *
 * Parameters:
 *	
 */
AUTH *
authesv_create()
{
	AUTH	*auth;

	/*
	 * Allocate and set up auth handle
	 */
	auth = (AUTH *)kmem_alloc((u_int)sizeof(*auth), KM_SLEEP);
	auth->ah_ops = &auth_esv_ops;
	auth->ah_cred.oa_flavor = AUTH_ESV;
	auth->ah_verf = _null_auth;
	return (auth);
}

/*
 * authesv_nextverf(auth)
 *	Verify esv style credentials.
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
authesv_nextverf(AUTH *auth)
{
}

/*
 * authesv_marshal(auth, xdrs, cr, addr, pre4dot0)
 *	Serialize esv style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Called from client side.
 *
 *	Serializes the esv style credentials obtained from
 *	cr and also from utsname.nodename into xdrs.
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
 *	haddr			# address of server
 *
 */
bool_t
authesv_marshal(AUTH *auth, XDR *xdrs, struct cred *cr,
		struct netbuf *haddr, u_int pre4dot0)
{
	struct	opaque_auth	*cred;
	struct	authesv_parms	authp;
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
	authp.auc_aid = u.u_procp->p_pidp->pid_id;
	authp.auc_privs	= get_remote_token(haddr, PRIVS_T,
				(caddr_t)&cr->cr_wkgpriv, sizeof(pvec_t));
	authp.auc_sens	= get_remote_token(haddr, SENS_T,
				(caddr_t)&cr->cr_lid, sizeof(lid_t));
	authp.auc_info	= 0;		/* unsupported */
	authp.auc_integ	= 0;		/* unsupported */
	authp.auc_ncs	= 0;		/* unsupported */

	/*
	 * locking handled by getutsname().
	 */
	getutsname(utsname.nodename, myutsname);
	namelen = strlen(myutsname);
	rounded_namelen = RNDUP(namelen);
	credsize = rounded_namelen + ((BASE_ESVCREDSZ + gidlen) * sizeof_long);

	/*
	 * First we try a fast path to get through
	 * this very common operation.
	 */
	ptr = XDR_INLINE(xdrs, credsize + 4 * sizeof_long);
	if (ptr) {
		/*
		 * We can do the fast path.
		 */
		IXDR_PUT_LONG(ptr, AUTH_ESV);		/* cred flavor */
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
		IXDR_PUT_LONG(ptr, authp.auc_aid);	/* additional id, pid */
		IXDR_PUT_LONG(ptr, authp.auc_privs);	/* privilege vector */
		IXDR_PUT_LONG(ptr, authp.auc_sens);	/* sensitivity label */
		IXDR_PUT_LONG(ptr, authp.auc_info);	/* info label */
		IXDR_PUT_LONG(ptr, authp.auc_integ);	/* integrity token */
		IXDR_PUT_LONG(ptr, authp.auc_ncs);	/* nationality caveat */
		IXDR_PUT_LONG(ptr, AUTH_NULL);		/* verf flavor, null */
		IXDR_PUT_LONG(ptr, 0);			/* verf len */
		return (TRUE);
	}

	sercred = (char *)kmem_alloc((u_int)MAX_AUTH_BYTES, KM_SLEEP);

	/*
	 * setup sercred and xdrm for serialization
	 */
	xdrmem_create(&xdrm, sercred, MAX_AUTH_BYTES, XDR_ENCODE);

	/*
	 * serialize esv stuff into sercred.
	 */
	authp.auc_stamp = hrestime.tv_sec;
	authp.auc_machname = myutsname;
	authp.auc_uid = cr->cr_uid;
	authp.auc_gid = cr->cr_gid;
	authp.auc_len = cr->cr_ngroups;
	authp.auc_gids = cr->cr_groups;
	if (! xdr_authesv_parms(&xdrm, &authp)) {
		/*
		 *+ XDR of auth params failed.
		 *+ Warn and return error.
		 */
		cmn_err(CE_WARN, "authesv_marshal: xdr_authesv failed\n");

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
 * authesv_validate(auth, verf)
 *	Validate server's esv style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Always returns TRUE.
 *
 * Description:
 *	Called from client side.
 *
 *	Validates the esv style credentials based on the 
 *	verifier provided in verf. Actually does nothing.
 *	Hence there is no validation of server credentials
 *	for esv style.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *	verf			# server's verifier
 *
 */
/*ARGSUSED*/
bool_t
authesv_validate(AUTH *auth, struct opaque_auth verf)
{
	return (TRUE);
}

/*
 * authesv_refresh(auth)
 *	Refresh esv style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Always returns FALSE.
 *
 * Description:
 *	This operation is illegal for esv style auth.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *	
 */
/*ARGSUSED*/
bool_t
authesv_refresh(AUTH *auth)
{
	return (FALSE);
}

/*
 * authesv_destroy(auth)
 *	Destroy esv style credentials.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	No return value.
 *
 * Description:
 *	Destroys the esv style credentials by calling kmem_free.
 *
 * Parameters:
 *
 *	auth			# auth handle
 *
 */
void
authesv_destroy(AUTH *auth)
{
	kmem_free((caddr_t)auth, (u_int)sizeof(*auth));
}

#endif /* RPCESV */
