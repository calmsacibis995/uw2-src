/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:pong.c	1.5.10.11"
#ident  "$Header: $"

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
*	(c) 1990,1991  UNIX System Laboratories, Inc.
*          All rights reserved.
*/ 

#include <string.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <rpc/rpc.h>
#include <dirent.h>
#include <limits.h>
#include <netdir.h>
#include "yp_b.h"
#include "ypsym.h"

#include <unistd.h>
#include <rpc/nettype.h>
#include <sys/poll.h>
#include <netdir.h>

#ifndef NULL
#define NULL  0
#endif

#define BINDING "/var/yp/binding"
#define YPSERVERS "ypservers"

listofnames *names();
static int set_binding();
static bool firsttime = TRUE;

extern void free_listofnames();
extern int pipe_setdom();
extern void sysvconfig();
#ifdef __STDC__
extern void writeit();
#else
static void writeit();
#endif
extern int yp_getalias();
extern int advanced_mode;

enum clnt_stat yprpc_broadcast();
enum clnt_stat yprpc_broadcast_exp();
#define PINGTIME	10
#define ADVPINGTIME	5

/*
 * prefer datagram networks over vortual circuits, to avoid
 * lots of pending connections.
 */

char *nettypes[] = { "udp", "tcp", NULL };

static char *s_domain;
static struct domain *s_opaque_domain;

static listofnames *listnames;    /* private for do_broadcast and collectservers */
static listofnames *adv_listnames;    /* private for adv_do_broadcast and adv_collectservers */

extern char *gettxt();

static bool_t
collectservers(isok, addr, netconf)
	int *isok;
	struct netbuf *addr;
	struct netconfig *netconf;
{
	struct nd_hostservlist *service;
	char *servername;
	int dofree = 0;
	int ret;
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":6:collectservers: enter\n");
#endif

	if (! *isok) {
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":6:collectservers: got non-serving yp server\n");
#endif
		return(0);
	}

	if (!netdir_getbyaddr(netconf, &service, addr)) {
		servername = service->h_hostservs->h_host;
		dofree = ND_HOSTSERVLIST;
	} else {
		servername = taddr2uaddr(netconf, addr);
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":7:collectservers: got unknown yp server %s\n", 
			servername);
#endif
	}

	if (listnames) {
		register listofnames *list;

		for (list = listnames; list; list = list->nextname) {
			char *name = strtok(list->name," \t\n");

			if (strcmp(servername, name) == NULL)
				break;
		 }
		 if (list == NULL) {
			ret = 0;        /* no match found */
			goto out;
		 }
	}
	ret = 1;
	(void) set_binding(netconf, addr, s_domain,
				    servername, s_opaque_domain);
out:
	if (dofree)
		netdir_free(service, ND_HOSTSERVLIST);

	return(ret);
}

/*
 * try to find a yp server by broadcasting to all nets
 */

static int
do_broadcast(servername, lin)
	char *servername;
	listofnames *lin;
{
	int isok, stat;
	char *nettype = "udp";
	char *pname = s_domain;

#ifdef DEBUG
fprintf(stderr,"do_broadcast: pname = %s lin->name=%s servername=%s\n",pname,lin->name,servername);
#endif
	/*
	 * check for servers, if entry begins with +
	 */

	if (servername && servername[0] == '+')
		listnames = lin;
	else
		listnames = NULL;

	if (servername && servername[1] == '.') {
		nettype = &servername[2];
	}

	if ((stat = rpc_broadcast(YPPROG, YPVERS, YPPROC_DOMAIN_NONACK,
		(xdrproc_t)xdr_ypdomain_wrap_string, (caddr_t)&pname,
		xdr_int, (caddr_t)&isok,
		(resultproc_t)collectservers, nettype)) == RPC_SUCCESS) {
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":8:do_broadcast: isok %d\n", isok);
#endif
		return(0);
	} else {
		blog(gettxt(":9", "do_broadcast broadcast failed: %s\n"), clnt_sperrno(stat));
		return(-1);
	}
}


