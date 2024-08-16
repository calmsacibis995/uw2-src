#ident	"@(#)debugger:gui.d/common/Expand_dlg.C	1.13"

// GUI headers
#include "Boxes.h"
#include "Caption.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Expand_dlg.h"
#include "Proclist.h"
#include "Stext.h"
#include "Text_area.h"
#include "UI.h"
#include "Windows.h"
#include "Window_sh.h"
#include "gui_label.h"

// DEBUG headers
#include "Buffer.h"
#include "Machine.h"
#include "Msgtab.h"
#include "Vector.h"
#include "str.h"

#include <stdio.h>

// The contents of the expansion_vector is an array of Expansion_data structures,
// each of which represents one member of the class or array displayed in the
// result area.  The type - ET_class_member or ET_array_member - determines
// what is stored in the union.  If the member is itself a class or array,
// sub_type will point to another array of Expansion_data

enum Expansion_type { ET_none, ET_class_member, ET_array_member };

struct Expansion_data
{
	Expansion_type	type;
	long		line;
	Vector		*sub_type;
	Boolean		is_null;

	union
	{
		char	*name;
		long	index;
	};

			Expansion_data() { type = ET_none; line = 0; name = 0;
						sub_type = 0; is_null = FALSE; }
};

// The Expand button in the dialog will be insensitive until the user makes
// a selection that can be expanded.  Collapse will be insensitive until
// the user expands something.
// The Expand dialog is only accessible when a single process or thread is
// selected, or if nothing is selected, applies to the current process or
// thread.

Expand_dialog::Expand_dialog(Window_set *ws) : PROCESS_DIALOG(ws)
{
	static const Button	buttons[] =
	{
		{ B_non_exec, LAB_expand, LAB_expand_mne,
			(Callback_ptr)(&Expand_dialog::expand) },
		{ B_non_exec, LAB_collapse, LAB_collapse_mne,
			(Callback_ptr)(&Expand_dialog::collapse) },
		{ B_close, LAB_none, LAB_none, 0 },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Expansion_box	*box;
	Caption		*caption;

	dialog = new Dialog_shell(ws->get_window_shell(),
		LAB_expand, (Callback_ptr)(&Expand_dialog::dismiss), this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_expand_dialog);
	box = new Expansion_box(dialog, "expand", OR_vertical);
	dialog->add_component(box);
	component_init(box);

	caption = new Caption(box, LAB_expression_line, CAP_LEFT);
	expression = new Simple_text(caption, "", TRUE);
	caption->add_component(expression);
	box->add_component(caption);

	caption = new Caption(box, LAB_result_of_expr_line, CAP_TOP_LEFT);
	result = new Text_area(caption, "result", 8, 30, FALSE,
		(Callback_ptr)(&Expand_dialog::selection_cb), this);
	caption->add_component(result);
	box->add_component(caption, TRUE);

	expanded = 0;
	resbuf = 0;
	line_cnt = 0;
	expr = 0;
	expansion_vector = 0;
	type_name = 0;
	dialog->set_sensitive(LAB_collapse, FALSE);
	dialog->set_sensitive(LAB_expand, FALSE);
}

void
Expand_dialog::dismiss(Component *, void *)
{
	clear_expansion(expansion_vector);
	expansion_vector = 0;
	dismiss_cb(0, 0);
}

// set_expression is called when the dialog is first popped up,
// to initialize the expression and result
void
Expand_dialog::set_expression(const char *name)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	clear_expansion(expansion_vector);
	expansion_vector = 0;
	expression_vector.clear();
	expanded = 0;
	type_name_vector.clear();
	delete type_name;
	type_name = 0;

	expr = make_expression(name);
	if (!expr)
	{
		expression->set_text(name);
		return;
	}

	expression->set_text(expr);
	expression_vector.add(&expr, sizeof(char *));

	dispatcher.send_msg(this, pobjs[0]->get_id(),
		"print %s\n", expr);
	dialog->set_sensitive(LAB_collapse, FALSE);
	dialog->wait_for_response();
}

