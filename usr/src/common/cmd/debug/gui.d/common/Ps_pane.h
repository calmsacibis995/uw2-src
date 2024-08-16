/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _PS_PANE_H
#define _PS_PANE_H
#ident	"@(#)debugger:gui.d/common/Ps_pane.h	1.9"

#include "Base_win.h"
#include "Component.h"
#include "Windows.h"
#include "Panes.h"
#include "Reason.h"

class Base_window;
class Pane_descriptor;
class Process;
class Table_calldata;
class Vector;

class Ps_pane : public Pane
{
	Table		*pane;
	Window_set	*window_set;
	int		total_selections;
public:
			Ps_pane(Window_set *, Base_window *, Box *box, Pane_descriptor *);
			~Ps_pane();

			// access functions
	Table		*get_table()	{ return pane; }

			// functions to update the displayed information,
			// called by Window_set
	void		add_process(int index, Process *);
	void		add_obj(int index, ProcObj *);
	void		set_current(ProcObj *);
	void		update_obj(Reason_code, int index, ProcObj *);
	void		delete_obj(int index);

			// callbacks
	void		select_cb(Table *, void *);
	void		deselect_cb(Table *, void *);
	void		default_cb(Table *, const Table_calldata *);
	void		drop_proc(Table *, const Table_calldata *);

			// functions overriding virtuals in Pane base class
	Selection_type	selection_type();
	int		get_selections(Vector *);
	void		deselect();
	int		check_sensitivity(int sense);
};

#endif	// _PS_PANE_H
