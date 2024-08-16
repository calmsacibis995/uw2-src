/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/yp/yp_bind.c	1.4.9.7"
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include <rpc/rpcb_prot.h>
#include <sys/syslog.h>
#include "yp_b.h"
#include <sys/socket.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <sys/utsname.h>
#include <netdir.h>
#include "yp_mt.h"

#define BFSIZE (YPMAXDOMAIN + 32) /* size of binding file */
#define YPBINDPROTO "udp"

/* This should match the one in ypbind.c */

#define CACHE_DIR "/var/yp/binding"	

extern pid_t			getpid();
extern unsigned			sleep();
extern int			getdomainname();


static bool			check_binding();
static void			newborn();
static struct dom_binding	*load_dom_binding();
static void			_yp_dounbind ();

/*
 * Time parameters when talking to the ypbind and pmap processes
 */

#define YPSLEEPTIME 1			/* Time to sleep between tries */
const unsigned int _ypsleeptime = YPSLEEPTIME;

/*
 * Time parameters when talking to the ypserv process
 */

#ifdef  DEBUG
#define YPTIMEOUT 120			/* Total seconds for timeout */
#define YPINTER_TRY 60			/* Seconds between tries */
#else
#define YPTIMEOUT 20			/* Total seconds for timeout */
#define YPINTER_TRY 5			/* Seconds between tries */
#endif

#define MAX_TRIES_FOR_NEW_YP 1		/* Number of times we'll try to get
					 *   a new YP server before we'll
					 *   settle for an old one. */
const   struct timeval _ypserv_timeout = {
	YPTIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};


static struct dom_binding *bound_domains; /* List of bound domains */

static struct timeval ypbindtout = { 90, 0 };
static struct ypbindaddr {
	struct netbuf *addr;
	struct netconfig *nconf;
}ypaddr;

/*
 * This provides a quit and easy way of getting ypbind's
 * address from rpcbind on the local host. 
 * The lock _yp_domain_list_lock must be held for writing when
 * calling this routine, since it uses static variables.
 */
static struct netbuf *
_ping_rpcbind(nconf)
struct netconfig **nconf;
{
	struct sockaddr_in sin;
	char *localaddr = "\000\000\000\000";
	char nullstring[] = "\000";
	char *ua = NULL;
	register CLIENT *clnt;
	RPCB parms;
	static struct netbuf addr, *svcaddr;
	static struct timeval tottimeout = { 3, 0 };
	int stat;

	if (*nconf == NULL){
		if ((*nconf = getnetconfigent(YPBINDPROTO)) == NULL)
			return(NULL);
	}
	addr.buf = (char *)&sin;
	addr.len = sizeof(sin);
	sin.sin_family = AF_INET;
	sin.sin_port =  htons(111);
	sin.sin_addr = *((struct in_addr *)localaddr);
	clnt = clnt_tli_create(RPC_ANYFD, *nconf, &addr, RPCBPROG,
		 RPCBVERS, 0, 0);
	if (clnt == NULL){
		return(NULL);
	}
	CLNT_CONTROL(clnt, CLSET_TIMEOUT, (char *)&tottimeout);
	CLNT_CONTROL(clnt, CLSET_RETRY_TIMEOUT, (char *)&tottimeout);
	parms.r_prog = YPBINDPROG;
	parms.r_vers = YPBINDVERS;
	parms.r_netid = (*nconf)->nc_netid;
	parms.r_addr = nullstring;
	parms.r_owner = nullstring;
	stat =  CLNT_CALL(clnt, RPCBPROC_GETADDR, xdr_rpcb,
		(caddr_t)&parms, xdr_wrapstring, (caddr_t)&ua, tottimeout);

	CLNT_DESTROY(clnt);
	if (stat || ua == NULL  || *ua == NULL) {
		return(NULL);
	}
	if ((svcaddr = uaddr2taddr(*nconf, ua)) == NULL){
			return(NULL);
	}
	return(svcaddr);
}
/*
 * ping the local ypbind to see if it is bound. This use used by
 * the code in tcpip_nis.so to stop recurives problems 
 * when ypbind is using NIS to find the NIS server.
 * This routine must be called while holding the lock _yp_domain_list_lock
 * for writing, since it calls check_binding() and _ping_rpcbind().
 */
