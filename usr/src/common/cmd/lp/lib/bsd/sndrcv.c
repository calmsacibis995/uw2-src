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


#ident	"@(#)lp:lib/bsd/sndrcv.c	1.3.1.4"
#ident	"$Header: $"

#if defined(__STDC__)
#include <stdarg.h>
#else
#include <varargs.h>
#endif
#include "lp.h"
#include "msgs.h"
#define WHO_AM_I	I_AM_OZ		/* to get oam.h to unfold */
#include "oam_def.h"
#include "oam.h"
#include "lpd.h"

/*
 * Format and send message to lpsched
 * (die if any errors occur)
 */
/*VARARGS1*/
void
#if defined (__STDC__)
snd_msg(int type, ...)
#else
snd_msg(type, va_alist)
int	type;
va_dcl
#endif
{
	va_list	ap;

#if defined (__STDC__)
	va_start(ap, type);
#else
	va_start(ap);
#endif
	(void)_putmessage (Msg, type, ap);
	va_end(ap);
	if (msend(Msg) == -1) {
		lp_fatal(E_LP_MSEND, NOLOG); 
		/*NOTREACHED*/
	}
}

/*
 * Recieve message from lpsched
 * (die if any errors occur)
 */
void
#if defined (__STDC__)
rcv_msg(int type, ...)
#else
rcv_msg(type, va_alist)
int	type;
va_dcl
#endif
{
	va_list ap;
	int rc;

	if ((rc = mrecv(Msg, MSGMAX)) != type) {
		if (rc == -1)
			lp_fatal(E_LP_MRECV, NOLOG); 
		else
			lp_fatal(E_LP_BADREPLY, NOLOG, rc); 
		/*NOTREACHED*/
	}
#if defined (__STDC__)
	va_start(ap, type);
#else
	va_start(ap);
#endif
	rc = _getmessage(Msg, type, ap);
	va_end(ap);
	if (rc < 0) {
		lp_fatal(E_LP_GETMSG, NOLOG, PERROR); 
		/*NOTREACHED*/ 
	} 
}
