/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)rpcbind:rpcbind.c	1.14.12.8"
#ident  "$Header: $"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*+++++++++++++++++++++++++++++++++++++++++++++++++++++++++
*	PROPRIETARY NOTICE (Combined)
*
* This source code is unpublished proprietary information
* constituting, or derived under license from AT&T's UNIX(r) System V.
* In addition, portions of such source code were derived from Berkeley
* 4.3 BSD under license from the Regents of the University of
* California.
*
*
*
*	Copyright Notice 
*
* Notice of copyright on this source code product does not indicate 
*  publication.
*
*	(c) 1986,1987,1988,1989,1990  Sun Microsystems, Inc
*	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
*	(c) 1990,1991,1992  UNIX System Laboratories, Inc.
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
 * Copyright (c) 1984 - 1991 by Sun Microsystems, Inc.
 */

/*
 * rpcbind.c
 * Implements the program, version to address mapping for rpc.
 *
 */

#include <dlfcn.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <xti.h>
#include <rpc/rpc.h>
#include <netconfig.h>
#include <netdir.h>
#include <sys/wait.h>
#include <sys/signal.h>
#ifdef PORTMAP
#include <netinet/in.h>
#endif
#include <sys/termios.h>
#include "rpcbind.h"
#include <sys/syslog.h>
#include <sys/stat.h>
#include <sys/utsname.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <libgen.h>
#include <pfmt.h>
#include <locale.h>

#define USAGE	":66:Usage:\n\t%s %s\n"
#define OPTIONS	"[-d|-q] [-w]"

#ifdef PORTMAP
extern void pmap_service();
#endif
extern void rpcb_service_3();
extern void rpcb_service_4();
extern void read_warmstart();
extern void write_warmstart();
#ifdef WAIT3
void reap();
#endif

static void terminate();
static void detachfromtty();
static void parseargs();
static void rbllist_add();
static int init_transport();


/* Global variables */
int debugging = 0;	/* Tell me what's going on */
int quiet = 0;		/* If 1, suppress messages to stderr */
rpcblist_ptr list_rbl;	/* A list of version 3/4 rpcbind services */
char *loopback_dg;	/* Datagram loopback transport, for set and unset */
char *loopback_vc;	/* COTS loopback transport, for set and unset */
char *loopback_vc_ord;	/* COTS_ORD loopback transport, for set and unset */
char *rpcbcmd;		/* Name of rpcbind command (from argv[0]) */
char syslogmsg[BUFSIZ];	/* For building prefixed message for syslog() */
char *syslogmsgp;	/* Points to position after label */

/* Local Variable */
static int warmstart = 0; /* Grab an old copy of registrations */

#ifdef PORTMAP
PMAPLIST *list_pml;	/* A list of version 2 rpcbind services */
char *udptrans;		/* Name of UDP transport */
char *tcptrans;		/* Name of TCP transport */
char *udp_uaddr;	/* Universal UDP address */
char *tcp_uaddr;	/* Universal TCP address */
#endif
static char servname[] = "rpcbind";
static char superuser[] = "superuser";
static char label[64];

