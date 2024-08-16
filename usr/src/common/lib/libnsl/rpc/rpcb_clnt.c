/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/rpcb_clnt.c	1.10.14.4"
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
 * rpcb_clnt.c
 * interface to rpcbind rpc service.
 */

#include <rpc/rpc.h>
#include "trace.h"
#include <rpc/rpcb_prot.h>
#include <netconfig.h>
#include <netdir.h>
#include <rpc/nettype.h>
#ifdef PORTMAP
#include <netinet/in.h>		/* FOR IPPROTO_TCP/UDP definitions */
#endif
#ifdef ND_DEBUG
#include <stdio.h>
#endif
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include "rpc_mt.h"

#undef rpc_createerr	/* Need automatic to give set_rpc_createerr() */

static struct timeval tottimeout = { 60, 0 };
static struct timeval rmttimeout = { 3, 0 };

#ifdef __STDC__
bool_t xdr_wrapstring(XDR *, char **);
#else
extern  bool_t xdr_wrapstring();
#endif

static char nullstring[] = "\000";

#define	CACHESIZE 6

struct address_cache {
	char *ac_host;
	char *ac_netid;
	char *ac_uaddr;
	struct netbuf *ac_taddr;
	struct address_cache *ac_next;
};

/*
 * front:
 * cachesize:
 * __rpcbind_lock is held in getclnthandle().
 */
static struct address_cache *front;
static int cachesize;

/*
 * The routines check_cache(), add_cache(), delete_cache() manage the
 * cache of rpcbind addresses for (host, netid).
 */

struct address_cache *
check_cache(host, netid)
	char *host, *netid;
{
	struct address_cache *cptr;

	trace1(TR_check_cache, 0);
	for (cptr = front; cptr != NULL; cptr = cptr->ac_next) {
		if (!strcmp(cptr->ac_host, host) &&
		    !strcmp(cptr->ac_netid, netid)) {
#ifdef ND_DEBUG
			fprintf(stderr, "Found cache entry for %s: %s\n",
				host, netid);
#endif
			trace1(TR_check_cache, 1);
			return (cptr);
		}
	}
	trace1(TR_check_cache, 1);
	return ((struct address_cache *) NULL);
}

void
delete_cache(ad_cache)
struct address_cache *ad_cache;
{
	struct address_cache *cptr, *prevptr = NULL;

	trace1(TR_delete_cache, 0);
	for (cptr = front; cptr != NULL; cptr = cptr->ac_next) {
		if (!strcmp(cptr->ac_host, ad_cache->ac_host) &&
		    !strcmp(cptr->ac_netid, ad_cache->ac_netid)) {
			free(cptr->ac_host);
			free(cptr->ac_netid);
			free(cptr->ac_taddr->buf);
			free(cptr->ac_taddr);
			if (cptr->ac_uaddr)
				free(cptr->ac_uaddr);
			if (prevptr)
				prevptr->ac_next = cptr->ac_next;
			else
				front = cptr->ac_next;
			free(cptr);
			cachesize--;
			break;
		}
		prevptr = cptr;
	}
	trace1(TR_delete_cache, 1);
}

void
add_cache(host, netid, taddr, uaddr)
char *host, *netid, *uaddr;
struct netbuf *taddr;
{
	struct address_cache  *ad_cache, *cptr, *prevptr;

	trace1(TR_add_cache, 0);
	ad_cache = (struct address_cache *)
			malloc(sizeof (struct address_cache));
	if (!ad_cache) {
		trace1(TR_add_cache, 1);
		return;
	}
	ad_cache->ac_uaddr = uaddr ? strdup(uaddr) : NULL;
	if ((uaddr && !ad_cache->ac_uaddr) ||
	    (ad_cache->ac_host = strdup(host)) == NULL ||
	    (ad_cache->ac_netid = strdup(netid)) == NULL ||
	    (ad_cache->ac_taddr =
	     (struct netbuf *)malloc(sizeof (struct netbuf))) == NULL) {
		if (ad_cache->ac_uaddr)
			free(ad_cache->ac_uaddr);
		if (ad_cache->ac_host)
			free(ad_cache->ac_host);
		if (ad_cache->ac_netid)
			free(ad_cache->ac_netid);
		free(ad_cache);
		trace1(TR_add_cache, 1);
		return;
	}
	ad_cache->ac_taddr->len = taddr->len;
	ad_cache->ac_taddr->maxlen = taddr->maxlen;
	ad_cache->ac_taddr->buf = (char *) malloc(taddr->len);
	if (ad_cache->ac_taddr->buf == NULL) {
		free(ad_cache->ac_taddr);
		free(ad_cache->ac_netid);
		free(ad_cache->ac_host);
		if (ad_cache->ac_uaddr)
			free(ad_cache->ac_uaddr);
		free(ad_cache);
		trace1(TR_add_cache, 1);
		return;
	}
	memcpy(ad_cache->ac_taddr->buf, taddr->buf, taddr->len);
#ifdef ND_DEBUG
	fprintf(stderr, "Added to cache: %s : %s\n", host, netid);
#endif