pong_servers(domain,opaque_domain)
char *domain;
struct domain *opaque_domain; /*to pass back*/
{
	CLIENT *clnt2;
	char *servername;
	char domain_alias[MAXNAMLEN+1];
	char outstring[YPMAXDOMAIN + 256];
	listofnames *list,*lin;
	char serverfile[MAXNAMLEN];
	struct timeval timeout;
	char *pname;
	int count=0;
	int isok, res, tried;
	enum clnt_stat clstat;
	struct netconfig *nconf;

	/*
	 * We are not able to pass these names to all procedures,
	 * so save them into these static variables.
	 */

	s_domain = domain;
	s_opaque_domain = opaque_domain;
#ifdef DEBUG
fprintf(stderr,"pong_servers: domain = %s pid = %d\n",s_domain,getpid());
#endif
	/*
	 * get list of possible servers for this domain
	 */

	/* get alias for domain */
	sysvconfig();
	if (yp_getalias(domain, domain_alias, MAXNAMLEN) < 0)
		pname = domain;
	else
		pname = domain_alias;
	sprintf(serverfile,"%s/%s/%s",BINDING, pname, YPSERVERS);
	list=names(serverfile, count);
	lin=list;
	if (list == NULL) {
		/*
		 * if binding file cannot be found, we try broadcast
		 */
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":10:pong_servers: No binding file, doing broadcast\n");
#endif
		return(do_broadcast(NULL, NULL));
	}
	for (tried = count; list; list = list->nextname, tried--){
		servername=strtok(list->name," \t\n");
		if (servername == NULL) continue;
		if (tried == 0) {
			/*
			 * After ypbind is started up it will not be bound
			 * immediately.  This is normal, no error message
			 * is needed
			 */
			if (firsttime == TRUE) {
				firsttime = FALSE;
			} else {
				sprintf(outstring, gettxt(":5", "yp: server not responding for domain %s; still trying.\n"), domain);
				writeit(outstring);
			}
			tried = count;
		}
		pname=domain;
		if (servername[0] == '*' || servername[0] == '+') {
			if (do_broadcast(servername, lin) == 0) {
				free_listofnames(lin);
				return(0);
			}
		} else {
		    char **typ;
			int err;

		    /*
		     * for all network types (datagram, cots)
		     */
		    for (typ = nettypes; *typ; typ++) {
			if (advanced_mode) {
				timeout.tv_sec=ADVPINGTIME;
			} else {
				timeout.tv_sec=PINGTIME;
			}
			timeout.tv_usec=0;
#ifdef DEBUG
			pfmt(stderr, MM_STD | MM_INFO, ":11:pong_servers: pinging %s nettype %s\n", 
				servername, *typ);
#endif
/* 
 * for advanced mode processing, we will ensure that ypbind does 
 * not call itself recursively. So we get our our netconfig
 * structure and modify the nc_lookups list of libs and
 * eliminate tcpip_nis.so.
 *
 * A cleaner way to do this would have been to define a new
 * entry in /etc/netconfig (i.e. netid = nisudp) that did 
 * not contain the tcpip_nis.so in it's lib list. Unfortunately,
 * older versions of rpcbind protocol (version 3 and earlier) do
 * not deal with unrecognized netids (like nisudp) and fail the
 * rpcb_getaddr lookup when it comes in with this netid. Version 4
 * rpcbind protocol seems to deal with it just fine but there are 
 * rpcbind servers in the world (mainly, NetWare4.0 and 3.11) that
 * do not support version 4. The problem is that the NIS server is 
 * not registered with the nisudp netid and pre version 4 rpcbinds
 * will not match it to a rpcb_getaddr request if it has a different
 * netid (request has netid=nisudp and NIS server is registered 
 * with netid=udp). So we use this method instead. It does not
 * require a new entry in /etc/netconfig and older versions of 
 * the rpcbind protocol deal with it because we use well known
 * netids (udp), and the NIS server is registered with this netid
 * at the remote side.
 */ 
			if (advanced_mode) {
				int i,j;
				nconf = getnetconfigent("udp");

				if (nconf == NULL) {
					blog(gettxt(":12", "ypbind: getnetconfigent failed on udp\n"), servername, clnt_spcreateerror("")); 
					continue;
				} 

				for (i=0; i<nconf->nc_nlookups; i++) {
					if (!strcmp("/usr/lib/tcpip_nis.so",nconf->nc_lookups[i])) {
						for (j=i; j<nconf->nc_nlookups;j++) {
							nconf->nc_lookups[j]=nconf->nc_lookups[j+1];
						} 
						nconf->nc_nlookups--;
					}
				}


				clnt2 = clnt_tp_create(servername, YPPROG, YPVERS,nconf); 
	
			} else {
				clnt2 = clnt_create(servername, YPPROG, YPVERS, *typ);
			}
			if (clnt2 == NULL) {
				blog(gettxt(":12", "ypbind: clnt_create failed to ypserv on %s: %s\n"), 
					servername, clnt_spcreateerror("")); 
				continue;
			}

			if  ((err = clnt_call(clnt2, YPPROC_DOMAIN,
			    (xdrproc_t)xdr_ypdomain_wrap_string, (caddr_t)&pname,
			    xdr_int, (caddr_t)&isok, timeout)) == RPC_SUCCESS) {
			    if (isok) {
				struct netconfig *netconf;
				struct netbuf servaddr;

				netconf = getnetconfigent(clnt2->cl_netid);
				clnt_control(clnt2, CLGET_SVC_ADDR,
						(char *)&servaddr);
				res = set_binding(netconf, &servaddr, domain,
						  servername, opaque_domain);
				clnt_destroy(clnt2);
				free_listofnames(lin);
				return(res);
			    } else {
				blog(gettxt(":13", "pong_servers: server %s doesn't serve domain %s\n"),
				    servername, domain);
			    }
			} else {
				blog(gettxt(":14", "pong_servers: clnt_call failed to %s: %s\n"),
					servername, clnt_sperrno(err));
			}
			clnt_destroy(clnt2);
		    }
		}
	}
	free_listofnames(lin);
	return(-2);
}


