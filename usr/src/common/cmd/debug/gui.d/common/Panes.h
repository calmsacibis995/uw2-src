/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	PANES_H
#define	PANES_H
#ident	"@(#)debugger:gui.d/common/Panes.h	1.1"

#include "Sender.h"
#include "Component.h"

class Base_window;
class Vector;

// cfront 2.1 requires class name in constructor, 1.2 doesn't accept it
#ifdef __cplusplus
#define PANE	Pane
#else
#define PANE
#endif

enum Pane_type {
	PT_disassembler,
	PT_event,
	PT_process,
	PT_registers,
	PT_source,
	PT_stack,
	PT_status,
	PT_symbols,
	PT_command,
	PT_last,
};

class Menu_bar_table;

class Pane : public Command_sender
{
protected:
	Base_window		*parent;
	Pane_type		type;

public:
				Pane(Window_set *ws, Base_window *base, Pane_type t)
					: COMMAND_SENDER(ws)
					{ parent = base; type = t; }
	virtual			~Pane() {}

				// access functions
	Pane_type		get_type()		{ return type; }
	Base_window		*get_parent()		{ return parent; }

	virtual void		popup();
	virtual void		popdown();
	virtual char		*get_selection();
	virtual int		get_selections(Vector *);
	virtual void		copy_selection();
	virtual Selection_type	selection_type();
	virtual void		deselect();
	virtual	int		check_sensitivity(int sense);
};

#endif // PANES_H