	if (cachesize < CACHESIZE) {
		ad_cache->ac_next = front;
		front = ad_cache;
		cachesize++;
	} else {
		/* Free the last entry */
		cptr = front;
		prevptr = NULL;
		while (cptr->ac_next) {
			prevptr = cptr;
			cptr = cptr->ac_next;
		}

#ifdef ND_DEBUG
		fprintf(stderr, "Deleted from cache: %s : %s\n",
			cptr->ac_host, cptr->ac_netid);
#endif
		free(cptr->ac_host);
		free(cptr->ac_netid);
		free(cptr->ac_taddr->buf);
		free(cptr->ac_taddr);
		if (cptr->ac_uaddr)
			free(cptr->ac_uaddr);

		if (prevptr) {
			prevptr->ac_next = NULL;
			ad_cache->ac_next = front;
			front = ad_cache;
		} else {
			front = ad_cache;
			ad_cache->ac_next = NULL;
		}
		free(cptr);
	}
	trace1(TR_add_cache, 1);
}

/*
 * This routine will return a client handle that is connected to the
 * rpcbind. Returns NULL on error and free's everything.
 */

/*
 * temporarily not static.
 */
CLIENT *
getclnthandle(host, nconf, targaddr)
	char *host;
	struct netconfig *nconf;
	char **targaddr;
{
	register CLIENT *client;
	struct netbuf addr1;
	struct netbuf *addr_ptr;
	struct nd_addrlist *nas;
	struct nd_hostserv rpcbind_hs;
	struct address_cache *ad_cache;
	char *tmpaddr;
	char *tmp_targaddr = NULL;
	rpc_createerr_t rpc_createerr = { 0 };

	trace1(TR_getclnthandle, 0);
	/* Get the address of the rpcbind.  Check cache first */
	MUTEX_LOCK(&__rpcbind_lock);
	ad_cache = check_cache(host, nconf->nc_netid);
	if (ad_cache != NULL) {
		addr1.len = ad_cache->ac_taddr->len;
		addr1.maxlen = ad_cache->ac_taddr->maxlen;
		addr1.buf = (char *) malloc(ad_cache->ac_taddr->len);
		if (addr1.buf == NULL) {
			MUTEX_UNLOCK(&__rpcbind_lock);
			return(NULL);
		}
		memcpy(addr1.buf, ad_cache->ac_taddr->buf,
		       ad_cache->ac_taddr->len);
		if (ad_cache->ac_uaddr) {
			tmp_targaddr = strdup(ad_cache->ac_uaddr);
			if (tmp_targaddr == NULL) {
				free(addr1.buf);
				MUTEX_UNLOCK(&__rpcbind_lock);
				return(NULL);
			}
		}
		MUTEX_UNLOCK(&__rpcbind_lock);

		client = clnt_tli_create(RPC_ANYFD, nconf, &addr1, RPCBPROG,
					 RPCBVERS4, 0, 0);
		free(addr1.buf);
		if (client != NULL) {
			if (targaddr)
				*targaddr = tmp_targaddr;
			else {
				if (tmp_targaddr)
					free(tmp_targaddr);
			}
			trace1(TR_getclnthandle, 1);
			return (client);
		} else {
			/*
			 * Assume this may be due to cache data being
			 *  outdated
			 */
			if (tmp_targaddr)
				free(tmp_targaddr);
#ifdef _REENTRANT
			MUTEX_LOCK(&__rpcbind_lock);
			if (!MULTI_THREADED ||
			    check_cache(host, nconf->nc_netid) == ad_cache)
				delete_cache(ad_cache);
			MUTEX_UNLOCK(&__rpcbind_lock);
#else
			delete_cache(ad_cache);
#endif /* _REENTRANT */
		}
	} else
		MUTEX_UNLOCK(&__rpcbind_lock);

	rpcbind_hs.h_host = host;
	rpcbind_hs.h_serv = "rpcbind";
#ifdef ND_DEBUG
	fprintf(stderr, "rpcbind client routines: diagnostics :\n");
	fprintf(stderr, "\tGetting address for (%s, %s, %s) ... \n",
		rpcbind_hs.h_host, rpcbind_hs.h_serv, nconf->nc_netid);
#endif

	if (netdir_getbyname(nconf, &rpcbind_hs, &nas)) {
		rpc_createerr.cf_stat = RPC_N2AXLATEFAILURE;
		set_rpc_createerr(rpc_createerr);
		trace1(TR_getclnthandle, 1);
		return ((CLIENT *)NULL);
	}
	/* XXX nas should perhaps be cached for better performance */

#ifdef ND_DEBUG
{
	char *ua;

	ua = taddr2uaddr(nconf, nas->n_addrs);
	fprintf(stderr, "Got it [%s]\n", ua);
	free(ua);
}
#endif

	addr_ptr = nas->n_addrs;
#ifdef ND_DEBUG
{
	int i;

	fprintf(stderr, "\tnetbuf len = %d, maxlen = %d\n",
		addr_ptr->len, addr_ptr->maxlen);
	fprintf(stderr, "\tAddress is");
	for (i = 0; i < addr_ptr->len; i++)
		fprintf(stderr, "%u.", addr_ptr->buf[i]);
	fprintf(stderr, "\n");
}
#endif
	client = clnt_tli_create(RPC_ANYFD, nconf, addr_ptr, RPCBPROG,
				RPCBVERS4, 0, 0);
#ifdef ND_DEBUG
	if (! client) {
		clnt_pcreateerror("rpcbind clnt interface");
	}
#endif

	if (client) {
		/* always make uaddr. */
		tmpaddr = taddr2uaddr(nconf, addr_ptr);
#ifdef _REENTRANT
		MUTEX_LOCK(&__rpcbind_lock);
		if (!MULTI_THREADED ||
		    check_cache(host, nconf->nc_netid) == NULL)
			add_cache(host, nconf->nc_netid, addr_ptr, tmpaddr);
		MUTEX_UNLOCK(&__rpcbind_lock);
#else
		add_cache(host, nconf->nc_netid, addr_ptr, tmpaddr);
#endif /* _REENTRANT */
		if (targaddr)
			*targaddr = tmpaddr;
	}
	netdir_free((char *)nas, ND_ADDRLIST);
	trace1(TR_getclnthandle, 1);
	return (client);
}

