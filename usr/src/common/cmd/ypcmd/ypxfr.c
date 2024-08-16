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

#ident	"@(#)ypcmd:ypxfr.c	1.4.7.6"
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
* This is a user command which gets a yp data base from some running
* server, and gets it to the local site by using the normal yp client
* enumeration functions.  The map is copied to a temp name, then the real
* map is removed and the temp map is moved to the real name.  ypxfr then
* sends a "YPPROC_CLEAR" message to the local server to insure that he will
* not hold a removed map open, so serving an obsolete version.  
* 
* ypxfr [-h <host>] [-d <domainname>] 
*		     [-s <domainname>] [-f] [-c] [-C tid proto host] map
* 
* If the host is ommitted, ypxfr will attempt to discover the master by 
* using normal yp services.  If it can't get the record, it will use
* the address of the callback, if specified. 
* 
* If the domain is not specified, the default domain of the local machine
* is used.
* 
* If the -f flag is used, the transfer will be done even if the master's
* copy is not newer than the local copy.
*
* The -c flag suppresses the YPPROC_CLEAR request to the local ypserv.  It
* may be used if ypserv isn't currently running to suppress the error message.
* 
* The -C flag is used to pass callback information to ypxfr when it is
* activated by ypserv.  The callback information is used to send a
* yppushresp_xfr message with transaction id "tid" to a yppush process
* speaking a transient protocol number "prot".  The yppush program is
* running on the host "name". If accompanied by the -O flag, use the
* YPXFR_PROC rather than the newer, transport independent YPNEWXFR_PROC.
*
* The -s option is used to specify a source domain which may be
* different from the destination domain, for transfer of maps
* that are identical in different domains (e.g. services.byname)
*  
*/

#include <ctype.h>
#include <pfmt.h>
#include <locale.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/file.h>
#include <memory.h>
#include "ypsym.h"
#include "yp_b.h"
#include "ypdefs.h"

/*
 * SNI: the following include files are needed for oldypxfr
 */
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/stat.h>

USE_YP_MASTER_NAME
USE_YP_SECURE
USE_YP_LAST_MODIFIED
USE_YPDBPATH
USE_DBM

# define PARANOID 1		/* make sure maps have the right # entries */

#define TIMEOUT 10

static struct timeval sh_timeout = {
TIMEOUT,
0
};

static struct timeval lg_timeout = {
TIMEOUT*4,
0
};

static struct timeval map_timeout = {
TIMEOUT*18,	/* timeout for map enumeration (could be long) */
0
};


static int	debug = FALSE;
static int	oldxfr = FALSE;

static char *domain = NULL;
static char domain_alias[MAXNAMLEN]; 	/* nickname for domain - 
						used in sysv filesystems */
static char map_alias[MAXNAMLEN];	/* nickname for map -
						used in sysv filesystems */
static char *source = NULL;
static char *map = NULL;
static char *master = NULL;	/* The name of the xfer peer as specified as a
			 *   -h option, or from querying the yp */
static struct dom_binding master_server;/* To talk to above */
static char *master_name = NULL;	/* Map's master as contained in the map */
static unsigned int *master_version = NULL; /* Order number as contained in the map */
static char *master_ascii_version;	/* ASCII order number as contained in the map */
static bool fake_master_version = FALSE; /* TRUE only if there's no order number in
			  	   *  the map, and the user specified -f */
static bool force = FALSE;		/* TRUE iff user specified -f flag */
static bool logging = FALSE;		/* TRUE iff no tty, but log file exists */
static bool send_clear = TRUE;		/* FALSE iff user specified -c flag */
static bool callback = FALSE;		/* TRUE iff -C flag set.  tid, prog, and name
			 	 * will be set to point to the
				 * command line args. */
static bool secure_map = FALSE;	/* TRUE if there is yp_secure in the map */

static char *tid;
static char *name;
static char *proto;

/*
 * SNI: the following variables are needed for oldypxfr
 */
static char *ipaddr;
static char *port;

static int entry_count;		/* counts entries in the map */
static char logfile[] = "/var/yp/ypxfr.log";
static char aliasfile[] = "/var/yp/aliases";

#if MAP_TO_SYSTEM
 static char map_to_system[] = "/var/yp/map2system";
#endif

typedef struct err_msg {
	char *num;
	char *msg;
} ERR_MSG; 

static ERR_MSG err_usage_t = {
":1", "Usage:\n\
ypxfr [-f] [-h <host>] [-d <domainname>]\n\
\t[-s <domainname>] [-c] [-C tid prot host] map\n\n\
where\n\
	-f forces transfer even if the master's copy is not newer.\n\
	-c inhibits sending a \"Clear map\" message to the local ypserv.\n\
	-C is used by ypserv to pass callback information.\n"};
static ERR_MSG err_bad_args_t = {
	":2", "%s argument is bad.\n"};
static ERR_MSG err_cant_get_kname_t = {
	":3", "Can't get %s back from system call.\n"};
static ERR_MSG err_null_kname_t = {
	":4", "%s hasn't been set on this machine.\n"};
static ERR_MSG err_bad_hostname_t = {
	":5", "hostname"};
static ERR_MSG err_bad_mapname_t = {
	":6", "mapname"};
static ERR_MSG err_bad_domainname_t = {
	":7", "domainname"};

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

static char yptempname_prefix[] = "t.";
static char ypbkupname_prefix[] = "b.";

