/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)keyserv:key_generic.c	1.4.9.6"
#ident	"$Header: $"

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice
*
* Notice of copyright on this source code product does not indicate
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
*/

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
 */

/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <string.h>
#include <rpc/rpc.h>
#include <errno.h>
#include <sys/syslog.h>
#include <rpc/nettype.h>
#include <netconfig.h>
#include <netdir.h>

extern int t_errno;
extern char *t_errlist[];

#define syslog _abi_syslog

/*
 * The highest level interface for server creation.
 * It tries for all the nettokens in that particular class of token
 * and returns the number of handles it can create and/or find.
 *
 * It creates a link list of all the handles it could create.
 * If svc_create() is called multiple times, it uses the handle
 * created earlier instead of creating a new handle every time.
 *
 * Copied from svc_generic.c
 */
int
svc_create_local_service(dispatch, prognum, versnum, nettype, servname)
void (*dispatch) ();		/* Dispatch function */
u_long prognum;			/* Program number */
u_long versnum;			/* Version number */
char *nettype;			/* Networktype token */
char *servname;			/* name of the service */
{
	struct xlist {
		SVCXPRT *xprt;		/* Server handle */
		struct xlist *next;	/* Next item */
	} *l;
	static struct xlist *xprtlist;
	int num = 0;
	SVCXPRT *xprt;
	struct netconfig *nconf;
	struct t_bind *bind_addr;
	void *handle;
	int fd;
	struct nd_hostserv ns;
	struct nd_addrlist *nas;

	if ((handle = _rpc_setconf(nettype)) == NULL) {
		(void) syslog(LOG_ERR,
		"svc_create: could not read netconfig database");
		return (0);
	}
	while (nconf = _rpc_getconf(handle)) {
		if (strcmp(nconf->nc_protofmly, NC_LOOPBACK))
			continue;
		for (l = xprtlist; l; l = l->next) {
			if (strcmp(l->xprt->xp_netid, nconf->nc_netid) == 0) {
				/* Found an  old  one,  use  it */
				(void) rpcb_unset(prognum, versnum, nconf);
				if (svc_reg(l->xprt, prognum, versnum,
					dispatch, nconf) == FALSE)
					(void) syslog(LOG_ERR,
	    "svc_create: could not register prog %d vers %d on %s",
					    prognum, versnum, nconf->nc_netid);
				else
					num++;
				break;
			}
		}
		if (l)
			continue;
		/* It was not found. Now create a new one */
		if ((fd = t_open(nconf->nc_device, O_RDWR, NULL)) < 0) {
			syslog(LOG_ERR,
		"svc_create: %s: cannot open connection: %s",
			    nconf->nc_netid, t_errlist[t_errno]);
			continue;
		}
		bind_addr = (struct t_bind *) t_alloc(fd, T_BIND, T_ADDR);
		if ((bind_addr == NULL)) {
			t_close(fd);
			(void) syslog(LOG_ERR, "svc_create: t_alloc failed\n");
			continue;
		}
		ns.h_host = HOST_SELF;
		ns.h_serv = servname;
		if (!netdir_getbyname(nconf, &ns, &nas)) {
			/* Copy the address */
			bind_addr->addr.len = nas->n_addrs->len;
			memcpy(bind_addr->addr.buf, nas->n_addrs->buf,
				(int) nas->n_addrs->len);
			netdir_free((char *) nas, ND_ADDRLIST);
		} else {
			syslog(LOG_ERR,
	"svc_create: no well known address for %s on transport %s\n",
			    servname, nconf->nc_netid);
			(void) t_free(bind_addr, T_BIND);
			bind_addr = NULL;
		}

		xprt = svc_tli_create(fd, nconf, bind_addr, 0, 0);
		if (bind_addr)
			(void) t_free(bind_addr, T_BIND);
		if (xprt) {
			(void) rpcb_unset(prognum, versnum, nconf);
			if (svc_reg(xprt, prognum, versnum,
				dispatch, nconf) == FALSE) {
				(void) syslog(LOG_ERR,
	    "svc_create: could not register prog %d vers %d on %s",
				    prognum, versnum, nconf->nc_netid);
				SVC_DESTROY(xprt);
				continue;
			}
			l = (struct xlist *) malloc(sizeof (struct xlist));
			if (l == (struct xlist *) NULL) {
				(void) syslog(LOG_ERR,
					    "svc_create: no memory");
				SVC_DESTROY(xprt);
				return (num);
			}
			l->xprt = xprt;
			l->next = xprtlist;
			xprtlist = l;
			num++;
			_rpc_negotiate_uid(xprt->xp_fd);
		}
	}
	_rpc_endconf(handle);
	return (num);
}
