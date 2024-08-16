/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ifndef NOIDENT
#ident	"@(#)Dt:error.c	1.3"
#endif

#ifdef NOT_USE
/* error.c */
#include <stdio.h>

void
Dt__MemoryError()
{
	fprintf(stderr,"out of memory\n");
}
#else
Dt__MemoryNoError() {}
#endif
