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

#ident	"@(#)libsocket:common/lib/libsocket/inet/getprotoent.c	1.1.8.10"
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
#include "../libsock_mt.h"
#include <ns.h>

#define	MAXALIASES	35

extern int _get_s_proto_stayopen();

static const char PROTO_DB[] = "/etc/protocols";

static struct protoinfo {
	FILE *protof;
	char line[BUFSIZ+1];
	struct protoent proto;
	char *proto_aliases[MAXALIASES];
	int stayopen;
} protoinfo;

static char *any();

static struct protoinfo *
get_s_protoinfo()
{

#ifdef _REENTRANT
        struct _s_tsd *key_tbl;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&protoinfo);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL) return (struct protoinfo *)NULL;
	if (key_tbl->proto_info_p == NULL) 
		key_tbl->proto_info_p = calloc(1, sizeof(struct protoinfo));
	return ((struct protoinfo *)key_tbl->proto_info_p);
#else /* !_REENTRANT */
	return (&protoinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_proto_info(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

setprotoent(f)
	int f;
{
	NS_SETENT(PROTODB, f);

	return(_setprotoent(f));
}

_setprotoent(f)
	int f;
{
	struct protoinfo *pip;

	/* Get thread-specific data */
	if ((pip = get_s_protoinfo()) == NULL)
		return ;

	if (pip->protof == NULL)
		pip->protof = fopen(PROTO_DB, "r" );
	else
		rewind(pip->protof);
	pip->stayopen |= f;
}

endprotoent()
{
	NS_ENDENT(PROTODB);

	return(_endprotoent());
}

_endprotoent()
{
	struct protoinfo *pip;

	/* Get thread-specific data */
	if ((pip = get_s_protoinfo()) == NULL)
		return ;

	if (pip->protof) {
		fclose(pip->protof);
		pip->protof = NULL;
	}
	pip->stayopen = 0;
}

struct protoent *
_getprotoent()
{
	char *p;
	register char *cp, **q;
	struct protoent *pe;
	struct protoinfo *pip;

	/* Get thread-specific data */
	if ((pip = get_s_protoinfo()) == NULL)
		return (NULL);


	if (pip->protof == NULL && (pip->protof = fopen(PROTO_DB, "r" )) == NULL)
		return (NULL);
again:
	if ((p = fgets(pip->line, BUFSIZ, pip->protof)) == NULL)
		return (NULL);
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	pip->proto.p_name = p;
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	while (*cp == ' ' || *cp == '\t')
		cp++;
	p = any(cp, " \t");
	if (p != NULL)
		*p++ = '\0';
	pip->proto.p_proto = atoi(cp);
	q = pip->proto.p_aliases = pip->proto_aliases;
	if (p != NULL) {
		cp = p;
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &pip->proto_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL)
				*cp++ = '\0';
		}
	}
	*q = NULL;
	return (&pip->proto);
}

struct protoent *
getprotoent()
{
	NS_GETENT(PROTODB, struct protoent *);

	return (_getprotoent());
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
_get_s_proto_stayopen()
{
	struct protoinfo	*pip;

	pip = get_s_protoinfo();
	if (pip == NULL)
		return (0);
	return (pip->stayopen);
}