// Before the user can push the Expand button, selection_cb verifies
// that the selection is expandable, and saves the full expression
// needed to expand it in expr.  expand saves the expression in the
// expression_vector and issues the command to the debugger to get the contents
void
Expand_dialog::expand(Component *, void *)
{
	dialog->clear_msg();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	if (!expr)
		return;

	expanded++;
	expression->set_text(expr);
	expression_vector.add(&expr, sizeof(char *));
	type_name_vector.add(&type_name, sizeof(char *));
	type_name = 0;

	dispatcher.send_msg(this, pobjs[0]->get_id(),
		"print %s\n", expr);
	dialog->wait_for_response();
	clear_expansion(expansion_vector);
	expansion_vector = 0;
	dialog->set_sensitive(LAB_collapse, TRUE);
}

static void
do_clear(Vector *vec)
{
	// assert( vec != 0 );
	Expansion_data	*ptr = (Expansion_data *)vec->ptr();
	int		nentries = vec->size()/sizeof(Expansion_data);

	for (int i = 0; i < nentries; i++, ptr++)
	{
		if (ptr->type == ET_class_member)
			delete ptr->name;
		if (ptr->sub_type)
			do_clear(ptr->sub_type);
	}
	delete vec;
}

// clear_expansion frees the space used by the expansion_vector and
// its sub-vectors
void
Expand_dialog::clear_expansion(Vector *vec)
{
	if (vec)
		do_clear(vec);

	result->clear();
	if (resbuf)
	{
		buf_pool.put(resbuf);
		resbuf = 0;
	}
	// first line contains a '{'
	line_cnt = 1;
}

// make_expression builds up the expression in the current language
// needed to dereference the selected member
// if the selected text can't be expanded, make_expression
// will display an error message and return 0
char *
Expand_dialog::make_expression(const char *expression)
{
	Language	lang = cur_language;

	if (cur_language == UnSpec)
	{
		Message	*msg;

		dispatcher.query(this, pobjs[0]->get_id(),
			"print %%db_lang\n");
		while ((msg = dispatcher.get_response()) != 0)
		{
			if (msg->get_msg_id() == MSG_print_val)
			{
				char *l;
				msg->unbundle(l);
				if (strcmp(l, "\"C\"\n") == 0)
					lang = C;
				else if (strcmp(l, "\"C++\"\n") == 0)
					lang = CPLUS;
			}
		}
	}

	switch(lang)
	{
	case C:
	case CPLUS:
		return CC_expression(expression);

	case UnSpec:
	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return NULL;
	}
}

// CC_expression builds up an expression from the previous expression
// and the current selection, and checks that the resulting expression
// is valid and can be dereferenced
char *
Expand_dialog::CC_expression(const char *expression)
{
	Buffer	*buf = buf_pool.get();
	char	*selection = result->get_selection();
	char	*string;
	int	indirection = (*expression == '*');

	if (indirection)
		expression++;
	buf->clear();
	if (type_name)
	{
		// type_name is set only when printing a derived class 
		// when given the base class ptr
		buf->add("((");
		buf->add(type_name);
		buf->add("*)");
		buf->add(expression);
		buf->add(')');
	}
	else
		buf->add(expression);

	if (selection && *selection)
	{
		int	line = result->get_current_position();

		if (!CC_member_name(line, buf, expansion_vector, indirection))
		{
			buf_pool.put(buf);
			dialog->error(E_ERROR, GE_cant_expand);
			return 0;
		}
	}
	else if (expansion_vector)
	{
		// if this is a class, can't be expanded further without selecting
		// a member
		dialog->error(E_ERROR, GE_cant_expand);
		buf_pool.put(buf);
		return 0;
	}

	dispatcher.query(this, pobjs[0]->get_id(),
		"whatis %s\n", (char *)*buf);

	Message	*msg;
	char	*type_string = 0;
	while ((msg = dispatcher.get_response()) != 0)
	{
		Msg_id	mtype = msg->get_msg_id();

		if (mtype == MSG_type)
		{
			msg->unbundle(type_string);
		}
		else if (mtype == MSG_type_derived)
		{
			char	*tmp;
			msg->unbundle(type_string, tmp);
		}
	}

	string = 0;
	if (type_string)
	{
		if (strcmp(type_string, "void *") == 0)
		{
			dialog->error(E_ERROR, GE_ptr_to_void);
		}
		else if (type_string[strlen(type_string)-1] == '*')
		{
			if (expression_vector.size() || pointer_ok((char *)*buf))
				string = makesf("*%s", (char *)*buf);
		}
		else if (!expression_vector.size())
		{
			// when first popped up, it is ok to display the value
			// of a simple variable
			string = makestr((char *)*buf);
		}
		else
		{
			dialog->error(E_ERROR, GE_cant_expand);
		}
	}
	else
	{
		dialog->error(E_ERROR, GE_cant_expand);
	}
	buf_pool.put(buf);
	return string;
}

