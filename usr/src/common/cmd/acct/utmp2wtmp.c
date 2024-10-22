/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)acct:common/cmd/acct/utmp2wtmp.c	1.2.5.3"
#ident "$Header: $"
/*
 *	create entries for users who are still logged on when accounting
 *	is being run. Look at utmp, and update the time stamp. New info
 *	goes to wtmp. Call by runacct. 
 */

#include <stdio.h>
#include <sys/types.h>
#include <utmp.h>

long time();

main(argc, argv)
int	argc;
char	**argv;
{
	struct utmp *getutent(), *utmp;
	FILE *fp;

	if ((fp = fopen(WTMP_FILE, "a+")) == NULL) {
		fprintf(stderr, "%s: cannot open %s for writing\n",
			argv[0], WTMP_FILE);
		exit(2);
	}
	while ((utmp=getutent()) != NULL) {
		if (utmp->ut_type == USER_PROCESS) {
			time( &utmp->ut_time );
			fwrite( utmp, sizeof(*utmp), 1, fp);
		}
	}
	fclose(fp);
}
