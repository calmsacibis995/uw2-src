/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)sgs:libsgs/common/strerror.c	1.2"
/*LINTLIBRARY*/
#include "synonyms.h"
#include <stdio.h>

extern const char _sys_errs[];
extern const int _sys_index[];
extern int _sys_num_err;

char *
strerror(errnum)
int errnum;
{
	if (errnum < _sys_num_err && errnum >= 0)
		return((char *)(&_sys_errs[_sys_index[errnum]]));
	else
		return(NULL);
}
