/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)oamintf:common/cmd/oamintf/intf_include/menu_io.h	1.1.7.2"
#ident  "$Header: menu_io.h 2.0 91/07/12 $"

#define P_NONE		0	/* no placeholder */
#define P_INACTIVE	1	/* placeholder - inactive */
#define P_ACTIVE	2	/* placeholder - active */

/* pull in sizes from intf.h */
struct item_def {
	char mname[(NAMELEN+1)];	/* menu item name */
	char mdescr[(DESCLEN+1)];	/* menu item description */
	char help[HELPLEN];		/* menu item help file */
	int pholder;			/* placeholder status */
	char maction[ACTLEN];		/* menu item action */
	char pkginsts[PKGILEN];		/* pkg instance identifiers */
	char orig_name[(NAMELEN+1)];	/* original name if rename */
	struct item_def *next;		/* next menu item */
};

struct menu_file {
	struct menu_line *head;		/* menu file header lines */
	struct item_def *entries;	/* menu file entries */
};