static void get_command_line_args();
static bool bind_to_server();
static bool get_private_recs();
static bool get_order();
static bool get_secure_rec();
static bool get_master_name();
static bool find_map_master();
static bool move_map();
static unsigned get_local_version();
static void mkfilename();
static void mk_tmpname();
static bool rename_map();
static bool check_map_existence();
static bool get_map();
static bool add_private_entries();
static bool new_mapfiles();
static void del_mapfiles();
static void set_output();
static void logprintf();
static bool send_ypclear();
static void xfr_exit();
static void send_callback();
static int ypall_callback();
static int map_yperr_to_pusherr();
static int count_mismatch();

extern char *gettxt();
extern int access();
extern int atoi();
extern int chmod();
extern int close();
extern char *ctime();
extern int dbmclose();
extern int dbminit();
extern int errno;
extern void exit();
extern int getdomainname();
extern struct hostent *gethostbyname();
extern int gethostname();
extern int gettimeofday();
extern long getpid();
extern int isatty();
extern int store();
extern char *strcat();
extern char *strcpy();
extern unsigned int strlen();
extern void mkmap_alias();
extern int unlink();
extern void sysvconfig();
extern int yp_getalias();

#define YPOLDVERS           (YPVERS - 1)

/*
 * This is the mainline for the ypxfr process.
 */

main(argc, argv)
	int argc;
	char **argv;
	
{
	static char default_domain_name[YPMAXDOMAIN];
	static unsigned big = 0xffffffff;
	FILE *fp;
	int status;

	set_output();

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxypxfr");
	(void)setlabel("UX:ypxfr");


	get_command_line_args(argc, argv);

	(void) getdomainname(default_domain_name, YPMAXDOMAIN);
	if (!domain) {
		if (default_domain_name[0]) {
			domain = default_domain_name;
		} else {
			logprintf( err_cant_get_kname,
			    err_bad_domainname);
			xfr_exit(YPPUSH_RSRC);
		}

		if (strlen(domain) == 0) {
			logprintf( err_null_kname,
			    err_bad_domainname);
			xfr_exit(YPPUSH_RSRC);
		}
	}

	sysvconfig();
	if (yp_getalias(domain, domain_alias, MAXALIASLEN) == 0) {
		domain = domain_alias;
	}
	if (yp_getalias(map, map_alias, MAXALIASLEN) == 0) {
		map = map_alias;
	}

	if (!source)
		source = domain;
	
	if (!master) {
		if (!find_map_master())
			xfr_exit(YPPUSH_MADDR);
	}
	
	if (!bind_to_server(master, &master_server, &status) ) {
		xfr_exit(status);
	}

	if (!get_private_recs(&status) ) {
		xfr_exit(status);
	}
	
	if (!master_version) {

		if (force) {
			master_version = &big;
			fake_master_version = TRUE;
		} else {
			logprintf(gettxt(":10", "Can't get order number for map %s from server at %s: use the -f flag.\n"),
				  map, master);
			xfr_exit(YPPUSH_FORCE);
		}
	}
	
	if (!move_map(&status) ) {
		xfr_exit(status);
	}

	if (send_clear && !send_ypclear(&status) ) {
		xfr_exit(status);
	}

#if MAP_TO_SYSTEM
	/*
	 * SNI: if ypxfr for default domain,
	 *      update the ascii files from the new yp maps
	 */

	if (strcmp(default_domain_name, domain) == 0) {
		char *result = NULL;

		if (updateSystemFiles(map)) {
			result = gettxt(":11", "succeeded");
		} else
			result = gettxt(":12", "failed");

		if (result) {
			logprintf(gettxt(":13", "Update of system files %s (map = %s)\n"),
				result, map);
		}
	}
#endif /* MAP_TO_SYSTEM */

	if (logging) {
		logprintf(gettxt(":14", "Transferred map %s from %s (%d entries).\n"),
			map, master, entry_count);
	}

	xfr_exit(YPPUSH_SUCC);
	/* NOTREACHED */
}

/*
 * This decides whether we're being run interactively or not, and, if not,
 * whether we're supposed to be logging or not.  If we are logging, it sets
 * up stderr to point to the log file, and sets the "logging"
 * variable.  If there's no logging, the output goes in the bit bucket.
 * Logging output differs from interactive output in the presence of a
 * timestamp, present only in the log file.  stderr is reset, too, because it
 * it's used by various library functions, including clnt_perror.
 */
static void
set_output()
{
	if (!isatty(2)) {
		if (access(logfile, 2)) {
			(void) freopen("/dev/null", "w", stderr);
		} else {
			(void) freopen(logfile, "a", stderr);
			logging = TRUE;
		}
	}
}

/*
 * This constructs a logging record.
 */
