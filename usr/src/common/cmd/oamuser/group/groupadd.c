/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamuser:group/groupadd.c	1.4.19.6"
#ident  "$Header: $"

/*
 * Command:	groupadd
 *
 * Usage:	groupadd [-g gid [-o]] group
 *
 * Inheritable Privileges:	P_MACWRITE,P_AUDIT,P_SETFLEVEL,P_DACWRITE
 *				P_MACREAD,P_DACREAD
 *       Fixed Privileges:	None
 *
 * Notes:	Add (create) a new group definition on the system.
 *
 *		Arguments are:
 *
 *			gid - a gid_t less than MAXUID
 *			group - a string of printable characters excluding
 *				colon(:) and less than MAXGLEN characters long.
 *
 *		P_MACWRITE is required for writing to /etc/group.
 */

/* LINTLIBRARY */
#include	<sys/types.h>
#include	<sys/param.h>
#include	<stdio.h>
#include	<ctype.h>
#include	<limits.h>
#include	<userdefs.h>
#include	<string.h>
#include	<users.h>
#include	"messages.h"
#include	<audit.h>
#include	<priv.h>
#include	<pfmt.h>
#include	<locale.h>
#include <rpc/rpc.h>
#include <rpc/rpc_com.h>
#include <rpc/rpcb_prot.h>
#include <rpcsvc/yp_prot.h>
#include <rpcsvc/ypclnt.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <netconfig.h>
#include <stddef.h>

static char *domain;
#define       EX_NO_NIS         47
#define       EX_NO_NISMATCH    48
#define       EX_UNK_NIS        49


extern	char	*optarg,		/* used by getopt */
		*errmsgs[];

extern int optind, opterr;	/* used by getopt */

extern int getopt(), errmsg(), valid_gname();
extern gid_t findnextgid();
extern int valid_gid(), add_group();
extern char	*argvtostr();
extern long strtol();
extern void exit(), adumprec();

char *msg_label = "UX:groupadd";
static char *cmdline = (char *)0;

#define nisname(n) (*n == '+' || *n == '-')

/*
 * Procedure:     main
 *
 * Restrictions:
 *                printf: none
 *                getopt: none
 */

main(argc, argv)
int argc;
char *argv[];
{
	int ch;				/* return from getopt */
	gid_t gid;			/* group id */
	int oflag = 0;	/* flags */
	register rc;
	char *ptr;
	char *gidstr = NULL;	/* gid from command line */
	char *grpname;			/* group name from command line */
	int niserr=0;

	(void) setlocale(LC_ALL, "");
	(void) setcat("uxcore");
	(void) setlabel(msg_label);
	
	/* save command line arguments */
	if (( cmdline = (char *)argvtostr(argv)) == NULL) {
                printf("failed argvtostr()\n");
		adumprec(ADT_ADD_GRP,1,strlen(argv[0]),argv[0]);
                exit(1);
        }

	while((ch = getopt(argc, argv, "g:o")) != EOF)
		switch(ch) {
			case 'g':
				gidstr = optarg;
				break;
			case 'o':
				oflag++;
				break;
			case '?':
				errmsg( M_AUSAGE );
				adumprec(ADT_ADD_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
				exit( EX_SYNTAX );
		}

	if( (oflag && !gidstr) || optind != argc - 1 ) {
		errmsg( M_AUSAGE );
		adumprec(ADT_ADD_GRP,EX_SYNTAX,strlen(cmdline),cmdline);
		exit( EX_SYNTAX );
	}


	grpname = argv[optind];

	switch( valid_gname( grpname, NULL ) ) {
	case INVALID:
		errmsg( M_GRP_INVALID, grpname );
		adumprec(ADT_ADD_GRP,EX_BADARG,strlen(cmdline),cmdline);
		exit( EX_BADARG );
		/*NOTREACHED*/
	case NOTUNIQUE:
		errmsg( M_GRP_USED, grpname );
		adumprec(ADT_ADD_GRP,EX_NAME_EXISTS,strlen(cmdline),cmdline);
		exit( EX_NAME_EXISTS );
		/*NOTREACHED*/
	}

	if (nisname(grpname)) {
		if((niserr = nis_getgrp(grpname+1)) != 0) {
			switch (niserr) {
				case YPERR_YPBIND:
				 	(void) pfmt(stderr, MM_ERROR, ":1338:NIS not available\n");
					adumprec(ADT_ADD_GRP,EX_NO_NIS,strlen(cmdline),cmdline);
					exit( EX_NO_NIS );
					break;
				case YPERR_KEY:
				 	(void) pfmt(stderr, MM_ERROR, ":1349:%s not found in NIS group map\n", grpname+1);
					adumprec(ADT_ADD_GRP,EX_NO_NISMATCH,strlen(cmdline),cmdline);
					exit( EX_NO_NISMATCH );
					break;
				default:
				 	(void) pfmt(stderr, MM_ERROR, ":1340:Unknown NIS error\n");
					adumprec(ADT_ADD_GRP,EX_UNK_NIS,strlen(cmdline),cmdline);
					exit( EX_UNK_NIS );
						break;
				}
		}
		gid = 1; /* group default */
	} else if ( gidstr ) {
		/* Given a gid string - validate it */
		char *ptr;

		gid = (gid_t) strtol( gidstr, &ptr, 10 );

		if( *ptr ) {
			errmsg( M_GID_INVALID, gidstr );
			adumprec(ADT_ADD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );
		}

		switch( valid_gid( gid, NULL ) ) {
		case RESERVED:
			(void) pfmt(stderr, MM_WARNING, errmsgs[M_RESERVED], gid);
			break;

		case NOTUNIQUE:
			if( !oflag ) {
				errmsg( M_GRP_USED, gidstr );
				adumprec(ADT_ADD_GRP,EX_ID_EXISTS,strlen(cmdline),cmdline);
				exit( EX_ID_EXISTS );
			}
			break;

		case INVALID:
			errmsg( M_GID_INVALID, gidstr );
			adumprec(ADT_ADD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );

		case TOOBIG:
			errmsg( M_TOOBIG, gid );
			adumprec(ADT_ADD_GRP,EX_BADARG,strlen(cmdline),cmdline);
			exit( EX_BADARG );

		}

	} else {
		if( (gid = findnextgid()) < 0 ) {
			errmsg( M_GID_INVALID, "default id" );
			exit( EX_ID_EXISTS );
		}
	} 

	if( (rc = add_group(grpname, gid) ) != EX_SUCCESS )
		errmsg( M_UPDATE, "created" );

	adumprec(ADT_ADD_GRP,rc,strlen(cmdline),cmdline);
	exit( rc );
	/*NOTREACHED*/
}
/*
 * Checks to see if NIS is up and running
 */
nis_check()
{
	if (domain == NULL) {
		if (yp_get_default_domain(&domain)){
			return(-1);
		}
	}
	return(0);
}
nis_getgrp(grp)
char *grp;
{
	register char *bp;
	char *val = NULL;
	int vallen, err;
	char *key = grp;

	if (nis_check() < 0)
		return(-1);

	if (err = yp_match(domain, "group.byname", key, strlen(key),
				&val, &vallen)) {
				return(err);
	}
}
