/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/gettable.c	1.2.9.2"
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
 * +++++++++++++++++++++++++++++++++++++++++++++++++++++++++
 * 		PROPRIETARY NOTICE (Combined)
 * 
 * This source code is unpublished proprietary information
 * constituting, or derived under license from AT&T's UNIX(r) System V.
 * In addition, portions of such source code were derived from Berkeley
 * 4.3 BSD under license from the Regents of the University of
 * California.
 * 
 * 
 * 
 * 		Copyright Notice 
 * 
 * Notice of copyright on this source code product does not indicate 
 * publication.
 * 
 * 	(c) 1986,1987,1988.1989  Sun Microsystems, Inc
 * 	(c) 1983,1984,1985,1986,1987,1988,1989,1990  AT&T.
 *	(c) 1990,1991  UNIX System Laboratories, Inc.
 * 	          All rights reserved.
 *  
 */


#include <sys/types.h>
#include <sys/socket.h>

#include <netinet/in.h>

#include <stdio.h>
#include <netdb.h>

#define	OUTFILE		"hosts.txt"	/* default output file */
#define	VERFILE		"hosts.ver"	/* default version file */
#define	QUERY		"ALL\r\n"	/* query to hostname server */
#define	VERSION		"VERSION\r\n"	/* get version number */

#define	equaln(s1, s2, n)	(!strncmp(s1, s2, n))

#ifdef SYSV
#define bcopy(a,b,c)	  memcpy(b,a,c)
#endif

struct	sockaddr_in sin;
char	buf[BUFSIZ];
char	*outfile = OUTFILE;

main(argc, argv)
	int argc;
	char *argv[];
{
	int s;
	register len;
	register FILE *sfi, *sfo, *hf;
	char *host;
	register struct hostent *hp;
	struct servent *sp;
	int version = 0;
	int beginseen = 0;
	int endseen = 0;
	extern int optind;

	while ( (s = getopt(argc, argv, "v")) != -1 ) {
		switch (s) {
		case 'v':
			version++;
			outfile = VERFILE;
			break;
		default:
			fprintf(stderr, "gettable: unknown option ignored\n");
			break;
		}
	}
	switch ( (argc - optind) ) {
	case 2:
		outfile = argv[optind+1];
		/* fall through */
	case 1:
		host = argv[optind];
		break;
	default:
		fprintf(stderr, "usage: gettable [-v] host [ file ]\n");
		exit(1);
	}
	sp = getservbyname("hostnames", "tcp");
	if (sp == NULL) {
		fprintf(stderr, "gettable: hostnames/tcp: unknown service\n");
		exit(3);
	}
	hp = gethostbyname(host);
	if (hp == NULL) {
		fprintf(stderr, "gettable: %s: ", host);
		herror((char *)NULL);
		exit(2);
	}
	host = hp->h_name;
	sin.sin_family = hp->h_addrtype;
	s = socket(hp->h_addrtype, SOCK_STREAM, 0);
	if (s < 0) {
		perror("gettable: socket");
		exit(4);
	}
	if (bind(s, &sin, sizeof (sin)) < 0) {
		perror("gettable: bind");
		exit(5);
	}
	bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
	sin.sin_port = sp->s_port;
	if (connect(s, &sin, sizeof (sin)) < 0) {
		perror("gettable: connect");
		exit(6);
	}
	fprintf(stderr, "Connection to %s opened.\n", host);
	sfi = fdopen(s, "r");
	sfo = fdopen(s, "w");
	if (sfi == NULL || sfo == NULL) {
		perror("gettable: fdopen");
		close(s);
		exit(1);
	}
	hf = fopen(outfile, "w");
	if (hf == NULL) {
		fprintf(stderr, "gettable: "); perror(outfile);
		close(s);
		exit(1);
	}
	fprintf(sfo, version ? VERSION : QUERY);
	fflush(sfo);
	while (fgets(buf, sizeof(buf), sfi) != NULL) {
		len = strlen(buf);
		buf[len-2] = '\0';
		if (!version && equaln(buf, "BEGIN", 5)) {
			if (beginseen || endseen) {
				fprintf(stderr,
				    "gettable: BEGIN sequence error\n");
				exit(90);
			}
			beginseen++;
			continue;
		}
		if (!version && equaln(buf, "END", 3)) {
			if (!beginseen || endseen) {
				fprintf(stderr,
				    "gettable: END sequence error\n");
				exit(91);
			}
			endseen++;
			continue;
		}
		if (equaln(buf, "ERR", 3)) {
			fprintf(stderr,
			    "gettable: hostname service error: %s", buf);
			exit(92);
		}
		fprintf(hf, "%s\n", buf);
	}
	fclose(hf);
	if (!version) {
		if (!beginseen) {
			fprintf(stderr, "gettable: no BEGIN seen\n");
			exit(93);
		}
		if (!endseen) {
			fprintf(stderr, "gettable: no END seen\n");
			exit(94);
		}
		fprintf(stderr, "Host table received.\n");
	} else
		fprintf(stderr, "Version number received.\n");
	close(s);
	fprintf(stderr, "Connection to %s closed\n", host);
	exit(0);
	/* NOTREACHED */
}
