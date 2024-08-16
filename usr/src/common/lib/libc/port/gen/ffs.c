/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/ffs.c	1.3"

#ifdef __STDC__
	#pragma weak ffs = _ffs
#endif

#include "synonyms.h"

int
ffs(field)
int field;
{
	int	idx=1;

	if (field == 0)
		return(0);
	for (;;) {
		if (field & 1)
			return(idx);
		field >>= 1;
		++idx;
	}
}
