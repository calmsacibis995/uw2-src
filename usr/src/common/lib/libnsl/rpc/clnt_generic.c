/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/clnt_generic.c	1.3.8.2"
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
 * clnt_generic.c
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include "trace.h"
#include <errno.h>
#include <rpc/nettype.h>
#include "rpc_mt.h"

#undef rpc_createerr

/*
 * Generic client creation with version checking the value of
 * vers_out is set to the highest server supported value
 * vers_low <= vers_out <= vers_high  AND an error results
 * if this can not be done.
 */
CLIENT *
clnt_create_vers(hostname, prog, vers_out, vers_low, vers_high, nettype)
	char *hostname;
	unsigned long prog;
	unsigned long *vers_out;
	unsigned long vers_low;
	unsigned long vers_high;
	char *nettype;
{
	CLIENT *clnt;
	struct timeval to;
	enum clnt_stat rpc_stat;
	struct rpc_err rpcerr;
	rpc_createerr_t rpc_createerr = { 0 };

	trace4(TR_clnt_create_vers, 0, prog, vers_low, vers_high);
	clnt = clnt_create(hostname, prog, vers_high, nettype);
	if (clnt == NULL) {
		trace4(TR_clnt_create_vers, 1, prog, vers_low, vers_high);
		return (NULL);
	}
	to.tv_sec = 10;
	to.tv_usec = 0;
	rpc_stat = clnt_call(clnt, NULLPROC, (xdrproc_t) xdr_void,
			(char *) NULL, (xdrproc_t) xdr_void, (char *) NULL, to);
	if (rpc_stat == RPC_SUCCESS) {
		*vers_out = vers_high;
		trace4(TR_clnt_create_vers, 1, prog, vers_low, vers_high);
		return (clnt);
	}
	if (rpc_stat == RPC_PROGVERSMISMATCH) {
		unsigned long minvers, maxvers;

		clnt_geterr(clnt, &rpcerr);
		minvers = rpcerr.re_vers.low;
		maxvers = rpcerr.re_vers.high;
		if (maxvers < vers_high)
			vers_high = maxvers;
		if (minvers > vers_low)
			vers_low = minvers;
		if (vers_low > vers_high) {
			goto error;
		}
		CLNT_CONTROL(clnt, CLSET_VERS, (char *)&vers_high);
		rpc_stat = clnt_call(clnt, NULLPROC, (xdrproc_t) xdr_void,
				(char *) NULL, (xdrproc_t) xdr_void,
				(char *) NULL, to);
		if (rpc_stat == RPC_SUCCESS) {
			*vers_out = vers_high;
			trace4(TR_clnt_create_vers, 1, prog,
				vers_low, vers_high);
			return (clnt);
		}
	}
	clnt_geterr(clnt, &rpcerr);

error:	rpc_createerr.cf_stat = rpc_stat;
	rpc_createerr.cf_error = rpcerr;
	set_rpc_createerr(rpc_createerr);
	clnt_destroy(clnt);
	trace4(TR_clnt_create_vers, 1, prog, vers_low, vers_high);
	return (NULL);
}

/*
 * Top level client creation routine.
 * Generic client creation: takes (servers name, program-number, nettype) and
 * returns client handle. Default options are set, which the user can
 * change using the rpc equivalent of ioctl()'s.
 *
 * It tries for all the netids in that particular class of netid until
 * it succeeds.
 * XXX The error message in the case of failure will be the one
 * pertaining to the last create error.
 *
 * It calls clnt_tp_create();
 */
CLIENT *
clnt_create(hostname, prog, vers, nettype)
	char *hostname;				/* server name */
	u_long prog;				/* program number */
	u_long vers;				/* version number */
	char *nettype;				/* net type */
{
	struct netconfig *nconf;
	CLIENT *clnt = NULL;
	void *handle;
	enum clnt_stat	save_cf_stat = RPC_SUCCESS;
	struct rpc_err	save_cf_error;
	rpc_createerr_t rpc_createerr = { 0 };

	trace3(TR_clnt_create, 0, prog, vers);
	if ((handle = _rpc_setconf(nettype)) == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_clnt_create, 1, prog, vers);
		return ((CLIENT *)NULL);
	}
	rpc_createerr.cf_stat = RPC_SUCCESS;
	while (clnt == (CLIENT *)NULL) {
		if ((nconf = _rpc_getconf(handle)) == NULL) {
			rpc_createerr = get_rpc_createerr();
			if (rpc_createerr.cf_stat == RPC_SUCCESS)
				rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			break;
		}
		clnt = clnt_tp_create(hostname, prog, vers, nconf);
		if (clnt)
			break;
		else {
			/*
			 *	Since we didn't get a name-to-address
			 *	translation failure here, we remember
			 *	this particular error.  The object of
			 *	this is to enable us to return to the
			 *	caller a more-specific error than the
			 *	unhelpful ``Name to address translation
			 *	failed'' which might well occur if we
			 *	merely returned the last error (because
			 *	the local loopbacks are typically the
			 *	last ones in /etc/netconfig and the most
			 *	likely to be unable to translate a host
			 *	name).
			 */
			rpc_createerr = get_rpc_createerr();
			if (rpc_createerr.cf_stat != RPC_N2AXLATEFAILURE) {
				save_cf_stat = rpc_createerr.cf_stat;
				save_cf_error = rpc_createerr.cf_error;
			}
		}
	}

	/*
	 *	Attempt to return an error more specific than ``Name to address
	 *	translation failed''
	 */
	rpc_createerr = get_rpc_createerr();
        if ((rpc_createerr.cf_stat == RPC_N2AXLATEFAILURE) &&
                	(save_cf_stat != RPC_SUCCESS)) {
		rpc_createerr.cf_stat = save_cf_stat;
		rpc_createerr.cf_error = save_cf_error;
	}
	set_rpc_createerr(rpc_createerr);
	_rpc_endconf(handle);
	trace3(TR_clnt_create, 1, prog, vers);
	return (clnt);
}

