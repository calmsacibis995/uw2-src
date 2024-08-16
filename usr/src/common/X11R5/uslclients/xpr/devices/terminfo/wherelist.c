#ident	"@(#)xpr:devices/terminfo/wherelist.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/* EMACS_MODES: !fill, lnumb, !overwrite, !nodelete, !picture */

#include "string.h"

#define	STREQU(A,B)	(strcmp((A), (B)) == 0)

/**
 ** wherelist() - RETURN POINTER TO ITEM IN LIST
 **/

char			**wherelist (item, list)
	register char		*item;
	register char		**list;
{
	if (!list || !*list)
		return (0);

	while (*list) {
		if (STREQU(*list, item))
			return (list);
		list++;
	}
	return (0);
}
