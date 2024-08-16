extern char *gettxt();
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)cs:cs/log.c	1.3.2.6"
#ident	"$Header: $"

# include <stdio.h>
# include <unistd.h>
# include <sys/types.h>
# include <sys/stat.h>
# include <dial.h>
# include <time.h>
# include <string.h>
# include <pfmt.h>
# include <locale.h>
# include <errno.h>
# include <cs.h>
# include "extern.h"

static	FILE	*Lfp;	/* log file */
static	FILE	*Dfp;	/* debug file */


/*
 * openlog - open log file, sets global file pointer Lfp
 */


void
openlog()
{
	FILE *fp;		/* scratch file pointer for problems */

	Lfp = fopen(LOGFILE, "a+");
	if (Lfp == NULL) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt(fp, MM_ERROR, ":99:could not open log file: %s\n",
			     LOGFILE);
		}
		exit(1);
	}
	chmod(LOGFILE, 0600);

/*
 * lock logfile to indicate presence
 */

	if (lockf(fileno(Lfp), F_LOCK, 0) < 0) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt(fp, MM_ERROR, ":100:could not lock log file: %s\n",
			     LOGFILE);
		}
		exit(1);
	}
}


/*
 * logmsg - put a message into the log file
 *
 *	args:	msg - message to be logged
 */


void
logmsg(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	char buf[MSGSIZE];	/* scratch buffer */
	struct tm *tms;

	(void) time(&clock);
	tms = localtime(&clock);
	(void) strftime(buf, MSGSIZE, "%x %X", tms);
	(void) fprintf(Lfp, "%s; %5ld; %s\n", buf, getpid(),msg);
	(void) fflush(Lfp);
}


/*
 * opendebug - open debugging file, sets global file pointer Dfp
 */


void
opendebug()
{
	FILE *fp;	/* scratch file pointer for problems */

	Dfp = fopen(DBGFILE, "a+");
	if (Dfp == NULL) {
		fp = fopen("/dev/console", "w");
		if (fp) {
			pfmt(fp, MM_ERROR, ":101:could not open debug file: %s\n",
			     DBGFILE);
		}
		exit(1);
	}
	chmod(DBGFILE, 0600);
	setbuf(Dfp,NULL);
}


/*
 * debug - put a message into debug file
 *
 *	args:	msg - message to be output
 */

void
debug(msg)
char *msg;
{
	char *timestamp;	/* current time in readable form */
	long clock;		/* current time in seconds */
	char buf[MSGSIZE];	/* scratch buffer */
	struct tm *tms;

	(void) time(&clock);
	tms = localtime(&clock);
	(void) strftime(buf, MSGSIZE, "%x %X", tms);
	(void) fprintf(Dfp, "%s; %5ld; %s\n", buf, getpid(),msg);
	(void) fflush(Dfp);
}

/*
 * TEMP FIX:  old debugging did not expect timestamps, etc with
 *            each call to debug, and sometimes only one character
 *	      at a time is printed; this is tuff to read using
 *            the debug above, so uux, uucp, etc use uudebug.
 *
 * uudebug - put a message into debug file WITHOUT ANY GARBAGE
 *
 *	args:	msg - message to be output
 */

void
uudebug(msg)
char *msg;
{
	(void) fprintf(Dfp, msg);
}

/*
 * consolemsg - when all else fails, write a message to the console
 */

void
consolemsg(msg)
char	*msg;
{
	FILE *fp;

	fp = fopen("/dev/console", "w");
	if (fp) {
		pfmt(fp, MM_ERROR, msg);
		fclose(fp);
	} 
	return;
}


/*
 * check_log_size()
 *	- check the size of the cs.log file
 *      - if necessary, close, move to cs.log_old and reopen
 */

