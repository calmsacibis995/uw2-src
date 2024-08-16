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

#ident	"@(#)ypcmd:ypcat.c	1.3.10.6"
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
 * This is a user command which dumps each entry in a yp data base.  It gets
 * the stuff using the normal ypclnt package; the user doesn't get to choose
 * which server gives him the input.  Usage is:
 * ypcat [-k] [-d domain] map
 * where the -k switch will dump keys followed by a single blank space
 * before the value, and the -d switch can be used to specify a domain other
 * than the default domain.
 * 
 */
#ifdef NULL
#undef NULL
#endif
#define NULL 0
#include <limits.h>
#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <rpc/rpc.h>
#include "yp_b.h"
#include "ypsym.h"

/*
 * SNI: sysvconfig and yp_getalias not needed here!
 *      we need this stuff only on yp servers
 */

static int translate = TRUE;
static int aliases = FALSE;
static int dumpkeys = FALSE;
static char *domain = NULL;
static char default_domain_name[YPMAXDOMAIN];
static char *map = NULL;
static char nullstring[] = "";
static char domain_alias[YPMAXDOMAIN]; 	/* nickname for domain */
static char map_alias[YPMAXMAP];	/* nickname for map */

typedef struct err_msg {
	char *num;
	char *msg;
} ERR_MSG; 

static ERR_MSG err_usage_t = {
":1", "Usage:\n\
	ypcat [-k] [-d domainname] [-t] mapname\n\
	ypcat -x\n\
where\n\
	mapname may be either a mapname or an alias for a map.\n\
	-t inhibits map alias translation.\n\
	-k prints keys as well as values.\n\
	-x shows map aliases.\n"};
static ERR_MSG err_bad_args_t = {
	":2", "%s argument is bad.\n"};
static ERR_MSG err_cant_get_kname_t = {
	":3", "can't get %s back from system call.\n"};
static ERR_MSG err_null_kname_t = {
	":4", "the %s hasn't been set on this machine.\n"};
static ERR_MSG err_bad_domainname_t = {
	":5", "domainname"};
static ERR_MSG err_cant_bind_t = {
	":6", "can't bind to yp server for domain %s.\n		 Reason:  %s.\n"};
static ERR_MSG err_first_failed_t = {
	":7", "can't get first record from yp.\n		 Reason:  %s.\n"};
static ERR_MSG err_next_failed_t = {
	":8", "can't get next record from yp.\n		 Reason:  %s.\n"};

#define err_usage \
	gettxt(err_usage_t.num, err_usage_t.msg)
#define err_bad_args \
	gettxt(err_bad_args_t.num, err_bad_args_t.msg)
#define err_cant_get_kname  \
	gettxt(err_cant_get_kname_t.num, err_cant_get_kname_t.msg)
#define err_null_kname \
	gettxt(err_null_kname_t.num, err_null_kname_t.msg)
#define err_cant_bind \
	gettxt(err_cant_bind_t.num, err_cant_bind_t.msg)
#define err_first_failed \
	gettxt(err_first_failed_t.num, err_first_failed_t.msg)
#define err_next_failed \
	gettxt(err_next_failed_t.num, err_next_failed_t.msg)
#define err_bad_domainname \
	gettxt(err_bad_domainname_t.num, err_bad_domainname_t.msg)

#define alias_fmt \
	gettxt(":12","Use \"%s\" for %s \"%s\"\n")
#define alias_dom \
	gettxt(":13","domain")
#define alias_map \
	gettxt(":14","map")

static void get_command_line_args();
static int callback();
static void one_by_one_all();

extern size_t strlen();
extern int strcmp();
extern int getdomainname();
extern void exit();
extern void free();
extern char *gettxt();
extern listofnames *names();
extern char *strtok();

/*
 * This is the mainline for the ypcat process.  It pulls whatever arguments
 * have been passed from the command line, and uses defaults for the rest.
 */

main (argc, argv)
	int argc;
	char **argv;
	
