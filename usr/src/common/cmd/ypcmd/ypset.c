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

#ident	"@(#)ypcmd:ypset.c	1.3.7.4"
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
 * This is a user command which issues a "Set domain binding" command to a 
 * YP binder (ypbind) process
 *
 *	ypset [-h <host>] [-d <domainname>] server_to_use
 *
 * where host and server_to_use may be either names or internet addresses.
 */
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>
#include "yp_b.h"
#include <rpcsvc/yp_prot.h>

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#define TIMEOUT 30			/* Total seconds for timeout */

static char *domain = NULL;
static char default_domain_name[YPMAXDOMAIN];
static char default_host_name[256];
static char *host = NULL;
static char *server_to_use;
static struct timeval timeout = {
	TIMEOUT,			/* Seconds */
	0				/* Microseconds */
	};

typedef struct err_msg {
	char *num;
	char *msg;
} ERR_MSG; 

static ERR_MSG err_usage_set_t = {
":1", "Usage:\n\
	ypset [-h <host>] [-d <domainname>] server_to_use\n\n"};
static ERR_MSG err_bad_args_t = {
	":2", "Sorry, the %s argument is bad.\n"};
static ERR_MSG err_cant_get_kname_t = {
	":3", "Sorry, can't get %s back from system call.\n"};
static ERR_MSG err_null_kname_t = {
	":4", "Sorry, the %s hasn't been set on this machine.\n"};
static ERR_MSG err_bad_hostname_t = {
	":5", "hostname"};
static ERR_MSG err_bad_domainname_t = {
	":6", "domainname"};
static ERR_MSG err_bad_server_t = {
	":7", "server_to_use"};
static ERR_MSG err_tp_failure_t = {
	":8", "Sorry, I can't set up a connection to host %s.\n"};
static ERR_MSG err_rpc_failure_t = {
	":9", "Sorry, I couldn't send my rpc message to ypbind on host %s.\n"};

#define err_usage_set \
	gettxt(err_usage_set_t.num, err_usage_set_t.msg)
#define err_bad_args \
	gettxt(err_bad_args_t.num, err_bad_args_t.msg)
#define err_cant_get_kname \
	gettxt(err_cant_get_kname_t.num, err_cant_get_kname_t.msg)
#define err_null_kname \
	gettxt(err_null_kname_t.num, err_null_kname_t.msg)
#define err_tp_failure \
	gettxt(err_tp_failure_t.num, err_tp_failure_t.msg)
#define err_rpc_failure \
	gettxt(err_rpc_failure_t.num, err_rpc_failure_t.msg)
#define err_bad_hostname \
	gettxt(err_bad_hostname_t.num, err_bad_hostname_t.msg)
#define err_bad_domainname \
	gettxt(err_bad_domainname_t.num, err_bad_domainname_t.msg)
#define err_bad_server \
	gettxt(err_bad_server_t.num, err_bad_server_t.msg)

static void get_command_line_args();
static void send_message();

extern char *gettxt();
extern char *malloc();
extern void exit();
extern int getdomainname();
extern int gethostname();
extern struct netconfig *getnetconfigent();
extern unsigned int strlen();

/*
 * This is the mainline for the ypset process.  It pulls whatever arguments
 * have been passed from the command line, and uses defaults for the rest.
 */

main (argc, argv)
	int argc;
	char **argv;
	
