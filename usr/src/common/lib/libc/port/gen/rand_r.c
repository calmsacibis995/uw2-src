/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/rand_r.c	1.1"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak	rand_r = _rand_r
#endif

#include "synonyms.h"

int
rand_r(unsigned int *seed)
{
	return(((*seed = *seed * 1103515245L + 12345)>>16) & 0x7fff);
}
