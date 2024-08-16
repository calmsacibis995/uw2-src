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

#ident	"@(#)curses:common/lib/xlibcurses/screen/unctrl.h	1.3.2.3"
#ident  "$Header: unctrl.h 1.2 91/06/27 $"
/*
 * unctrl.h
 *
 */

#ifndef UNCTRL_H
#define	UNCTRL_H

extern char	*_unctrl[];

#if	!defined(NOMACROS) && !defined(lint)

#define	unctrl(ch)	(_unctrl[(unsigned) ch])

#endif	/* NOMACROS && lint */

#endif	/* UNCTRL_H */
