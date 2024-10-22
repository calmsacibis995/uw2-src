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

#ident	"@(#)keyserv:newkey.c	1.9.8.7"
#ident	"$Header: $"

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
 * Copyright (C) 1986, Sun Microsystems, Inc.
 */

/*
 * Administrative tool to add a new user to the publickey database
 */
#include <stdio.h>
#include <stdlib.h>
#include <rpc/rpc.h>
#include <rpc/key_prot.h>
#ifdef YP
#include <rpcsvc/ypclnt.h>
#include <sys/wait.h>
#include <netdb.h>
#endif	/* YP */
#include <pwd.h>
#include <string.h>
#include <sys/resource.h>
#include <netconfig.h>
#include <netdir.h>

#ifdef YP
#define	MAXMAPNAMELEN 256
#else
#define	YPOP_CHANGE 1			/* change, do not add */
#define	YPOP_INSERT 2			/* add, do not change */
#define	YPOP_DELETE 3			/* delete this entry */
#define	YPOP_STORE  4			/* add, or change */
#define	ERR_ACCESS	1
#define	ERR_MALLOC	2
#define	ERR_READ	3
#define	ERR_WRITE	4
#define	ERR_DBASE	5
#define	ERR_KEY		6
#endif

#ifdef YP
static char *basename();
static char YPDBPATH[]="/var/yp";
static char PKMAP[] = "publickey.byname";
#else
static char PKFILE[] = "/etc/publickey";
static char *err_string();
#endif	/* YP */

main(argc, argv)
	int argc;
	char *argv[];
{
	char name[MAXNETNAMELEN + 1];
	char public[HEXKEYBYTES + 1];
	char secret[HEXKEYBYTES + 1];
	char crypt1[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	char crypt2[HEXKEYBYTES + KEYCHECKSUMSIZE + 1];
	int status;
	char *pass;
	struct passwd *pw;
	NCONF_HANDLE *nc_handle;
	struct netconfig *nconf;
	struct nd_hostserv service;
	struct nd_addrlist *addrs;
	bool_t validhost;

	if (argc != 3 || !(strcmp(argv[1], "-u") == 0 ||
		strcmp(argv[1], "-h") == 0)) {
		(void)fprintf(stderr, "usage: %s [-u username]\n",
					argv[0]);
		(void)fprintf(stderr, "usage: %s [-h hostname]\n",
					argv[0]);
		exit(1);
	}
	if (geteuid() != 0) {
		(void)fprintf(stderr, "must be superuser to run %s\n", argv[0]);
		exit(1);
	}

#ifdef YP
	if (chdir(YPDBPATH) < 0) {
		(void)fprintf(stderr, "cannot chdir to ");
		perror(YPDBPATH);
	}
#endif	/* YP */
	if (strcmp(argv[1], "-u") == 0) {
		pw = getpwnam(argv[2]);
		if (pw == NULL) {
			(void)fprintf(stderr, "unknown user: %s\n", argv[2]);
			exit(1);
		}
		(void)user2netname(name, (int)pw->pw_uid, (char *)NULL);
	} else {
		/* -h hostname option */
		service.h_host = argv[2];
		service.h_serv = "111"; 
		validhost = FALSE;
		/* verify if this is a valid hostname */
		nc_handle = setnetconfig();
		if (nc_handle == NULL) {
			/* fails to open netconfig file */
			fprintf(stderr, 
				"error: failed in routine setnetconfig(0)\n");
			exit(2);
		}
		while (nconf = getnetconfig(nc_handle)) {
			/* check to see if hostname exists for this transport */
			if ( (netdir_getbyname(nconf, &service, &addrs) == 0) &&
			    (addrs->n_cnt != 0)) {
				/* atleast one valid address */
				validhost = TRUE;
				break;
			}
		}
		endnetconfig(nc_handle);
		if (!validhost) {
			(void)fprintf(stderr, "unknown host: %s\n", argv[2]);
			exit(1);
		}
		
		(void)host2netname(name, argv[2], (char *)NULL);
	}

	(void)printf("Adding new key for %s.\n", name);
	pass = getpass("New password:");
	genkeys(public, secret, pass);

	memcpy(crypt1, secret, HEXKEYBYTES);
	memcpy(crypt1 + HEXKEYBYTES, secret, KEYCHECKSUMSIZE);
	crypt1[HEXKEYBYTES + KEYCHECKSUMSIZE] = 0;
	xencrypt(crypt1, pass);

	if (memcmp(crypt1, secret, HEXKEYBYTES) == 0) {
		(void)fprintf(stderr, "WARNING: Password verification is disabled\n");
		(void)fprintf(stderr,
			"Possible Cause: The \"crypt\" package is not installed\n");
	}

	memcpy(crypt2, crypt1, HEXKEYBYTES + KEYCHECKSUMSIZE + 1);
	xdecrypt(crypt2, getpass("Retype password:"));
	if (memcmp(crypt2, crypt2 + HEXKEYBYTES, KEYCHECKSUMSIZE) != 0 ||
		memcmp(crypt2, secret, HEXKEYBYTES) != 0) {
		(void)fprintf(stderr, "Password incorrect.\n");
		exit(1);
	}

#ifdef YP
	(void)printf("Please wait for the database to get updated...\n");
#endif
	if (status = setpublicmap(name, public, crypt1)) {
#ifdef YP
		(void)fprintf(stderr,
		"%s: unable to update yp database (%u): %s\n",
			argv[0], status, yperr_string(status));
#else
		(void)fprintf(stderr,
		"%s: unable to update publickey database (%u): %s\n",
			argv[0], status, err_string(status));
#endif
		exit(1);
	}
	(void)printf("Your new key has been successfully stored away.\n");
	exit(0);
	/* NOTREACHED */
}

/*
 * Set the entry in the public key file
 */
setpublicmap(name, public, secret)
	char *name;
	char *public;
	char *secret;
{
	char pkent[1024];

	(void)sprintf(pkent, "%s:%s", public, secret);
#ifdef YP
	return (mapupdate(name, PKMAP, YPOP_STORE,
		strlen(name), name, strlen(pkent), pkent));
#else
	return (localupdate(name, PKFILE, YPOP_STORE,
		strlen(name), name, strlen(pkent), pkent));
#endif
}

#ifndef YP
/*
 * This returns a pointer to an error message string appropriate
 * to an input error code.  An input value of zero will return
 * a success message.
 */
static char *
err_string(code)
	int code;
{
	char *pmesg;

	switch (code) {
	case 0:
		pmesg = "update operation succeeded";
		break;
	case ERR_KEY:
		pmesg = "no such key in file";
		break;
	case ERR_READ:
		pmesg = "cannot read the database";
		break;
	case ERR_WRITE:
		pmesg = "cannot write to the database";
		break;
	case ERR_DBASE:
		pmesg = "cannot update database";
		break;
	case ERR_ACCESS:
		pmesg = "permission denied";
		break;
	case ERR_MALLOC:
		pmesg = "malloc failed";
		break;
	default:
		pmesg = "unknown error";
		break;
	}
	return (pmesg);
}
#endif
