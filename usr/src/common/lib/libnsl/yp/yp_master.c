/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/yp/yp_master.c	1.3.7.3"
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
#include "yp_b.h"
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include "yp_mt.h"

static int domaster();

extern char *malloc();
extern unsigned sleep();
extern char *strcpy();
extern size_t strlen();

extern struct timeval _ypserv_timeout;
extern int _yp_dobind();
extern unsigned int _ypsleeptime;

/*
 * This checks parameters, and implements the outer "until binding success"
 * loop.
 */
int
yp_master (domain, map, master)
	char *domain;
	char *map;
	char **master;
{
	int domlen;
	int maplen;
	int reason;
	struct dom_binding *pdomb;

	if ( (map == NULL) || (domain == NULL) ) {
		return (YPERR_BADARGS);
	}
	
	domlen = (int) strlen(domain);
	maplen = (int) strlen(map);
	
	if ( (domlen == 0) || (domlen > YPMAXDOMAIN) ||
	    (maplen == 0) || (maplen > YPMAXMAP) ||
	    (master == NULL) ) {
		return (YPERR_BADARGS);
	}

	for (;;) {
		if (reason = _yp_dobind(domain, &pdomb) ) {
			/*
			 * error means no lock held.
			 */
			return (reason);
		}

		if (pdomb->dom_binding->ypbind_hi_vers >= YPVERS) {
			/*
			 * get the _yp_domain_entry_lock to use handle.
			 */
		        MUTEX_LOCK (&pdomb->_yp_domain_entry_lock);
			reason = domaster(domain, map, pdomb, _ypserv_timeout,
						master);
		        MUTEX_UNLOCK (&pdomb->_yp_domain_entry_lock);
			RW_UNLOCK(&_yp_domain_list_lock);

			if (reason == YPERR_RPC) {
				yp_unbind(domain);
				(void) sleep(_ypsleeptime);
			} else {
				break;
			}
		} else {
			RW_UNLOCK(&_yp_domain_list_lock);
			return(YPERR_VERS);
		}
	}
	
	return (reason);
}

/*
 * This talks v2 to ypserv
 */
static int
domaster (domain, map, pdomb, timeout, master)
	char *domain;
	char *map;
	struct dom_binding *pdomb;
	struct timeval timeout;
	char **master;
{
	struct ypreq_nokey req;
	struct ypresp_master resp;
	unsigned int retval = 0;

	req.domain = domain;
	req.map = map;
	memset((char *) &resp, 0, sizeof(struct ypresp_master));

	/*
	 * Do the get_master request.  If the rpc call failed, return with
	 * status from this point.  
	 */
	
	if(clnt_call(pdomb->dom_client, YPPROC_MASTER,
	    (xdrproc_t)xdr_ypreq_nokey, (caddr_t)&req,
	    (xdrproc_t)xdr_ypresp_master, (caddr_t)&resp,
	    timeout) != RPC_SUCCESS) {
		return (YPERR_RPC);
	}

	/* See if the request succeeded */
	
	if (resp.status != YP_TRUE) {
		retval = ypprot_err((unsigned) resp.status);
	}

	/* Get some memory which the user can get rid of as he likes */

	if (!retval && ((*master = malloc(strlen(resp.master) + 1)) 
	    == NULL)) {
		retval = YPERR_RESRC;

	}

	if (!retval) {
		(void) strcpy(*master, resp.master);
	}
	
	CLNT_FREERES(pdomb->dom_client, (xdrproc_t)xdr_ypresp_master,
	    (caddr_t)&resp);
	return (retval);
}