/*
 * This routine will return a client handle that is connected to the local
 * rpcbind. Returns NULL on error and free's everything.
 */
/*
 * hostname:
 * loopnconf:
 * __rpc_lock is held during this variable's initialization.
 */
static CLIENT *
local_rpcb()
{
	CLIENT *client;
	static struct netconfig *loopnconf;
	static char *hostname;
	rpc_createerr_t rpc_createerr = { 0 };

	trace1(TR_local_rpcb, 0);
	if (loopnconf == NULL) {
		struct utsname utsname;
		struct netconfig *nconf, *tmpnconf = NULL;
		void *nc_handle;
		char *h;

		if (hostname == (char *) NULL) {

			if ((uname(&utsname) == -1) ||
			    ((h = strdup(utsname.nodename)) == NULL)) {
				rpc_createerr.cf_stat = RPC_UNKNOWNHOST;
				set_rpc_createerr(rpc_createerr);
				return ((CLIENT *) NULL);
			}
			MUTEX_LOCK(&__rpc_lock);
			if (hostname == (char *) NULL)
				hostname = h;
			else
				free(h);
			MUTEX_UNLOCK(&__rpc_lock);
		}
		nc_handle = setnetconfig();
		if (nc_handle == NULL) {
			/* fails to open netconfig file */
			rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			set_rpc_createerr(rpc_createerr);
			trace1(TR_local_rpcb, 1);
			return (NULL);
		}
		while (nconf = getnetconfig(nc_handle)) {
			if (strcmp(nconf->nc_protofmly, NC_LOOPBACK) == 0) {
				tmpnconf = nconf;
				if (nconf->nc_semantics == NC_TPI_CLTS)
					break;
			}
		}
		if (tmpnconf == NULL) {
			rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			set_rpc_createerr(rpc_createerr);
			return (NULL);
		}
		nconf = getnetconfigent(tmpnconf->nc_netid);
		if (nconf == NULL) {
			rpc_createerr.cf_stat = RPC_SYSTEMERROR;
			set_rpc_createerr(rpc_createerr);
			return (NULL);
		}
		MUTEX_LOCK(&__rpc_lock);
		if (loopnconf == NULL) {
			loopnconf = nconf;
		} else
			freenetconfigent(nconf);
		MUTEX_UNLOCK(&__rpc_lock);
		/* loopnconf is never freed */
		endnetconfig(nc_handle);
	}
	client = getclnthandle(hostname, loopnconf, (char **)NULL);
	trace1(TR_local_rpcb, 1);
	return (client);
}

/*
 * Set a mapping between program, version and address.
 * Calls the rpcbind service remotely to do the mapping.
 */
bool_t
rpcb_set(program, version, nconf, address)
	u_long program;
	u_long version;
	struct netconfig *nconf;	/* Network structure of transport */
	struct netbuf *address;		/* Services netconfig address */
{
	register CLIENT *client;
	bool_t rslt = FALSE;
	RPCB parms;
	char uidbuf[32];
	rpc_createerr_t rpc_createerr = { 0 };

