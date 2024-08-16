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

#ident	"@(#)ypcmd:yppoll.c	1.4.8.3"
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
 * This is a user command which asks a particular ypserv which version of a
 * map it is using.  Usage is:
 * 
 * yppoll [-h <host>] [-d <domainname>] mapname
 * 
 * If the host is ommitted, the local host will be used.  If host is specified
 * as an internet address, no yp services need to be locally available.
 *  
 */
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <ctype.h>
#include <rpc/rpc.h>
#include <rpcsvc/ypclnt.h>
#include <rpcsvc/yp_prot.h>
#include "yp_b.h"

#ifdef NULL
#undef NULL
#endif
#define NULL 0

#define TIMEOUT 30			/* Total seconds for timeout */

static int status = 0;				/* exit status */
static char *domain = NULL;
static char default_domain_name[YPMAXDOMAIN];
static char *map = NULL;
static char *host = NULL;
static char default_host_name[256];

typedef struct err_msg {
	char *num;
	char *msg;
} ERR_MSG; 

static ERR_MSG  err_usage_t = {
":1", "Usage:\n\
	yppoll [-h <host>] [-d <domainname>] mapname\n\n"};
static ERR_MSG err_bad_args_t =
	{":2", "Bad %s argument.\n"};
static ERR_MSG err_cant_get_kname_t =
	{":3", "Can't get %s back from system call.\n"};
static ERR_MSG err_null_kname_t =
	{":4", "%s hasn't been set on this machine.\n"};
static ERR_MSG err_bad_hostname_t =
	{":5", "hostname"};
static ERR_MSG err_bad_mapname_t =
	{":6", "mapname"};
static ERR_MSG err_bad_domainname_t =
	{":7", "domainname"};

#define err_usage \
	gettxt(err_usage_t.num, err_usage_t.msg)
#define err_bad_args \
	gettxt(err_bad_args_t.num, err_bad_args_t.msg)
#define err_cant_get_kname \
	gettxt(err_cant_get_kname_t.num, err_cant_get_kname_t.msg)
#define err_null_kname \
	gettxt(err_null_kname_t.num, err_null_kname_t.msg)
#define err_bad_hostname \
	gettxt(err_bad_hostname_t.num, err_bad_hostname_t.msg)
#define err_bad_mapname \
	gettxt(err_bad_mapname_t.num, err_bad_mapname_t.msg)
#define err_bad_domainname \
	gettxt(err_bad_domainname_t.num, err_bad_domainname_t.msg)

static void get_command_line_args();
static void getdomain();
static void getlochost();
static void getmapparms();
static void newresults();
static void getypserv();

extern void exit();
extern int getdomainname();
extern int gethostname();
extern unsigned int strlen();
extern int strcmp();
extern char *gettxt();

/*
 * This is the mainline for the yppoll process.
 */

main (argc, argv)
	int argc;
	char **argv;
	
{

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxyppoll");
	(void)setlabel("UX:yppoll");

	get_command_line_args(argc, argv);

	if (!domain) {
		getdomain();
	}
	
	if (!host) {
		getypserv();
	}
	
	getmapparms();
	exit(status);
	/* NOTREACHED */
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
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'h': 

				if (argc > 1) {
					argv++;
					argc--;
					host = *argv;
					argv++;

					if ((int)strlen(host) > 256) {
						(void) pfmt(stderr, MM_NOGET,
						    err_bad_args, err_bad_hostname);
						exit(1);
					}
					
				} else {
					(void) pfmt(stderr, MM_NOGET, err_usage);
					exit(1);
				}
				
				break;
				
			case 'd': 

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if ((int)strlen(domain) > YPMAXDOMAIN) {
						(void) pfmt(stderr, MM_NOGET,
						    err_bad_args,
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
			if (!map) {
				map = *argv;

				if ((int)strlen(map) > YPMAXMAP) {
					(void) pfmt(stderr, MM_NOGET, err_bad_args,
					    err_bad_mapname);
					exit(1);
				}

			} else {
				(void) pfmt(stderr, MM_NOGET, err_usage);
				exit(1);
			}
		}
	}

	if (!map) {
		(void) pfmt(stderr, MM_NOGET, err_usage);
		exit(1);
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
		(void) pfmt(stderr, MM_NOGET, err_cant_get_kname, 
			err_bad_domainname);
		exit(1);
	}

	if ((int)strlen(domain) == 0) {
		(void) pfmt(stderr, MM_NOGET, err_null_kname, 
			err_bad_domainname);
		exit(1);
	}
}

/*
 * This gets the local hostname back from the kernel
 */
static void
getlochost()
{

	if (gethostname(default_host_name, 256)) {
		(void) pfmt(stderr, MM_NOGET, err_cant_get_kname, 
				err_bad_hostname);
		exit(1);
	}
}

