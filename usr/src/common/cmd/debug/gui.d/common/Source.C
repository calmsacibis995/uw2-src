#ident	"@(#)debugger:gui.d/common/Source.C	1.64"

// GUI headers
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Eventlist.h"
#include "Source.h"
#include "Status.h"
#include "Windows.h"
#include "Window_sh.h"
#include "Base_win.h"
#include "Menu.h"
#include "Text_disp.h"
#include "Text_line.h"
#include "Panes.h"
#include "Radio.h"
#include "Sel_list.h"
#include "Stext.h"
#include "Boxes.h"
#include "Caption.h"
#include "Proclist.h"
#include "UI.h"
#include "config.h"
#include "gui_label.h"

// Debug headers
#include "Message.h"
#include "Msgtab.h"
#include "Vector.h"
#include "str.h"

#include <stdio.h>
#include <stdlib.h>

class Open_dialog : public Process_dialog
{
	Source_pane	*source;
	Selection_list	*files;
	int		selection;	// state saved for cancel

	void		do_it(Source_pane *);
public:
			Open_dialog(Source_pane *);
			~Open_dialog() {}

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		default_cb(Component *, int);
	void		drop_cb(Selection_list *, Component *);

			// get list of files from debug
	int		get_files(Vector *);
	void		open_file(Source_pane *,int);

			// inherited from Process_dialog
	void		set_obj(Boolean reset);
};

Open_dialog::Open_dialog(Source_pane *sw) : PROCESS_DIALOG(sw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply, LAB_open, LAB_open_mne,
			(Callback_ptr)(&Open_dialog::apply) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Open_dialog::cancel) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Expansion_box	*box;
	const char	*initial_string = "";

	selection = -1;
	source = sw;

	dialog = new Dialog_shell(window_set->get_window_shell(), LAB_open,
		(Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_open_dialog);
	box = new Expansion_box(dialog, "box", OR_vertical);
	dialog->add_component(box);
	component_init(box);

	files = new Selection_list(box, "files", 7, 1, "%s", 1,
		&initial_string, SM_single, this, 0, 0, 
		(Callback_ptr)(&Open_dialog::default_cb),
		(Callback_ptr)(&Open_dialog::drop_cb));
	box->add_component(files, TRUE);
}

// get list of files from debug
int
Open_dialog::get_files(Vector *vscratch)
{
	Message	*msg;
	char	*fname;
	int	nfiles;

	nfiles = 0;
	vscratch->clear();

	dispatcher.query(this, window_set->current_obj()->get_id(), "pfiles\n");
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() != MSG_source_file)
		{
			display_msg(msg);
			continue;
		}
		msg->unbundle(fname);
		fname = str(fname);
		vscratch->add(&fname, sizeof(char *));
		nfiles++;
	}

	qsort((char **)vscratch->ptr(), nfiles, sizeof(char *), alpha_comp);
	return nfiles;
}

// update the list of files when the current process changes
void
Open_dialog::set_obj(Boolean reset)
{
	Vector	*flist;
	int	nfiles;

	if (!pobjs)
	{
		files->set_list(0,0);
		return;
	}

	if (reset)
		dialog->set_busy(TRUE);

	flist = vec_pool.get();
	if ((nfiles = get_files(flist)) == 0)
		dialog->error(E_ERROR, GE_no_source, pobjs[0]->get_name());
	else
		files->set_list(nfiles, (const char **)flist->ptr());
	vec_pool.put(flist);

	selection = -1;
	if (reset)
		dialog->set_busy(FALSE);
}

// open the selected file in the given source window.  if sp is null,
// (meaning the selection was dropped on the workspace) create a new
// source window
void
Open_dialog::do_it(Source_pane *sp)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;
	}

	Vector	*v = vec_pool.get();
	if (files->get_selections(v) == 0)
	{
		dialog->error(E_ERROR, GE_file_needed);
		vec_pool.put(v);
		return;
	}
	selection = *(int *)v->ptr();
	vec_pool.put(v);

	open_file(sp, selection);
}

void
Open_dialog::open_file(Source_pane *sp, int sel)
{
	Message		*msg;
	const char	*fname;
	char		*path = 0;
	Base_window	*sw;

	fname = files->get_item(sel, 0);
	dispatcher.query(this, pobjs[0]->get_id(), "ppath %s\n", fname);
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() != MSG_source_file)
		{
			display_msg(msg);
			continue;
		}
		msg->unbundle(path);
	}

	if (path)
	{
		if (!sp)
		{
			sw = new Base_window(window_set,
					&second_source_wdesc);
			sp = (Source_pane *)sw->get_pane(PT_source);
		}
		else
			sw = sp->get_parent();
		sw->popup();
		sp->set_file(fname, path);
		sp->get_pane()->position(1);
		sp->select_cb(0, 0);
	}
}

void
Open_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs)
		dialog->error(E_ERROR, GE_selection_gone);
	else
		open_file(source, item_index);
	dialog->default_done();
}

// button push callback - open the selected file in the parent source window
void
Open_dialog::apply(Component *, void *)
{
	do_it(source);
}

void
Open_dialog::cancel(Component *, void *)
{
	Vector	*v = vec_pool.get();

	if (files->get_selections(v) != 0)
		files->deselect(*(int *)v->ptr());
	vec_pool.put(v);

	if (selection > -1)
		files->select(selection);
}

// find out where the selection was dropped, and open the file in that
// source window.  If it was not dropped on a source window, sw will be
// null, which will make do_it create a new, secondary source window
void
Open_dialog::drop_cb(Selection_list *, Component *drop_window)
{
	Source_pane	*sw = 0;

	if (drop_window)
	{
		Base_window	*window = drop_window->get_base();
		if (window && 
			window->get_window_set() == window_set)
			sw = (Source_pane *)window->get_pane(PT_source);
	}

	dialog->clear_msg();
	do_it(sw);
	dialog->popdown();
}

class Show_line_dialog : public Dialog_box
{
	Source_pane	*source;
	Text_line	*line;
	char		*save_string;	// saved for cancel operation

public:
			Show_line_dialog(Source_pane *);
			~Show_line_dialog() { delete save_string; }

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
};

Show_line_dialog::Show_line_dialog(Source_pane *sw)
	: DIALOG_BOX(sw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply, LAB_show_line, LAB_show_line_mne,
			(Callback_ptr)(&Show_line_dialog::apply) },
		{ B_cancel, LAB_none,  LAB_none, 
			(Callback_ptr)(&Show_line_dialog::cancel) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Packed_box	*box;
	Caption		*caption;

	save_string = 0;
	source = sw;

	dialog = new Dialog_shell(window_set->get_window_shell(), LAB_show_line, 0, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_show_line_dialog);
	box = new Packed_box(dialog, "show line", OR_vertical);

	caption = new Caption(box, LAB_line_line, CAP_LEFT);
	line = new Text_line(caption, "line", "", 5, 1);

	caption->add_component(line, FALSE);
	box->add_component(caption);
	dialog->add_component(box);
	dialog->set_popup_focus(line);
}

void
Show_line_dialog::cancel(Component *, void *)
{
	line->set_text(save_string);
}

// position the file in the parent source window at the given line
void
Show_line_dialog::apply(Component *, void *)
{
	char	*s = line->get_text();
	char	*p;
	int	l;

	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_number);
		return;
	}

	l = (int) strtol(s, &p, 10);
	if (p == s || *p)
	{
		dialog->error(E_ERROR, GE_no_number);
		return;
	}

	if (l <= 0 || l > source->get_pane()->get_last_line())
	{
		dialog->error(E_ERROR, GE_out_of_bounds);
		return;
	}

	source->get_pane()->position(l);
	source->get_pane()->set_line(l);
	delete save_string;
	save_string = makestr(s);
}

class Show_function_dialog : public Object_dialog
{
	Source_pane	*source;
	Selection_list	*functions;
	Radio_list	*choice;
	Caption		*caption;

			// state information saved for cancel
	int		f_selection;
	unsigned char	use_object;

	void		do_it(Source_pane *);
	void		do_apply(Source_pane *);
	void		set_list(const char *object, const char *file);
	void		init_functions(const char *file, const char *object);
	void		object_changed(const char *object);

public:
			Show_function_dialog(Source_pane *);
			~Show_function_dialog()	{}

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		default_cb(Component *, int);
	void		drop_cb(Selection_list *, Component *);
	void		set_function_type(Radio_list *, int);

			// inherited from Process_dialog
	void		set_obj(Boolean reset);

	void		set_file(const char *);
};

#define CURRENT_FILE 0
#define SELECTED_OBJ 1

Show_function_dialog::Show_function_dialog(Source_pane *sw)
	: OBJECT_DIALOG(sw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply, LAB_show_func, LAB_show_func_mne, 
			(Callback_ptr)(&Show_function_dialog::apply) },
		{ B_cancel, LAB_none,  LAB_none, 
			(Callback_ptr)(&Show_function_dialog::cancel) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	static const LabelId radio_buttons[] =
	{
		LAB_current_file,
		LAB_selected_obj,
	};

	Caption		*lcaption;
	Expansion_box	*box;
	const char	*initial_string = "";	// blank data for selection list
				// real data is filled in later - after set_plist

	f_selection = -1;
	source = sw;
	use_object = (sw->get_current_file() == 0 ? SELECTED_OBJ : CURRENT_FILE);

	dialog = new Dialog_shell(window_set->get_window_shell(), LAB_show_func, 
		(Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_show_source_function_dialog);
	box = new Expansion_box(dialog, "show function", OR_vertical);
	dialog->add_component(box);
	component_init(box);

	lcaption = new Caption(box, LAB_funcs_from_line, CAP_LEFT);
	choice = new Radio_list(lcaption, "function type", OR_vertical,
		radio_buttons, sizeof(radio_buttons)/sizeof(LabelId), use_object,
		(Callback_ptr)(&Show_function_dialog::set_function_type), this);
	lcaption->add_component(choice, FALSE);
	box->add_component(lcaption);

	box->add_component(make_obj_list(box));

	caption = new Caption(box, LAB_functions_line, CAP_TOP_LEFT);
	box->add_component(caption, TRUE);
	functions = new Selection_list(caption, "functions", 8, 1, "%25s", 1,
		&initial_string, SM_single, this, 0, 0,
		(Callback_ptr)(&Show_function_dialog::default_cb),
		(Callback_ptr)(&Show_function_dialog::drop_cb));
	caption->add_component(functions);
}

// update the selection lists when the current process changes
void
Show_function_dialog::set_obj(Boolean reset)
{
	if (!pobjs)
	{
		objects->set_list(0, 0);
		functions->set_list(0, 0);
		saved_object = current_object = f_selection = -1;
		delete save_proc;
		save_proc = 0;
		return;
	}

	const char	*file = source->get_current_file();

	if (reset)
		dialog->set_busy(TRUE);

	if (file && use_object != SELECTED_OBJ
		&& (!save_proc || strcmp(save_proc, pobjs[0]->get_name()) != 0))
	{
		f_selection = -1;
		init_functions(file, 0);
	}
	else if (!file)
	{
		use_object = SELECTED_OBJ;
		choice->set_button(use_object);
	}

	Object_dialog::set_obj(reset);
	if (reset)
		dialog->set_busy(FALSE);
}

// Display the function in the given source window.  If sw is NULL,
// meaning the function was dragged and dropped onto the workspace,
// create a new secondary source window
void
Show_function_dialog::do_apply(Source_pane *sp)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;	
	}

	Vector	*v = vec_pool.get();
	if (functions->get_selections(v) == 0)
	{
		dialog->error(E_ERROR, GE_no_function);
		vec_pool.put(v);
		return;
	}

	// selection list ensures that there is at most one selection
	f_selection = *(int *)v->ptr();
	vec_pool.put(v);
	do_it(sp);
}

void
Show_function_dialog::do_it(Source_pane *sp)
{
	const char	*fname;
	Message		*msg;
	Word		line = 0;
	char		*file = 0;
	char		*tmp;
	Base_window	*sw;

	fname = functions->get_item(f_selection, 0);
	use_object = choice->which_button();
	if (use_object == SELECTED_OBJ)
	{
		Vector *v = vec_pool.get();
		objects->get_selections(v);
		saved_object = *(int *)v->ptr();
		vec_pool.put(v);
	}

	// find the file and line number for the selected function
	dispatcher.query(this, pobjs[0]->get_id(), "list -c1 %s\n", fname);
	while ((msg = dispatcher.get_response()) != 0)
	{
		switch (msg->get_msg_id())
		{
		case MSG_line_src:
			msg->unbundle(line, tmp);
			break;

		case ERR_newer_file:
			display_msg(msg);
			break;
	
		default:
			dialog->error(E_ERROR, GE_no_source, fname);
			break;
		}
	}
	if (!line)
		return;

	dispatcher.query(this, pobjs[0]->get_id(), "print %%list_file\n");
	while ((msg = dispatcher.get_response()) != 0)
	{
		if (msg->get_msg_id() == MSG_print_val)
			msg->unbundle(file);
		else
			dialog->error(msg);
	}

	if (file)
	{
		// strip off quotes
		if (*file == '"')
		{
			file++;
			char *p = strchr(file, '"');
			if(p)
				*p = '\0';
		}
		if (!sp)
		{
			sw = new Base_window(window_set,
					&second_source_wdesc);
			sp = (Source_pane *)sw->get_pane(PT_source);
		}
		else
			sw = sp->get_parent();
		sw->popup();
		sp->set_file(file, 0);
		sp->get_pane()->position((int)line-1);
	}
}

// Display the function in the parent source window
void
Show_function_dialog::apply(Component *, void *)
{
	do_apply(source);
}

void
Show_function_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs)
		dialog->error(E_ERROR, GE_selection_gone);
	else
	{
		f_selection = item_index;
		do_it(source);
	}
	dialog->default_done();
}

// reset the radio button and both selection lists
void
Show_function_dialog::cancel(Component *, void *)
{
	if (use_object != choice->which_button())
	{
		f_selection = -1;
		set_function_type(0, use_object);
	}

	if (f_selection > -1)
		functions->select(f_selection);
	else
		functions->deselect(f_selection);

	if (use_object == SELECTED_OBJ)
	{
		if (!dialog->is_pinned())
			return;
		cancel_change();
	}
}

// Display the selected function in the dropped on window
void
Show_function_dialog::drop_cb(Selection_list *, Component *drop_window)
{
	Source_pane	*sw = 0;

	if (drop_window)
	{
		Base_window	*window = drop_window->get_base();
		if (window && 
			window->get_window_set() == window_set)
			sw = (Source_pane *)window->get_pane(PT_source);
	}
	dialog->clear_msg();
	do_apply(sw);
	dialog->popdown();
}

// Reset the function selection list whenever the user switches between
// the current file and the current object
void
Show_function_dialog::set_function_type(Radio_list *radio, int button)
{
	if (!pobjs)
	{
		dialog->error(E_ERROR, GE_selection_gone);
		return;	
	}

	if (!radio)	// i.e. this was called from cancel, not from the radio button
		choice->set_button(button);

	dialog->set_busy(TRUE);
	dialog->clear_msg();
	use_object = button;

	if (use_object == SELECTED_OBJ)
	{
		Vector	*v = vec_pool.get();
		if (objects->get_selections(v) == 1)
		{
			int	selection = *(int *)v->ptr();
			saved_object = current_object = -1;
			select_cb(objects, selection);
		}
		vec_pool.put(v);
	}
	else
	{
		const char	*file = source->get_current_file();

		if (!file)
		{
			dialog->error(E_ERROR, GE_no_current_file);
			choice->set_button(SELECTED_OBJ);
			dialog->set_busy(FALSE);
			return;
		}
		init_functions(file, 0);
	}

	dialog->set_busy(FALSE);
}

void
Show_function_dialog::init_functions(const char *file, const char *obj)
{
	char	*buf = makesf(gm_format(GM_functions_from), file ? file : obj);
	Vector	*v = vec_pool.get();
	int	nfuncs;
	const char	**funclist;

	f_selection = -1;
	caption->set_label(buf);
	delete buf;

	if (file)
		nfuncs = pobjs[0]->get_functions(v, file, 0, 1);
	else
		nfuncs = pobjs[0]->get_functions(v, 0, obj, 1);

	if (nfuncs)
	{
		funclist = (const char **)v->ptr();
		functions->set_list(nfuncs, funclist);
		free_strings(funclist, nfuncs);
	}
	else
	{
		dialog->error(E_ERROR, GE_no_source, file ? file : obj);
		functions->set_list(0, 0);
	}
	vec_pool.put(v);
}

void
Show_function_dialog::object_changed(const char *obj)
{
	if (use_object == SELECTED_OBJ)
		init_functions(0, obj);
}

void
Show_function_dialog::set_file(const char *file)
{
	if (use_object != SELECTED_OBJ)
		init_functions(file, 0);
}

Source_pane::Source_pane(Window_set *ws, Base_window *base, Box *box,
	const Pane_descriptor *pd, Boolean p) : PANE(ws, base, PT_source)
{
	current_file = 0;
	current_path = 0;
	current_line = 0;
	selected_line = 0;
	file_ptr = 0;
	flink = 0;
	pdesc = pd;

	pane = 0;
	caption = 0;

	show_line = 0;
	show_function = 0;
	open_box = 0;

	animated = FALSE;
	registered = FALSE;
	primary = p;

	caption = new Caption(box, LAB_none, CAP_TOP_CENTER);
	pane = new Text_display(caption, "source", base,
		(Callback_ptr)(&Source_pane::select_cb), 
		(Callback_ptr)(&Source_pane::toggle_break_cb), 
		this, HELP_source_pane);
	// initialize text window size
	pane->setup_source_file(0, pdesc->nlines, pdesc->ncolumns);
	pane->set_breaklist(0);		// initialize breaks

	caption->add_component(pane);
	box->add_component(caption, TRUE);
}

Source_pane::~Source_pane()
{
	delete show_line;
	delete show_function;
	delete open_box;
	delete current_path;
}

void
Source_pane::clear()
{
	//if called inside constructor, caption is null
	if (caption)
		caption->set_label(" ");
	if (pane)
		pane->clear();
	if (selected_line)
	{
		parent->set_selection(0);
		selected_line = 0;
	}
	current_line = 0;
	current_file = 0;
	delete current_path;
	current_path = 0;
	file_ptr = 0;
}

void
Source_pane::update_state_cb(void *, Reason_code rc, void *, ProcObj *)
{
	if (rc == RC_start_script)
	{
		current_line = 0;
		pane->set_line(0);
	}
	parent->set_sensitivity();
}

void
Source_pane::update_cb(void *, Reason_code rc, void *, ProcObj *cdata)
{
	if (rc == RC_animate)
	{
		if (!animated) // instruction level animation
		{
			current_line = 0;
			pane->set_line(0);
		}
		return;
	}

	if (cdata && cdata->is_animated())
	{
		if (!animated)	// instruction level animation
			return;
	}

	if (rc == RC_delete || rc == RC_rename)
		return;

	// The source window is only registered with the Event list's notifier -
	// to get notifications of new or deleted breakpoints - while there is
	// a live (non-core) current process.  
	if (rc == RC_set_current)
	{
		if (!cdata || cdata->is_core())
		{
			if (registered)
			{
				registered = FALSE;
				event_list.change_list.remove(this,
					(Notify_func)(&Source_pane::break_list_cb), 0);
			}
		}
		else if (!registered)
		{
			registered = TRUE;
			event_list.change_list.add(this,
				(Notify_func)(&Source_pane::break_list_cb), 0);
		}
	}
		
	if (!cdata)
	{
		clear();	// clear calls set_sensitivity
		return;
	}

	if (!primary)
	{
		// secondary source windows are not updated when the process
		// state changes, except to update the current line indicator
		if (strcmp(current_file, cdata->get_file()) == 0)
		{
			current_line = cdata->get_line();
			pane->set_line(current_line);
		}
		else	// file where process stopped no longer matches displayed file
			pane->set_line(0);
		return;
	}

	if (cdata->is_running())
	{
		// if this is a new current process, the displayed file from
		// the previous process is no longer valid.  Otherwise, the
		// process has just been set running - the file is still ok
		// to display, but there is no current line.
		if  (rc == RC_set_current)
			clear();
		else if (!animated)
		{
			current_line = 0;
			pane->set_line(0);
		}
		return;
	}

	// state == stopped
	if (cdata->in_bad_state())
	{
		clear();
		return;
	}

	if (in_script)
		return;

	if (!cdata->get_file())
	{
		clear();
		parent->get_window_shell()->clear_msg();
		parent->get_window_shell()->display_msg(E_WARNING,
						GE_no_source, cdata->get_function());
		return;
	}

	current_file = cdata->get_file();
	current_line = cdata->get_line();

	if (set_path(rc == RC_set_current))
	{
		if (current_path)
		{
			pane->set_file(current_path);
			caption->set_label(current_path ? current_path : " ");
			if ((file_ptr = event_list.find_file(current_path, 0)) == 0)
			{
				pane->set_breaklist(0);
			}
			else
			{
				Vector	*v = vec_pool.get();

				file_ptr->get_break_list(cdata, v);
				pane->set_breaklist((int *)v->ptr());
				vec_pool.put(v);
			}
		}
		else
		{
			clear();
			return;
		}
	}
	if (current_line)
	{
		pane->position(current_line);
		pane->set_line(current_line);
	}
	else
	{
		// have source but no line info, e.g. not compiled
		// with -g
		parent->get_window_shell()->clear_msg();
		parent->get_window_shell()->display_msg(E_WARNING,
			GE_src_no_line, cdata->get_function());
	}


	if (show_function && show_function->dialog->is_open())
		show_function->set_file(current_file);

	parent->set_sensitivity();
}

int
Source_pane::check_sensitivity(int sense)
{
	if ((sense & SEN_file_required) && !current_file)
		return 0;
	if (sense & SEN_sel_required)
	{
		if (!(sense & (SEN_source_sel|SEN_text_sel)) ||
			!selected_line)
			// wrong selection type or no selection
			return 0;
		if ((sense & SEN_breakpt_required) && 
			!pane->has_breakpoint(selected_line))
			return 0;
	}
	return 1;
}

// An event with breakpoints is being added or deleted
// Check each breakpoint in the event to see if it effects the current file
void
Source_pane::break_list_cb(void *, Reason_code_break rc, void *, Stop_event *event)
{
	ProcObj	*proc = window_set->current_obj();
	if (!proc || !event->has_obj(proc))
		return;

	Breakpoint	*breaks = event->get_breakpts();

	for (int i = event->get_nbreakpts(); i; i--, breaks++)
	{
		File_list	*fptr = event_list.find_file(breaks->file, 0);

		if (!fptr || // e.g. breakpt with no file and line
		    (strcmp(fptr->get_name(), current_path) != 0 &&
		     strcmp(fptr->get_name(), current_file) != 0))
			continue;

		if (rc == BK_add)
		{
			if (!file_ptr)
				file_ptr = fptr;
			pane->set_stop(breaks->line);
		}
		else if (rc == BK_delete)
		{
			Vector	*elist = vec_pool.get();
			int	total = proc->get_event_list(breaks->addr, elist);
			int	*events = (int *)elist->ptr();

			if (!total)
			{
				parent->get_window_shell()->clear_msg();
				parent->get_window_shell()->display_msg(E_ERROR, GE_internal,
					__FILE__, __LINE__);
				vec_pool.put(elist);
				continue;
			}

			for (; *events; events++)
			{
				if (*events != event->get_id()
					&& event_list.findEvent(*events)->get_state() == ES_valid)
					break; // one enabled event exists
			}

			if (!*events) // didn't break out of loop
				pane->clear_stop(breaks->line);
			vec_pool.put(elist);
		}
		else
		{
			parent->get_window_shell()->clear_msg();
			parent->get_window_shell()->display_msg(E_ERROR,
					GE_internal, __FILE__, __LINE__);
			return;
		}
	}
	parent->set_sensitivity();
}

void
Source_pane::popup()
{
	ProcObj	*pobj = window_set->current_obj();
	update_cb(0, RC_set_current, 0, pobj);
	if (pobj && !pobj->is_core())
	{
		registered = TRUE;
		event_list.change_list.add(this,
			(Notify_func)(&Source_pane::break_list_cb), 0);
	}
	window_set->change_current.add(this, (Notify_func)(&Source_pane::update_cb), 0);
	window_set->change_state.add(this, (Notify_func)(&Source_pane::update_state_cb), 0);
}

void
Source_pane::popdown()
{
	window_set->change_current.remove(this, (Notify_func)(&Source_pane::update_cb), 0);
	window_set->change_state.remove(this, (Notify_func)(&Source_pane::update_state_cb), 0);
	if (registered)
	{
		registered = FALSE;
		event_list.change_list.remove(this,
			(Notify_func)(&Source_pane::break_list_cb), 0);
	}
}

void
Source_pane::toggle_break_cb(Component *, void *cdata)
{
	selected_line = (int)cdata;
	if (pane->has_breakpoint(selected_line))
		delete_break_cb(0, 0);
	else
		set_break_cb(0, 0);
}

void
Source_pane::set_break_cb(Component *, void *)
{
	ProcObj	*proc = window_set->current_obj();
	int	lev = window_set->get_event_level();
	dispatcher.send_msg(this, proc->get_id(), "stop -p %s %s@%d\n",
		lev == PROGRAM_LEVEL ?
			proc->get_program()->get_name() : 
			(lev == PROCESS_LEVEL ?
				proc->get_process()->get_name() :
				proc->get_name()),
		current_file, selected_line);
}

void
Source_pane::delete_break_cb(Component *, void *)
{
	Window_shell	*ws = parent->get_window_shell();

	if (!file_ptr)
	{
		ws->clear_msg();
		ws->display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	for (flink = file_ptr->get_head(); flink; flink = flink->next())
	{
		if (flink->get_line() >= selected_line)
			break;
	}

	if (flink->get_line() > selected_line)
	{
		ws->clear_msg();
		ws->display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	// if one entry in the chain and the next both point to the same line,
	//	there are multiple breakpoints on the same line - ask for
	// 	confirmation before deleteing them,
	// else if the stop event has multiple breakpoints at different places
	//	ask for confirmation (this would have to come from an expression
	//	entered in the Stop dialog - it couldn't happen with Set Breakpoint)
	// else delete
	if (flink->next() && flink->next()->get_line() == selected_line)
		display_msg((Callback_ptr)(&Source_pane::ok_to_delete), this,
			LAB_delete, LAB_cancel, GE_multiple_events);
	else if (((Stop_event *)flink->get_event())->get_nbreakpts() > 1)
		display_msg((Callback_ptr)(&Source_pane::ok_to_delete), this,
			LAB_delete, LAB_cancel, GE_multiple_breaks);
	else
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"delete %d\n", flink->get_event()->get_id());
}

// ok_to_delete is called only from delete_break_cb, above, which sets flink
// to point to the entry to be deleted
void
Source_pane::ok_to_delete(Component *, int delete_flag)
{
	if (!delete_flag)
		return;

	for ( ; flink && flink->get_line() == selected_line; flink = flink->next())
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"delete %d\n", flink->get_event()->get_id());
}

void
Source_pane::copy_selection()
{
	pane->copy_selection();
}

void
Source_pane::open_dialog_cb(Component *, void *)
{
	if (!open_box)
		open_box = new Open_dialog(this);
#ifdef DEBUG_THREADS
	open_box->set_plist(parent, THREAD_LEVEL);
#else
	open_box->set_plist(parent, PROCESS_LEVEL);
#endif
	open_box->display();
}

