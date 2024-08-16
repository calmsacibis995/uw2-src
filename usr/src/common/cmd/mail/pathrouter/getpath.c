/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pathrouter/getpath.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)getpath.c	1.2 'attmail mail(1) command'"
/* static char 	*sccsid="@(#)getpath.c	2.6 (smail) 5/24/88"; */

#include	"defs.h"

extern enum edebug debug;	/* how verbose we are 		*/ 
extern char *pathfile;		/* location of path database	*/

/*
**
** getpath(): look up key in ascii sorted path database.
**
*/

int
getpath( key, path , cost)
char *key;		/* what we are looking for */
char *path;		/* where the path results go */
int *cost;		/* where the cost results go */
{
	register char *s;
	static FILE *file;
	int tcost;
	char tkey[SMLBUF];

DEBUG("getpath: looking for '%s'\n", key);

	if(file == NULL) {	/* open file on first use */
		if((file = fopen(pathfile, "r")) == NULL) {
			(void) fprintf(stderr,
				"can't access '%s':", pathfile);
			perror("");
			return(EX_OSFILE);
		}
	}

	(void) strcpy( tkey, key );
	(void) strcat( tkey, "\t" );

	if(blook(file, tkey, path) < 0) {
		return(EX_NOHOST);
	}
/*
** See if the next field on the line is numeric.
** If so, use it as the cost for the route.
*/
	if((s = index(path, '\t')) != NULL) {

		*s++ = '\0';

		tcost = -1;
		while(isdigit(*s)) {
			if(tcost < 0) tcost = 0;
			tcost *= 10;
			tcost += *s - '0';
			s++;
		}
		if(tcost >= 0) *cost = tcost;
	}
	return (EX_OK);
}
