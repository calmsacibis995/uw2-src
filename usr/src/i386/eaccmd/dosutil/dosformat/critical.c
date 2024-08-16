/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)eac:i386/eaccmd/dosutil/dosformat/critical.c	1.1.1.2"
#ident  "$Header: critical.c 1.1 91/07/03 $"

/* #define		DEBUG		1	/* */

#include	<signal.h>

#include	<stdio.h>

void
critical(on)
int	on;
{
	(void) signal(SIGHUP, on ? SIG_IGN : SIG_DFL);
	(void) signal(SIGINT, on ? SIG_IGN : SIG_DFL);
	(void) signal(SIGQUIT, on ? SIG_IGN : SIG_DFL);

#ifdef DEBUG
	(void) fprintf(stderr, "critical(): DEBUG - Critical code is %s\n", on ? "ON" : "OFF");
#endif
}
