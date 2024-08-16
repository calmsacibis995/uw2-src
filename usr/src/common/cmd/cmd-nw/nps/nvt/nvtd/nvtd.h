/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)cmd-nw:common/cmd/cmd-nw/nps/nvt/nvtd/nvtd.h	1.6"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"$Id: nvtd.h,v 1.5 1994/05/11 21:49:32 mark Exp $"

/*
 * Copyright 1991, 1992 Unpublished Work of Novell, Inc.
 * All Rights Reserved.
 *
 * This work is an unpublished work and contains confidential,
 * proprietary and trade secret information of Novell, Inc. Access
 * to this work is restricted to (I) Novell employees who have a
 * need to know to perform tasks within the scope of their
 * assignments and (II) entities other than Novell who have
 * entered into appropriate agreements.
 *
 * No part of this work may be used, practiced, performed,
 * copied, distributed, revised, modified, translated, abridged,
 * condensed, expanded, collected, compiled, linked, recast,
 * transformed or adapted without the prior written consent
 * of Novell.  Any use or exploitation of this work without
 * authorization could subject the perpetrator to criminal and
 * civil liability.
 */

/* INCLUDES *******************/
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/time.h>
#include <errno.h>
#include <poll.h>
#include <termios.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdarg.h> 
#include <fcntl.h> 
#include <syslog.h>

#include <iaf.h>

#include <stropts.h>
#include <sac.h>
#include <utmpx.h>

#include "nwmsg.h"
#include "npsmsgtable.h"
#include "nwconfig.h"

/* GLOBALS **************/

extern	int  FdErr;   /* error file descriptor for ErrorOut()*/
extern	int  t_errno;
int		LogErrors;

char	*ptsname(int);			/* prototype not in any system header */

void	main(void);
void	err_doit(int , const char *, va_list);
pid_t	pty_fork(int *, char *, const struct termios *, const struct winsize *);
void	err_sys(const char *, ...);
void 	ErrorOut(const char *fmt, ...);

void	loop(int);

#define	BUFFSIZE	4096		/* read/write buffer size for nvtd */
#define ARG_COUNT	256	
#define CNULL		(char*)0	/* character pointer null replaces (char*)NULL*/
#define EOS			'\0'		/* End Of String */

/* MACROS **************/

#ifdef	DEBUG
#define	SYSLOG(x)			syslog(LOG_NOTICE, x)
#define	SYSLOG1(x, p1)		syslog(LOG_NOTICE, x, p1)
#define	SYSLOG2(x, p1, p2)	syslog(LOG_NOTICE, x, p1, p2)
#define	OPENLOG(x)			openlog(x, LOG_CONS, LOG_USER)
#else
#define	SYSLOG(x)
#define	SYSLOG1(x, p1)
#define	SYSLOG2(x, p1, p2)
#define	OPENLOG(x)
#endif /* DEBUG */
