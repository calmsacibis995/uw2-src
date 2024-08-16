/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-i386:gen/siglongjmp.c	1.2"

#ifdef __STDC__
	#pragma weak siglongjmp = _siglongjmp
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <sys/ucontext.h>

#include <setjmp.h>

void 
siglongjmp(env,val)
sigjmp_buf env;
int val;
{
	register ucontext_t *ucp = (ucontext_t *)env;
	if (val)
		ucp->uc_mcontext.gregs[ EAX ] = val;
	asm("cld");
	setcontext(ucp);
}
