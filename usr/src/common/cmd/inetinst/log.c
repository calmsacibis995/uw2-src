/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)inetinst:log.c	1.7"

/*
 *  Routines to manage the logging of activities in inetinst.
 *  Basic philosophy is that a call to a service should not mix
 *  its log info with other service calls on a single server.
 *  Each operation creates its own logfile, and at completion
 *  appends it to a system-wide logfile and removes the temp
 *  logfile.
 */

#include "inetinst.h"
#include <string.h>

static int  logfd=0;		/* File descriptor for logging */
static char *logfile;		/* Name of logfile */
static char *service;		/* Name of service */
static char logbuf[IBUF_SIZE];	/* Buffer for use in this routine */

/*
 *  Initialize the logfile for this session
 *	INPUT:	Name of service being logged
 *	OUTPUT:	None
 *	ACTION:	Opens new logfile for writing, leaves descriptor for use
 *		by other routines in this C file.
 */
void
log_init(char *logname)
{
	time_t	my_time;
	char	*datestr;

	service = strdup(logname);
	time(&my_time);

	memset(logbuf, '\0', IBUF_SIZE);
	/*
	 *  Generate a temp name for this logfile.
	 */
	logfile = strdup(tempnam(IADM_DIR, service));

	/*
	 *  Open logfile and send initialization message
	 */
	if ((logfd = open(logfile, O_WRONLY|O_CREAT|O_TRUNC, 0644)) < 0) {
		system("echo 'couldnt open logfile' >> /tmp/inetinst.msg");
		exit(IERR_PERM);
	}

	datestr = strdup(ctime(&my_time));
	*(datestr + strlen(datestr) - 1) = '\0';
	sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_BEGIN, M_LOG_BEGIN), datestr, service);
	write(logfd, logbuf, strlen(logbuf));
}

/*
 *  Log an event to the logfile
 *	INPUT:	Character string to be logged
 *	OUTPUT: Formatted string in logfile
 *	ACTION:	Prints string into logfile
 */
void
log(char *event)
{
	char	*ptr ;
	/*
	 *  If we try to log something before init_log, just return.
	 */
	if (!logfd)
	    return;

	for (ptr=event; *ptr != '\0'; ptr++) ;
	ptr-- ;

	if ( *ptr == '\n' )
	    sprintf(logbuf, "* %s", event);
	else
	    sprintf(logbuf, "* %s\n", event);

	write(logfd, logbuf, strlen(logbuf));
	if (vflag)
		fprintf(stderr, "%s", logbuf);
	fsync(logfd);
}

/*
 *  Close the logfile for this session
 *	INPUT:	Name of service being logged
 *	OUTPUT: None
 *	ACTION:	Closes logfile and appends it to system-wide logfile.
 */
void
log_close()
{
	time_t	my_time;
	char	*datestr;

	/*
	 *  First, we write one last entry, then we close the log file.
	 */
	time(&my_time);
	datestr = strdup(ctime(&my_time));
	*(datestr + strlen(datestr) - 1) = '\0';

	memset(logbuf, '\0', IBUF_SIZE);
	sprintf(logbuf, catgets(mCat, MSG_SET, C_LOG_END, M_LOG_END), datestr, service);
	write(logfd, logbuf, strlen(logbuf));
	close(logfd);

	/*
	 *  Now we append the contents of this logfile to the
	 *  system-wide logfile, and remove the temp logfile.
	 */
	log_send();
/* TBD
	lock(system-wide lockfile)
*/

	memset(logbuf, '\0', IBUF_SIZE);
	sprintf(logbuf, "/bin/cat %s >> %s 2>/dev/null", logfile, ILOG_FILE);
	system(logbuf);
/* TBD
	unlock(system-wide lockfile)
*/
	unlink(logfile);
	
}

/*
 *  Send a completed logfile to an email address
 *	INPUT	none
 *	OUTPUT	none
 *	ACTIONS	gets 'mail' option.  Mails logfile to address specified.
 */
void
log_send()
{
	char cmdbuf[IBUF_SIZE];
	char logbuf[IBUF_SIZE];

	/*
	 *  Don't send mail unless an address is specified.
	 */
	if (!strcmp("none", get_option(IOPT_MAIL)))
		return;

	sprintf(cmdbuf, "%s -s\"%s\" %s < %s 2>/dev/null",
		MAILX, service, get_option(IOPT_MAIL), logfile);

	system(cmdbuf);
	
	return;
}
