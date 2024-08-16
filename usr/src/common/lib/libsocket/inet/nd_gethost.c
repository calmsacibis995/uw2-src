/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libsocket:common/lib/libsocket/inet/nd_gethost.c	1.4.3.4"
#ident	"$Header: $"

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

#include "../socketabi.h"
#include <sys/param.h>
#include <sys/socket.h>
#include <sys/xti.h>		/* REQUIRED - XPG4 */
#include <netinet/in.h>
#include <ctype.h>
#include <netdb.h> 
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <netdir.h>
#include <sys/byteorder.h>
#include "../libsock_mt.h"

#define	MAXALIASES	35
#define	MAXADDRS	35

#define bcopy(s1, s2, len)	memcpy(s2, s1, len)
#define bzero(s, len)		memset((char *)(s), 0, len)

static const char HOSTDB[] = "/etc/hosts";

static struct ndhostinfo {
	FILE *hostf;
	char *h_addr_ptrs[MAXADDRS + 1];
	struct hostent host;
	char *host_aliases[MAXALIASES + 1];
	char hostbuf[BUFSIZ+1];
	struct in_addr host_addr;
	char hostaddr[MAXADDRS*sizeof(u_long)];
	char *host_addrs[2];
	int stayopen;
} ndhostinfo;

static char *any();


#undef h_errno
int h_errno;

static struct ndhostinfo *
get_s_ndhostinfo()
{

#ifdef _REENTRANT
        struct _s_tsd *key_tbl;
	struct ndhostinfo *nip;

	if (FIRST_OR_NO_THREAD) {
		/*
		 * This is the case of the initial thread or when
		 * libthread is not linked.
		 */
		return (&ndhostinfo);
	} 
	/*
	 * This is the case of threads other than the first.
	 */
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL)
		return ((struct ndhostinfo *)NULL);
	if (key_tbl->ndhost_info_p == NULL) {
		nip = (struct ndhostinfo *)calloc(1, sizeof(struct ndhostinfo));
		if (nip == NULL)
			return ((struct ndhostinfo *)NULL);
		key_tbl->ndhost_info_p = nip;
		nip->hostf = NULL;
	}
	
	return ((struct ndhostinfo *)key_tbl->ndhost_info_p);
#else /* !_REENTRANT */
	return (&ndhostinfo);
#endif /* _REENTRANT */
}

#ifdef _REENTRANT

void
_free_s_ndhost_info(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

int
get_h_errno()
{
#ifdef _REENTRANT
	struct _s_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) return (h_errno);

	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->h_errno_p != NULL)
		return(*(int *)key_tbl->h_errno_p);
	return (NO_ERRORMEM);
#else
	return (h_errno);
#endif /* _REENTRANT */
}

int
set_h_errno(errcode)
	int errcode;
{
#ifdef _REENTRANT
	struct _s_tsd *key_tbl;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) {
		h_errno = errcode;
		return 0;
	}
	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl == NULL) return -1;
	if (key_tbl->h_errno_p == NULL) 
		key_tbl->h_errno_p = calloc(1, sizeof(int));
	if (key_tbl->h_errno_p == NULL) return -1;
	*(int *)key_tbl->h_errno_p = errcode;
#else
	h_errno = errcode;
#endif /* _REENTRANT */
	return 0;
}

#ifdef _REENTRANT

void
_free_s_h_errno(p)
	void *p;
{
	if (FIRST_OR_NO_THREAD) 
		return;
	if (p != NULL)
		free(p);
	return;
}

#endif /* _REENTRANT */

const int *
_h_errno()
{
#ifdef _REENTRANT
	struct _s_tsd *key_tbl;
	static const int __h_errno = NO_ERRORMEM;

	/*
	 * This is the case of the initial thread.
	 */
        if (FIRST_OR_NO_THREAD) return (&h_errno);

	key_tbl = (struct _s_tsd *)
		  _mt_get_thr_specific_storage(_s_key, _S_KEYTBL_SIZE);
	if (key_tbl != NULL && key_tbl->h_errno_p != NULL)
		return((int *)key_tbl->h_errno_p);
	return (&__h_errno);
#else
	return (&h_errno);
#endif /* _REENTRANT */
}

