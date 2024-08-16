/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/ns/nis/socket/getprotoent.c	1.2"
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
#include "nis.h"
#include <ns.h>
#ifdef _REENTRANT
#include "nis_mt.h"
#endif

#define	MAXALIASES	35

static struct protodata {
	char _line[BUFSIZ+1];
	struct protoent _proto;
	char *_proto_aliases[MAXALIASES];
	char *_domain;
	char *_ypkey;
	int   _ypkeylen;
} protodata;

#define line           (d->_line)
#define _proto         (d->_proto)
#define proto_aliases  (d->_proto_aliases)
#define domain         (d->_domain)
#define ypkey          (d->_ypkey)
#define ypkeylen       (d->_ypkeylen)

static struct protoent *interpret();

static struct protodata *
_protodata()
{
	struct protodata *d;

#ifdef _REENTRANT
	struct _nis_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		d = &protodata;
	} else {
		/*
		 * This is the case of threads other than the first.
		 */
		key_tbl = (struct _nis_tsd *)
			_mt_get_thr_specific_storage(_nis_key,_NIS_KEYTBL_SIZE);
		if (key_tbl == NULL)
			return ((struct protodata *)NULL);
		if (key_tbl->proto_info_p == NULL)
			key_tbl->proto_info_p = 
				(struct protodata *)calloc(1, sizeof(struct protodata));
		d = (struct protodata *)key_tbl->proto_info_p;
	}
#else
	d = &protodata;
#endif
	if (d && domain == NULL)
		domain = nis_domain();
	return(d);
}

#ifdef _REENTRANT
void
_free_nis_proto_info(p)
    void *p;
{
    if (FIRST_OR_NO_THREAD)
        return;
    if (p != NULL)
        free(p);
    return;
}
#endif /* _REENTRANT */

nis_setprotoent()
{
	register struct protodata *d = _protodata();

	if (d == NULL)
		return(-1);

	if (ypkey)
		free(ypkey);
	ypkey = (char *)0;
	return(0);
}

nis_endprotoent()
{
	register struct protodata *d = _protodata();

	if (d == NULL)
		return(-1);

	if (ypkey) {
		free(ypkey);
		ypkey = (char *)0;
	}
	return(0);
}

struct protoent *
nis_getprotoent()
{
	register struct protodata *d = _protodata();
	register struct protoent *p;
	char *map = "protocols.bynumber";
	char *val = NULL;
	int   vallen, res;

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
	p = interpret(val, vallen);
	free(val);
	return(p);
}

struct protoent *
nis_getprotobyname(name)
	register char *name;
{
	register struct protodata *d = _protodata();
	register struct protoent *p;
	char *map = "protocols.byname";
	char *val = NULL;
	int   vallen, res;

	if (d == NULL)
		return(NULL);

	res = yp_match(domain, map, name, strlen(name), &val, &vallen);
	if (res){
		set_niserror(res);
		yp_retcode(res);
		return(NULL);
	}
	set_nsaction(NS_SUCCESS);
	p = interpret(val, vallen);
	free(val);
	return(p);
}

struct protoent *
nis_getprotobynumber(proto)
	register int proto;
{
	register struct protodata *d = _protodata();
	register struct protoent *p;
	char *map = "protocols.bynumber";
	char *val = NULL;
	char pnum[12];
	int   vallen, res;

	if (d == NULL)
		return(NULL);

	sprintf(pnum, "%d", proto);
	res = yp_match(domain, map, pnum, strlen(pnum), &val, &vallen);
	if (res) {
		set_niserror(res);
		yp_retcode(res);
		return(NULL);
	}
	set_nsaction(NS_SUCCESS);
	p = interpret(val, vallen);
	free(val);
	return(p);

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
static struct protoent *
interpret(val, len)
char *val;
int len;
{
	register struct protodata *d = _protodata();
	char *p;
	register char *cp, **q;

	strncpy(line, val, len);
	p = line;
	line[len] = '\n';

	_proto.p_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		return(NULL);

	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	_proto.p_proto = atoi(cp);
	q = _proto.p_aliases = proto_aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &proto_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&_proto);
}