/*VARARGS1*/
static void
logprintf(arg1,arg2,arg3,arg4,arg5,arg6,arg7)
char *arg1;
{
	struct timeval t;

	(void) fseek(stderr,0,2);
	if (logging) {
		(void) gettimeofday(&t, (struct timezone *) NULL);
		(void) pfmt(stderr, MM_NOSTD | MM_NOGET, "%19.19s: ", ctime(&t.tv_sec));
	}
	(void) pfmt(stderr, MM_NOSTD | MM_NOGET, arg1,arg2,arg3,arg4,arg5,arg6,arg7);
	(void) fflush(stderr);
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

	if (argc < 2) {
		logprintf(err_usage);
		xfr_exit(YPPUSH_BADARGS);
	}
	
	while (--argc) {

		if ( (*argv)[0] == '-') {

			switch ((*argv)[1]) {

			case 'f': {
				force = TRUE;
				argv++;
				break;
			}

			case 'D': {
				debug = TRUE;
				argv++;
				break;
			}

			case 'c': {
				send_clear = FALSE;
				argv++;
				break;
			}

			case 'h': {

				if (argc > 1) {
 					argv++;
					argc--;
					master = *argv;
					argv++;

					if (strlen(master) > 256) {
						logprintf(
						    err_bad_args,
						    err_bad_hostname);
						xfr_exit(YPPUSH_BADARGS);
					}
					
				} else {
					logprintf( err_usage);
					xfr_exit(YPPUSH_BADARGS);
				}
				
				break;
			}
				
			case 'd':
				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if (strlen(domain) > YPMAXDOMAIN) {
						logprintf(
						    err_bad_args,
						    err_bad_domainname);
						xfr_exit(YPPUSH_BADARGS);
					}
					
				} else {
					logprintf( err_usage);
					xfr_exit(YPPUSH_BADARGS);
				}				
				break;

			case 's':
				if (argc > 1) {
					argv++;
					argc--;
					source = *argv;
					argv++;

					if (strlen(source) > YPMAXDOMAIN) {
						logprintf(
						    err_bad_args,
						    err_bad_domainname);
						xfr_exit(YPPUSH_BADARGS);
					}
					
				} else {
					logprintf( err_usage);
					xfr_exit(YPPUSH_BADARGS);
				}				
				break;

			case 'O': {
				oldxfr = TRUE;
				argv++;
				break;
			}

			case 'C': {

				if (oldxfr) {
					if (argc > 5) {
						callback = TRUE;
						argv++;
						tid = *argv++;
						proto = *argv++;
						ipaddr = *argv++;
						port = *argv++;
						argc -= 4;
					} else {
						logprintf( err_usage);
						xfr_exit(YPPUSH_BADARGS);
					}
				} else {
					if (argc > 4) {
						callback = TRUE;
						argv++;
						tid = *argv++;
						proto = *argv++;
						name = *argv++;
						argc -= 3;
					} else {
						logprintf( err_usage);
						xfr_exit(YPPUSH_BADARGS);
					}
				}
				
				break;
			}


			default: {
				logprintf( err_usage);
				xfr_exit(YPPUSH_BADARGS);
			}
			
			}
			
		} else {

			if (!map) {
				map = *argv;
				argv++;
			
				if (strlen(map) > YPMAXMAP) {
					logprintf( err_bad_args,
				   	err_bad_mapname);
					xfr_exit(YPPUSH_BADARGS);
				}
				
			} else {
				logprintf( err_usage);
				xfr_exit(YPPUSH_BADARGS);
			}
		}
	}

	if (!map) {
		logprintf( err_usage);
		xfr_exit(YPPUSH_BADARGS);
	}
}

/*
 * This sets up a channel to a server which is assumed to speak an input
 * version of YPPROG.  The channel is tested by pinging the server.  In all
 * error cases except "Program Version Number Mismatch", the error is
 * reported, and in all error cases, the client handle is destroyed 
 */
static bool
bind_to_server(host, pdomb, status)
	char *host;
	struct dom_binding *pdomb;
	int *status;
{
	enum clnt_stat rpc_stat;
	
	if ((pdomb->dom_client = clnt_create(host, YPPROG, YPVERS, 
	    "udp")) == (CLIENT *) NULL) {
		logprintf(gettxt(":16", "Unable to contact ypserv on %s"), 
				host);
		clnt_pcreateerror("");
		(void) fflush(stderr);
		*status = YPPUSH_RPC;
		return(FALSE);
	}

	rpc_stat = clnt_call(pdomb->dom_client, YPPROC_NULL,
	    xdr_void, 0, xdr_void, 0, sh_timeout);

	if (rpc_stat == RPC_SUCCESS) {
		return (TRUE);
	} else {
		if (rpc_stat != RPC_PROGVERSMISMATCH) {
			logprintf(gettxt(":18", "Call to ypserv on %s failed"), 
					host);
			(void) clnt_perror(pdomb->dom_client, "bind_to_server");
		}
		clnt_destroy(pdomb->dom_client);
		*status = YPPUSH_RPC;
		return (FALSE);
	}
}

/*
 * This gets values for the YP_LAST_MODIFIED and YP_MASTER_NAME keys from the
 * master server's version of the map.  Values are held in static variables
 * here.  In the success cases, global pointer variables are set to point at
 * the local statics.  
 */
static bool
get_private_recs(pushstat)
	int *pushstat;
{
	static char anumber[20];
	static unsigned number;
	static char m_name[YPMAXPEER + 1];
	int status;

	status = 0;
	
	if (get_order(anumber, &number, &status) ) {
		master_version = &number;
		master_ascii_version = anumber;
		if (debug) (void) pfmt(stderr, MM_NOSTD, ":19:Master Version is %s\n",master_ascii_version);
	} else {

		if (status != 0) {
			*pushstat = status;
			if (debug) (void) pfmt(stderr, MM_NOSTD, ":20:Couldn't get map's master version number, status was %d\n",status);
			return (FALSE);
		}
	}

	if (get_master_name(m_name, &status) ) {
		master_name = m_name;
		if (debug) (void) pfmt(stderr, MM_NOSTD, ":21:Maps master is '%s'\n",master_name);
	} else {

		if (status != 0) {
			*pushstat = status;
			if (debug) (void) pfmt(stderr, MM_NOSTD, ":22:Couldn't get map's master name, status was %d\n",status);
			return (FALSE);
		}
		master_name = master;
	}

	if (debug)
		(void) pfmt(stderr, MM_NOSTD, ":23:Getting private records from master.\n");
	if (get_secure_rec(&status)) {
		if (debug)
			if (secure_map)
				(void) pfmt(stderr, MM_NOSTD,
		    			":24:Masters map is secure map.\n");
			else
				(void) pfmt(stderr, MM_NOSTD,
		    			":25:Masters map is not secure map.\n");
	} else {
		if (status != 0) {
			*pushstat = status;
			if (debug) 
				(void) pfmt(stderr, MM_NOSTD, ":26:Couldn't get state of secure flag in map.\n");
			return(FALSE);
		}
	}

	return (TRUE);
}

