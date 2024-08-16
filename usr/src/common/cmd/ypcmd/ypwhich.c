/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:ypwhich.c	1.4.7.7"
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

/*
 * This is a user command which tells which yp server is being used by a
 * given machine, or which yp server is the master for a named map.
 * 
 * Usage is:
 * ypwhich [-d domain] [-m [mname] [-t] | [-V1 | -V2] host]
 * 
 * where:  the -d switch can be used to specify a domain other than the
 * default domain.  -m tells the master of that map.  mname is a mapname
 * If the -m option is used, ypwhich will act like a vanilla yp client,
 * and will not attempt to choose a particular yp server.  On the
 * other hand, if no -m switch is used, ypwhich will talk directly to the yp
 * bind process on the named host, or to the local ypbind process if no host
 * name is specified.
 *  
 */

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include "yp_b.h"
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include "ypv2_bind.h"

#define YPSLEEPTIME 5 /* between two tries of bind */

#define TIMEOUT 30			/* Total seconds for timeout */
#define INTER_TRY 10			/* Seconds between tries */
#define TRY_AGAIN 2

static int translate = TRUE;
static int aliases = FALSE;
static bool newvers = FALSE;
static bool oldvers = FALSE;
static bool ask_specific = FALSE;
static char *domain = NULL;
static char default_domain_name[YPMAXDOMAIN];
static char *host = NULL;
static char default_host_name[256];
static char domain_alias[YPMAXDOMAIN]; 	/* nickname for domain */
static char map_alias[YPMAXMAP];	/* nickname for map */

static bool get_master = FALSE;
static bool get_server = FALSE;
static char *map = NULL;
static struct timeval timeout = {
		TIMEOUT,			/* Seconds */
		0				/* Microseconds */
};

typedef struct err_msg {
	char *num;
	char *msg;
} ERR_MSG; 

static ERR_MSG err_usage_t = {
	":1", "Usage:\n\
	ypwhich [-d domain] [hostname]\n\
	ypwhich [-d domain] [-m [mname]]\n\
	ypwhich -x\n\
where\n\
	mname may be either a mapname or an alias for a map.\n\
	hostname if specified, is the machine whose NIS server is to be found.\n\
	-x shows map aliases.\n"};
static ERR_MSG err_bad_args_t = {
	":2", "%s argument is bad.\n"};
static ERR_MSG err_cant_get_kname_t = {
	":3", "can't get %s back from system call.\n"};
static ERR_MSG err_null_kname_t = {
	":4", "the %s hasn't been set on this machine.\n"};
static ERR_MSG err_bad_mapname_t = {
	":5", "mapname"};
static ERR_MSG err_bad_domainname_t = {
	":6", "domainname"};
static ERR_MSG err_bad_hostname_t = {
	":7", "hostname"};

#define err_usage \
	gettxt(err_usage_t.num, err_usage_t.msg)
#define err_bad_args \
	gettxt(err_bad_args_t.num, err_bad_args_t.msg)
#define err_cant_get_kname \
	gettxt(err_cant_get_kname_t.num, err_cant_get_kname_t.msg)
#define err_null_kname \
	gettxt(err_null_kname_t.num, err_null_kname_t.msg)
#define err_bad_mapname \
	gettxt(err_bad_mapname_t.num, err_bad_mapname_t.msg)
#define err_bad_domainname \
	gettxt(err_bad_domainname_t.num, err_bad_domainname_t.msg)
#define err_bad_hostname \
	gettxt(err_bad_hostname_t.num, err_bad_hostname_t.msg)

static void get_command_line_args();
static void getdomain();
static void getlochost();
static void get_server_name();
static int  call_binder();
static void get_map_master();
#ifdef DEBUG
static void dump_response();
#endif
static void dump_ypmaps();
static void dumpmaps();

extern size_t strlen();
extern int strcmp();
extern int strncmp();
extern int getdomainname();
extern int gethostname();
extern void exit();
extern void free();
extern char *gettxt();

extern CLIENT *clnt_create();
extern void clnt_pcreateerror();
extern void clnt_perror();

/*
 * This is the main line for the ypwhich process.
 */
