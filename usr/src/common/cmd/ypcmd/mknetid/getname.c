/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ypcmd:mknetid/getname.c	1.1"
#ident  "$Header: $"

#include <stdio.h>
#include <string.h>

#define iseol(c)	(c == 0 || c == '\n' || strchr(com, c) != NULL)
#define issep(c)	(strchr(sep, c) != NULL)
#define isignore(c) (strchr(ignore, c) != NULL)

/*
 * getline()
 * Read a line from a file.
 * What's returned is a cookie to be passed to getname
 */
char **
getline(line, maxlinelen, f, lcount, com)
	char *line;
	int maxlinelen;
	FILE *f;
	int *lcount;
	char *com;
{
	char *p;
	static char *lp;
	do {
		if (! fgets(line, maxlinelen, f)) {
			return (NULL);
		}
		(*lcount)++;
	} while (iseol(line[0]));
	p = line;
	for (;;) {	
		while (*p) {	
			p++;
		}
		if (*--p == '\n' && *--p == '\\') {
			if (! fgets(p, maxlinelen, f)) {
				break;	
			}
			(*lcount)++;
		} else {
			break;	
		}
	}
	lp = line;
	return (&lp);
}


/*
 * getname()
 * Get the next entry from the line.
 * You tell getname() which characters to ignore before storing them 
 * into name, and which characters separate entries in a line.
 * The cookie is updated appropriately.
 * return:
 *	  1: one entry parsed
 *	  0: partial entry parsed, ran out of space in name
 *  -1: no more entries in line
 */
getname(name, namelen, ignore, sep, linep, com)
	char *name;
	int namelen;
	char *ignore;	
	char *sep;
	char **linep;
	char *com;
{
	register char c;
	register char *lp;
	register char *np;

	lp = *linep;
	do {
		c = *lp++;
	} while (isignore(c) && !iseol(c));
	if (iseol(c)) {
		*linep = lp - 1;
		return (-1);
	}
	np = name;
	while (! issep(c) && ! iseol(c) && np - name < namelen) {	
		*np++ = c;	
		c = *lp++;
	} 
	lp--;
	if (np - name < namelen) {
		*np = 0;
		if (iseol(c)) {
			*lp = 0;
		} else {
			lp++; 	/* advance over separator */
		}
	} else {
		*linep = lp;
		return (0);
	}
	*linep = lp;
	return (1);
}