	trace3(TR_rpcb_set, 0, program, version);
	/* parameter checking */
	if (nconf == (struct netconfig *)NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_rpcb_set, 1, program, version);
		return (FALSE);
	}
	if (address == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNADDR;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_rpcb_set, 1, program, version);
		return (FALSE);
	}
	client = local_rpcb();
	if (! client) {
		trace3(TR_rpcb_set, 1, program, version);
		return (FALSE);
	}

	parms.r_addr = taddr2uaddr(nconf, address); /* convert to universal */
	if (!parms.r_addr) {
		rpc_createerr.cf_stat = RPC_N2AXLATEFAILURE;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_rpcb_set, 1, program, version);
		return (FALSE); /* no universal address */
	}
	parms.r_prog = program;
	parms.r_vers = version;
	parms.r_netid = nconf->nc_netid;
	/*
	 * Though uid is not being used directly, we still send it for
	 * completeness.  For non-unix platforms, perhaps some other
	 * string or an empty string can be sent.
	 */
	(void) sprintf(uidbuf, "%d", geteuid());
	parms.r_owner = uidbuf;

	CLNT_CALL(client, RPCBPROC_SET, (xdrproc_t) xdr_rpcb, (char *)&parms,
			(xdrproc_t) xdr_bool, (char *)&rslt, tottimeout);

	CLNT_DESTROY(client);
	free(parms.r_addr);
	trace3(TR_rpcb_set, 1, program, version);
	return (rslt);
}

/*
 * Remove the mapping between program, version and netbuf address.
 * Calls the rpcbind service to do the un-mapping.
 * If netbuf is NULL, unset for all the transports, otherwise unset
 * only for the given transport.
 */
bool_t
rpcb_unset(program, version, nconf)
	u_long program;
	u_long version;
	struct netconfig *nconf;
{
	register CLIENT *client;
	bool_t rslt = FALSE;
	RPCB parms;
	char uidbuf[32];

	trace3(TR_rpcb_unset, 0, program, version);
	client = local_rpcb();
	if (! client) {
		trace3(TR_rpcb_unset, 1, program, version);
		return (FALSE);
	}

	parms.r_prog = program;
	parms.r_vers = version;
	if (nconf)
		parms.r_netid = nconf->nc_netid;
	else
		parms.r_netid = nullstring; /* unsets  all */
	parms.r_addr = nullstring;
	(void) sprintf(uidbuf, "%d", geteuid());
	parms.r_owner = uidbuf;

	CLNT_CALL(client, RPCBPROC_UNSET, (xdrproc_t) xdr_rpcb, (char *)&parms,
			(xdrproc_t) xdr_bool, (char *)&rslt, tottimeout);

	CLNT_DESTROY(client);
	trace3(TR_rpcb_unset, 1, program, version);
	return (rslt);
}

/*
 * From the merged list, find the appropriate entry
 */
static struct netbuf *
got_entry(relp, nconf)
	rpcb_entry_list_ptr relp;
	struct netconfig *nconf;
{
	struct netbuf *na = NULL;
	rpcb_entry_list_ptr sp;
	rpcb_entry *rmap;

	trace1(TR_got_entry, 0);
	for (sp = relp; sp != NULL; sp = sp->rpcb_entry_next) {
		rmap = &sp->rpcb_entry_map;
		if ((strcmp(nconf->nc_netid, rmap->r_nc_netid) == 0) &&
		    (strcmp(nconf->nc_proto, rmap->r_nc_proto) == 0) &&
		    (strcmp(nconf->nc_protofmly, rmap->r_nc_protofmly) == 0) &&
		    (rmap->r_maddr != NULL) && (rmap->r_maddr[0] != NULL)) {
			na = uaddr2taddr(nconf, rmap->r_maddr);
#ifdef ND_DEBUG
			fprintf(stderr, "\tRemote address is [%s].\n",
				rmap->r_maddr);
			if (!na)
				fprintf(stderr,
				    "\tCouldn't resolve remote address!\n");
#endif
			break;
		}
	}
	trace1(TR_got_entry, 1);
	return (na);
}

/*
 * An internal function which optimizes rpcb_getaddr function.
 */
