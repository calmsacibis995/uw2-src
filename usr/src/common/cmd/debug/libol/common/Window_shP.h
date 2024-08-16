/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_WINDOW_SHP_H
#define _WINDOW_SHP_H
#ident	"@(#)debugger:libol/common/Window_shP.h	1.4"

// toolkit specific members of the Window_shell class
// included by ../../gui.d/common/Window_sh.h

#if 1
#include "List.h"
#endif

// toolkit specific members:
// 	busy_count
// 	focus
//	window_widget	primary widget, widget in base class is form widget			// 	msg_widget	static text widget for messages in footer
// 	string		text displayed in footer
// 	errors		number of errors generated by last action
// 	shadow		shadow thickness, needed to calculate padding

#define WINDOW_SHELL_TOOLKIT_SPECIFICS	\
	Boolean		first_busy;	\
	List		win_list;	\
	Component	*focus;		\
	Widget		window_widget;	\
	Widget		msg_widget;	\
	char		*string;	\
	int		errors;		\
	Dimension	shadow;
				
#endif // WINDOW_SHP_H