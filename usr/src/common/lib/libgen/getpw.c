/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libgen:getpw.c	1.10"
/*	3.0 SID #	1.2	*/
/*LINTLIBRARY*/
#ifdef __STDC__
	#pragma weak getpw = _getpw
#endif
#include "synonyms.h"
#include <sys/types.h>
#include <stdio.h>
#include <ctype.h>

extern void rewind();
extern FILE *fopen();

static FILE *pwf;

int
getpw(uid, buf)
uid_t	uid;
char	buf[];
{
	register n, c;
	register char *bp;

	if(pwf == 0)
		pwf = fopen("/etc/passwd", "r");
	if(pwf == NULL)
		return(1);
	rewind(pwf);

	for(;;) {
		bp = buf;
		while((c=getc(pwf)) != '\n') {
			if(c == EOF)
				return(1);
			*bp++ = (char)c;
		}
		*bp = '\0';
		bp = buf;
		n = 3;
		while(--n)
			while((c = *bp++) != ':')
				if(c == '\n')
					return(1);
		while((c = *bp++) != ':')
			if(isdigit(c))
				n = n*10+c-'0';
			else
				continue;
		if(n == uid)
			return(0);
	}
}