_ping_ypbind(domain)
char *domain;
{
	struct netconfig *nconf=NULL;
	struct netbuf *svcaddr;
	struct ypbind_domain ypbd;
	struct ypbind_resp resp;
	register CLIENT *clnt;
	int stat;
	struct dom_binding *binding;

	if (check_binding(domain, &binding) ) {
		return (1);		/* We are bound */
	}
	/*
	 * Get ypbind's address from rpcbind and create client handle
	 */
	if ((svcaddr = _ping_rpcbind(&nconf)) == NULL)
		return(0);

	clnt = clnt_tli_create(RPC_ANYFD, nconf, svcaddr, YPBINDPROG,
		YPBINDVERS, 0, 0);

	if (clnt == (CLIENT *)NULL)
		return(0);

	/*
	 * Reset default timeouts
	 */
	CLNT_CONTROL(clnt, CLSET_TIMEOUT, (char *)&ypbindtout);
	CLNT_CONTROL(clnt, CLSET_RETRY_TIMEOUT, (char *)&ypbindtout);

	ypbd.ypbind_domainname = domain;
	ypbd.ypbind_vers = YPVERS;
	memset((char *)&resp, 0, sizeof(struct ypbind_resp));

	/*
	 * Call ypbind 
	 */
	stat = clnt_call(clnt, YPBINDPROC_DOMAIN, xdr_ypbind_domain,
		(caddr_t)&ypbd, xdr_ypbind_resp, (caddr_t)&resp,ypbindtout);
	CLNT_DESTROY(clnt);

	if (stat || resp.ypbind_status != YPBIND_SUCC_VAL)
		return(0); /* not up or not bound */

	return(1); /* bound */
}
/*
 * Attempts to locate a YP server that serves a passed domain.  If one 
 * is found, an entry is created on the static list of domain-server pairs
 * pointed to by cell bound_domains, a udp path to the server is created and
 * the function returns 0.  Otherwise, the function returns a defined errorcode
 * YPERR_xxxx.
 *
 * Called with _yp_domain_list_lock not held. 
 * It is held in reader mode on exit, if success is returned. 
 * On failure, no lock is held on exit.
 */
int
_yp_dobind(char *domain, struct dom_binding **binding)
{
	struct dom_binding	*pdomb;	/* Ptr to new domain binding */
	struct ypbind_resp	ypbind_resp; /* Response from local ypbinder */
	struct ypbind_domain	ypbd; 
	CLIENT			*tb;
	int			status;
	int			tries;

	if ( (domain == NULL) ||((int) strlen(domain) == 0) ) {
		return (YPERR_BADARGS);
	}

	/*
	 * get the lock in writer mode as newborn may require it.
	 */
	RW_WRLOCK(&_yp_domain_list_lock);
	newborn();
	RW_UNLOCK(&_yp_domain_list_lock);

	RW_RDLOCK(&_yp_domain_list_lock);
	if (check_binding(domain, binding) ) {
		return (0);		/* We are bound */
	}

	RW_UNLOCK(&_yp_domain_list_lock);

	while(1) {
		/*
		 * We create a clnt handle to ypbind by first
		 * getting the its address from rpcbind. If we
		 * get the address, cached it, so it can be 
		 * used again. Then use clnt_tli_create() to
		 * create the handle. 
		 *
		 * NOTE: We use this non-standared way of creating a 
		 * clnt handle, to save time when rpcbind and/or
		 * ypbind are down. 
		 */
		RW_WRLOCK(&_yp_domain_list_lock);
		/*
		 * Call rpcbind to get ypbind's address
		 */
		if (ypaddr.addr == NULL){
			ypaddr.addr = _ping_rpcbind(&ypaddr.nconf);
			if (ypaddr.addr == NULL){
				RW_UNLOCK(&_yp_domain_list_lock);
				return(YPERR_YPBIND);
			}
		}
		RW_UNLOCK(&_yp_domain_list_lock);

		tb=clnt_tli_create(RPC_ANYFD, ypaddr.nconf, ypaddr.addr, 
				YPBINDPROG, YPBINDVERS, 0, 0);
		if (tb == NULL) {
			return(YPERR_YPBIND);
		}
		/*
		 * reset default timeouts
		 */
		CLNT_CONTROL(tb, CLSET_TIMEOUT, (char *)&ypbindtout);
		CLNT_CONTROL(tb, CLSET_RETRY_TIMEOUT, (char *)&ypbindtout);

		for(tries = 0; tries < 5; tries++) {
			ypbd.ypbind_domainname = domain;
			ypbd.ypbind_vers = YPVERS;
			if (!ypbindproc_domain_3x(&ypbd,tb,&ypbind_resp)) {
				/*lost ypbind?*/
				clnt_perror(tb,"ypbindproc_domain_3");
				RW_WRLOCK(&_yp_domain_list_lock);
				/*
				 * Clear addr to get ypbind's address again.
				 * ypbind may have gone down and come back up,
				 * which means this is a bad address.
				 */
				ypaddr.addr = NULL;
				RW_UNLOCK(&_yp_domain_list_lock);
				break;
			}
			if (ypbind_resp.ypbind_status == YPBIND_SUCC_VAL) {
				if ((pdomb = load_dom_binding(&ypbind_resp,
						domain, &status) ) ==
						(struct dom_binding *) NULL) {
					clnt_destroy(tb);
					return (status);
				}
				clnt_destroy(tb);

				/*
				 * Return ptr to the binding entry
				 */
				RW_RDLOCK(&_yp_domain_list_lock);
#ifdef _REENTRANT
				if (MULTI_THREADED) {
				  /*
				   * check for races again....
				   */
				  if (check_binding(domain, binding)) {
				    /*
				     * all is well. If raced, was harmless
				     */
				    return 0;
				  } else {
				    /*
				     * The entry just created has vanished
				     * and not been replaced.
				     */
				    RW_UNLOCK(&_yp_domain_list_lock);
				    /*
				     * retry the whole thing again
				     */
				    continue;
				  }
				}
#endif
				/*
				 * if we get here, we know a race is impossible
				 * because we aren't multithreaded, or maybe
				 * even aren't built to be multithread safe.
				 * So just use the entry in hand (more 
				 * efficient)
				 */
				*binding = pdomb;
				return (0);
			}

			if (ypbind_resp.ypbind_resp_u.ypbind_error !=
							YPBIND_ERR_NOSERV) {
				clnt_destroy(tb);
				switch(ypbind_resp.ypbind_resp_u.ypbind_error) {
				case YPERR_YPSERV:
					return(YPERR_YPSERV);
					break;
				default:
					return(YPERR_YPBIND);
					break;
				}
			}
			(void) sleep(_ypsleeptime*tries);
		} /* end of for loop */
		clnt_destroy(tb);
	} /* end of while loop */
}

