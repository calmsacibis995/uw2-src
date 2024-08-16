/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)lprof:libprof/common/symintErr.c	1.1.1.2"

#include "hidelibc.h"		/* uses "_" to hide libc functions */
#include "hidelibelf.h"		/* uses "_" to hide libelf functions */

#include "symint.h"

#include <stdio.h>
#include <varargs.h>
#include <pfmt.h>

/* * * * * *
 * symintFcns.c -- symbol information interface routines.
 * 
 * these routines form a symbol information access
 * interface, for the profilers to get at object file
 * information.  this interface was designed to aid
 * in the COFF to ELF conversion of prof, lprof and friends.
 * 
 */


/* * * * * *
 * _err_exit(format_s, va_alist)
 * format_s	- printf(3S) arg string.
 * va_alist	- var_args(3?) printf() arguments.
 * 
 * does not return - prints message and calls exit(3).
 * 
 * 
 * this routine spits out a message (passed as above)
 * and exits.
 */

_err_exit(format_s, va_alist)
char *format_s;
va_dcl
{
	va_list ap;
	char  msgbuf[512];

	va_start(ap);
	vsprintf(msgbuf, format_s, ap);
	pfmt(stderr,MM_ERROR,"uxcds:108:%s\n",msgbuf);
	va_end(ap);

debugp1("--- this is where we exit ---\n")

	exit(1);
}


/* * * * * *
 * _err_warn(format_s, va_alist)
 * format_s	- printf(3S) arg string.
 * va_alist	- var_args(3?) printf() arguments.
 * 
 * 
 * This routine prints a warning (passed as above)
 */

_err_warn(format_s, va_alist)
char *format_s;
va_dcl
{
	va_list ap;
	char  msgbuf[512];

	va_start(ap);
	vsprintf(msgbuf, format_s, ap);
	pfmt(stderr,MM_WARNING,"uxcds:108:%s\n",msgbuf);
	va_end(ap);
}