main(argc, argv)
	int argc;
	char *argv[];
{
	struct netconfig *nconf;
	void *nc_handle;	/* Net config handle */

	(void)setlocale(LC_ALL, "");
	(void)setcat("uxrpcbind");

	rpcbcmd = basename(argv[0]);
	(void)strcpy(label, "UX:");
	(void)strcat(label, rpcbcmd);
	(void)setlabel(label);
	/*
	 * Use syslog labeling, which includes date, system, and command.
	 * Therefore, drop this labeling and save messages beginning at
	 * the start of the syslogmsg buffer.
	(void)strcpy(syslogmsg, label);
	syslogmsgp = &syslogmsg[strlen(syslogmsg)];
	*/
	syslogmsgp = &syslogmsg[0];
	
	parseargs(argc, argv);

	openlog("rpcbind", LOG_CONS, LOG_DAEMON);
	if (geteuid()) { /* This command allowed only to root */
		if (!quiet) {
			pfmt(stderr, MM_ERROR,
			":68:You must be the superuser to run this command.\n");
		}
		exit(1);
	}
	nc_handle = setnetconfig(); 	/* open netconfig file */
	if (nc_handle == NULL) {
		(void)strcpy(syslogmsgp,
			     gettxt(":34", "Could not read %s."));
		syslog(LOG_ERR, syslogmsg, "/etc/netconfig");
		if (!quiet) {
			pfmt(stderr, MM_ERROR,
			     ":34:Could not read %s.", "/etc/netconfig");
			fprintf(stderr, "\n");
		}
		exit(1);
	}
	loopback_dg = "";
	loopback_vc = "";
	loopback_vc_ord = "";
#ifdef PORTMAP
	udptrans = "";
	tcptrans = "";
#endif

	{	/*
		 * rpcbind is the first application to encounter the
		 * various netconfig files.  check_netconfig() verifies
		 * that they are set up correctly and complains loudly
		 * if not.
		 */
		int trouble;

		trouble = check_netconfig();
		if (trouble) {
			(void)strcpy(syslogmsgp,
			     gettxt(":48", 
	"Found %d errors with network configuration files. Continuing."));
			syslog(LOG_ERR, syslogmsg, trouble);
			if (!quiet) {
				pfmt(stderr, MM_ERROR,
	":48:Found %d errors with network configuration files. Continuing.",
				trouble);
				fprintf(stderr, "\n");
			}
		}
	}
	while (nconf = getnetconfig(nc_handle)) {
		init_transport(nconf);
	}
	endnetconfig(nc_handle);

	if ((loopback_dg[0]     == NULL)
	 && (loopback_vc[0]     == NULL)
	 && (loopback_vc_ord[0] == NULL)) {
		(void)strcpy(syslogmsgp,
		     gettxt(":21", 
			"Could not find any loopback transport.  Exiting."));
		syslog(LOG_ERR, syslogmsg);
		if (!quiet) {
			pfmt(stderr, MM_ERROR,
			":21:Could not find any loopback transport.  Exiting.");
			fprintf(stderr, "\n");
		}
		exit(1);
	}

	/* catch the usual termination signals for graceful exit */
	(void) signal(SIGINT, terminate);
	(void) signal(SIGTERM, terminate);
	(void) signal(SIGQUIT, terminate);
	/* ignore others that could get sent */
	(void) signal(SIGHUP, SIG_IGN);
	(void) signal(SIGUSR1, SIG_IGN);
	(void) signal(SIGUSR2, SIG_IGN);
	if (warmstart) {
		read_warmstart();
	}
	if (debugging) {
		pfmt(stdout, MM_INFO, 
		     ":39:Debugging enabled -- will abort on errors!\n");
	} else {
		detachfromtty();
	}
	my_svc_run();
	(void)strcpy(syslogmsgp,
		     gettxt(":8", "%s returned unexpectedly"));
	syslog(LOG_ERR, syslogmsg, "svc_run");
	rpcbind_abort();
	/* NOTREACHED */
}

/*
 * Increments a counter each time a problem is found with the network
 * configuration information.
 */