/*
 * This gets the map's order number from the master server
 */
static bool
get_order(an, n, pushstat)
	char *an;
	unsigned *n;
	int *pushstat;
{
	struct ypreq_nokey req;
	struct ypresp_order resp;
	int retval;

	req.domain = source;
	req.map = map;
	
	/*
	 * Get the map''s order number, null-terminate it and store it,
	 * and convert it to binary and store it again.
	 */
	retval = FALSE;
	
	if((enum clnt_stat) clnt_call(master_server.dom_client, YPPROC_ORDER,
	    (xdrproc_t)xdr_ypreq_nokey, (caddr_t)&req,
	    (xdrproc_t)xdr_ypresp_order, (caddr_t)&resp,
	    lg_timeout) == RPC_SUCCESS) {

		if (resp.status == YP_TRUE) {
			(void) sprintf(an, "%d", (int) resp.ordernum);
			*n = resp.ordernum;
			retval = TRUE;
		} else if (resp.status != YP_BADDB) {
			*pushstat = map_yperr_to_pusherr(resp.status);
			logprintf(gettxt(":28", "Can't get order number of %s from ypserv at %s.\n		   Reason: %s.\n"),
				    map, master, 
					yperr_string(ypprot_err(resp.status)));
		}

		CLNT_FREERES(master_server.dom_client,
		    (xdrproc_t)xdr_ypresp_order, (caddr_t)&resp);
	} else {
		*pushstat = YPPUSH_RPC;
		logprintf(gettxt(":18", "Call to ypserv on %s failed"), 
			master);
		(void) clnt_perror(master_server.dom_client, "get_order");
	}

	return (retval);
}

/*
 * Pick up the state of the YP_SECURE record from the master. 
 * Only works on 4.0 V2 and V3 masters that will match a 
 * YP_ private key when asked to explicitly.
 */
static bool
get_secure_rec(pushstat)
	int *pushstat;
{
	struct ypreq_key req;
	struct ypresp_val resp;
	int retval;

	req.domain = source;
	req.map = map;
	req.keydat.dptr   = yp_secure;
	req.keydat.dsize  = yp_secure_sz;
	
	resp.valdat.dptr = NULL;
	resp.valdat.dsize = 0;
	
	/*
	 * Get the value of the IS_SECURE key in the map.
	 */
	retval = FALSE;
	
	if (debug)
		(void) pfmt(stderr, MM_NOSTD, ":30:Checking masters secure key.\n");
	if((enum clnt_stat) clnt_call(master_server.dom_client, YPPROC_MATCH,
	    (xdrproc_t)xdr_ypreq_key, (caddr_t)&req,
	    (xdrproc_t)xdr_ypresp_val, (caddr_t)&resp,
            lg_timeout) == RPC_SUCCESS) {
		if (resp.status == YP_TRUE) {
			if (debug)
				(void) pfmt(stderr, MM_NOSTD, ":31:SECURE\n");
			secure_map = TRUE;
			retval = TRUE;
		} else if ((resp.status != YP_NOKEY) &&
		    (resp.status != YPVERS && resp.status != YPOLDVERS)) {
			*pushstat = map_yperr_to_pusherr(resp.status);
			
			logprintf(gettxt(":32", "Can't get secure flag from ypserv at %s.\n		  Reason: %s.\n"),
				    master, yperr_string(
				    ypprot_err(resp.status)));
		}

		CLNT_FREERES(master_server.dom_client,
		    (xdrproc_t)xdr_ypresp_val, (caddr_t)&resp);
	} else {
		*pushstat = YPPUSH_RPC;
		logprintf(gettxt(":18", "Call to ypserv on %s failed"), master);
		(void) clnt_perror(master_server.dom_client, "get_secure_rec");
	}

	return (retval);
}

/*
 * This gets the map's master name from the master server
 */
static bool
get_master_name(m_name, pushstat)
	char *m_name;
	int *pushstat;
{
	struct ypreq_nokey req;
	struct ypresp_master resp;
	int retval;

	req.domain = source;
	req.map = map;
	resp.master = NULL;
	retval = FALSE;
	
	if((enum clnt_stat) clnt_call(master_server.dom_client, YPPROC_MASTER,
	    (xdrproc_t)xdr_ypreq_nokey, (caddr_t)&req,
	    (xdrproc_t)xdr_ypresp_master, (caddr_t)&resp,
	    lg_timeout) == RPC_SUCCESS) {

		if (resp.status == YP_TRUE) {
			(void) strcpy(m_name, resp.master);
			retval = TRUE;
		} else if (resp.status != YP_BADDB) {
			*pushstat = ypprot_err(resp.status);

			logprintf(gettxt(":34", "Can't get master name from ypserv at %s.\n		   Reason: %s.\n"),
				master, yperr_string(
				ypprot_err(resp.status)) );
		}
	
		CLNT_FREERES(master_server.dom_client,
		    (xdrproc_t)xdr_ypresp_master, (caddr_t)&resp);
	} else {
		*pushstat = YPPUSH_RPC;
		logprintf(gettxt(":18", "Call to ypserv on %s failed"), master);
		(void) clnt_perror(master_server.dom_client, "get_master_name");
	}

	return (retval);
}

/*
 * This tries to get the master name for the named map, from any
 * server's version, using the vanilla yp client interface.  If we get a
 * name back, the global "master" gets pointed to it.
 */
