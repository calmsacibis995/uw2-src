/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_MENU_BARP_H
#define	_MENU_BARP_H
#ident	"@(#)debugger:libol/common/Menu_barP.h	1.3"

// toolkit specific members of the Menu_bar class
// included by ../../gui.d/common/Menu.h

struct Bar_data;	// defined in Menu.C

#define	MENUBAR_TOOLKIT_SPECIFICS	\
private:				\
	Bar_data	*list;	/* flat list table for menu bar */

#endif	// _MENU_BARP_H
