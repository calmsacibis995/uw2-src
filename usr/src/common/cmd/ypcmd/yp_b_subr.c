/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:yp_b_subr.c	1.5.8.8"
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

#include <signal.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <netconfig.h>
#include <netdir.h>
#include <sys/wait.h>
#include "yp_b.h"
#include <netinet/in.h>
struct sockaddr_in *sin;
#define PINGTOTTIM 20	/*Total seconds for ping timeout*/
#define ADVMODEPING 1	/*Advanced mode Total seconds for ping timeout*/

static void broadcast_setup();
static struct ypbind_binding *dup_ypbind_binding();
static void enable_exit();
static int ypbind_find();
static bool ypbind_ping();
static struct domain *ypbind_point_to_domain();

extern struct netconfig *getnetconfigent();
extern int setok;

extern void *calloc();
extern int close();
extern void exit();
extern long fork();
extern void free();
extern int gethostname();
extern void *malloc();
extern int pipe();
extern int pong_servers();
extern int strcmp();
extern char *strcpy();
extern int findalt();

extern int errno;
extern char *gettxt();
extern int advanced_mode;

#ifdef DEBUG
static char *b_errs[] = {"", "YPBIND_ERR_ERR", "YPBIND_ERR_NOSERV",
	"YPBIND_ERR_RESC", "YPBIND_ERR_NODOMAIN"};

#endif

/*ARGSUSED*/
void	*
ypbindproc_null_3(argp, clnt)
void	*argp;
CLIENT *clnt;
{
	static char	res;

	memset((char *) & res, 0, sizeof(res));
	return ((void *) & res);
}

/*ARGSUSED*/
ypbind_resp *
ypbindproc_domain_3(argp, clnt)
ypbind_domain *argp;
CLIENT *clnt;
{
	static ypbind_resp res;
	struct domain *current_domain;
	int retry=0;

	memset((char *) & res, 0, sizeof(res));

#ifdef DEBUG
	pfmt(stderr, MM_STD | MM_INFO, ":20:ypbind_domain: domain: %s pid = %d\n", argp->ypbind_domainname,getpid());
#endif

	if ((current_domain = ypbind_point_to_domain(argp->ypbind_domainname)) != 
	    (struct domain *) NULL) {

		/*
		 * Ping the server to make sure it is up.
		 */


		if (current_domain->dom_boundp) {
#ifdef DEBUG
			sin = (struct sockaddr_in *)
				current_domain->dom_binding->ypbind_svcaddr->buf;
			pfmt(stderr, MM_STD | MM_INFO, ":21:ypbind_domain: domain is bound pinging: %s\n", 
					inet_ntoa(sin->sin_addr.s_addr));
#endif
			retry = ypbind_ping(current_domain);
		}

		/*
		 * Bound or not, return the current state of the binding.
		 */

		if (current_domain->dom_boundp) {
#ifdef DEBUG
			pfmt(stderr, MM_STD | MM_INFO, ":22:ypbind_domain: domain is bound returning: %s\n", 
				*argp);
			fflush(stderr);
#endif
			res.ypbind_status = YPBIND_SUCC_VAL;
			res.ypbind_resp_u.ypbind_bindinfo =
			    current_domain->dom_binding;
		} else {
#ifdef DEBUG
			pfmt(stderr, MM_STD | MM_INFO, ":23:ypbind_domain: domain is NOT bound returning: %s err: %s\n",
			argp->ypbind_domainname, b_errs[current_domain->dom_error]);
			fflush(stderr);
#endif
			res.ypbind_status = YPBIND_FAIL_VAL;
			if (advanced_mode) {
				retry = ypbind_ping(current_domain);
				if (retry == YPERR_YPSERV ) {
					res.ypbind_resp_u.ypbind_error = 
					YPERR_YPSERV;
				} else {
					res.ypbind_resp_u.ypbind_error =
				    	current_domain->dom_error;
				}
			} else {
				res.ypbind_resp_u.ypbind_error =
				current_domain->dom_error;
			}
		}

	} else {
		res.ypbind_status = YPBIND_FAIL_VAL;
		res.ypbind_resp_u.ypbind_error = YPBIND_ERR_RESC;
	}
	/* should look for answer if not found */
	/* Also should check version level and look if version is
			too low */
	if (res.ypbind_status == YPBIND_FAIL_VAL) {
		ypbind_find(current_domain);
	}
	return (&res);
}


