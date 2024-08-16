/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/novell/file_db.c	1.12"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: file_db.c,v 1.12 1994/09/14 19:10:23 vtag Exp $"
/*
 * This is the C library "getXXXbyYYY" routines after they have been stripped
 * down to using just the file /etc/hosts and /etc/services. When linked with 
 * the internal name to address resolver routines for TCP/IP they provide 
 * the file based version of those routines.
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * This file defines getservbyname().
 * The C library routines are not used as they may
 * one day be based on these routines, and that would cause recursion
 * problems. 
 */

#ifdef NW_UP
#define  _fopen	fopen
#else
#define  fopen	_fopen
#endif /* NW_UP */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <netdir.h>
#include <sys/byteorder.h>
#include <mt.h>
#ifndef NW_UP
#include <netinet/in.h>
#endif /* NW_UP */
#ifdef _REENTRANT
#include "novell_mt.h"
#endif

#define	MAXALIASES	35

static struct servdata {
	FILE	*servf;
	char	*current;
	int	currentlen;
	int	stayopen;
	char	*serv_aliases[MAXALIASES];
	struct	servent serv;
	char	line[BUFSIZ+1];
} *servdata, *_servdata();

static struct servent *_getservent();
static struct servent *_getsapent();
static const char SERVDB[] = "/etc/services";
static const char SAPDB[] = "/etc/netware/saptypes";

static void _setservent(), _endservent();
static void _setsapent(), _endsapent();

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static char charmap[] = {
	'\000', '\001', '\002', '\003', '\004', '\005', '\006', '\007',
	'\010', '\011', '\012', '\013', '\014', '\015', '\016', '\017',
	'\020', '\021', '\022', '\023', '\024', '\025', '\026', '\027',
	'\030', '\031', '\032', '\033', '\034', '\035', '\036', '\037',
	'\040', '\041', '\042', '\043', '\044', '\045', '\046', '\047',
	'\050', '\051', '\052', '\053', '\054', '\055', '\056', '\057',
	'\060', '\061', '\062', '\063', '\064', '\065', '\066', '\067',
	'\070', '\071', '\072', '\073', '\074', '\075', '\076', '\077',
	'\100', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\133', '\134', '\135', '\136', '\137',
	'\140', '\141', '\142', '\143', '\144', '\145', '\146', '\147',
	'\150', '\151', '\152', '\153', '\154', '\155', '\156', '\157',
	'\160', '\161', '\162', '\163', '\164', '\165', '\166', '\167',
	'\170', '\171', '\172', '\173', '\174', '\175', '\176', '\177',
	'\200', '\201', '\202', '\203', '\204', '\205', '\206', '\207',
	'\210', '\211', '\212', '\213', '\214', '\215', '\216', '\217',
	'\220', '\221', '\222', '\223', '\224', '\225', '\226', '\227',
	'\230', '\231', '\232', '\233', '\234', '\235', '\236', '\237',
	'\240', '\241', '\242', '\243', '\244', '\245', '\246', '\247',
	'\250', '\251', '\252', '\253', '\254', '\255', '\256', '\257',
	'\260', '\261', '\262', '\263', '\264', '\265', '\266', '\267',
	'\270', '\271', '\272', '\273', '\274', '\275', '\276', '\277',
	'\300', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\333', '\334', '\335', '\336', '\337',
	'\340', '\341', '\342', '\343', '\344', '\345', '\346', '\347',
	'\350', '\351', '\352', '\353', '\354', '\355', '\356', '\357',
	'\360', '\361', '\362', '\363', '\364', '\365', '\366', '\367',
	'\370', '\371', '\372', '\373', '\374', '\375', '\376', '\377',
};

int
strcasecmp(s1, s2)
	register char *s1, *s2;
{
	register char *cm = charmap;

	while (cm[*s1] == cm[*s2++])
		if (*s1++ == '\0')
			return(0);
	return(cm[*s1] - cm[*--s2]);
}

static struct servdata *
_servdata()
{
	struct servdata *d;

#ifdef _REENTRANT
        struct _ntoa_novell_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		if (servdata == NULL)
			servdata
			 = (struct servdata *)
			   calloc(1, sizeof (struct servdata));
		d = servdata;
	} else {

		key_tbl = (struct _ntoa_novell_tsd *)
			  _mt_get_thr_specific_storage(_ntoa_novell_key,
						       _NTOA_NOVELL_KEYTBL_SIZE);
		if (key_tbl == NULL) return NULL;
		if (key_tbl->servdata_p == NULL) 
		      key_tbl->servdata_p = calloc(1, sizeof (struct servdata));
		d = key_tbl->servdata_p;
	}
#else
	if (servdata == NULL)
		servdata = (struct servdata *)
			   calloc(1, sizeof (struct servdata));
	d = servdata;
#endif /* _REENTRANT */
	return (d);
}

#ifdef _REENTRANT

