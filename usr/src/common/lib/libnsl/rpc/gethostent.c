/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libnsl:common/lib/libnsl/rpc/gethostent.c	1.2.8.10"
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
 * Sun RPC is a product of Sun Microsystems, Inc. and is provided for
 * unrestricted use provided that this legend is included on all tape
 * media and as a part of the software program in whole or part.  Users
 * may copy or modify Sun RPC without charge, but are not authorized
 * to license or distribute it to anyone else except as part of a product or
 * program developed by the user.
 *
 * SUN RPC IS PROVIDED AS IS WITH NO WARRANTIES OF ANY KIND INCLUDING THE
 * WARRANTIES OF DESIGN, MERCHANTIBILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE, OR ARISING FROM A COURSE OF DEALING, USAGE OR TRADE PRACTICE.
 *
 * Sun RPC is provided with no support and without any obligation on the
 * part of Sun Microsystems, Inc. to assist in its use, correction,
 * modification or enhancement.
 *
 * SUN MICROSYSTEMS, INC. SHALL HAVE NO LIABILITY WITH RESPECT TO THE
 * INFRINGEMENT OF COPYRIGHTS, TRADE SECRETS OR ANY PATENTS BY SUN RPC
 * OR ANY PART THEREOF.
 *
 * In no event will Sun Microsystems, Inc. be liable for any lost revenue
 * or profits or other special, indirect and consequential damages, even if
 * Sun has been advised of the possibility of such damages.
 *
 * Sun Microsystems, Inc.
 * 2550 Garcia Avenue
 * Mountain View, California  94043
 *
 * Copyright (c) 1986-1991 by Sun Microsystems Inc.
 */

/*
 * gethostent.c
 */

#include <stdio.h>
#include <unistd.h>
#include <rpc/rpc.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <ctype.h>
#ifdef havdndbm
#include <ndbm.h>
#endif
#include "trace.h"
#include <sys/syslog.h>
#include "rpc_mt.h"
#include <ns.h>

#define bcmp(s1, s2, len)	memcmp(s1, s2, len)
#define bcopy(s1, s2, len)	memcpy(s2, s1, len)

/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14

/*
 * rpc_hostent_private:
 * rpc_hostent_private is used only under single-threaded or !_REENTRANT.
 * So no lock is held during its initialization.
 */
struct rpc_hostent_private {
	FILE *hostf;
	char line[BUFSIZ+1];
	char hostaddr[MAXADDRSIZE];
	struct hostent host;
	char *host_aliases[MAXALIASES];
	char *host_addrs[2];
	int stayopen;
} *rpc_hostent_private;

/*
 * The following is shared with gethostnamadr.c
 */
char	*_host_file = "/etc/hosts";

#ifdef havdndbm
DBM	*_host_db;	/* set by gethostbyname(), gethostbyaddr() */
#endif

static char *any();
struct hostent *_gethtent();

/*
 * The returned value of rpchostent() is maintained by
 * per-thread base.
 */
static struct rpc_hostent_private *
_get_rpc_hostent_private()
{
	struct rpc_hostent_private *rhp;
#ifdef _REENTRANT
	struct _rpc_tsd *key_tbl;
#endif /* _REENTRANT */

	trace1(TR___get_rpc_hostent_private, 0);

#ifdef _REENTRANT
	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		if (rpc_hostent_private != NULL) {
			trace1(TR___get_rpc_hostent_private, 1);
			return(rpc_hostent_private);
		}
	} else {

	/*
	 * This is the case of threads other than the first.
	 */
		key_tbl = (struct _rpc_tsd *)
			  _mt_get_thr_specific_storage(__rpc_key,
						       RPC_KEYTBL_SIZE);
		if (key_tbl == NULL) {
			trace1(TR___get_rpc_hostent_private, 1);
                	return(NULL);
		}
        	if (key_tbl->hostent_p != NULL) {
			trace1(TR___get_rpc_hostent_private, 1);
			return(key_tbl->hostent_p);
		}
	}
#else
	if (rpc_hostent_private != NULL) {
		trace1(TR___get_rpc_hostent_private, 1);
		return(rpc_hostent_private);
	}
#endif /* _REENTRANT */

	rhp = (struct rpc_hostent_private *)
	      calloc(1, sizeof(struct rpc_hostent_private));
	if (rhp == NULL) {
		(void) syslog(LOG_ERR, 
		     gettxt("uxnsl:32", "%s: out of memory"),
		     "gethostent");
		trace1(TR___get_rpc_hostent_private, 1);
		return(NULL);
	}
	rhp->hostf = NULL;
	rhp->host_addrs[0] = rhp->hostaddr;
	rhp->host_addrs[1] = NULL;
#ifdef _REENTRANT
	if (MULTI_THREADED)
		key_tbl->hostent_p = (void *)rhp;
	else
		rpc_hostent_private = rhp;
#else
	rpc_hostent_private = rhp;
#endif /* _REENTRANT */
	trace1(TR___get_rpc_hostent_private, 1);
	return(rhp);
}

#ifdef _REENTRANT

