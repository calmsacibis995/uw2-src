/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/bufsync.c	1.1"

#include <stdio.h>
#include "stdiom.h"

Uchar *const _bufendtab[61] = {0};

Uchar *
_realbufend(FILE *iop)
{
	return 0;
}

void
_bufsync(FILE *iop, Uchar *ptr)
{
	return;
}
