/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 * Copyright  (c) 1986 AT&T
 *	All Rights Reserved
 *
 */

#ident	"@(#)fmli:inc/menu.h	1.1.4.3"

struct menu {
	struct menu_line	(*disp)();
	char	*arg;
	vt_id	vid;
	int	flags;
	int	index;			/* current item */
	int	hcols;			/* # of chars highlighted */
	int	topline;		/* top line displayed */
	int	number;			/* number of items */
	/* max length of highlight and description parts */
	int	hwidth;
	int	dwidth;
	/* multi-column parameters */
	int	ncols;
};

#define MENU_DIRTY	1
#define MENU_USED	2
#define MENU_CENTER	4
#define MENU_NONUMBER	8
#define MENU_MSELECT	16
#define MENU_TRUNC	32	/* no room for description; show elipses */
#define ALL_MNU_FLAGS	63
#define MENU_ALL	1000	/* max number of chars to highlight on line */

extern struct menu	*MNU_array;
extern menu_id	MNU_curid;
