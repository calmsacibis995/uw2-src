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

#ident	"@(#)libsocket:common/lib/libsocket/inet/inet_sethost.c	1.1.6.10"
#ident	"$Header: $"

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
#include "../libsock_mt.h"
#include <ns.h>

#ifdef havdndbm
#include <ndbm.h>
#endif

#define bcopy(s1, s2, len)	memcpy(s2, s1, len)

/*
 * Internet version.
 */
#define	MAXALIASES	35
#define	MAXADDRSIZE	14
#define HOSTFILE	"/etc/hosts"

static struct hostinfo {
	FILE *hostf;
	char *_host_file;
	char line[BUFSIZ+1];
	char hostaddr[MAXADDRSIZE];
	struct hostent host;
	char *host_aliases[MAXALIASES];
	char *host_addrs[2];
	int stayopen;
} hostinfo = { NULL, HOSTFILE };

/*
 * Can the following ndbm support come out?  XXX
 * The following is shared with gethostnamadr.c
 */

#ifdef havdndbm
DBM	*_host_db;	/* set by gethostbyname(), gethostbyaddr() */
#endif

static char *any();

static struct hostinfo *
get_s_hostinfo()
{

#ifdef _REENTRANT
        struct _s_tsd *key_tbl;
	struct hostinfo *hip;

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
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return ((struct hostinfo *)NULL);
	if (key_tbl->host_info_p == NULL) {
		hip = (struct hostinfo *)calloc(1, sizeof(struct hostinfo));
		if (hip == NULL)
			return ((struct hostinfo *)NULL);
		key_tbl->host_info_p = hip;
		hip->hostf = NULL;
		hip->_host_file = HOSTFILE;
	}
	
	return ((struct hostinfo *)key_tbl->host_info_p);
#else /* !_REENTRANT */
	return (&hostinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_host_info(p)
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

endhostent()
{
	NS_ENDENT(HOSTDB);
	return (_endhtent());
}

struct hostent *
gethostent()
{
	NS_GETENT(HOSTDB, struct hostent *);
	return ((struct hostent *)_gethtent());
}

sethostfile(file)
	char *file;
{
	struct hostinfo *hip;

	/* Get thread-specific data */
	if ((hip = get_s_hostinfo()) == NULL)
		return;

	hip->_host_file = file;
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
_get_s_host_stayopen()
{
	struct hostinfo	*hip;

	hip = get_s_hostinfo();
	if (hip == NULL)
		return (0);
	return (hip->stayopen);
}