/*
 * Generic client creation: takes (servers name, program-number, netconf) and
 * returns client handle. Default options are set, which the user can
 * change using the rpc equivalent of ioctl()'s : clnt_control()
 * It finds out the server address from rpcbind and calls clnt_tli_create()
 */
CLIENT *
clnt_tp_create(hostname, prog, vers, nconf)
	char *hostname;				/* server name */
	u_long prog;				/* program number */
	u_long vers;				/* version number */
	register struct netconfig *nconf;	/* net config struct */
{
	struct netbuf *svcaddr;			/* servers address */
	CLIENT *cl;				/* client handle */
	extern struct netbuf *_rpcb_findaddr();
	rpc_createerr_t rpc_createerr = { 0 };

	trace3(TR_clnt_tp_create, 0, prog, vers);
	if (nconf == (struct netconfig *)NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_clnt_tp_create, 1, prog, vers);
		return ((CLIENT *)NULL);
	}
	/*
	 * Get the address of the server
	 */
	if ((svcaddr = _rpcb_findaddr(prog, vers, nconf, hostname)) == NULL) {
		/* appropriate error number is set by rpcbind libraries */
		trace3(TR_clnt_tp_create, 1, prog, vers);
		return ((CLIENT *)NULL);
	}
	cl = clnt_tli_create(RPC_ANYFD, nconf, svcaddr, prog, vers, 0, 0);
	trace3(TR_clnt_tp_create, 1, prog, vers);
	return (cl);
}

/*
 * Generic client creation:  returns client handle.
 * Default options are set, which the user can
 * change using the rpc equivalent of ioctl()'s : clnt_control().
 * If fd is RPC_ANYFD, it will be opened using nconf.
 * It will be bound if not so.
 * If sizes are 0; appropriate defaults will be chosen.
 */
CLIENT *
clnt_tli_create(fd, nconf, svcaddr, prog, vers, sendsz, recvsz)
	register int fd;		/* fd */
	struct netconfig *nconf;	/* netconfig structure */
	struct netbuf *svcaddr;		/* servers address */
	u_long prog;			/* program number */
	u_long vers;			/* version number */
	u_int sendsz;			/* send size */
	u_int recvsz;			/* recv size */
{
	CLIENT *cl;			/* client handle */
	struct t_info tinfo;		/* transport info */
	bool_t madefd = FALSE;		/* whether fd opened here */
	long servtype;
	extern char *strdup();
	rpc_createerr_t rpc_createerr = { 0 };

	trace5(TR_clnt_tli_create, 0, prog, vers, sendsz, recvsz);
	if (fd == RPC_ANYFD) {
		if (nconf == (struct netconfig *)NULL) {
			rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			set_rpc_createerr(rpc_createerr);
			trace3(TR_clnt_tli_create, 1, prog, vers);
			return ((CLIENT *)NULL);
		}
		fd = t_open(nconf->nc_device, O_RDWR, NULL);
		if (fd == -1)
			goto err;
		madefd = TRUE;
		if (t_bind(fd, (struct t_bind *)NULL,
			(struct t_bind *)NULL) == -1)
				goto err;

		switch (nconf->nc_semantics) {
		case NC_TPI_CLTS:
			servtype = T_CLTS;
			break;
		case NC_TPI_COTS:
			servtype = T_COTS;
			break;
		case NC_TPI_COTS_ORD:
			servtype = T_COTS_ORD;
			break;
		default:
			if (t_getinfo(fd, &tinfo) == -1)
				goto err;
			servtype = tinfo.servtype;
			break;
		}
	} else {
		int state;		/* Current state of provider */

		/*
		 * Sync the opened fd.
		 * Check whether bound or not, else bind it
		 */
		if (((state = t_sync(fd)) == -1) ||
		    ((state == T_UNBND) && (t_bind(fd, (struct t_bind *)NULL,
				(struct t_bind *)NULL) == -1)) ||
		    (t_getinfo(fd, &tinfo) == -1))
			goto err;
		servtype = tinfo.servtype;
		madefd = FALSE;
	}

	switch (servtype) {
	case T_COTS:
	case T_COTS_ORD:
		cl = clnt_vc_create(fd, svcaddr, prog, vers, sendsz, recvsz);
		break;
	case T_CLTS:
		cl = clnt_dg_create(fd, svcaddr, prog, vers, sendsz, recvsz);
		break;
	default:
		goto err;
	}

	if (cl == (CLIENT *)NULL)
		goto err1; /* borrow errors from clnt_dg/vc creates */
	if (nconf) {
		cl->cl_netid = strdup(nconf->nc_netid);
		cl->cl_tp = strdup(nconf->nc_device);
	} else {
		cl->cl_netid = "";
		cl->cl_tp = "";
	}
	if (madefd)
		(void) CLNT_CONTROL(cl, CLSET_FD_CLOSE, (char *)NULL);

	trace3(TR_clnt_tli_create, 1, prog, vers);
	return (cl);

err:
	rpc_createerr.cf_stat = RPC_TLIERROR;
	rpc_createerr.cf_error.re_errno = errno;
	rpc_createerr.cf_error.re_terrno = t_errno;
	set_rpc_createerr(rpc_createerr);
err1:	if (madefd)
		(void) t_close(fd);
	trace3(TR_clnt_tli_create, 1, prog, vers);
	return ((CLIENT *)NULL);
}
