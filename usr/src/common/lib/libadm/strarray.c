/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libadm:common/lib/libadm/strarray.c	1.1"

#include <stdarg.h>
#include <stdlib.h>
#include <locale.h>
#include <pfmt.h>
#include <string.h>

#define	MAX_ARGS	64

/*
 * int
 * str_array(char ***strtab, ...)
 *
 *	This function creates a string table to be stored in strtab.
 *	The table entries are the remainder of the arguments passed
 *	to the function.
 *
 *	The argument list is terminated by the NULL string.
 *
 * RETURN VALUE:
 *	When successful, the number of entries is returned.  Otherwise,
 *	-1 is returned on failure.
 *
 * SIDE EFFECTS:
 *	The first argument is updated with the locataion of the table
 *	on success, or is set to NULL on failure.
 */
int
str_array(char ***strtab, ...)
{
	register char *pt;
	register char **pstr;
	int n = 0;
	va_list args;

	*strtab =  NULL;
	if ((pstr = (char **)calloc(MAX_ARGS, sizeof(char *))) == NULL) {
		return -1;
	}
	va_start(args, strtab);
	while ((pt = va_arg(args, char *)) != NULL) {
		pstr[n++] = pt;
		if ((n % MAX_ARGS) == 0) {
			pstr = (char **)realloc((void *)pstr,
					(n + MAX_ARGS) * sizeof(char *));
			if (pstr == NULL) {
				return -1;
			}
		}
	}
	va_end(args);
	pstr[n] = NULL;
	pstr = (char **)realloc((void *)pstr, (n + 1) * sizeof(char *));
	*strtab = pstr;
	return (pstr != NULL) ? n : -1;
}