main(argc, argv)
char **argv;
{

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxypwhich");
	(void)setlabel("UX:ypwhich");
 
	sysvconfig();

	get_command_line_args(argc, argv);

	if (aliases) {
		yp_listaliases();
		exit(0);
	}

	if (!domain) {
		getdomain();
	}

	if (translate) {
		if (yp_getalias(domain, domain_alias, YPMAXDOMAIN) != 0)
			strcpy(domain_alias, domain);
		if (yp_getalias(map, map_alias, YPMAXMAP) != 0)
			strcpy(map_alias, map);
	} else {
		strcpy(domain_alias, domain);
		strcpy(map_alias, map);
	}
	if (get_server) {
		if (!host)
			getlochost();
		get_server_name();
	} else {
		if (map)
			get_map_master();
		else
			dump_ypmaps();
	}

	return(0);
}

/*
 * This does the command line argument processing.
 */
static void
get_command_line_args(argc, argv)
int argc;
char **argv;

{
	argv++;

	if (argc == 1) {
		get_server = TRUE;
		return;
	}

	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 't':
				translate = FALSE;
				argv++;
				break;

			case 'x':
				aliases = TRUE;
				argv++;
				break;

			case 'V':

				if ((*argv)[2] == '1') {
					oldvers = TRUE;
					argv++;
				} else if ((*argv)[2] == '2') {
					newvers = TRUE;
					argv++;
				} else {
					(void) pfmt(stderr, MM_NOGET, err_usage);
					exit(1);
				}
				break;

			case 'm':
				get_master = TRUE;
				argv++;

				if (argc > 1) {

					if ( (*(argv))[0] == '-') {
						break;
					}

					argc--;
					map = *argv;
					argv++;

					if ((int)strlen(map) > YPMAXMAP) {
						(void) pfmt(stderr,
						   MM_NOGET, err_bad_args, err_bad_mapname);
						exit(1);
					}

				}

				break;

			case 'd':

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if ((int)strlen(domain) > YPMAXDOMAIN) {
						(void) pfmt(stderr,
						  MM_NOGET, err_bad_args,
						    err_bad_domainname);
						exit(1);
					}

				} else {
					(void) pfmt(stderr, MM_NOGET, err_usage);
					exit(1);
				}

				break;

			default:
				(void) pfmt(stderr, MM_NOGET, err_usage);
				exit(1);
			}

		} else {

			if (get_server) {
				(void) pfmt(stderr, MM_NOGET, err_usage);
				exit(1);
			}

			get_server = TRUE;
			host = *argv;
			argv++;

			if ((int)strlen(host) > 256) {
				(void) pfmt(stderr, MM_NOGET, err_bad_args, 
					err_bad_hostname);
				exit(1);
			}
		}
	}

	if (newvers && oldvers) {
		(void) pfmt(stderr, MM_NOGET, err_usage);
		exit(1);
	}

	if (newvers || oldvers) {
		ask_specific = TRUE;
	}

	if (get_master && get_server) {
		(void) pfmt(stderr, MM_NOGET, err_usage);
		exit(1);
	}

	if (!get_master && !get_server) {
		get_server = TRUE;
	}
}

/*
 * This gets the local default domainname, and makes sure that it's set
 * to something reasonable.  domain is set here.
 */
static void
getdomain()
{
	if (!getdomainname(default_domain_name, YPMAXDOMAIN) ) {
		domain = default_domain_name;
	} else {
		(void) pfmt(stderr, MM_NOGET, gettxt(":2", "can not get domain name back from system call.\n"));
		exit(1);
	}

	if ((int)strlen(domain) == 0) {
		(void) pfmt(stderr, MM_NOGET, gettxt(":3", "the domain name has not been set on this machine.\n"));
		exit(1);
	}
}

/*
 * This gets the local hostname back from the kernel
 */
static void
getlochost()
{


	if (! gethostname(default_host_name, 256)) {
		host = default_host_name;
	} else {
		(void) pfmt(stderr, MM_NOGET, gettxt(":6", "can not get host name back from system call.\n"));
		exit(1);
	}

}

/*
 * This tries to find the name of the server to which the binder in question
 * is bound.  If one of the -Vx flags was specified, it will try only for
 * that protocol version, otherwise, it will start with the current version,
 * then drop back to the previous version.
 */
