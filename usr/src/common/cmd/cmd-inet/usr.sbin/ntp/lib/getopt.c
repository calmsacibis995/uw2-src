/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/ntp/lib/getopt.c	1.2"
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
 * getopt - get option letter from argv
 *
 * This is a version of the public domain getopt() implementation by
 * Henry Spencer, changed for 4.3BSD compatibility (in addition to System V).
 * It allows rescanning of an option list by setting optind to 0 before
 * calling.  Thanks to Dennis Ferguson for the appropriate modifications.
 *
 * This file is in the Public Domain.
 */

/*LINTLIBRARY*/

#include <stdio.h>

#ifdef	lint
#undef	putc
#define	putc	fputc
#endif	/* lint */

char	*optarg;	/* Global argument pointer. */
int	optind = 0;	/* Global argv index. */

/*
 * N.B. use following at own risk
 */
int	opterr = 1;	/* for compatibility, should error be printed? */
int	optopt;		/* for compatibility, option character checked */

static char	*scan = NULL;	/* Private scan pointer. */

/*
 * Print message about a bad option.  Watch this definition, it's
 * not a single statement.
 */
#define	BADOPT(mess, ch)	if (opterr) { \
					fputs(argv[0], stderr); \
					fputs(mess, stderr); \
					(void) putc(ch, stderr); \
					(void) putc('\n', stderr); \
				} \
				return('?')

int
getopt(argc, argv, optstring)
	int argc;
	char *argv[];
	char *optstring;
{
	register char c;
	register char *place;

	optarg = NULL;

	if (optind == 0) {
		scan = NULL;
		optind++;
	}
	
	if (scan == NULL || *scan == '\0') {
		if (optind >= argc || argv[optind][0] != '-' || argv[optind][1] == '\0')
			return EOF;
		if (argv[optind][1] == '-' && argv[optind][2] == '\0') {
			optind++;
			return EOF;
		}
	
		scan = argv[optind]+1;
		optind++;
	}

	c = *scan++;
	optopt = c & 0377;
	for (place = optstring; place != NULL && *place != '\0'; ++place)
		if (*place == c)
			break;

	if (place == NULL || *place == '\0' || c == ':' || c == '?') {
		BADOPT(": unknown option -", c);
	}

	place++;
	if (*place == ':') {
		if (*scan != '\0') {
			optarg = scan;
			scan = NULL;
		} else if (optind >= argc) {
			BADOPT(": option requires argument -", c);
		} else {
			optarg = argv[optind];
			optind++;
		}
	}

	return c&0377;
}
