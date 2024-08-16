/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip_nis/tcpip_nis.c	1.2.1.3"
#ident  "$Header: $"
#if !defined(lint) && defined(SCCSIDS)
static  char sccsid[] = "@(#)tcpip_nis.c	1.9 90/04/06 TIRPC 1.0";
#endif

/*
 * Copyright (c) 1989, 1990 by Sun Microsystems, Inc.
 */

#include <stdio.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <string.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#ifdef _REENTRANT
#include "tcpip_nis_mt.h"
#else 
char _ntoa_ypbind_lock;
#define MUTEX_LOCK(lockp)       (0)
#define MUTEX_UNLOCK(lockp)     (0)
#endif

/*
 * NIS based gethostent routines ...
 *
 * NOTE: Only four routines in this file are not static:
 *	_nis_gethostbyname(),_nis_gethostbyaddr(),_nis_getservbyname(),
 *	_nis_getservbyport()
 *
 *  These routines are called from the transport specific routines in
 *  tcpip.c, that must exist in the parent directory i.e. /usr/src/
 *  lib/nametoaddr/tcpip.
 */

#define HOSTMAP_BYNAME "hosts.byname"
#define HOSTMAP_BYADDR "hosts.byaddr"
#define SERVMAP_BYNAME "services.byname"    /* XXX  map is actually byport */

#define MAXTOKENS   64	    /* how many aliases can there be, anyway? */

static void parse_host_match();
static void parse_serv_match();
extern int ypbindisup;

static struct HostData {
		long hostaddr;
		long *hostaddrp[2];
		struct hostent	host;
		char *local_domain;
		char *match_tmp;
		int  hrecurs;
		int  arecurs;
} *hostdata, *_gethostdata();

static struct HostData *
_gethostdata()
{

	struct HostData *d;

#ifdef _REENTRANT
	struct _ntoa_nis_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {

		if (hostdata)
			return (hostdata);

		d = (struct HostData *)calloc(1, sizeof(struct HostData));
		if (d == NULL)
			return(NULL);
		hostdata = d;
		d->host.h_aliases = 
					(char **)malloc(MAXTOKENS*sizeof (char *));
		if (d->host.h_aliases == NULL) {
			free(d);
			hostdata = NULL;
			return (NULL);
		}
	} else {
		key_tbl = (struct _ntoa_nis_tsd *)
			_mt_get_thr_specific_storage(_ntoa_nis_key,
				_NTOA_NIS_KEYTBL_SIZE);
		if (key_tbl == NULL)
			return(NULL);

		if (key_tbl->hostdata_p)
			return(key_tbl->hostdata_p);

		d = (struct HostData *)calloc(1, sizeof(struct HostData));
		if (d == NULL)
			return(NULL);
		d->host.h_aliases = 
					(char **)malloc(MAXTOKENS*sizeof (char *));
		if (d->host.h_aliases == NULL) {
			free(d);
			return (NULL);
		}
		key_tbl->hostdata_p = d;
	}
#else
	if (hostdata)
		return (hostdata);

	d = (struct HostData *)calloc(1, sizeof(struct HostData));
	if (d == NULL)
		return(NULL);
	hostdata = d;
	d->host.h_aliases = (char **)malloc(MAXTOKENS*sizeof (char *));
	if (d->host.h_aliases == NULL) {
		free(d);
		hostdata = NULL;
	    return (NULL);
	}
#endif


	d->hostaddr = NULL;
	d->hostaddrp[0] = &(d->hostaddr);
	d->hostaddrp[1] = NULL;
	d->host.h_addr_list = (char **)(d->hostaddrp);
	d->match_tmp = NULL;
	
	return (d);
}
#ifdef _REENTRANT
void 
_free_ntoa_nis_hostdata(p)
	void *p;
{
	struct HostData *d = (struct HostData *)p;

	if (FIRST_OR_NO_THREAD)
		return;
	if (d != NULL){
		free(d->host.h_aliases);
		free(d);
	}
	return;
}
#endif

