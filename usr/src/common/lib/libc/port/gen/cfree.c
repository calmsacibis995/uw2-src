/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/cfree.c	1.2"
/*LINTLIBRARY*/
/*	cfree - clear memory block
*/
#define NULL 0
#ifdef __STDC__
	#pragma weak cfree = _cfree
#endif
#include "synonyms.h"
#include <stdlib.h>

/*ARGSUSED*/
void
cfree(p, num, size)
char *p;
unsigned num, size;
{
	free(p);
}