struct netbuf *
_rpcb_findaddr(program, version, nconf, host)
	u_long program;
	u_long version;
	struct netconfig *nconf;
	char *host;
{
	register CLIENT *client;
	RPCB parms;
	enum clnt_stat clnt_st;
	char *ua = NULL;
	long vers;
	struct netbuf *address = NULL;
	rpc_createerr_t rpc_createerr = { 0 };
	char *tmp_addr = NULL;

	trace3(TR_rpcb_findaddr, 0, program, version);
	/* parameter checking */
	if (nconf == (struct netconfig *)NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_rpcb_findaddr, 1, program, version);
		return (NULL);
	}
	parms.r_prog = program;
	parms.r_vers = version;
	parms.r_owner = nullstring;	/* not needed; just for xdring */
	parms.r_netid = nconf->nc_netid; /* not really needed */
	parms.r_addr = NULL;

	/*
	 * If a COTS transport is being used, try getting address via CLTS
	 * transport.
	 */
	if (nconf->nc_semantics == NC_TPI_COTS_ORD ||
	    nconf->nc_semantics == NC_TPI_COTS) {
		void *handle;
		struct netconfig *nconf_clts;
		rpcb_entry_list_ptr relp = NULL;

		if ((handle = _rpc_setconf("datagram_v")) != NULL) {
		    while ((nconf_clts = _rpc_getconf(handle)) != NULL) {
			if (strcmp(nconf_clts->nc_protofmly,
				   nconf->nc_protofmly) != 0) {
			    continue;
			}
			client = getclnthandle(host, nconf_clts, &tmp_addr);
			if (client == (CLIENT *)NULL) {
			    break;	/* Go the regular way */
			}
			/*
			 * We also send the remote system the address we
			 * used to contact it in case it can help it
			 * connect back with us
			 */
			if (tmp_addr == NULL)
			    parms.r_addr = nullstring;  /* for XDRing */
			else
			    parms.r_addr = tmp_addr;

			clnt_st = CLNT_CALL(client, RPCBPROC_GETADDRLIST,
				    (xdrproc_t) xdr_rpcb, (char *) &parms,
				    (xdrproc_t) xdr_rpcb_entry_list_ptr,
				    (char *) &relp, tottimeout);

			if (clnt_st == RPC_SUCCESS) {
			    if (address = got_entry(relp, nconf)) {
				_rpc_endconf(handle);
				xdr_free((xdrproc_t) xdr_rpcb_entry_list_ptr,
					(char *)&relp);
				CLNT_DESTROY(client);
				if (tmp_addr)
					free(tmp_addr);
				trace3(TR_rpcb_findaddr, 1, program, version);
				return (address);
			    }
			    xdr_free((xdrproc_t) xdr_rpcb_entry_list_ptr,
				     (char *)&relp);
			    CLNT_DESTROY(client);
			    break; /* Try the regular way */
			} else {
			    CLNT_DESTROY(client);
			    if (tmp_addr) {
			    	free(tmp_addr);
				tmp_addr = NULL;
			    }
		        }
		    }	/* While loop */
		    _rpc_endconf(handle);
		}
		if (parms.r_addr == nullstring)
			parms.r_addr = NULL;
		if (tmp_addr) {
			free(tmp_addr);
			tmp_addr = NULL;
		}
	}

	client = getclnthandle(host, nconf, &tmp_addr);
	if (client == (CLIENT *)NULL) {
		trace3(TR_rpcb_findaddr, 1, program, version);
		return (NULL);
	}

	/*
	 * We also send the remote system the address we used to
	 * contact it in case it can help it connect back with us
	 */
	if (tmp_addr == NULL)
		parms.r_addr = nullstring;
	else
		parms.r_addr = tmp_addr;

	for (vers = RPCBVERS4;  vers >= RPCBVERS; vers--) {
		/* Set the version */
		CLNT_CONTROL(client, CLSET_VERS, (char *)&vers);
		clnt_st = CLNT_CALL(client, RPCBPROC_GETADDR,
				    (xdrproc_t) xdr_rpcb, (char *) &parms,
				    (xdrproc_t) xdr_wrapstring,
				    (char *) &ua, tottimeout);
		if (clnt_st == RPC_SUCCESS) {
			if ((ua == NULL) || (ua[0] == NULL)) {
				/* address unknown */
				rpc_createerr.cf_stat = RPC_PROGNOTREGISTERED;
				set_rpc_createerr(rpc_createerr);
				goto done;
			}
			address = uaddr2taddr(nconf, ua);
#ifdef ND_DEBUG
			fprintf(stderr, "\tRemote address is [%s].\n", ua);
			if (!address)
				fprintf(stderr,
					"\tCouldn't resolve remote address!\n");
#endif
			xdr_free((xdrproc_t)xdr_wrapstring, (char *)&ua);

			if (! address) {
				/* We don't know about your universal address */
				rpc_createerr.cf_stat = RPC_N2AXLATEFAILURE;
				set_rpc_createerr(rpc_createerr);
				goto done;
			}
			break;
		} else if ((clnt_st != RPC_PROGVERSMISMATCH) &&
			   (clnt_st != RPC_PROGUNAVAIL)) {
			/* Cannot handle this error */
			rpc_createerr.cf_stat = clnt_st;
			clnt_geterr(client, &rpc_createerr.cf_error);
			set_rpc_createerr(rpc_createerr);
			goto done;
		}
	} /* end of for() */

