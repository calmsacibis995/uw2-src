/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/socket/getservent.c	1.3"
#ident  "$Header: $"

/*
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
 * 	          All rights reserved.
 *  
 */

/*	Copyright (c) 1992 Intel Corp.		*/
/*	  All Rights Reserved  	*/
/*
 *	INTEL CORPORATION PROPRIETARY INFORMATION
 *
 *	This software is supplied under the terms of a license
 *	agreement or nondisclosure agreement with Intel Corpo-
 *	ration and may not be copied or disclosed except in
 *	accordance with the terms of that agreement.
 */



#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/byteorder.h>
#include <netdb.h>
#include <ctype.h>
#include "nis.h"
#include <ns.h>
#ifdef _REENTRANT
#include "nis_mt.h"
#endif

#define	MAXALIASES	35
static struct cbdata_t {
	struct servent *cbserv;
	char *proto;
	char *name;
	int  port;
	int fail;
};

static struct servdata {
	char _line[BUFSIZ+1];
	struct servent _serv;
	char *_serv_aliases[MAXALIASES];
	char *_domain;
	char *_ypkey;
	struct cbdata_t _cbdata;
	int  _ypkeylen;
} servdata;

#define line         (d->_line)
#define serv         (d->_serv)
#define serv_aliases (d->_serv_aliases)
#define domain       (d->_domain)
#define ypkey        (d->_ypkey)
#define ypkeylen     (d->_ypkeylen)
#define cbdata       (d->_cbdata)

static char *map = "services.byname";
static struct servent *interpret();

static struct servdata *
_servdata()
{
	struct servdata *d;

#ifdef _REENTRANT
	struct _nis_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		d = &servdata;
	} else {
		/*
		 * This is the case of threads other than the first.
		 */
		key_tbl = (struct _nis_tsd *)
			_mt_get_thr_specific_storage(_nis_key,_NIS_KEYTBL_SIZE);
		if (key_tbl == NULL)
			return ((struct servdata *)NULL);
		if (key_tbl->serv_info_p == NULL)
			key_tbl->serv_info_p = 
				(struct servdata *)calloc(1, sizeof(struct servdata));
		d = (struct servdata *)key_tbl->serv_info_p;
	}
#else
	d = &servdata;
#endif
	if (d && domain == NULL)
		domain = nis_domain();
	return(d);
}
 
#ifdef _REENTRANT
void
_free_nis_serv_info(p)
    void *p;
{
    if (FIRST_OR_NO_THREAD)
        return;
    if (p != NULL)
        free(p);
    return;
}
#endif /* _REENTRANT */
 
nis_setservent()
{
	register struct servdata *d = _servdata();

	if (d == 0)
		return;

	if (ypkey)
		free(ypkey);
	ypkey = (char *)0;
}

nis_endservent()
{
	register struct servdata *d = _servdata();

	if (d == 0)
		return;

	if (ypkey)
		free(ypkey);
	ypkey = (char *)0;
}

struct servent *
nis_getservent()
{
	register struct servdata *d = _servdata();
	struct servent *sp;
	char *val;
	int res, vallen;

	if (d == 0)
		return;


	if (ypkey == (char *)0)
		res = yp_first(domain, map, &ypkey, &ypkeylen, 
				&val, &vallen);
	else
		res = yp_next(domain, map, ypkey, ypkeylen, 
				&ypkey, &ypkeylen, &val, &vallen);
	if (res) {
		set_niserror(res);
		if (res == YPERR_NOMORE)
			set_nsaction(NS_SUCCESS);
		else
			yp_retcode(res);
		return(NULL);
	}
	sp = interpret(val, vallen);
	free(val);
	return (sp);
}
static char *
any(cp, match)
	register char *cp;
	char *match;
{
	register char *mp, c;

	while (c = *cp) {
		for (mp = match; *mp; mp++)
			if (*mp == c)
				return (cp);
		cp++;
	}
	return ((char *)0);
}
static struct servent *
interpret(val, len)
char *val;
int len;
{
	register struct servdata *d = _servdata();
	register char *cp, **q;
	char *p;

	(void)strncpy(line, val, len);
	p = line;
	line[len] = '\n';

	cp = any(p, "#\n");
	if (cp == NULL)
		return(NULL);

	*cp = '\0';
	serv.s_name = p;
	p = any(p, " \t");
	if (p == NULL)
		return(NULL);

	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	cp = any(p, ",/");
	if (cp == NULL)
		return(NULL);

	*cp++ = '\0';
	serv.s_port = htons((u_short)atoi(p));
	serv.s_proto = cp;
	q = serv.s_aliases = serv_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &serv_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&serv);

}
static int
callback(status, key, keylen, val, vallen, cb)
	int status;
	char *key;
	int  keylen;
	char *val;
	int  vallen;
	struct cbdata_t *cb;
{
	register struct servent *p;
	register char **cp;
	int err;