int
check_netconfig()
{
	void	*nc;
	void	*dlcookie;
	int	busted = 0;
	int	i;
	int	lo_clts_found = 0, lo_cots_found = 0, lo_cotsord_found = 0;
	struct netconfig	*nconf, *np;
	struct stat	sb;
	struct utsname	utsname;
	struct nd_hostserv	nh;
	struct nd_addrlist	*na;
	extern int	errno;

	nc = setnetconfig();
	if (nc == (void *) 0) {
		if (debugging) {
			pfmt(stderr, MM_ERROR, ":3:%s failed: ",
			     "setnetconfig");
			nc_perror("");
		}
		(void)strcpy(syslogmsgp, gettxt(":3", "%s failed: "));
		(void)strcat(syslogmsgp, nc_sperror());
		syslog(LOG_ALERT, syslogmsg, "setnetconfig");
		return (1);
	}
	while (np = getnetconfig(nc)) {
		if ((np->nc_flag & NC_VISIBLE) == 0)
			continue;
		if (debugging) {
			pfmt(stderr, MM_INFO,
			     ":13:Checking transport provider %s\n",
			     np->nc_netid);
		}
		if (strcmp(np->nc_protofmly, NC_LOOPBACK) == 0)
			switch (np->nc_semantics) {
			case NC_TPI_CLTS:
				lo_clts_found = 1;
				break;

			case NC_TPI_COTS:
				lo_cots_found = 1;
				break;

			case NC_TPI_COTS_ORD:
				lo_cotsord_found = 1;
				break;
			}
		if (stat(np->nc_device, &sb) == -1 && errno == ENOENT) {
			if (debugging) {
				pfmt(stderr, MM_ERROR,
				     ":40: Device %s does not exist for %s.",
				     np->nc_device, np->nc_netid);
				fprintf(stderr, "\n");
			}
			(void)strcpy(syslogmsgp,
			     gettxt(":40", 
		"Device %s does not exist for %s"));
			syslog(LOG_ERR, syslogmsg,
			       np->nc_device, np->nc_netid);
			busted++;
		} else {
			if (debugging) {
				pfmt(stderr, MM_INFO,
				     ":41:  Device %s is present.\n",
				     np->nc_device);
			}
		}
		for (i = 0; i < np->nc_nlookups; i++) {
			char	*libname = np->nc_lookups[i];

			if ((dlcookie = dlopen(libname, RTLD_NOW)) ==
				(void *) NULL) {

				char *dlerrstr;

				dlerrstr = dlerror();
				if (debugging) {
					pfmt(stderr, MM_ERROR,
	       ":33: Could not open name-to-address mapping library %s for %s",
					     libname, np->nc_netid);
					fprintf(stderr, "\n");
					pfmt(stderr, MM_ERROR|MM_NOGET, " %s",
					     dlerrstr);
					fprintf(stderr, "\n");
				}
				(void)strcpy(syslogmsgp, 
					     gettxt(":33", 
     		 "Could not open name-to-address mapping library %s for %s"));
				syslog(LOG_ERR, syslogmsg,
				       libname, np->nc_netid);
				syslog(LOG_ERR, "%s", dlerrstr);
				busted++;
			} else {
				if (debugging) {
					pfmt(stderr, MM_INFO,
     ":61:  Opened name-to-address mapping library %s\n", libname);
				}
				(void) dlclose(dlcookie);
			}
		}

		if (uname(&utsname) == -1) {
			char *errp;

			errp = strerror(errno);
			if (debugging) {
				pfmt(stderr, MM_ERROR,
			    ":25: Could not get the name of this system: %s\n",
				     errp);
			}
			(void)strcpy(syslogmsgp,
			     gettxt(":4", 
	    "%s failed: Could not determine the node name of this system: %s"));
			syslog(LOG_ERR, syslogmsg, "uname", errp);
			busted++;
			break;
		}

		nconf = getnetconfigent(np->nc_netid);

/* This piece of code is commneted out.
		nh.h_host = HOST_SELF;
		nh.h_serv = "";
		if (netdir_getbyname(nconf, &nh, &na) != 0 || na->n_cnt == 0) {
			if (debugging)
				fprintf(stderr,
	"\tnetdir_getbyname for HOST_SELF failed\n");
			syslog(LOG_ALERT,
	"netid %s:  cannot find an address for HOST_SELF", np->nc_netid);
			busted++;
		} else
			if (debugging)
				fprintf(stderr,
	"\tnetdir_getbyname for HOST_SELF succeeded\n");

		nh.h_host = utsname.nodename;
		nh.h_serv = "";
		if (netdir_getbyname(nconf, &nh, &na) != 0 || na->n_cnt == 0) {
			syslog(LOG_ALERT,
	"netid %s:  cannot find an address for \"%s\"",
				np->nc_netid, utsname.nodename);
			if (debugging)
				fprintf(stderr,
	"\tnetdir_getbyname for %s failed\n", utsname.nodename);
			busted++;
		} else
			if (debugging)
				fprintf(stderr,
	"\tnetdir_getbyname for %s succeeded\n", utsname.nodename);
*/
		nh.h_host = HOST_SELF;
		nh.h_serv = "rpcbind";
		if (netdir_getbyname(nconf, &nh, &na) != 0 || na->n_cnt == 0) {
			(void)strcpy(syslogmsgp,
				     gettxt(":2", 
			        "%s failed on %s for host %s, service %s\n"));
			syslog(LOG_ALERT, syslogmsg, "netdir_getbyname",
			       np->nc_netid, "HOST_SELF", "rpcbind");
			if (debugging) {
				pfmt(stderr, MM_ERROR,
	     ":20: Could not find an address on %s for host %s, service %s\n",
				     np->nc_netid, "HOST_SELF", "rpcbind");
			}
			busted++;
		} else {
			if (debugging) {
				pfmt(stderr, MM_INFO,
			  ":50:  Found address on %s for host %s, service %s\n",
				     np->nc_netid, "HOST_SELF", "rpcbind");
			}
		}
		nh.h_host = utsname.nodename;
		nh.h_serv = "rpcbind";
		if (netdir_getbyname(nconf, &nh, &na) != 0 || na->n_cnt == 0) {
			(void)strcpy(syslogmsgp,
			     	     gettxt(":2", 
			          "%s failed on %s for host %s, service %s\n"));
			syslog(LOG_ALERT, syslogmsg, "netdir_getbyname",
			       np->nc_netid, utsname.nodename, "rpcbind");
			if (debugging) {
				pfmt(stderr, MM_ERROR,
	     ":20: Could not find an address on %s for host %s, service %s\n",
				     np->nc_netid, utsname.nodename, "rpcbind");
			}
			busted++;
		} else
			if (debugging)
				pfmt(stderr, MM_INFO,
			  ":50:  Found address on %s for host %s, service %s\n",
				     np->nc_netid, utsname.nodename, "rpcbind");
		freenetconfigent(nconf);
	}
	endnetconfig(nc);

	if (lo_clts_found) {
		if (debugging) {
			pfmt(stderr, MM_INFO, 
			     ":49:Found %s loopback transport", "CLTS");
			fprintf(stderr, "\n");
		}
	} else {
		(void)strcpy(syslogmsgp,
			     gettxt(":54",
				    "No %s loopback transport was found."));
		syslog(LOG_ALERT, syslogmsg, "CLTS");
		if (debugging) {
			pfmt(stderr, MM_WARNING,
			     ":54:No %s loopback transport was found.", "CLTS");
			fprintf(stderr, "\n");
		}
	}
	if (lo_cots_found) {
		if (debugging) {
			pfmt(stderr, MM_INFO, 
			     ":49:Found %s loopback transport", "COTS");
			fprintf(stderr, "\n");
		}
	} else {
		(void)strcpy(syslogmsgp,
			     gettxt(":54",
				    "No %s loopback transport was found."));
		syslog(LOG_ALERT, syslogmsg, "COTS");
		if (debugging) {
			pfmt(stderr, MM_WARNING,
			     ":54:No %s loopback transport was found.", "COTS");
			fprintf(stderr, "\n");
		}
	}
	if (lo_cotsord_found) {
		if (debugging) {
			pfmt(stderr, MM_INFO, 
			     ":49:Found %s loopback transport", "COTS ORD");
			fprintf(stderr, "\n");
		}
	} else {
		(void)strcpy(syslogmsgp,
			     gettxt(":54",
				    "No %s loopback transport was found."));
		syslog(LOG_ALERT, syslogmsg, "COTS ORD");
		if (debugging) {
			pfmt(stderr, MM_WARNING,
			     ":54:No %s loopback transport was found.", 
			     "COTS ORD");
			fprintf(stderr, "\n");
		}
	}

	return (busted);
}

