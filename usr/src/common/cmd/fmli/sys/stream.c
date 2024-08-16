/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1985 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:sys/stream.c	1.1.4.3"

#include	<stdio.h>
#include	"wish.h"
#include	"token.h"

token
stream(t, s)
register token t;
register token (*s[])();
{
	register int	i;

	for (i = 0; s[i]; i++)
		if ((t = (*(s[i]))(t)) == TOK_NOP)
			break;
	return t;
}
