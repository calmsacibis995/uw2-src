/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_SCH_DLG_H
#define _SCH_DLG_H
#ident	"@(#)debugger:gui.d/common/Sch_dlg.h	1.4"

#include "Component.h"
#include "Dialogs.h"

class Base_window;
class Source_pane;
class Disassembly_pane;
class Radio_list;

class Search_dialog : public Dialog_box
{
	Text_line	*string;
	char		*save_string;
	Source_pane	*source;
	Disassembly_pane	*dis;
	Radio_list	*radio;
	int		choice;
public:
			Search_dialog(Base_window *);
			~Search_dialog()	{ delete save_string; }

			// callbacks
	void		apply(Component *, int mnemonic);
	void		cancel(Component *, void *);

			// initialization routines
	void		set_string(const char *);
};

#endif	// _SCH_DLG_H
