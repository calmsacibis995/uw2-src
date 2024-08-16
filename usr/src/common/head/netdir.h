/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)head.usr:netdir.h	1.5.8.6"
#ident	"$Header: $"

/*
 * netdir.h
 *
 * This is the include file that defines various structures and
 * constants used by the netdir routines.
 */

#ifndef _NETDIR_H
#define _NETDIR_H
#include <netconfig.h>

#if defined(__cplusplus)
extern "C" {
#endif

#ifdef _REENTRANT

#define _nderror (*_netdir_nderror())

#else /* !_REENTRANT */

extern int _nderror;

#endif /* _REENTRANT */

struct nd_addrlist {
	int 		n_cnt;		/* number of netbufs */
	struct netbuf 	*n_addrs;	/* the netbufs */
};

struct nd_hostservlist {
	int			h_cnt;		/* number of nd_hostservs */
	struct nd_hostserv	*h_hostservs;	/* the entries */
};

struct nd_hostserv {
	char		*h_host;	/* the host name */
	char		*h_serv;	/* the service name */
};

struct nd_mergearg {
	char		*s_uaddr;	/* servers universal address */
	char		*c_uaddr;	/* clients universal address */
	char		*m_uaddr;	/* merged universal address */
};

#ifdef __STDC__		/* ANSI */

struct netconfig ;

int netdir_options(const struct netconfig *, int option, int fd, const char *par);
int netdir_getbyname(const struct netconfig *, const struct nd_hostserv *, struct nd_addrlist **);
int netdir_getbyaddr(const struct netconfig *, struct nd_hostservlist **, const struct netbuf *);
void netdir_free(void *, int);
struct netbuf *uaddr2taddr(const struct netconfig *, const char *);
char *taddr2uaddr(const struct netconfig *, const struct netbuf *);
void netdir_perror(const char *);
char *netdir_sperror();
struct nd_addrlist *_netdir_getbyname(const struct netconfig *, const struct nd_hostserv *);
struct nd_hostservlist *_netdir_getbyaddr(const struct netconfig *, const struct netbuf *);
struct netbuf *_uaddr2taddr(const struct netconfig *, const char *);
char *_taddr2uaddr(const struct netconfig *, const struct netbuf *);
int set_nderror(int);
int get_nderror();
const int *_netdir_nderror();

#else

int netdir_options();
int netdir_getbyname();
int netdir_getbyaddr();
void netdir_free();
struct netbuf *uaddr2taddr();
void netdir_perror();
char *netdir_sperror();
char *taddr2uaddr();
struct nd_addrlist *_netdir_getbyname();
struct nd_hostservlist *_netdir_getbyaddr();
struct netbuf *_uaddr2taddr();
char *_taddr2uaddr();
int set_nderror();
int get_nderror();
const int *_netdir_nderror();

#endif /* ANSI */

/*
 * These are all objects that can be freed by netdir_free
 */
#define ND_HOSTSERV	0
#define ND_HOSTSERVLIST	1
#define ND_ADDR		2
#define ND_ADDRLIST	3

/* 
 * These are the various errors that can be encountered while attempting
 * to translate names to addresses. Note that none of them (except maybe
 * no memory) are truely fatal unless the ntoa deamon is on its last attempt
 * to translate the name. 
 *
 * Negative errors terminate the search resolution process, positive errors
 * are treated as warnings.
 */

#define ND_BADARG	(-2)	/* Bad arguments passed 	*/
#define ND_NOMEM 	(-1)	/* No virtual memory left	*/
#define ND_OK		0	/* Translation successful	*/
#define ND_NOHOST	1	/* Hostname was not resolvable	*/
#define ND_NOSERV	2	/* Service was unknown		*/
#define ND_NOSYM	3	/* Couldn't resolve symbol	*/
#define ND_OPEN		4	/* File couldn't be opened	*/
#define ND_ACCESS	5	/* File is not accessable	*/
#define ND_UKNWN	6	/* Unknown object to be freed	*/
#define ND_NOCTRL	7       /* Unknown option passed to netdir_options */
#define ND_FAILCTRL	8       /* Option failed in netdir_options */
#define ND_SYSTEM	9       /* Other System error           */
#define ND_NOERRMEM	10      /* No memory for error variable */
#define ND_NOLIB	11	/* No library in netconfig list was found */
#define ND_XTIERROR	12	/* An xti call failed; check get_t_errno  */
#define ND_BADSTATE	13	/* Incorrect state to attempt t_bind      */

/*
 * The following netdir_options commands can be given to the fd. This is
 * a way of providing for any transport-specific action which the caller
 * may want to initiate. It is up to the transport provider
 * to choose which netdir options to support.
 */

#define ND_SET_BROADCAST      1   /* Do t_optmgmt to support broadcast*/
#define ND_SET_RESERVEDPORT   2   /* bind to reserve address */
#define ND_CHECK_RESERVEDPORT 3   /* check if address is reserved */
#define ND_MERGEADDR 	      4   /* Merge universal address        */
#define ND_CLEAR_BROADCAST    5   /* Do t_optmgmt to prevent broadcast*/
#define ND_SET_REUSEADDR      6   /* Do t_optmgmt to support address reuse*/
#define ND_CLEAR_REUSEADDR    7   /* Do t_optmgmt to prevent address reuse*/

/*
 *	The following special host names are used to give the underlying
 *	transport provider a clue as to the intent of the request.
 */

#define	HOST_SELF	"\\1"
#define	HOST_ANY	"\\2"
#define	HOST_BROADCAST	"\\3"

#if defined(__cplusplus)
}
#endif

#endif /* !_NETDIR_H */
