/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/setup_exec.c	1.1"
#ident	"@(#)libmail:libmail/setup_exec.c	1.1"
#include "libmail.h"
/*
    NAME
	setup_exec - set up an argument vector

    SYNOPSIS
	char **setup_exec(char *command)

    DESCRIPTION
	Parse the command into a list of arguments appropriate
	for use by exec().
*/

#define MAXARGS 64
#define CHUNKSIZE 64
static char	*_argvec[MAXARGS]; /* enough to begin with */
static int	argcnt = MAXARGS-1;
static char	**argvec = &_argvec[0];

char **setup_exec(s)
char *s;
{
    register int i = parse_execarg(s, 0, &argcnt, &argvec, CHUNKSIZE, _argvec);
    argvec[i] = (char *)NULL;
    return (i == 0) ? (char**)NULL : argvec;
}