static bool
find_map_master()
{
	int err;
		
	if (err = yp_master(source, map, &master)) {
		logprintf(gettxt(":36", "Can't get master of %s.\n		   Reason: %s.\n"), map,
			yperr_string(err));
		return(FALSE);
	}
	
	yp_unbind(source);
	return(TRUE);
}

/*
 * This does the work of transferring the map.
 */
static bool
move_map(pushstat)
	int *pushstat;
{
	unsigned local_version;
	char map_name[MAXNAMLEN + 1];
	char tmp_name[MAXNAMLEN + 1];
	char bkup_name[MAXNAMLEN + 1];
	char an[11];
	unsigned n;
	datum key;
	datum val;

	mkfilename(map_name);
	
	if (!force) {
		local_version = get_local_version(map_name);
		if (debug) (void) pfmt(stderr, MM_NOSTD, ":37:Local version of map '%s' is %d\n",map_name,local_version);

		if (local_version >= *master_version) {
			logprintf(gettxt(":38","Map %s at %s is not more recent than local.\n"), map, master);
			*pushstat = YPPUSH_AGE;
			return (FALSE);
		}
	}
	 
	mk_tmpname(yptempname_prefix, tmp_name);
	
	if (!new_mapfiles(tmp_name) ) {
		logprintf(
		    gettxt(":39", "Can't create temp map %s.\n"), tmp_name);
		*pushstat = YPPUSH_FILE;
		return (FALSE);
	}

	if (dbminit(tmp_name) < 0) {
		logprintf(
		    gettxt(":40", "Can't dbm init temp map %s.\n"), tmp_name);
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_DBM;
		return (FALSE);
	}

	if (!get_map(pushstat) ) {
		del_mapfiles(tmp_name);
		return (FALSE);
	}

	if (!add_private_entries(tmp_name) ) {
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_DBM;
		return (FALSE);
	}

	/*
	 * Decide whether the map just transferred is a secure map.
	 * If we already know the local version was secure, we do not
	 * need to check this version.
	 */
	if (!secure_map) {
		key.dptr = yp_secure;
		key.dsize = yp_secure_sz;
		val = fetch(key);
		if (val.dptr != NULL) {
			secure_map = TRUE;
		}
	}

	if (dbmclose(tmp_name) < 0) {
		logprintf(
		    gettxt(":41", "Can't do dbm close operation on temp map %s.\n"),
		    tmp_name);
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_DBM;
		return (FALSE);
	}

	if (!get_order(an, &n, pushstat)) {
		return(FALSE);
	}
	if (n != *master_version) {
			logprintf(gettxt(":42", "Version skew at %s while transferring map %s.\n"),
			    master, map);
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_SKEW;
		return (FALSE);
	}

# ifdef PARANOID
	if (!count_mismatch(tmp_name,entry_count)) {
		del_mapfiles(tmp_name);
		*pushstat = YPPUSH_DBM;
		return(FALSE);
	}
# endif PARANOID

	if (!check_map_existence(map_name) ) {

		if (!rename_map(tmp_name, map_name) ) {
			del_mapfiles(tmp_name);
			logprintf(
			  gettxt(":43", "Rename error:  couldn't mv %s to %s.\n"),
			    tmp_name, map_name);
			*pushstat = YPPUSH_FILE;
			return (FALSE);
		}
		
	} else {
		mk_tmpname(ypbkupname_prefix, bkup_name);
	
		if (!rename_map(map_name, bkup_name) ) {
			(void) rename_map(bkup_name, map_name);
			logprintf(
			  gettxt(":44", "Rename error:  check that old %s is still intact.\n"),
			    map_name);
			del_mapfiles(tmp_name);
			*pushstat = YPPUSH_FILE;
			return (FALSE);
		}

		if (rename_map(tmp_name, map_name) ) {
			del_mapfiles(bkup_name);
		} else {
			del_mapfiles(tmp_name);
			(void) rename_map(bkup_name, map_name);
				logprintf(gettxt(":44",
			  "Rename error:  check that old %s is still intact.\n"),
			    map_name);
			*pushstat = YPPUSH_FILE;
			return (FALSE);
		}
	}

	return (TRUE);
}

/*
 * This tries to get the order number out of the local version of the map.
 * If the attempt fails for any version, the function will return "0"
 */
static unsigned
get_local_version(l_name)
	char *l_name;
{
	datum key;
	datum val;
	char number[11];
	
	if (!check_map_existence(l_name) ) {
		return (0);
	}
	if (debug) (void) pfmt(stderr, MM_NOSTD, ":45:Map does exist, checking version now.\n");

	if (dbminit(l_name) < 0) {
		return (0);
	}
		
	key.dptr = yp_last_modified;
	key.dsize = yp_last_modified_sz;
	val = fetch(key);
	if (!val.dptr) {	/* Check to see if dptr is NULL */
		return (0);
	}
	if (val.dsize == 0 || val.dsize > 10) {
		return (0);
	}
	/* Now save this value while we have it available */
	(void) memcpy(number, val.dptr, (unsigned int)val.dsize);
	number[val.dsize] = '\0';

	/* 
	 * Now check to see if it is 'secure'. If we haven't already 
	 * determined that it is secure in get_private_recs() then we check
	 * the local map here.
	 */
	if (!secure_map) { 
		key.dptr = yp_secure;
		key.dsize = yp_secure_sz;
		val = fetch(key);
		secure_map = (val.dptr != NULL);
	}

	/* finish up */
	(void) dbmclose(l_name);

	return ((unsigned) atoi(number) );
}

/*
 * This constructs a file name for a map, minus its dbm_dir or 
 *	 dbm_pag extensions
 */
