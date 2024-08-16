/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/mlockall.c	1.4"
#ifdef __STDC__
	#pragma weak mlockall = _mlockall
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/mman.h>

/*
 * Function to lock address space in memory.
 */

/*LINTLIBRARY*/
mlockall(flags)
	int flags;
{

	return (memcntl(0, 0, MC_LOCKAS, (caddr_t)flags, 0, 0));
}