#ifdef PORTMAP
	if (address == NULL) {
		/*
		 * rpcbind/portmapper do not return PROGVERSMISMATCH because
		 * of svc_versquiet, and hence this
		 */
		if (strcmp(nconf->nc_protofmly, NC_INET) == 0) {
			/*
			 * version 3 and 4 not available. Try version 2
			 * The assumption here is that the netbuf
			 * is arranged in the sockaddr_in style for IP cases.
			 */
			u_short port;
			u_int protocol;
			struct netbuf remote;

			CLNT_CONTROL(client, CLGET_SVC_ADDR, (char *)&remote);
			protocol = strcmp(nconf->nc_proto, NC_TCP) ?
			           IPPROTO_UDP : IPPROTO_TCP;
			/*
			 * XXX: Should have used the code directly from
			 * pmap_getport() because we already have all the
			 * client handles and other information.
			 */
			port = (u_short)pmap_getport((struct sockaddr_in *)
						remote.buf,
						program, version, protocol);
			if (port == 0) {
				/* error number set by pmap_getport() itself */
				goto done;
			}
			port = htons(port);
			if (((address = (struct netbuf *)
			            malloc(sizeof (struct netbuf))) == NULL) ||
			    ((address->buf = (char *)
				    malloc(remote.len)) == NULL)) {
				rpc_createerr.cf_stat = RPC_SYSTEMERROR;
				clnt_geterr(client, &rpc_createerr.cf_error);
				set_rpc_createerr(rpc_createerr);
				if (address) {
					free(address);
					address = NULL;
				}
				goto done;
			}
			memcpy(address->buf, remote.buf, remote.len);
			memcpy((char *)&address->buf[sizeof (short)],
					(char *)&port, sizeof (short));
			address->len = remote.len;
			address->maxlen = remote.maxlen;
		} else {
			rpc_createerr.cf_stat = clnt_st;
			clnt_geterr(client, &rpc_createerr.cf_error);
			set_rpc_createerr(rpc_createerr);
			goto done;
		}
	}

	if ((address == NULL) || (address->len == 0)) {
		rpc_createerr.cf_stat = RPC_PROGNOTREGISTERED;
		clnt_geterr(client, &rpc_createerr.cf_error);
		set_rpc_createerr(rpc_createerr);
		goto done;
	}
#endif /* PORTMAP */

	rpc_createerr.cf_stat = clnt_st;
	clnt_geterr(client, &rpc_createerr.cf_error);
	set_rpc_createerr(rpc_createerr);

done:
	CLNT_DESTROY(client);
	if (tmp_addr)
		free(tmp_addr);
	trace3(TR_rpcb_findaddr, 1, program, version);
	return (address);
}


/*
 * Find the mapped address for program, version.
 * Calls the rpcbind service remotely to do the lookup.
 * Uses the transport specified in nconf.
 * Returns FALSE (0) if no map exists, else returns 1.
 *
 * Assuming that the address is all properly allocated
 */
int
rpcb_getaddr(program, version, nconf, address, host)
	u_long program;
	u_long version;
	struct netconfig *nconf;
	struct netbuf *address;
	char *host;
{
	struct netbuf *na;
	rpc_createerr_t rpc_createerr = { 0 };

	trace3(TR_rpcb_getaddr, 0, program, version);
	if ((na = _rpcb_findaddr(program, version, nconf, host)) == NULL)
		return (FALSE);

	if (na->len > address->maxlen) {
		/* Too long address */
		netdir_free((char *)na, ND_ADDR);
		rpc_createerr.cf_stat = RPC_FAILED;
		set_rpc_createerr(rpc_createerr);
		trace3(TR_rpcb_getaddr, 1, program, version);
		return (FALSE);
	}
	memcpy(address->buf, na->buf, (int)na->len);
	address->len = na->len;
	netdir_free((char *)na, ND_ADDR);
	trace3(TR_rpcb_getaddr, 1, program, version);
	return (TRUE);
}

/*
 * Get a copy of the current maps.
 * Calls the rpcbind service remotely to get the maps.
 *
 * It returns only a list of the services
 * It returns NULL on failure.
 */
rpcblist *
rpcb_getmaps(nconf, host)
	struct netconfig *nconf;
	char *host;
{
	rpcblist_ptr head = (rpcblist_ptr)NULL;
	register CLIENT *client;
	enum clnt_stat clnt_st;
	long vers = 0;
	rpc_createerr_t	rpc_createerr = { 0 };

	trace1(TR_rpcb_getmaps, 0);
	client = getclnthandle(host, nconf, (char **)NULL);
	if (client == (CLIENT *)NULL) {
		trace1(TR_rpcb_getmaps, 1);
		return (head);
	}
	clnt_st = CLNT_CALL(client, RPCBPROC_DUMP,
			(xdrproc_t) xdr_void, NULL,
			(xdrproc_t) xdr_rpcblist_ptr,
			(char *)&head, tottimeout);
	if (clnt_st == RPC_SUCCESS)
		goto done;

	if ((clnt_st != RPC_PROGVERSMISMATCH) &&
		    (clnt_st != RPC_PROGUNAVAIL)) {
		rpc_createerr.cf_stat = RPC_RPCBFAILURE;
		clnt_geterr(client, &rpc_createerr.cf_error);
		set_rpc_createerr(rpc_createerr);
		goto done;
	}

	/* fall back to earlier version */
	CLNT_CONTROL(client, CLGET_VERS, (char *)&vers);
	if (vers == RPCBVERS4) {
		vers = RPCBVERS;
		CLNT_CONTROL(client, CLSET_VERS, (char *)&vers);
		if (CLNT_CALL(client, RPCBPROC_DUMP,
			(xdrproc_t) xdr_void,
			(char *) NULL, (xdrproc_t) xdr_rpcblist_ptr,
			(char *)&head, tottimeout) == RPC_SUCCESS)
				goto done;
	}
	rpc_createerr.cf_stat = RPC_RPCBFAILURE;
	clnt_geterr(client, &rpc_createerr.cf_error);
	set_rpc_createerr(rpc_createerr);

done:
	CLNT_DESTROY(client);
	trace1(TR_rpcb_getmaps, 1);
	return (head);
}

