/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/clnt_simple.c	1.2.9.3"
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
 * clnt_simple.c
 *
 * Simplified front end to client rpc.
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <rpc/rpc.h>
#include "trace.h"
#include <string.h>
#include <sys/param.h>
#include "rpc_mt.h"

#ifndef MAXHOSTNAMELEN
#define	MAXHOSTNAMELEN	256
#endif

#ifndef NETIDLEN
#define	NETIDLEN 32
#endif

#undef rpc_createerr	/* Need to have automatic to give set_rpc_createerr() */

/*
 * rpc_call_private:
 * This variable is used only under single-threaded or !_REENTRANT.
 * So no lock is held during its initialization here.
 */
static struct rpc_call_private {
	int	valid;			/* Is this entry valid ? */
	CLIENT	*client;		/* Client handle */
	u_long	prognum, versnum;	/* Program, version */
	char	host[MAXHOSTNAMELEN+1];	/* Servers host */
	char	nettype[NETIDLEN];	/* Network type */
} *rpc_call_private;

/*
 * This is the simplified interface to the client rpc layer.
 * The client handle is not destroyed here and is reused for
 * the future calls to same prog, vers, host and nettype combination.
 *
 * And this handle is maintained per-thread base.
 *
 * The total time available is 25 seconds.
 */
enum clnt_stat
rpc_call(host, prognum, versnum, procnum, inproc, in, outproc, out, nettype)
	char *host;			/* host name */
	u_long prognum;			/* program number */
	u_long versnum;			/* version number */
	u_long procnum;			/* procedure number */
	xdrproc_t inproc, outproc;	/* in/out XDR procedures */
	char *in, *out;			/* recv/send data */
	char *nettype;			/* nettype */
{
	register struct rpc_call_private *rcp;
	enum clnt_stat clnt_stat;
	struct timeval timeout, tottimeout;
	rpc_createerr_t rpc_createerr = { 0 };
#ifdef _REENTRANT
	struct _rpc_tsd *key_tbl;
#endif /* _REENTRANT */

	trace4(TR_rpc_call, 0, prognum, versnum, procnum);

#ifdef _REENTRANT
	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		if (rpc_call_private == NULL)
			rpc_call_private
			   = (struct rpc_call_private *)
			     calloc(1, sizeof(struct rpc_call_private));
                rcp = rpc_call_private;
	} else {

		/*
		 * This is the case of threads other than the first.
		 */
		key_tbl = (struct _rpc_tsd *)
			  _mt_get_thr_specific_storage(__rpc_key,
						       RPC_KEYTBL_SIZE);
		if (key_tbl == NULL) {
			trace4(TR_rpc_call, 1, prognum, versnum, procnum);
                	return(NULL);
		}
		if (key_tbl->call_p == NULL) 
			key_tbl->call_p
		   	= (void *)calloc(1, sizeof(struct rpc_call_private));
        	rcp = (struct rpc_call_private *)key_tbl->call_p;
	}
#else
	if (rpc_call_private == NULL)
		rpc_call_private
		   = (struct rpc_call_private *)
		     calloc(1, sizeof(struct rpc_call_private));
	rcp = rpc_call_private;
#endif /* _REENTRANT */

	if (rcp == (struct rpc_call_private *)NULL) {
		rpc_createerr.cf_stat = RPC_SYSTEMERROR;
		rpc_createerr.cf_error.re_errno = errno;
		set_rpc_createerr(rpc_createerr);
		trace4(TR_rpc_call, 1, prognum, versnum, procnum);
		return (RPC_SYSTEMERROR);
	}

	if ((nettype == NULL) || (nettype[0] == NULL))
		nettype = "netpath";
	if (!(rcp->valid && (rcp->prognum == prognum) &&
		(rcp->versnum == versnum) &&
		(!strcmp(rcp->host, host)) &&
		(!strcmp(rcp->nettype, nettype)))) {

		rcp->valid = 0;
		if (rcp->client)
			CLNT_DESTROY(rcp->client);
		/*
		 * Using the first successful transport for that type
		 */
		rcp->client = clnt_create(host, prognum, versnum, nettype);
		if (rcp->client == (CLIENT *)NULL) {
			trace4(TR_rpc_call, 1, prognum, versnum, procnum);
			rpc_createerr = get_rpc_createerr();
			return (rpc_createerr.cf_stat);
		}
		/*
		 * Set time outs for connectionless case.  Do it
		 * unconditionally.  Faster than doing a t_getinfo()
		 * and then doing the right thing.
		 */
		timeout.tv_usec = 0;
		timeout.tv_sec = 5;
		(void) CLNT_CONTROL(rcp->client,
				CLSET_RETRY_TIMEOUT, (char *) &timeout);
		rcp->prognum = prognum;
		rcp->versnum = versnum;
		if ((strlen(host) < (size_t)MAXHOSTNAMELEN) &&
		    (strlen(nettype) < (size_t)NETIDLEN)) {
			(void) strcpy(rcp->host, host);
			(void) strcpy(rcp->nettype, nettype);
			rcp->valid = 1;
		} else {
			rcp->valid = 0;
		}
	} /* else reuse old client */
	tottimeout.tv_sec = 25;
	tottimeout.tv_usec = 0;
	clnt_stat = CLNT_CALL(rcp->client, procnum, inproc, in, outproc,
				out, tottimeout);
	/*
	 * if call failed, empty cache
	 */
	if (clnt_stat != RPC_SUCCESS)
		rcp->valid = 0;
	trace4(TR_rpc_call, 1, prognum, versnum, procnum);
	return (clnt_stat);
}

#ifdef _REENTRANT

void
_free_rpc_call(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */
