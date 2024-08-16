#ident	"@(#)debugger:gui.d/common/Queries.C	1.8"

// Debug headers
#include "Message.h"
#include "Msgtypes.h"
#include "Vector.h"
#include "gui_label.h"

// GUI headers
#include "Boxes.h"
#include "Component.h"
#include "Dialogs.h"
#include "Dispatcher.h"
#include "Sel_list.h"
#include "Stext.h"
#include "UI.h"
#include "Windows.h"
#include "Window_sh.h"

#include <string.h>
#include <stdio.h>

// Query_dialog handles debugger queries about overloaded functions
// It displays a scrolling list of the overloaded function names
// and asks the user to pick one

class Query_dialog : public Dialog_box
{
	Selection_list	*choices;
	DBcontext	dbcontext;
	Simple_text	*text;
	int		nfuncs;

public:
			Query_dialog();
			~Query_dialog() {}

			// button callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		select_all(Component *, void *);
	void		default_cb(Component *, int);

			// initialize the scrolling list
	void		set_list(Message *);
};

Query_dialog::Query_dialog() : DIALOG_BOX((Window_set *)windows.first())
{
	static const Button	buttons[] =
	{
		{ B_exec, LAB_select, LAB_select_mne, 
			(Callback_ptr)(&Query_dialog::apply), },
		{ B_exec, LAB_select_all, LAB_select_all_mne, 
			(Callback_ptr)(&Query_dialog::select_all), },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Query_dialog::cancel), },
		{ B_help, LAB_none, LAB_none, 0},
	};

	Expansion_box	*box;
	const char	*initial_string = "";

	dialog = new Dialog_shell(window_set->get_window_shell(),
		LAB_overload_func, 0, this, buttons,
		sizeof(buttons)/sizeof(Button), HELP_none);
	box = new Expansion_box(dialog, "box", OR_vertical);
	dialog->add_component(box);

	text = new Simple_text(box, "", TRUE);
	box->add_component(text);

	choices = new Selection_list(box, "function list", 10, 1, "%s", 1,
		&initial_string, SM_single, this,
		0, 0,
		(Callback_ptr)(&Query_dialog::default_cb));
	box->add_component(choices, TRUE);
	nfuncs = 0;
}

void
Query_dialog::set_list(Message *m)
{
	char		buf[BUFSIZ];
	char		*name;
	char		*functions;
	char		*fptr;
	char		*p;
	Vector		*v = vec_pool.get();
	Word		n;

	dbcontext = m->get_dbcontext();

	if (m->get_msg_id() == QUERY_which_function)
	{
		m->unbundle(name, functions);
		dialog->set_sensitive(LAB_select_all, FALSE);
	}
	else
	{
		// QUERY_which_func_or_all allows selection of
		// "all of the above functions"
		m->unbundle(name, functions, n);
		dialog->set_sensitive(LAB_select_all, TRUE);
	}

	sprintf(buf, gm_format(GM_overloaded), name);
	text->set_text(buf);

	// Create the list of functions.  This assumes the list is one string in
	// the form: number\tfunction\n ...
	v->clear();
	nfuncs = 0;
	p = functions;
	while (*p)
	{
		fptr = strchr(p, '\t') + 1;
		p = strchr(fptr, '\n');
		*p++ = '\0';
		v->add(&fptr, sizeof(char *));
		++nfuncs;
	}
	choices->set_list(nfuncs, (const char **)v->ptr());
	vec_pool.put(v);
}

void
Query_dialog::apply(Component *, void *)
{
	Vector	*v = vec_pool.get();

	if (!choices->get_selections(v))
	{
		dialog->error(E_ERROR, GE_no_function);
		vec_pool.put(v);
		return;
	}

	dispatcher.send_response(dbcontext, *(int *)v->ptr() + 1);
	vec_pool.put(v);
}

void
Query_dialog::default_cb(Component *,int item_index)
{
	dialog->default_start();
	dispatcher.send_response(dbcontext, item_index + 1);
	dialog->default_done();
}

void
Query_dialog::select_all(Component *, void *)
{
	dispatcher.send_response(dbcontext, nfuncs+1);
}

void
Query_dialog::cancel(Component *, void *)
{
	dispatcher.send_response(dbcontext, 0);
}

// called by Transport layer when CLI queries are received
void
query_handler(Message *m)
{
	static Query_dialog *query_box;

	switch(m->get_msg_id())
	{
	case QUERY_which_function:
	case QUERY_which_func_or_all:
		if (!query_box)
			query_box = new Query_dialog();
		query_box->set_list(m);
		query_box->display();
		break;

	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}