/*if it pongs ok*/
static int
set_binding(setnc, setua, domain, servername, opaque_domain)
struct netconfig *setnc;
struct netbuf *setua;
char *domain;
char *servername;
struct domain *opaque_domain;
{
	ypbind_binding setb;
	ypbind_setdom setd;

	setb.ypbind_nconf= setnc;
	setb.ypbind_svcaddr= setua;
	setb.ypbind_servername=servername;
	setb.ypbind_hi_vers=0;
	setb.ypbind_lo_vers=0; /*system will figure this out*/
	setd.ypsetdom_bindinfo= & setb;
	setd.ypsetdom_domain=domain;
#ifdef DEBUG
	{ 
	struct sockaddr_in *sin;

	sin = (struct sockaddr_in *) setb.ypbind_svcaddr->buf;
	pfmt(stderr, MM_STD | MM_INFO, ":15:set_binding: server settings: \n\tserver: %s addr: %s vers: %d,%d\n",
	    setb.ypbind_servername, 
	    inet_ntoa(sin->sin_addr.s_addr),
	    setb.ypbind_lo_vers, 
	    setb.ypbind_hi_vers);
	pfmt(stderr, MM_STD | MM_INFO, ":16:\t nc_lookups %s proto %s protofmly %s\n",
	    *setb.ypbind_nconf->nc_lookups,
	    setb.ypbind_nconf->nc_proto, 
	    setb.ypbind_nconf->nc_protofmly);
	}
#endif

	if (pipe_setdom(&setd, opaque_domain) < 0 ) {
#ifdef DEBUG
		pfmt(stderr, MM_STD | MM_INFO, ":17:set_binding: pipe_setdom failed to server %s\n",
		servername);
#endif
		return(-1);
	}
#ifdef DEBUG
	else pfmt(stderr, MM_STD | MM_INFO, ":18:set_binding: bound to %s\n", servername);
#endif
	return(0);
}

static void
writeit(s)
char *s;
{
	FILE *f;

	if ((f = fopen("/dev/console", "w")) != NULL) {
		(void) pfmt(f, MM_NOSTD, ":19:%s.\n", s);
		(void) fclose(f);
	}
}
int
findalt(opaque_domain)
struct domain *opaque_domain; /*to pass back*/
{
	CLIENT *clnt2;
	char *servername;
	char domain_alias[MAXNAMLEN+1];
	listofnames *list,*lin;
	char serverfile[MAXNAMLEN];
	struct timeval timeout;
	char *pname;
	int count=0;
	int isok, res, tried;
	char *domain;
	int err;
	struct netconfig *nconf;

