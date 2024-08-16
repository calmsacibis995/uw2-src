/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libresolv:common/lib/libsresolv/res_query.c	1.1.1.7"
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
#include <string.h>
#include <arpa/inet.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include "libres_mt.h"

#if PACKETSZ > 1024
#define MAXPACKET	PACKETSZ
#else
#define MAXPACKET	1024
#endif

#pragma weak res_query=_rs_res_query
#pragma weak res_search=_rs_res_search
#pragma weak res_querydomain=_rs_res_querydomain
#pragma weak hostalias=_rs_hostalias

extern struct state *get_rs__res();

/*
 * Formulate a normal query, send, and await answer.
 * Returned answer is placed in supplied buffer "answer".
 * Perform preliminary check of answer, returning success only
 * if no error is indicated and the answer count is nonzero.
 * Return the size of the response on success, -1 on error.
 * Error number is left in h_errno.
 * Caller must parse answer and determine whether it answers the question.
 */
_rs_res_query(name, class, type, answer, anslen)
	char *name;		/* domain name */
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer buffer */
{
	char buf[MAXPACKET];
	HEADER *hp;
	int n;
	struct state *rp;

	/* Get thread-specific storage */
	if ((rp = get_rs__res()) == NULL)
		return (-1);

	if ((rp->options & RES_INIT) == 0 && _rs_res_init() == -1)
		return (-1);
#ifdef DEBUG
	if (rp->options & RES_DEBUG)
		printf("_rs_res_query(%s, %d, %d)\n", name, class, type);
#endif
	n = _rs_res_mkquery(QUERY, name, class, type, (char *)NULL, 0, NULL,
	    buf, sizeof(buf));

	if (n <= 0) {
#ifdef DEBUG
		if (rp->options & RES_DEBUG)
			printf("res_query: mkquery failed\n");
#endif
		set_h_errno(NO_RECOVERY);
		return (n);
	}
	n = _rs_res_send(buf, n, answer, anslen);
	if (n < 0) {
#ifdef DEBUG
		if (rp->options & RES_DEBUG)
			printf("res_query: send error\n");
#endif
		set_h_errno(TRY_AGAIN);
		return(n);
	}

	hp = (HEADER *) answer;
	if (hp->rcode != NOERROR || ntohs(hp->ancount) == 0) {
#ifdef DEBUG
		if (rp->options & RES_DEBUG)
			printf("rcode = %d, ancount=%d\n", hp->rcode,
			    ntohs(hp->ancount));
#endif
		switch (hp->rcode) {
			case NXDOMAIN:
				set_h_errno(HOST_NOT_FOUND);
				break;
			case SERVFAIL:
				set_h_errno(TRY_AGAIN);
				break;
			case NOERROR:
				set_h_errno(NO_DATA);
				break;
			case FORMERR:
			case NOTIMP:
			case REFUSED:
			default:
				set_h_errno(NO_RECOVERY);
				break;
		}
		return (-1);
	}
	return(n);
}

/*
 * Formulate a normal query, send, and retrieve answer in supplied buffer.
 * Return the size of the response on success, -1 on error.
 * If enabled, implement search rules until answer or unrecoverable failure
 * is detected.  Error number is left in h_errno.
 * Only useful for queries in the same name hierarchy as the local host
 * (not, for example, for host address-to-name lookups in domain in-addr.arpa).
 */