static void
getmapparms()
{
        CLIENT * map_clnt;
        struct ypresp_order oresp;
        struct ypreq_nokey req;
        struct ypresp_master mresp;
	struct ypresp_master *mresults = (struct ypresp_master *) NULL;
        struct ypresp_order *oresults = (struct ypresp_order *) NULL;

	struct timeval timeout;

	if ((map_clnt = clnt_create(host, YPPROG, YPVERS, 
	    "netpath"))  == NULL) {
		(void) pfmt(stderr, MM_STD,
		    ":9:Can't create connection to %s.\n", host);
		clnt_pcreateerror(gettxt(":10", "Reason"));
		exit(1);
	}

	timeout.tv_sec=TIMEOUT;
        timeout.tv_usec=0;
	req.domain = domain;
	req.map = map;
        mresp.master = NULL;

	if (clnt_call(map_clnt, YPPROC_MASTER,
		(xdrproc_t)xdr_ypreq_nokey, (caddr_t)&req, 
		(xdrproc_t)xdr_ypresp_master, (caddr_t)&mresp,
		timeout) == RPC_SUCCESS &&
	    clnt_call(map_clnt, YPPROC_ORDER,
		(xdrproc_t)xdr_ypreq_nokey, (caddr_t)&req,
		(xdrproc_t)xdr_ypresp_order, (caddr_t)&oresp,
		timeout) == RPC_SUCCESS) {

		mresults = &mresp;
		oresults = &oresp;
		newresults(mresults, oresults);
	} else {
		(void) pfmt(stderr, MM_STD,
		":11:Can't make YPPROC_ORDER call to ypserv at %s.\n	",
				host);
		clnt_perror(map_clnt, gettxt(":12", "Reason"));
		clnt_destroy(map_clnt);
		exit(1);
	}
}

static void
newresults(m, o)
	struct ypresp_master *m;
	struct ypresp_order *o;
{
	char *s_domok = gettxt(":13", "Domain %s is supported.\n");
	char *s_ook = gettxt(":14", "Map %s has order number %d.\n");
	char *s_mok = gettxt(":15", "The master server is %s.\n");
	char *s_mbad = gettxt(":16", "Can't get master for map %s.\n	Reason:  %s\n");
	char *s_obad = gettxt(":17", "Can't get order number for map %s.\n	Reason:  %s\n");

	if (m->status == YP_TRUE && o->status == YP_TRUE) {
		(void) pfmt(stdout, MM_NOSTD | MM_NOGET, s_domok, domain);
		(void) pfmt(stdout, MM_NOSTD | MM_NOGET, s_ook, map, o->ordernum);
		(void) pfmt(stdout, MM_NOSTD | MM_NOGET, s_mok, m->master);
	} else if (o->status == YP_TRUE)  {
		(void) pfmt(stdout, MM_NOSTD | MM_NOGET, s_domok, domain);
		(void) pfmt(stdout, MM_NOSTD | MM_NOGET, s_ook, map, o->ordernum);
		(void) pfmt(stderr, MM_STD | MM_NOGET,  s_mbad, map,
		    yperr_string(ypprot_err(m->status)) );
		status = 1;
	} else if (m->status == YP_TRUE)  {
		(void) pfmt(stdout, MM_NOSTD | MM_NOGET, s_domok, domain);
		(void) pfmt(stderr, MM_STD | MM_NOGET, s_obad, map,
		    yperr_string(ypprot_err(o->status)) );
		(void) pfmt(stdout, MM_NOSTD | MM_NOGET, s_mok, m->master);
		status = 1;
	} else {
		(void) pfmt(stderr, MM_STD, ":18:Can't get any map parameter information.\n");
		(void) pfmt(stderr, MM_STD | MM_NOGET, s_obad, map,
		    yperr_string(ypprot_err(o->status)) );
		(void) pfmt(stderr, MM_STD | MM_NOGET, s_mbad, map,
		    yperr_string(ypprot_err(m->status)) );
		status = 1;
	}
}

static void
getypserv()
{
	struct ypbind_resp response;
	struct ypbind_domain ypdomain;
	extern char *malloc();

	getlochost();

	(void) memset((char *)&response, 0, sizeof(response));
	ypdomain.ypbind_domainname = domain;
	ypdomain.ypbind_vers = YPBINDVERS;
	(void) rpc_call(default_host_name, YPBINDPROG, YPBINDVERS, YPBINDPROC_DOMAIN,
	    xdr_ypbind_domain, (caddr_t)&ypdomain, xdr_ypbind_resp,
	    (caddr_t)&response, "netpath");
	if (response.ypbind_status != YPBIND_SUCC_VAL) {
		(void) pfmt(stderr, MM_STD, ":19:couldn't get yp server - status %u\n",
		    response.ypbind_status);
		exit(1);
	}
	host = malloc(strlen(response.ypbind_resp_u.ypbind_bindinfo->ypbind_servername) + 1);
	strcpy(host, response.ypbind_resp_u.ypbind_bindinfo->ypbind_servername);
}
