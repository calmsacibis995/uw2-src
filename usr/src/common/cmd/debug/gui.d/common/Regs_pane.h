/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef REGS_PANE_H
#define REGS_PANE_H
#ident	"@(#)debugger:gui.d/common/Regs_pane.h	1.3"

// GUI headers
#include "Component.h"
#include "Panes.h"
#include "Reason.h"

// CLI headers
#include "Buffer.h"

class Base_window;
class Message;
class Pane_descriptor;
class Text_area;
class Window_set;

class Register_pane : public Pane
{
	Text_area	*reg_pane;
	Buffer		regs_buffer;
	Boolean		text_selected;
public:
			Register_pane(Window_set *ws, Base_window *parent, Box *,
				const Pane_descriptor *);
			~Register_pane();

			// component callbacks
	void		select_cb(Text_area *, int line);

			// callbacks
	void		update_cb(void *server, Reason_code, void *, ProcObj *);
	void		update_state_cb(void *server, Reason_code, void *, ProcObj *);

			// functions overriding virtuals in Pane base class
	void		popup();
	void		popdown();
	Selection_type	selection_type();
	void		copy_selection();

			// functions overriding virtuals in Command_sender base class
	int		check_sensitivity(int);
	void		de_message(Message *);
	void		cmd_complete();
};

#endif // REGS_PANE_H
