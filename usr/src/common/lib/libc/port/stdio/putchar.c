/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/putchar.c	1.8.2.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include "stdiom.h"

#undef putchar

#ifdef _REENTRANT
	#pragma weak putchar_unlocked = putchar
	#pragma weak _putchar_unlocked = putchar
#endif

int
#ifdef __STDC__
putchar(int ch)	/* append byte to stdout */
#else
putchar(ch)int ch;
#endif
{
	return fputc(ch, stdout);
}
