/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*LINTLIBRARY*/
#ident	"@(#)libpkg:common/lib/libpkg/canonize.c	1.2.4.2"
#ident "$Header: /sms/sinixV5.4es/rcs/s19-full/usr/src/lib/libpkg/canonize.c,v 1.1 91/02/28 20:55:00 ccs Exp $"
#include <string.h>

#define isdot(x)	((x[0]=='.')&&(!x[1]||(x[1]=='/')))
#define isdotdot(x)	((x[0]=='.')&&(x[1]=='.')&&(!x[2]||(x[2]=='/')))

void
canonize(file)
char *file;
{
	char *pt, *last;
	int level;

	/* remove references such as './' and '../' and '//' */
	for(pt=file; *pt; ) {
		if(isdot(pt))
			(void) strcpy(pt, pt[1] ? pt+2 : pt+1);
		else if(isdotdot(pt)) {
			level = 0;
			last = pt;
			do {
				level++;
				last += 2;
				if(*last)
					last++;
			} while(isdotdot(last));
			--pt; /* point to previous '/' */
			while(level--) {
				if(pt <= file)
					return;
				while((*--pt != '/') && (pt > file))
					;
			}
			if(*pt == '/')
				pt++;
			(void) strcpy(pt, last);
		} else {
			while(*pt && (*pt != '/'))
				pt++;
			if(*pt == '/') {
				while(pt[1] == '/')
					(void) strcpy(pt, pt+1);
				pt++;
			}
		}
	}
	if((--pt > file) && (*pt == '/'))
		*pt = '\0';
}
