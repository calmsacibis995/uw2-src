/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)ucb:common/ucbcmd/plot/libplot/imagen/linemod.c	1.2"
#ident	"$Header: $"

/*
 * Copyright (c) 1980 Regents of the University of California.
 * All rights reserved. The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

/*
 * Copyright (c) 1983, 1984 1985, 1986, 1987, 1988, Sun Microsystems, Inc.
 * All Rights Reserved.
 */


#include "imp.h"

/*
 * Hack to set font.
 */
linemod(s)
char *s;
{
	register char *tit;
	register char *nam;
	int siz = 0;
	nam = s;
	for(tit = "charset="; *tit; )
		if (*tit++ != *nam++)
			return;
	s = nam;
	while(*nam) 
		switch(*nam++) {
		case ',':
		case '\n':
			*--nam = 0;
		}
	siz = atoi(++nam);
	if (siz == 0) {
		while (*--nam >= '0' && *nam <= '9')
			;
		siz = (atoi(++nam)*4)/3;
	}
	if (siz == 0)
		siz = imPcsize;
	setfont(s, siz);
}
