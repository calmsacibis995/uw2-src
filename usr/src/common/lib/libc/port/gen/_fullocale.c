/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_fullocale.c	1.1"

#include "synonyms.h"
#include <string.h>
#include "_locale.h"

char *
#ifdef __STDC__
_fullocale(const char *dir, const char *file) /* only for compatibility */
#else
_fullocale(dir, file)const char *dir, *file;
#endif
{
	static const char sep[] = "/";
	static char path[16 + LC_NAMELEN + LC_NAMELEN] = "/usr/lib/locale/";

	strlist(&path[16], dir, sep, file, (char *)0);
	return path;
}
