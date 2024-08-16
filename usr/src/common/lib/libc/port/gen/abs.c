/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/abs.c	1.10"
/*LINTLIBRARY*/
#include "synonyms.h"
#include <stdlib.h>

int
abs(arg)
register int arg;
{
	return (arg >= 0 ? arg : -arg);
}

long
labs(arg)
register long int arg;
{
	return (arg >= 0 ? arg : -arg);
}