/*ARGSUSED*/
void	*
ypbindproc_setdom_3(argp, clnt, transp)
ypbind_setdom *argp;
CLIENT *clnt;
SVCXPRT *transp;
{
	static char	res;
	struct domain *a_domain;
	struct netbuf *who;
	extern int _nderror;

	if (transp != NULL) {
		/* find out who originated the request */
		who = svc_getrpccaller(transp);
#ifdef DEBUG
		sin = (struct sockaddr_in *)who->buf;
		pfmt(stderr, MM_STD | MM_INFO, ":24:ypbind_setdom: from %s setok %d\n", 
			inet_ntoa(sin->sin_addr.s_addr), setok);
#endif

		if (setok == FALSE || setok == YPSETLOCAL) {
			struct nd_hostservlist *hostservs = NULL;
			struct netbuf addr;
			struct netconfig *nconf;
			char localhost[64+1];
			int i=0;
			int j=0;

			if ((nconf = getnetconfigent(transp->xp_netid))
			    == (struct netconfig *)NULL) {
				svcerr_systemerr(transp);
				return(0);
			}
			addr.maxlen = who->maxlen;
			addr.len = who->len;
			if (!(addr.buf = malloc(who->maxlen))) {
				blog(gettxt(":25", "ypbindproc_setdom_3: malloc failed.\n"));
				return(0);
			}
			memcpy(addr.buf, who->buf, who->len);
			/*
			 * ypbind is about to call netdir_getbyaddr.
			 * Make sure it does not recurse on itself.
			 */
			for (i=0; i<nconf->nc_nlookups; i++) {
				if (!strcmp("/usr/lib/tcpip_nis.so",nconf->nc_lookups[i])) {
					for (j=i; j<nconf->nc_nlookups;j++) {
						nconf->nc_lookups[j]=nconf->nc_lookups[j+1];
					} 
					nconf->nc_nlookups--;
				}
			}
			netdir_getbyaddr(nconf, &hostservs, &addr);
			if (hostservs == NULL) {
				sin = (struct sockaddr_in *)who->buf;
				blog(
					gettxt(":26", "ypbind_setdom: netdir_getbyaddr failed: host %s addr %s _nderror %d\n"),
				    argp->ypsetdom_bindinfo->ypbind_servername,
					inet_ntoa(sin->sin_addr.s_addr), _nderror);
				return(0);
			}
			switch (setok) { 
			case FALSE:
				blog(gettxt(":27", "ypbind: Set domain request to host %s failed (ypset not allowed)"),
				    argp->ypsetdom_bindinfo->ypbind_servername);
				blog(gettxt(":28", "from host %s, failed (ypset not allowed)!\n"),
				    hostservs->h_hostservs->h_host);
				svcerr_systemerr(transp);
				return(0);
			case YPSETLOCAL:
				gethostname(localhost, 64);
				blog(gettxt(":29", "ypbind: ypsetlocal: localhost %s host %s\n"),
				    localhost, hostservs->h_hostservs->h_host);
				if (strcmp(hostservs->h_hostservs->h_host,
				    localhost) != 0) {
					blog(gettxt(":30", "ypbind: Set domain request to host %s, "),
					    argp->ypsetdom_bindinfo->ypbind_servername);
					blog(gettxt(":31", "from host %s, failed (not local).\n"),
					    hostservs->h_hostservs->h_host);
					svcerr_systemerr(transp);
					return(0);
				}
			}
			netdir_free((char *)hostservs, ND_HOSTSERVLIST);
		}
	}

	memset((char *) & res, 0, sizeof(res));

	if ( (a_domain = ypbind_point_to_domain(argp->ypsetdom_domain) )
	    != (struct domain *) NULL) {
		a_domain->dom_boundp = TRUE;
		a_domain->dom_yps_complete = FALSE;
#ifdef DEBUG
		pfmt(stderr, MM_STD | MM_INFO, ":32:ypbind_setdom: setting domain: %s\n", argp->ypsetdom_domain);
#endif
		/* this does the set --should copy the structure */
		a_domain->dom_binding = 
			dup_ypbind_binding(argp->ypsetdom_bindinfo);

		/* get rid of old pinging client if one exists */
		if (a_domain->ping_clnt != (CLIENT * )NULL) {

			clnt_destroy(a_domain->ping_clnt);
			a_domain->ping_clnt = (CLIENT * )NULL;
		}
	}

	return ((void *) & res);
}


