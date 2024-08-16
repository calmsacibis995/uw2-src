/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/socket/getnetent.c	1.3"
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
#include <netdb.h>
#include <ctype.h>
#include <string.h>
#include <netinet/in.h>
#include "nis.h"
#include <ns.h>
#ifdef _REENTRANT
#include "nis_mt.h"
#endif

#define	MAXALIASES	35

static struct netdata {
	struct netent _net;
	char _line[BUFSIZ+1];
	char *_net_aliases[MAXALIASES];
	char *_domain;
	char *_ypkey;
	int   _ypkeylen;
	char _ntoabuf[16];
} netdata;

#define line        (d->_line)
#define net         (d->_net)
#define net_aliases (d->_net_aliases)
#define domain      (d->_domain)
#define ypkey       (d->_ypkey)
#define ypkeylen    (d->_ypkeylen)
#define ntoabuf     (d->_ntoabuf)

static char *any();

static struct netent *interpret();

static struct netdata *
_netdata()
{
	struct netdata *d;

#ifdef _REENTRANT
	struct _nis_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		d = &netdata;
	} else {
		/*
		 * This is the case of threads other than the first.
		 */
		key_tbl = (struct _nis_tsd *)
			_mt_get_thr_specific_storage(_nis_key,_NIS_KEYTBL_SIZE);
		if (key_tbl == NULL)
			return ((struct netdata *)NULL);
		if (key_tbl->net_info_p == NULL)
			key_tbl->net_info_p = 
				(struct netdata *)calloc(1, sizeof(struct netdata));
		d = (struct netdata *)key_tbl->net_info_p;
	}
#else
	d = &netdata;
#endif
	if (d && domain == NULL)
		domain = nis_domain();
	return(d);
}

#ifdef _REENTRANT
void
_free_nis_net_info(p)
    void *p;
{
    if (FIRST_OR_NO_THREAD)
        return;
    if (p != NULL)
        free(p);
    return;
}
#endif /* _REENTRANT */

nis_setnetent()
{
	register struct netdata *d = _netdata();

	if (d == 0)
		return(-1);

	if (ypkey)
		free(ypkey);
	ypkey = (char *)0;
	return(0);
}

nis_endnetent()
{
	register struct netdata *d = _netdata();

	if (d == 0)
		return(-1);

	if (ypkey){
		free(ypkey);
		ypkey = (char *)0;
	}
	return(0);
}

struct netent *
nis_getnetent()
{
	register struct netdata *d = _netdata();
	register char *cp, **q;
	register struct netent *np;
	char *p;
	char *val = NULL;
	int   vallen, res;
	char *map = "networks.byaddr";

	if (d == 0)
		return(NULL);

	if (ypkey == NULL)
		res = yp_first(domain, map, &ypkey, &ypkeylen, &val, &vallen);
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
	np = interpret(val, vallen);
	free(val);
	return(np);
}

struct netent *
nis_getnetbyname(name)
	register char *name;
{
	register struct netdata *d = _netdata();
	register struct netent *np;
	register char **cp;
	int   vallen, res;
	char *val = NULL;
	char *map = "networks.byname";

	if (d == NULL)
		return(NULL);
	
	res = yp_match(domain, map, name, strlen(name), &val, &vallen);
	if (res) {
		set_niserror(res);
		yp_retcode(res);
		return(NULL);
	}
	set_nsaction(NS_SUCCESS);
	np = interpret(val, vallen);
	free(val);
	return(np);
}

static char *
inet_networktoa(naddr)
	int naddr;
{
	register struct netdata *d = _netdata();
	struct in_addr in, inet_makeaddr();
	char *ptr;
	int addr;

	in = inet_makeaddr(naddr, INADDR_ANY);
	addr = in.s_addr;
	strcpy(ntoabuf, (char *)inet_ntoa(in));

	if ((IN_CLASSA_HOST & htonl(addr))==0) {
		if ((ptr = strchr(ntoabuf, '.')) == NULL)
			return(NULL);
		*ptr = '\0';
	} else if ((IN_CLASSB_HOST & htonl(addr))==0) {
		if ((ptr = strchr(ntoabuf, '.')) == NULL)
			return(NULL);
		if ((ptr = strchr(ptr+1, '.')) == NULL)
			return(NULL);
		*ptr = '\0';
	} else if ((IN_CLASSC_HOST & htonl(addr))==0) {
		if ((ptr = strrchr(ntoabuf, '.')) == NULL)
			return(NULL);
		*ptr = '\0';
	}
    return(ntoabuf);
}

struct netent *
nis_getnetbyaddr(anet, type)
	int anet, type;
{
	register struct netdata *d = _netdata();
	register struct netent *np;
	int   vallen, res;
	char *val = NULL;
	char *map = "networks.byaddr";
	char *addr;
	
	if (d == NULL)
		return(NULL);

	addr = inet_networktoa(anet);
	res = yp_match(domain, map, addr, strlen(addr), &val, &vallen);
	if (res) {
		set_niserror(res);
		yp_retcode(res);
		return(NULL);
	}
	set_nsaction(NS_SUCCESS);
	np = interpret(val, vallen);
	free(val);
	return(np);
}


static struct netent *
interpret(val, len)
char *val;
int len;
{
	register struct netdata *d = _netdata();
	char *p;
	register char *cp, **q;

	if (d == NULL)
		return(NULL);

	(void)strncpy(line, val, len);
	p = line;
	line[len] = '\n';

	if ((cp = strpbrk(p, "#\n")) == NULL)
		return(NULL);

	*cp = '\0';
	net.n_name = p;
	
	if ((cp = strpbrk(p, " \t")) == NULL)
		return(NULL);

	*cp++ = '\0';
	while(*cp == ' ' || *cp == '\t')
		cp++;

	net.n_net = inet_network(cp);
	net.n_addrtype = AF_INET;

	q = net.n_aliases = net_aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &net_aliases[MAXALIASES - 1])
				*q++ = cp;
			if ((cp = strpbrk(cp, " \t")) != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return(&net);
}

