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

#ident	"@(#)rpcsvc:spray_clnt.c	1.1.5.5"
#ident  "$Header: spray_clnt.c 1.2 91/06/27 $"

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
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 
/*
 * "spray_clnt.c 1.1 89/03/23 Copyr 1984 Sun Micro";
 */

/*
 * Please do not edit this file.
 * It was generated using rpcgen.
 */

#include <rpc/rpc.h>
#include <rpcsvc/spray.h>

/* Default timeout can be changed using clnt_control() */
static struct timeval TIMEOUT = { 25, 0 };

void *
sprayproc_spray_1(argp, clnt)
	sprayarr *argp;
	CLIENT *clnt;
{
	static char res;

	(void) memset((char *)&res, 0, sizeof(res));
	if (clnt_call(clnt, SPRAYPROC_SPRAY, xdr_sprayarr, (caddr_t)argp,
			xdr_void, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)&res);
}

spraycumul *
sprayproc_get_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static spraycumul res;

	(void) memset((char *)&res, 0, sizeof(res));
	if (clnt_call(clnt, SPRAYPROC_GET, xdr_void, (caddr_t)argp,
			xdr_spraycumul, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return (&res);
}

void *
sprayproc_clear_1(argp, clnt)
	void *argp;
	CLIENT *clnt;
{
	static char res;

	(void) memset((char *)&res, 0, sizeof(res));
	if (clnt_call(clnt, SPRAYPROC_CLEAR, xdr_void, argp, xdr_void, &res, TIMEOUT) != RPC_SUCCESS) {
		return (NULL);
	}
	return ((void *)&res);
}