/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)ttymon:common/cmd/ttymon/ttymon.notp/tmlog.c	1.2"

/*
 * error/logging/cleanup functions for ttymon.
 */

#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>

#ifdef i386
#include <errno.h>
#endif

#include <string.h>
#include <sys/types.h>
#include <signal.h>

#ifdef i386
#include <sys/stat.h>
#endif

#include "ttymon.h"
#include "tmstruct.h"
#include "tmextern.h"

# ifdef DEBUG
extern FILE *Debugfp;
# endif

extern	time_t	time();
extern	char	*ctime();

void
openlog()
{
	int	fd, ret;
	char	logfile[BUFSIZ];
	extern	char	*Tag;

	/* the log file resides in /var/saf/pmtag/ */
	(void)strcpy(logfile, LOGDIR);
	(void)strcat(logfile, Tag);
	(void)strcat(logfile, "/");
	(void)strcat(logfile, LOGFILE);
	Logfp = NULL;
	(void)close(0);
	if ((fd = open(logfile, O_WRONLY|O_CREAT|O_APPEND,0444)) != -1) 
		if ((ret = fcntl(fd, F_DUPFD, 3)) == 3) {
			/* set close-on-exec flag */
			if (fcntl(ret, F_SETFD, 1) == 0) {
				Logfp = fdopen(ret, "a");
			}
		}
	if (!Logfp) {

		if ((fd = open(CONSOLE, O_WRONLY|O_NOCTTY)) != -1) 
			(void)write(fd,"ttymon cannot create log file.\n",31);

		exit(1);
	}
	log(" ");
	log("********** ttymon starting **********");

#ifdef	DEBUG
	(void)sprintf(Scratch,"fd(log)\t = %d",fileno(Logfp));
	log(Scratch);
#endif
}

/*
 * log(msg) - put a message into the log file
 *	    - if Logfp is NULL, write message to stderr or CONSOLE
 */

void
log(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	int	fd;
	extern	time_t	time();
	extern	char 	*ctime();

	if (Logfp) {
		(void) time(&clock);
		timestamp = ctime(&clock);
		*(strchr(timestamp, '\n')) = '\0';
		(void) fprintf(Logfp,"%s; %ld; %s\n", timestamp, getpid(), msg);
		(void) fflush(Logfp);
	}
	else if (isatty(2)) {
		(void) fprintf(stderr, "%s\n", msg);
		(void) fflush(stderr);
	}

	else if ((fd = open(CONSOLE, O_WRONLY|O_NOCTTY)) != -1) {
		(void) write(fd, msg, strlen(msg));
		(void) write(fd, "\n", 1);
		(void) close(fd);
	}
	else
		exit(1);
}

void
logexit(exitcode, msg)
int exitcode;
char *msg;
{
	if (msg) 
		log(msg); 
	log("********** ttymon exiting ***********");
	exit(exitcode);
}

# ifdef DEBUG

/*
 * opendebug - open debugging file, sets global file pointer Debugfp
 * 	arg:   getty - if TRUE, ttymon is in getty_mode and use a different
 *		       debug file
 */

void
opendebug(getty_mode)
int	getty_mode;
{
	int  fd, ret;
	char	debugfile[BUFSIZ];
	extern	char	*Tag;

	if (!getty_mode) {
		(void)strcpy(debugfile, LOGDIR);
		(void)strcat(debugfile, Tag);
		(void)strcat(debugfile, "/");
		(void)strcat(debugfile, DBGFILE);
		if ((Debugfp = fopen(debugfile, "a+")) == NULL)
			logexit(1,"open debug file failed");
	}
	else {
		if ((fd = open(EX_DBG, O_WRONLY|O_APPEND|O_CREAT)) < 0) {
			(void)sprintf(Scratch,"open %s failed, errno = %d", 
				EX_DBG, errno);
			logexit(1,Scratch);
		}
		if (fd >= 3) 
			ret = fd;
		else {
			if ((ret = fcntl(fd, F_DUPFD, 3)) < 0) {
				(void)sprintf(Scratch,
				"F_DUPFD fcntl failed, errno = %d", errno);
				logexit(1,Scratch);
			}
		}
		if ((Debugfp = fdopen(ret, "a")) == NULL) {
			(void)sprintf(Scratch,"fdopen failed, errno = %d", 
				errno);
			logexit(1,Scratch);
		}
		if (ret != fd)
			(void)close(fd);
	}
	/* set close-on-exec flag */
	if (fcntl(fileno(Debugfp), F_SETFD, 1) == -1) {
		(void)sprintf(Scratch, 
		"F_SETFD fcntl failed, errno = %d", errno);
		logexit(1,Scratch);
	}
}

/*
 * debug(msg) - put a message into debug file
 */

void
debug(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	char buf[BUFSIZ];	/* scratch buffer */

	(void) time(&clock);
	timestamp = ctime(&clock);
	*(strchr(timestamp, '\n')) = '\0';
	(void) sprintf(buf, "%s; %ld; %s\n", timestamp, getpid(), msg);
	(void) fprintf(Debugfp, buf);
	(void) fflush(Debugfp);
}
#endif
