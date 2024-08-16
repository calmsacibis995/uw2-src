/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/munlockall.c	1.3"
#ifdef __STDC__
	#pragma weak munlockall = _munlockall
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/mman.h>

/*
 * Function to unlock address space from memory.
 */

/*LINTLIBRARY*/
munlockall()
{

	return (memcntl(0, 0, MC_UNLOCKAS, 0, 0, 0));
}
