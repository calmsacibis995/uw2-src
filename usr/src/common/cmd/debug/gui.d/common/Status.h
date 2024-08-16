/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_STATUS_H
#define	_STATUS_H
#ident	"@(#)debugger:gui.d/common/Status.h	1.6"

// GUI headers
#include "Panes.h"
#include "Reason.h"

class Expansion_box;
class ProcObj;
class Base_window;
class Window_set;
class Table;

class Status_pane : public Pane
{
	Table		*pane;

public:
			Status_pane(Window_set *ws, Base_window *parent, Box *);

			// callbacks
	void		update_cb(void *server, Reason_code, void *, ProcObj *);

			// functions overriding virtuals in Pane base class
	void		popup();
	void		popdown();
};

#endif	// _STATUS_H
