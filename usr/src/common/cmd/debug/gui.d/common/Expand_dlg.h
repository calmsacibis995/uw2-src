/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef EXPAND_DLG_H
#define EXPAND_DLG_H
#ident	"@(#)debugger:gui.d/common/Expand_dlg.h	1.5"

// GUI headers
#include "Dialogs.h"
#include "Component.h"

// DEBUG headers
#include "Vector.h"
#include "Link.h"

class Buffer;
class Window_set;
class Message;
class ProcObj;

// The Expand_dialog displays the contents of a structure or array in result.
// The user can further expand displayed pointers by selecting the line
// containing the pointer and pushing the Expand button.  The Expand dialog
// uses the expansion_vector to track which lines contain pointers that can
// be expanded.  Since the information displayed is not necessarily flat,
// but may contain nested structures and arrays, the expansion_vector is a
// tree which may contain pointers to other vectors.  The stack is used to
// build up the tree as the messages are read from the debugger.
// As pointers are expanded, the expressions are stored in the expression_vector,
// so the previous expression can be retrieved easily, instead of having to
// be built up again from scratch when Collapse is pushed.
// As the messages making up the output of the print statement are read
// from the debugger, the text is stored in resbuf, and only written to
// result at the end, to avoid excessive traffic to the screen

struct Expansion_stack : public Stack
{
	Vector		*v;
};

class Expand_dialog : public Process_dialog
{
	Simple_text	*expression;
	Text_area	*result;
	char		*expr;
	int		expanded;
	Vector		expression_vector;
	Vector		*expansion_vector;
	Expansion_stack	stack;
	Buffer		*resbuf;
	int		line_cnt;
	char		*type_name;
	Vector		type_name_vector;

	char		*CC_expression(const char *expression);
	int		CC_member_name(int, Buffer *, Vector *, int indirection);

	void		clear_expansion(Vector *);
	void		add_text(const char *);
	int		pointer_ok(const char *expr);

public:
			Expand_dialog(Window_set *);
			~Expand_dialog() {};

	char		*make_expression(const char *expression);
	void		set_expression(const char *name);

			// callbacks
	void		expand(Component *, void *);
	void		collapse(Component *, void *);
	void		dismiss(Component *, void *);
	void		selection_cb(Text_area *, void *);

			// functions overriding those from Dialog_box
	void		de_message(Message *);
	void		cmd_complete();
	void		update_obj(ProcObj *);
	void		set_obj(Boolean);
};

#endif