static
sethostent()
{
	struct HostData *hd = _gethostdata();

	if (hd == NULL)
	    return (-1);
	if (yp_get_default_domain(&hd->local_domain) != 0)
	    return (-1);
	if (hd->match_tmp != NULL) { /* free previous result */
            free(hd->match_tmp);
	    hd->match_tmp = NULL;
	}
	return (0);
}

struct hostent *
_nis_gethostbyname(name)
	char	*name;
{
	struct HostData *hd = _gethostdata();
	int match_len, base, err;
	char bname[BUFSIZ], *sp;

	if (sethostent() == -1)
	    return (NULL);

	/*
	 * Check to make sure ypbind is up and bound 
	 */
	if (!ypbindisup && _nis_check_ypbind(hd->local_domain)) {
		return(NULL);
	}

	if (hd->hrecurs++) {
		return(NULL);
	}

	if ((err = yp_match(hd->local_domain, HOSTMAP_BYNAME, name,
		strlen(name), &hd->match_tmp, &match_len)) != 0) {
		hd->hrecurs = 0;
		/*
		 * re-set ypbindisup to FALSE if failure comes from
		 * ypbind or rpc
		 */
		if (err == YPERR_RPC || err == YPERR_YPBIND){
			MUTEX_LOCK(&_ntoa_ypbind_lock);
			ypbindisup = FALSE;
			MUTEX_UNLOCK(&_ntoa_ypbind_lock);
		}
	    return (NULL);
	}
found:
	parse_host_match(hd->match_tmp, &hd->host);
	hd->hrecurs = 0;
	return (&hd->host);
}

struct hostent *
_nis_gethostbyaddr(addr, len, type)
	char	*addr;
	int	len, type;
{
	struct HostData *hd;
	int match_len, err;
	char *addr_str;

	/* Don't know about anything but inet yet */
	if ((len != 4) || (type != AF_INET))
		return NULL;

	hd = _gethostdata();

	if (sethostent() == -1)
	    return (NULL);

	/*
	 * Check to make sure ypbind is up and bound 
	 */
	if (!ypbindisup && _nis_check_ypbind(hd->local_domain)) {
		return(NULL);
	}

	if (hd->arecurs++)
		return(NULL);

	addr_str = (char *)inet_ntoa(*(struct in_addr *)addr);
	if ((err = yp_match(hd->local_domain, HOSTMAP_BYADDR, addr_str,
		strlen(addr_str), &hd->match_tmp, &match_len)) != 0) {
	    hd->arecurs = 0;
		/*
		 * re-set ypbindisup to FALSE if failure comes from
		 * ypbind or rpc.
		 */
		if (err == YPERR_RPC || err == YPERR_YPBIND){
			MUTEX_LOCK(&_ntoa_ypbind_lock);
			ypbindisup = FALSE;
			MUTEX_UNLOCK(&_ntoa_ypbind_lock);
		}
	    return (NULL);
	}
	hd->arecurs = 0;
	parse_host_match(hd->match_tmp, &hd->host);
	return (&hd->host);
}

static void
parse_host_match(field, he)
    char *field;
    struct hostent *he;
{
    char *bp;
    char **ap;

    bp = strtok(field, " \t");	    /* IP address */
    *(unsigned long *)he->h_addr_list[0] = inet_addr(bp);
    he->h_length = 4;
    he->h_name = strtok(NULL, " \t\n");	/* XXX check for null? */
    ap = he->h_aliases;
    do {
	bp = strtok(NULL, " \t\n");
	if (bp != NULL) {
	    if (*bp == '#') {
		bp = NULL;	    /* comment character; we're done */
	    }
	}
	*ap++ = bp;
    } while (bp != NULL);
}


static struct ServData {
		CLIENT *cl;
		struct servent	serv;
		char *local_domain;
		char *key_tmp;
		char *match_tmp;
		int  nrecurs;
		int  precurs;
		char token_tmp[YPMAXRECORD];
}*servdata, *_getservdata();

