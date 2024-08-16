/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/valloc.c	1.2"

#ifdef __STDC__
	#pragma weak valloc = _valloc
#endif

#include "synonyms.h"
#include <stdlib.h>
#include <unistd.h>

VOID *
valloc(size)
	size_t size;
{
	static unsigned pagesize;
	if (!pagesize)
		pagesize = sysconf(_SC_PAGESIZE);
	return memalign(pagesize, size);
}