static void
mkfilename(ppath)
	char *ppath;
{

	if ( (strlen(domain_alias) + strlen(map_alias) + strlen(ypdbpath) + 3) 
	    > (MAXNAMLEN + 1) ) {
		logprintf( gettxt(":46", "Map name string too long.\n"));
	}

	(void) strcpy(ppath, ypdbpath);
	(void) strcat(ppath, "/");
	(void) strcat(ppath, domain_alias);
	(void) strcat(ppath, "/");
	(void) strcat(ppath, map_alias);
}

/*
 * This returns a temporary name for a map transfer minus its dbm_dir or
 * dbm_pag extensions.
 */
static int counter;

static void
mk_tmpname(prefix, xfr_name)
	char *prefix;
	char *xfr_name;
{
	char xfr_anumber[2];

	if (!xfr_name) {
		return;
	}
	if ( (strlen(domain_alias) + strlen(map_alias) + strlen(ypdbpath) + 3) 
	    > (MAXNAMLEN + 1) ) {
		logprintf( gettxt(":46", "Map name string too long.\n"));
	}

	if (counter == 99)
		counter = 0;
	(void) sprintf(xfr_anumber, "%d", counter++);
	
	(void) strcpy(xfr_name, ypdbpath);
	(void) strcat(xfr_name, "/");
	(void) strcat(xfr_name, domain_alias);
	(void) strcat(xfr_name, "/");
	(void) strcat(xfr_name, prefix);
	(void) strcat(xfr_name, map_alias);
	(void) strcat(xfr_name, ".");
	(void) strcat(xfr_name, xfr_anumber);
}

/*
 * This deletes the .pag and .dir files which implement a map.
 *
 * Note:  No error checking is done here for a garbage input file name or for
 * failed unlink operations.
 */
static void
del_mapfiles(basename)
	char *basename;
{
	char dbfilename[MAXNAMLEN + 1];

	if (!basename) {
		return;
	}
	
	(void) strcpy(dbfilename, basename);
	(void) strcat(dbfilename, dbm_pag);
	(void) unlink(dbfilename);
	(void) strcpy(dbfilename, basename);
	(void) strcat(dbfilename, dbm_dir);
	(void) unlink(dbfilename);
}

/*
 * This checks to see if the source map files exist, then renames them to the
 * target names.  This is a boolean function.  The file names from.pag and
 * from.dir will be changed to to.pag and to.dir in the success case.
 *
 * Note:  If the second of the two renames fails, yprename_map will try to
 * un-rename the first pair, and leave the world in the state it was on entry.
 * This might fail, too, though...
 */
static bool
rename_map(from, to)
	char *from;
	char *to;
{
	char fromfile[MAXNAMLEN + 1];
	char tofile[MAXNAMLEN + 1];
	char savefile[MAXNAMLEN + 1];

	if (!from || !to) {
		return (FALSE);
	}
	
	if (!check_map_existence(from) ) {
		return (FALSE);
	}
	
	(void) strcpy(fromfile, from);
	(void) strcat(fromfile, dbm_pag);
	(void) strcpy(tofile, to);
	(void) strcat(tofile, dbm_pag);
	
	if (rename(fromfile, tofile) ) {
		logprintf( gettxt(":47", "Can't rename %s to %s.\n"), fromfile,
		    tofile);
		return (FALSE);
	}
	
	(void) strcpy(savefile, tofile);
	(void) strcpy(fromfile, from);
	(void) strcat(fromfile, dbm_dir);
	(void) strcpy(tofile, to);
	(void) strcat(tofile, dbm_dir);
	
	if (rename(fromfile, tofile) ) {
		logprintf(gettxt(":47", "Can't rename %s to %s.\n"), fromfile,
		    tofile);
		(void) strcpy(fromfile, from);
		(void) strcat(fromfile, dbm_pag);
		(void) strcpy(tofile, to);
		(void) strcat(tofile, dbm_pag);
		
		if (rename(tofile, fromfile) ) {
			logprintf(
			    gettxt(":48", "Can't recover from rename failure.\n"));
			return (FALSE);
		}
		
		return (FALSE);
	}
	
	if (!secure_map) {
		(void) chmod(savefile, 0644);
		(void) chmod(tofile, 0644);
	}

	return (TRUE);
}

/*
 * This performs an existence check on the dbm data base files <pname>.pag and
 * <pname>.dir.
 */
static bool
check_map_existence(pname)
	char *pname;
{
	char dbfile[MAXNAMLEN + 1];
	struct stat filestat;
	int len;

	if (debug) (void) pfmt(stderr, MM_NOSTD, ":49:Checking the existence of '%s'\n",pname);
	if (!pname || ((len = strlen(pname)) == 0) ||
	    (len + 5) > (MAXNAMLEN + 1) ) {
		return (FALSE);
	}
		
	errno = 0;
	(void) strcpy(dbfile, pname);
	(void) strcat(dbfile, dbm_dir);

	if (debug) (void) pfmt(stderr, MM_NOSTD, ":50:First file is '%s'\n",dbfile);
	if (stat(dbfile, &filestat) != -1) {
		(void) strcpy(dbfile, pname);
		(void) strcat(dbfile, dbm_pag);

		if (stat(dbfile, &filestat) != -1) {
			return (TRUE);
		} else {

			if (errno != ENOENT) {
				logprintf(
				    gettxt(":51", "Stat error on map file %s.\n"),
				    dbfile);
			}

			return (FALSE);
		}

	} else {

		if (errno != ENOENT) {
			logprintf(gettxt(":51",
			    "Stat error on map file %s.\n"),
			    	dbfile);
		}

		return (FALSE);
	}
}

/*
 * This creates <pname>.dir and <pname>.pag
 */
