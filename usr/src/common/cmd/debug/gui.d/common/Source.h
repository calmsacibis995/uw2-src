/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _SOURCE_H
#define _SOURCE_H
#ident	"@(#)debugger:gui.d/common/Source.h	1.12"

#include "Component.h"
#include "Eventlist.h"
#include "Sender.h"
#include "Panes.h"
#include "Reason.h"

class Menu_bar_table;
class ProcObj;
class Status_pane;
class Show_function_dialog;
class Show_line_dialog;
class Open_dialog;
struct Pane_descriptor;

class Source_pane : public Pane
{
	Caption			*caption;
	Text_display		*pane;
	File_list		*file_ptr;	// ptr to breakpoint info for current file
	Flink			*flink;
	const char		*current_file;
	char			*current_path;
	int			current_line;
	int			selected_line;
	const Pane_descriptor	*pdesc;
	Boolean			primary;	// primary source window for
						// the window set
	Boolean			registered;
	Boolean			animated;

	Show_function_dialog	*show_function;
	Show_line_dialog	*show_line;
	Open_dialog		*open_box;

	Boolean			set_path(Boolean must_set);
	void			clear();
public:
				Source_pane(Window_set *, Base_window *, Box *,
					const Pane_descriptor *, Boolean is_primary);
				~Source_pane();

				// access functions
	const char		*get_current_file()	{ return current_file; }
	int			get_selected_line()	{ return selected_line; }
	Text_display		*get_pane()		{ return pane; }
	void			set_current_line(int l)	{ current_line = l; }
	void			animate(Boolean a) 	{ animated = a; }
	Boolean			is_animated()		{ return animated; }

				// framework callbacks
	void			update_cb(void *, Reason_code, void *, ProcObj *);
	void			update_state_cb(void *, Reason_code, void *, ProcObj *);
	void			break_list_cb(void *, Reason_code_break, void *, Stop_event *);

				// component callbacks
	void			toggle_break_cb(Component *, void *);
	void			set_break_cb(Component *, void *);
	void			delete_break_cb(Component *, void *);
	void			ok_to_delete(Component *, int);
	void			show_function_cb(Component *, void *);
	void			show_line_cb(Component *, void *);
	void			open_dialog_cb(Component *, void *);
	void			new_source_cb(Component *, void *);
	void			select_cb(Text_display *, int line);

	void			set_file(const char *file, const char *path = 0);
	void			de_message(Message *m);

				// functions overriding virtuals in Pane base class
	void			popup();
	void			popdown();
	char			*get_selection();
	void			copy_selection();
	Selection_type		selection_type();
	int			check_sensitivity(int sense);

};

#endif	// _SOURCE_H