{
	int err;
	int fail=0;
	struct ypall_callback cbinfo;

	(void)setlocale(LC_ALL,"");
	(void)setcat("uxypcat");
	(void)setlabel("UX:ypcat");

	sysvconfig();

	get_command_line_args(argc, argv);

	if (aliases) {
		yp_listaliases();
		exit(0);
	}
	if (!domain) {
		
		if (!getdomainname(default_domain_name, YPMAXDOMAIN) ) {
			domain = default_domain_name;
		} else {
			(void) pfmt(stderr, MM_NOGET, 
				 err_cant_get_kname, err_bad_domainname);
			exit(1);
		}

		if ((int)strlen(domain) == 0) {
			(void) pfmt(stderr, MM_NOGET, err_null_kname, 
				err_bad_domainname);
			exit(1);
		}
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

	if (err = yp_bind(domain_alias) ) {
		(void) pfmt(stderr, MM_NOGET, err_cant_bind, domain_alias,
		    yperr_string(err) );
		exit(1);
	}

	cbinfo.foreach = callback;
	cbinfo.data = (char *) &fail;
	err = yp_all(domain_alias, map_alias, &cbinfo);

	if (err == YPERR_VERS) {
		one_by_one_all(domain_alias, map_alias);
	}
	
	exit(fail);
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

			case 't':
				translate = FALSE;
				argv++;
				break;

			case 'x':
				aliases = TRUE;
				argv++;
				break;

			case 'k': 
				dumpkeys = TRUE;
				argv++;
				break;
				
			case 'd': 

				if (argc > 1) {
					argv++;
					argc--;
					domain = *argv;
					argv++;

					if ((int)strlen(domain) > YPMAXDOMAIN) {
						(void) pfmt(stderr, MM_NOGET, err_bad_args,
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
				argv++;
			} else {
				(void) pfmt(stderr, MM_NOGET, err_usage);
				exit(1);
			}
		}
	}

	if (!map && !aliases) {
		(void) pfmt(stderr, MM_NOGET, err_usage);
		exit(1);
	}
}

/*
 * This dumps out the value, optionally the key, and perhaps an error message.
 */
static int
callback(status, key, kl, val, vl, fail)
	int status;
	char *key;
	int kl;
	char *val;
	int vl;
	int *fail;
{
	int e;

	if (status == YP_TRUE) {

		if (dumpkeys)
			(void) pfmt(stdout, MM_NOSTD, ":9:%.*s ", kl, key);

		(void) pfmt(stdout, MM_NOSTD, ":10:%.*s\n", vl, val);
		return (FALSE);
	} else {

		e = ypprot_err(status);

		if (e != YPERR_NOMORE) {
			(void) pfmt(stderr, MM_STD, ":11:%s\n", yperr_string(e));
			*fail = TRUE;
		}
		
		return (TRUE);
	}
}

/*
 * This cats the map out by using the old one-by-one enumeration interface.
 * As such, it is prey to the old-style problems of rebinding to different
 * servers during the enumeration.
 */
static void
one_by_one_all(domain, map)
char *domain;
char *map;
{
	char *key;
	int keylen;
	char *outkey;
	int outkeylen;
	char *val;
	int vallen;
	int err;

	key = nullstring;
	keylen = 0;
	val = nullstring;
	vallen = 0;
	
	if (err = yp_first(domain, map, &outkey, &outkeylen, &val, &vallen) ) {

		if (err == YPERR_NOMORE) {
			exit(0);
		} else {
			(void) pfmt(stderr, MM_NOGET, err_first_failed,
			    yperr_string(err) );
			exit(1);
		}
	}

	for (;;) {

		if (dumpkeys) {
			(void) pfmt(stdout, MM_NOSTD, ":9:%.*s ", outkeylen, outkey);
		}

		(void) pfmt(stdout, MM_NOSTD, ":10:%.*s\n", vallen, val);
		free(val);
		key = outkey;
		keylen = outkeylen;
		
		if (err = yp_next(domain, map, key, keylen, &outkey, &outkeylen,
		    &val, &vallen) ) {

			if (err == YPERR_NOMORE) {
				break;
			} else {
				(void) pfmt(stderr, MM_NOGET,  err_next_failed,
				    yperr_string(err) );
				exit(1);
			}
		}

		free(key);
	}
}
