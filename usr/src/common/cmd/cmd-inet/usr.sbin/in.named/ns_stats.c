/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.named/ns_stats.c	1.2"
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
 * All rights reserved.
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
static char sccsid[] = "@(#)ns_stats.c	4.10 (Berkeley) 6/27/90";
#endif /* not lint */

/**************************************************************************/
/*                simple monitoring of named behavior                     */
/*            dumps a bunch of values into a well-know file               */
/*                                                                        */
/**************************************************************************/

#ifdef STATS

#include <sys/types.h>
#include <sys/param.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <stdio.h>
#include <syslog.h>
#include <arpa/nameser.h>
#include "ns.h"
#include "pathnames.h"

#ifdef STATSFILE
char *statsfile = STATSFILE;
#else
char *statsfile = _PATH_STATS;
#endif /* STATSFILE */

extern	time_t	boottime, resettime;
extern	int	needStatsDump;

/*
 * General statistics gathered
 */
/* The position in this table must agree with the defines in ns.h */
struct stats stats[S_NSTATS] = {
	{ 0, "input packets" },
	{ 0, "output packets" },
	{ 0, "queries" },
	{ 0, "iqueries" },
	{ 0, "duplicate queries" },
	{ 0, "responses" },
	{ 0, "duplicate responses" },
	{ 0, "OK answers" },
	{ 0, "FAIL answers" },
	{ 0, "FORMERR answers" },
	{ 0, "system queries" },
	{ 0, "prime cache calls" },
	{ 0, "check_ns calls" },
	{ 0, "bad responses dropped" },
	{ 0, "martian responses" },
};

/*
 *  Statistics for queries (by type)
 */
unsigned long typestats[T_ANY+1];
char *typenames[T_ANY+1] = {
	/* 5 types per line */
	"Unknown", "A", "NS", "invalid(MD)", "invalid(MF)",
	"CNAME", "SOA", "MB", "MG", "MR",
	"NULL", "WKS", "PTR", "HINFO", "MINFO",
	"MX", "TXT", 0, 0, 0,
	/* 20 per line */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 100 */
	"UINFO", "UID", "GID", "UNSPEC", 0, 0, 0, 0, 0, 0,
	/* 110 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 120 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 200 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 240 */
	0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
	/* 250 */
	0, 0, "AXFR", "MAILB", "MAILA", "ANY" 
};

ns_stats()
{
	time_t timenow;
	register FILE *f;
	register int i;

	if ((f = fopen(statsfile,"a")) == 0)
	{
#ifdef DEBUG
		if (debug)
			fprintf(ddt,"can't open stat file, \"%s\"\n",statsfile);
#endif
		syslog(LOG_ERR, "cannot open stat file, \"%s\"\n",statsfile);
		return;
	}

	time(&timenow);
	fprintf(f, "###  %s", ctime(&timenow));
	fprintf(f, "%d\ttime since boot (secs)\n", timenow - boottime);
	fprintf(f, "%d\ttime since reset (secs)\n", timenow - resettime);

	/* general statistics */
	for (i = 0; i < S_NSTATS; i++)
		fprintf(f,"%lu\t%s\n", stats[i].cnt, stats[i].description);

	/* query type statistics */
	fprintf(f, "%d\tUnknown query types\n", typestats[0]);
	for(i=1; i < T_ANY+1; i++)
		if (typestats[i])
			if (typenames[i])
				fprintf(f, "%lu\t%s queries\n", typestats[i],
					typenames[i]);
			else
				fprintf(f, "%lu\ttype %d queries\n",
					typestats[i], i);
	(void) fclose(f);
}
#endif /* STATS */