/*
 * Copy the information acquired in the various "netdir" structures to the
 * "struct hostent" format that the user expects.  When done, free the
 * netdir structures and return a pointer to the hostent structure.
 * The hostent information is stored in local static variables, which are
 * overwritten on subsequent invocations.
 *
 * This routine is a utility for gethostbyname() and gethostbyaddr().
 */
static struct hostent *
nd_to_hostent(nconf, naddrs, hservs)
	struct netconfig *nconf;
	struct nd_addrlist *naddrs;
	struct nd_hostservlist *hservs;
{
	register char *cp;
	int m;	/* indexes host/service pairs */
	int n;	/* indexes host aliases */
	int len;
	struct hostent *hp;
	struct nd_hostserv *sp;  /* points to current host/service pair */
	struct nd_hostserv *sp0; /* points to previous host/service pair */
	struct sockaddr_in *sin;
	struct netbuf *na;
	struct ndhostinfo *nip;

	/* Get thread-specific data */
	if ((nip = get_s_ndhostinfo()) == NULL)
		return (NULL);

	/*
	 * Official hostname...
	 */
	hp = &nip->host;
	cp = nip->hostbuf;
	hp->h_name = cp;
	sp = hservs->h_hostservs;
	/* first host/service pair contains host name */
	(void) strcpy(cp, sp->h_host);
	cp += strlen(sp->h_host) + 1;
	/*
	 * Alias(es)...
	 * Take only unique aliases from the remaining host/service pairs!
	 * Assume host/service pairs are grouped by host.
	 */
	hp->h_aliases = nip->host_aliases;
	for (m = 1, n = 0;
	     m < hservs->h_cnt && n < MAXALIASES; 
	     m++) {
		sp0 = sp++;
		if (!strcmp(sp0->h_host, sp->h_host)) {
			continue;	/* do not increment alias counter */
		}
		len = strlen(sp->h_host) + 1;
		if ((cp + len) > (nip->hostbuf + sizeof(nip->hostbuf))) {
			/*
			 * This alias, if added to the list and stored in the
			 * static buffer, would overflow that buffer.  We'll
			 * quit processing alias names, beginning with this
			 * one, to avoid the nasty overflow consequences.
			 */
			break;
		}
		(void) strcpy(cp, sp->h_host);
		nip->host_aliases[n] = cp;
		cp += len;
		n++; 	/* increment alias counter */
	}
	nip->host_aliases[n] = NULL;
	/*
	 * Address(es)...
	 */
	hp->h_addr_list = nip->h_addr_ptrs;
	cp = nip->hostaddr;
	for (n = 0, na = naddrs->n_addrs; 
	     n < naddrs->n_cnt && n < MAXADDRS; n++, na++) {
		sin = (struct sockaddr_in *) na->buf;
		*((u_long *) cp) = sin->sin_addr.s_addr;
		nip->h_addr_ptrs[n] = cp;
		cp += sizeof(u_long);
	}
	nip->h_addr_ptrs[n] = NULL;
	/*
	 * Address length and family...
	 */
	hp->h_length = sizeof(u_long);
	hp->h_addrtype = AF_INET;

	/*
	 * Free netdir structures and return a pointer to the hostent structure.
	 */
	(void) freenetconfigent(nconf);
	(void) netdir_free(naddrs, ND_ADDRLIST);
	(void) netdir_free(hservs, ND_HOSTSERVLIST);

	return (hp);
}