/*
 * This returns a pointer to a domain entry.  If no such domain existed on
 * the list previously, an entry will be allocated, initialized, and linked
 * to the list.  Note:  If no memory can be malloc-ed for the domain structure,
 * the functional value will be (struct domain *) NULL.
 */
static struct domain *known_domains;
static struct domain *
ypbind_point_to_domain(pname)
register char	*pname;
{
	register struct domain *pdom;
	char *strdup();

	for (pdom = known_domains; pdom != (struct domain *)NULL;
	    pdom = pdom->dom_pnext) {
		if (!strcmp(pname, pdom->dom_name))
			return (pdom);
	}

	/* Not found.  Add it to the list */

	if (pdom = (struct domain *)calloc(1, sizeof (struct domain ))) {
		pdom->dom_name=strdup(pname);
		if (pdom->dom_name==NULL){
			free((char *)pdom);
			pfmt(stderr, MM_STD, ":33:ypbind_point_to_domain: strdup failed\n");
			return(NULL);
		}
		pdom->dom_pnext = known_domains;
		known_domains = pdom;
		pdom->dom_boundp = FALSE;
		pdom->dom_yps_complete = FALSE;
		pdom->dom_binding = NULL;
		pdom->dom_error = YPBIND_ERR_NOSERV;
		pdom->ping_clnt = (CLIENT * )NULL;
		pdom->dom_report_success = -1;
		pdom->dom_broadcaster_pid = 0;
		pdom->bindfile = -1;
	}
	else 
		pfmt(stderr, MM_STD, ":34:ypbind_point_to_domain: malloc failed\n");

	return (pdom);
}


static bool
ypbind_ping(pdom)
struct domain *pdom;
{
	enum clnt_stat clnt_stat;
	struct timeval timeout;
	int	vers;
	struct rpc_err rpcerr;
	int	isok;
	char	*pname;
	bool new_binding = FALSE;
	CLIENT	*hold;

	if (advanced_mode) {
		if (pdom->dom_boundp == FALSE) { 
			if ((findalt(pdom)) == 1 ) {
				return (new_binding);
			} else {
				new_binding=YPERR_YPSERV;
				return (new_binding);
			}
		}
	} else if (pdom->dom_boundp == FALSE) { 
			return (FALSE);
	}

	if (pdom->dom_yps_complete)
		vers = pdom->dom_binding->ypbind_hi_vers;
	else 
		vers = 0;

	if (pdom->ping_clnt == (CLIENT *) NULL) {
#ifdef DEBUG
	pfmt(stderr, MM_STD | MM_INFO, ":35:ypbind_ping: device: %s netid: %s ",
		    pdom->dom_binding->ypbind_nconf->nc_device,
		    pdom->dom_binding->ypbind_nconf->nc_netid);
	sin = (struct sockaddr_in *)pdom->dom_binding->ypbind_svcaddr->buf;
	pfmt(stderr, MM_STD | MM_INFO, ":36:addr: %s\n", inet_ntoa(sin->sin_addr.s_addr));
	pfmt(stderr, MM_STD | MM_INFO, ":37:ypbind_ping: supports versions %d thru %d\n",
	    pdom->dom_binding->ypbind_lo_vers,
	    pdom->dom_binding->ypbind_hi_vers);
	pfmt(stderr, MM_STD | MM_INFO, ":38:\t nc_lookups %s proto %s protofmly %s\n\n",
	    *(pdom->dom_binding->ypbind_nconf->nc_lookups),
	    pdom->dom_binding->ypbind_nconf->nc_proto,
	    pdom->dom_binding->ypbind_nconf->nc_protofmly);
#endif
		if (pdom->dom_binding->ypbind_nconf->nc_semantics ==
		    (unsigned long) NC_TPI_CLTS ||
		    (strcmp(pdom->dom_binding->ypbind_nconf->nc_netid, "tcp") 
		    == 0))
{
			pdom->ping_clnt = clnt_tli_create(RPC_ANYFD,
			    pdom->dom_binding->ypbind_nconf,
			    pdom->dom_binding->ypbind_svcaddr,
			    YPPROG, vers, 0, 0);
}
		else
			pdom->ping_clnt = clnt_tp_create(
			    pdom->dom_binding->ypbind_servername,
			    YPPROG, vers, pdom->dom_binding->ypbind_nconf);
	}