/*
 * This is a "wrapper" function for _yp_dobind for vanilla user-level
 * functions which neither know nor care about struct dom_bindings.
 *
 * Called with _yp_domain_list_lock not held.
 */
int
yp_bind(domain)
char *domain;
{
	struct	dom_binding	*binding;
	int			retval;
	
	retval = _yp_dobind(domain, &binding);
	if (retval == 0)
        	RW_UNLOCK(&_yp_domain_list_lock);

	return retval;
}

/*
 * Attempts to find a dom_binding in the list at bound_domains having the
 * domain name field equal to the passed domain name, and removes it if found.
 * The domain-server binding will not exist after the call to this function.
 * All resources associated with the binding will be freed.
 *
 * Called with _yp_domain_list_lock held in writer mode. The lock is still
 * held on exit. If you don't have the lock, call yp_unbind() instead. 
 */
static void
_yp_dounbind (domain)
char *domain;
{
	struct dom_binding *pdomb;
	struct dom_binding *ptrail = 0;


	if ( (domain == NULL) ||((int) strlen(domain) == 0) ) {
		return;
	}

	for (pdomb = bound_domains; pdomb != NULL;
				ptrail = pdomb, pdomb = pdomb->dom_pnext) {
		if (strcmp(domain, pdomb->dom_domain) == 0) {
			clnt_destroy(pdomb->dom_client);

			if (pdomb == bound_domains) {
				bound_domains = pdomb->dom_pnext;
			} else {
				ptrail->dom_pnext = pdomb->dom_pnext;
			}

			free((char *) pdomb);
			break;
		}

	}
}

/*
 * This is a "wrapper" function for _yp_dounbind(). Its added value is that
 * it handles locks.
 */
void
yp_unbind(domain)
char *domain;
{

        RW_WRLOCK(&_yp_domain_list_lock);
	_yp_dounbind(domain);
        RW_UNLOCK(&_yp_domain_list_lock);
}

/*
 * This is a wrapper for the system call getdomainname which returns a
 * ypclnt.h error code in the failure case.
 */
int
yp_get_default_domain(domain)
char **domain;
{
	if (_rpc_get_default_domain(domain) == 0)
		return (0);
	return (YPERR_YPERR);
}

/*
 * This checks to see if this is a new process incarnation which has
 * inherited bindings from a parent, and unbinds the world if so.
 *
 * Called with _yp_domain_list_lock held in writer mode. The lock is
 * still held on exit.
 */
static void
newborn()
{
	static pid_t	mypid;	/* Cached to detect forks */
	pid_t		testpid;

	if ((testpid = getpid() ) != mypid) {
		mypid = testpid;

		while (bound_domains) {
			_yp_dounbind(bound_domains->dom_domain);
		}
	}
}

/*
 * This checks that the socket for a domain which has already been bound
 * hasn't been closed or changed under us.  If it has, unbind the domain
 * without closing the socket, which may be in use by some higher level
 * code.  This returns TRUE and points the binding parameter at the found
 * dom_binding if the binding is found and the socket looks OK, and FALSE
 * otherwise.  
 *
 * Called with _yp_domain_list_lock held in reader mode. The lock may
 * also be held in writer mode on entry. The lock is still held on exit.
 */