static void
get_server_name()
{
	int vers, stat;
	char *notbound = gettxt(":8", "Domain %s not bound.\n");

	if (ask_specific) {
		vers = oldvers ? YPBINDOLDVERS : YPBINDVERS;
		stat = call_binder(vers);
		if (stat == TRY_AGAIN) {
			(void)sleep(YPSLEEPTIME);
			stat = call_binder(vers);
			if ((stat == FALSE) || (stat == TRY_AGAIN))
				(void) pfmt(stderr, MM_NOGET, 
					notbound, domain_alias);
		} else if (stat == FALSE)
				(void) pfmt(stderr, MM_NOGET, 
					notbound, domain_alias);
	} else {
		vers = YPBINDVERS;
		stat = call_binder(vers);
		if (stat == TRY_AGAIN) {
			/*
			 * Don't need to drop back version, 
			 * ypbind IS running 
			 */
			(void)sleep(YPSLEEPTIME);
            stat = call_binder(vers);
            if ((stat == FALSE) || (stat == TRY_AGAIN))
				(void) pfmt(stderr, MM_NOGET,notbound,domain_alias);
		} else if (stat == FALSE) { /* drop back a version */
			vers = YPBINDOLDVERS;
			stat = call_binder(vers);
			if (stat == TRY_AGAIN) {
				(void)sleep(YPSLEEPTIME);
				if ((stat == FALSE) || (stat == TRY_AGAIN))
					(void) pfmt(stderr, MM_NOGET,
								notbound, domain_alias);
			} else if (stat == FALSE)
				(void) pfmt(stderr, MM_NOGET,
						notbound, domain_alias);
		}
	}
}

/*
 * This sends a message to the ypbind process on the node with 
 * the host name
 */
static int
call_binder(vers)
int vers;
{
	CLIENT *client;
	struct ypbind_resp *response;
	/* The next definition is for the reentrant ypbindproc_domain_3x() */
	struct ypbind_resp response_buf;
	struct ypbind_domain ypbd;
	char errstring[256];


	if ((client = clnt_create(host, YPBINDPROG, vers, "netpath"
	    )) == NULL) {
		if (rpc_createerr.cf_stat == RPC_PROGNOTREGISTERED ||
			rpc_createerr.cf_stat == RPC_PROGUNAVAIL) {
			(void) pfmt(stderr, MM_ERROR,
				":9: %s is not running ypbind\n", host);
			exit(1);
		}
		if (rpc_createerr.cf_stat == RPC_PMAPFAILURE) {
			(void) pfmt(stderr, MM_ERROR,
				":10: %s is not running rpcbind\n", host);
			exit(1);
		}
		(void) clnt_pcreateerror(gettxt(":11", "  clnt_create error"));
		exit(1);
	}
	ypbd.ypbind_domainname=domain_alias;
	ypbd.ypbind_vers=vers;
	response= ypbindproc_domain_3x(&ypbd, client, &response_buf);

	if (response==NULL){
		(void) sprintf(errstring,
		    gettxt(":12", " can't call ypbind on %s"), host);
		(void) clnt_perror(client, errstring);
		exit(1);
	}

	clnt_destroy(client);

	if(response->ypbind_status != YPBIND_SUCC_VAL)  {
		/*
		 * XXX: we will make one more call to ypbind
		 * to get rid of the "first time" error of ypwhich.
		 */
		if (response->ypbind_resp_u.ypbind_error == YPBIND_ERR_NOSERV)
			return(TRY_AGAIN);
		return (FALSE);
	}
	if (response->ypbind_resp_u.ypbind_bindinfo)
		(void) pfmt(stdout, MM_NOSTD, ":13:%s\n",
			response->ypbind_resp_u.ypbind_bindinfo->ypbind_servername);
#ifdef DEBUG
	dump_response(response);
#endif
	return (TRUE);
}