static struct ServData *
_getservdata()
{

	struct ServData *d;

#ifdef _REENTRANT
	struct _ntoa_nis_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {

		if (servdata)
			return (servdata);

		d = (struct ServData *)calloc(1, sizeof(struct ServData));
		if (d == NULL)
			return (NULL);
		servdata = d;
		d->serv.s_aliases = 
			(char **)malloc(MAXTOKENS*sizeof (char *));
		if (d->serv.s_aliases == NULL) {
			free(d);
			servdata = NULL;
			return (NULL);
		}
	} else {
		key_tbl = (struct _ntoa_nis_tsd *)
			_mt_get_thr_specific_storage(_ntoa_nis_key,
				_NTOA_NIS_KEYTBL_SIZE);
		if (key_tbl == NULL)
			return(NULL);

		if (key_tbl->servdata_p)
			return(key_tbl->servdata_p);

		d = (struct ServData *)calloc(1, sizeof(struct ServData));
		if (d == NULL)
			return(NULL);
		d->serv.s_aliases = 
			(char **)malloc(MAXTOKENS*sizeof (char *));
		if (d->serv.s_aliases == NULL) {
			free(d);
			return (NULL);
		}
		key_tbl->servdata_p = d;
	}
#else
	if (servdata)
		return (servdata);

	d = (struct ServData *)calloc(1, sizeof(struct ServData));
	if (d == NULL)
		return(NULL);
	servdata = d;
	d->serv.s_aliases = (char **)malloc(MAXTOKENS*sizeof (char *));
	if (d->serv.s_aliases == NULL) {
		free(d);
		servdata = NULL;
	    return (NULL);
	}
#endif
	d->match_tmp = d->key_tmp = NULL;
	return (d);
}
#ifdef _REENTRANT
void 
_free_ntoa_nis_servdata(p)
	void *p;
{
	struct ServData *d = (struct ServData *)p;

	if (FIRST_OR_NO_THREAD)
		return;
	if (d != NULL){
		free(d->serv.s_aliases);
		free(d);
	}
	return;
}
#endif

static setservent()
{
	struct ServData *sd = _getservdata();

	if (sd == NULL)
	    return (-1);
	if (yp_get_default_domain(&sd->local_domain) != 0)
	    return (-1);
        if (sd->match_tmp != NULL) {	/* free previous match data */
	    free(sd->match_tmp);
            sd->match_tmp = NULL;
	}
        if (sd->key_tmp != NULL) {	/* free previous key data */
	    free(sd->key_tmp);
	    sd->key_tmp = NULL;
	}
	return  (0);
}

static struct cbdata {
	struct servent *cbserv;
	char *proto;
	char *name;
	int  port;
	int fail;
}cbdata;

static int
callback(status, key, keylen, val, vallen, cbdata)
	int status;
	char *key;
	int  keylen;
	char *val;
	int  vallen;
	struct cbdata *cbdata;
{
	struct ServData *sd = _getservdata();
	register struct servent *se;
	register char *bp;
	int err;

	se = &sd->serv;

	if (status == YP_TRUE) {
	    strcpy(sd->token_tmp, val);
		/*
		 * We are looking for server name
		 */
		if (cbdata->name) {
			bp = strtok(val, " \t");	/* official name */
			bp = strtok(NULL, "/");	    /* eat the port field */
			bp = strtok(NULL, " \t\n");	    /* check protocol */
			if (bp == NULL)
				return(FALSE);
			/*
			 * If we are looking for a particular protocol
			 * see if this is the right one.
			 */
			if (cbdata->proto && strcmp(bp, cbdata->proto))
				return(FALSE);

			if (strcmp(cbdata->name, val) == 0) {
				parse_serv_match(sd->token_tmp, se);
				cbdata->cbserv = se;
				return(TRUE);
			}
			/* 
			 * go through the aliases, if any 
			 */
			while ((bp = strtok(NULL, " \t\n")) != NULL) {
				if (*bp == '#') {
					break; /* comment character; we're done */
				}
				if (strcmp(cbdata->name, bp) == 0) {
					parse_serv_match(sd->token_tmp, se);
					cbdata->cbserv = se;
					return(TRUE);
				}
			}
		} else { /* looking for a port */
			bp = strtok(val, " \t"); /* official name */
			if (bp = strtok(NULL, "/")) { /* port number */
				if (atoi(bp) == cbdata->port) {
					parse_serv_match(sd->token_tmp, se);
					cbdata->cbserv = se;
					return (TRUE);
				}
			}
		}
		return(FALSE);
	}
	if (ypprot_err(status) != YPERR_NOMORE)
		cbdata->fail = ypprot_err(status);

