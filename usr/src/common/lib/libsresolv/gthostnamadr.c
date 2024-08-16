/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresolv:common/lib/libsresolv/gthostnamadr.c	1.1.1.9"
#ident  "$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

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

#include "res.h"
#include <sys/byteorder.h>
#include <sys/param.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "libres_mt.h"

#define	MAXALIASES	35
#define	MAXADDRS	35

#if PACKETSZ > 1024
#define	MAXPACKET	PACKETSZ
#else
#define	MAXPACKET	1024
#endif

#pragma weak gethostbyaddr=_rs_gethostbyaddr
#pragma weak gethostbyname=_rs_gethostbyname
#pragma weak _sethtent=_rs__sethtent
#pragma weak _endhtent=_rs__endhtent
#pragma weak _gethtent=_rs__gethtent
#pragma weak _gethtbyname=_rs__gethtbyname
#pragma weak _gethtbyaddr=_rs__gethtbyaddr

typedef union {
    HEADER hdr;
    u_char buf[MAXPACKET];
} querybuf;

typedef union {
    long al;
    char ac;
} align;

struct state *get_rs__res();

static char *_rs_any();

static struct hostinfo {
	char *h_addr_ptrs[MAXADDRS + 1];
	struct hostent host;
	char *host_aliases[MAXALIASES];
	char hostbuf[BUFSIZ+1];
	struct in_addr host_addr;
	FILE *hostf;
	char hostaddr[MAXADDRS];
	char *host_addrs[2];
	int stayopen;
} hostinfo;

static const char HOSTDB[] = "/etc/hosts";

static struct hostinfo *
get_rs_hostinfo()
{

#ifdef _REENTRANT
        struct _rs_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&hostinfo);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rs_tsd *)
		  _mt_get_thr_specific_storage(_rs_key, _RS_KEYTBL_SIZE);
	if (key_tbl == NULL) return (struct hostinfo *)NULL;
	if (key_tbl->hostinfo_p == NULL) 
		key_tbl->hostinfo_p = calloc(1, sizeof(struct hostinfo));
	return ((struct hostinfo *)key_tbl->hostinfo_p);
