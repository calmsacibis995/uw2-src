/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/setlabel.c	1.3"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdlib.h>
#include <string.h>
#include "pfmtm.h"

#ifdef __STDC__
	#pragma weak setlabel = _setlabel
#endif

#ifdef _REENTRANT
StdLock _pfmt_lock_setlabel;
#endif

int
#ifdef __STDC__
setlabel(const char *label)
#else
setlabel(label)const char *label;
#endif
{
	int ret = 0;

	STDLOCK(&_pfmt_lock_setlabel);
	if (_pfmt_label != 0)
	{
		if (label != 0 && strcmp(label, _pfmt_label) == 0)
			goto skip;
		free((void *)_pfmt_label);
	}
	if (label != 0 && (label = strdup(label)) == 0)
		ret = -1;
	_pfmt_label = label;
skip:;
	STDUNLOCK(&_pfmt_lock_setlabel);
	return ret;
}