struct hostent *
gethostbyname(name)
	char *name;
{
	register char *cp;
	struct netconfig *nconf;
	struct nd_hostserv serv;
	struct nd_hostservlist *hservs;
	struct nd_addrlist *naddrs;
	struct sockaddr_in sin;

	/*
	 * just in case "name" really is address
	 * (some applications don't check)
	 */
	sin.sin_addr.s_addr = inet_addr(name);
	if ( (long)sin.sin_addr.s_addr != -1 )
		return(gethostbyaddr((char *)&(sin.sin_addr),
			sizeof(sin.sin_addr), AF_INET));

	/*
	 * Use the transport-independent (netdir) interface to obtain the
	 * host information for which the user is asking.  This will allow
	 * the dynamic shared object library to perform the actual name
	 * lookup, which could involve either the local static tables (i.e.,
	 * /etc/hosts) or the domain name server.
	 *
	 * Since the generic interface requires a hostname/service pair, we
	 * will provide an arbitrary port number (ascii string) instead of
	 * a service name, as we really don't care about the port number.
	 * Once we've retrieved the address information for the "name" passed
	 * to us, use the (first) address to obtain the official hostname and
	 * aliases.  Finally, encapsulate all this and return it to the user.
	 */
	if ((nconf = getnetconfigent("tcp")) == NULL &&
	    (nconf = getnetconfigent("udp")) == NULL) {
		/*
		 * Couldn't find an entry in /etc/netconfig for either
		 * Internet protocol, TCP or UDP.
		 */
		set_h_errno(HOST_NOT_FOUND);
		return ((struct hostent *) NULL);
	}
	/*
	 * Use netdir_getbyname() and the netconfig entry returned by the
	 * getnetconfigent() call above to get the address(es) associated
	 * with the argument "name" passed to us.
	 */
	serv.h_host = name;
	serv.h_serv = "111";
	naddrs = (struct nd_addrlist *) NULL;
	set_h_errno(0);
	if (netdir_getbyname(nconf, &serv, &naddrs) != ND_OK ||
	    naddrs == (struct nd_addrlist *) NULL ||
	    naddrs->n_cnt < 1) {
		/*
		 * Hostname lookup failed, so clean up and return failure.
		 */
		(void) freenetconfigent(nconf);
		if (naddrs != (struct nd_addrlist *) NULL) {
			(void) netdir_free(naddrs, ND_ADDRLIST);
		}
		if (get_h_errno() == 0)
			set_h_errno(HOST_NOT_FOUND);
		return ((struct hostent *) NULL);
	}
	/*
	 * Now try to get the official hostname and alias(es) information via
	 * netdir_getbyaddr(), using the first address returned by the
	 * netdir_getbyname() call above.  
	 * If this attempt fails, then just use the address info found above.
	 */
	hservs = (struct nd_hostservlist *) NULL;
	if (netdir_getbyaddr(nconf, &hservs, naddrs->n_addrs) != ND_OK
	 || hservs == (struct nd_hostservlist *) NULL 
	 || hservs->h_cnt < 1) {
		
		/*
		 * Address lookup failed, so convert the address info found
		 * by netdir_getbyname() into an nd_hostservlist.
		 * Make sure that hservs points to a data structure that
		 * netdir_free() can safely free.
		 */

		hservs = (struct nd_hostservlist *)
			 malloc(sizeof (struct nd_hostservlist));
		if (hservs == (struct nd_hostservlist *)NULL) {
			return ((struct hostent *)NULL);
		}

		hservs->h_hostservs = (struct nd_hostserv *)
				      calloc(1, sizeof (struct nd_hostserv));
		if (hservs->h_hostservs == (struct nd_hostserv *)NULL) {
			free((void *)hservs);
			return ((struct hostent *)NULL);
		}

		hservs->h_cnt = 1;
		hservs->h_hostservs->h_host = strdup(serv.h_host);
		hservs->h_hostservs->h_serv = strdup(serv.h_serv);

		/*
		 * Convert host info into a persistent struct hostent and
		 * return its address.
		 */
		return (nd_to_hostent(nconf, naddrs, hservs));
	}

	return (nd_to_hostent(nconf, naddrs, hservs));
}

