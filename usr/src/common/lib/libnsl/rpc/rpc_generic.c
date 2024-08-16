/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpc_generic.c	1.11.12.2"
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
 * rpc_generic.c
 *
 * Miscl routines for RPC.
 */

#include <stdio.h>
#include <sys/types.h>
#include "trace.h"
#include <rpc/rpc.h>
#include <rpc/nettype.h>
#include <sys/param.h>
#include <sys/mkdev.h>
#include <sys/stat.h>
#include <ctype.h>
#include <sys/resource.h>
#include <netconfig.h>
#include <malloc.h>
#include "rpc_mt.h"

struct handle {
	NCONF_HANDLE *nhandle;
	int nflag;		/* Whether NETPATH or NETCONFIG */
	int nettype;
};

struct _rpcnettype {
	const char *name;
	const int type;
} _rpctypelist[] = {
	"netpath", _RPC_NETPATH,
	"visible", _RPC_VISIBLE,
	"circuit_v", _RPC_CIRCUIT_V,
	"datagram_v", _RPC_DATAGRAM_V,
	"circuit_n", _RPC_CIRCUIT_N,
	"datagram_n", _RPC_DATAGRAM_N,
	"tcp", _RPC_TCP,
	"udp", _RPC_UDP,
	0, _RPC_NONE
};

/*
 * Cache the result of getrlimit(), so we don't have to do an
 * expensive call every time.
 */
/*
 * tbsize:
 * No lock is held during its initialization since there is
 * little damage on the system.
 */
int
_rpc_dtbsize()
{
	static int tbsize = 0;
	struct rlimit rl;

	trace1(TR___rpc_dtbsize, 0);
	if (tbsize) {
		trace1(TR___rpc_dtbsize, 1);
		return (tbsize);
	}
	if (getrlimit(RLIMIT_NOFILE, &rl) == 0) {
		tbsize = (rl.rlim_cur > FD_SETSIZE ? FD_SETSIZE:rl.rlim_cur);
		/*
		 * This is because the size of svc_fdset and pollfd_set
		 * are limited to FD_SETSIZE.
		 * Or just return FD_SETSIZE?
		 */
		trace1(TR___rpc_dtbsize, 1);
		return (tbsize);
	}
	/*
	 * Something wrong.  I'll try to save face by returning a
	 * pessimistic number.
	 */
	trace1(TR___rpc_dtbsize, 1);
	return (32);
}


/*
 * Find the appropriate buffer size
 */
u_int
_rpc_get_t_size(size, bufsize)
	int size;	/* Size requested */
	long bufsize;	/* Supported by the transport */
{
	trace3(TR___rpc_get_t_size, 0, size, bufsize);
	if (bufsize == -2) {
		/* transfer of data unsupported */
		trace3(TR___rpc_get_t_size, 1, size, bufsize);
		return ((u_int)0);
	}
	if (size == 0) {
		if ((bufsize == -1) || (bufsize == 0)) {
			/*
			 * bufsize == -1 : No limit on the size
			 * bufsize == 0 : Concept of tsdu foreign. Choose
			 *			a value.
			 */
			trace3(TR___rpc_get_t_size, 1, size, bufsize);
			return ((u_int)RPC_MAXDATASIZE);
		} else {
			trace3(TR___rpc_get_t_size, 1, size, bufsize);
			return ((u_int)bufsize);
		}
	}
	if ((bufsize == -1) || (bufsize == 0)) {
		trace3(TR___rpc_get_t_size, 1, size, bufsize);
		return ((u_int)size);
	}
	/* Check whether the value is within the upper max limit */
	trace3(TR___rpc_get_t_size, 1, size, bufsize);
	return (size > bufsize ? (u_int)bufsize : (u_int)size);
}

/*
 * Find the appropriate address buffer size
 */
u_int
_rpc_get_a_size(size)
	long size;	/* normally tinfo.addr */
{
	trace2(TR___rpc_get_a_size, 0, size);
	if (size >= 0) {
		trace2(TR___rpc_get_a_size, 1, size);
		return ((u_int)size);
	}
	if (size <= -2) {
		trace2(TR___rpc_get_a_size, 1, size);
		return ((u_int)0);
	}
	/*
	 * (size == -1) No limit on the size. we impose a limit here.
	 */
	trace2(TR___rpc_get_a_size, 1, size);
	return ((u_int)RPC_MAXADDRSIZE);
}

static char *
strlocase(p)
	char *p;
{
	char *t = p;

	trace1(TR_strlocase, 0);
	for (; *p; p++)
		if (isupper(*p))
			*p = tolower(*p);
	trace1(TR_strlocase, 1);
	return (t);
}

