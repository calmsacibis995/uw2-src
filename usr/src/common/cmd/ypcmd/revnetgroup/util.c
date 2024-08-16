/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:revnetgroup/util.c	1.1"
#ident  "$Header: $"

#include <stdio.h>
#include <pfmt.h>
#include <locale.h>
#include "util.h"




/*
 * This is just like fgets, but recognizes that "\\n" signals a continuation
 * of a line
 */
char *
getline(line,maxlen,fp)
	char *line;
	int maxlen;
	FILE *fp;
{
	register char *p;
	register char *start;


	start = line;

nextline:
	if (fgets(start,maxlen,fp) == NULL) {
		return(NULL);
	}	
	for (p = start; ; p++) {
		if (*p == '\n') {       
			if (*(p-1) == '\\') {
				start = p - 1;
				goto nextline;
			} else {
				return(line);	
			}
		}
	}
}	




	
void
fatal(message)
	char *message;
{
	(void) pfmt(stderr, MM_STD, ":5:fatal error: %s\n", message);
	exit(1);
}
