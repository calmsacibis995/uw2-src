/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)ktool:common/ktool/locklist/getpl.c	1.1"

/* Target system headers: */
#include <sys/ipl.h>

/*
 * int
 * getpl(unsigned pl)
 *	Translate a pl_t value to a logical PL number (0, 1, 2, ...).
 */
int
getpl(pl)
unsigned pl;
{
	switch (pl) {
	case PL0:
		return 0;
#ifdef PL1
	case PL1:
		return 1;
#endif
#ifdef PL2
	case PL2:
		return 2;
#endif
#ifdef PL3
	case PL3:
		return 3;
#endif
#ifdef PL4
	case PL4:
		return 4;
#endif
#ifdef PL5
	case PL5:
		return 5;
#endif
#ifdef PL6
	case PL6:
		return 6;
#endif
#ifdef PL7
	case PL7:
		return 7;
#endif
#ifdef PL8
	case PL8:
		return 8;
#endif
	}

	return -1;
}