/*
 * Adds the entry into the rpcbind database.
 * If PORTMAP, then for UDP and TCP, it adds the entries for version 2 also
 * Returns 0 if succeeds, else fails
 */
static int
init_transport(nconf)
	struct netconfig *nconf;	/* Transport provider info */
{
	int fd;
	struct t_bind *taddr, *baddr;
	SVCXPRT	*my_xprt;
	struct nd_addrlist *nas;
	struct nd_hostserv hs;
	int status;	/* bound checking ? */

	if ((nconf->nc_semantics != NC_TPI_CLTS) &&
		(nconf->nc_semantics != NC_TPI_COTS) &&
		(nconf->nc_semantics != NC_TPI_COTS_ORD))
		return (1);	/* not my type */
#ifdef ND_DEBUG
	{
	int i;
	char **s;

	(void) fprintf(stderr, "%s: %d lookup routines :\n",
		nconf->nc_netid, nconf->nc_nlookups);
	for (i = 0, s = nconf->nc_lookups; i < nconf->nc_nlookups; i++, s++)
		fprintf(stderr, "[%d] - %s\n", i, *s);
	}
#endif

	if ((fd = t_open(nconf->nc_device, O_RDWR, NULL)) < 0) {
		(void)strcpy(syslogmsgp,
			     gettxt(":30", 
				 "Could not open connection on %s: %s"));
		syslog(LOG_ERR, syslogmsg,
		       nconf->nc_netid, t_strerror(t_errno));
		return (1);
	}

