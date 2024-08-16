/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/setcat.c	1.1"

#include "synonyms.h"
#include <pfmt.h>
#include <string.h>
#include "_locale.h"
#include "stdlock.h"

#ifdef __STDC__
	#pragma weak setcat = _setcat
#endif

char *
#ifdef __STDC__
setcat(const char *name)
#else
setcat(name)const char *name;
#endif
{
#ifdef _REENTRANT
	static StdLock lock;
#endif
	static char defname[LC_NAMELEN];
	const char *q;
	char *p;

	STDLOCK(&lock);
	p = &defname[0];
	if (name != 0)	/* assign new default */
	{
		q = &name[0];
		while ((*p = *q++) != '\0')
		{
			if (p == &defname[LC_NAMELEN])
			{
				*p = '\0';
				break;
			}
			p++;
		}
		p = &defname[0];
	}
	if (*p == '\0')
		p = 0;
	STDUNLOCK(&lock);
	return p;
}
