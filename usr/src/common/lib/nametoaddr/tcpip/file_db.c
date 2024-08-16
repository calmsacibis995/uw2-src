/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nametoaddr:common/lib/nametoaddr/tcpip/file_db.c	1.3.6.8"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * This is the C library "getXXXbyYYY" routines after they have been stripped
 * down to using just the file /etc/hosts and /etc/services. When linked with 
 * the internal name to address resolver routines for TCP/IP they provide 
 * the file based version of those routines.
 *
 * Copyright (c) 1989 by Sun Microsystems, Inc.
 *
 * This file defines gethostbyname(), gethostbyaddr(), getservbyname(), 
 * and getservbyport(). The C library routines are not used as they may
 * one day be based on these routines, and that would cause recursion
 * problems. 
 */

#define  fopen	_fopen
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netdir.h>
#include <netinet/in.h>
#include "tcpip_mt.h"

#define	MAXALIASES	35
#define MAXADDRS	10
#define	MAXADDRSIZE	14

static struct hostdata {
	FILE	*hostf;
	char	*current;
	int	currentlen;
	int	stayopen;
	char	*host_aliases[MAXALIASES];
	char	hostaddr[MAXADDRSIZE][MAXADDRS];
	char	*addr_list[MAXADDRS+1];
	char	line[BUFSIZ+1];
	struct	hostent host;
} *hostdata, *_hostdata();

static struct servdata {
	FILE	*servf;
	char	*current;
	int	currentlen;
	int	stayopen;
	char	*serv_aliases[MAXALIASES];
	struct	servent serv;
	char	line[BUFSIZ+1];
} *servdata, *_servdata();

static struct hostent *_gethostent();
static const char HOSTDB[] = "/etc/hosts";

static struct servent *_getservent();
static const char SERVDB[] = "/etc/services";

static void _sethostent(), _endhostent(), _setservent(), _endservent();

static u_long _tcpip_inet_addr();

static struct hostdata *
_hostdata()
{
	struct hostdata *d;

#ifdef _REENTRANT
        struct _ntoa_tcpip_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		if (hostdata == NULL)
			hostdata
			 = (struct hostdata *)
			   calloc(1, sizeof (struct hostdata));
		d = hostdata;
	} else {

		key_tbl = (struct _ntoa_tcpip_tsd *)
			  _mt_get_thr_specific_storage(_ntoa_tcpip_key,
						       _NTOA_TCPIP_KEYTBL_SIZE);
		if (key_tbl == NULL) return NULL;
		if (key_tbl->hostdata_p == NULL) 
		      key_tbl->hostdata_p = calloc(1, sizeof (struct hostdata));
		d = key_tbl->hostdata_p;
	}
#else
	if (hostdata == NULL)
		hostdata = (struct hostdata *)
			   calloc(1, sizeof (struct hostdata));
	d = hostdata;
#endif /* _REENTRANT */
	return (d);
}

#ifdef _REENTRANT

