/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:gen/_locale.c	1.9"

#include "synonyms.h"
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include "_locale.h"

#define NTAIL	(LC_NAMELEN + LC_NAMELEN + 1)
#define NHEAD	(sizeof(locdir) - 1 + LC_NAMELEN - 1)
#define NTOTAL	(NHEAD + NTAIL < 128 ? 128 : NHEAD + NTAIL)

int
#ifdef __STDC__
_openlocale(int cat, const char *loc, const char *part)
#else
_openlocale(cat, loc, part)int cat; const char *loc, *part;
#endif
{
	static const char locdir[] = "/usr/lib/locale/";
	char pathbuf[NTOTAL];
	char *p, *path = pathbuf;
	size_t n = strlen(loc);
	int ans;

	/*
	* Set ans to the maximum number of bytes we may need
	* beyond those for loc.  If pathbuf isn't guaranteed
	* to be long enough, allocate a buffer, temporarily.
	*/
	if (loc[0] == '/')
		ans = NTAIL;
	else
		ans = sizeof(locdir) - 1 + NTAIL;
	if (n > sizeof(pathbuf) - ans)
	{
		if ((path = (char *)malloc(n + ans)) == 0)
			return -1;
	}
	/*
	* Build the path piece by piece.
	* Use the system's directory if loc isn't a full pathname.
	*/
	p = path;
	if (loc[0] != '/')
	{
		(void)strcpy(p, locdir);
		p += sizeof(locdir) - 1;
	}
	(void)strcpy(p, loc);
	p += n;
	*p = '/';
	(void)strcpy(p + 1, _str_catname[cat]);
	/*
	* If part is "", just return whether the file could have been opened.
	* Otherwise when part is nonnull, append it to the path.
	*/
	if (part != 0)
	{
		if (part[0] == '\0')
		{
			ans = access(path, R_OK | EFF_ONLY_OK);
			goto skip;
		}
		p += strlen(p);
		*p = '/';
		(void)strcpy(p + 1, part);
	}
	ans = open(path, O_RDONLY);
skip:;
	if (path != pathbuf)
		free((void *)path);
	return ans;
}