static void
add_member(Buffer *buf, Expansion_data *ptr, int indirection)
{
	char		number[MAX_LONG_DIGITS+3];

	if (ptr->type == ET_class_member)
	{
		if (indirection)
			buf->add("->");
		else
			buf->add('.');
		buf->add(ptr->name);
	}
	else
	{
		sprintf(number, "[%d]", ptr->index);
		buf->add(number);
	}
}

// CC_member_name builds up the string of ->, ., and [] operators needed
// to reference the selected member
int
Expand_dialog::CC_member_name(int line, Buffer *buf, Vector *vector, int indirection)
{
	if (!vector)
		return 0;

	Expansion_data	*ptr = (Expansion_data *)vector->ptr();
	int		nentries = vector->size()/sizeof(Expansion_data);

	for (int i = 0; i < nentries; i++, ptr++)
	{
		if (ptr->sub_type)
		{
			if (i < nentries-1 && line >= (ptr+1)->line)
				continue;

			if (line > ptr->line)
			{
				// line is in this subtree
				add_member(buf, ptr, indirection);
				return CC_member_name(line, buf, ptr->sub_type, 0);
			}
			else
				return 0;
		}

		if (line == ptr->line)
		{
			if (ptr->is_null)
			{
				dialog->error(E_ERROR, GE_deref_null);
				return 0;
			}
			add_member(buf, ptr, indirection);
			return 1;
		}
		if (line < ptr->line)
			// line not found?
			return 0;
	}

	return 0;
}

void
Expand_dialog::de_message(Message *m)
{
	Expansion_data	data;
	Expansion_stack	*es;
	Msg_id		mtype = m->get_msg_id();
	char		*name;
	char		*spaces;
	char		*val;
	char		*prefix;
	Word		index;

	if (!resbuf)
	{
		resbuf = buf_pool.get();
		resbuf->clear();
	}

	switch (mtype)
	{
	case ERR_assume_size:
		dialog->error(m);
		return;

	case MSG_start_class:
	case MSG_start_array:
		// If expansion_vector is already set, this indicates that there is
		// a nested structure or array.  The current expansion_vector is
		// pushed onto the stack, and a new expansion_vector is built for
		// the sub-aggregate, until the matching MSG_val_close_brack
		// is received
		// Note: the only purpose of the stack is to keep track of the
		//  nesting levels
		if (expansion_vector)
		{
			Expansion_data	*ptr = (Expansion_data *)expansion_vector->ptr();
			int		index = expansion_vector->size()/sizeof(data) - 1;

			Vector	*vector = new Vector();

			ptr[index].sub_type = vector;

			es = new Expansion_stack;
			es->v = expansion_vector;
			stack.push((Link *)es);
			expansion_vector = vector;
		}
		else	
			expansion_vector = new Vector;
		break;

	case MSG_val_close_brack:
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		if (!stack.is_empty())
		{
			es = (Expansion_stack *)stack.pop();
			expansion_vector = es->v;
		}
		break;

	case MSG_print_member1:
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		m->unbundle(spaces, prefix, name, val);
		data.name = makesf("%s%s", prefix, name);
		data.line = line_cnt;
		data.type = ET_class_member;
		if (strcmp(val, "0x0") == 0 || strcmp(val, "NULL") == 0)
			data.is_null = TRUE;
		expansion_vector->add(&data, sizeof(data));
		break;
	case MSG_print_member2:
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		m->unbundle(spaces, name, val);
		data.name = makestr(name);
		data.line = line_cnt;
		data.type = ET_class_member;
		if (strcmp(val, "0x0") == 0 || strcmp(val, "NULL") == 0)
			data.is_null = TRUE;
		expansion_vector->add(&data, sizeof(data));
		break;

	case MSG_array_member:
		if (!expansion_vector)
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
		m->unbundle(spaces, index, val);
		data.type = ET_array_member;
		data.index = (long)index;
		data.line = line_cnt;
		if (strcmp(val, "0x0") == 0 || strcmp(val, "NULL") == 0)
			data.is_null = TRUE;
		expansion_vector->add(&data, sizeof(data));
		break;

	case MSG_type:
		m->unbundle(name);
		type_name = makestr(name);
		break;
		
	default:
		if (Mtable.msg_class(mtype) == MSGCL_error)
		{
			show_error(m, TRUE);
			return;
		}
		break;
	}

	add_text(m->format());
}

