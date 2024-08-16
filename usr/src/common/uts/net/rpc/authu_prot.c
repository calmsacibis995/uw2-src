/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)kern:net/rpc/authu_prot.c	1.11"
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
 *	authunix_prot.c, XDR routines for UNIX style authentication
 *	parameters for kernel rpc.
 */

#include <util/param.h>
#include <util/types.h>
#include <svc/time.h>
#include <svc/utsname.h>
#include <proc/cred.h>
#include <proc/proc.h>
#include <proc/lwp.h>
#include <proc/user.h>
#include <net/rpc/types.h>
#include <net/rpc/xdr.h>
#include <net/rpc/auth.h>
#include <net/rpc/clnt.h>
#include <net/rpc/auth_unix.h>

extern timestruc_t	hrestime;

/*
 * xdr_authunix_params(xdrs, p)
 *	XDR routine for unix authentication parameters.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Serializes unix style credentials. Used by server
 *	side in the kernel.
 *
 * Parameters:
 *	
 *	xdrs			# stream to serialize into
 *	p			# pointer to unix parameters
 *
 */
bool_t
xdr_authunix_parms(XDR *xdrs, struct authunix_parms *p)
{
	if (xdr_u_long(xdrs, &(p->aup_time))
		&& xdr_string(xdrs, &(p->aup_machname), MAX_MACHINE_NAME)
		&& xdr_int(xdrs, (int *)&(p->aup_uid))
		&& xdr_int(xdrs, (int *)&(p->aup_gid))
		&& xdr_array(xdrs, (caddr_t *)&(p->aup_gids),
		&(p->aup_len), NGROUPS_UMAX, sizeof(int), xdr_int) ) {

		return (TRUE);
	}

	return (FALSE);
}

/*
 * xdr_authkern(xdrs)
 *	XDR routine for the kernel unix authentication parameters.
 *
 * Calling/Exit State:
 *	No locking assumptions made.
 *
 *	Returns TRUE on success, FALSE on failure.
 *
 * Description:
 *	Serializes the kernel unix style credentials. Picks up info
 *	from the lwp struct directly. This is an XDR_ENCODE only
 *	routine. It is used by the client side in the kernel.
 *
 * Parameters:
 *	
 *	xdrs			# stream to serialize into
 *
 */
bool_t
xdr_authkern(XDR *xdrs)
{

	uid_t			uid;
	gid_t			gid;
	int			len;
	caddr_t			groups;
	char			myutsname[SYS_NMLN];
	char			*name;
	struct cred		*cr;

	if (xdrs->x_op != XDR_ENCODE) {
		return (FALSE);
	}

	/*
	 * locking handled by getutsname().
	 */
	getutsname(utsname.nodename, myutsname);
	name = myutsname;

	cr = u.u_lwpp->l_cred;
	uid = cr->cr_uid;
	gid = cr->cr_gid;
	len = cr->cr_ngroups;
	groups = (caddr_t)cr->cr_groups;
	if (xdr_u_long(xdrs, (u_long *)&hrestime.tv_sec)
		&& xdr_string(xdrs, &name, MAX_MACHINE_NAME)
		&& xdr_int(xdrs, (int *)&uid)
		&& xdr_int(xdrs, (int *)&gid)
		&& xdr_array(xdrs, &groups, (u_int *)&len, NGRPS,
			sizeof (int), xdr_int) ) {

		return (TRUE);
	}

	return (FALSE);
}