/*
 * Returns the type of the network as defined in <rpc/nettype.h>
 * If nettype is NULL, it defaults to NETPATH.
 */
static int
getnettype(nettype)
	char *nettype;
{
	int i;

	trace1(TR_getnettype, 0);
	if ((nettype == NULL) || (nettype[0] == NULL)) {
		trace1(TR_getnettype, 1);
		return (_RPC_NETPATH);	/* Default */
	}

	nettype = strlocase(nettype);
	for (i = 0; _rpctypelist[i].name; i++)
		if (strcmp(nettype, _rpctypelist[i].name) == 0) {
			trace1(TR_getnettype, 1);
			return (_rpctypelist[i].type);
		}
	trace1(TR_getnettype, 1);
	return (_rpctypelist[i].type);
}

/*
 * For the given nettype (tcp or udp only), return the first structure found.
 * This should be freed by calling freenetconfigent()
 */
/*
 * netid_tcp:
 * netid_udp:
 * __rpc_lock is held during their initialization.
 */
struct netconfig *
_rpc_getconfip(nettype)
	char *nettype;
{
	char *netid;
	static char *netid_tcp = NULL;
	static char *netid_udp = NULL;
	struct netconfig *dummy;

	trace1(TR___rpc_getconfip, 0);
	if (!netid_udp && !netid_tcp) {
		struct netconfig *nconf;
		extern char *strdup();
		void *confighandle;

		if (!(confighandle = setnetconfig())) {
			trace1(TR___rpc_getconfip, 1);
			return (NULL);
		}
		while (nconf = getnetconfig(confighandle)) {
			if (strcmp(nconf->nc_protofmly, NC_INET) == 0) {
				MUTEX_LOCK(&__rpc_lock);
				if (strcmp(nconf->nc_proto, NC_TCP) == 0 &&
				    !netid_tcp)
					netid_tcp = strdup(nconf->nc_netid);
				else if (strcmp(nconf->nc_proto, NC_UDP) == 0 &&
				    !netid_udp)
					netid_udp = strdup(nconf->nc_netid);
				MUTEX_UNLOCK(&__rpc_lock);
			}
		}
		endnetconfig(confighandle);
	}
	if (strcmp(nettype, "udp") == 0)
		netid = netid_udp;
	else if (strcmp(nettype, "tcp") == 0)
		netid = netid_tcp;
	else {
		trace1(TR___rpc_getconfip, 1);
		return ((struct netconfig *)NULL);
	}
	if ((netid == NULL) || (netid[0] == NULL)) {
		trace1(TR___rpc_getconfip, 1);
		return ((struct netconfig *)NULL);
	}
	dummy = getnetconfigent(netid);
	trace1(TR___rpc_getconfip, 1);
	return (dummy);
}


/*
 * Returns the type of the nettype, which should then be used with
 * _rpc_getconf().
 */
void *
_rpc_setconf(nettype)
	char *nettype;
{
	struct handle *handle;

	trace1(TR___rpc_setconf, 0);
	handle = (struct handle *) malloc(sizeof (struct handle));
	if (handle == NULL) {
		trace1(TR___rpc_setconf, 1);
		return (NULL);
	}
	switch (handle->nettype = getnettype(nettype)) {
	case _RPC_NETPATH:
	case _RPC_CIRCUIT_N:
	case _RPC_DATAGRAM_N:
		if (!(handle->nhandle = setnetpath())) {
			free(handle);
			trace1(TR___rpc_setconf, 1);
			return (NULL);
		}
		handle->nflag = TRUE;
		break;
	case _RPC_VISIBLE:
	case _RPC_CIRCUIT_V:
	case _RPC_DATAGRAM_V:
	case _RPC_TCP:
	case _RPC_UDP:
		if (!(handle->nhandle = setnetconfig())) {
			free(handle);
			trace1(TR___rpc_setconf, 1);
			return (NULL);
		}
		handle->nflag = FALSE;
		break;
	default:
		trace1(TR___rpc_setconf, 1);
		return (NULL);
	}

	trace1(TR___rpc_setconf, 1);
	return (handle);
}

/*
 * Returns the next netconfig struct for the given "net" type.
 * _rpc_setconf() should have been called previously.
 */
