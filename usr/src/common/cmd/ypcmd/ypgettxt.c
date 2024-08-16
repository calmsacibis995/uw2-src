/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:ypgettxt.c	1.1"
#ident  "$Header: $"

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include <malloc.h>

char *
ypgettxt(s1, s2)
char *s1, *s2;
 
{
 
	char *ptr, *ptr2;
	extern char *gettxt();
	extern int errno;
 
	if ((ptr = gettxt(s1, s2)) == NULL) {
		pfmt(stderr, MM_ERROR, ":2:gettxt failed\n");
		return(NULL);
	}

	if ((ptr2 = malloc(strlen(ptr)+1)) == NULL) {
		pfmt(stderr, MM_ERROR, ":3:getcpytxt: %s\n",
		strerror(errno));
		return(NULL);
	}

	strcpy(ptr2, ptr);
	 
	return(ptr2);
 
}
