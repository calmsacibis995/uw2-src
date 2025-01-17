/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)curses:common/lib/xlibcurses/screen/mbgetwidth.c	1.3.2.3"
#ident  "$Header: mbgetwidth.c 1.2 91/06/26 $"
#include	"curses_inc.h"
#include <ctype.h>
#define CSWIDTH	514

short		cswidth[4] = {-1,1,1,1};	/* character length */
short		_scrwidth[4] = {1,1,1,1};	/* screen width */

/*
 * This function is called only once in a program.
 * Before cgetwidth() is called, setlocale() must be called.
 */

void
mbgetwidth()
{
#ifdef __STDC__
	unsigned char *cp = &__ctype[CSWIDTH];
#else
	unsigned char *cp = &_ctype[CSWIDTH];
#endif

	cswidth[0] = cp[0];
	cswidth[1] = cp[1];
	cswidth[2] = cp[2];
	_scrwidth[0] = cp[3];
	_scrwidth[1] = cp[4];
	_scrwidth[2] = cp[5];

	return;
}

int
mbeucw(c)
int	c;
{
	c &= 0xFF;

	if (c & 0x80) {
		if (!multibyte) {
			return (cswidth[0]);
		} else if (c == SS2) {
			return(cswidth[1]);
		} else if (c == SS3) {
			return(cswidth[2]);
		}
		return(cswidth[0]);
	}
	return(1);
}

int
mbscrw(c)
int	c;
{
	c &= 0xFF;

	if (c & 0x80) {
		if (!multibyte) {
			return (cswidth[0]);
		} else if (c == SS2) {
			return(_scrwidth[1]);
		} else if (c == SS3) {
			return(_scrwidth[2]);
		}
		return(_scrwidth[0]);
	}
	return(1);
}

#ifdef	_WCHAR16
int
wcscrw(wc)
wchar_t	wc;
{
	int	rv;

	switch (wc & H_EUCMASK) {
	case	H_P11:	/* Code set 1 */
		rv = _scrwidth[0];
		break;
	case	H_P01:	/* Code set 2 */
		rv = _scrwidth[1];
		break;
	case	H_P10:	/* Code set 3 */
		rv = _scrwidth[2];
		break;
	default	:	/* Code set 0 */
		rv = 1;
		break;
	}

	return (rv);
}
#else	/*_WCHAR16*/
int
wcscrw(wc)
wchar_t	wc;
{
	int	rv;

	switch (wc & EUCMASK) {
	case	P11:	/* Code set 1 */
		rv = _scrwidth[0];
		break;
	case	P01:	/* Code set 2 */
		rv = _scrwidth[1];
		break;
	case	P10:	/* Code set 3 */
		rv = _scrwidth[2];
		break;
	default	:	/* Code set 0 */
		rv = 1;
		break;
	}

	return (rv);
}
#endif	/*_WCHAR16*/
