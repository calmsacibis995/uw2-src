/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:yppasswd/yppasswd.h	1.1"
#ident  "$Header: $"

#ifndef _PWD_H
#include <pwd.h>
#endif

#ifndef _RPCSVC_YPPASSWD_H
#define _RPCSVC_YPPASSWD_H

#define YPPASSWDPROG ((u_long)100009)
#define YPPASSWDVERS ((u_long)1)
#define YPPASSWDPROC_UPDATE ((u_long)1)

struct yppasswd {
	char *oldpass;		/* old (unencrypted) password */
	struct passwd newpw;	/* new pw structure */
};
int xdr_yppasswd();

#endif /*!_RPCSVC_YPPASSWD_H*/