struct hostent *
gethostbyaddr(addr, len, type)
	char *addr;
	int len, type;
{
	struct netconfig *nconf;
	struct nd_hostservlist *hservs;
	struct nd_addrlist *naddrs;
	struct sockaddr_in sa;
	struct netbuf nb;
	
	if (type != AF_INET || len != sizeof(u_long)) {
		return ((struct hostent *) NULL);
	}

	/*
	 * Use the transport-independent (netdir) interface to obtain the
	 * host information for which the user is asking.  This will allow
	 * the dynamic shared object library to perform the actual name
	 * lookup, which could involve either the local static tables (i.e.,
	 * /etc/hosts) or the domain name server.
	 *
	 * Since the generic interface requires a full sockaddr_in structure,
	 * including port number and Internet address, we will provide an
	 * arbitrary port number.  Once we've retrieved the hostname/alias(es)
	 * information for the "addr" passed to us, use the first hostname to
	 * obtain the full list of addresses (probably only one, but done for
	 * completeness).  Finally, encapsulate this and return it to the user.
	 */
	if ((nconf = getnetconfigent("tcp")) == NULL &&
	    (nconf = getnetconfigent("udp")) == NULL) {
		/*
		 * Couldn't find an entry in /etc/netconfig for either
		 * Internet protocol, TCP or UDP.
		 */
		set_h_errno(HOST_NOT_FOUND);
		return ((struct hostent *) NULL);
	}
	/*
	 * Get the official hostname and alias(es) information via
	 * netdir_getbyaddr().  First, construct a sockaddr_in structure to
	 * contain the address, and a netbuf structure (whose "buf" pointer
	 * points to the sockaddr_in structure just constructed).
	 */
	bzero(&sa, sizeof(sa));
	sa.sin_port = htons(111);
	sa.sin_family = AF_INET;
	bcopy(addr, &sa.sin_addr.s_addr, sizeof(u_long));
	nb.maxlen = sizeof(struct sockaddr_in);
	nb.len = sizeof(struct sockaddr_in);
	nb.buf = (char *) &sa;
	hservs = (struct nd_hostservlist *) NULL;
	if (netdir_getbyaddr(nconf, &hservs, &nb) != ND_OK
	 || hservs == (struct nd_hostservlist *) NULL
	 || hservs->h_cnt < 1) {
		/*
		 * Address lookup failed, so clean up and return failure.
		 */
		(void) freenetconfigent(nconf);
		if (hservs != (struct nd_hostservlist *) NULL) {
			(void) netdir_free(hservs, ND_HOSTSERVLIST);
		}
		if (get_h_errno() == 0)
			set_h_errno(HOST_NOT_FOUND);
		return ((struct hostent *) NULL);
	}
	/*
	 * Now get the address(es) associated with the official hostname
	 * just retrieved via netdir_getbyaddr().  This may be overkill, but
	 * it ensures that ALL addresses are returned to the user, not just
	 * the one they passed to us.
	 */
	naddrs = (struct nd_addrlist *) NULL;
	if (netdir_getbyname(nconf, hservs->h_hostservs, &naddrs) != ND_OK 
	 || naddrs == (struct nd_addrlist *) NULL
	 || naddrs->n_cnt < 1) {
		/*
		 * Hostname lookup failed, so clean up and return failure.
		 */
		(void) freenetconfigent(nconf);
		if (hservs != (struct nd_hostservlist *) NULL) {
			(void) netdir_free(hservs, ND_HOSTSERVLIST);
		}
		if (naddrs != (struct nd_addrlist *) NULL) {
			(void) netdir_free(naddrs, ND_ADDRLIST);
		}
		if (get_h_errno() == 0)
			set_h_errno(HOST_NOT_FOUND);
		return ((struct hostent *) NULL);
	}

	return (nd_to_hostent(nconf, naddrs, hservs));
}

