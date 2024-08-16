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

#ident	"@(#)libresolv:common/lib/libsresolv/gtservnamprt.c	1.2"
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
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */

#define fopen	_fopen

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/byteorder.h>
#include "libres_mt.h"

#define	MAXALIASES	35

static const char SERV_DB[] = "/etc/services";

static struct servinfo {
	FILE *servf;
	char line[BUFSIZ+1];
	struct servent serv;
	char *serv_aliases[MAXALIASES];
	int stayopen;
} servinfo;

static char *any();

static struct servinfo *
get_rs_servinfo()
{

#ifdef _REENTRANT
        struct _rs_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&servinfo);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rs_tsd *)
		  _mt_get_thr_specific_storage(_rs_key, _RS_KEYTBL_SIZE);
	if (key_tbl == NULL) return ((struct servinfo *)NULL);
	if (key_tbl->servinfo_p == NULL) 
		key_tbl->servinfo_p = calloc(1, sizeof(struct servinfo));
	return ((struct servinfo *)key_tbl->servinfo_p);
#else /* !_REENTRANT */
	return (&servinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rs_servinfo(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

static void
_rs_setservent(f)
	int f;
{
	struct servinfo *sip;

	/* Get thread-specific data */
	if ((sip = get_rs_servinfo()) == NULL)
		return ;

	if (sip->servf == NULL)
		sip->servf = fopen(SERV_DB, "r" );
	else
		rewind(sip->servf);
	sip->stayopen |= f;
}

static void
_rs_endservent()
{
	struct servinfo *sip;

	/* Get thread-specific data */
	if ((sip = get_rs_servinfo()) == NULL)
		return ;

	if (sip->servf) {
		fclose(sip->servf);
		sip->servf = NULL;
	}
	sip->stayopen = 0;
}

static struct servent *
_rs_getservent()
{
	char *p;
	register char *cp, **q;
	struct servent *s;
	struct servinfo *sip;

	/* Get thread-specific data */
	if ((sip = get_rs_servinfo()) == NULL)
		return (NULL);


	if (sip->servf == NULL && (sip->servf = fopen(SERV_DB, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(sip->line, BUFSIZ, sip->servf)) == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	sip->serv.s_name = p;
	p = any(p, " \t");
	if (p == NULL)
		goto again;
	*p++ = '\0';
	while (*p == ' ' || *p == '\t')
		p++;
	cp = any(p, ",/");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	sip->serv.s_port = htons((u_short)atoi(p));
	sip->serv.s_proto = cp;
	q = sip->serv.s_aliases = sip->serv_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &sip->serv_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&sip->serv);
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

static int
_get_rs_serv_stayopen()
{
	struct servinfo	*sip;

	sip = get_rs_servinfo();
	if (sip == NULL)
		return (0);
	return (sip->stayopen);
}

struct servent *
_rs_getservbyname(name, proto)
	char *name, *proto;
{
	struct servent *p;
	char **cp;
	int serv_stayopen;

	serv_stayopen = _get_rs_serv_stayopen();
	_rs_setservent(serv_stayopen);
	while (p = _rs_getservent()) {
		if (strcmp(name, p->s_name) == 0)
			goto gotname;
		for (cp = p->s_aliases; *cp; cp++)
			if (strcmp(name, *cp) == 0)
				goto gotname;
		continue;
gotname:
		if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			break;
	}
	if (!serv_stayopen)
		_rs_endservent();
	return (p);
}

struct servent *
_rs_getservbyport(port, proto)
	int port;
	char *proto;
{
	struct servent *p;
	int serv_stayopen;

	serv_stayopen = _get_rs_serv_stayopen();
	_rs_setservent(serv_stayopen);
	while (p = _rs_getservent()) {
		if (p->s_port != port)
			continue;
		if (proto == 0 || strcmp(p->s_proto, proto) == 0)
			break;
	}
	if (!serv_stayopen)
		_rs_endservent();
	return (p);
}
