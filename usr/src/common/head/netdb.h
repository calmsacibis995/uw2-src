/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.usr:netdb.h	1.2.7.6"
#ident  "$Header: $"

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

/*
 * Structures returned by network data base library.
 * All addresses are supplied in host order, and
 * returned in network order (suitable for use in system calls).
 */

#ifndef _NETDB_H
#define _NETDB_H

#if defined(__cplusplus)
extern "C" {
#endif

struct	hostent {
	char	*h_name;	/* official name of host */
	char	**h_aliases;	/* alias list */
	int	h_addrtype;	/* host address type */
	int	h_length;	/* length of address */
	char	**h_addr_list;	/* list of addresses from name server */
#define	h_addr	h_addr_list[0]	/* address, for backward compatiblity */
};

/*
 * Assumption here is that a network number
 * fits in 32 bits -- probably a poor one.
 */
struct	netent {
	char		*n_name;	/* official name of net */
	char		**n_aliases;	/* alias list */
	int		n_addrtype;	/* net address type */
	unsigned long	n_net;		/* network # */
};

struct	servent {
	char	*s_name;	/* official service name */
	char	**s_aliases;	/* alias list */
	int	s_port;		/* port # */
	char	*s_proto;	/* protocol to use */
};

struct	protoent {
	char	*p_name;	/* official protocol name */
	char	**p_aliases;	/* alias list */
	int	p_proto;	/* protocol # */
};

#ifdef __STDC__

extern struct hostent	*gethostbyname(char *);
extern struct hostent	*gethostbyaddr(char *, int, int);
extern struct hostent	*gethostent(void);
extern struct netent	*getnetbyname(char *);
extern struct netent	*getnetbyaddr(long, int);
extern struct netent	*getnetent(void);
extern struct servent	*getservbyname(char *, char *);
extern struct servent	*getservbyport(int, char *);
extern struct servent	*getservent(void);
extern struct protoent	*getprotobyname(char *);
extern struct protoent	*getprotobynumber(int);
extern struct protoent	*getprotoent(void);
extern int		endhostent(void);
extern int		endnetent(void);
extern int		endprotoent(void);
extern int		endservent(void);
extern int		rcmd(char **, unsigned short, char *, char *, char *,
			     int *);
extern int		rexec(char **, int, char *, char *, char *, int *);
extern int		rresvport(int *);
extern int		ruserok(char *, int, char *, char *);
extern int		sethostent(int);
extern int		setnetent(int);
extern int		setprotoent(int);
extern int		setservent(int);
extern int		set_h_errno(int);
extern int		get_h_errno(void);
extern const int	*_h_errno(void);
extern int		ifignore(const char *, const char *);
extern int		getnetgrent(char **, char **, char **);
extern int		setnetgrent(char *);
extern int		endnetgrent(void);
extern int		innetgr(char *, char *, char *, char *);

#else /* ! __STDC__ */

extern struct hostent	*gethostbyname();
extern struct hostent	*gethostbyaddr();
extern struct hostent	*gethostent();
extern struct netent	*getnetbyname();
extern struct netent	*getnetbyaddr();
extern struct netent	*getnetent();
extern struct servent	*getservbyname();
extern struct servent	*getservbyport();
extern struct servent	*getservent();
extern struct protoent	*getprotobyname();
extern struct protoent	*getprotobynumber();
extern struct protoent	*getprotoent();
extern int		endhostent();
extern int		endnetent();
extern int		endprotoent();
extern int		endservent();
extern int		rcmd();
extern int		rexec();
extern int		rresvport();
extern int		ruserok();
extern int		sethostent();
extern int		setnetent();
extern int		setprotoent();
extern int		setservent();
extern int		set_h_errno(int);
extern int		get_h_errno(void);
extern const int	*_h_errno(void);
extern int		set_h_errno();
extern int		get_h_errno();
extern const int	*_h_errno();
extern int		ifignore();
extern int		getnetgrent();
extern int		setgetgrent();
extern int		endnetgrent();
entern int		innetgr();

#endif /* __STDC__ */

#ifdef _REENTRANT

#define h_errno		(*_h_errno())

#else /* ! _REENTRANT */

extern int	h_errno;	

#endif /* _REENTRANT */

/*
 * Error return codes from gethostbyname() and gethostbyaddr()
 * (when using the resolver)
 */

#define	HOST_NOT_FOUND	1 /* Authoritive Answer Host not found */
#define	TRY_AGAIN	2 /* Non-Authoritive Host not found, or SERVERFAIL */
#define	NO_RECOVERY	3 /* Non recoverable errors, FORMERR, REFUSED, NOTIMP */
#define	NO_DATA		4 /* Valid name, no data record of requested type */
#define	NO_ADDRESS	NO_DATA		/* no address, look for MX record */
#define	NO_ERRORMEM	99 /* No memory could be allocated for error variable.*/


#define	MAXHOSTNAMELEN	256

#if defined(__cplusplus)
}
#endif

#endif /*!_NETDB_H*/