static bool
new_mapfiles(pname)
	char *pname;
{
	char dbfile[MAXNAMLEN + 1];
	int f;
	int len;

	if (!pname || ((len = strlen(pname)) == 0) ||
	    (len + 5) > (MAXNAMLEN + 1) ) {
		return (FALSE);
	}
		
	errno = 0;
	(void) strcpy(dbfile, pname);
	(void) strcat(dbfile, dbm_dir);

	if ((f = open(dbfile, (O_WRONLY | O_CREAT | O_TRUNC), 0600)) >= 0) {
		(void) close(f);
		(void) strcpy(dbfile, pname);
		(void) strcat(dbfile, dbm_pag);

		if ((f = open(dbfile, (O_WRONLY | O_CREAT | O_TRUNC),
		    0600)) >= 0) {
			(void) close(f);
			return (TRUE);
		} else {
			return (FALSE);
		}

	} else {
		return (FALSE);
	}
}

/*
 * This counts the entries in the dbm file after the transfer to
 * make sure that the dbm file was built correctly.  
 * Returns TRUE if everything is OK, FALSE if they mismatch.
 */
static int
count_mismatch(pname,oldcount)
	char *pname;
	int oldcount;
{
	datum key;

	entry_count = 0;
	dbminit(pname);
	for (key = firstkey(); key.dptr != NULL; key = nextkey(key))
		entry_count++;
	dbmclose(pname);

	if (oldcount != entry_count) {
		logprintf( 
		    gettxt(":52", "*** Count mismatch in dbm file %s: old=%d, new=%d ***\n"),
		    map, oldcount, entry_count);
		return(FALSE);
	}

	return(TRUE);
}

/*
 * This sets up a connection to the master server, and either gets
 * ypall_callback to do all the work of writing it to the local dbm file
 * (if the ypserv is current version), or does it itself for an old ypserv.  
 */
static bool
get_map(pushstat)
	int *pushstat;
{
	CLIENT *clnt;
	enum clnt_stat s;
	struct ypreq_nokey allreq;
	struct ypall_callback cbinfo;
	bool retval = FALSE;
	int tmpstat = 0;

	cbinfo.foreach = ypall_callback;
	cbinfo.data = (char *) &tmpstat;
	
	if ((clnt = clnt_create(master_name, YPPROG, 
	    YPVERS, "circuit_n")) == (CLIENT *) NULL) {
		logprintf(gettxt(":16", "Unable to contact ypserv on %s"), 
				master_name);
		clnt_pcreateerror("");
		*pushstat = YPPUSH_RPC;
		return(FALSE);
	}

	entry_count = 0;

	allreq.domain = source;
	allreq.map = map;

	s = clnt_call(clnt, YPPROC_ALL,
	    (xdrproc_t)xdr_ypreq_nokey, (caddr_t)&allreq,
	    (xdrproc_t)xdr_ypall, (caddr_t)&cbinfo, map_timeout);

	if (tmpstat == 0) {
		
		if (s == RPC_SUCCESS) {
			retval = TRUE;
		} else {
			logprintf(gettxt(":18", "Call to ypserv on %s failed"), 
					master_name);
			(void) clnt_perror(clnt, "get_map");
			*pushstat = YPPUSH_RPC;
		}
			
	} else {
		*pushstat = tmpstat;
	}

cleanup:
	clnt_destroy(clnt);
	return (retval);
}

/*
 * This sticks each key-value pair into the current map.  It returns FALSE as
 * long as it wants to keep getting called back, and TRUE on error conditions
 * and "No more k-v pairs".
 */
static int
ypall_callback(status, key, kl, val, vl, pushstat)
	int status;
	char *key;
	int kl;
	char *val;
	int vl;
	int *pushstat;
{
	datum keydat;
	datum valdat;
	datum test;

	if (status != YP_TRUE) {
		
		if (status != YP_NOMORE) {
			logprintf(
			    gettxt(":55", "Error from ypserv on %s (ypall_callback) = %s.\n"),
			    master, yperr_string(ypprot_err(status)));
			*pushstat = map_yperr_to_pusherr(status);
		}
		
		return(TRUE);
	}

	keydat.dptr = key;
	keydat.dsize = kl;
	valdat.dptr = val;
	valdat.dsize = vl;
	entry_count++;

# ifdef PARANOID
	test = fetch(keydat);
	if (test.dptr!=NULL) {
		logprintf(gettxt(":56", "Duplicate key %s in map %s\n"), key,map);
		*pushstat  = YPPUSH_DBM;
		return(TRUE);
	}
# endif PARANOID
	if (store(keydat, valdat) < 0) {
		logprintf(gettxt(":57", "Can't do dbm store into temp map %s.\n"),map);
		*pushstat  = YPPUSH_DBM;
		return(TRUE);
	}
# ifdef PARANOID
	test = fetch(keydat);
	if (test.dptr==NULL) {
		logprintf(gettxt(":58", "Key %s was not inserted into dbm file %s\n"), key, map);
		*pushstat  = YPPUSH_DBM;
		return(TRUE);
	}
# endif PARANOID
	return(FALSE);
}

/*
 * This maps a YP_xxxx error code into a YPPUSH_xxxx error code
 */
static int
map_yperr_to_pusherr(yperr)
	int yperr;
{
	int reason;

	switch (yperr) {
	
 	case YP_NOMORE:
		reason = YPPUSH_SUCC;
		break;

 	case YP_NOMAP:
		reason = YPPUSH_NOMAP;
		break;

 	case YP_NODOM:
		reason = YPPUSH_NODOM;
		break;

 	case YP_NOKEY:
		reason = YPPUSH_YPERR;
		break;

 	case YP_BADARGS:
		reason = YPPUSH_BADARGS;
		break;

 	case YP_BADDB:
		reason = YPPUSH_YPERR;
		break;

	default:
		reason = YPPUSH_XFRERR;
		break;
	}
	
  	return(reason);
}

