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

#ident	"@(#)curses:common/lib/xlibcurses/screen/object.h	1.1.2.3"
#ident  "$Header: object.h 1.2 91/06/26 $"
/*********************************************************************
*                         COPYRIGHT NOTICE                           *
**********************************************************************
*        This software is copyright (C) 1982 by Pavel Curtis         *
*                                                                    *
*        Permission is granted to reproduce and distribute           *
*        this file by any means so long as no fee is charged         *
*        above a nominal handling fee and so long as this            *
*        notice is always included in the copies.                    *
*                                                                    *
*        Other rights are reserved except as explicitly granted      *
*        by written permission of the author.                        *
*                Pavel Curtis                                        *
*                Computer Science Dept.                              *
*                405 Upson Hall                                      *
*                Cornell University                                  *
*                Ithaca, NY 14853                                    *
*                                                                    *
*                Ph- (607) 256-4934                                  *
*                                                                    *
*                Pavel.Cornell@Udel-Relay   (ARPAnet)                *
*                decvax!cornell!pavel       (UUCPnet)                *
*********************************************************************/

/*
** $Header: object.h 1.2 91/06/26 $
**
**	object.h - Format of compiled terminfo files
**
**		Header (12 bytes), containing information given below
**		Names Section, containing the names of the terminal
**		Boolean Section, containing the values of all of the
**				boolean capabilities
**				A null byte may be inserted here to make
**				sure that the Number Section begins on an
**				even word boundary.
**		Number Section, containing the values of all of the numeric
**				capabilities, each as a short integer
**		String Section, containing short integer offsets into the
**				String Table, one per string capability
**		String Table, containing the actual characters of the string
**				capabilities.
**
**	NOTE that all short integers in the file are stored using VAX/PDP-style
**	byte-swapping, i.e., least-significant byte first.  The code in
**	read_entry() automatically fixes this up on machines which don't use
**	this system (I hope).
**
**  $Log:	object.h,v $
 * Revision 1.2  91/06/26  23:28:48  pascal_V4ES
 * rebased on SVR4ES S22 source
 * 
Revision 2.1  82/10/25  14:49:50  pavel
Added Copyright Notice

Revision 2.0  82/10/24  15:18:19  pavel
Beta-one Test Release

Revision 1.3  82/08/23  22:31:12  pavel
The REAL Alpha-one Release Version

Revision 1.2  82/08/19  19:10:18  pavel
Alpha Test Release One

Revision 1.1  82/08/12  18:48:55  pavel
Initial revision

**
*/


#define MAGIC	0432

struct header
{
	short	magic;		/* Magic Number (0432)			*/
	short	name_size;	/* Size of names section		*/
	short	bool_count;	/* Number of booleans			*/
	short	num_count;	/* Number of numbers			*/
	short	str_count;	/* Number of strings			*/
	short	str_size;	/* Size of string table			*/
};
