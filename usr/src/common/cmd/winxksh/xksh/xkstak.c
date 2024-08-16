/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)winxksh:xksh/xkstak.c	1.2"

#include <stdio.h>
#include "xksh.h" /* which includes sys/types.h */
#include <sys/param.h>
#include <string.h>
#include <varargs.h>
#include <search.h>
#include <ctype.h>
#include "stak.h"

char *
bgrow(orig_ptr, orig_size, add)
char *orig_ptr;
ulong orig_size;
ulong add;
{
	return(realloc(orig_ptr, orig_size + add));
}

char *
stakbgrow(orig_ptr, orig_size, add)
char *orig_ptr;
ulong orig_size;
ulong add;
{
	if ((orig_ptr + orig_size == stakptr(staktell())) && (_stak_cur.stakleft < add)) {
		stakalloc(add);
		return(orig_ptr);
	}
	else {
		char *ret;

		ret = stakalloc(orig_size + add);
		memcpy(ret, orig_ptr, orig_size);
		return(ret);
	}
}

void
xkstakfree()
{
}
