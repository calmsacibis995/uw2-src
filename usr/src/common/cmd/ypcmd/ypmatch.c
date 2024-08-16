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

#ident	"@(#)ypcmd:ypmatch.c	1.2.7.5"
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
 * This is a user command which looks up the value of a key in a map
 *
 * Usage is:
 *	ypmatch [-d domain] [-k] key [key ...] mname 
 *
 * where:  the -d switch can be used to specify a domain other than the
 * default domain.  mname is a mapname.  The -k switch prints keys as 
 * well as values.
 */

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <rpc/rpc.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>

static void get_command_line_args();
static void getdomain();
static bool match_list();
static bool match_one();
static void print_one();

extern void exit();
extern int strcmp();
extern unsigned int strlen();
extern int getdomainname();
extern void free();
extern char *gettxt();

static int translate = TRUE;
static int printkeys = FALSE;
static char *domain = NULL;
static char default_domain_name[YPMAXDOMAIN];
static char *map = NULL;
static char **keys = NULL;
static int nkeys, aliases;
static char domain_alias[YPMAXDOMAIN]; 	/* nickname for domain */
static char map_alias[YPMAXMAP];	/* nickname for map */

typedef struct err_msg {
	char *num;
	char *msg;
} ERR_MSG; 

static ERR_MSG  err_usage_t = {
":1", "Usage:\n\
	ypmatch [-d domain] [-k] key [key ...] [-t] mname\n\
	ypmatch -x\n\
where\n\
	mname may be either a mapname or an alias for a map.\n\
	-t inhibits map alias translation.\n\
	-k prints keys as well as values.\n\
	-x shows map aliases.\n"};
static ERR_MSG  err_bad_args_t = {
	":2", "%s argument is bad.\n"};
static ERR_MSG  err_cant_get_kname_t = {
	":3", "can't get %s back from system call.\n"};
static ERR_MSG  err_null_kname_t = {
	":4", "the %s hasn't been set on this machine.\n"};
static ERR_MSG  err_bad_mapname_t = {
	":5", "mapname"};
static ERR_MSG  err_bad_domainname_t = {
	":6", "domainname"};

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

/*
 * This is the main line for the ypmatch process.
 */
main(argc, argv)
	char **argv;
{
 
	(void)setlocale(LC_ALL,"");
	(void)setcat("uxypmatch");
	(void)setlabel("UX:ypmatch");

	sysvconfig();

	get_command_line_args(argc, argv);

	if (aliases){
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

	if (!match_list())
		return(1);
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
		
	if (argc < 2) {
		(void) pfmt(stderr, MM_NOGET, err_usage);
		exit(1);
	}
	argv++;

	while (--argc > 0 && (*argv)[0] == '-') {

		switch ((*argv)[1]) {

		case 't':
			translate = FALSE;
			break;

		case 'k':
			printkeys = TRUE;
			break;

		case 'x':
			aliases = TRUE;
			break;

		case 'd':

			if (argc > 1) {
				argv++;
				argc--;
				domain = *argv;

				if ((int) strlen(domain) > YPMAXDOMAIN) {
					(void) pfmt(stderr, MM_NOGET,
					  err_bad_args, err_bad_domainname);
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

		argv++;
	}

	if (aliases)
		return;

	if (argc < 2) {
		(void) pfmt(stderr, MM_NOGET, err_usage);
		exit(1);
	}

	keys = argv;
	nkeys = argc -1;
	map = argv[argc -1];

	if ((int) strlen(map) > YPMAXMAP) {
		(void) pfmt(stderr, MM_NOGET,
			  err_bad_args, err_bad_mapname);
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
		(void) pfmt(stderr, MM_NOGET,
			  err_cant_get_kname, err_bad_domainname);
		exit(1);
	}

	if ((int) strlen(domain) == 0) {
		(void) pfmt(stderr, MM_NOGET,
			  err_null_kname, err_bad_domainname);
		exit(1);
	}
}

/*
 * This traverses the list of argument keys.
 */
static bool
match_list()
{
	bool error;
	bool errors = FALSE;
	char *val;
	int len;
	int n = 0;

	while (n < nkeys) {
		error = match_one(keys[n], &val, &len);

		if (!error) {
			print_one(keys[n], val, len);
			free(val);
		} else {
			errors = TRUE;
		}

		n++;
	}
	
	return (!errors);
}

/*
 * This fires off a "match" request to any old yp server, using the vanilla
 * yp client interface.  To cover the case in which trailing NULLs are included
 * in the keys, this retrys the match request including the NULL if the key
 * isn't in the map.
 */
static bool
match_one(key, val, len)
	char *key;
	char **val;
	int *len;
{
	int err;
	bool error = FALSE;

	*val = NULL;
	*len = 0;

	err = yp_match(domain_alias, map_alias, key, 
			(int)strlen(key), val, len);
	
	if (err == YPERR_KEY) {
		err = yp_match(domain_alias, map_alias, key, 
				((int)strlen(key) + 1), val, len);
	}
		
	if (err) {
		(void) pfmt(stderr, MM_STD,
		    ":7:Can't match key %s in map %s.\n\t\t   Reason: %s.\n",
			key, map_alias, yperr_string(err));
		error = TRUE;
	}
	
	return (error);
}

/*
 * This prints the value, (and optionally, the key) after first checking that
 * the last char in the value isn't a NULL.  If the last char is a NULL, the
 * \n\0 sequence which the yp client layer has given to us is shuffled back
 * one byte.
 */
static void
print_one(key, val, len)
	char *key;
	char *val;
	int len;
{
	if (printkeys) {
		(void) pfmt(stdout, MM_NOSTD, ":8:%s: ", key);
	}

	(void) pfmt(stdout, MM_NOSTD, ":9:%.*s\n", len, val);
}