void
Expand_dialog::add_text(const char *string)
{
	resbuf->add(string);

	char	c;
	while ((c = *string++) != '\0')
	{
		if (c == '\n')
			line_cnt++;
	}
}

void
Expand_dialog::cmd_complete()
{
	result->add_text((char *)*resbuf);
	result->position(1);	// scroll back to top of text
	buf_pool.put(resbuf);
	resbuf = 0;

	if (!stack.is_empty())
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		while (!stack.is_empty())
			(void) stack.pop();
	}
	dialog->cmd_complete();
	dialog->set_sensitive(LAB_expand, FALSE);
}

// collapse pops the stack of expressions, and redisplays the previous
// expression
void
Expand_dialog::collapse(Component *, void *)
{
	if (!expanded)
		return;

	dialog->clear_msg();
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	delete expr;
	expression_vector.drop(sizeof(char *));

	expanded--;
	expr = ((char **)expression_vector.ptr())[expanded];
	expression->set_text(expr);
	delete type_name;
	type_name = ((char **)type_name_vector.ptr())[expanded];
	type_name_vector.drop(sizeof(char *));

	dispatcher.send_msg(this, pobjs[0]->get_id(),
		"print %s\n", expr);
	dialog->wait_for_response();
	clear_expansion(expansion_vector);
	expansion_vector = 0;
	if (!expanded)
		dialog->set_sensitive(LAB_collapse, FALSE);
}

// Don't let the user try to dereference a null pointer
int
Expand_dialog::pointer_ok(const char *str)
{
	dispatcher.query(this, pobjs[0]->get_id(),
		"print %s\n", str);

	Message	*msg;
	int	ret = 1;
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_print_val)
		{
			char	*ptr_value;
			msg->unbundle(ptr_value);
			if (strcmp(ptr_value, "0x0\n") == 0
				|| strcmp(ptr_value, "NULL\n") == 0)
			{
				dialog->error(E_ERROR, GE_deref_null);
				ret = 0;
			}
		}
		else if (Mtable.msg_class(msg->get_msg_id()) == MSGCL_error)
		{
			dialog->error(msg);
			ret = 0;
		}
	}
	return ret;
}

void
Expand_dialog::set_obj(Boolean reset)
{
	if (reset)
		dialog->set_sensitive(FALSE);
}

// Whenever the current object stops, update the result pane to
// show the new value of the expression
void
Expand_dialog::update_obj(ProcObj *p)
{
	if (!p->is_running() && !p->is_animated())
	{
		dispatcher.send_msg(this, pobjs[0]->get_id(),
			"print %s\n",
			((char **)expression_vector.ptr())[expanded]);
		dialog->wait_for_response();
		dialog->clear_msg();
		clear_expansion(expansion_vector);
		expansion_vector = 0;
	}
}

// set the sensitivity on the expand button whenever the user selects
// or deselects text in the result pane
void
Expand_dialog::selection_cb(Text_area *, void *cdata)
{
	if ((int)cdata)
	{
		// selection
		dialog->clear_msg();
		if (!pobjs)
		{
			dialog->error(E_ERROR, GE_selection_gone);
			return;
		}

		expr = make_expression(((char **)expression_vector.ptr())[expanded]);
		if (expr)
		{
			dialog->set_sensitive(LAB_expand, TRUE);
			return;
		}
	}

	// no selection, or unexpandable selection
	dialog->set_sensitive(LAB_expand, FALSE);
}
