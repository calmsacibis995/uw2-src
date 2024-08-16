/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:str/strerror.c	1.7"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <string.h>
#include "_locale.h"

extern const char _sys_errs[];
extern const int _sys_index[];
extern int _sys_num_err;

char *
#ifdef __STDC__
strerror(int errnum)
#else
strerror(errnum)int errnum;
#endif
{
	static const char unknown[] = "Unknown error";
	const char *p;

	if (errnum < 0 || errnum >= _sys_num_err)
	{
		p = unknown;
		errnum = 1;
	}
	else
	{
		p = &_sys_errs[_sys_index[errnum]];
		errnum += 3;
	}
	return (char *)__gtxt(_str_uxsyserr, errnum, p);
}
