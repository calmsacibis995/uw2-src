/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)bkrs:common/cmd/bkrs/rcmds.d/del_cron.c	1.1"
#ident	"$Header: $"

#include <stdio.h>
#include <string.h>

#define	TRUE	1
#define	FALSE	0
#define	READ	0
#define	WRITE	1
#define	STRLEN	256

main(argc, argv)
int	argc;
char	*argv[];
{
	int	p[2];
	FILE	*cronfp, *delfp, *fopen();
	int	found;
	int	pid, w, status;
	char	newcron[STRLEN], tobedel[STRLEN], job[STRLEN];

	if (pipe(p) < 0) {
		printf("Can't open pipe.\n");
		exit(1);
	}
	if (fork() == 0) {
		close(p[READ]);
		close(1); dup(p[WRITE]);
		close(p[WRITE]);
		execlp("crontab", "crontab", "-l", (char *) 0);
		exit(1);
	}
	close(p[WRITE]);
	close(0); dup(p[READ]);
	close(p[READ]);
	sprintf(newcron, "/tmp/tmpcron.%d", getpid());
	cronfp = fopen(newcron, "w");
	delfp = fopen(argv[1], "r");
	do {
		found = FALSE;
		if (fgets(tobedel, STRLEN, delfp) != NULL)
			while (fgets(job, STRLEN, stdin) != NULL) {
				if (strncmp(tobedel, job, strlen(tobedel) - 1) == 0) {
					found = TRUE;
					break;
				}
				fputs(job, cronfp);
			}
		else
			while (fgets(job, STRLEN, stdin) != NULL)
				fputs(job, cronfp);
	} while (found == TRUE);
	fclose(cronfp);
	fclose(delfp);
	if ((pid = fork()) == 0) {
		execlp("crontab", "crontab", newcron, (char *) 0);
		exit(1);
	}
	while ((w = wait(&status)) != pid && w != -1)
		;
	unlink(newcron);
	unlink(argv[1]);
}
