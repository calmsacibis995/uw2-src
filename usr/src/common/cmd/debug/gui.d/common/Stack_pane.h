/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _STACK_PANE_H
#define _STACK_PANE_H
#ident	"@(#)debugger:gui.d/common/Stack_pane.h	1.8"

#include "Component.h"
#include "Windows.h"
#include "Panes.h"

enum Stack_cell { ST_current, ST_frame, ST_func, ST_params, ST_loc };

class Base_window;
class Message;
class ProcObj;
class Divided_box;
struct Pane_descriptor;

class Stack_pane : public Pane
{
	Table		*pane;
	int		top_frame;
	int		cur_frame;
	int		next_row;
	int		has_selection;
	char		*func;
	int		update_in_progress;
	Boolean		update_abort;
public:
			Stack_pane(Window_set *, Base_window *, Box *,
				const Pane_descriptor *);
			~Stack_pane();

			// access functions
	Table		*get_table()	{ return pane; }
	int		get_frame(int frame, const char *&func, const char *&loc);
	void		reset();

			// callbacks
	void		update_cb(void *server, Reason_code, void *, ProcObj *);
	void		update_state_cb(void *server, Reason_code, void *, ProcObj *);
	void		select_frame(Table *, void *);
	void		deselect_frame(Table *, int);
	void		set_current();
	void		default_action(Table *, Table_calldata *);

			// functions inherited from Command_sender
	void		de_message(Message *);
	void		cmd_complete();

			// functions overriding virtuals in Pane base class
	void		deselect();
	Selection_type	selection_type();
	void		popup();
	void		popdown();
	int		check_sensitivity(int sense);
};

#endif	// _STACK_PANE_H
