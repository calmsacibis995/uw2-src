#ident	"@(#)ucb:common/ucbcmd/sendmail/ucblib/rmail.c	1.4"
#ident	"$Header: $"
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


/*******************************************************************

		PROPRIETARY NOTICE (Combined)

This source code is unpublished proprietary information
constituting, or derived under license from AT&T's UNIX(r) System V.
In addition, portions of such source code were derived from Berkeley
4.3 BSD under license from the Regents of the University of
California.



		Copyright Notice 

Notice of copyright on this source code product does not indicate 
publication.

	(c) 1986,1987,1988,1989  Sun Microsystems, Inc
	(c) 1983,1984,1985,1986,1987,1988,1989  AT&T.
	          All rights reserved.
********************************************************************/ 

/*
**  RMAIL -- UUCP mail server.
**
**	This program reads the >From ... remote from ... lines that
**	UUCP is so fond of and turns them into something reasonable.
**	It calls sendmail giving it a -f option built from these
**	lines.
*/

#include <stdio.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/stat.h>

typedef char	bool;
#define TRUE	1
#define FALSE	0

extern FILE	*popen();
#define index strchr
#define rindex strrchr
extern char	*index();
extern char	*rindex();

bool	Debug;
char	*Domain = "UUCP";	/* Default "Domain" */

# define MAILER	"/usr/ucblib/sendmail"

main(argc, argv)
	char **argv;
{
	FILE *out;	/* output to sendmail */
	char lbuf[1024];	/* one line of the message */
	char from[512];		/* accumulated path of sender */
	char ufrom[512];	/* user on remote system */
	char sys[512];		/* a system in path */
	char fsys[512];		/* first system in path */
	char junk[1024];	/* scratchpad */
	char *args[100];	/* arguments to mailer command */
	register char *cp;
	register char *uf;	/* ptr into ufrom */
	int i;
	long position;
	struct stat sbuf;
	

# ifdef DEBUG
	if (argc > 1 && strcmp(argv[1], "-T") == 0)
	{
		Debug = TRUE;
		argc--;
		argv++;
	}
# endif DEBUG

	while (argc > 1 && *argv[1] == '-') {
		switch (argv[1][1]) {
			case 'D': 
				Domain = &argv[2];
				argc -= 2;
				argv += 2;		
				break;
			case 't':		/* drop the flag */
				argc -= 1;
				argv += 1;
				break;
			default:
				fprintf(stderr, "Usage: rmail [-D domain] [-t] user ...\n");
				exit(EX_USAGE);
		}
	}

	if (argc < 2)
	{
		fprintf(stderr, "Usage: rmail [-D domain] [-t] user ...\n");
		exit(EX_USAGE);
	}

	(void) strcpy(from, "");
	(void) strcpy(fsys, "");
	(void) strcpy(ufrom, "/dev/null");
	uf = NULL;

	for (position=0;;position=ftell(stdin))
	{
		(void) fgets(lbuf, sizeof lbuf, stdin);
		if (strncmp(lbuf, "From ", 5) != 0 && strncmp(lbuf, ">From ", 6) != 0)
			break;
		(void) sscanf(lbuf, "%s %s", junk, ufrom);
		cp = lbuf;
		uf = ufrom;
		for (;;)
		{
			cp = index(cp+1, 'r');
			if (cp == NULL)
			{
				register char *p = rindex(uf, '!');

				if (p != NULL)
				{
					*p = '\0';
					(void) strcpy(sys, uf);
					uf = p + 1;
					break;
				}
				(void) strcpy(sys, "");
				break;	/* no "remote from" found */
			}
#ifdef DEBUG
			if (Debug)
				printf("cp='%s'\n", cp);
#endif
			if (strncmp(cp, "remote from ", 12)==0)
				break;
		}
		if (cp != NULL)
			(void) sscanf(cp, "remote from %s", sys);
		if (fsys[0] == '\0')
			(void) strcpy(fsys, sys);
		if (sys[0])
		{
			(void) strcat(from, sys);
			(void) strcat(from, "!");
		}
#ifdef DEBUG
		if (Debug)
			printf("ufrom='%s', sys='%s', from now '%s'\n", uf, sys, from);
#endif
	}
	if (uf)
		(void) strcat(from, uf);
	(void) fstat(0,&sbuf);
	(void) lseek(0,position,0);

	  /*
	   * Now we rebuild the argument list and chain to sendmail.
	   * Note that the above lseek might fail on irregular files,
	   * but we check for that case below.
	   */
	i = 0;
	args[i++] = MAILER;
	args[i++] = "-oem";	/* mail back error messages */
	args[i++] = "-om";
	args[i++] = "-m";	/* pass multiple messages to same target */
	if (fsys[0] != '\0') {
		static char junk2[512];

		if (index(fsys,'.') == NULL) {
			(void) strcat(fsys, ".");
			(void) strcat(fsys, Domain);
		}
		(void) sprintf(junk2, "-oMs%s", fsys);
		args[i++] = junk2;
	}
	(void) sprintf(junk, "-oMr%s", Domain);
	args[i++] = junk;
	if (from[0] != '\0' && uf != NULL) {
		static char junk2[512];

		(void) sprintf(junk2, "-f%s", from);
		args[i++] = junk2;
	}

	for (;*++argv != NULL;i++)
	{
		args[i] = *argv;
	}
	args[i] = NULL;
#ifdef DEBUG
	if (Debug)
	{
		printf("Command:");
		for (i=0;args[i];i++)
			printf(" %s",args[i]);
		printf("\n");
	}
#endif
	if ( (sbuf.st_mode & S_IFMT) != S_IFREG)
	{
	    /*
	     * If we were not called with standard input on a regular
	     * file, then we have to fork another process to send the
	     * first line down the pipe.
	     */
	    int pipefd[2];
# ifdef DEBUG
	    if (Debug) printf("Not a regular file!\n");
# endif DEBUG
	    if (pipe(pipefd) < 0) exit(EX_OSERR);
	    if (fork()==0)
	    {
	      /*
	       * Child: send the message down the pipe.
	       */
		FILE *out;
	       
		out = fdopen(pipefd[1],"w");
		close(pipefd[0]);
		fputs(lbuf, out);
		while (fgets(lbuf, sizeof lbuf, stdin))
			fputs(lbuf, out);
		fclose(out);
		exit(EX_OK);
	    }
	      /*
	       * Parent: call sendmail with pipe as standard input
	       */
	    close(pipefd[1]);
	    dup2(pipefd[0],0);
	}
	execv(MAILER,args);
	printf("Exec of %s failed!\n", MAILER);
	exit(EX_OSERR);
	/* NOTREACHED */
}