	taddr = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
	baddr = (struct t_bind *)t_alloc(fd, T_BIND, T_ADDR);
	if ((baddr == NULL) || (taddr == NULL)) {
		(void)strcpy(syslogmsgp,
			     gettxt(":14", 
				    "Could not allocate buffer for %s: %s"));
		syslog(LOG_ERR, syslogmsg,
		       nconf->nc_netid, t_strerror(t_errno));
		exit(1);
	}

	/* Get rpcbind's address on this transport */
	hs.h_host = HOST_SELF;
	hs.h_serv = servname;
	if (netdir_getbyname(nconf, &hs, &nas))
		goto error;

	/* Copy the address */
	taddr->addr.len = nas->n_addrs->len;
	memcpy(taddr->addr.buf, nas->n_addrs->buf, (int)nas->n_addrs->len);
#ifdef ND_DEBUG
	{
	/* for debugging print out our universal address */
	char *uaddr;

	uaddr = taddr2uaddr(nconf, nas->n_addrs);
	(void) fprintf(stderr, "rpcbind : my address is %s\n", uaddr);
	(void) free(uaddr);
	}
#endif
	netdir_free((char *)nas, ND_ADDRLIST);

	if (nconf->nc_semantics == NC_TPI_CLTS)
		taddr->qlen = 0;
	else
		taddr->qlen = 8;	/* should be enough */

	if (t_bind(fd, taddr, baddr) != 0) {
		(void)strcpy(syslogmsgp,
			     gettxt(":15", 
				 "Could not bind address on transport %s: %s"));
		syslog(LOG_ERR, syslogmsg,
		       nconf->nc_netid, t_strerror(t_errno));
		goto error;
	}

	if (memcmp(taddr->addr.buf, baddr->addr.buf, (int)baddr->addr.len)) {
		(void)strcpy(syslogmsgp,
			    gettxt(":10", "Address is in use on transport %s"));
		syslog(LOG_ERR, syslogmsg, nconf->nc_netid);
		goto error;
	}