static bool
check_binding(char *domain, struct dom_binding **binding)
{
	struct dom_binding	*pdomb;

	for (pdomb = bound_domains; pdomb != NULL; pdomb = pdomb->dom_pnext) {
		if (strcmp(domain, pdomb->dom_domain) == 0) {
			*binding = pdomb;
			return (TRUE);
		}
	}

	return (FALSE);
}

/*
 * This allocates some memory for a domain binding, initialize it, and
 * returns a pointer to it.  Based on the program version we ended up
 * talking to ypbind with, fill out an opvector of appropriate protocol
 * modules.  
 *
 * Called with no locks held.
 * Returns with no locks held.
 * Acquires for writing and releases _yp_domain_list_lock.
 */
static struct dom_binding *
load_dom_binding(ypbind_res, domain, err)
struct ypbind_resp *ypbind_res;
char *domain;
int *err;
{
	struct dom_binding *pdomb, *pbound;
	struct netconfig *nconf;

	/*
	 * Create a new struct dom_binding.
	 */

	pdomb = (struct dom_binding *) NULL;

	if ((pdomb = (struct dom_binding *) malloc(sizeof(struct dom_binding)))
	    == NULL) {
		(void) syslog(LOG_ERR, "load_dom_binding:  malloc failure.");
		*err = YPERR_RESRC;
		return (struct dom_binding *) (NULL);
	}

	pdomb->dom_binding = ypbind_res->ypbind_resp_u.ypbind_bindinfo;
	nconf = pdomb->dom_binding->ypbind_nconf;

	/*
	 * Open up a path to the server, which will remain active globally.
	 *	If using CLTS or "tcp" use netbuf passed by ypbind
	 */
	if (nconf->nc_semantics == (unsigned long) NC_TPI_CLTS || 
				(strcmp(nconf->nc_netid,"tcp") == 0)) {
		pdomb->dom_client = clnt_tli_create(RPC_ANYFD, nconf,
					pdomb->dom_binding->ypbind_svcaddr,
					YPPROG, YPVERS, 0, 0);
	} else  {
		/* 
		 * clnt_tp_create() calls rpc_findaddr(), which may 
		 * ultimately lead to an attempt to get _yp_domain_list_lock.
		 * Therefore, we must NOT be holding this lock here.
		 */
		pdomb->dom_client = clnt_tp_create(
		    			pdomb->dom_binding->ypbind_servername,
		    			(u_long) YPPROG, YPVERS, nconf);
	}

	if (pdomb->dom_client == (CLIENT * ) NULL) {
		clnt_pcreateerror("yp_bind: clnt_tp_create");
		free((char *) pdomb);
		*err = YPERR_RPC;
		return (struct dom_binding *) (NULL);
	}
	MUTEX_INIT(&pdomb->_yp_domain_entry_lock, USYNC_THREAD, NULL);

	/*
	 * We will be modifying bound_domains, so
	 * get _yp_domain_list_lock in writer mode.
	 */
	RW_WRLOCK(&_yp_domain_list_lock);
#ifdef _REENTRANT
	if (MULTI_THREADED) {
		/* Check for races */
		if (check_binding(domain, &pbound)) {
			/* Racing. Use the duplicate, discard our candidate. */
			clnt_destroy(pdomb->dom_client);
			free(pdomb);
			RW_UNLOCK(&_yp_domain_list_lock);
			return(pbound);
		}
	}
#endif

	pdomb->dom_pnext = bound_domains;	/* Link this to the list as */
	pdomb->dom_domain=malloc(strlen(domain)+(unsigned)1);
	if (pdomb->dom_domain == NULL){
		clnt_destroy(pdomb->dom_client);
		free((char *) pdomb);
		*err = YPERR_RESRC;
		RW_UNLOCK(&_yp_domain_list_lock);
		return (struct dom_binding *) (NULL);
	}

	(void) strcpy(pdomb->dom_domain, domain);
	bound_domains = pdomb;
	RW_UNLOCK(&_yp_domain_list_lock);

	return (pdomb);
}

int
usingypmap(ddn, map)
char **ddn;  /* the default domainname set by this routine */
char *map;  /* the map we are interested in. */
{
	char in, *outval = NULL;
	int outvallen, stat;
	char *tmp;

	if (!yp_get_default_domain(&tmp)) return FALSE;
	/* does the map exist ? */
	in = 0xFF;
	stat = yp_match(*ddn=tmp, map, &in, 1, &outval, &outvallen);
	if (outval != NULL)
		free(outval);
	switch (stat) {

	case 0:  /* it actually succeeded! */
	case YPERR_KEY:  /* no such key in map */
	case YPERR_NOMORE:
	case YPERR_BUSY:
		return (TRUE);
	}
	return (FALSE);
}
