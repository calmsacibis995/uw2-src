/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/socket/inet_sethost.c	1.3"
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
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 * 	(c) 1990,1991  UNIX System Laboratories, Inc.
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
#include <netinet/in.h>
#include "nis.h"
#include <ns.h>
#ifdef _REENTRANT
#include "nis_mt.h"
#endif


/*
 * Internet version.
 */
#define	MAXALIASES	 35
#define	MAXADDRS	 14
#define HOSTADDRSIZE sizeof(u_long)

static struct hostdata {
	char _line[BUFSIZ+1];
	char _hostaddr[MAXADDRS][HOSTADDRSIZE];
	char *_host_addrs[MAXADDRS+1];
	struct hostent _host;
	char *_host_aliases[MAXALIASES];
	char *_domain;
	char *_ypkey;
	int  _ypkeylen;
} hostdata;

#define hostf        (d->_hostf)
#define line         (d->_line)
#define hostaddr     (d->_hostaddr)
#define host         (d->_host)
#define host_aliases (d->_host_aliases)
#define host_addrs   (d->_host_addrs)
#define domain       (d->_domain)
#define ypkey        (d->_ypkey)
#define ypkeylen     (d->_ypkeylen)
#define usenis       (d->_usenis)

struct hostent *interpret();

static struct hostdata *
_hostdata()
{
	struct hostdata *d;

#ifdef _REENTRANT
	struct _nis_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		d = &hostdata;
	} else {
		/*
		 * This is the case of threads other than the first.
		 */
		key_tbl = (struct _nis_tsd *)
			_mt_get_thr_specific_storage(_nis_key,_NIS_KEYTBL_SIZE);
		if (key_tbl == NULL)
			return ((struct hostdata *)NULL);
		if (key_tbl->host_info_p == NULL)
			key_tbl->host_info_p = 
				(struct hostdata *)calloc(1, sizeof(struct hostdata));
		d = (struct hostdata *)key_tbl->host_info_p;
	}
#else
	d = &hostdata;
#endif
	if (d && domain == NULL)
		domain = nis_domain();
	return(d);
}

#ifdef _REENTRANT
void
_free_nis_host_info(p)
    void *p;
{
    if (FIRST_OR_NO_THREAD)
        return;
    if (p != NULL)
        free(p);
    return;
}
#endif /* _REENTRANT */

nis_sethostent()
{
	register struct hostdata *d = _hostdata();

	if (d == NULL)
		return(-1);

	if (ypkey)
		free(ypkey);
	ypkey = (char *)0;
	return(0);
}

nis_endhostent()
{
	register struct hostdata *d = _hostdata();

	if (d == NULL)
		return(-1);

	if (ypkey) {
		free(ypkey);
		ypkey = (char *)0;
	}
	return(0);
}

struct hostent *
nis_gethostent()
{
	register struct hostdata *d = _hostdata();
	struct hostent *hp;
	char *map = "hosts.byaddr";
	char *val;
	int res, vallen;

	if (d == NULL)
		return(NULL);

	if (ypkey == (char *)0)
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
	hp = interpret(val, vallen);
	free(val);
	return (hp);
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
static struct hostent *
interpret(val, len)
char    *val;
int len;
{
	register struct hostdata *d = _hostdata();
	register char *p, *cp, **q;
	char *z;
	int i;
	u_long theaddr;

	if (d == NULL)
		return(NULL);

	(void)strncpy(line, val, len);
	p = line;
	line[len] = '\n';
	line[len+1] = '\0';

	cp = any(p, "#\n");
	z = cp;
	if (*z == '#')
		z = any(cp, '\n');

	*cp = '\0';
	cp = any(p, " \t");
	*cp++ = '\0';

	/* THIS STUFF IS INTERNET SPECIFIC */
	host_addrs[0] = hostaddr[0];
	host_addrs[1] = NULL;
	host.h_addr_list = host_addrs;
	*((u_long *)hostaddr[0]) = inet_addr(p);
	host.h_length = HOSTADDRSIZE;
	host.h_addrtype = AF_INET;

	while (*cp == ' ' || *cp == '\t')
		cp++;
	host.h_name = cp;
	q = host.h_aliases = host_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &host_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&host);
}
