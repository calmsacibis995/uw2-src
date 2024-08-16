/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libc-port:stdio/cuserid.c	1.15.1.2"
/*LINTLIBRARY*/

#include "synonyms.h"
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#ifdef __STDC__
	#pragma weak cuserid = _cuserid
#endif

char *
#ifdef __STDC__
cuserid(char *s)
#else
cuserid(s)char *s;
#endif
{
	static char res[L_cuserid];
	struct passwd *pw;
	char *p;

	if (s == 0)
		s = res;
	if ((p = getlogin()) != 0)
		return strcpy(s, p);
	if ((pw = getpwuid(getuid())) != 0)
		return strcpy(s, pw->pw_name);
	*s = '\0';
	return 0;
}