	if (opaque_domain->dom_broadcaster_pid)
		return(1);

	timeout.tv_sec = 3;
	timeout.tv_usec = 0;

	/*
	 * We are not able to pass these names to all procedures,
	 * so save them into these static variables.
	 */

	s_domain = domain = opaque_domain->dom_name;

	pfmt(stdout, MM_NOSTD, ":10:findalt: No binding file, doing broadcast\n");
	/*
	 * get list of possible servers for this domain
	 */
	if (yp_getalias(domain, domain_alias, MAXNAMLEN) < 0)
		pname = domain;
	else
		pname = domain_alias;
	sprintf(serverfile,"%s/%s/%s",BINDING, pname, YPSERVERS);
	list=names(serverfile, count);
	lin=list;
	if ((list == NULL)) {
		/*
		 * if binding file cannot be found, we try broadcast
		 */
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":10:findalt: No binding file, doing broadcast\n");
#endif
		if (!adv_do_broadcast(NULL, NULL)){
			return(1);
		} else return(0);
	}
	for (tried = count; list; list = list->nextname, tried--){
		servername=strtok(list->name," \t\n");
		if (servername[0] == '*' || servername[0] == '+') {
			if (adv_do_broadcast(servername, lin) == 0) {
				free_listofnames(lin);
				return(1);
			}
		}  else {
		if (servername == NULL || ((strcmp(servername,opaque_domain->dom_binding->ypbind_servername))==0))  {
			continue;
		} else {
			int i,j;
			nconf = getnetconfigent("udp");

			if (nconf == NULL) {
				blog(gettxt(":12", "findalt: getnetconfigent failed on udp"), servername, clnt_spcreateerror("")); 
				continue;
			} 

			for (i=0; i<nconf->nc_nlookups; i++) {
				if (!strcmp("/usr/lib/tcpip_nis.so",nconf->nc_lookups[i])) {
					for (j=i; j<nconf->nc_nlookups;j++) {
						nconf->nc_lookups[j]=nconf->nc_lookups[j+1];
					}
					nconf->nc_nlookups--;
				}
			}


			clnt2 = clnt_tp_create(servername, YPPROG, YPVERS,nconf); 
	
			if(clnt2 == NULL){
				fprintf(stderr,"findalt: ERROR: clnt_tli_create failed \n");
				blog(gettxt(":12", "findalt: clnt_create failed to ypserv on %s: %s\n"), servername, clnt_spcreateerror("")); 
				continue;
			}
		
				CLNT_CONTROL(clnt2, CLSET_TIMEOUT, (char *)&timeout);
				CLNT_CONTROL(clnt2, CLGET_TIMEOUT, (char *)&timeout);
				if  ((err = clnt_call(clnt2, YPPROC_DOMAIN, (xdrproc_t)xdr_ypdomain_wrap_string, (caddr_t)&pname, xdr_int, (caddr_t)&isok, timeout)) == RPC_SUCCESS) {
					if (isok) {
						clnt_destroy(clnt2);
						return(1);
					}
				}
				clnt_destroy(clnt2);
		} /* end else */
		}
	} /* for loop */
}

static bool_t
adv_collectservers(isok, addr, netconf)
	int *isok;
	struct netbuf *addr;
	struct netconfig *netconf;
{
	struct nd_hostservlist *service;
	char *servername;
	int dofree = 0;
	int ret;

	if (! *isok) {
		pfmt(stdout, MM_NOSTD, ":6:adv_collectservers: got non-serving yp server\n");
		return(0);
	}

	if (!netdir_getbyaddr(netconf, &service, addr)) {
		servername = service->h_hostservs->h_host;
		dofree = ND_HOSTSERVLIST;
	} else {
		servername = taddr2uaddr(netconf, addr);
		pfmt(stdout, MM_NOSTD, ":7:adv_collectservers: got unknown yp server %s\n", 
			servername);
	}

