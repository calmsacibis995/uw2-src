/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*ident	"@(#)sc:Regex/libx/reerror.c	3.1" */
/*
 * AT&T Bell Laboratories
 *
 * regular expression error routine
 */

#include <re.h>
#include <error.h>

void
reerror_Regex_ATTLC(s)
char*	s;
{
	printf("Regex error is %s\n", s);
}
