/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _DIS_H
#define _DIS_H
#ident	"@(#)debugger:gui.d/common/Dis.h	1.14"

#include "Component.h"
#include "Eventlist.h"
#include "Sender.h"
#include "Iaddr.h"
#include "Vector.h"
#include "Buffer.h"
#include "Panes.h"
#include "Windows.h"
#include "Reason.h"

class Menu_bar_table;
class ProcObj;
class Function_dialog;
class Show_loc_dialog;
struct Pane_descriptor;

class Disassembly_pane : public Pane
{
	Caption			*caption;
	Text_display		*pane;
	char			*current_func;
	char			*current_loc;
	int			current_line;
	int			selected_line;
	Vector			locations;
	Buffer			dis_buffer;
	Iaddr			start_addr;
	Iaddr			end_addr;

	Show_loc_dialog		*show_loc;
	Function_dialog		*show_function;
	Boolean			animated;

	void			clear();
	int			has_inst(const char *);
	int			has_inst(Iaddr address);
	void			disasm_func(const char *);
	void			set_display_line();
public:
				Disassembly_pane(Window_set *, Base_window *, Box *,
					const Pane_descriptor *);
				~Disassembly_pane();

				// access functions
	int			get_selected_line()	{ return selected_line; }
	Text_display		*get_pane()		{ return pane; }
	void			animate(Boolean a)	{ animated = a; }
	Boolean			is_animated()		{ return animated; }

				// framework callbacks
	void			update_cb(void *, Reason_code, void *, ProcObj *);
	void			update_state_cb(void *, Reason_code, void *, ProcObj *);
	void			break_list_cb(void *, Reason_code_break, void *, Stop_event *);

				// component callbacks
	void			select_cb(Text_display *, int line);
	void			toggle_break_cb(Component *, void *);
	void			set_break_cb(Component *, void *);
	void			delete_break_cb(Component *, void *);
	void			ok_to_delete(Component *, int);
	void			show_function_cb(Component *, void *);
	void			show_loc_cb(Component *, void *);
	void			copy_cb(Component *, void *);

				// functions overriding ones in Pane base class
	void			popup();
	void			popdown();
	Selection_type		selection_type();
	char			*get_selection();
	void			copy_selection();

	int			check_sensitivity(int sense);

	void			disasm_set_func(const char *);
	void			disasm_addr(Iaddr, char *);
	const char		*get_selected_addr();

				// functions overriding ones in Command_sender base class
	void			de_message(Message *);
	void			cmd_complete();
};

#endif	// _DIS_H
