/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_TOOLKIT_H
#define	_TOOLKIT_H
#ident	"@(#)debugger:libol/common/Toolkit.h	1.13"

#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <Xol/OpenLook.h>
#include "Help.h"

#define	PADDING	4
#define DISMISS		"Close Window"
#define DISMISS_MNE	'C'

extern	Widget		base_widget;
extern	OlDefine	gui_mode;

void	busy_window(Widget, Boolean);
void	register_help(Widget, const char *title, Help_id);
void	register_button_help(Widget, int index, const char *title, 
		Help_id);
void	display_help(Widget, Help_mode, Help_id);
void	helpdesk_help(Widget);
int	get_widget_pad(Widget, Widget);
void	recover_notice();
void	eventCB(Widget, XtPointer, OlVirtualEventRec *e);

#endif // _TOOLKIT_H