void
_free_ntoa_novell_data(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

struct servent *
_novell_getservbysocket(socket, proto)
	int socket;
	char *proto;
{
	register struct servdata *d = _servdata();
	register struct servent *p = NULL;
	register ushort_t port = (ushort_t)socket;

	if (d == NULL)
		return (NULL);

	_setservent(d, 0);
	while (p = _getservent(d)) {
		if (p->s_port != port)
			continue;
		if (proto == 0 || strcasecmp(p->s_proto, proto) == 0)
			break;
	}
	_endservent(d);
	return (p);
}

struct servent *
_novell_getservbyname(name, proto)
	register char *name, *proto;
{
	register struct servdata *d = _servdata();
	register struct servent *p;
	register char **cp;

	if (d == NULL)
		return (NULL);
	_setservent(d, 0);
	while (p = _getservent(d)) {
		if (proto != 0 && strcasecmp(p->s_proto, proto) != 0)
			continue;
		if (strcasecmp(name, p->s_name) == 0)
			break;
		for (cp = p->s_aliases; *cp; cp++)
			if (strcasecmp(name, *cp) == 0)
				break;
		if (*cp) 
			break;	/* we found it */
	}
	_endservent(d);
	return (p);
}

static void
_setservent(d, f)
	struct servdata *d;
	int f;
{

	if (d->servf == NULL)
		d->servf = _fopen(SERVDB, "r");
	else
		rewind(d->servf);
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
}

static void
_endservent(d)
	struct servdata *d;
{
	if (d->current && !d->stayopen) {
		free(d->current);
		d->current = NULL;
	}
	if (d->servf && !d->stayopen) {
		fclose(d->servf);
		d->servf = NULL;
	}
}

static struct servent *
_getservent(d)
	struct servdata *d;
{
	char *p;
	register char *cp, **q;

	/* Assume d is not NULL */
	if (d->servf == NULL && (d->servf = _fopen(SERVDB, "r")) == NULL)
		return (NULL);

tryagain:
	if (fgets(d->line, BUFSIZ, d->servf) == NULL)
		return (NULL);

	p = d->line;

	if (*p == '#')
		goto tryagain;

	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		goto tryagain;
	*cp = '\0';

	d->serv.s_name = p;
	p = strpbrk(p, " \t");
	if (p == NULL)
		goto tryagain;
	*p++ = '\0';

	while (*p == ' ' || *p == '\t')
		p++;
	cp = strpbrk(p, ",/");
	if (cp == NULL)
		goto tryagain;
	*cp++ = '\0';
	d->serv.s_port = htons((ushort_t)atoi(p));
	d->serv.s_proto = cp;
	q = d->serv.s_aliases = d->serv_aliases;
	cp = strpbrk(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &(d->serv_aliases[MAXALIASES - 1]))
				*q++ = cp;
			cp = strpbrk(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&d->serv);
}

static struct servdata *
_sapdata()
{
	struct servdata *d;

#ifdef _REENTRANT
        struct _ntoa_novell_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		if (servdata == NULL)
			servdata
			 = (struct servdata *)
			   calloc(1, sizeof (struct servdata));
		d = servdata;
	} else {

		key_tbl = (struct _ntoa_novell_tsd *)
			  _mt_get_thr_specific_storage(_ntoa_novell_key,
						       _NTOA_NOVELL_KEYTBL_SIZE);
		if (key_tbl == NULL) return NULL;
		if (key_tbl->sapdata_p == NULL) 
		      key_tbl->sapdata_p = calloc(1, sizeof (struct servdata));
		d = key_tbl->sapdata_p;
	}
#else
	if (servdata == NULL)
		servdata = (struct servdata *)
			   calloc(1, sizeof (struct servdata));
	d = servdata;
#endif /* _REENTRANT */
	return (d);
}

struct servent *
_novell_getsapnamebynumber(saptype)
	register unsigned short saptype;
{
	register struct servdata *d = _sapdata();
	register struct servent *p;

	if (d == NULL)
		return (NULL);
	_setsapent(d, 0);
	while (p = _getsapent(d)) {
		if (p->s_port == saptype)
			break;
	}
	_endsapent(d);
	return (p);
}

struct servent *
_novell_getsapnumberbyname(name)
	register char *name;
{
	register struct servdata *d = _sapdata();
	register struct servent *p;
	register char **cp;

	if (d == NULL)
		return (NULL);
	_setsapent(d, 0);
	while (p = _getsapent(d)) {
		if (strcasecmp(name, p->s_name) == 0)
			break;
		for (cp = p->s_aliases; *cp; cp++)
			if (strcasecmp(name, *cp) == 0)
				break;
		if (*cp) 
			break;	/* we found it */
	}
	_endsapent(d);
	return (p);
}

static void
_setsapent(d, f)
	struct servdata *d;
	int f;
{

	if (d->servf == NULL)
		d->servf = _fopen(SAPDB, "r");
	else
		rewind(d->servf);
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
}

static void
_endsapent(d)
	struct servdata *d;
{
	if (d->current && !d->stayopen) {
		free(d->current);
		d->current = NULL;
	}
	if (d->servf && !d->stayopen) {
		fclose(d->servf);
		d->servf = NULL;
	}
}

static struct servent *
_getsapent(d)
	struct servdata *d;
{
	char *p;
	register char *cp, **q;

	/* Assume d is not NULL */
	if (d->servf == NULL && (d->servf = _fopen(SAPDB, "r")) == NULL)
		return (NULL);

tryagain:
	if (fgets(d->line, BUFSIZ, d->servf) == NULL)
		return (NULL);

	p = d->line;

	if (*p == '#')
		goto tryagain;

	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		goto tryagain;
	*cp = '\0';

	d->serv.s_name = p;
	p = strpbrk(p, " \t");
	if (p == NULL)
		goto tryagain;
	*p++ = '\0';

	while (*p == ' ' || *p == '\t')
		p++;
	d->serv.s_port = (ushort_t)atoi(p);
	q = d->serv.s_aliases = d->serv_aliases;
	cp = strpbrk(p, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &(d->serv_aliases[MAXALIASES - 1]))
				*q++ = cp;
			cp = strpbrk(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&d->serv);
}