{
 
	(void)setlocale(LC_ALL,"");
	(void)setcat("uxypset");
	(void)setlabel("UX:ypset");

	get_command_line_args(argc, argv);

	if (!domain) {
		
		if (!getdomainname(default_domain_name, YPMAXDOMAIN) ) {
			domain = default_domain_name;
		} else {
			(void) pfmt(stderr, MM_NOGET, err_cant_get_kname, 
				err_bad_domainname);
			exit(1);
		}

		if ((int) strlen(domain) == 0) {
			(void) pfmt(stderr, MM_NOGET, err_null_kname, 
				err_bad_domainname);
			exit(1);
		}
	}

	if (!host) {
		
		if (! gethostname(default_host_name, 256)) {
			host = default_host_name;
		} else {
			(void) pfmt(stderr, MM_NOGET, err_cant_get_kname, 
				err_bad_hostname);
			exit(1);
		}
	}

	send_message();
	exit(0);
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
	
	while (--argc > 1) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'h': {

				if (argc > 1) {
					argv++;
					argc--;
					host = *argv;
					argv++;

					if ((int) strlen(host) > 256) {
						(void) pfmt(stderr, MM_NOGET, err_bad_args,
						    err_bad_hostname);
						exit(1);
					}
					
				} else {
					(void) pfmt(stderr, MM_NOGET, err_usage_set);
					exit(1);
				}
				
				break;
			}
				
			case 'd': {

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if ((int) strlen(domain) > YPMAXDOMAIN) {
						(void) pfmt(stderr, MM_NOGET, err_bad_args,
						    err_bad_domainname);
						exit(1);
					}
					
				} else {
					(void) pfmt(stderr, MM_NOGET, err_usage_set);
					exit(1);
				}
				
				break;
			}
				
			default: {
				(void) pfmt(stderr, MM_NOGET, err_usage_set);
				exit(1);
			}
			
			}
			
		} else {
			(void) pfmt(stderr, MM_NOGET, err_usage_set);
			exit(1);
		}
	}

	if (argc == 1) {
		
		if ( (*argv)[0] == '-') {
			(void) pfmt(stderr, MM_NOGET, err_usage_set);
			exit(1);
		}
		
		server_to_use = *argv;

		if ((int) strlen(server_to_use) > 256) {
			(void) pfmt(stderr, MM_NOGET, err_bad_args,
			    err_bad_server);
			exit(1);
		}

	} else {
		(void) pfmt(stderr, MM_NOGET, err_usage_set);
		exit(1);
	}
}

/*
 * This takes the name of the YP host of interest, and fires off 
 * the "set domain binding" message to the ypbind process.
 */
 
static void
send_message()
{
	CLIENT *server, *client;
	struct ypbind_setdom req;
	struct ypbind_binding ypbind_info;
	enum clnt_stat clnt_stat;
	struct netconfig *nconf;
	struct netbuf nbuf;

	/*
	 * Open up a path to the server
	 */

	if ((server = clnt_create(server_to_use, YPPROG, YPVERS, 
	    "netpath")) == NULL) {
		(void) pfmt(stderr, MM_NOGET, err_tp_failure, server_to_use);
		exit(1);
	}

	/* get nconf, netbuf structures */
	nconf = getnetconfigent(server->cl_netid);
	clnt_control(server, CLGET_SVC_ADDR, (char *)&nbuf);

	/*
	 * Open a path to host
	 */
        if ((client = clnt_create(host, YPBINDPROG, YPBINDVERS, 
	   "netpath")) == NULL) {
                clnt_pcreateerror(gettxt(":10", "clnt_create"));
                exit(1);
        }

	client->cl_auth = authunix_create_default();

	/*
	 * Load up the message structure and fire it off.
	 */
	ypbind_info.ypbind_nconf = nconf;
	ypbind_info.ypbind_svcaddr = (struct netbuf *)(&nbuf);
	ypbind_info.ypbind_servername = server_to_use;
	ypbind_info.ypbind_hi_vers = 0;
	ypbind_info.ypbind_lo_vers = 0;
	req.ypsetdom_bindinfo = &ypbind_info;
	req.ypsetdom_domain =  domain;

	clnt_stat = (enum clnt_stat) clnt_call(client,
	    YPBINDPROC_SETDOM, xdr_ypbind_setdom, (caddr_t)&req, xdr_void, 0,
	    timeout);
	auth_destroy((client)->cl_auth);
	if( clnt_stat != RPC_SUCCESS) {
		(void) pfmt(stderr, MM_NOGET, err_rpc_failure, host);
		exit(1);
	}
}