/*
 * This writes the last-modified and master entries into the new dbm file
 */
static bool
add_private_entries(pname)
	char *pname;
{
	datum key;
	datum val;

	if (!fake_master_version) {
		key.dptr = yp_last_modified;
		key.dsize = yp_last_modified_sz;
		val.dptr = master_ascii_version;
		val.dsize = strlen(master_ascii_version);

		if (store(key, val) < 0) {
			logprintf(gettxt(":57",
			    "Can't do dbm store into temp map %s.\n"),
		    	    pname);
			return (FALSE);
		}
		entry_count++;
	}
	
	if (master_name) {
		key.dptr = yp_master_name;
		key.dsize = yp_master_name_sz;
		val.dptr = master_name;
		val.dsize = strlen(master_name);
		if (store(key, val) < 0) {
			logprintf(gettxt(";57",
			    "Can't do dbm store into temp map %s.\n"),
		    	    pname);
			return (FALSE);
		}
		entry_count++;
	}
	
	if (secure_map) {
		key.dptr = yp_secure;
		key.dsize = yp_secure_sz;
		val.dptr = yp_secure;
		val.dsize = yp_secure_sz;
		if (store(key, val) < 0) {
			logprintf(gettxt(":57",
			    "Can't do dbm store into temp map %s.\n"),
		    	    pname);
			return (FALSE);
		}
		entry_count++;
	}
	
	return (TRUE);
}
	
	
/*
 * This sends a YPPROC_CLEAR message to the local ypserv process.
 */
static bool
send_ypclear(pushstat)
	int *pushstat;
{
	struct dom_binding domb;
	char local_host_name[256];
	int status;

	if (gethostname(local_host_name, 256)) {
		logprintf( gettxt(":59", "Can't get local machine name.\n"));
		*pushstat = YPPUSH_RSRC;
		return (FALSE);
	}

	if (!bind_to_server(local_host_name, &domb, &status) ) {
		*pushstat = YPPUSH_CLEAR;
		return (FALSE);
	}

	if((enum clnt_stat) clnt_call(domb.dom_client,
	    YPPROC_CLEAR, xdr_void, 0, xdr_void, 0,
	    lg_timeout) != RPC_SUCCESS) {
		logprintf(
		gettxt(":60", "Can't send ypclear message to ypserv on the local machine.\n"));
		(void) clnt_perror(domb.dom_client, "send_ypclear");
		xfr_exit(YPPUSH_CLEAR);
	}

	return (TRUE);
}

/*
 * This decides if send_callback has to get called, and does the process exit.
 */
static void
xfr_exit(status)
	int status;
{
	if (callback) {
		send_callback(&status);
	}

	if (status == YPPUSH_SUCC || status == YPPUSH_AGE) {
		exit(0);
	} else {
		exit(1);
	}
}

/*
 * This sets up a connection to the yppush process which contacted our
 * parent ypserv, and sends him a status on the requested transfer.
 */
static void
send_callback(status)
	int *status;
{
	struct yppushresp_xfr resp;
	struct dom_binding domb;
	struct timeval timeout;

	resp.transid = (unsigned long) atoi(tid);
	resp.status = (unsigned long) *status;

	timeout.tv_sec = TIMEOUT*6;
	timeout.tv_usec = 0;

	if (oldxfr) {
		struct netbuf netBuffer;
		struct sockaddr_in sin;
		struct netconfig *nconf;

		sin.sin_addr.s_addr = inet_addr (ipaddr);
		sin.sin_family = AF_INET;
		sin.sin_port = htons (atoi (port));
		netBuffer.maxlen = sizeof (struct sockaddr_in);
		netBuffer.len    = sizeof (struct sockaddr_in);
		netBuffer.buf    = (char *) &sin;
		nconf = getnetconfigent("udp");
		if ((domb.dom_client = clnt_tli_create(RPC_ANYFD, nconf,
				    &netBuffer, (unsigned long) atoi (proto),
				    YPPUSHVERS, NULL, NULL)) == NULL) {
			*status = YPPUSH_RPC;
			return;
		}
       } else {
		if ((domb.dom_client = clnt_create(name, 
		    (unsigned long) atoi(proto),
		    YPPUSHVERS, "udp")) == NULL) {
			*status = YPPUSH_RPC;
			return;
		}	
        }

	if((enum clnt_stat) clnt_call(domb.dom_client, YPPUSHPROC_XFRRESP,
	     (xdrproc_t)xdr_yppushresp_xfr, (caddr_t)&resp,
	     xdr_void, 0, timeout)
	    != RPC_SUCCESS) {
		logprintf(gettxt(":18","Call to ypserv on %s failed"),name);
		(void) clnt_perror(domb.dom_client, "send_callback");
		*status = YPPUSH_RPC;
		return;
	} 
}

#if MAP_TO_SYSTEM
int
updateSystemFiles(map)
      char *map;
{
      char cmd[1024];
      struct stat statBuffer;
      int ex;

      if (stat (map_to_system, &statBuffer) == 0) { /* does the file exist ? */
	  (void) strcpy(cmd, map_to_system);
	  (void) strcat(cmd, " ");
	  (void) strcat(cmd, map);
	  if (debug) pfmt (stderr, MM_NOSTD, ":61:command: %s\n", cmd);
	  if ((ex = system(cmd)) != 0) {
	      logprintf(gettxt(":62", "update of system file for map %s failed, exit code %d!\n"),
			map, ex);
	      return (FALSE);
	  }
      }
      return (TRUE);
}
#endif /* MAP_TO_SYSTEM */