#else /* !_REENTRANT */
	return (&hostinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rs_hostinfo(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

static struct hostent *
_rs_getanswer(answer, anslen, iquery)
	querybuf *answer;
	int anslen;
	int iquery;
{
	register HEADER *hp;
	register u_char *cp;
	register int n;
	u_char *eom;
	char *bp, **ap;
	int type, class, buflen, ancount, qdcount;
	int haveanswer, getclass = C_ANY;
	char **hap;
	struct hostinfo *hip;
#ifdef DEBUG
	struct state *rp;
#endif /* DEBUG */

	/* Get thread-specific data */
	if ((hip = get_rs_hostinfo()) == NULL)
		return ((struct hostent *)NULL);
#ifdef DEBUG
	if ((rp = get_rs__res()) == NULL)
		return ((struct hostent *)NULL);
#endif /* DEBUG */

	eom = answer->buf + anslen;
	/*
	 * find first satisfactory answer
	 */
	hp = &answer->hdr;
	ancount = ntohs(hp->ancount);
	qdcount = ntohs(hp->qdcount);
	bp = hip->hostbuf;
	buflen = sizeof(hip->hostbuf);
	cp = answer->buf + sizeof(HEADER);
	if (qdcount) {
		if (iquery) {
			if ((n = _rs_dn_expand((char *)answer->buf, eom,
			     cp, bp, buflen)) < 0) {
				set_h_errno(NO_RECOVERY);
				return ((struct hostent *) NULL);
			}
			cp += n + QFIXEDSZ;
			hip->host.h_name = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
		} else
			cp += _rs_dn_skipname(cp, eom) + QFIXEDSZ;
		while (--qdcount > 0)
			cp += _rs_dn_skipname(cp, eom) + QFIXEDSZ;
	} else if (iquery) {
		if (hp->aa)
			set_h_errno(HOST_NOT_FOUND);
		else
			set_h_errno(TRY_AGAIN);
		return ((struct hostent *) NULL);
	}
	ap = hip->host_aliases;
	hip->host.h_aliases = hip->host_aliases;
	hap = hip->h_addr_ptrs;
	hip->host.h_addr_list = hip->h_addr_ptrs;
	haveanswer = 0;
	while (--ancount >= 0 && cp < eom) {
		if ((n = _rs_dn_expand((char *)answer->buf, eom, cp, bp, buflen)) < 0)
			break;
		cp += n;
		type = _rs__getshort(cp);
 		cp += sizeof(u_short);
		class = _rs__getshort(cp);
 		cp += sizeof(u_short) + sizeof(u_long);
		n = _rs__getshort(cp);
		cp += sizeof(u_short);
		if (type == T_CNAME) {
			cp += n;
			if (ap >= &hip->host_aliases[MAXALIASES-1])
				continue;
			*ap++ = bp;
			n = strlen(bp) + 1;
			bp += n;
			buflen -= n;
			continue;
		}
		if (iquery && type == T_PTR) {
			if ((n = _rs_dn_expand((char *)answer->buf, eom,
			    cp, bp, buflen)) < 0) {
				cp += n;
				continue;
			}
			hip->host.h_name = bp;
			haveanswer++;
			break;
		}
		if (iquery || type != T_A)  {
#ifdef DEBUG
			if (rp->options & RES_DEBUG)
				printf("unexpected answer type %d, size %d\n",
					type, n);
#endif
			cp += n;
			continue;
		}
		if (haveanswer) {
			if (n != hip->host.h_length) {
				cp += n;
				continue;
			}
			if (class != getclass) {
				cp += n;
				continue;
			}
		} else {
			hip->host.h_length = n;
			getclass = class;
			hip->host.h_addrtype
			   = (class == C_IN) ? AF_INET : AF_UNSPEC;
			if (!iquery) {
				hip->host.h_name = bp;
				bp += strlen(bp) + 1;
			}
		}

		bp += (sizeof(align) - 1) -
			(((u_long)bp + sizeof(align) - 1) % sizeof(align));

		if (bp + n >= &hip->hostbuf[sizeof(hip->hostbuf)]) {
#ifdef DEBUG
			if (rp->options & RES_DEBUG)
				printf("size (%d) too big\n", n);
#endif
			break;
		}
		bcopy(cp, *hap++ = bp, n);
		bp +=n;
		cp += n;
		haveanswer++;
	}
	if (haveanswer) {
		/* Terminate alias and address lists with NULL */
		*ap = NULL;
		*hap = NULL;
		return (&hip->host);
	} else {
		set_h_errno(TRY_AGAIN);
		return ((struct hostent *) NULL);
	}
}

struct hostent *
_rs_gethostbyname(name)
	char *name;
{
	querybuf buf;
	register char *cp;
	int n;
	struct hostent *hp, *gethostdomain();
	extern struct hostent *_rs__gethtbyname();

	/*
	 * disallow names consisting only of digits/dots, unless
	 * they end in a dot.
	 */
	if (isdigit(name[0]))
		for (cp = name;; ++cp) {
			if (!*cp) {
				if (*--cp == '.')
					break;
				set_h_errno(HOST_NOT_FOUND);
				return ((struct hostent *) NULL);
			}
			if (!isdigit(*cp) && *cp != '.') 
				break;
		}

	if ((n = _rs_res_search(name, C_IN, T_A, buf.buf, sizeof(buf))) < 0) {
#ifdef DEBUG
		struct state *rp;

		if (((rp = get_rs__res()) != NULL)
		 && (rp->options & RES_DEBUG))
			printf("res_search failed\n");
#endif
		if (errno == ECONNREFUSED || get_h_errno() == HOST_NOT_FOUND)
			return (_rs__gethtbyname(name));
		else
			return ((struct hostent *) NULL);
	}
	return (_rs_getanswer(&buf, n, 0));
}

struct hostent *
_rs_gethostbyaddr(addr, len, type)
	char *addr;
	int len, type;
{
	int n;
	querybuf buf;
	register struct hostent *hp;
	char qbuf[MAXDNAME];
	extern struct hostent *_rs__gethtbyaddr();
	struct hostinfo *hip;
#ifdef DEBUG
	struct state *rp;
#endif /* DEBUG */
	
	/* Get thread-specific data */
	if ((hip = get_rs_hostinfo()) == NULL)
		return ((struct hostent *)NULL);
#ifdef DEBUG
	if ((rp = get_rs__res()) == NULL)
		return ((struct hostent *)NULL);
#endif /* DEBUG */

	if (len != sizeof(struct in_addr))
		return ((struct hostent *) NULL);
	if (type != AF_INET)
		return ((struct hostent *) NULL);
	(void)sprintf(qbuf, "%u.%u.%u.%u.in-addr.arpa",
		((unsigned)addr[3] & 0xff),
		((unsigned)addr[2] & 0xff),
		((unsigned)addr[1] & 0xff),
		((unsigned)addr[0] & 0xff));
	n = _rs_res_query(qbuf, C_IN, T_PTR, (char *)&buf, sizeof(buf));
	if (n < 0) {
#ifdef DEBUG
		if (rp->options & RES_DEBUG)
			printf("res_query failed\n");
#endif
		if (errno == ECONNREFUSED || get_h_errno() == HOST_NOT_FOUND)
			return (_rs__gethtbyaddr(addr, len, type));
		return ((struct hostent *) NULL);
	}
	hp = _rs_getanswer(&buf, n, 1);
	if (hp == NULL)
		return ((struct hostent *) NULL);
	hp->h_addrtype = type;
	hp->h_length = len;
	hip->h_addr_ptrs[0] = (char *)&hip->host_addr;
	hip->h_addr_ptrs[1] = (char *)0;
	hip->host_addr = *(struct in_addr *)addr;
	return(hp);
}

_rs__sethtent(f)
	int f;
{
	struct hostinfo *hip;

	/* Get thread-specific data */
	if ((hip = get_rs_hostinfo()) == NULL)
		return;

	if (hip->hostf == NULL)
		hip->hostf = _fopen(HOSTDB, "r" );
	else
		rewind(hip->hostf);
	hip->stayopen |= f;
}

_rs__endhtent()
{
	struct hostinfo *hip;

	/* Get thread-specific data */
	if ((hip = get_rs_hostinfo()) == NULL)
		return;

	if (hip->hostf && !hip->stayopen) {
		(void) fclose(hip->hostf);
		hip->hostf = NULL;
	}
}

struct hostent *
_rs__gethtent()
{
	char *p;
	register char *cp, **q;
	struct hostinfo *hip;

	/* Get thread-specific data */
	if ((hip = get_rs_hostinfo()) == NULL)
		return ((struct hostent *)NULL);

	if (hip->hostf == NULL && (hip->hostf = _fopen(HOSTDB, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(hip->hostbuf, BUFSIZ, hip->hostf)) == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = _rs_any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	cp = _rs_any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	hip->host.h_addr_list = hip->host_addrs;
	hip->host.h_addr = hip->hostaddr;
	*((u_long *)hip->host.h_addr) = inet_addr(p);
	hip->host.h_length = sizeof (u_long);
	hip->host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	hip->host.h_name = cp;
	q = hip->host.h_aliases = hip->host_aliases;
	cp = _rs_any(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &hip->host_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = _rs_any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&hip->host);
}

static char *
_rs_any(cp, match)
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

struct hostent *
_rs__gethtbyname(name)
	char *name;
{
	register struct hostent *p;
	register char **cp;
	
	_rs__sethtent(0);
	while (p = _rs__gethtent()) {
		if (_rs_strcasecmp(p->h_name, name) == 0)
			break;
		for (cp = p->h_aliases; *cp != 0; cp++)
			if (_rs_strcasecmp(*cp, name) == 0)
				goto found;
	}
found:
	_rs__endhtent();
	return (p);
}

struct hostent *
_rs__gethtbyaddr(addr, len, type)
	char *addr;
	int len, type;
{
	register struct hostent *p;

	_rs__sethtent(0);
	while (p = _rs__gethtent())
		if (p->h_addrtype == type && !bcmp(p->h_addr, addr, len))
			break;
	_rs__endhtent();
	return (p);
}
