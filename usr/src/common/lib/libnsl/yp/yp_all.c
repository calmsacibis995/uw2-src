/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/yp/yp_all.c	1.2.8.5"
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

#define NULL 0

#include <rpc/rpc.h>
#include <sys/syslog.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include "yp_b.h"
#include "yp_mt.h"

const static struct timeval tp_timout = {
	120,	/* seconds */
	0
	};
extern int _yp_dobind();

/*
 * This does the "glommed enumeration" stuff.  callback->foreach is the name
 * of a function which gets called per decoded key-value pair:
 * 
 * (*callback->foreach)(status, key, keylen, val, vallen, callback->data);
 *
 * If the server we get back from _yp_dobind speaks the old protocol, this
 * returns YPERR_VERS, and does not attempt to emulate the new functionality
 * by using the old protocol.
 */
int
yp_all(domain, map, callback)
	char *domain;
	char *map;
	struct ypall_callback *callback;
{
	int domlen;
	int maplen;
	struct ypreq_nokey req;
	int reason;
	struct dom_binding *pdomb;
	enum clnt_stat s;
	CLIENT *allc;
	char *server;
	long high_version;

	if ( (map == NULL) || (domain == NULL) ) {
		return(YPERR_BADARGS);
	}
	
	domlen = (int) strlen(domain);
	maplen = (int) strlen(map);
	
	if ( (domlen == 0) || (domlen > YPMAXDOMAIN) ||
	    (maplen == 0) || (maplen > YPMAXMAP) ||
	    (callback == (struct ypall_callback *) NULL) ) {
		return(YPERR_BADARGS);
	}

	if (reason = _yp_dobind(domain, &pdomb) ) {
		/*
		 * failure of _yp_dobind means no lock is held.
		 */
		return(reason);
	}
	/*
	 * the _yp_domain_list_lock is now held in reader or writer mode.
	 */
	high_version = pdomb->dom_binding->ypbind_hi_vers;
	server = strdup(pdomb->dom_binding->ypbind_servername);
	RW_UNLOCK(&_yp_domain_list_lock);
	if (server == NULL) {
		return (YPERR_RESRC);
	}

	if (high_version < YPVERS) {
		free(server);
		return (YPERR_VERS);
	}

	/*
	 * we do not need the _yp_domain_entry_lock as a local client
	 * handle is used here.
	 */
	if ((allc = clnt_create(server, YPPROG, YPVERS, "circuit_n"))
	    == (CLIENT *) NULL) {
		char	*errmsg;

		errmsg = clnt_spcreateerror(
				"yp_all - transport level create failure");
		if (errmsg != NULL)
			(void) syslog(LOG_ERR, errmsg);
		free (server);
		return(YPERR_RPC);
	}
	free (server);

	req.domain = domain;
	req.map = map;
	
	s = clnt_call(allc, YPPROC_ALL, (xdrproc_t)xdr_ypreq_nokey,
	    (caddr_t)&req, (xdrproc_t)xdr_ypall, (caddr_t)callback, tp_timout);

	if (s != RPC_SUCCESS) {
		char	*errmsg;

		errmsg = clnt_sperror(allc,
			"yp_all - RPC clnt_call (transport level) failure");
		if (errmsg != NULL)
			(void) syslog(LOG_ERR, errmsg);
	}

	clnt_destroy(allc);	

	if (s == RPC_SUCCESS) {
		return(0);
	} else {
		return(YPERR_RPC);
	}
}