	if (pdom->ping_clnt == (CLIENT *) NULL) {
		pfmt(stderr, MM_STD | MM_NOGET, "%s: %s\n",
			"clnt_create", strerror(errno));
		clnt_pcreateerror(gettxt(":39", "ypbind_ping()"));
		pdom->dom_boundp = FALSE;
		pdom->dom_yps_complete = FALSE;
		pdom->dom_error = YPBIND_ERR_NOSERV;
		return(FALSE);
	}


#ifdef DEBUG
	pfmt(stderr, MM_STD | MM_INFO, ":40:ypbind_ping: clnt: %x yps_complete %d \n", 
		pdom->ping_clnt, pdom->dom_yps_complete);
#endif
	if (advanced_mode)
		timeout.tv_sec = ADVMODEPING;
	else
		timeout.tv_sec = PINGTOTTIM;
	timeout.tv_usec =  0;
	if (!pdom->dom_yps_complete) {
		/* Find out versions */
		if ((clnt_stat = (enum clnt_stat) clnt_call(pdom->ping_clnt,
		    YPPROC_NULL, xdr_void, 0, xdr_void, 0, timeout)) == RPC_SUCCESS) {
#ifdef DEBUG
			pfmt(stderr, MM_STD | MM_INFO, ":41:ypbind: Error couldn't find versions!!!\n");
#endif
			pdom->dom_boundp = FALSE;
			pdom->dom_yps_complete = FALSE;
			pdom->dom_error = YPBIND_ERR_NOSERV;
			return(new_binding);
			/* Didn't find versions */
		} else if (clnt_stat == RPC_PROGVERSMISMATCH) {
			clnt_geterr(pdom->ping_clnt, &rpcerr);
			pdom->dom_binding->ypbind_lo_vers = rpcerr.re_vers.low;
			pdom->dom_binding->ypbind_hi_vers = rpcerr.re_vers.high;
			pdom->dom_boundp = TRUE;
			pdom->dom_yps_complete = TRUE;
#ifdef DEBUG
			pfmt(stderr, MM_STD | MM_INFO, ":42:ypbind_ping: Server pinged successfully, supports versions %d thru %d\n",
			    pdom->dom_binding->ypbind_lo_vers,
			    pdom->dom_binding->ypbind_hi_vers);
#endif
			hold=pdom->ping_clnt;
			pdom->ping_clnt = (CLIENT *) NULL;
			new_binding=ypbind_ping(pdom);
			if (hold) clnt_destroy(hold);
			return(new_binding);
		} else
		{
			clnt_perror(pdom->ping_clnt, 
				gettxt(":43", "ypbind_ping: ping for versions"));
			pdom->dom_boundp = FALSE;
			if (pdom->ping_clnt)
				clnt_destroy(pdom->ping_clnt);
			pdom->ping_clnt = (CLIENT *)NULL;
		}
	}
	if (pdom->dom_boundp) {
		pname = pdom->dom_name;
		CLNT_CONTROL(pdom->ping_clnt, CLSET_TIMEOUT, (char *)&timeout);
		if ((clnt_stat = (enum clnt_stat) clnt_call(pdom->ping_clnt,
		    YPPROC_DOMAIN,
		    (xdrproc_t)xdr_ypdomain_wrap_string, (caddr_t)&pname,
		    xdr_int, (caddr_t)&isok, timeout)) == RPC_SUCCESS) {
			pdom->dom_boundp = isok;
#ifdef DEBUG
			pfmt(stderr, MM_STD | MM_INFO, ":44:ypbind_ping: server status for domain %s is %d\n", 
				pname, isok);
#endif
			return(new_binding);

		} else	{
			clnt_perror(pdom->ping_clnt, gettxt(":45", "ypbind_ping:ping error"));
			pdom->dom_boundp = FALSE;
			pdom->dom_yps_complete = FALSE;
			pdom->dom_error = YPBIND_ERR_NOSERV;
			if (advanced_mode) {
				if ((findalt(pdom)) == 1 ) {
				} else {
					new_binding=YPERR_YPSERV;
				}
			}
		}
	}
#ifdef DEBUG
	pfmt(stderr, MM_STD | MM_INFO, ":46:ypbind_ping: rpc call returned %s\n", 
		clnt_sperrno(clnt_stat));
#endif
	/* Could leave this open */
	if (pdom->ping_clnt)
		clnt_destroy(pdom->ping_clnt);
	pdom->ping_clnt = (CLIENT *)NULL;
	return(new_binding);
}