	my_xprt = (SVCXPRT *)svc_tli_create(fd, nconf, baddr, 0, 0);
	if (my_xprt == (SVCXPRT *)NULL) {
		(void)strcpy(syslogmsgp,
			     gettxt(":18", 
				   "Could not create service on transport %s"));
		syslog(LOG_ERR, syslogmsg, nconf->nc_netid);
		goto error;
	}

#ifdef PORTMAP
	/*
	 * Register both the versions for tcp/ip and udp/ip
	 */
	if ((strcmp(nconf->nc_protofmly, NC_INET) == 0) &&
		((strcmp(nconf->nc_proto, NC_TCP) == 0) ||
		(strcmp(nconf->nc_proto, NC_UDP) == 0))) {
		PMAPLIST *pml;

		if (!svc_register(my_xprt, PMAPPROG, PMAPVERS,
				  pmap_service, NULL)) {
			(void)strcpy(syslogmsgp,
			     	     gettxt(":35", 
					 "Could not register on transport %s"));
			syslog(LOG_ERR, syslogmsg, nconf->nc_netid);
			goto error;
		}
		pml = (PMAPLIST *)malloc((u_int)sizeof (PMAPLIST));
		if (pml == (PMAPLIST *)NULL) {
			(void)strcpy(syslogmsgp,
				     gettxt(":56", "No memory!"));
			syslog(LOG_ERR, syslogmsg);
			exit(1);
		}
		pml->pml_map.pm_prog = PMAPPROG;
		pml->pml_map.pm_vers = PMAPVERS;
		pml->pml_map.pm_port = PMAPPORT;
		if (strcmp(nconf->nc_proto, NC_TCP) == 0) {
			if (tcptrans[0]) {
				(void)strcpy(syslogmsgp,
					     gettxt(":12", 
				     "Cannot have more than one %s transport"));
				syslog(LOG_ERR, syslogmsg, "TCP");
				goto error;
			}
			tcptrans = strdup(nconf->nc_netid);
			pml->pml_map.pm_prot = IPPROTO_TCP;

			/* Let's snarf the universal address */
			/* "h1.h2.h3.h4.p1.p2" */
			tcp_uaddr = taddr2uaddr(nconf, &baddr->addr);
		} else {
			if (udptrans[0]) {
				(void)strcpy(syslogmsgp,
					     gettxt(":12", 
				     "Cannot have more than one %s transport"));
				syslog(LOG_ERR, syslogmsg, "UDP");
				goto error;
			}
			udptrans = strdup(nconf->nc_netid);
			pml->pml_map.pm_prot = IPPROTO_UDP;

			/* Let's snarf the universal address */
			/* "h1.h2.h3.h4.p1.p2" */
			udp_uaddr = taddr2uaddr(nconf, &baddr->addr);
		}
		pml->pml_next = list_pml;
		list_pml = pml;

		/* Add version 3 information */
		pml = (PMAPLIST *)malloc((u_int)sizeof (PMAPLIST));
		if (pml == (PMAPLIST *)NULL) {
			(void)strcpy(syslogmsgp,
				     gettxt(":56", "No memory!"));
			syslog(LOG_ERR, syslogmsg); 
			exit(1);
		}
		pml->pml_map = list_pml->pml_map;
		pml->pml_map.pm_vers = RPCBVERS;
		pml->pml_next = list_pml;
		list_pml = pml;

		/* Also add version 2 stuff to rpcbind list */
		rbllist_add(PMAPPROG, PMAPVERS, nconf, &baddr->addr);
	}
#endif

	/* version 3 registration */
	if (!svc_reg(my_xprt, RPCBPROG, RPCBVERS, rpcb_service_3, NULL)) {
		(void)strcpy(syslogmsgp,
			     gettxt(":36", 
			      "Could not register version %s on transport %s"));
		syslog(LOG_ERR, syslogmsg, "3", nconf->nc_netid);
		goto error;
	}
	rbllist_add(RPCBPROG, RPCBVERS, nconf, &baddr->addr);

	/* version 4 registration */
	if (!svc_reg(my_xprt, RPCBPROG, RPCBVERS4, rpcb_service_4, NULL)) {
		(void)strcpy(syslogmsgp,
			     gettxt(":36", 
			      "Could not register version %s on transport %s"));
		syslog(LOG_ERR, syslogmsg, "4", nconf->nc_netid);
		goto error;
	}
	rbllist_add(RPCBPROG, RPCBVERS4, nconf, &baddr->addr);

	/*
	 * Tell RPC library to shut up about version mismatches so that new
	 * revs of broadcast protocols don't cause all the old servers to
	 * say: "wrong version".
	 */
	(void) svc_versquiet(my_xprt);