	if (adv_listnames) {
		register listofnames *list;

		for (list = adv_listnames; list; list = list->nextname) {
			char *name = strtok(list->name," \t\n");

			if (strcmp(servername, name) == NULL)
				break;
		 }
		 if (list == NULL) {
			ret = 0;        /* no match found */
			goto out;
		 }
	}
	ret = 1;
out:
/*
	if (dofree)
		netdir_free(service, ND_HOSTSERVLIST);
*/

	return(ret);
}

static int
adv_do_broadcast(servername, lin)
	char *servername;
	listofnames *lin;
{
	int isok, stat;
	char *pname = s_domain;
	char *nettype = "udp";

	/*
	 * check for servers, if entry begins with +
	 */

	if (servername && servername[0] == '+')
		adv_listnames = lin;
	else {
		adv_listnames = NULL;
}

	if ((stat = yprpc_broadcast(YPPROG, YPVERS, YPPROC_DOMAIN_NONACK,
		(xdrproc_t)xdr_ypdomain_wrap_string, (caddr_t)&pname,
		xdr_int, (caddr_t)&isok,
		(resultproc_t)adv_collectservers, nettype)) == RPC_SUCCESS) {
#ifdef DEBUG
		pfmt(stdout, MM_NOSTD, ":8:adv_do_broadcast: isok %d\n", isok);
#endif
		return(0);
	} else {
		return(-1);
	}
}


#define	MAXBCAST 20	/* Max no of broadcasting transports */
#define	INITTIME 4000	/* Time to wait initially */
#define	WAITTIME 8000	/* Maximum time to wait */


/*
 * The following two functions (yprpc_braodcast_exp and yprpc_broadcast)
 * are essentially dupilcates of the rpc_broadcast functions in 
 * libnsl with one exception. The function, yprpc_broadcast_exp 
 * modifies the netconfig structure and removes the tcpip_nis.so
 * lib from the nc_lookups list so that ypbind does not recursively
 * call itself during a netdir_getbyname function. It's ugly but
 * there is really no good way to do this. The recursive call to itself
 * is only a problem when the server is not responding.
 *
 *
 * The current parameter xdr packet size is limited by the max tsdu
 * size of the transport. If the max tsdu size of any transport is
 * smaller than the parameter xdr packet, then broadcast is not
 * sent on that transport.
 *
 * Also, the packet size should be less the packet size of
 * the data link layer (for ethernet it is 1400 bytes).  There is
 * no easy way to find out the max size of the data link layer and
 * we are assuming that the args would be smaller than that.
 *
 * The result size has to be smaller than the transport tsdu size.
 *
 */