static struct ypbind_binding *
dup_ypbind_binding(a)
struct ypbind_binding *a;
{
	char *strdup();
	struct ypbind_binding *b;
	struct netconfig *nca, *ncb;
	struct netbuf *nxa, *nxb;
	int i;

	b=(struct ypbind_binding *)calloc(1,sizeof(*b));
	if (b==NULL) return(b);
	b->ypbind_hi_vers=a->ypbind_hi_vers;
	b->ypbind_lo_vers=a->ypbind_lo_vers;
	b->ypbind_servername=strdup(a->ypbind_servername);
	ncb=(b->ypbind_nconf=(struct netconfig *) calloc(1,sizeof(struct netconfig)));
	nxb=(b->ypbind_svcaddr= (struct netbuf *) calloc(1,sizeof(struct netbuf)));
	nca=a->ypbind_nconf;
	nxa=a->ypbind_svcaddr;
	ncb->nc_flag=nca->nc_flag;
	ncb->nc_protofmly=nca->nc_protofmly;
	ncb->nc_proto=nca->nc_proto;
	ncb->nc_semantics=nca->nc_semantics;
	ncb->nc_netid=strdup(nca->nc_netid);
	ncb->nc_device=strdup(nca->nc_device);
	ncb->nc_nlookups=nca->nc_nlookups;
	ncb->nc_lookups=(char **)calloc(nca->nc_nlookups,sizeof(char *));
	for (i = 0; i < nca->nc_nlookups; i++)
		ncb->nc_lookups[i] = strdup(nca->nc_lookups[i]);
	for (i=0; i < 8; i++)
		ncb->nc_unused[i]=nca->nc_unused[i];
	nxb->maxlen=nxa->maxlen;
	nxb->len=nxa->len;
	nxb->buf=malloc(nxa->maxlen);
	memcpy(nxb->buf, nxa->buf, nxb->len);
	return(b);
}

void
broadcast_proc_exit()
{
	int pid;
	int wait_status;
	register struct domain *pdom;
	pid = 0;

	pid = wait(&wait_status);
#ifdef DEBUG
	pfmt(stderr, MM_STD | MM_INFO, ":47:broadcast_proc_exit: got wait from %d status=%d\n",pid,wait_status);
#endif
	if (pid == 0) {
		enable_exit();
		return;
	} else if (pid == -1) {
		enable_exit();
		return;
	}

	for (pdom = known_domains; pdom != (struct domain *)NULL;
	    pdom = pdom->dom_pnext) {
#ifdef DEBUG
		pfmt(stderr, MM_STD | MM_INFO, ":48:broadcast_proc_exit: domain %s pid=%d\n", pdom->dom_name,
		    pdom->dom_broadcaster_pid);
#endif

		if (pdom->dom_broadcaster_pid == pid) {
			pdom->dom_broadcaster_pid = 0;
#ifdef DEBUG
			pfmt(stderr, MM_STD | MM_INFO, ":49:broadcast_proc_exit: got match %s\n", pdom->dom_name);
#endif
			if ((WTERMSIG(wait_status) == 0) &&
			    (WEXITSTATUS(wait_status) == 0)) {
				broadcast_setup(pdom);
			}

		}
	}
	enable_exit();
	return;
}

static void
enable_exit()
{
	sigset(SIGCHLD, (void (*)())broadcast_proc_exit);
}

