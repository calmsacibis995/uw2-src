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

#ident	"@(#)libsocket:common/lib/libsocket/inet/getservent.c	1.1.8.10"
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
#include <sys/byteorder.h>
#include "../libsock_mt.h"
#include <ns.h>

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
get_s_servinfo()
{

#ifdef _REENTRANT
        struct _s_tsd *key_tbl;

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
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL) return ((struct servinfo *)NULL);
	if (key_tbl->serv_info_p == NULL) 
		key_tbl->serv_info_p = calloc(1, sizeof(struct servinfo));
	return ((struct servinfo *)key_tbl->serv_info_p);
#else /* !_REENTRANT */
	return (&servinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_serv_info(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

setservent(f)
	int f;
{
	NS_SETENT(SERVDB, f);

	return(_setservent(f));
}

_setservent(f)
	int f;
{
	struct servinfo *sip;

	/* Get thread-specific data */
	if ((sip = get_s_servinfo()) == NULL)
		return ;

	if (sip->servf == NULL)
		sip->servf = fopen(SERV_DB, "r" );
	else
		rewind(sip->servf);
	sip->stayopen |= f;
}

endservent()
{
	NS_ENDENT(SERVDB);

	return(_endservent());
}

_endservent()
{
	struct servinfo *sip;

	/* Get thread-specific data */
	if ((sip = get_s_servinfo()) == NULL)
		return ;

	if (sip->servf) {
		fclose(sip->servf);
		sip->servf = NULL;
	}
	sip->stayopen = 0;
}

struct servent *
_getservent()
{
	char *p;
	register char *cp, **q;
	struct servent *s;
	struct servinfo *sip;

	/* Get thread-specific data */
	if ((sip = get_s_servinfo()) == NULL)
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
	if (cp != NULL)
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
	*q = NULL;
	return (&sip->serv);
}

struct servent *
getservent()
{
	NS_GETENT(SERVDB, struct servent *);

	return(_getservent());
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
_get_s_serv_stayopen()
{
	struct servinfo	*sip;

	sip = get_s_servinfo();
	if (sip == NULL)
		return (0);
	return (sip->stayopen);
}
