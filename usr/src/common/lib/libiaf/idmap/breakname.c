/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)libiaf:common/lib/libiaf/idmap/breakname.c	1.3.1.2"
#ident  "$Header: breakname.c 1.2 91/06/25 $"


/* This file should be kept in sync with file breakname.c */
/* in the idmap component. */

/* The following files are needed if breakname.c is compiled by itself. */
/* Since it is being #included by programs which already include the */
/* files below, the following #include's are not necessary. The function */
/* declaration should also be changed to become global (not static). */

/* #include <string.h> */
/* #include "idmap.h" */

#define	EMPTYSTR	""

/*
 * breakname() - breaks a name into fields according to a field
 * descriptor.
 *
 * input:	name - unbroken name (see note below)
 *		fd - field descriptor line (like "!M1@M2\n")
 *		     NOTE: fd has to end with a new line
 * output:      fields[] - array of pointers into field info structures
 *
 * NOTE: This function modifies the argument 'name' by replacing each field
 * separator with the end-of-string character ('\0').
 *
 */

static int
breakname(name, fd, fields)
char *name;		/* name string */
char *fd;		/* field descriptor line, with leading '!' */
FIELD fields[];		/* field information pointers */
{
	int	fieldno;	/* field number */
	char	delim;		/* delimiter */

	/* initialize field pointers */
	for (fieldno = 0; fieldno < MAXFIELDS; fieldno++) {
		fields[fieldno].value = EMPTYSTR;
		fields[fieldno].type = ' ';
	}

	while (*(fd + 3) != '\n') {

		fieldno = ((char) *(fd + 2)) - '0';
		delim = *(fd + 3);

		fields[fieldno].value = name;
		fields[fieldno].type = *(fd + 1);

		while ((*name != delim) && (*name != '\0'))
			name++;

		if (*name == '\0')
			/* early end of name (not all fields were present) */
			return(-1);

		*name = '\0';
		name++;
		fd += 3;
	}

	/* take care of the last field */

	fieldno = ((char) *(fd + 2)) - '0';
	fields[fieldno].value = name;
	fields[fieldno].type = *(fd + 1);

	return(0);
}
