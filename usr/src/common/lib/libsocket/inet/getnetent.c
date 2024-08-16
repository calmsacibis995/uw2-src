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

#ident	"@(#)libsocket:common/lib/libsocket/inet/getnetent.c	1.1.8.12"
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


#include "../socketabi.h"
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#include <stdlib.h>
#include <ns.h>
#include "../libsock_mt.h"

#define	MAXALIASES	35

static const char DBNET[] = "/etc/networks";

static struct netinfo {
	FILE *netf;
	char line[BUFSIZ+1];
	struct netent net;
	char *net_aliases[MAXALIASES];
	int stayopen;
} netinfo;

static char *any();

static struct netinfo *
get_s_netinfo()
{

#ifdef _REENTRANT
        struct _s_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&netinfo);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL) return (struct netinfo *)NULL;
	if (key_tbl->net_info_p == NULL) 
		key_tbl->net_info_p = calloc(1, sizeof(struct netinfo));
	return ((struct netinfo *)key_tbl->net_info_p);
#else /* !_REENTRANT */
	return (&netinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_net_info(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

setnetent(f)
	int f;
{
	NS_SETENT(NETDB, f);

	return(_setnetent(f));
}

_setnetent(f)
	int f;
{
	struct netinfo *nip;

	/* Get thread-specific data */
	if ((nip = get_s_netinfo()) == NULL)
		return ;

	if (nip->netf == NULL)
		nip->netf = fopen(DBNET, "r" );
	else
		rewind(nip->netf);
	nip->stayopen |= f;
}

endnetent()
{
	NS_ENDENT(NETDB);

	return(_endnetent());
}

_endnetent()
{
	struct netinfo *nip;

	/* Get thread-specific data */
	if ((nip = get_s_netinfo()) == NULL)
		return ;

	if (nip->netf) {
		fclose(nip->netf);
		nip->netf = NULL;
	}
	nip->stayopen = 0;
}

struct netent *
_getnetent()
{
	char *p;
	register char *cp, **q;
	struct netent *n;
	struct netinfo *nip;

	/* Get thread-specific data */
	if ((nip = get_s_netinfo()) == NULL)
		return (NULL);


	if (nip->netf == NULL && (nip->netf = fopen(DBNET, "r" )) == NULL)
		return (NULL);
again:
	p = fgets(nip->line, BUFSIZ, nip->netf);
	if (p == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	nip->net.n_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	nip->net.n_net = inet_network(cp);
	nip->net.n_addrtype = AF_INET;
	q = nip->net.n_aliases = nip->net_aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &nip->net_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&nip->net);
}

struct netent *
getnetent()
{
	NS_GETENT(NETDB, struct netent *);

	return(_getnetent());
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

int
_get_s_net_stayopen()
{
	struct netinfo	*nip;

	nip = get_s_netinfo();
	if (nip == NULL)
		return (0);
	return (nip->stayopen);
}