#ifdef DEBUG
static void
dump_response(which)
ypbind_resp * which;
{
	struct netconfig *nc;
	struct netbuf *ua;
	ypbind_binding * b;

	int i;

	{
		b = which->ypbind_resp_u.ypbind_bindinfo;
		if (b == NULL)
			(void) pfmt(stderr, MM_STD, ":14:???NO Binding information\n");
		else {
			(void) pfmt(stderr, MM_STD,
				":15:server=%s lovers=%ld hivers=%ld\n", 
			    b->ypbind_servername, b->ypbind_lo_vers, b->ypbind_hi_vers);
			nc = b->ypbind_nconf;
			ua = b->ypbind_svcaddr;
			if (nc == NULL)
				(void) pfmt(stderr, MM_STD,
					":16: NO netconfig information\n");
			else {
				(void) pfmt(stderr, MM_STD,
				  ":17: id %s device %s flag %x protofmly %s proto %s\n",
				    nc->nc_netid, nc->nc_device, 
				    (int) nc->nc_flag, nc->nc_protofmly,
				    nc->nc_proto);
			}
			if (ua == NULL)
				(void) pfmt(stderr, MM_STD,
					":18: NO netbuf information available from binder\n");
			else {
				(void) pfmt(stderr, MM_STD,
					":19:maxlen=%d len=%d\naddr=", ua->maxlen,
						ua->len);
				for (i = 0; i < ua->len; i++) {
					if (i != (ua->len - 1))
						(void) pfmt(stderr, MM_STD,
							":20:%d.", ua->buf[i]);
					else 
						(void) pfmt(stderr, MM_STD,
							":21:%d\n", ua->buf[i]);
				}
			}
		}
	}

}
#endif

/*
 * This translates a server address to a name and prints it.  If the address
 * is the same as the local address as returned by get_myaddress, the name
 * is that retrieved from the kernel.  If it's any other address (including
 * another ip address for the local machine), we'll get a name by using the
 * standard library routine (which calls the yp).  
 */

/*
 * This asks any yp server for the map's master.  
 */
static void
get_map_master()
{
	int err;
	char *master;

	err = yp_master(domain_alias, map_alias, &master);

	if (err) {
		(void) pfmt(stderr, MM_STD,
		    ":22:  Can't find the master of %s.\n		   Reason: %s.\n",
		    	map_alias, yperr_string(err) );
	} else {
		(void) pfmt(stdout, MM_NOSTD, ":13:%s\n", master);
	}
}

/*
 * This enumerates the entries within map "ypmaps" in the domain at global 
 * "domain", and prints them out key and value per single line.  dump_ypmaps
 * just decides whether we are (probably) able to speak the new YP protocol,
 * and dispatches to the appropriate function.
 */
static void
dump_ypmaps()
{
	int err;
	struct dom_binding *binding;

	if (err = _yp_dobind(domain_alias, &binding)) {
		(void) pfmt(stderr, MM_STD,
		    ":23:dump_ypmaps: Can't bind for domain %s.\n		    Reason: %s\n",
		    	domain_alias, yperr_string(ypprot_err(err)));
		return;
	}

	if (binding->dom_binding->ypbind_hi_vers  >= YPVERS) {
		dumpmaps(binding);
	} 
}

static void
dumpmaps(binding)
struct dom_binding *binding;
{
	enum clnt_stat rpc_stat;
	int err;
	char *master;
	char *dom = domain_alias;
	struct ypmaplist *pmpl;
	struct ypresp_maplist maplist;

	maplist.list = (struct ypmaplist *) NULL;

	rpc_stat = clnt_call(binding->dom_client, YPPROC_MAPLIST,
	    (xdrproc_t)xdr_ypdomain_wrap_string, (caddr_t)&dom,
	    (xdrproc_t)xdr_ypresp_maplist, (caddr_t)&maplist,
	    timeout);

	if (rpc_stat != RPC_SUCCESS) {
		(void) clnt_perror(binding->dom_client,
		    gettxt(":24", "dumpmaps: can't get maplist"));
		exit(1);
	}

	if (maplist.status != YP_TRUE) {
		(void) pfmt(stderr, MM_STD,
		    ":25:  Can't get maplist.\n		   Reason:  %s.\n",
		    	yperr_string(ypprot_err(maplist.status)) );
		exit(1);
	}

	for (pmpl = maplist.list; pmpl; pmpl = pmpl->ypml_next) {
		(void) pfmt(stdout, MM_NOSTD, ":26:%s ", pmpl->ypml_name);

		err = yp_master(domain_alias, pmpl->ypml_name, &master);

		if (err) {
			(void) pfmt(stderr, MM_STD, ":27:????????\n");
			(void) pfmt(stderr, MM_STD,
			    	":28:  Can't find the master of %s.\n		   Reason: %s.\n",
			    		pmpl->ypml_name, yperr_string(err) );
		} else {
			(void) pfmt(stdout, MM_NOSTD, ":13:%s\n", master);
		}
	}
}