enum clnt_stat
yprpc_broadcast_exp(prog, vers, proc, xargs, argsp, xresults, resultsp,
	eachresult, inittime, waittime, nettype)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
	int 		inittime;	/* how long to wait initially */
	int 		waittime;	/* maximum time to wait */
	char		*nettype;	/* transport type */
{
	enum clnt_stat	stat = RPC_SUCCESS;	/* Return status */
	XDR 		xdr_stream;		/* XDR stream */
	register XDR 	*xdrs = &xdr_stream;
	struct rpc_msg	msg;			/* RPC message */
	struct timeval	t;
	char 		*outbuf = NULL;		/* Broadcast msg buffer */
	char		*inbuf = NULL;		/* Reply buf */
	long 		maxbufsize = 0;
	AUTH 		*sys_auth = authsys_create_default();
	register int	i, j;
	void		*handle;
	char		uaddress[1024];		/* A self imposed limit */
	char		*uaddrp = uaddress;
	int 		pmap_reply_flag;	/* reply recvd from PORTMAP */
	/* An array of all the suitable broadcast transports */
	struct {
		int fd;				/* File descriptor */
		struct netconfig *nconf;	/* Netconfig structure */
		u_int asize;			/* Size of the addr buf */
		u_int dsize;			/* Size of the data buf */
		struct netbuf raddr;		/* Remote address */
		struct nd_addrlist *nal;	/* Broadcast addrs */
	} fdlist[MAXBCAST];
	struct pollfd pfd[MAXBCAST];
	register int 	fdlistno = 0;
	struct r_rpcb_rmtcallargs barg;		/* Remote arguments */
	struct r_rpcb_rmtcallres bres;		/* Remote results */
	struct t_unitdata t_udata, t_rdata;
	struct netconfig *nconf;
	struct nd_hostserv hs;
	int msec;
	int pollretval;
	int fds_found;


	if (sys_auth == (AUTH *)NULL) {
		return (RPC_SYSTEMERROR);
	}
	/*
	 * initialization: create a fd, a broadcast address, and send the
	 * request on the broadcast transport.
	 * Listen on all of them and on replies, call the user supplied
	 * function.
	 */

	if (nettype == NULL)
		nettype = "datagram_n";
	if ((handle = _rpc_setconf(nettype)) == NULL) {
		return (RPC_UNKNOWNPROTO);
	}
	while (nconf = _rpc_getconf(handle)) {
		struct t_info tinfo;
		int fd;
		u_int addrlen;
		int i,j;

	for (i=0; i<nconf->nc_nlookups; i++) {
		if (!strcmp("/usr/lib/tcpip_nis.so",nconf->nc_lookups[i])) {
			for (j=i; j<nconf->nc_nlookups;j++) {
				nconf->nc_lookups[j]=nconf->nc_lookups[j+1];
			}
			nconf->nc_nlookups--;
		}
	}
		if (nconf->nc_semantics != NC_TPI_CLTS)
			continue;
		if (fdlistno >= MAXBCAST)
			break;	/* No more slots available */
		if ((fd = t_open(nconf->nc_device, O_RDWR, &tinfo)) == -1) {
			stat = RPC_CANTSEND;
			continue;
		}
		if (t_bind(fd, (struct t_bind *)NULL,
			(struct t_bind *)NULL) == -1) {
			(void) t_close(fd);
			stat = RPC_CANTSEND;
			continue;
		}
		/* Do protocol specific negotiating for broadcast */
		if (netdir_options(nconf, ND_SET_BROADCAST, fd, NULL)) {
			(void) t_close(fd);
			stat = RPC_NOBROADCAST;
			continue;
		}
		fdlist[fdlistno].fd = fd;
		fdlist[fdlistno].nconf = nconf;
		if (((addrlen = _rpc_get_a_size(tinfo.addr)) == 0) ||
		    ((fdlist[fdlistno].raddr.buf =
		      (char *)malloc(addrlen)) == NULL)) {
			t_close(fd);
			stat = RPC_SYSTEMERROR;
			goto done_broad;
		}
		fdlist[fdlistno].raddr.maxlen = addrlen;
		fdlist[fdlistno].raddr.len = addrlen;
		pfd[fdlistno].events = POLLIN | POLLPRI |
			POLLRDNORM | POLLRDBAND;
		pfd[fdlistno].fd = fdlist[fdlistno].fd = fd;
		fdlist[fdlistno].asize = addrlen;

		if ((fdlist[fdlistno].dsize = _rpc_get_t_size(0,
						tinfo.tsdu)) == 0) {
			t_close(fd);
			free(fdlist[fdlistno].raddr.buf);
			stat = RPC_SYSTEMERROR; /* XXX */
			goto done_broad;
		}

		if (maxbufsize <= fdlist[fdlistno].dsize)
			maxbufsize = fdlist[fdlistno].dsize;
		fdlistno++;
	}

	if (fdlistno == 0) {
		if (stat == RPC_SUCCESS)
			stat = RPC_UNKNOWNPROTO;
		goto done_broad;
	}
	if (maxbufsize == 0) {
		if (stat == RPC_SUCCESS)
			stat = RPC_CANTSEND;
		goto done_broad;
	}
	inbuf = (char *)malloc(maxbufsize);
	outbuf = (char *)malloc(maxbufsize);
	if ((inbuf == NULL) || (outbuf == NULL)) {
		stat = RPC_SYSTEMERROR;
		goto done_broad;
	}

	/* Serialize all the arguments which have to be sent */
	(void) gettimeofday(&t, (struct timezone *) 0);
	msg.rm_xid = getpid() ^ t.tv_sec ^ t.tv_usec;
	msg.rm_direction = CALL;
	msg.rm_call.cb_rpcvers = RPC_MSG_VERSION;
	msg.rm_call.cb_prog = RPCBPROG;
	msg.rm_call.cb_vers = RPCBVERS;
	msg.rm_call.cb_proc = RPCBPROC_CALLIT;
	barg.prog = prog;
	barg.vers = vers;
	barg.proc = proc;
	barg.args.args_val = argsp;
	barg.xdr_args = xargs;
	bres.addr = uaddrp;
	bres.results.results_val = resultsp;
	bres.xdr_res = xresults;
	msg.rm_call.cb_cred = sys_auth->ah_cred;
	msg.rm_call.cb_verf = sys_auth->ah_verf;
	xdrmem_create(xdrs, outbuf, maxbufsize, XDR_ENCODE);
	if ((! xdr_callmsg(xdrs, &msg)) ||
	    (! xdr_rpcb_rmtcallargs(xdrs, &barg))) {
		stat = RPC_CANTENCODEARGS;
		goto done_broad;
	}
	t_udata.opt.len = 0;
	t_udata.udata.buf = outbuf;
	t_udata.udata.len = xdr_getpos(xdrs);
	t_udata.udata.maxlen = t_udata.udata.len;
	/* XXX Should have set opt to its legal maxlen. */
	t_rdata.opt.len = t_rdata.opt.maxlen = 0;
	xdr_destroy(xdrs);


	/*
	 * Basic loop: broadcast the packets to transports which
	 * support data packets of size such that one can encode
	 * all the arguments.
	 * Wait a while for response(s).
	 * The response timeout grows larger per iteration.
	 */
	hs.h_host = HOST_BROADCAST;
	hs.h_serv = "rpcbind";

	for (msec = inittime; msec <= waittime; msec += msec) {
		/* Broadcast all the packets now */
		for (i = 0; i < fdlistno; i++) {
			struct nd_addrlist *addrlist;

			if (fdlist[i].dsize < t_udata.udata.len) {
				stat = RPC_CANTSEND;
				continue;
			}
			if (netdir_getbyname(fdlist[i].nconf, &hs, &addrlist) ||
			    (addrlist->n_cnt == 0)) {
				stat = RPC_N2AXLATEFAILURE;
				continue;
			}
			for (j = 0; j < addrlist->n_cnt; j++) {
				struct netconfig *nconf = fdlist[i].nconf;
				t_udata.addr = addrlist->n_addrs[j];
				if (t_sndudata(fdlist[i].fd, &t_udata)) {
#ifdef RPC_DEBUG
					t_error("rpc_broadcast: t_sndudata");
#endif
					stat = RPC_CANTSEND;
					continue;
				}
#ifdef RPC_DEBUG
				fprintf(stderr,
				    "Broadcast packet sent for %s\n",
				    nconf->nc_netid);
#endif
			} /* End for sending all packets on this transport */
			(void) netdir_free((char *)addrlist, ND_ADDRLIST);
		} /* End for sending on all transports */

		if (eachresult == NULL) {
			stat = RPC_SUCCESS;
			goto done_broad;
		}

		/*
		 * Get all the replies from these broadcast requests
		 */
	recv_again:

		switch (pollretval = poll(pfd, fdlistno, msec)) {
		case 0:		/* timed out */
			stat = RPC_TIMEDOUT;
			continue;
		case -1:	/* some kind of error - we ignore it */
			goto recv_again;
		}		/* end of poll results switch */

		t_rdata.udata.buf = inbuf;

		for (i = fds_found = 0;
			i < fdlistno && fds_found < pollretval; i++) {

			int flag;
			bool_t	done = FALSE;

			if (pfd[i].revents == 0)
				continue;
			else if (pfd[i].revents & POLLNVAL) {
			/*
			 * Something bad has happened to this descriptor.
			 * Poll(BA_OS) says we can cause poll() to ignore
			 * it simply by using a negative fd.  We do that
			 * rather than compacting the pfd[] and fdlist[]
			 * arrays.
			 */
				pfd[i].fd = -1;
				fds_found++;
				continue;
			} else
				fds_found++;
#ifdef RPC_DEBUG
			fprintf(stderr, "response for %s\n",
				fdlist[i].nconf->nc_netid);
#endif
		try_again:
			t_rdata.udata.maxlen = fdlist[i].dsize;
			t_rdata.udata.len = 0;
			t_rdata.addr = fdlist[i].raddr;
			if (t_rcvudata(fdlist[i].fd, &t_rdata, &flag) == -1) {
				if (t_errno == TLOOK) {
					int lookres;

					lookres = t_look(fdlist[i].fd);
					if ((lookres & T_UDERR)
					 && (t_rcvuderr(fdlist[i].fd,
							(struct t_uderr *) 0)
					     < 0)) {
					}
					if (lookres & T_DATA)
						goto try_again;
				} else if ((t_errno == TSYSERR)
					&& (errno == EINTR)) {
					goto try_again;
				}
				stat = RPC_CANTRECV;
				continue;
			}
			/*
			 * Not taking care of flag for T_MORE.
			 * We are assuming that
			 * such calls should not take more than one
			 * transport packet.
			 */
			if (flag & T_MORE)
				continue; /* Drop that and go ahead */
			if (t_rdata.udata.len < sizeof (u_long))
				continue; /* Drop that and go ahead */
			/*
			 * see if reply transaction id matches sent id.
			 * If so, decode the results. If return id is xid + 1
			 * it was a PORTMAP reply
			 */
			if (*((u_long *)(inbuf)) == *((u_long *)(outbuf))) {
				pmap_reply_flag = 0;
				msg.acpted_rply.ar_verf = _null_auth;
				msg.acpted_rply.ar_results.where =
					(caddr_t)&bres;
				msg.acpted_rply.ar_results.proc =
					(xdrproc_t)xdr_rpcb_rmtcallres;
			} else
				continue;
			xdrmem_create(xdrs, inbuf,
				(u_int)t_rdata.udata.len, XDR_DECODE);
			if (xdr_replymsg(xdrs, &msg)) {
				if ((msg.rm_reply.rp_stat == MSG_ACCEPTED) &&
				    (msg.acpted_rply.ar_stat == SUCCESS)) {
					struct netbuf *taddr;
						taddr = uaddr2taddr(
							fdlist[i].nconf,
							uaddrp);
					done = (*eachresult)(resultsp, taddr,
							fdlist[i].nconf);
#ifdef RPC_DEBUG
				{
					int k;

					printf("rmt addr = ");
					for (k = 0; k < taddr->len; k++)
						printf("%d ", taddr->buf[k]);
					printf("\n");
				}
#endif
					if (taddr && !pmap_reply_flag)
						netdir_free((char *)taddr,
							    ND_ADDR);
				}
				/* otherwise, we just ignore the errors ... */
			}
			/* else some kind of deserialization problem ... */

			xdrs->x_op = XDR_FREE;
			msg.acpted_rply.ar_results.proc = (xdrproc_t) xdr_void;
			(void) xdr_replymsg(xdrs, &msg);
			(void) (*xresults)(xdrs, resultsp);
			XDR_DESTROY(xdrs);
			if (done) {
				stat = RPC_SUCCESS;
				goto done_broad;
			} else {
				goto recv_again;
			}
		}		/* The recv for loop */
	}			/* The giant for loop */

done_broad:
	if (inbuf)
		(void) free(inbuf);
	if (outbuf)
		(void) free(outbuf);
	for (i = 0; i < fdlistno; i++) {
		(void) t_close(fdlist[i].fd);
		(void) free(fdlist[i].raddr.buf);
	}
	AUTH_DESTROY(sys_auth);
	(void) _rpc_endconf(handle);

	return (stat);
}


enum clnt_stat
yprpc_broadcast(prog, vers, proc, xargs, argsp, xresults, resultsp,
			eachresult, nettype)
	u_long		prog;		/* program number */
	u_long		vers;		/* version number */
	u_long		proc;		/* procedure number */
	xdrproc_t	xargs;		/* xdr routine for args */
	caddr_t		argsp;		/* pointer to args */
	xdrproc_t	xresults;	/* xdr routine for results */
	caddr_t		resultsp;	/* pointer to results */
	resultproc_t	eachresult;	/* call with each result obtained */
	char		*nettype;	/* transport type */
{
	enum clnt_stat	dummy;

	dummy = yprpc_broadcast_exp(prog, vers, proc, xargs, argsp,
		xresults, resultsp, eachresult,
		INITTIME, WAITTIME, nettype);
	return (dummy);
}
