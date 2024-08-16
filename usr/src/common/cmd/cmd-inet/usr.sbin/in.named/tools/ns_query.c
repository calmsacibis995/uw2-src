/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/tools/ns_query.c	1.2"
#ident	"$Header: $"

/*
 *	STREAMware TCP
 *	Copyright 1987, 1993 Lachman Technology, Inc.
 *	All Rights Reserved.
 */

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 *	System V STREAMS TCP - Release 4.0
 *
 *  Copyright 1990 Interactive Systems Corporation,(ISC)
 *  All Rights Reserved.
 *
 *	Copyright 1987, 1988, 1989 Lachman Associates, Incorporated (LAI)
 *	All Rights Reserved.
 *
 *	The copyright above and this notice must be preserved in all
 *	copies of this source code.  The copyright above does not
 *	evidence any actual or intended publication of this source
 *	code.
 *
 *	This is unpublished proprietary trade secret source code of
 *	Lachman Associates.  This source code may not be copied,
 *	disclosed, distributed, demonstrated or licensed except as
 *	expressly authorized by Lachman Associates.
 *
 *	System V STREAMS TCP was jointly developed by Lachman
 *	Associates and Convergent Technologies.
 */
/*      SCCS IDENTIFICATION        */
/*
 * Copyright (c) 1986 Regents of the University of California.
 * All Rights Reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 */

#ifndef lint
char copyright[] =
"@(#) Copyright (c) 1986 Regents of the University of California.\n\
 All rights reserved.\n";
#endif /* not lint */

#ifndef lint
static char sccsid[] = "@(#)ns_query.c	4.8 (Berkeley) 6/1/90";
#endif /* not lint */

#include <sys/param.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/nameser.h>
#include <resolv.h>
#include <stdio.h>

main(argc, argv)
	int argc;
	char **argv;
{
	extern struct state _res;
	register struct hostent *hp;
	register char *s;

	if (argc >= 2 && strcmp(argv[1], "-d") == 0) {
		_res.options |= RES_DEBUG;
		argc--;
		argv++;
	}
	if (argc < 2) {
		fprintf(stderr, "usage: nsquery [-d] host [server]\n");
		exit(1);
	}
	if (argc == 3) {
		hp = gethostbyname(argv[2]);
		if (hp == NULL) {
			fprintf(stderr, "nsquery:");
			herror(argv[2]);
			exit(1);
		}
		printf("\nServer:\n");
		printanswer(hp);
		_res.nsaddr.sin_addr = *(struct in_addr *)hp->h_addr;
	}

	hp = gethostbyname(argv[1]);
	if (hp == NULL) {
		fprintf(stderr, "nsquery: %s: ", argv[1]);
		herror((char *)NULL);
		exit(1);
	}
	printanswer(hp);
	exit(0);
}

printanswer(hp)
	register struct hostent *hp;
{
	register char **cp;
	extern char *inet_ntoa();

	printf("Name: %s\n", hp->h_name);
#if BSD >= 43 || defined(h_addr)
	printf("Addresses:");
	for (cp = hp->h_addr_list; cp && *cp; cp++)
		printf(" %s", inet_ntoa(*(struct in_addr *)(*cp)));
	printf("\n");
#else
	printf("Address: %s\n", inet_ntoa(*(struct in_addr *)hp->h_addr));
#endif
	printf("Aliases:");
	for (cp = hp->h_aliases; cp && *cp && **cp; cp++)
		printf(" %s", *cp);
	printf("\n\n");
}

#ifdef SYSV
/*
 *******************************************************************************
 *
 *  herror --
 *	
 *	Converts an error code from gethostbyname into a character string.
 *	(Called if using 4.3BSD gethostbyname.)
 *
 *
 *******************************************************************************
 */

herror(errno) 
    int errno;
{
    switch(errno) {
	case HOST_NOT_FOUND:
		fprintf(stderr,"*** Host not found.\n");
		break;
	case TRY_AGAIN:
		fprintf(stderr,"*** Host not found, try again.\n");
		break;
	case NO_RECOVERY:
		fprintf(stderr,"*** No recovery, Host not found.\n");
		break;
	case NO_ADDRESS:
		fprintf(stderr,"*** No Address, look for MF record.\n");
		break;
	default:
		fprintf(stderr,"*** Unknown error %d from gethostbyname.\n", 
			errno);
		break;
	}
}
#endif /* SYSV */
