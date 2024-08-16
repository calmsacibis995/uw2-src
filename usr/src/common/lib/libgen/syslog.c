/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:syslog.c	1.2"

/* from "@(#)syslog.c 1.18 88/02/08 SMI"; from UCB 5.9 5/7/86 */

/*
 * SYSLOG -- print message on log file
 *
 * This routine looks a lot like printf, except that it
 * outputs to the log file instead of the standard output.
 * Also:
 *	adds a timestamp,
 *	prints the module name in front of the message,
 *	has some other formatting types (or will sometime),
 *	adds a newline on the end of the message.
 *
 * The output of this routine is intended to be read by /etc/syslogd.
 */

#ifdef __STDC__
	#pragma weak syslog = _syslog
	#pragma weak vsyslog = _vsyslog
	#pragma weak openlog = _openlog
	#pragma weak closelog = _closelog
	#pragma weak setlogmask = _setlogmask
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <stropts.h>
#ifdef __STDC__
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include <sys/strlog.h>
#include <sys/syslog.h>
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <errno.h>
#include <wait.h>

#define	MAXLINE	1024			/* max message size */

#define PRIMASK(p)	(1 << ((p) & LOG_PRIMASK))
#define PRIFAC(p)	(((p) & LOG_FACMASK) >> 3)
#define IMPORTANT 	LOG_ERR

#define logname "/dev/conslog"
#define ctty "/dev/syscon"

static struct __syslog {
	int	_LogFile;
	int	_LogStat;
	const char	*_LogTag;
	int	_LogMask;
	char	*_SyslogHost;
	int	_LogFacility;
} *__syslog;

#define	LogFile (__syslog->_LogFile)
#define	LogStat (__syslog->_LogStat)
#define	LogTag (__syslog->_LogTag)
#define	LogMask (__syslog->_LogMask)
#define	SyslogHost (__syslog->_SyslogHost)
#define	LogFacility (__syslog->_LogFacility)

static int
#ifdef __STDC__
alloc_syslog(void)
#else
alloc_syslog()
#endif
{
		if ((__syslog = (struct __syslog *)calloc(1, sizeof (struct __syslog))) == 0)
			return -1;
		LogFile = -1;		/* fd for log */
		LogStat	= 0;		/* status bits, set by openlog() */
		LogTag = "syslog";	/* string to tag the entry with */
		LogMask = 0xff;		/* mask of priorities to be logged */
		LogFacility = LOG_USER;	/* default facility code */
		return 0;
}

void
#ifdef __STDC__
vsyslog(int pri, const char *fmt, va_list ap)
#else
vsyslog(pri, fmt, ap)int pri; char *fmt; va_list ap;
#endif
{
	register char *b;
	register const char *f;
	register char *o;
	register int c;
	char buf[MAXLINE + 1], outline[MAXLINE + 1];
	long now;
	pid_t pid;
	struct log_ctl hdr;
	struct strbuf dat;
	struct strbuf ctl;
	sigset_t sigs;
	int olderrno = errno;

	if (__syslog == 0)
	{
		if (alloc_syslog() < 0)
			return;
	}
	/* see if we should just throw out this message */
	if (pri <= 0 || PRIFAC(pri) >= LOG_NFACILITIES || (PRIMASK(pri) & LogMask) == 0)
		return;
	if (LogFile < 0)
		openlog(LogTag, LogStat | LOG_NDELAY, 0);

	/* set default facility if none specified */
	if ((pri & LOG_FACMASK) == 0)
		pri |= LogFacility;

	/* build the header */
	hdr.pri = pri;
	hdr.flags = SL_CONSOLE;
	hdr.level = 0;

	/* build the message */
	o = outline;
	(void) time(&now);
	(void) sprintf(o, "%.15s ", ctime(&now) + 4);
	o += strlen(o);
	if (LogTag) {
		(void) strcpy(o, LogTag);
		o += strlen(o);
	}
	if (LogStat & LOG_PID) {
		(void) sprintf(o, "[%d]", getpid());
		o += strlen(o);
	}
	if (LogTag) {
		(void) strcpy(o, ": ");
		o += 2;
	}

	b = buf;
	f = fmt;
	while ((c = *f++) != '\0' && c != '\n' && b < &buf[MAXLINE]) {
		char *errmsg;
		if (c != '%') {
			*b++ = c;
			continue;
		}
		if ((c = *f++) != 'm') {
			*b++ = '%';
			*b++ = c;
			continue;
		}
		if ((errmsg = strerror(olderrno)) == NULL)
			(void) sprintf(b, "error %d", olderrno);
		else
			(void) strcpy(b, errmsg);
		b += strlen(b);
	}
	*b++ = '\n';
	*b = '\0';
	(void) vsprintf(o, buf, ap);
	c = strlen(outline) + 1;	/* add one for NULL byte */
	if (c > MAXLINE)
		c = MAXLINE;

	/* set up the strbufs */
	ctl.maxlen = sizeof(struct log_ctl);
	ctl.len = sizeof(struct log_ctl);
	ctl.buf = (caddr_t)&hdr;
	dat.maxlen = sizeof(outline);
	dat.len = c;
	dat.buf = outline;

	/* output the message to the local logger */
	if (putmsg(LogFile, &ctl, &dat, 0) >= 0)
		return;
	if (!(LogStat & LOG_CONS))
		return;

	/* output the message to the console */
	pid = vfork();
	if (pid == -1)
		return;
	if (pid == 0) {
		int fd;

		(void) signal(SIGALRM, SIG_DFL);
		sigprocmask(SIG_BLOCK, NULL, &sigs);
		sigdelset(&sigs, SIGALRM);
		sigprocmask(SIG_SETMASK, &sigs, NULL);
		alarm(5);
		fd = open(ctty, O_WRONLY);
		if (fd >= 0) {
			alarm(0);
			(void) strcat(o, "\r");
			write(fd, o, c + 1);
			close(fd);
		}
		_exit(0);
	}
	if (!(LogStat & LOG_NOWAIT))
		waitpid(pid, (int *)0, 0);
}

void 
#ifdef __STDC__
syslog(int pri, const char *fmt, ...)
#else
/*VARARGS2*/
syslog(pri, fmt, va_alist)int pri; char *fmt; va_dcl
#endif
{
	va_list ap;

#ifdef __STDC__
	va_start(ap, fmt);
#else
	va_start(ap);
#endif
	vsyslog(pri, fmt, ap);
	va_end(ap);
}

/*
 * OPENLOG -- open system log
 */

void
#ifdef __STDC__
openlog(const char *ident, int logstat, int logfac)
#else
openlog(ident, logstat, logfac)
	char *ident;
	int logstat, logfac;
#endif
{
	if (__syslog == 0) {
		if (alloc_syslog() < 0)
			return;
	}

	if (ident != NULL)
		LogTag = ident;
	LogStat = logstat;
	if (logfac != 0)
		LogFacility = logfac & LOG_FACMASK;
	if (LogFile >= 0)
		return;
	if (LogStat & LOG_NDELAY) {
		LogFile = open(logname, O_WRONLY);
		fcntl(LogFile, F_SETFD, 1);
	}
}

/*
 * CLOSELOG -- close the system log
 */

void closelog()
{

	if (__syslog == 0)
		return;
	(void) close(LogFile);
	LogFile = -1;
}

/*
 * SETLOGMASK -- set the log mask level
 */
setlogmask(pmask)
	int pmask;
{
	int omask;

	omask = LogMask;
	if (pmask != 0)
		LogMask = pmask;
	return (omask);
}