static void
broadcast_setup(pdom)
struct domain *pdom;
{
	ypbind_setdom req;
	memset(&req,0,sizeof(req));
	if (pdom->broadcaster_pipe){
		if(xdr_ypbind_setdom( &(pdom->broadcaster_xdr), &req)){
			pdom->dom_report_success = -1;
#ifdef DEBUG
			pfmt(stderr, MM_STD | MM_INFO, ":50:broadcast_setup: got xdr ok \n");
#endif
			ypbindproc_setdom_3(&req, (CLIENT *)NULL, 
			    (SVCXPRT *)NULL);
#ifdef DEBUG
		} else {
			pfmt(stderr, MM_STD | MM_INFO, ":51:broadcast_setup: xdr failed\n");
#endif
		}
		xdr_destroy( &(pdom->broadcaster_xdr));
		fclose(pdom->broadcaster_pipe);
#ifdef DEBUG
	} else {
		pfmt(stderr, MM_STD | MM_INFO, ":52:boradcast_setup: no broadcaster pipe\n");
#endif
	}

	close(pdom->broadcaster_fd);
	pdom->broadcaster_pipe=0;
	pdom->broadcaster_fd= -1;
}

static int
ypbind_find(domain)
struct domain *domain;
{
	int bpid;
	int fildes[2];
	if ((domain) && (!domain->dom_boundp) &&
	    (!domain->dom_broadcaster_pid)) {
		/*
		 * The current domain is unbound, and there is no broadcaster 
		 * process active now.  Fork off a child who will yell out on 
		 * the net.  Because of the flavor of request we're making of 
		 * the server, we only expect positive ("I do serve this
		 * domain") responses.
		 */
		if (pipe(fildes)<0) return(-1);

		enable_exit();
		sighold(SIGCLD);
		bpid=fork();
		if (bpid!=0) { /*parent*/
			if (bpid>0){ /*parent started*/
				close (fildes[1]);
				domain->dom_broadcaster_pid=bpid;
				domain->dom_report_success++;
				domain->broadcaster_fd=fildes[0];
				domain->broadcaster_pipe=fdopen(fildes[0],"r");
				if (domain->broadcaster_pipe)
{
					xdrstdio_create(&(domain->broadcaster_xdr), (domain->broadcaster_pipe),XDR_DECODE);
}

#ifdef DEBUG
				pfmt(stderr, MM_STD | MM_INFO, ":53:ypbind_find: %s starting pid=%d try=%d\n",
				    domain->dom_name,bpid,
				    domain->dom_report_success);
#endif
				sigrelse(SIGCLD);
				return(0);
			}
			else { /*fork failed*/
				pfmt(stderr, MM_STD | MM_NOGET,
					"%s: %s\n", gettxt(":54", "fork"),
						strerror(errno));
				close(fildes[0]);
				close(fildes[1]);
				sigrelse(SIGCLD);
				return(-1);
			}
		} /*end parent*/
		/*child only code*/
		sigrelse(SIGCLD);
		close(fildes[0]);
		domain->broadcaster_fd=fildes[1];
		domain->broadcaster_pipe=fdopen(fildes[1],"w");
		if (domain->broadcaster_pipe)
{
			xdrstdio_create(&(domain->broadcaster_xdr), (domain->broadcaster_pipe),XDR_ENCODE);
}
		exit(pong_servers(domain->dom_name,domain));
	}
	return(0);
}

int
pipe_setdom(res,pdom)
ypbind_setdom *res;
struct domain *pdom;
{
	int retval;

	if(xdr_ypbind_setdom( &(pdom->broadcaster_xdr), res)) {
#ifdef DEBUG
		pfmt(stderr, MM_STD | MM_INFO, ":55:ypbind_setdom:sent xdr ok \n");
#endif
		retval = 0;
	} else {
#ifdef DEBUG
		pfmt(stderr, MM_STD | MM_INFO, ":56:ypbind_setdom:xdr failed\n") ;
#endif
		retval = -1;
	}
	xdr_destroy( &(pdom->broadcaster_xdr));
	fclose(pdom->broadcaster_pipe);
	close(pdom->broadcaster_fd);
	pdom->broadcaster_pipe=0;
	pdom->broadcaster_fd= -1;
	return(retval);
}
