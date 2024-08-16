/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/msync.c	1.4"
#ifdef __STDC__
	#pragma weak msync = _msync
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/mman.h>

msync(addr, len, flags)
caddr_t	addr;
size_t	len;
int flags;
{
	return (memcntl(addr, len, MC_SYNC, (caddr_t)flags, 0, 0));
}
