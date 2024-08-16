/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	MEM_DLG_H
#define	MEM_DLG_H
#ident	"@(#)debugger:gui.d/common/Mem_dlg.h	1.4"

#include "Component.h"
#include "Dialogs.h"

class Window_set;
class Message;
class ProcObj;

class Dump_dialog : public Process_dialog
{
	Text_line	*location;
	Text_line	*count;
	Text_area	*dump_pane;
public:
			Dump_dialog(Window_set *);
			~Dump_dialog() {};

			// callbacks
	void		do_dump(Component *, void *);
	
	void		set_location(const char *);
	void		clear();

			// functions overriding those of Dialog_box
	void		de_message(Message *);
	void		update_obj(ProcObj *);
};

class Map_dialog : public Process_dialog
{
	Text_area	*map_pane;
public:
			Map_dialog(Window_set *);
			~Map_dialog() {};

	void		do_map();
	void		update_obj(ProcObj *);
};

#endif // MEM_DLG_H
