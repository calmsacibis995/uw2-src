/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_MENUP_H
#define	_MENUP_H
#ident	"@(#)debugger:libol/common/MenuP.h	1.4"

// toolkit specific members of the Menu class
// included by ../../gui.d/common/Menu.h

struct Menu_data;	// defined in Menu.C

#define	MENU_TOOLKIT_SPECIFICS		\
private:				\
	Widget		menu;		/* list widget */ \
	Menu_data	*list;		/* flat list table for menu data */ \
	Boolean		delete_table;	/* TRUE if the table was reallocated by add_item */

#endif	// _MENUP_H
