#ident	"@(#)xpr:devices/terminfo/strfld.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

extern char		*strpbrk();

/**
 ** strfld()
 **/

/*
 * This routine is like "strtok()", except that it does not
 * count multiple separators as one, but treats each as a field
 * separator, thus allowing empty fields.
 */

char			*strfld (string, sepset)
	char			*string,
				*sepset;
{
	static char		*savept;

	register char		*p	= (string? string : savept),
				*r;


	if (!p)
		return (0);

	/*
	 * Find the end of the next field, and put a null there
	 * to terminate the returned string.
	 */
	if (!(r = strpbrk(p, sepset)))
		savept = 0;	/* last field */
	else {
		*r = 0;
		savept = ++r;	/* more fields left */
	}

	return (p);
}

/**
 ** istrfld() - GET NEXT INTEGER FIELD
 ** uistrfld() - GET NEXT UNSIGNED INTEGER FIELD
 **/

int			istrfld (src, sep, def)
	char			*src,
				*sep;
	int			def;
{
	extern int		atoi();

	extern char		*strfld();

	register char		*s	= strfld(src, sep);


	return ((s && *s)? atoi(s) : def);
}

unsigned int		uistrfld (src, sep, def)
	char			*src,
				*sep;
	int			def;
{
	register int		result	= istrfld(src, sep, def);


	return (result < 0? 0 : result);
}