/*
 * rpcbinder remote-call-service interface.
 * This routine is used to call the rpcbind remote call service
 * which will look up a service program in the address maps, and then
 * remotely call that routine with the given parameters. This allows
 * programs to do a lookup and call in one step.
*/
enum clnt_stat
rpcb_rmtcall(nconf, host, prog, vers, proc, xdrargs, argsp,
		xdrres, resp, tout, addr_ptr)
	struct netconfig *nconf;	/* Netconfig structure */
	char *host;			/* Remote host name */
	u_long prog, vers, proc;	/* Remote proc identifiers */
	xdrproc_t xdrargs, xdrres;	/* XDR routines */
	caddr_t argsp, resp;		/* Argument and Result */
	struct timeval tout;		/* Timeout value for this call */
	struct netbuf *addr_ptr;	/* Preallocated netbuf address */
{
	register CLIENT *client;
	enum clnt_stat stat;
	struct r_rpcb_rmtcallargs a;
	struct r_rpcb_rmtcallres r;
	long rpcb_vers;

	trace4(TR_rpcb_rmtcall, 0, prog, vers, proc);

	client = getclnthandle(host, nconf, (char **)NULL);
	if (client == (CLIENT *)NULL) {
		trace4(TR_rpcb_rmtcall, 1, prog, vers, proc);
		return (RPC_FAILED);
	}
	CLNT_CONTROL(client, CLSET_RETRY_TIMEOUT, (char *)&rmttimeout);
	a.prog = prog;
	a.vers = vers;
	a.proc = proc;
	a.args.args_val = argsp;
	a.xdr_args = xdrargs;
	r.addr = NULL;
	r.results.results_val = resp;
	r.xdr_res = xdrres;

	for (rpcb_vers = RPCBVERS4; rpcb_vers >= RPCBVERS; rpcb_vers--) {
		CLNT_CONTROL(client, CLSET_VERS, (char *)&rpcb_vers);
		stat = CLNT_CALL(client, RPCBPROC_CALLIT,
			(xdrproc_t) xdr_rpcb_rmtcallargs, (char *)&a,
			(xdrproc_t) xdr_rpcb_rmtcallres, (char *)&r, tout);
		if ((stat == RPC_SUCCESS) && (addr_ptr != NULL)) {
			struct netbuf *na;

			na = uaddr2taddr(nconf, r.addr);
			if (! na) {
				stat = RPC_N2AXLATEFAILURE;
				addr_ptr->len = 0;
				goto error;
			}
			if (na->len > addr_ptr->maxlen) {
				/* Too long address */
				stat = RPC_FAILED; /* XXX A better error no */
				netdir_free((char *)na, ND_ADDR);
				addr_ptr->len = 0;
				goto error;
			}
			memcpy(addr_ptr->buf, na->buf, (int)na->len);
			addr_ptr->len = na->len;
			netdir_free((char *)na, ND_ADDR);
			break;
		} else if ((stat != RPC_PROGVERSMISMATCH) &&
			    (stat != RPC_PROGUNAVAIL)) {
			goto error;
		}
	}
error:
	CLNT_DESTROY(client);
	if (r.addr)
		xdr_free((xdrproc_t) xdr_wrapstring, (char *)&r.addr);
	trace4(TR_rpcb_rmtcall, 1, prog, vers, proc);
	return (stat);
}

/*
 * Gets the time on the remote host.
 * Returns 1 if succeeds else 0.
 */
