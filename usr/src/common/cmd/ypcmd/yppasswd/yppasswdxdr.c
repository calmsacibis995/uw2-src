/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:yppasswd/yppasswdxdr.c	1.1"
#ident  "$Header: $"


#include <rpc/rpc.h>
#include "yppasswd.h"

bool_t
xdr_passwd(xdrs, pw)
	XDR *xdrs;
	struct passwd *pw;
{
	if (!xdr_wrapstring(xdrs, &pw->pw_name)) {
		return (FALSE);
	}
	if (!xdr_wrapstring(xdrs, &pw->pw_passwd)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, (int *)&pw->pw_uid)) {
		return (FALSE);
	}
	if (!xdr_int(xdrs, (int *)&pw->pw_gid)) {
		return (FALSE);
	}
	if (!xdr_wrapstring(xdrs, &pw->pw_gecos)) {
		return (FALSE);
	}
	if (!xdr_wrapstring(xdrs, &pw->pw_dir)) {
		return (FALSE);
	}
	if (!xdr_wrapstring(xdrs, &pw->pw_shell)) {
		return (FALSE);
	}
	return (TRUE);
}

bool_t
xdr_yppasswd(xdrs, yppw)
	XDR *xdrs;
	struct yppasswd *yppw;
{
	if (!xdr_wrapstring(xdrs, &yppw->oldpass)) {
		return (FALSE);
	}
	if (!xdr_passwd(xdrs, &yppw->newpw)) {
		return (FALSE);
	}
	return (TRUE);
}