void
_free_rpc_hostent(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

sethostent(f)
	int f;
{
	NS_SETENT(HOSTDB, f);

	return(_sethtent(f));
}

_sethtent(f)
	int f;
{
	struct rpc_hostent_private *rhp;

	trace1(TR_sethtent, 0);
	rhp = _get_rpc_hostent_private();
	if (rhp == NULL) {
		trace1(TR_sethtent, 1);
		return;
	}
	if (rhp->hostf == NULL)
		rhp->hostf = (FILE *)_fopen(_host_file, "r" );	
	else
		rewind(rhp->hostf);
	rhp->stayopen |= f;
	trace1(TR_sethtent, 1);
}

endhostent()
{
	NS_ENDENT(HOSTDB);

	return(_endhtent());
}
_endhtent()
{
	struct rpc_hostent_private *rhp;

	trace1(TR_endhtent, 0);
	rhp = _get_rpc_hostent_private();
	if (rhp == NULL) {
		trace1(TR_endhtent, 1);
		return;
	}
	if (rhp->hostf && !rhp->stayopen) {
		fclose(rhp->hostf);
		rhp->hostf = NULL;
	}
#ifdef havdndbm
	if (_host_db) {
		dbm_close(_host_db);
		_host_db = (DBM *)NULL;
	}
#endif
	trace1(TR_endhtent, 1);
}

struct hostent *
_gethtent()
{
	struct rpc_hostent_private *rhp;
	char *p;
	struct hostent *h;
	register char *cp, **q;
	u_long theaddr;

	trace1(TR_gethtent, 0);
	rhp = _get_rpc_hostent_private();
	if (rhp == NULL) {
		trace1(TR_gethtent, 1);
		return(NULL);
	}
	if (rhp->hostf == NULL
		&& (rhp->hostf = (FILE *)_fopen(_host_file, "r" )) == NULL) {
		trace1(TR_gethtent, 1);
		return (NULL);
	}
again:
	if ((p = fgets(rhp->line, BUFSIZ, rhp->hostf)) == NULL) {
		trace1(TR_gethtent, 1);
		return (NULL);
	}
	if (*p == '#')
		goto again;
	cp = any(p, "#\n");
	if (cp == NULL)
		goto again;
	*cp = '\0';
	cp = any(p, " \t");
	if (cp == NULL)
		goto again;
	*cp++ = '\0';
	/* THIS STUFF IS INTERNET SPECIFIC */
	rhp->host.h_addr_list = rhp->host_addrs;
	theaddr= inet_addr(p);
	bcopy( &theaddr, rhp->host.h_addr_list[0], sizeof (u_long));
	rhp->host.h_length = sizeof (u_long);
	rhp->host.h_addrtype = AF_INET;
	while (*cp == ' ' || *cp == '\t')
		cp++;
	rhp->host.h_name = cp;
	q = rhp->host.h_aliases = rhp->host_aliases;
	cp = any(cp, " \t");
	if (cp != NULL) {
		*cp++ = '\0';
		while (cp && *cp) {
			if (*cp == ' ' || *cp == '\t') {
				cp++;
				continue;
			}
			if (q < &rhp->host_aliases[MAXALIASES - 1])
				*q++ = cp;
			cp = any(cp, " \t");
			if (cp != NULL) 
				*cp++ = '\0';
		}
	}
	*q = NULL;
	trace1(TR_gethtent, 1);
	return (&rhp->host);
}

struct hostent *
gethostent()
{
	NS_GETENT(HOSTDB, struct hostent *);

	return(_gethtent());
}

sethostfile(file)
	char *file;
{
	trace1(TR_sethostfile, 0);
	MUTEX_LOCK(&__rpc_lock);
	_host_file = file;
	MUTEX_UNLOCK(&__rpc_lock);
	trace1(TR_sethostfile, 1);
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

struct hostent *
_gethtbyname(name)
	char *name;
{
	register struct hostent *p;
	register char **cp;
	
	_sethtent(0);
	while (p = _gethtent()) {
		if (strcasecmp(p->h_name, name) == 0)
			break;
		for (cp = p->h_aliases; *cp != 0; cp++)
			if (strcasecmp(*cp, name) == 0)
				goto found;
	}
found:
	_endhtent();
	return (p);
}

struct hostent *
_gethtbyaddr(addr, len, type)
	char *addr;
	int len, type;
{
	register struct hostent *p;

	_sethtent(0);
	while (p = _gethtent())
		if (p->h_addrtype == type && !bcmp(p->h_addr, addr, len))
			break;
	_endhtent();
	return (p);
}

/*
 * This array is designed for mapping upper and lower case letter
 * together for a case independent comparison.  The mappings are
 * based upon ascii character sequences.
 */
static const char charmap[] = {
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

static
strcasecmp(s1, s2)
	register char *s1, *s2;
{
	/* we promise not to change charmap! */
	register char *cm = (char *)charmap;

	while (cm[*s1] == cm[*s2++])
		if (*s1++ == '\0')
			return(0);
	return(cm[*s1] - cm[*--s2]);
}

int
_get_rpc_host_stayopen()
{
	struct rpc_hostent_private	*rhp;

	rhp = _get_rpc_hostent_private();
	if (rhp == NULL)
		return (0);
	return (rhp->stayopen);
}
