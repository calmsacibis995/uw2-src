/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/getchar.c	1.8.2.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#undef getchar

#ifdef _REENTRANT
	#pragma weak getchar_unlocked = getchar
	#pragma weak _getchar_unlocked = getchar
#endif

int
#ifdef __STDC__
getchar(void)	/* return next byte for stdin */
#else
getchar()
#endif
{
	return fgetc(stdin);
}