_rs_res_search(name, class, type, answer, anslen)
	char *name;		/* domain name */
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer */
{
	register char *cp, **domain;
	int n, ret, got_nodata = 0;
	char *_rs_hostalias();
	struct state *rp;

	/* Get thread-specific storage */
	if ((rp = get_rs__res()) == NULL)
		return (-1);

	if ((rp->options & RES_INIT) == 0 && _rs_res_init() == -1)
		return (-1);

	errno = 0;
	set_h_errno(HOST_NOT_FOUND);		/* default, if we never query */
	for (cp = name, n = 0; *cp; cp++)
		if (*cp == '.')
			n++;
	if (n == 0 && (cp = _rs_hostalias(name)))
		return (_rs_res_query(cp, class, type, answer, anslen));

	/*
         * We do at least one level of search if
         *      - there is no dot and RES_DEFNAME is set, or
         *      - there is at least one dot, there is no trailing dot,
         *        and RES_DNSRCH is set.
         */
	if ((n == 0 && rp->options & RES_DEFNAMES) ||
	   (n != 0 && *--cp != '.' && rp->options & RES_DNSRCH))
	    for (domain = rp->dnsrch; *domain; domain++) {
		ret = _rs_res_querydomain(name, *domain, class, type,
		    answer, anslen);
		if (ret > 0)
			return (ret);
		/*
		 * If no server present, give up.
		 * If name isn't found in this domain,
		 * keep trying higher domains in the search list
		 * (if that's enabled).
		 * On a NO_DATA error, keep trying, otherwise
		 * a wildcard entry of another type could keep us
		 * from finding this entry higher in the domain.
		 * If we get some other error (negative answer or
		 * server failure), then stop searching up,
		 * but try the input name below in case it's fully-qualified.
		 */
		if (errno == ECONNREFUSED) {
			set_h_errno(TRY_AGAIN);
			return (-1);
		}
		if (get_h_errno() == NO_DATA)
			got_nodata++;
		if ((get_h_errno() != HOST_NOT_FOUND
		     && get_h_errno() != NO_DATA) 
		 || (rp->options & RES_DNSRCH) == 0)
			break;
	}
	/*
	 * If the search/default failed, try the name as fully-qualified,
	 * but only if it contained at least one dot (even trailing).
	 * This is purely a heuristic; we assume that any reasonable query
	 * about a top-level domain (for servers, SOA, etc) will not use
	 * _rs_res_search.
	 */
	if (n && (ret = _rs_res_querydomain(name, (char *) NULL, class, type,
	    answer, anslen)) > 0)
		return (ret);
	if (got_nodata)
		set_h_errno(NO_DATA);
	return (-1);
}

/*
 * Perform a call on res_query on the concatenation of name and domain,
 * removing a trailing dot from name if domain is NULL.
 */
_rs_res_querydomain(name, domain, class, type, answer, anslen)
	char *name, *domain;
	int class, type;	/* class and type of query */
	u_char *answer;		/* buffer to put answer */
	int anslen;		/* size of answer */
{
	char nbuf[2*MAXDNAME+2];
	char *longname = nbuf;
	int n;
	struct state *rp;

	/* Get thread-specific storage */
	if ((rp = get_rs__res()) == NULL)
		return (-1);

#ifdef DEBUG
	if (rp->options & RES_DEBUG)
		printf("_rs_res_querydomain(%s, %s, %d, %d)\n",
		    name, domain, class, type);
#endif
	if (domain == NULL) {
		/*
		 * Check for trailing '.';
		 * copy without '.' if present.
		 */
		n = strlen(name) - 1;
		if (name[n] == '.' && n < sizeof(nbuf) - 1) {
			bcopy(name, nbuf, n);
			nbuf[n] = '\0';
		} else
			longname = name;
	} else
		(void)sprintf(nbuf, "%.*s.%.*s",
		    MAXDNAME, name, MAXDNAME, domain);

	return (_rs_res_query(longname, class, type, answer, anslen));
}

static char *
get_rs_abuf()
{
	static char abuf[MAXDNAME];
#ifdef _REENTRANT
        struct _rs_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (abuf);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _rs_tsd *)
		  _mt_get_thr_specific_storage(_rs_key, _RS_KEYTBL_SIZE);
	if (key_tbl == NULL) return (char *)NULL;
	if (key_tbl->abuf_p == NULL) 
		key_tbl->abuf_p = calloc(1, sizeof(abuf));
	return ((char *)key_tbl->abuf_p);
#else /* !_REENTRANT */
	return (abuf);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_rs_abuf(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

char *
_rs_hostalias(name)
	register char *name;
{
	register char *C1, *C2;
	FILE *fp;
	char *file, *getenv(), *strcpy(), *strncpy();
	char buf[BUFSIZ];
	char *ap;

	/* Get thread-specific data */
	if ((ap = get_rs_abuf()) == NULL)
		return ((char *)NULL);

	file = getenv("HOSTALIASES");
	if (file == NULL || (fp = _fopen(file, "r")) == NULL)
		return (NULL);
	buf[sizeof(buf) - 1] = '\0';
	while (fgets(buf, sizeof(buf), fp)) {
		for (C1 = buf; *C1 && !isspace(*C1); ++C1);
		if (!*C1)
			break;
		*C1 = '\0';
		if (!_rs_strcasecmp(buf, name)) {
			while (isspace(*++C1));
			if (!*C1)
				break;
			for (C2 = C1 + 1; *C2 && !isspace(*C2); ++C2);
			*(ap + sizeof(*ap) - 1) = *C2 = '\0';
			(void)strncpy(ap, C1, sizeof(*ap) - 1);
			fclose(fp);
			return (ap);
		}
	}
	fclose(fp);
	return (NULL);
}
