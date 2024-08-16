/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	BASE_WIN_H
#define	BASE_WIN_H
#ident	"@(#)debugger:gui.d/common/Base_win.h	1.5"

#include "Component.h"
#include "Sender.h"
#include "Reason.h"
#include "Panes.h"

class Window_descriptor;
class Button_bar;
class Menu_bar;
class Pane;
class Vector;
class ProcObj;
class Panes_dialog;
class Search_dialog;

// flags
#define	BW_IS_OPEN		1
#define BW_SET_LINE_EXCLUSIVE	2

class Base_window : public Command_sender
{
protected:
	Window_shell		*window;
	Menu_bar		*menu_bar;
	Button_bar		*button_bar;
	unsigned char		flags;
	unsigned char		busy_count;
	unsigned char		sense_count; // for set_sensitivity
	unsigned char		set_line_count; // for Text_disp::set_line

	Pane 			*selection_pane;
	Pane			*panes[PT_last];

	const Window_descriptor	*win_desc;
	Panes_dialog		*panes_dialog;
	Search_dialog		*search_dialog;

	int			check_sensitivity(int sense);
public:
				Base_window(Window_set *, const Window_descriptor *);
				~Base_window();

				// access functions
	Window_shell		*get_window_shell()	{ return window; }
	Menu_bar		*get_menu_bar()		{ return menu_bar; }
	Boolean			is_open()		{ return (flags & BW_IS_OPEN); }
	Pane			*get_selection_pane()	{ return selection_pane; }
	Pane			*get_pane(Pane_type pt) {return panes[pt]; }
	void			set_sense_count(int c)  { sense_count = (unsigned char)c; }
	int			get_sense_count()  	{ return sense_count; }
	int			get_set_line_count()  	{ return set_line_count; }
	void			set_set_line_count(int c) { set_line_count = c; }
				// functions inherited from Command_sender
	void			de_message(Message *);

				// display functions
	void			popup(Boolean focus = TRUE);
	void			popdown();

				//callbacks
	void			update_state_cb(void *, Reason_code, void *, ProcObj *);

	void			inc_busy();
	void			dec_busy();
	void			set_sensitivity();
	int			is_sensitive(int sense);
	void			setup_panes_cb(Component *, void *);
	void			copy_cb(Component *, void *);
	void			set_break_cb(Component *, void *);
	void			delete_break_cb(Component *, void *);
	void			search_dialog_cb(Component *, void *);
	void 			help_sect_cb(Component *, void * );

				// selection handling routines
	Selection_type		selection_type();
	void			set_selection(Pane *);
	char			*get_selection();
	int			get_selections(Vector *);
};

#endif // BASE_WIN_H
