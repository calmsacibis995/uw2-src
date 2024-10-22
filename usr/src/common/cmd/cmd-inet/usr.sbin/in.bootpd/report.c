/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-inet:common/cmd/cmd-inet/usr.sbin/in.bootpd/report.c	1.2"

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*
 * report() - calls syslog
 */

#ifdef	__STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif

#include <stdio.h>
#include <syslog.h>

#include "report.h"

#ifndef LOG_NDELAY
#define LOG_NDELAY	0
#endif
#ifndef LOG_DAEMON
#define LOG_DAEMON	0
#endif
#ifndef	LOG_BOOTP
#define LOG_BOOTP	LOG_DAEMON
#endif

extern int debug;
extern char *progname;

/*
 * This is initialized so you get stderr until you call
 *	report_init()
 */
static int stderr_only = 1;

void
report_init(nolog)
    int nolog;
{
    stderr_only = nolog;
#ifdef SYSLOG
    if (!stderr_only) {
	openlog(progname, LOG_PID | LOG_NDELAY, LOG_BOOTP);
    }
#endif
}

/*
 * This routine reports errors and such via stderr and syslog() if
 * appopriate.  It just helps avoid a lot of "#ifdef SYSLOG" constructs
 * from being scattered throughout the code.
 *
 * The syntax is identical to syslog(3), but %m is not considered special
 * for output to stderr (i.e. you'll see "%m" in the output. . .).  Also,
 * control strings should normally end with \n since newlines aren't
 * automatically generated for stderr output (whereas syslog strips out all
 * newlines and adds its own at the end).
 */

static char *levelnames[] = {
#ifdef LOG_SALERT
	"level(0): ",
	"alert(1): ",
	"alert(2): ",
	"emerg(3): ",
	"error(4): ",
	"crit(5):  ",
	"warn(6):  ",
	"note(7):  ",
	"info(8):  ",
	"debug(9): ",
	"level(?): "
#else
	"emerg(0): ",
	"alert(1): ",
	"crit(2):  ",
	"error(3): ",
	"warn(4):  ",
	"note(5):  ",
	"info(6):  ",
	"debug(7): ",
	"level(?): "
#endif
};
static int numlevels = sizeof(levelnames) / sizeof(levelnames[0]);


/*
 * Print a log message using syslog(3) and/or stderr.
 * The message passed in should not include a newline.
 */
#ifdef	__STDC__
void report(int priority, char *fmt, ...)
#else
/*VARARGS2*/
void report(priority, fmt, va_alist)
    int priority;
    char *fmt;
    va_dcl
#endif
{
    va_list ap;
    static char buf[128];

    if ( (priority < 0) || (priority >= numlevels) ) {
	priority = numlevels - 1;
    }

#ifdef	__STDC__
    va_start(ap, fmt);
#else
    va_start(ap);
#endif
    vsprintf(buf, fmt, ap);
    va_end(ap);

    /*
     * Print the message
     */
    if (stderr_only || (debug > 2)) {
	fprintf(stderr, "%s: %s %s\n",
		progname, levelnames[priority], buf);
    }
#ifdef SYSLOG
    if (!stderr_only)
	syslog((priority | LOG_BOOTP), "%s", buf);
#endif
}



/*
 * Return pointer to static string which gives full filesystem error message.
 */
char *get_errmsg()
{
    extern int errno;
    extern char *strerror();

    return strerror(errno);
}
