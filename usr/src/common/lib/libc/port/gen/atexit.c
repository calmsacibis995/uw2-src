/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/atexit.c	1.5"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdlib.h>
#include "stdlock.h"

#define MAXEXITFNS	37

static void	(*exitfns[MAXEXITFNS])();
static int	numexitfns = 0;
#ifdef _REENTRANT
static StdLock __exit_lock;
#endif

int
atexit(func)
void	(*func)();
{
	register int retval = 0;
	STDLOCK(&__exit_lock);
	if (numexitfns >= MAXEXITFNS)
	{
		retval = -1;
		goto atexit_out;
	}

	exitfns[numexitfns++] = func;
atexit_out:
	STDUNLOCK(&__exit_lock);
	return(retval);
}

void
_exithandle()
{
	STDLOCK(&__exit_lock);
	while (--numexitfns >= 0)
		(*exitfns[numexitfns])();
	STDUNLOCK(&__exit_lock);
}
