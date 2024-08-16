/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/xmail.h	1.2.7.2"
#ident "@(#)xmail.h	1.5 'attmail mail(1) command'"
#ifndef XMAIL_H
#define XMAIL_H

extern char *MAILROOT;	/* root of mail system */
extern char *UPASROOT;	/* root of upas system */
extern char *SMTPQROOT; /* root of smtpq directory */
extern char *SYSALIAS;	/* file system alias files are listed in */
extern char *USERALIAS;	/* file system alias files are listed in */
extern int MBOXMODE;	/* default mailbox protection mode */

/* format of REMOTE FROM lines */
extern char *REMFROMRE;
extern int REMSENDERMATCH;
extern int REMDATEMATCH;
extern int REMSYSMATCH;

/* format of mailbox FROM lines */
#define IS_HEADER(p) ((p)[0]=='F'&&(p)[1]=='r'&&(p)[2]=='o'&&(p)[3]=='m'&&(p)[4]==' ')
extern char *FROMRE;
extern int SENDERMATCH;
extern int DATEMATCH;

extern void print_header proto((FILE *fp, char *sender, char *date));
extern void print_remote_header proto((FILE *fp, char *sender, char *date, char *system));


#ifndef NULL
#define NULL 0
#endif

#if defined(SYS5) || defined(SVR3) || defined(SVR4) || defined(SUN41)

#define SIGRETURN void
typedef void (*SIG_TYP) proto((int));

#else

#ifdef BSD

#define SIGRETURN int
typedef int (*SIG_TYP) proto((int));

#else

#define SIGRETURN int

#endif

#endif

#if defined(SYS5) || defined(SVR3) || defined(SVR4) || defined(SUN41)

#include <dirent.h>
typedef struct dirent Direct;

#else

#ifdef BSD

#include <sys/dir.h>
#else
#include <ndir.h>
#endif
typedef struct direct Direct;

#endif

#ifdef SVR3
/* --------------------------------------------------- */
/* | Copyright (c) 1986 MIPS Computer Systems, Inc.  | */
/* | All Rights Reserved.                            | */
/* --------------------------------------------------- */
/* $Header: syslog.h,v 1.6 89/01/19 13:47:23 purves Exp $ */
/*
 * Copyright (c) 1982, 1986, 1988 Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms are permitted
 * provided that the above copyright notice and this paragraph are
 * duplicated in all such forms and that any documentation,
 * advertising materials, and other materials related to such
 * distribution and use acknowledge that the software was developed
 * by the University of California, Berkeley.  The name of the
 * University may not be used to endorse or promote products derived
 * from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED ``AS IS'' AND WITHOUT ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, WITHOUT LIMITATION, THE IMPLIED
 * WARRANTIES OF MERCHANTIBILITY AND FITNESS FOR A PARTICULAR PURPOSE.
 *
 *	@(#)syslog.h	7.10 (Berkeley) 6/27/88
 */

#ifndef	_BSD_SYSLOG_
#define	_BSD_SYSLOG_	1

/*
 *  Facility codes
 */

#define LOG_KERN	(0<<3)	/* kernel messages */
#define LOG_USER	(1<<3)	/* random user-level messages */
#define LOG_MAIL	(2<<3)	/* mail system */
#define LOG_DAEMON	(3<<3)	/* system daemons */
#define LOG_AUTH	(4<<3)	/* security/authorization messages */
#define LOG_SYSLOG	(5<<3)	/* messages generated internally by syslogd */
#define LOG_LPR		(6<<3)	/* line printer subsystem */
#define LOG_NEWS	(7<<3)	/* network news subsystem */
#define LOG_UUCP	(8<<3)	/* UUCP subsystem */
	/* other codes through 15 reserved for system use */
#define LOG_LOCAL0	(16<<3)	/* reserved for local use */
#define LOG_LOCAL1	(17<<3)	/* reserved for local use */
#define LOG_LOCAL2	(18<<3)	/* reserved for local use */
#define LOG_LOCAL3	(19<<3)	/* reserved for local use */
#define LOG_LOCAL4	(20<<3)	/* reserved for local use */
#define LOG_LOCAL5	(21<<3)	/* reserved for local use */
#define LOG_LOCAL6	(22<<3)	/* reserved for local use */
#define LOG_LOCAL7	(23<<3)	/* reserved for local use */

#define LOG_NFACILITIES	24	/* maximum number of facilities */
#define LOG_FACMASK	0x03f8	/* mask to extract facility part */

#define LOG_FAC(p)	(((p) & LOG_FACMASK) >> 3)	/* facility of pri */

/*
 *  Priorities (these are ordered)
 */

#define LOG_EMERG	0	/* system is unusable */
#define LOG_ALERT	1	/* action must be taken immediately */
#define LOG_CRIT	2	/* critical conditions */
#define LOG_ERR		3	/* error conditions */
#define LOG_WARNING	4	/* warning conditions */
#define LOG_NOTICE	5	/* normal but signification condition */
#define LOG_INFO	6	/* informational */
#define LOG_DEBUG	7	/* debug-level messages */

#define LOG_PRIMASK	0x0007	/* mask to extract priority part (internal) */
#define LOG_PRI(p)	((p) & LOG_PRIMASK)	/* extract priority */

#define	LOG_MAKEPRI(fac, pri)	(((fac) << 3) | (pri))

#ifdef KERNEL
#define LOG_PRINTF	-1	/* pseudo-priority to indicate use of printf */
#endif

/*
 * arguments to setlogmask.
 */
#define	LOG_MASK(pri)	(1 << (pri))		/* mask for one priority */
#define	LOG_UPTO(pri)	((1 << ((pri)+1)) - 1)	/* all priorities through pri */

/*
 *  Option flags for openlog.
 *
 *	LOG_ODELAY no longer does anything; LOG_NDELAY is the
 *	inverse of what it used to be.
 */
#define	LOG_PID		0x01	/* log the pid with each message */
#define	LOG_CONS	0x02	/* log on the console if errors in sending */
#define	LOG_ODELAY	0x04	/* delay open until first syslog() (default) */
#define LOG_NDELAY	0x08	/* don't delay open */
#define LOG_NOWAIT	0x10	/* if forking to log on console, don't wait() */

#endif	_BSD_SYSLOG_
#endif
#ifdef SVR4
# include <sys/syslog.h>
#endif

#ifndef DEFAULT_LOG_LEVEL
# define DEFAULT_LOG_LEVEL	LOG_INFO
#endif

/*
 *
 * Syslogs used to keep track of users/intruders.  These are the assignments
 * of the LOCAL logs.
 *
 */

#define	LOG_INETD	LOG_LOCAL0
#define LOG_FTPD	LOG_LOCAL1
#define LOG_TELNETD	LOG_LOCAL2
#define LOG_SMTPD	LOG_LOCAL3
#define LOG_PROXY	LOG_LOCAL4
#define LOG_SMTP	LOG_LOCAL5
#define LOG_SMTPSCHED	LOG_LOCAL6



extern void Syslog proto((int level, const char *fmt, ...));
extern void Openlog proto((const char *label, int flags, int log));
extern void setloglevel proto((char *arg));
extern int setlogmask proto((int));

extern char *thedate proto((void));
#endif /* XMAIL_H */