bool_t
rpcb_gettime(host, timep)
	char *host;
	time_t *timep;
{
	CLIENT *client = NULL;
	void *handle;
	struct netconfig *nconf;
	long vers;
	enum clnt_stat st;
	rpc_createerr_t rpc_createerr = { 0 };

	trace1(TR_rpcb_gettime, 0);

	if ((host == NULL) || (host[0] == NULL)) {
		time(timep);
		trace1(TR_rpcb_gettime, 1);
		return (TRUE);
	}

	if ((handle = _rpc_setconf("netpath")) == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		set_rpc_createerr(rpc_createerr);
		trace1(TR_rpcb_gettime, 1);
		return (FALSE);
	}
	rpc_createerr.cf_stat = RPC_SUCCESS;
	while (client == (CLIENT *)NULL) {
		if ((nconf = _rpc_getconf(handle)) == NULL) {
			if (rpc_createerr.cf_stat == RPC_SUCCESS)
				rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
			break;
		}
		client = getclnthandle(host, nconf, (char **)NULL);
		if (client)
			break;
	}
	set_rpc_createerr(rpc_createerr);
	_rpc_endconf(handle);
	if (client == (CLIENT *) NULL) {
		trace1(TR_rpcb_gettime, 1);
		return (FALSE);
	}

	st = CLNT_CALL(client, RPCBPROC_GETTIME,
		(xdrproc_t) xdr_void, (char *)NULL,
		(xdrproc_t) xdr_int, (char *)timep, tottimeout);

	if ((st == RPC_PROGVERSMISMATCH) || (st == RPC_PROGUNAVAIL)) {
		CLNT_CONTROL(client, CLGET_VERS, (char *)&vers);
		if (vers == RPCBVERS4) {
			/* fall back to earlier version */
			vers = RPCBVERS;
			CLNT_CONTROL(client, CLSET_VERS, (char *)&vers);
			st = CLNT_CALL(client, RPCBPROC_GETTIME,
				(xdrproc_t) xdr_void, (char *)NULL,
				(xdrproc_t) xdr_int, (char *) timep,
				tottimeout);
		}
	}
	trace1(TR_rpcb_gettime, 1);
	CLNT_DESTROY(client);
	return (st == RPC_SUCCESS? TRUE: FALSE);
}

/*
 * Converts taddr to universal address.  This routine should never
 * really be called because local n2a libraries are always provided.
 */
char *
rpcb_taddr2uaddr(nconf, taddr)
	struct netconfig *nconf;
	struct netbuf *taddr;
{
	CLIENT *client;
	char *uaddr = NULL;
	rpc_createerr_t rpc_createerr = { 0 };

	trace1(TR_rpcb_taddr2uaddr, 0);

	/* parameter checking */
	if (nconf == (struct netconfig *)NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		set_rpc_createerr(rpc_createerr);
		trace1(TR_rpcb_taddr2uaddr, 1);
		return (NULL);
	}
	if (taddr == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNADDR;
		set_rpc_createerr(rpc_createerr);
		trace1(TR_rpcb_taddr2uaddr, 1);
		return (NULL);
	}
	client = local_rpcb();
	if (! client) {
		trace1(TR_rpcb_taddr2uaddr, 1);
		return (NULL);
	}

	CLNT_CALL(client, RPCBPROC_TADDR2UADDR, (xdrproc_t) xdr_netbuf,
		(char *)taddr, (xdrproc_t) xdr_wrapstring, (char *)&uaddr,
		tottimeout);
	CLNT_DESTROY(client);
	trace1(TR_rpcb_taddr2uaddr, 1);
	return (uaddr);
}

/*
 * Converts universal address to netbuf.  This routine should never
 * really be called because local n2a libraries are always provided.
 */
struct netbuf *
rpcb_uaddr2taddr(nconf, uaddr)
	struct netconfig *nconf;
	char *uaddr;
{
	CLIENT *client;
	struct netbuf *taddr;
	rpc_createerr_t rpc_createerr = { 0 };

	trace1(TR_rpcb_uaddr2taddr, 0);

	/* parameter checking */
	if (nconf == (struct netconfig *)NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
		set_rpc_createerr(rpc_createerr);
		trace1(TR_rpcb_uaddr2taddr, 1);
		return (NULL);
	}
	if (uaddr == NULL) {
		rpc_createerr.cf_stat = RPC_UNKNOWNADDR;
		set_rpc_createerr(rpc_createerr);
		trace1(TR_rpcb_uaddr2taddr, 1);
		return (NULL);
	}
	client = local_rpcb();
	if (! client) {
		trace1(TR_rpcb_uaddr2taddr, 1);
		return (NULL);
	}

	taddr = (struct netbuf *)calloc(1, sizeof (struct netbuf));
	if (taddr == NULL) {
		CLNT_DESTROY(client);
		trace1(TR_rpcb_uaddr2taddr, 1);
		return (NULL);
	}
	if (CLNT_CALL(client, RPCBPROC_UADDR2TADDR, (xdrproc_t) xdr_wrapstring,
		(char *) &uaddr, (xdrproc_t) xdr_netbuf, (char *)taddr,
		tottimeout) != RPC_SUCCESS) {
		free(taddr);
		taddr = NULL;
	}
	CLNT_DESTROY(client);
	trace1(TR_rpcb_uaddr2taddr, 1);
	return (taddr);
}