	/*
	 * In case of loopback transports, negotiate for
	 * returning of the uid of the caller.
	 */
	if (strcmp(nconf->nc_protofmly, NC_LOOPBACK) == 0) {
		if (nconf->nc_semantics == NC_TPI_CLTS)
			loopback_dg = strdup(nconf->nc_netid);
		else if (nconf->nc_semantics == NC_TPI_COTS)
			loopback_vc = strdup(nconf->nc_netid);
		else if (nconf->nc_semantics == NC_TPI_COTS_ORD)
			loopback_vc_ord = strdup(nconf->nc_netid);
		if (_rpc_negotiate_uid(fd)) {
			(void)strcpy(syslogmsgp,
				     gettxt(":28", 
			      "Could not negotiate with loopback tranport %s"));
			syslog(LOG_ERR, syslogmsg, nconf->nc_netid);
		}
	}

	/* decide if bound checking works for this transport */
	status = add_bndlist(nconf, taddr, baddr);
#ifdef RPCBIND_DEBUG
	if (status < 0) {
		fprintf(stderr, "Error in finding bind status for %s\n",
			nconf->nc_netid);
	} else if (status == 0) {
		fprintf(stderr, "check binding for %s\n",
			nconf->nc_netid);
	} else if (status > 0) {
		fprintf(stderr, "No check binding for %s\n",
			nconf->nc_netid);
	}
#endif

	(void) t_free((char *)taddr, T_BIND);
	(void) t_free((char *)baddr, T_BIND);
	return (0);
error:
	(void) t_free((char *)taddr, T_BIND);
	(void) t_free((char *)baddr, T_BIND);
	(void) t_close(fd);
	return (1);
}

static void
rbllist_add(prog, vers, nconf, addr)
	u_long prog;
	u_long vers;
	struct netconfig *nconf;
	struct netbuf *addr;
{
	rpcblist_ptr rbl;

	rbl = (rpcblist_ptr)malloc((u_int)sizeof (rpcblist));
	if (rbl == (rpcblist_ptr)NULL) {
		(void)strcpy(syslogmsgp,
			     gettxt(":56", "No memory!"));
		syslog(LOG_ERR, syslogmsg);
		exit(1);
	}

	rbl->rpcb_map.r_prog = prog;
	rbl->rpcb_map.r_vers = vers;
	rbl->rpcb_map.r_netid = strdup(nconf->nc_netid);
	rbl->rpcb_map.r_addr = taddr2uaddr(nconf, addr);
	rbl->rpcb_map.r_owner = superuser;
	rbl->rpcb_next = list_rbl;	/* Attach to global list */
	list_rbl = rbl;
}

/*
 * Catch the signal and die
 */
static void
terminate()
{
	(void)strcpy(syslogmsgp,
		     gettxt(":64",
			    "Terminating on signal. Restart with %s %s"));
	syslog(LOG_ERR, syslogmsg, rpcbcmd, "-w");
	write_warmstart();	/* Dump yourself */
	exit(2);
}

void
rpcbind_abort()
{
	write_warmstart();	/* Dump yourself */
	abort();
}

/*
 * detach from tty
 */
static void
detachfromtty()
{
	close(0);
	close(1);
	close(2);
	switch (fork()) {
	case (pid_t)-1:
		perror("fork");
		break;
	case 0:
		break;
	default:
		exit(0);
	}
	setsid();
	(void) open("/dev/null", O_RDWR, 0);
	dup(0);
	dup(0);
}

/* get command line options */
static void
parseargs(argc, argv)
	int argc;
	char *argv[];
{
	int c;

	while ((c = getopt(argc, argv, "dqw")) != EOF) {
		switch (c) {
		case 'd':
			debugging = 1;
			break;
		case 'q':
			quiet = 1;
			break;
		case 'w':
			warmstart = 1;
			break;
		default:	/* error */
			pfmt(stderr, MM_ACTION, USAGE, rpcbcmd, OPTIONS);
			exit (1);
		}
	}
	if (quiet && debugging) {
		pfmt(stderr, MM_ERROR,
		     ":62:Options %s and %s are incompatible.\n", "-d", "-q");
		pfmt(stderr, MM_ACTION, USAGE, rpcbcmd, OPTIONS);
		exit (1);
	}
}