void
_free_ntoa_tcpip_hostdata(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

struct hostent *
_tcpip_gethostbyaddr(addr, len, type)
	char *addr;
	int len, type;
{
	register struct hostdata *d = _hostdata();
	register struct hostent *p;

	if (d == NULL)
		return ((struct hostent*)NULL);

	_sethostent(d, 0);
	while (p = _gethostent(d)) {
		if (p->h_addrtype != type || p->h_length != len)
			continue;
		if (memcmp(p->h_addr_list[0], addr, len) == 0)
			break;
	}
	_endhostent(d);
	return (p);
}

struct hostent *
_tcpip_gethostbyname(name)
	register char *name;
{
	register struct hostdata *d = _hostdata();
	register struct hostent *p;
	register char **cp;
	u_long addr;

	if (d == NULL)
		return ((struct hostent*)NULL);

	addr = _tcpip_inet_addr(name);
	if ((int)addr != -1) {
		/* parse 1.2.3.4 case */
		d->host.h_name = name;
		d->addr_list[0] = d->hostaddr[0];
		d->addr_list[1] = NULL;
		d->host.h_addr_list = d->addr_list;
		*((u_long *)d->host.h_addr) = addr;
		d->host.h_length = sizeof (u_long);
		d->host.h_addrtype = AF_INET;
		d->host.h_aliases = d->host_aliases;
		return (&d->host);
	}

	if (strcmp(name, HOST_ANY) == 0) 
		return ((struct hostent *)NULL);

	_sethostent(d, 0);
	while (p = _gethostent(d)) {
		if (strcasecmp(p->h_name, name) == 0) {
			break;
		}
		for (cp = p->h_aliases; *cp != 0; cp++) 
			if (strcasecmp(*cp, name) == 0)
				break;
		if (*cp) 
			break;	/* We found it */
	}

	_endhostent(d);
	return (p);
}

static void
_sethostent(d, f)
	struct hostdata *d;
	int f;
{

	if (d->hostf == NULL)
		d->hostf = _fopen(HOSTDB, "r");
	else
		rewind(d->hostf);
	if (d->current)
		free(d->current);
	d->current = NULL;
	d->stayopen |= f;
}

static void
_endhostent(d)
	struct hostdata *d;
{

	if (d->current && !d->stayopen) {
		free(d->current);
		d->current = NULL;
	}
	if (d->hostf && !d->stayopen) {
		fclose(d->hostf);
		d->hostf = NULL;
	}
}

static struct hostent *
_gethostent(d)
	struct hostdata *d;
{
	struct hostent *he;
	register char	*cp, **q;
	char 		*p;

	/* Assume d is not NULL */
	if (d->hostf == NULL && (d->hostf = _fopen(HOSTDB, "r")) == NULL)
		return (NULL);

more:
	if (fgets(d->line, BUFSIZ, d->hostf) == NULL) 
		return (NULL);

	/*
	 * This code interprets the current line. 
	 */


	p = d->line;

	/* Check for comment lines (start with # mark) */
	if (*p == '#')
		goto more;
	cp = strpbrk(p, "#\n");
	if (cp == NULL)
		goto more;
	*cp = '\0'; /* Null out any trailing comment */

	/* Now check for whitespace */
	cp = strpbrk(p, " \t");
	if (cp == NULL)
		goto more;
	*cp++ = '\0'; /* This breaks the line into name/address components */

	/* return one address */
	d->addr_list[0] = (d->hostaddr[0]);
	d->addr_list[1] = NULL;
	d->host.h_addr_list = d->addr_list;
	*((u_long *)d->host.h_addr) = _tcpip_inet_addr(p);
	d->host.h_length = sizeof (u_long);
	d->host.h_addrtype = AF_INET;

	/* skip whitespace between address and the name */
	while (*cp == ' ' || *cp == '\t')
		cp++;
	d->host.h_name = cp;
	q = d->host.h_aliases = d->host_aliases;

	cp = strpbrk(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';

		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &(d->host_aliases[MAXALIASES - 1]))
				*q++ = cp;
			cp = strpbrk(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&d->host);
}

/*
 * The services routines. These nearly identical to the host routines
 * above. The Interpret routine differs. Seems there should be some way
 * to fold this code together.
 */

static struct servdata *
_servdata()
{
	struct servdata *d;

#ifdef _REENTRANT
        struct _ntoa_tcpip_tsd *key_tbl;

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

		key_tbl = (struct _ntoa_tcpip_tsd *)
			  _mt_get_thr_specific_storage(_ntoa_tcpip_key,
						       _NTOA_TCPIP_KEYTBL_SIZE);
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
_free_ntoa_tcpip_servdata(p)
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
_tcpip_getservbyport(svc_port, proto)
	int svc_port;
	char *proto;
{
	register struct servdata *d = _servdata();
	register struct servent *p = NULL;
	register u_short port = svc_port;

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
_tcpip_getservbyname(name, proto)
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
	struct servent *se;
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
	d->serv.s_port = htons((u_short)atoi(p));
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

/*
 * Internet address interpretation routine.
 * All the network library routines call this
 * routine to interpret entries in the data bases
 * which are expected to be an address.
 * The value returned is in network order.
 */
static u_long
_tcpip_inet_addr(cp)
	register char *cp;
{
	register u_long val, base, n;
	register char c;
	u_long parts[4], *pp = parts;

again:
	/*
	 * Collect number up to ``.''.
	 * Values are specified as for C:
	 * 0x=hex, 0=octal, other=decimal.
	 */
	val = 0; base = 10;
	if (*cp == '0') {
		if (*++cp == 'x' || *cp == 'X')
			base = 16, cp++;
		else
			base = 8;
	}
	while (c = *cp) {
#undef isdigit
		if (isdigit(c)) {
			if ((c - '0') >= base)
			    break;
			val = (val * base) + (c - '0');
			cp++;
			continue;
		}
		if (base == 16 && isxdigit(c)) {
			val = (val << 4) + (c + 10 - (islower(c) ? 'a' : 'A'));
			cp++;
			continue;
		}
		break;
	}
	if (*cp == '.') {
		/*
		 * Internet format:
		 *	a.b.c.d
		 *	a.b.c	(with c treated as 16-bits)
		 *	a.b	(with b treated as 24 bits)
		 */
		if (pp >= parts + 4)
			return (-1);
		*pp++ = val, cp++;
		goto again;
	}
	/*
	 * Check for trailing characters.
	 */
	if (*cp && !isspace(*cp))
		return (-1);
	*pp++ = val;
	/*
	 * Concoct the address according to
	 * the number of parts specified.
	 */
	n = pp - parts;
	switch (n) {

	case 1:				/* a -- 32 bits */
		val = parts[0];
		break;

	case 2:				/* a.b -- 8.24 bits */
		val = (parts[0] << 24) | (parts[1] & 0xffffff);
		break;

	case 3:				/* a.b.c -- 8.8.16 bits */
		val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
			(parts[2] & 0xffff);
		break;

	case 4:				/* a.b.c.d -- 8.8.8.8 bits */
		val = (parts[0] << 24) | ((parts[1] & 0xff) << 16) |
		      ((parts[2] & 0xff) << 8) | (parts[3] & 0xff);
		break;

	default:
		return (-1);
	}
	val = htonl(val);
	return (val);
}

/*
 * XXX: This should be apart of C library. If not so, then please include
 * the correct routine. This is just a wrapper.
 */
int static
strcasecmp(a, b)
	char *a, *b;
{
	char str1[BUFSIZ], str2[BUFSIZ], *tmp;

	strcpy(str1, a);
	strcpy(str2, b);

	for (tmp = str1; *tmp != NULL; ++tmp) {
		*tmp = (char)tolower((int)((unsigned char)*tmp));
	}

	for (tmp = str2; *tmp != NULL; ++tmp) {
		*tmp = (char)tolower((int)((unsigned char)*tmp));
	}

	return(strcmp(str1, str2));
}