struct netconfig *
_rpc_getconf(vhandle)
	void *vhandle;
{
	struct handle *handle;
	struct netconfig *nconf;

	trace1(TR___rpc_getconf, 0);
	handle = (struct handle *)vhandle;
	if (handle == NULL) {
		trace1(TR___rpc_getconf, 1);
		return (NULL);
	}
	while (1) {
		if (handle->nflag)
			nconf = getnetpath(handle->nhandle);
		else
			nconf = getnetconfig(handle->nhandle);
		if (nconf == (struct netconfig *)NULL)
			break;
		if ((nconf->nc_semantics != NC_TPI_CLTS) &&
			(nconf->nc_semantics != NC_TPI_COTS) &&
			(nconf->nc_semantics != NC_TPI_COTS_ORD))
			continue;
		switch (handle->nettype) {
		case _RPC_VISIBLE:
			if (!(nconf->nc_flag & NC_VISIBLE))
				continue;
			/* falls through */
		case _RPC_NETPATH:	/* Be happy */
			break;
		case _RPC_CIRCUIT_V:
			if (!(nconf->nc_flag & NC_VISIBLE))
				continue;
			/* falls through */
		case _RPC_CIRCUIT_N:
			if ((nconf->nc_semantics != NC_TPI_COTS) &&
				(nconf->nc_semantics != NC_TPI_COTS_ORD))
				continue;
			break;
		case _RPC_DATAGRAM_V:
			if (!(nconf->nc_flag & NC_VISIBLE))
				continue;
			/* falls through */
		case _RPC_DATAGRAM_N:
			if (nconf->nc_semantics != NC_TPI_CLTS)
				continue;
			break;
		case _RPC_TCP:
			if (((nconf->nc_semantics != NC_TPI_COTS) &&
				(nconf->nc_semantics != NC_TPI_COTS_ORD)) ||
				strcmp(nconf->nc_protofmly, NC_INET) ||
				strcmp(nconf->nc_proto, NC_TCP))
				continue;
			break;
		case _RPC_UDP:
			if ((nconf->nc_semantics != NC_TPI_CLTS) ||
				strcmp(nconf->nc_protofmly, NC_INET) ||
				strcmp(nconf->nc_proto, NC_UDP))
				continue;
			break;
		}
		break;
	}
	trace1(TR___rpc_getconf, 1);
	return (nconf);
}

void
_rpc_endconf(vhandle)
	void * vhandle;
{
	struct handle *handle;

	trace1(TR___rpc_endconf, 0);
	handle = (struct handle *) vhandle;
	if (handle == NULL) {
		trace1(TR___rpc_endconf, 1);
		return;
	}
	if (handle->nflag) {
		endnetpath(handle->nhandle);
	} else {
		endnetconfig(handle->nhandle);
	}
	free(handle);
	trace1(TR___rpc_endconf, 1);
}

/*
 * Used to ping the NULL procedure for clnt handle.
 * Returns NULL if fails, else a non-NULL pointer.
 */
void *
rpc_nullproc(clnt)
	CLIENT *clnt;
{
	struct timeval TIMEOUT = {25, 0};

	trace2(TR_rpc_nullproc, 0, clnt);
	if (clnt_call(clnt, NULLPROC, (xdrproc_t) xdr_void, (char *)NULL,
		(xdrproc_t) xdr_void, (char *)NULL, TIMEOUT) != RPC_SUCCESS) {
		trace2(TR_rpc_nullproc, 1, clnt);
		return (NULL);
	}
	trace2(TR_rpc_nullproc, 1, clnt);
	return ((void *)1);
}

/*
 * Try all possible transports until
 * one succeeds in finding the netconf for the given fd.
 */
struct netconfig * _rpcgettp(fd)
int fd;
{
	struct stat statbuf;
	void *hndl;
	struct netconfig *nconf, *newnconf = NULL;
	dev_t devno;

	trace2(TR___rpcgettp, 0, fd);
	if (fstat(fd, &statbuf) == -1) {
		trace2(TR___rpcgettp, 1, fd);
		return (NULL);
	}

	devno = major(statbuf.st_rdev);
	hndl = setnetconfig();
	if (hndl == NULL) {
		trace2(TR___rpcgettp, 1, fd);
		return (NULL);
	}
	/*
	 * For a cloned device, the major device number will be
	 * equal to the minor device number of the device from which
	 * the clone was made. note that st_rdev has to be used
	 * rather than st_dev
	 */

	while (nconf = getnetconfig(hndl)) {
		if (!stat(nconf->nc_device, &statbuf) &&
		    (devno == minor(statbuf.st_rdev))) {
			break;
		}
	}
	if (nconf)
		newnconf = getnetconfigent(nconf->nc_netid);
	endnetconfig(hndl);
	trace2(TR___rpcgettp, 1, fd);
	return (newnconf);
}
