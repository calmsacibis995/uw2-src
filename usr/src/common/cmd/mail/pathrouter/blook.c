/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/pathrouter/blook.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)blook.c	1.2 'attmail mail(1) command'"
/* static char 	*sccsid="@(#)blook.c	2.6 (smail) 5/24/88"; */

#include "defs.h"

int
blook(file, key, buf)
FILE *file;
char *key, *buf;
{
	int c, keylen;
	long middle, hi, lo;
	long length;
	register char *s;
	char line[SMLBUF];

	(void) fseek(file, 0L, 2);	/* find length */
	length = ftell(file);
	if( length == -1 ) {
		return(-1);
	}

	lo = 0;
	hi = length;
	keylen = strlen(key);
/*
** "Binary search routines are never written right the first time around."
** - Robert G. Sheldon.
*/
	 while((hi - lo) > 1024) {

		middle = (hi + lo)/2;
		(void) fseek(file, middle, 0);	/* find midpoint */
		/* skip to beginning of next line */
		if(fgets(line, sizeof(line), file) == NULL) {
			break;
		}

		middle = ftell(file);

		if(fgets(line, sizeof(line), file) == NULL) {
			break;
		}

		c = casncmp(key, line, keylen);
		if(c == 0) {
			goto solved;
		} else if(c > 0) {
			lo = middle;
		} else if(c < 0) {
			hi = middle;
		}
	}
	(void) fseek(file, lo, 0);
	while(fgets(line, sizeof(line), file)) {
		if(casncmp(key, line, keylen) == 0) {
solved:
			(void) strcpy(buf, line+keylen);
			if((s = index(buf, '\n')) != NULL) *s = '\0';
			return(0);
		}
		if(ftell(file) > hi) break;
	}
	return(-1);

}
