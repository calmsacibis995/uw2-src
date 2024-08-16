/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/yp/yp_b_clnt.c	1.1.7.2"
#ident  "$Header: $"

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
*	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

#include <rpc/rpc.h>
#include <sys/time.h>
#include "yp_b.h"
#define bzero(a,b) memset(a,0,b)
#define YPBIND_ERR_ERR 1		/* Internal error */
#define YPBIND_ERR_NOSERV 2		/* No bound server for passed domain */
#define YPBIND_ERR_RESC 3		/* System resource allocation failure */
#define YPBIND_ERR_NODOMAIN 4		/* Domain doesn't exist */

/* Default timeout can be changed using clnt_control() */
const static struct timeval TIMEOUT = { 25, 0 };

void *
ypbindproc_null_3x(argp, clnt, res)
	void *argp;
	CLIENT *clnt;
        char *res;
{
	bzero((char *)res, sizeof(*res));
	if (clnt_call(clnt, YPBINDPROC_NULL, xdr_void, argp, xdr_void, res,
			TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)res);
}

ypbind_resp_t *
ypbindproc_domain_3x(argp, clnt, res)
	ypbind_domain *argp;
	CLIENT *clnt;
	ypbind_resp_t *res;
{
	bzero((char *)res, sizeof(*res));
	if (clnt_call(clnt, YPBINDPROC_DOMAIN, (xdrproc_t)xdr_ypbind_domain,
			(caddr_t)argp, (xdrproc_t)xdr_ypbind_resp,
			(caddr_t)res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}

	return (res);
}

void *
ypbindproc_setdom_3x(argp, clnt, res)
	ypbind_setdom *argp;
	CLIENT *clnt;
	char *res;
{
	bzero((char *)res, sizeof(*res));
	if (clnt_call(clnt, YPBINDPROC_SETDOM, xdr_ypbind_setdom,
			(caddr_t)argp, xdr_void, (caddr_t)res, TIMEOUT)
			!= RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)res);
}

/*
 * thread UN-safe versions of above routines. provided only for
 * compatibility.
 */


void *
ypbindproc_null_3(argp, clnt)
        void *argp;
        CLIENT *clnt;
{
        static char res;

        bzero((char *)&res, sizeof(res));
        if (clnt_call(clnt, YPBINDPROC_NULL, xdr_void, argp, xdr_void,
			&res, TIMEOUT) != RPC_SUCCESS) {
                return (NULL);
        }
        return ((void *)&res);
}

ypbind_resp_t *
ypbindproc_domain_3(argp, clnt)
        ypbind_domain *argp;
        CLIENT *clnt;
{
        static ypbind_resp_t res;

        bzero((char *)&res, sizeof(res));
        if (clnt_call(clnt, YPBINDPROC_DOMAIN, xdr_ypbind_domain,
			(caddr_t)argp, xdr_ypbind_resp, (caddr_t)&res,
			TIMEOUT) != RPC_SUCCESS) {
                return (NULL);
        }
        return (&res);
}

void *
ypbindproc_setdom_3(argp, clnt)
        ypbind_setdom *argp;
        CLIENT *clnt;
{
        static char res;

        bzero((char *)&res, sizeof(res));
        if (clnt_call(clnt, YPBINDPROC_SETDOM, xdr_ypbind_setdom,
			(caddr_t)argp, xdr_void, (caddr_t)&res, TIMEOUT)
			!= RPC_SUCCESS) {
                return (NULL);
        }
        return ((void *)&res);
}