void
check_log_size(size)
off_t	size;
{
	struct	stat	stat_buf;
	char	errmsgbuf[200];		/* error message buffer */

	/* get and compare the size of the log file */
	if(stat(LOGFILE, &stat_buf)) {
		sprintf(errmsgbuf,
		    ":102:stat(%s) failed, errno=%d\n",
		    LOGFILE, errno);
		consolemsg(errmsgbuf);
		return;
	}
	if(stat_buf.st_size < size) { /* AOK */
		return;
	}

	/* remove old LOGFILE_OLD, if present */
	if(unlink(LOGFILE_OLD) && (errno != ENOENT)) {
		sprintf(errmsgbuf,
		    ":103:unlink(%s) failed, errno=%d\n",
		    LOGFILE_OLD, errno);
		consolemsg(errmsgbuf);
		return;
	}

	/* create link to LOGFILE */
	if(link(LOGFILE, LOGFILE_OLD)) {
		sprintf(errmsgbuf,
		    ":104:link(%s, %s) failed, errno=%d\n",
		    LOGFILE, LOGFILE_OLD, errno);
		consolemsg(errmsgbuf);
		return;
	}

	/* remove current LOGFILE (link still remains) */
	if(unlink(LOGFILE)) {
		sprintf(errmsgbuf,
		    ":103:unlink(%s) failed, errno=%d\n",
		    LOGFILE, errno);
		consolemsg(errmsgbuf);
		return;
	}

	/* close current LOGFILE */
	fclose(Lfp);

	/* open new LOGFILE */
	if((Lfp = fopen(LOGFILE, "a+")) == NULL) {
		sprintf(errmsgbuf,
		    ":105: could not open new %s file: %s\n",
		    LOGFILE, strerror(errno));
		consolemsg(errmsgbuf);

		/* there may be attempts to fprint to Lfp; the
		 * following line is so that it won't dump core,
		 * but the output is lost from now on.
		 */
		Lfp = fopen("/dev/null","a+");
	}
	chmod(LOGFILE, 0600);
	return;
}


/*
 * check_debug_size()
 *	- check the size of the cs.debug file
 *      - if necessary, close, move to cs.debug_old and reopen
 */

void
check_debug_size(size)
off_t	size;
{
	struct	stat	stat_buf;
	char	errmsgbuf[200];		/* error message buffer */

	/* get and compare the size of the debug file */
	if(stat(DBGFILE, &stat_buf)) {
		sprintf(errmsgbuf,
		    gettxt("uxcs:325", "check_debug_size: stat(%s) failed, errno=%d\n"),
		    DBGFILE, errno);
		logmsg(errmsgbuf);
		return;
	}
	if(stat_buf.st_size < size) { /* AOK */
		return;
	}

	/* remove old DBGFILE_OLD, if present */
	if(unlink(DBGFILE_OLD) && (errno != ENOENT)) {
		sprintf(errmsgbuf,
		    gettxt("uxcs:326", "check_debug_size: unlink(%s) failed, errno=%d\n"),
		    DBGFILE_OLD, errno);
		logmsg(errmsgbuf);
		return;
	}

	/* create link to DBGFILE */
	if(link(DBGFILE, DBGFILE_OLD)) {
		sprintf(errmsgbuf,
		    gettxt("uxcs:327", "check_debug_size: link(%s %s) failed, errno=%d\n"),
		    DBGFILE, DBGFILE_OLD, errno);
		logmsg(errmsgbuf);
		return;
	}

	/* remove current DBGFILE (link still remains) */
	if(unlink(DBGFILE)) {
		sprintf(errmsgbuf,
		    gettxt("uxcs:326", "check_debug_size: unlink(%s) failed, errno=%d\n"),
		    DBGFILE, errno);
		logmsg(errmsgbuf);
		return;
	}

	/* close current DBGFILE */
	fclose(Dfp);

	/* open new DBGFILE */
	if((Dfp = fopen(DBGFILE, "a+")) == NULL) {
		sprintf(errmsgbuf,
		    gettxt("uxcs:328", "check_debug_size: cannot fopen %s, errno=%d\n"),
		    DBGFILE, errno);
		logmsg(errmsgbuf);

		/* there may be attempts to fprint to Dfp; the
		 * following line is so that it won't dump core,
		 * but the output is lost from now on.
		 */
		Dfp = fopen("/dev/null","a+");
	}
	chmod(DBGFILE, 0600);
	setbuf(Dfp,NULL);
	return;
}
