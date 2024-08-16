/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)rpcinfo:rpcinfo.c	1.7.11.7"
#ident	"$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*       PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*       Copyright Notice
*
* Notice of copyright on this source code product does not indicate
*  publication.
*
*       (c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*       (c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*       (c) 1990,1991,1992  UNIX System Laboratories, Inc.
*          All rights reserved.
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
 */

/*
 * Copyright (c) 1986 - 1991 by Sun Microsystems, Inc.
 */

/*
 * rpcinfo: ping a particular rpc program
 * 	or dump the the registered programs on the remote machine.
 */

/*
 * For now we are defining PORTMAP here.  This file doesn't even compile
 * unless PORTMAP is defined.
 */
#ifndef	PORTMAP
#define	PORTMAP
#endif

/*
 * If PORTMAP is defined, rpcinfo will talk to both portmapper and
 * rpcbind programs; else it talks only to rpcbind. In the latter case
 * all the portmapper specific options such as -u, -t, -p become void.
 */
#include <rpc/rpc.h>
#include <stdio.h>
#include <rpc/rpcb_prot.h>
#include <rpc/nettype.h>
#include <netdir.h>
#include <libgen.h>
#include <pfmt.h>
#include <locale.h>
#include <rpc/rpcent.h>
#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#ifdef PORTMAP		/* Support for version 2 portmapper */
#include <netinet/in.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <rpc/pmap_prot.h>
#include <rpc/pmap_clnt.h>
#endif

#define	MAXHOSTLEN	256
#define	MIN_VERS	((u_long) 0)
#define	MAX_VERS	((u_long) 4294967295L)
#define	UNKNOWN		"unknown"

#define	MAX(a, b) (((a) > (b)) ? (a) : (b))

extern int	t_errno;
extern long	strtol();

#ifdef PORTMAP
static void	ip_ping(/*u_short portflag, char *trans,
				int argc, char **argv*/);
static CLIENT	*clnt_com_create(/* struct sockaddr_in *addr, long prog,
			long vers, int *fd, char *trans*/);
static void	pmapdump(/*int argc, char **argv*/);
static void	get_inet_address(/*struct sockaddr_in *addr, char *host*/);
#endif

static bool_t	reply_proc(/*void *res, struct netbuf *who*,
			struct netconfig *nconf*/);
static void	brdcst(/*int argc, char **argv*/);
static void	addrping(/*char *address, char *netid,
				int argc, char **argv*/);
static void	progping(/* char *netid, int argc, char **argv*/);
static CLIENT	*clnt_addr_create(/* char *addr, struct netconfig *nconf,
				long prog, long vers*/);
static CLIENT   *clnt_rpcbind_create(/* char *host, int vers */);
static CLIENT   *getclnthandle(/* host, nconf, rpcbversnum */);
static int	pstatus(/*CLIENT *client, u_long prognum, u_long vers*/);
static void	rpcbdump(/*char *netid, int argc, char **argv*/);
static void	rpcbgetstat(/* int argc, char **argv*/);
static void	rpcbaddrlist(/*char *netid, int argc, char **argv*/);
static void	deletereg(/*char *netid, int argc, char **argv */);
static void	print_rmtcallstat(/* rtype, infp */);
static void	print_getaddrstat(/* rtype, infp */);
static void	usage(/*void*/);
static u_long	getprognum(/*char *arg*/);
static u_long	getvers(/*char *arg*/);
static char	*owner(/*char *arg*/);

/*
 * Functions to be performed.
 */
#define	NONE		0	/* no function */
#define	PMAPDUMP	1	/* dump portmapper registrations */
#define	TCPPING		2	/* ping TCP service */
#define	UDPPING		3	/* ping UDP service */
#define	BROADCAST	4	/* ping broadcast service */
#define	DELETES		5	/* delete registration for the service */
#define	ADDRPING	6	/* pings at the given address */
#define	PROGPING	7	/* pings a program on a given host */
#define	RPCBDUMP	8	/* dump rpcbind registrations */
#define	RPCBDUMP_SHORT	9	/* dump rpcbind registrations - short version */
#define	RPCBADDRLIST	10	/* dump addr list about one prog */
#define	RPCBGETSTAT	11	/* Get statistics */

struct netidlist {
	char *netid;
	struct netidlist *next;
};

struct verslist {
	int vers;
	struct verslist *next;
};

struct rpcbdump_short {
	u_long prog;
	struct verslist *vlist;
	struct netidlist *nlist;
	struct rpcbdump_short *next;
	char *owner;
};


static char label[64];
static char *rpcicmd;
static char errmsg[BUFSIZ];

int
main(argc, argv)
	int argc;
	char **argv;
{
	register int c;
	extern char *optarg;
	extern int optind;
	int errflg;
	int function;
	char *netid = NULL;
	char *address = NULL;
#ifdef PORTMAP
	char *strptr;
	u_short portnum = 0;
#endif

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxrpcinfo");

	rpcicmd = basename(argv[0]);
	(void)strcpy(label, "UX:");
	(void)strcat(label, rpcicmd);
	(void)setlabel(label);

	function = NONE;
	errflg = 0;

#ifdef PORTMAP
	while ((c = getopt(argc, argv, "a:bdlmn:pstT:u")) != EOF) {
#else
	while ((c = getopt(argc, argv, "a:bdlmn:sT:")) != EOF) {
#endif
		switch (c) {
#ifdef PORTMAP
		case 'p':
			if (function != NONE)
				errflg = 1;
			else
				function = PMAPDUMP;
			break;

		case 't':
			if (function != NONE)
				errflg = 1;
			else
				function = TCPPING;
			break;

		case 'u':
			if (function != NONE)
				errflg = 1;
			else
				function = UDPPING;
			break;

		case 'n':
			portnum = (u_short) strtol(optarg, &strptr, 10);
			if (strptr == optarg || *strptr != '\0') {
				pfmt(stderr, MM_ERROR,
				     ":14:%s is an illegal port number\n",
				     optarg);
				exit(1);
			}
			break;
#endif
		case 'a':
			address = optarg;
			if (function != NONE)
				errflg = 1;
			else
				function = ADDRPING;
			break;
		case 'b':
			if (function != NONE)
				errflg = 1;
			else
				function = BROADCAST;
			break;

		case 'd':
			if (function != NONE)
				errflg = 1;
			else
				function = DELETES;
			break;

		case 'l':
			if (function != NONE)
				errflg = 1;
			else
				function = RPCBADDRLIST;
			break;

		case 'm':
			if (function != NONE)
				errflg = 1;
			else
				function = RPCBGETSTAT;
			break;

		case 's':
			if (function != NONE)
				errflg = 1;
			else
				function = RPCBDUMP_SHORT;
			break;

		case 'T':
			netid = optarg;
			break;
		case '?':
			errflg = 1;
			break;
		}
	}

	if (errflg || ((function == ADDRPING) && !netid)) {
		usage();
		return (1);
	}
	if (function == NONE) {
		if (argc - optind > 1)
			function = PROGPING;
		else
			function = RPCBDUMP;
	}

	switch (function) {
#ifdef PORTMAP
	case PMAPDUMP:
		if (portnum != 0) {
			usage();
			return (1);
		}
		pmapdump(argc - optind, argv + optind);
		break;

	case UDPPING:
		ip_ping(portnum, "udp", argc - optind, argv + optind);
		break;

	case TCPPING:
		ip_ping(portnum, "tcp", argc - optind, argv + optind);
		break;
#endif
	case BROADCAST:
		brdcst(argc - optind, argv + optind);
		break;
	case DELETES:
		deletereg(netid, argc - optind, argv + optind);
		break;
	case ADDRPING:
		addrping(address, netid, argc - optind, argv + optind);
		break;
	case PROGPING:
		progping(netid, argc - optind, argv + optind);
		break;
	case RPCBDUMP:
	case RPCBDUMP_SHORT:
		rpcbdump(function, netid, argc - optind, argv + optind);
		break;
	case RPCBGETSTAT:
		rpcbgetstat(argc - optind, argv + optind);
		break;
	case RPCBADDRLIST:
		rpcbaddrlist(netid, argc - optind, argv + optind);
		break;
	}
	return (0);
}

#ifdef PORTMAP
static CLIENT *
clnt_com_create(addr, prog, vers, fdp, trans)
	struct sockaddr_in *addr;
	u_long prog;
	u_long vers;
	int *fdp;
	char *trans;
{
	CLIENT *clnt;

	if (strcmp(trans, "tcp") == 0) {
		clnt = clnttcp_create(addr, prog, vers, fdp, 0, 0);
	} else {
		struct timeval to;

		to.tv_sec = 5;
		to.tv_usec = 0;
		clnt = clntudp_create(addr, prog, vers, to, fdp);
	}
	if (clnt == (CLIENT *)NULL) {
		clnt_pcreateerror(rpcicmd);
		if (vers == MIN_VERS)
			pfmt(stderr, MM_ERROR,
			     ":34:Program %lu is not available.\n", prog);
		else
			pfmt(stderr, MM_ERROR,
			     ":35:Program %lu version %lu is not available",
			     prog, vers);
			fprintf(stderr, ".\n");
		exit(1);
	}
	return (clnt);
}

/*
 * If portnum is 0, then go and get the address from portmapper, which happens
 * transparently through clnt*_create(); If version number is not given, it
 * tries to find out the version number by making a call to version 0 and if
 * that fails, it obtains the high order and the low order version number. If
 * version 0 calls succeeds, it tries for MAXVERS call and repeats the same.
 */
static void
ip_ping(portnum, trans, argc, argv)
	u_short portnum;
	char *trans;
	int argc;
	char **argv;
{
	CLIENT *client;
	int fd = RPC_ANYFD;
	struct timeval to;
	struct sockaddr_in addr;
	enum clnt_stat rpc_stat;
	u_long prognum, vers, minvers, maxvers;
	struct rpc_err rpcerr;
	int failure = 0;

	if (argc < 2 || argc > 3) {
		usage();
		exit(1);
	}
	to.tv_sec = 10;
	to.tv_usec = 0;
	prognum = getprognum(argv[1]);
	get_inet_address(&addr, argv[0]);
	if (argc == 2) {	/* Version number not known */
		/*
		 * A call to version 0 should fail with a program/version
		 * mismatch, and give us the range of versions supported.
		 */
		vers = MIN_VERS;
	} else {
		vers = getvers(argv[2]);
	}
	addr.sin_port = htons(portnum);
	client = clnt_com_create(&addr, prognum, vers, &fd, trans);
	rpc_stat = CLNT_CALL(client, NULLPROC, (xdrproc_t) xdr_void,
			(char *)NULL, (xdrproc_t) xdr_void, (char *)NULL,
			to);
	if (argc != 2) {
		/* Version number was known */
		if (pstatus(client, prognum, vers) < 0)
			exit(1);
		(void) CLNT_DESTROY(client);
		return;
	}
	/* Version number not known */
	(void) CLNT_CONTROL(client, CLSET_FD_NCLOSE, (char *)NULL);
	if (rpc_stat == RPC_PROGVERSMISMATCH) {
		clnt_geterr(client, &rpcerr);
		minvers = rpcerr.re_vers.low;
		maxvers = rpcerr.re_vers.high;
	} else if (rpc_stat == RPC_SUCCESS) {
		/*
		 * Oh dear, it DOES support version 0.
		 * Let's try version MAX_VERS.
		 */
		(void) CLNT_DESTROY(client);
		addr.sin_port = htons(portnum);
		client = clnt_com_create(&addr, prognum, MAX_VERS, &fd, trans);
		rpc_stat = CLNT_CALL(client, NULLPROC, (xdrproc_t) xdr_void,
				(char *)NULL, (xdrproc_t) xdr_void,
				(char *)NULL, to);
		if (rpc_stat == RPC_PROGVERSMISMATCH) {
			clnt_geterr(client, &rpcerr);
			minvers = rpcerr.re_vers.low;
			maxvers = rpcerr.re_vers.high;
		} else if (rpc_stat == RPC_SUCCESS) {
			/*
			 * It also supports version MAX_VERS.
			 * Looks like we have a wise guy.
			 * OK, we give them information on all
			 * 4 billion versions they support...
			 */
			minvers = 0;
			maxvers = MAX_VERS;
		} else {
			(void) pstatus(client, prognum, MAX_VERS);
			exit(1);
		}
	} else {
		(void) pstatus(client, prognum, (u_long)0);
		exit(1);
	}
	(void) CLNT_DESTROY(client);
	for (vers = minvers; vers <= maxvers; vers++) {
		addr.sin_port = htons(portnum);
		client = clnt_com_create(&addr, prognum, vers, &fd, trans);
		rpc_stat = CLNT_CALL(client, NULLPROC, (xdrproc_t) xdr_void,
				(char *)NULL, (xdrproc_t) xdr_void,
				(char *)NULL, to);
		if (pstatus(client, prognum, vers) < 0)
				failure = 1;
		(void) CLNT_DESTROY(client);
	}
	if (failure)
		exit(1);
	(void) t_close(fd);
	return;
}

/*
 * Dump all the portmapper registerations
 */
static void
pmapdump(argc, argv)
	int argc;
	char **argv;
{
	struct sockaddr_in server_addr;
	pmaplist_ptr head = NULL;
	int socket = RPC_ANYSOCK;
	struct timeval minutetimeout;
	register CLIENT *client;
	struct rpcent *rpc;
	enum clnt_stat clnt_st;
	struct rpc_err err;
	struct utsname utsname;
	char *host;

	if (argc > 1) {
		usage();
		exit(1);
	}
	if (argc == 1) {
		host = argv[0];
		get_inet_address(&server_addr, host);
	} else {
		uname(&utsname);
		host = utsname.nodename;
		get_inet_address(&server_addr, host);
	}
	minutetimeout.tv_sec = 60;
	minutetimeout.tv_usec = 0;
	server_addr.sin_port = htons(PMAPPORT);
	if ((client = clnttcp_create(&server_addr, PMAPPROG,
		PMAPVERS, &socket, 50, 500)) == NULL) {
		strcpy(errmsg,
		       clnt_spcreateerror(
			       gettxt(":23", "Could not contact %s: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, "portmapper");
		exit(1);
	}
	clnt_st = CLNT_CALL(client, PMAPPROC_DUMP, (xdrproc_t) xdr_void,
		NULL, (xdrproc_t) xdr_pmaplist_ptr, (char *)&head,
		minutetimeout);
	if (clnt_st != RPC_SUCCESS) {
		if ((clnt_st == RPC_PROGVERSMISMATCH) ||
		    (clnt_st == RPC_PROGUNAVAIL)) {
			CLNT_GETERR(client, &err);
			if (err.re_vers.low > PMAPVERS)
				pfmt(stderr, MM_ERROR,
			    ":12:%s does not support %s.  Try %s %s instead.\n",
				     host, "portmapper", rpcicmd, host);
			exit(1);
		}
		strcpy(errmsg,
		       clnt_sperror(client,
			       gettxt(":23", "Could not contact %s: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, "portmapper");
		exit(1);
	}
	if (head == NULL) {
		pfmt(stderr, MM_ERROR,
		     ":32:No remote programs are registered.\n");
	} else {
		pfmt(stdout, MM_NOSTD,
		     ":9:   program version protocol port  service\n");
		for (; head != NULL; head = head->pml_next) {
			printf("%10ld%5ld",
				head->pml_map.pm_prog,
				head->pml_map.pm_vers);
			if (head->pml_map.pm_prot == IPPROTO_UDP)
				printf("%10s", "udp");
			else if (head->pml_map.pm_prot == IPPROTO_TCP)
				printf("%10s", "tcp");
			else
				printf("%6ld", head->pml_map.pm_prot);
			printf("%7ld", head->pml_map.pm_port);
			rpc = getrpcbynumber(head->pml_map.pm_prog);
			if (rpc)
				printf("  %s\n", rpc->r_name);
			else
				printf("\n");
		}
	}
}

static void
get_inet_address(addr, host)
	struct sockaddr_in *addr;
	char *host;
{
	struct netconfig *nconf;
	struct nd_hostserv service;
	struct nd_addrlist *naddrs;

	(void) memset((char *)addr, 0, sizeof (*addr));
	addr->sin_addr.s_addr = inet_addr(host);
	if (addr->sin_addr.s_addr == -1 || addr->sin_addr.s_addr == 0) {
		if ((nconf = _rpc_getconfip("udp")) == NULL &&
		    (nconf = _rpc_getconfip("tcp")) == NULL) {
			pfmt(stderr, MM_ERROR,
			     ":28:Could not find a suitable transport\n");
			exit(1);
		} else {
			service.h_host = host;
			service.h_serv = "rpcbind";
			if (netdir_getbyname(nconf, &service, &naddrs)) {
				pfmt(stderr, MM_ERROR|MM_NOGET, "%s: %s\n",
				     host, netdir_sperror());
				exit(1);
			} else {
				(void) memcpy((caddr_t)addr,
				    naddrs->n_addrs->buf, naddrs->n_addrs->len);
				(void) netdir_free((char *)naddrs, ND_ADDRLIST);
			}
			(void) freenetconfigent(nconf);
		}
	} else {
		addr->sin_family = AF_INET;
	}
}
#endif /* PORTMAP */

/*
 * reply_proc collects replies from the broadcast.
 * to get a unique list of responses the output of rpcinfo should
 * be piped through sort(1) and then uniq(1).
 */

/*ARGSUSED*/
static bool_t
reply_proc(res, who, nconf)
	void *res;		/* Nothing comes back */
	struct netbuf *who;	/* Who sent us the reply */
	struct netconfig *nconf; /* On which transport the reply came */
{
	struct nd_hostservlist *serv;
	char *uaddr;
	char *hostname;

	if (netdir_getbyaddr(nconf, &serv, who)) {
		hostname = UNKNOWN;
	} else {
		hostname = serv->h_hostservs->h_host;
	}
	if (!(uaddr = taddr2uaddr(nconf, who))) {
		uaddr = UNKNOWN;
	}
	printf("%s\t%s\n", uaddr, hostname);
	if (strcmp(hostname, UNKNOWN))
		netdir_free((char *)serv, ND_HOSTSERVLIST);
	if (strcmp(uaddr, UNKNOWN))
		free((char *)uaddr);
	return (FALSE);
}

static void
brdcst(argc, argv)
	int argc;
	char **argv;
{
	enum clnt_stat rpc_stat;
	u_long prognum, vers;

	if (argc != 2) {
		usage();
		exit(1);
	}
	prognum = getprognum(argv[0]);
	vers = getvers(argv[1]);
	rpc_stat = rpc_broadcast(prognum, vers, NULLPROC,
		(xdrproc_t) xdr_void, (char *)NULL, (xdrproc_t) xdr_void,
		(char *)NULL, (resultproc_t) reply_proc, NULL);
	if ((rpc_stat != RPC_SUCCESS) && (rpc_stat != RPC_TIMEDOUT)) {
		pfmt(stderr, MM_ERROR, ":21:Broadcast failed: %s\n",
		     clnt_sperrno(rpc_stat));
		exit(1);
	}
	exit(0);
}

static bool_t
add_version(rs, vers)
	struct rpcbdump_short *rs;
	u_long vers;
{
	struct verslist *vl;

	for (vl = rs->vlist; vl; vl = vl->next)
		if (vl->vers == vers)
			break;
	if (vl)
		return (TRUE);
	vl = (struct verslist *)malloc(sizeof (struct verslist));
	if (vl == NULL)
		return (FALSE);
	vl->vers = vers;
	vl->next = rs->vlist;
	rs->vlist = vl;
	return (TRUE);
}

static bool_t
add_netid(rs, netid)
	struct rpcbdump_short *rs;
	char *netid;
{
	struct netidlist *nl;

	for (nl = rs->nlist; nl; nl = nl->next)
		if (strcmp(nl->netid, netid) == 0)
			break;
	if (nl)
		return (TRUE);
	nl = (struct netidlist *)malloc(sizeof (struct netidlist));
	if (nl == NULL)
		return (FALSE);
	nl->netid = netid;
	nl->next = rs->nlist;
	rs->nlist = nl;
	return (TRUE);
}

static void
rpcbdump(dumptype, netid, argc, argv)
	int dumptype;
	char *netid;
	int argc;
	char **argv;
{
	rpcblist_ptr head = NULL;
	struct timeval minutetimeout;
	register CLIENT *client;
	struct rpcent *rpc;
	char *host;
	struct netidlist *nl;
	struct verslist *vl;
	struct rpcbdump_short *rs, *rs_tail;
	char buf[256];
	enum clnt_stat clnt_st;
	struct rpc_err err;
	struct utsname utsname;
	struct rpcbdump_short *rs_head = NULL;

	if (argc > 1) {
		usage();
		exit(1);
	}
	if (argc == 1) {
		host = argv[0];
	} else {
		uname(&utsname);
		host = utsname.nodename;
	}
	if (netid == NULL) {
		client = clnt_rpcbind_create(host, RPCBVERS, NULL);
	} else {
		struct netconfig *nconf;

		nconf = getnetconfigent(netid);
		client = getclnthandle(host, nconf, RPCBVERS, NULL);
		if (nconf)
			(void) freenetconfigent(nconf);
	}
	if (client == (CLIENT *)NULL) {
		strcpy(errmsg,
		       clnt_spcreateerror(
			       gettxt(":23", "Could not contact %s: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, "rpcbind");
		exit(1);
	}
	minutetimeout.tv_sec = 60;
	minutetimeout.tv_usec = 0;
	clnt_st = CLNT_CALL(client, RPCBPROC_DUMP, (xdrproc_t) xdr_void,
		NULL, (xdrproc_t) xdr_rpcblist_ptr, (char *) &head,
		minutetimeout);
	if (clnt_st != RPC_SUCCESS) {
		if ((clnt_st == RPC_PROGVERSMISMATCH) ||
		    (clnt_st == RPC_PROGUNAVAIL)) {
			CLNT_GETERR(client, &err);
			if (err.re_vers.high == PMAPVERS)
				pfmt(stderr, MM_ERROR,
			 ":13:%s does not support %s.  Try %s %s %s instead.\n",
				     host, "rpcbind", rpcicmd, "-p", host);
			exit(1);
		}
		strcpy(errmsg,
		       clnt_sperror(client,
			       gettxt(":23", "Could not contact %s: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, "rpcbind");
		exit(1);
	}
	if (head == NULL) {
		pfmt(stderr, MM_ERROR,
		     ":32:No remote programs are registered.\n");
	} else if (dumptype == RPCBDUMP) {
		pfmt(stdout, MM_NOSTD,
     ":10:   program version transport address             service    owner\n");
		for (; head != NULL; head = head->rpcb_next) {
			printf("%10ld%5ld    ",
				head->rpcb_map.r_prog, head->rpcb_map.r_vers);
			printf("%-9s ", head->rpcb_map.r_netid);
			printf("%-19s", head->rpcb_map.r_addr);
			rpc = getrpcbynumber(head->rpcb_map.r_prog);
			if (rpc)
				printf(" %-10s", rpc->r_name);
			else
				printf(" %-10s", "-");
			printf(" %s\n", owner(head->rpcb_map.r_owner));
		}
	} else if (dumptype == RPCBDUMP_SHORT) {
		for (; head != NULL; head = head->rpcb_next) {
			for (rs = rs_head; rs; rs = rs->next)
				if (head->rpcb_map.r_prog == rs->prog)
					break;
			if (rs == NULL) {
				rs = (struct rpcbdump_short *)
					malloc(sizeof (struct rpcbdump_short));
				if (rs == NULL)
					goto error;
				rs->next = NULL;
				if (rs_head == NULL) {
					rs_head = rs;
					rs_tail = rs;
				} else {
					rs_tail->next = rs;
					rs_tail = rs;
				}
				rs->prog = head->rpcb_map.r_prog;
				rs->owner = head->rpcb_map.r_owner;
				rs->nlist = NULL;
				rs->vlist = NULL;
			}
			if (add_version(rs, head->rpcb_map.r_vers) == FALSE)
				goto error;
			if (add_netid(rs, head->rpcb_map.r_netid) == FALSE)
				goto error;
		}
		pfmt(stdout, MM_NOSTD, ":11:   program version(s) transport(s)                     service     owner\n");
		for (rs = rs_head; rs; rs = rs->next) {
			char *p = buf;

			printf("%10ld  ", rs->prog);
			for (vl = rs->vlist; vl; vl = vl->next) {
				sprintf(p, "%d", vl->vers);
				p = p + strlen(p);
				if (vl->next)
					sprintf(p++, ",");
			}
			printf("%-10s", buf);
			buf[0] = NULL;
			for (nl = rs->nlist; nl; nl = nl->next) {
				strcat(buf, nl->netid);
				if (nl->next)
					strcat(buf, ",");
			}
			printf("%-32s", buf);
			rpc = getrpcbynumber(rs->prog);
			if (rpc)
				printf(" %-11s", rpc->r_name);
			else
				printf(" %-11s", "-");
			printf(" %s\n", owner(rs->owner));
		}
	}
	clnt_destroy(client);
	return;
error:	pfmt(stderr, MM_ERROR, ":31:No memory\n");
	return;
}

static char nullstring[] = "\000";

static void
rpcbaddrlist(netid, argc, argv)
	char *netid;
	int argc;
	char **argv;
{
	rpcb_entry_list_ptr head = NULL;
	struct timeval minutetimeout;
	register CLIENT *client;
	struct rpcent *rpc;
	char *host;
	RPCB parms;
	struct netbuf *targaddr;

	if (argc != 3) {
		usage();
		exit(1);
	}
	host = argv[0];
	if (netid == NULL) {
		client = clnt_rpcbind_create(host, RPCBVERS4, &targaddr);
	} else {
		struct netconfig *nconf;

		nconf = getnetconfigent(netid);
		client = getclnthandle(host, nconf, RPCBVERS4, &targaddr);
		if (nconf)
			(void) freenetconfigent(nconf);
	}
	if (client == (CLIENT *)NULL) {
		strcpy(errmsg,
		       clnt_spcreateerror(
			       gettxt(":23", "Could not contact %s: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, "rpcbind");
		exit(1);
	}
	minutetimeout.tv_sec = 60;
	minutetimeout.tv_usec = 0;

	parms.r_prog = 	getprognum(argv[1]);
	parms.r_vers = 	getvers(argv[2]);
	parms.r_netid = client->cl_netid;
	if (targaddr == NULL) {
		parms.r_addr = nullstring;	/* for XDRing */
	} else {
		/*
		 * We also send the remote system the address we
		 * used to contact it in case it can help it
		 * connect back with us
		 */
		struct netconfig *nconf;

		nconf = getnetconfigent(client->cl_netid);
		if (nconf != NULL) {
			parms.r_addr = taddr2uaddr(nconf, targaddr);
			if (parms.r_addr == NULL)
				parms.r_addr = nullstring;
			freenetconfigent(nconf);
		} else {
			parms.r_addr = nullstring;	/* for XDRing */
		}
		free(targaddr->buf);
		free(targaddr);
	}
	parms.r_owner = nullstring;

	if (CLNT_CALL(client, RPCBPROC_GETADDRLIST, (xdrproc_t) xdr_rpcb,
		(char *) &parms, (xdrproc_t) xdr_rpcb_entry_list_ptr,
		(char *) &head, minutetimeout) != RPC_SUCCESS) {
		strcpy(errmsg,
		       clnt_sperror(client,
			       gettxt(":23", "Could not contact %s: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, "rpcbind");
		exit(1);
	}
	if (head == NULL) {
		pfmt(stderr, MM_ERROR,
		     ":32:No remote programs are registered.\n");
	} else {
		pfmt(stdout, MM_NOSTD,
        ":8:   program version tp_family/name/class    address\t\t  service\n");
		for (; head != NULL; head = head->rpcb_entry_next) {
			rpcb_entry *re;
			char buf[128];

			re = &head->rpcb_entry_map;
			printf("%10ld%5ld   ",
				parms.r_prog, parms.r_vers);
			sprintf(buf, "%s/%s/%s",
				re->r_nc_protofmly, re->r_nc_proto,
				re->r_nc_semantics == NC_TPI_CLTS ? "clts" :
				re->r_nc_semantics == NC_TPI_COTS ? "cots" :
						"cots_ord");
			printf(" %-23s", buf);
			printf(" %-22s", re->r_maddr);
			rpc = getrpcbynumber(parms.r_prog);
			if (rpc)
				printf(" %-11s", rpc->r_name);
			else
				printf(" %-11s", "-");
			printf("\n");
		}
	}
	clnt_destroy(client);
	return;
}

/*
 * monitor rpcbind
 */
static void
rpcbgetstat(argc, argv)
	int argc;
	char **argv;
{
	rpcb_stat_byvers inf;
	struct timeval minutetimeout;
	register CLIENT *client;
	char *host;
	int i, j;
	rpcbs_addrlist *pa;
	rpcbs_rmtcalllist *pr;
	int cnt;
	struct utsname utsname;

	if (argc >= 1) {
		host = argv[0];
	} else {
		uname(&utsname);
		host = utsname.nodename;
	}
	client = clnt_rpcbind_create(host, RPCBVERS4, NULL);
	if (client == (CLIENT *)NULL) {
		strcpy(errmsg,
		       clnt_spcreateerror(
			       gettxt(":23", "Could not contact %s: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, "rpcbind");
		exit(1);
	}
	minutetimeout.tv_sec = 60;
	minutetimeout.tv_usec = 0;
	memset((char *)&inf, 0, sizeof (rpcb_stat_byvers));
	if (CLNT_CALL(client, RPCBPROC_GETSTAT, (xdrproc_t) xdr_void, NULL,
		(xdrproc_t) xdr_rpcb_stat_byvers, (char *)&inf, minutetimeout)
			!= RPC_SUCCESS) {
		strcpy(errmsg,
		       clnt_sperror(client,
			       gettxt(":23", "Could not contact %s: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, "rpcbind");
		exit(1);
	}
	printf("PORTMAP (version 2) statistics\n");
	pfmt(stdout, MM_NOSTD,
	     ":33:%s (version %s) statistics\n", "PORTMAP", "2");
	pfmt(stdout, MM_NOSTD, ":30:NULL\tSET\tUNSET\tGETPORT\tDUMP\tCALLIT\n");
	for (i = 0; i <= rpcb_highproc_2; i++) {
		switch (i) {
		case PMAPPROC_SET:
			printf("%d/", inf[RPCBVERS_2_STAT].setinfo);
			break;
		case PMAPPROC_UNSET:
			printf("%d/", inf[RPCBVERS_2_STAT].unsetinfo);
			break;
		case PMAPPROC_GETPORT:
			cnt = 0;
			for (pa = inf[RPCBVERS_2_STAT].addrinfo; pa;
				pa = pa->next)
				cnt += pa->success;
			printf("%d/", cnt);
			break;
		case PMAPPROC_CALLIT:
			cnt = 0;
			for (pr = inf[RPCBVERS_2_STAT].rmtinfo; pr;
				pr = pr->next)
				cnt += pr->success;
			printf("%d/", cnt);
			break;
		default: break;  /* For the remaining ones */
		}
		printf("%d\t", inf[RPCBVERS_2_STAT].info[i]);
	}
	printf("\n\n");

	if (inf[RPCBVERS_2_STAT].info[PMAPPROC_CALLIT]) {
		pfmt(stdout, MM_NOSTD, ":22:%s call statistics\n",
		     "PMAP_RMTCALL");
		print_rmtcallstat(RPCBVERS_2_STAT, &inf[RPCBVERS_2_STAT]);
		printf("\n");
	}

	if (inf[RPCBVERS_2_STAT].info[PMAPPROC_GETPORT]) {
		pfmt(stdout, MM_NOSTD, ":22:%s call statistics\n",
		     "PMAP_GETPORT");
		print_getaddrstat(RPCBVERS_2_STAT, &inf[RPCBVERS_4_STAT]);
		printf("\n");
	}

	pfmt(stdout, MM_NOSTD, ":20:%s (version %s) statistics\n",
	     "RPCBIND", "3");
	pfmt(stdout, MM_NOSTD,
	     ":29:NULL\tSET\tUNSET\tGETADDR\tDUMP\tCALLIT\tTIME\tU2T\tT2U\n");
	for (i = 0; i <= rpcb_highproc_3; i++) {
		switch (i) {
		case RPCBPROC_SET:
			printf("%d/", inf[RPCBVERS_3_STAT].setinfo);
			break;
		case RPCBPROC_UNSET:
			printf("%d/", inf[RPCBVERS_3_STAT].unsetinfo);
			break;
		case RPCBPROC_GETADDR:
			cnt = 0;
			for (pa = inf[RPCBVERS_3_STAT].addrinfo; pa;
				pa = pa->next)
				cnt += pa->success;
			printf("%d/", cnt);
			break;
		case RPCBPROC_CALLIT:
			cnt = 0;
			for (pr = inf[RPCBVERS_3_STAT].rmtinfo; pr;
				pr = pr->next)
				cnt += pr->success;
			printf("%d/", cnt);
			break;
		default: break;  /* For the remaining ones */
		}
		printf("%d\t", inf[RPCBVERS_3_STAT].info[i]);
	}
	printf("\n\n");

	if (inf[RPCBVERS_3_STAT].info[RPCBPROC_CALLIT]) {
		pfmt(stdout, MM_NOSTD, ":19:%s (version %s) call statistics\n",
		     "RPCB_RMTCALL", "3");
		print_rmtcallstat(RPCBVERS_3_STAT, &inf[RPCBVERS_3_STAT]);
		printf("\n");
	}

	if (inf[RPCBVERS_3_STAT].info[RPCBPROC_GETADDR]) {
		pfmt(stdout, MM_NOSTD, ":19:%s (version %s) call statistics\n",
		     "RPCB_GETADDR", "3");
		print_getaddrstat(RPCBVERS_3_STAT, &inf[RPCBVERS_3_STAT]);
		printf("\n");
	}

	printf("RPCBIND (version 4) statistics\n");
	pfmt(stdout, MM_NOSTD, ":20:%s (version %s) statistics\n",
	     "RPCBIND", "4");

	for (j = 0; j <= 9; j += 9) { /* Just two iterations for printing */
		if (j == 0) {
			pfmt(stdout, MM_NOSTD,
		":29:NULL\tSET\tUNSET\tGETADDR\tDUMP\tCALLIT\tTIME\tU2T\tT2U\n"
			    );
		} else {
			pfmt(stdout, MM_NOSTD,
			     ":39:VERADDR\tINDRECT\tGETLIST\tGETSTAT\n");
		}
		for (i = j; i <= MAX(8, rpcb_highproc_4 - 9 + j); i++) {
			switch (i) {
			case RPCBPROC_SET:
				printf("%d/", inf[RPCBVERS_4_STAT].setinfo);
				break;
			case RPCBPROC_UNSET:
				printf("%d/", inf[RPCBVERS_4_STAT].unsetinfo);
				break;
			case RPCBPROC_GETADDR:
				cnt = 0;
				for (pa = inf[RPCBVERS_4_STAT].addrinfo; pa;
					pa = pa->next)
					cnt += pa->success;
				printf("%d/", cnt);
				break;
			case RPCBPROC_CALLIT:
				cnt = 0;
				for (pr = inf[RPCBVERS_4_STAT].rmtinfo; pr;
					pr = pr->next)
					cnt += pr->success;
				printf("%d/", cnt);
				break;
			default: break;  /* For the remaining ones */
			}
			printf("%d\t", inf[RPCBVERS_4_STAT].info[i]);
		}
		printf("\n");
	}

	if (inf[RPCBVERS_4_STAT].info[RPCBPROC_CALLIT] ||
			    inf[RPCBVERS_4_STAT].info[RPCBPROC_INDIRECT]) {
		printf("\n");
		pfmt(stdout, MM_NOSTD,
		     ":19:%s (version %s) call statistics\n", 
		     "RPCB_RMTCALL", "4");
		print_rmtcallstat(RPCBVERS_4_STAT, &inf[RPCBVERS_4_STAT]);
	}

	if (inf[RPCBVERS_4_STAT].info[RPCBPROC_GETADDR]) {
		printf("\n");
		pfmt(stdout, MM_NOSTD,
		     ":19:%s (version %s) call statistics\n", 
		     "RPCB_GETADDR", "4");
		print_getaddrstat(RPCBVERS_4_STAT, &inf[RPCBVERS_4_STAT]);
	}
	clnt_destroy(client);
}

/*
 * Delete registeration for this (prog, vers, netid)
 */
static void
deletereg(netid, argc, argv)
	char *netid;
	int argc;
	char **argv;
{
	struct netconfig *nconf = NULL;

	if (argc != 2) {
		usage();
		exit(1);
	}
	if (netid) {
		nconf = getnetconfigent(netid);
		if (nconf == NULL) {
			pfmt(stderr, MM_ERROR, 
			     ":37:Transport provider %s is not supported\n",
			     netid);
			exit(1);
		}
	}
	if ((rpcb_unset(getprognum(argv[0]), getvers(argv[1]), nconf)) == 0) {
		pfmt(stderr, MM_ERROR,
	":26:Could not delete registration for program %s version %s\n",
		     argv[0], argv[1]);
		exit(1);
	}
}

/*
 * Create and return a handle for the given nconf.
 * Exit if cannot create handle.
 */
static CLIENT *
clnt_addr_create(address, nconf, prog, vers)
	char *address;
	struct netconfig *nconf;
	u_long prog;
	u_long vers;
{
	CLIENT *client;
	static struct netbuf *nbuf;
	static int fd = RPC_ANYFD;
	struct t_info tinfo;

	if (fd == RPC_ANYFD) {
		if ((fd = t_open(nconf->nc_device, O_RDWR, &tinfo)) == -1) {
			rpc_createerr.cf_stat = RPC_TLIERROR;
			rpc_createerr.cf_error.re_terrno = t_errno;
			strcpy(errmsg,
			       clnt_spcreateerror(
				  gettxt(":25", "Could not open device %s: ")));
			strcat(errmsg, "\n");
			pfmt(stderr, MM_ERROR|MM_NOGET, errmsg,
			     nconf->nc_device);
			exit(1);
		}
		/* Convert the uaddr to taddr */
		nbuf = uaddr2taddr(nconf, address);
		if (nbuf == NULL) {
			pfmt(stderr, MM_ERROR,
		     ":18:Could not convert address %s to network format: %s\n",
			     address, netdir_sperror());
			exit(1);
		}
	}
	client = clnt_tli_create(fd, nconf, nbuf, prog, vers, 0, 0);
	if (client == (CLIENT *)NULL) {
		strcpy(errmsg,
		       clnt_spcreateerror(gettxt(":48",
		    "Could not create handle for program %lu, version %lu: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, prog, vers);
		exit(1);
	}
	return (client);
}

/*
 * If the version number is given, ping that (prog, vers); else try to find
 * the version numbers supported for that prog and ping all the versions.
 * Remote rpcbind is not contacted for this service. The requests are
 * sent directly to the services themselves.
 */
static void
addrping(address, netid, argc, argv)
	char *address;
	char *netid;
	int argc;
	char **argv;
{
	CLIENT *client;
	struct timeval to;
	enum clnt_stat rpc_stat;
	u_long prognum, versnum, minvers, maxvers;
	struct rpc_err rpcerr;
	int failure = 0;
	struct netconfig *nconf;
	int fd;

	if (argc < 1 || argc > 2 || (netid == NULL)) {
		usage();
		exit(1);
	}
	nconf = getnetconfigent(netid);
	if (nconf == (struct netconfig *)NULL) {
		pfmt(stderr, MM_ERROR,
		     ":27:Could not find transport provider %s: %s\n",
		     netid, netdir_sperror());
		exit(1);
	}
	to.tv_sec = 10;
	to.tv_usec = 0;
	prognum = getprognum(argv[0]);
	if (argc == 1) {	/* Version number not known */
		/*
		 * A call to version 0 should fail with a program/version
		 * mismatch, and give us the range of versions supported.
		 */
		versnum = MIN_VERS;
	} else {
		versnum = getvers(argv[1]);
	}
	client = clnt_addr_create(address, nconf, prognum, versnum);
	rpc_stat = CLNT_CALL(client, NULLPROC, (xdrproc_t) xdr_void,
			(char *)NULL, (xdrproc_t) xdr_void,
			(char *)NULL, to);
	if (argc == 2) {
		/* Version number was known */
		if (pstatus(client, prognum, versnum) < 0)
			failure = 1;
		(void) CLNT_DESTROY(client);
		if (failure)
			exit(1);
		return;
	}
	/* Version number not known */
	(void) CLNT_CONTROL(client, CLSET_FD_NCLOSE, (char *)NULL);
	(void) CLNT_CONTROL(client, CLGET_FD, (char *)&fd);
	if (rpc_stat == RPC_PROGVERSMISMATCH) {
		clnt_geterr(client, &rpcerr);
		minvers = rpcerr.re_vers.low;
		maxvers = rpcerr.re_vers.high;
	} else if (rpc_stat == RPC_SUCCESS) {
		/*
		 * Oh dear, it DOES support version 0.
		 * Let's try version MAX_VERS.
		 */
		(void) CLNT_DESTROY(client);
		client = clnt_addr_create(address, nconf, prognum, MAX_VERS);
		rpc_stat = CLNT_CALL(client, NULLPROC, (xdrproc_t) xdr_void,
				(char *)NULL, (xdrproc_t) xdr_void,
				(char *)NULL, to);
		if (rpc_stat == RPC_PROGVERSMISMATCH) {
			clnt_geterr(client, &rpcerr);
			minvers = rpcerr.re_vers.low;
			maxvers = rpcerr.re_vers.high;
		} else if (rpc_stat == RPC_SUCCESS) {
			/*
			 * It also supports version MAX_VERS.
			 * Looks like we have a wise guy.
			 * OK, we give them information on all
			 * 4 billion versions they support...
			 */
			minvers = 0;
			maxvers = MAX_VERS;
		} else {
			(void) pstatus(client, prognum, MAX_VERS);
			exit(1);
		}
	} else {
		(void) pstatus(client, prognum, (u_long)0);
		exit(1);
	}
	(void) CLNT_DESTROY(client);
	for (versnum = minvers; versnum <= maxvers; versnum++) {
		client = clnt_addr_create(address, nconf, prognum, versnum);
		rpc_stat = CLNT_CALL(client, NULLPROC, (xdrproc_t) xdr_void,
				(char *)NULL, (xdrproc_t) xdr_void,
				(char *)NULL, to);
		if (pstatus(client, prognum, versnum) < 0)
				failure = 1;
		(void) CLNT_DESTROY(client);
	}
	(void) t_close(fd);
	if (failure)
		exit(1);
	return;
}

/*
 * If the version number is given, ping that (prog, vers); else try to find
 * the version numbers supported for that prog and ping all the versions.
 * Remote rpcbind is *contacted* for this service. The requests are
 * then sent directly to the services themselves.
 */
static void
progping(netid, argc, argv)
	char *netid;
	int argc;
	char **argv;
{
	CLIENT *client;
	struct timeval to;
	enum clnt_stat rpc_stat;
	u_long prognum, versnum, minvers, maxvers;
	struct rpc_err rpcerr;
	int failure = 0;
	struct netconfig *nconf;
	struct netbuf svcaddr;
	int fd;

	if (argc < 2 || argc > 3 || (netid == NULL)) {
		usage();
		exit(1);
	}
	prognum = getprognum(argv[1]);
	if (argc == 2) { /* Version number not known */
		/*
		 * A call to version 0 should fail with a program/version
		 * mismatch, and give us the range of versions supported.
		 */
		versnum = MIN_VERS;
	} else {
		versnum = getvers(argv[2]);
	}
	if (netid) {
		nconf = getnetconfigent(netid);
		if (nconf == (struct netconfig *)NULL) {
			pfmt(stderr, MM_ERROR,
			     ":27:Could not find transport provider %s: %s\n",
			     netid, netdir_sperror());
			exit(1);
		}
		client = clnt_tp_create(argv[0], prognum, versnum, nconf);
	} else {
		client = clnt_create(argv[0], prognum, versnum, "NETPATH");
	}
	if (client == (CLIENT *)NULL) {
		strcpy(errmsg,
		       clnt_spcreateerror(gettxt(":48",
		    "Could not create handle for program %lu, version %lu: ")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, prognum, versnum);
		exit(1);
	}
	to.tv_sec = 10;
	to.tv_usec = 0;
	rpc_stat = CLNT_CALL(client, NULLPROC, (xdrproc_t) xdr_void,
			(char *)NULL, (xdrproc_t) xdr_void,
			(char *)NULL, to);
	if (argc == 3) {
		/* Version number was known */
		if (pstatus(client, prognum, versnum) < 0)
			failure = 1;
		(void) CLNT_DESTROY(client);
		if (failure)
			exit(1);
		return;
	}
	/* Version number not known */
	(void) CLNT_CONTROL(client, CLGET_SVC_ADDR, (char *)&svcaddr);
	(void) CLNT_CONTROL(client, CLSET_FD_NCLOSE, (char *)NULL);
	(void) CLNT_CONTROL(client, CLGET_FD, (char *)&fd);
	if (rpc_stat == RPC_PROGVERSMISMATCH) {
		clnt_geterr(client, &rpcerr);
		minvers = rpcerr.re_vers.low;
		maxvers = rpcerr.re_vers.high;
	} else if (rpc_stat == RPC_SUCCESS) {
		/*
		 * Oh dear, it DOES support version 0.
		 * Let's try version MAX_VERS.
		 */
		(void) CLNT_DESTROY(client);
		client = clnt_tli_create(fd, nconf, &svcaddr, prognum,
				MAX_VERS, 0, 0);
		if (client == NULL) {
			strcpy(errmsg,
			       clnt_spcreateerror(gettxt(":48",
		    "Could not create handle for program %lu, version %lu: ")));
			strcat(errmsg, "\n");
			pfmt(stderr, MM_ERROR|MM_NOGET, errmsg,
			     prognum, MAX_VERS);
			exit(1);
		}
		rpc_stat = CLNT_CALL(client, NULLPROC,
				(xdrproc_t) xdr_void, (char *)NULL,
				(xdrproc_t)  xdr_void, (char *)NULL, to);
		if (rpc_stat == RPC_PROGVERSMISMATCH) {
			clnt_geterr(client, &rpcerr);
			minvers = rpcerr.re_vers.low;
			maxvers = rpcerr.re_vers.high;
		} else if (rpc_stat == RPC_SUCCESS) {
			/*
			 * It also supports version MAX_VERS.
			 * Looks like we have a wise guy.
			 * OK, we give them information on all
			 * 4 billion versions they support...
			 */
			minvers = 0;
			maxvers = MAX_VERS;
		} else {
			(void) pstatus(client, prognum, MAX_VERS);
			exit(1);
		}
	} else {
		(void) pstatus(client, prognum, (u_long)0);
		exit(1);
	}
	(void) CLNT_DESTROY(client);
	for (versnum = minvers; versnum <= maxvers; versnum++) {
		client = clnt_tli_create(fd, nconf, &svcaddr, prognum,
				versnum, 0, 0);
		if (client == NULL) {
			strcpy(errmsg,
			       clnt_spcreateerror(gettxt(":48",
		    "Could not create handle for program %lu, version %lu: ")));
			strcat(errmsg, "\n");
			pfmt(stderr, MM_ERROR|MM_NOGET, errmsg,
			     prognum, versnum);
			exit(1);
		}
		rpc_stat = CLNT_CALL(client, NULLPROC, (xdrproc_t) xdr_void,
					(char *)NULL, (xdrproc_t) xdr_void,
					(char *)NULL, to);
		if (pstatus(client, prognum, versnum) < 0)
				failure = 1;
		(void) CLNT_DESTROY(client);
	}
	(void) t_close(fd);
	if (failure)
		exit(1);
	return;
}

static void
usage()
{
	pfmt(stderr, MM_ACTION, 
	     ":38:Usage:\n");
	pfmt(stderr, MM_NOSTD, 
	     ":6:       %s %s [host]\n",
	     rpcicmd, "[-ms]");
#ifdef PORTMAP
	pfmt(stderr, MM_NOSTD, 
	     ":6:       %s %s [host]\n",
	     rpcicmd, "-p");
#endif
	pfmt(stderr, MM_NOSTD, 
 ":1:       %s %s transport host program_number [version_number]\n",
	     rpcicmd, "-T");
	pfmt(stderr, MM_NOSTD, 
 ":5:       %s %s host program_number version_number\n",
	     rpcicmd, "-l");
#ifdef PORTMAP
	pfmt(stderr, MM_NOSTD, 
 ":7:       %s [%s port_number] %s host program_number [version_number]\n",
	     rpcicmd, "-n", "-t");
	pfmt(stderr, MM_NOSTD, 
 ":7:       %s [%s port_number] %s host program_number [version_number]\n",
	     rpcicmd, "-n", "-u");
#endif
	pfmt(stderr, MM_NOSTD, 
 ":2:       %s %s service_address %s transport program_number [version_number]\n",
	     rpcicmd, "-a", "-T");
	pfmt(stderr, MM_NOSTD, 
 ":3:       %s %s program_number version_number\n",
	     rpcicmd, "-b");
	pfmt(stderr, MM_NOSTD, 
 ":4:       %s %s [%s transport] program_number version_number\n",
	     rpcicmd, "-d", "-T");
}

static u_long
getprognum  (arg)
	char *arg;
{
	char *strptr;
	register struct rpcent *rpc;
	register u_long prognum;
	char *tptr = arg;

	while (*tptr && isdigit(*tptr++));
	if (*tptr || isalpha(*(tptr - 1))) {
		rpc = getrpcbyname(arg);
		if (rpc == NULL) {
			pfmt(stderr, MM_ERROR, 
			     ":17:%s is an unknown service\n", arg);
			exit(1);
		}
		prognum = rpc->r_number;
	} else {
		prognum = strtol(arg, &strptr, 10);
		if (strptr == arg || *strptr != '\0') {
			pfmt(stderr, MM_ERROR,
			     ":15:%s is an illegal program number\n", arg);
			exit(1);
		}
	}
	return (prognum);
}

static u_long
getvers(arg)
	char *arg;
{
	char *strptr;
	register u_long vers;

	vers = (int) strtol(arg, &strptr, 10);
	if (strptr == arg || *strptr != '\0') {
		pfmt(stderr, MM_ERROR, 
		     ":16:%s is an illegal version number\n", arg);
		exit(1);
	}
	return (vers);
}

/*
 * This routine should take a pointer to an "rpc_err" structure, rather than
 * a pointer to a CLIENT structure, but "clnt_perror" takes a pointer to
 * a CLIENT structure rather than a pointer to an "rpc_err" structure.
 * As such, we have to keep the CLIENT structure around in order to print
 * a good error message.
 */
static int
pstatus(client, prog, vers)
	register CLIENT *client;
	u_long prog;
	u_long vers;
{
	struct rpc_err rpcerr;

	clnt_geterr(client, &rpcerr);
	if (rpcerr.re_status != RPC_SUCCESS) {
		strcpy(errmsg, clnt_sperror(client, gettxt(":35",
				  "Program %lu version %lu is not available")));
		strcat(errmsg, "\n");
		pfmt(stderr, MM_ERROR|MM_NOGET, errmsg, prog, vers);
		return (-1);
	} else {
		pfmt(stdout, MM_INFO, 
		     ":36:Program %lu version %lu is ready and waiting\n",
			prog, vers);
		return (0);
	}
}

static CLIENT *
clnt_rpcbind_create(host, rpcbversnum, targaddr)
	char *host;
	u_long rpcbversnum;
	struct netbuf **targaddr;
{
	static char *tlist[3] = {
		"circuit_n", "circuit_v", "datagram_v"
	};
	int i;
	struct netconfig *nconf;
	CLIENT *clnt = NULL;
	void *handle;

	rpc_createerr.cf_stat = RPC_SUCCESS;
	for (i = 0; i < 3; i++) {
		if ((handle = _rpc_setconf(tlist[i])) == NULL)
			continue;
		while (clnt == (CLIENT *)NULL) {
			if ((nconf = _rpc_getconf(handle)) == NULL) {
				if (rpc_createerr.cf_stat == RPC_SUCCESS)
				    rpc_createerr.cf_stat = RPC_UNKNOWNPROTO;
				break;
			}
			clnt = getclnthandle(host, nconf, rpcbversnum,
					targaddr);
		}
		if (clnt)
			break;
		_rpc_endconf(handle);
	}
	return (clnt);
}

static CLIENT*
getclnthandle(host, nconf, rpcbversnum, targaddr)
	char *host;
	struct netconfig *nconf;
	u_long rpcbversnum;
	struct netbuf **targaddr;
{
	struct netbuf *addr;
	struct nd_addrlist *nas;
	struct nd_hostserv rpcbind_hs;
	CLIENT *client = NULL;

	/* Get the address of the rpcbind */
	rpcbind_hs.h_host = host;
	rpcbind_hs.h_serv = "rpcbind";
	if (netdir_getbyname(nconf, &rpcbind_hs, &nas)) {
		rpc_createerr.cf_stat = RPC_N2AXLATEFAILURE;
		return (NULL);
	}
	addr = nas->n_addrs;
	client = clnt_tli_create(RPC_ANYFD, nconf, addr, RPCBPROG,
			rpcbversnum, 0, 0);
	if (client) {
		if (targaddr != NULL) {
			*targaddr = (struct netbuf *)malloc(sizeof
					(struct netbuf));
			if (*targaddr != NULL) {
				(*targaddr)->maxlen = addr->maxlen;
				(*targaddr)->len = addr->len;
				(*targaddr)->buf = (char *)malloc(addr->len);
				if ((*targaddr)->buf != NULL) {
					memcpy((*targaddr)->buf, addr->buf,
						addr->len);
				}
			}
		}
	}
	netdir_free((char *)nas, ND_ADDRLIST);
	return (client);
}

static void
print_rmtcallstat(rtype, infp)
	int rtype;
	rpcb_stat *infp;
{
	register rpcbs_rmtcalllist_ptr pr;
	struct rpcent *rpc;

	if (rtype == RPCBVERS_4_STAT)
		pfmt(stdout, MM_NOSTD,
    ":44:program\t\tversion\tprocedure\ttransport\tindirect success failure\n");
	else
		pfmt(stdout, MM_NOSTD,
	    ":43:program\t\tversion\tprocedure\ttransport\tsuccess\tfailure\n");
	for (pr = infp->rmtinfo; pr; pr = pr->next) {
		rpc = getrpcbynumber(pr->prog);
		if (rpc)
			printf("%-16s", rpc->r_name);
		else
			printf("%-16d", pr->prog);
		printf("%d\t%d\t%s\t",
			pr->vers, pr->proc, pr->netid);
		if (rtype == RPCBVERS_4_STAT)
			printf("%d\t ", pr->indirect);
		printf("%d\t%d\n", pr->success, pr->failure);
	}
}

static void
print_getaddrstat(rtype, infp)
	int rtype;
	rpcb_stat *infp;
{
	rpcbs_addrlist_ptr al;
	register struct rpcent *rpc;

	pfmt(stdout, MM_NOSTD, 
	     ":43:program\t\tversion\ttransport\tsuccess\tfailure\n");
	for (al = infp->addrinfo; al; al = al->next) {
		rpc = getrpcbynumber(al->prog);
		if (rpc)
			printf("%-16s", rpc->r_name);
		else
			printf("%-16d", al->prog);
		printf("%d\t%s\t  %d\t%d\n",
			al->vers, al->netid,
			al->success, al->failure);
	}
}

static char *
owner(char *owner)
{
	if (strcmp(owner, "superuser") == 0) {
		return (gettxt(":46", "superuser"));
	} else if (strcmp(owner, "unknown") == 0) {
		return (gettxt(":47", "unknown"));
	} else {
		return (owner);
	}
}
