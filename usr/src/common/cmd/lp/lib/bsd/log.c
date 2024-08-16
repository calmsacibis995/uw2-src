/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:lib/bsd/log.c	1.2.5.4"
#ident	"$Header: $"

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "logMgmt.h"
#include "lpd.h"

/*
 * Format and log message
 */
/*VARARGS2*/
void
#if defined (__STDC__)
logit(int type, char *msg, ...)
#else
logit(type, msg, va_alist)
int	 type;
char	*msg;
va_dcl
#endif
{
	va_list		 argp;
	static char	*buf;

	if (!(type & LOG_MASK))
		return;
	/*
	 * Use special buffer for log activity to avoid overwriting
	 * general buffer, Buf[], which may otherwise, be in use.
	 */
	if (!buf && !(buf = (char *)malloc(LOGBUFSZ)))
		return;			/* We're in trouble */
#if defined (__STDC__)
	va_start(argp, msg);
#else
	va_start(argp);
#endif
	(void)vsprintf(buf, msg, argp);
	va_end(argp);
	WriteLogMsg(buf);
}