	if (status == YP_TRUE) {
		/*
		 * Interpret val, see if we have a match.
		 * If so return TRUE otherwise return FALSE which
		 * means get another key/val pair
		 */
		if ((p = interpret(val, vallen)) == NULL){
			return(FALSE);
		}
		/*
		 * If name is set, we are looking of a server name
		 */
		if (cb->name) {
			/*
			 * If we are looking for a particular protocol
			 * see if this is the right one.
			 */
			if (cb->proto && strcmp(cb->proto, p->s_proto))
				return(FALSE);

			/*
			 * check service name 
			 */
			if (strcmp(cb->name, p->s_name) == 0) {
				cb->cbserv = p;
				return(TRUE);
			}
			/*
			 * Now check aliases
			 */
			for (cp = p->s_aliases; *cp; cp++){
				if (strcmp(cb->name, *cp) == 0) {
					cb->cbserv = p;
					return(TRUE);
				}
			}
		} else {
			if (p->s_port == cb->port) { /* looking for a port */
				cb->cbserv = p;
				return(TRUE);
			}
		}
		return(FALSE);
	}
	if (ypprot_err(status) != YPERR_NOMORE)
		cb->fail = ypprot_err(status);

	return(TRUE);
}

struct servent *
nis_getservbyport(port, proto)
	int port;
	char *proto;
{
	register struct servdata *d = _servdata();
	struct servent *p;
	int res;
	char portstr[12];
	char *key, *val;
	int keylen, vallen;
	struct ypall_callback cbinfo;

	if (d == 0)
		return(NULL);

	if (proto == NULL){

		cbinfo.foreach = callback;
		cbinfo.data = (char *)&cbdata;
		cbdata.name = NULL;
		cbdata.port = port;
		cbdata.proto = proto;
		cbdata.fail = 0;
		cbdata.cbserv = (struct servent *)NULL;
		res = yp_all(domain, map, &cbinfo);
		if (res == 0 && cbdata.fail == 0){
			set_nsaction(NS_SUCCESS);
			return(cbdata.cbserv);
		}
		if (res) {
			set_niserror(res);
			yp_retcode(res);
		} else {
			set_niserror(cbdata.fail);
			yp_retcode(cbdata.fail);
		}
		return(NULL);
	}

	sprintf(portstr, "%d", ntohs(port));
	keylen = strlen(portstr) + 1 + strlen(proto);
	if ((key = (char *)malloc(keylen + 1)) == NULL){
		set_nsaction(NS_UNAVAIL);
		return(NULL);
	}
	sprintf(key, "%s/%s", portstr, proto);

	res = yp_match(domain, map, key, keylen, &val, &vallen);
	if (res){
		set_niserror(res);
		yp_retcode(res);
		free(key);
		return(NULL);
	}
	set_nsaction(NS_SUCCESS);
	p = interpret(val, vallen);
	free(val);
	free(key);
	return(p);
}

struct servent *
nis_getservbyname(name, proto)
	char *name, *proto;
{
	register struct servdata *d = _servdata();
	register struct servent *p;
	struct ypall_callback cbinfo;
	int res;

	if (d == 0)
		return(NULL);

	cbinfo.foreach = callback;
	cbinfo.data = (char *)&cbdata;
	cbdata.name = name;
	cbdata.proto = proto;
	cbdata.fail = 0;
	cbdata.cbserv = (struct servent *)NULL;

	res = yp_all(domain, map, &cbinfo);
	if (res == 0 && cbdata.fail == 0){
		set_nsaction(NS_SUCCESS);
		return(cbdata.cbserv);
	}

	if (res) {
		set_niserror(res);
		yp_retcode(res);
	} else {
		set_niserror(cbdata.fail);
		yp_retcode(cbdata.fail);
	}

	return(NULL);
}