void
Source_pane::show_function_cb(Component *, void *)
{
	if (!show_function)
		show_function = new Show_function_dialog(this);
	show_function->display();
#ifdef DEBUG_THREADS
	show_function->set_plist(parent, THREAD_LEVEL);
#else
	show_function->set_plist(parent, PROCESS_LEVEL);
#endif
}

void
Source_pane::show_line_cb(Component *, void *)
{
	if (!show_line)
		show_line = new Show_line_dialog(this);
	show_line->display();
}

void
Source_pane::new_source_cb(Component *, void *)
{
	Source_pane	*sp;
	Base_window	*sw = new Base_window(window_set,
					&second_source_wdesc);

	sw->popup();
	sp = (Source_pane *)sw->get_pane(PT_source);
	sp->set_file(current_file,current_path);
	sp->get_pane()->position(pane->get_position());
	sp->set_current_line(current_line);
}

void
Source_pane::set_file(const char *file, const char *path)
{
	if (strcmp(file, current_file) == 0)
		return;

	current_file = str(file);
	if (strcmp(current_file, window_set->current_obj()->get_file()) == 0)
		current_line = window_set->current_obj()->get_line();
	else
		current_line = 0;

	if (path)
	{
		delete current_path;
		current_path = makestr(path);
	}
	else
		(void) set_path(TRUE);

	if (!current_path)
	{
		clear();
		return;
	}

	pane->set_file(current_path);
	caption->set_label(current_path);
	if (current_line)
		pane->set_line(current_line);

	file_ptr = event_list.find_file(current_path, 0);

	if (file_ptr)
	{
		Vector	*v = vec_pool.get();

		file_ptr->get_break_list(window_set->current_obj(), v);
		pane->set_breaklist((int *)v->ptr());
		vec_pool.put(v);
	}
	else
		pane->set_breaklist(0);

	if (show_function && show_function->dialog->is_open())
		show_function->set_file(current_file);
}

void
Source_pane::select_cb(Text_display *, int line)
{
	selected_line = line;
	parent->set_selection(selected_line ? this : 0);
}

Selection_type
Source_pane::selection_type()
{
	return SEL_src_line;
}

char *
Source_pane::get_selection()
{
	return pane->get_selection();
}
	
Boolean
Source_pane::set_path(Boolean must_set)
{
	Message		*msg;
	ProcObj		*cdata = window_set->current_obj();
	Boolean		path_changed = must_set;
	const char	*ppath;

	if (!cdata || !current_file)
	{
		delete current_path;
		current_path = 0;
		return 1;
	}

	ppath = cdata->get_path();
	if (!must_set && current_file == cdata->get_file() && ppath)
	{
		if (current_path == 0 ||
		    strcmp(current_path, ppath) != 0)
		{
			delete current_path;
			current_path = makestr(ppath);
			path_changed = TRUE;
		}
	}
	else
	{
		dispatcher.query(this, cdata->get_id(), "ppath %s\n", current_file);
		while ((msg = dispatcher.get_response()) != 0)
		{
			if (msg->get_msg_id() == MSG_source_file)
			{
				char	*buf = 0;

				msg->unbundle(buf);
				if (current_path == 0 ||
				    strcmp(current_path, buf) != 0)
				{
					path_changed = TRUE;
					delete current_path;
					current_path = makestr(buf);
				}
			}
			else if (msg->get_msg_id() == ERR_no_source)
			{
				delete current_path;
				current_path = 0;
				parent->get_window_shell()->clear_msg();
				parent->get_window_shell()->display_msg(msg);
				path_changed = TRUE;
			}
			else
				display_msg(msg);
		}
	}
	return path_changed;
}

void
Source_pane::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (mtype == ERR_no_line)
	{
		char	*tmp;
		Word	line;

		m->unbundle(tmp, line);
		display_msg(E_ERROR, GE_cant_set_breakpt, (int)line);
	}
	else if (Mtable.msg_class(mtype) == MSGCL_error)
		display_msg(m);
	else if (!gui_only_message(m))
		parent->get_window_shell()->display_msg(m);
}