	return(TRUE);
}
struct servent *
_nis_getservbyname(name, proto)
	char *name, *proto;
{
	struct ServData *sd = _getservdata();
	struct ypall_callback cbinfo;
	int res;

	if (setservent() == -1)
	    return (NULL);

	/*
	 * Check to make sure ypbind is up and bound 
	 */
	if (!ypbindisup && _nis_check_ypbind(sd->local_domain)) {
		return(NULL);
	}

	if (sd->nrecurs++)
		return(NULL);

	cbinfo.foreach = callback;
	cbinfo.data = (char *)&cbdata;
	cbdata.name = name;
	cbdata.port = -1;
	cbdata.proto = proto;
	cbdata.fail = 0;
	cbdata.cbserv = (struct servent *)NULL;

	res = yp_all(sd->local_domain, SERVMAP_BYNAME, &cbinfo);
	if (res == 0 && cbdata.fail == 0){
		sd->nrecurs = 0;
		return(cbdata.cbserv);
	}
	/*
	 * re-set ypbindisup to FALSE if failure comes from
	 * ypbind or rpc
	 */
	if (res == YPERR_RPC || res == YPERR_YPBIND){
		MUTEX_LOCK(&_ntoa_ypbind_lock);
		ypbindisup = FALSE;
		MUTEX_UNLOCK(&_ntoa_ypbind_lock);
	}

	sd->nrecurs = 0;
	return(NULL);
}

struct servent *
_nis_getservbyport(port, proto)
	int port;
	char *proto;
{
	struct ServData *sd = _getservdata();
	register struct servent *se;
	char servport[64];
	int keylen, matchlen, err;

	if (setservent() == -1)
	    return (NULL);

	/*
	 * Check to make sure ypbind is up and bound 
	 */
	if (!ypbindisup && _nis_check_ypbind(sd->local_domain)) {
		return(NULL);
	}

	if (sd->precurs++)
		return(NULL);

	err = 0;
	if (proto != NULL) {
		se = &sd->serv;
	    sprintf(servport, "%d/%s", ntohs(port), proto);
	    if ((err = yp_match(sd->local_domain, SERVMAP_BYNAME, servport,
		    strlen(servport), &sd->match_tmp, &matchlen)) == 0) {
			parse_serv_match(sd->match_tmp, se);
			sd->precurs = 0;
			return (se);
	    }
	} else {
		struct ypall_callback cbinfo;

		cbinfo.foreach = callback;
		cbinfo.data = (char *)&cbdata;
		cbdata.name = NULL;
		cbdata.port = port;
		cbdata.proto = proto;
		cbdata.fail = 0;
		cbdata.cbserv = (struct servent *)NULL;

		err = yp_all(sd->local_domain, SERVMAP_BYNAME, &cbinfo);

		if (err == 0 && cbdata.fail == 0){
			sd->precurs = 0;
		    return (cbdata.cbserv);
		}
	}
	/*
	 * re-set ypbindisup to FALSE if failure comes from
	 * ypbind or rpc
	 */
	if (err == YPERR_RPC || err == YPERR_YPBIND){
		MUTEX_LOCK(&_ntoa_ypbind_lock);
		ypbindisup = FALSE;
		MUTEX_UNLOCK(&_ntoa_ypbind_lock);
	}
	sd->precurs = 0;
	return (NULL);
}

static void
parse_serv_match(field, se)
char *field;
struct servent *se;
{
    char *bp;
    char **ap;

    se->s_name = strtok(field, " \t");
    bp = strtok(NULL, "/");
    se->s_port = htons((u_short)atoi(bp));
    se->s_proto = strtok(NULL, " \t\n");
    ap = se->s_aliases;
    do {
	bp = strtok(NULL, " \t\n");
	if (bp != NULL) {
	    if (*bp == '#') {
		bp = NULL;	    /* comment character; we're done */
	    }
	}
	*ap++ = bp;
    } while (bp != NULL);
}
