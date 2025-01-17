/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)bnu:unknown.c	1.8.5.3"
#ident "$Header: unknown.c 1.1 91/02/28 $"
/*
 *	logs attempts by unknown remote machines to run uucico in FOREIGN
 *	("/var/uucp/.Admin/Foreign").  if anything goes wrong,
 *	sends mail to login MAILTO ("uucp").  the executable should be 
 *	placed in /usr/lib/uucp/remote.unknown, and should run setuid-uucp.
 */

#include	"uucp.h"

#define	FOREIGN	"/var/uucp/.Admin/Foreign"
#define	MAILTO	"uucp"
#define	LOGLEN	256

void fall_on_sword();

main(argc, argv)
int	argc;
char	*argv[];
{
	char		buf[LOGLEN], *ctoday, *logname;
	FILE		*fp;
	time_t		today;
	extern char	*ctime();
	extern FILE	*fopen();

	if ( argc != 2 ) {
		(void) fprintf(stderr, "USAGE: %s remotename\n", argv[0]);
		exit(101);
	}

	if ( time(&today) != -1 ) {
		ctoday = ctime(&today);
		*(ctoday + strlen(ctoday) - 1) = '\0';	/* no ending \n */
	} else
		ctoday = "NO DATE";

	logname = cuserid((char *) NULL);
	sprintf(buf, "%s: call from system %s login %s\n",
		ctoday, argv[1], (logname == NULL ? "<unknown>" : logname));

	errno = 0;
	(void) mkdirs(ADMIN, DIRMASK);		/* ensure intermediate dirs */
	if ( (fp = fopen(FOREIGN, "a+")) == (FILE *)NULL )
		fall_on_sword("cannot open", buf);
	if ( fputs(buf, fp) == EOF )
		fall_on_sword("cannot write", buf);
	if ( fclose(fp) != 0 )
		fall_on_sword("cannot close", buf);

	exit(0);
}

/* don't return from here */
void
fall_on_sword(errmsg, logmsg)
char	*errmsg, *logmsg;
{
	char		ebuf[BUFSIZ];
	int		fds[2];
	extern int	sys_nerr;
	extern char	*sys_errlist[];

	sprintf(ebuf, "%s %s:\t%s (%d)\nlog msg:\t%s",
		errmsg, FOREIGN,
		( errno < sys_nerr ? sys_errlist[errno] : "Unknown error " ),
		errno, logmsg);

	/* reset to real uid. get a pipe. put error message on	*/
	/* "write end" of pipe, close it. dup "read end" to	*/
	/* stdin and then execl mail (which will read the error	*/
	/* message we just wrote).				*/

	if ( setuid(getuid()) == -1 || pipe(fds) != 0 
	|| write(fds[1], ebuf, strlen(ebuf)) != strlen(ebuf)
	|| close(fds[1]) != 0 )
		exit(errno);

	if ( fds[0] != 0 ) {
		close(0);
		if ( dup(fds[0]) != 0 )
			exit(errno);
	}

	execl("/usr/bin/mail", "mail", MAILTO, (char *) 0);
	exit(errno);	/* shouldn't get here */
}
