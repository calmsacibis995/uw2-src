/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/acctwtmp.c	1.9.3.4"
#ident "$Header: $"
/*
 *	acctwtmp reason >> /var/adm/wtmp
 *	writes utmp.h record (with current time) to end of std. output
 *	acctwtmp `uname` >> /var/adm/wtmp as part of startup
 *	acctwtmp pm >> /var/adm/wtmp  (taken down for pm, for example)
 */
#include <stdio.h>
#include <sys/types.h>
#include <sys/fs/s5dir.h>
#include "acctdef.h"
#include <utmp.h>

struct	utmp	wb;
char	*strncpy();

main(argc, argv)
char **argv;
{
	if(argc < 2) 
		fprintf(stderr, "Usage: %s reason [ >> %s ]\n",
			argv[0], WTMP_FILE), exit(1);

	strncpy(wb.ut_line, argv[1], LSZ);
	wb.ut_line[11] = NULL;
	wb.ut_type = ACCOUNTING;
	time(&wb.ut_time);
	fseek(stdout, 0L, 2);
	fwrite(&wb, sizeof(wb), 1, stdout);
}
