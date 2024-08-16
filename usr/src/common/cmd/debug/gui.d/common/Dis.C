#ident	"@(#)debugger:gui.d/common/Dis.C	1.67"

// GUI headers
#include "Dialogs.h"
#include "Sch_dlg.h"
#include "Dialog_sh.h"
#include "Dispatcher.h"
#include "Eventlist.h"
#include "Dis.h"
#include "Windows.h"
#include "Window_sh.h"
#include "Menu.h"
#include "Text_disp.h"
#include "Text_line.h"
#include "Stext.h"
#include "Sel_list.h"
#include "Boxes.h"
#include "Caption.h"
#include "Proclist.h"
#include "UI.h"
#include "config.h"
#include "gui_label.h"
#include "Label.h"

// Debug headers
#include "Message.h"
#include "Msgtab.h"
#include "Vector.h"
#include "Buffer.h"
#include "Machine.h"
#include "str.h"

#include <stdio.h>
#include <stdlib.h>
#include <limits.h>

// DispLine maps addresses and source line numbers (if available) to lines
// in the Disassembly pane - there will be one entry in an array of DispLines
// for each line of disassembly
class DispLine
{
	int	srcLine;
	Iaddr	addr;
public :
		DispLine(Iaddr a, int line = 0){ srcLine = line; addr = a;}
		~DispLine(){};

	Iaddr	getAddr(){return addr;}
	int	getLine(){return srcLine;}
};

class Show_loc_dialog : public Dialog_box
{
	Disassembly_pane	*disasm;
	Text_line	*line;
	char		*save_string;	// saved state for cancel callback

public:
			Show_loc_dialog(Disassembly_pane *);
			~Show_loc_dialog(){ delete save_string; }

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

	void		set_string(const char *s){ line->set_text(s); }

};

Show_loc_dialog::Show_loc_dialog(Disassembly_pane *dw)
	: DIALOG_BOX(dw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply, LAB_show_loc, LAB_show_loc_mne,
			(Callback_ptr)(&Show_loc_dialog::apply) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Show_loc_dialog::cancel) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Caption		*caption;

	save_string = 0;
	disasm = dw;

	dialog = new Dialog_shell(window_set->get_window_shell(), LAB_show_loc, 0, this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_show_location_dialog);

	caption = new Caption(dialog, LAB_location_line, CAP_LEFT);
	line = new Text_line(caption, "loc", "", 15, 1);

	caption->add_component(line, FALSE);
	dialog->add_component(caption);
	dialog->set_popup_focus(line);
}

void
Show_loc_dialog::cancel(Component *, void *)
{
	line->set_text(save_string);
}

void
Show_loc_dialog::apply(Component *, void *)
{
	char	*s = line->get_text();

	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_number);
		return;
	}

	delete save_string;
	save_string = makestr(s);

	char	*endP = 0;
	Iaddr	retLong = strtoul(s, &endP, 0);

	// ask the Disassembly window to disassemble the code
	if (retLong != ULONG_MAX && endP != 0 && *endP == '\0') 
		disasm->disasm_addr(retLong, s);
	else
	{
		dialog->error(E_ERROR, GE_invalid_addr);
		return;
	}
}

class Function_dialog : public Object_dialog
{
	Disassembly_pane	*disasm;
	Selection_list	*functions;
	Caption		*caption;

			// saved state for cancel callback
	int		f_selection;

	void		object_changed(const char *);
	void		do_it();
public:
			Function_dialog(Disassembly_pane *);
			~Function_dialog() {}

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);
	void		default_cb(Component *, int);
	void		drop_cb(Selection_list *, Component *);
};

Function_dialog::Function_dialog(Disassembly_pane *dw)
	: OBJECT_DIALOG(dw->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_apply, LAB_show_func, LAB_show_func_mne, 	
			(Callback_ptr)(&Function_dialog::apply) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Function_dialog::cancel) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Expansion_box	*box;

	f_selection = -1;
	disasm = dw;

	dialog = new Dialog_shell(window_set->get_window_shell(), LAB_show_func,
		(Callback_ptr)(&Process_dialog::dismiss_cb), this,
		buttons, sizeof(buttons)/sizeof(Button), HELP_show_dis_function_dialog);
	box = new Expansion_box(dialog, "show function", OR_vertical);
	dialog->add_component(box);
	component_init(box);

	const char *dummy = "";	// used to initalize selection list, real values
				// are set later in set_process, called by set_plist

	box->add_component(make_obj_list(box));

	caption = new Caption(box, LAB_functions_line, CAP_TOP_LEFT);
	box->add_component(caption, TRUE);
	functions = new Selection_list(caption, "functions", 8, 1, "%25s", 1,
		&dummy, SM_single, this, 
		0, 0, 
		(Callback_ptr)(&Function_dialog::default_cb), 
		(Callback_ptr)(&Function_dialog::drop_cb));

	caption->add_component(functions);
}

void
Function_dialog::apply(Component *, void *)
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

	// selection list in SM_single mode ensures only one selection
	f_selection = *(int *)v->ptr();
	vec_pool.put(v);
	do_it();
}

void
Function_dialog::do_it()
{
	Vector	*v = vec_pool.get();

	objects->get_selections(v);
	saved_object = *(int *)v->ptr();
	vec_pool.put(v);

	disasm->disasm_set_func(functions->get_item(f_selection, 0));
}

void 
Function_dialog::default_cb(Component *, int item_index)
{
	dialog->default_start();
	if (!pobjs)
		dialog->error(E_ERROR, GE_selection_gone);
	else
	{
		f_selection = item_index;
		do_it();
	}
	dialog->default_done();
}

void
Function_dialog::cancel(Component *, void *)
{
	if (f_selection > -1)
		functions->select(f_selection);
	else
		functions->deselect(f_selection);

	if (!dialog->is_pinned())
		return;
	cancel_change();
}

// display the selected function in the Disassembly window
// complain if the function was not dropped on the parent window
void
Function_dialog::drop_cb(Selection_list *, Component *drop_window)
{
	if (drop_window)
	{
		Base_window	*window = drop_window->get_base();

		if (window == disasm->get_parent()) 
		{
			dialog->clear_msg();
			// conform to prototype
			apply(0,0);
			dialog->popdown();
			return;
		}
	}
	dialog->error(E_ERROR, GE_drop_to_disasm);
}

// initialize the function list with the functions in the shared object obj
void
Function_dialog::object_changed(const char *obj)
{
	if (!obj)
	{
		caption->set_label(labeltab.get_label(LAB_functions_line));
		functions->set_list(0, 0);
		f_selection = -1;
		return;
	}

	char	*buf = makesf(gm_format(GM_functions_from), obj);
	Vector	*v = vec_pool.get();
	int	total = pobjs[0]->get_functions(v, 0, obj, 0);
	const char	**list = (const char **)v->ptr();

	if (total == 0)
	{
		dialog->error(E_ERROR, GE_stripped_file, obj);
		vec_pool.put(v);
		return;
	}

	caption->set_label(buf);
	delete buf;

	functions->set_list(total, list);
	free_strings(list, total);
	vec_pool.put(v);
	f_selection = -1;
}

Disassembly_pane::Disassembly_pane(Window_set *ws, Base_window *parent, Box *box,
	const Pane_descriptor *pdesc) : PANE(ws, parent, PT_disassembler)
{
	int		*breaklist = 0;

	current_func = 0;
	current_loc  = 0;
	current_line = 0;
	selected_line = 0;
	animated = FALSE;

	// Disassembly pane
	caption = new Caption(box, LAB_disassembly, CAP_TOP_CENTER);
	pane = new Text_display(caption, "disassembly", parent,
		(Callback_ptr)(&Disassembly_pane::select_cb),
		(Callback_ptr)(&Disassembly_pane::toggle_break_cb), 
		this, HELP_dis_pane);
	pane->setup_disassembly(pdesc->nlines, pdesc->ncolumns);
	pane->set_breaklist(breaklist);

	caption->add_component(pane);
	box->add_component(caption);

	show_loc 	= 0;
	show_function 	= 0;
}

Disassembly_pane::~Disassembly_pane()
{
	delete show_loc;
	delete show_function;
	delete current_func;
	delete current_loc;
	window_set->change_current.remove(this,
		(Notify_func)(&Disassembly_pane::update_cb), 0);
	event_list.change_list.remove(this, (Notify_func)(&Disassembly_pane::break_list_cb), 0);
}

void
Disassembly_pane::clear()
{
	pane->set_buf("");
	caption->set_label(" ");
	current_line = 0;
	delete current_func;
	delete current_loc;
	current_loc = 0;
	current_func = 0;
	locations.clear();
	if (selected_line)
	{
		selected_line = 0;
		parent->set_selection(0);
	}
}

void
Disassembly_pane::update_state_cb(void *, Reason_code rc, void *, ProcObj *)
{
	if (rc == RC_start_script)
	{
		current_line = 0;
		pane->set_line(0);
	}
	parent->set_sensitivity();
}

void
Disassembly_pane::update_cb(void *, Reason_code rc, void *, ProcObj *proc)
{
	if (rc == RC_animate)
	{
		if (!animated) // source level animation
		{
			current_line = 0;
			pane->set_line(0);
		}
		return;
	}

	if (proc && proc->is_animated())
	{
		if (!animated)	// source animation
			return;
	}

	if (rc == RC_delete || rc == RC_rename || in_script)
		// delete is always followed by set_current
		return;

	if (!proc)
	{
		clear();
		parent->get_window_shell()->clear_msg();
		parent->get_window_shell()->display_msg(E_WARNING, GE_no_process);
		return;
	}

	if (proc->in_bad_state())
	{
		clear();
		return;
	}

	if (proc->is_running())
	{
		if (!animated)
		{
			pane->set_line(0);
			current_line = 0;
			if (selected_line)
			{
				selected_line = 0;
				parent->set_selection(0);
			}
		}
		return;
	}

	if (!proc->get_location() || !*proc->get_location())
	{
		clear();
		return;
	}
	delete current_loc;
	current_loc = makestr(proc->get_location());

	//change disassembled display when
	// 	there is no current function and current_location does not exist
	//		in the current display
	//	or the current function changes
	if (!proc->get_function() || !*proc->get_function())
	{

		char	*endP = 0;
		Iaddr	retLong = (Iaddr)strtoul(current_loc, &endP, 0);

		delete current_func;
		current_func = 0;

		// ask the Disassembly window to disasm the code
		if (retLong != ULONG_MAX && endP != 0 && *endP == '\0') 
		{
			if (has_inst(retLong))
				set_display_line();
			else
			{
				caption->set_label(" ");
				disasm_func(current_loc);
			}
		}
		else
		{
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			return;
		}
	}
	else if (rc == RC_set_current || !current_func
		|| strcmp(proc->get_function(), current_func) != 0)
	{
		delete current_func;
		current_func = makestr(proc->get_function());
		disasm_func(current_func);
		caption->set_label(current_func);
	}

	set_display_line();
}

int
Disassembly_pane::check_sensitivity(int sense)
{
	if ((sense & SEN_disp_dis_required) && locations.size() == 0)
		return 0;
	if (sense & SEN_sel_required)
	{
		if (!(sense & (SEN_dis_sel|SEN_text_sel)) ||
			!selected_line)
			// wrong selection type or no selection
			return 0;
		if ((sense & SEN_breakpt_required) && 
			!pane->has_breakpoint(selected_line))
			return 0;
	}
	return 1;
}

//return line number if address is within the disassembly display
//return 0 otherwise
int
Disassembly_pane::has_inst(Iaddr address)
{
	DispLine	*p = (DispLine *)locations.ptr();
	DispLine	*endP = (DispLine *)((char *)locations.ptr() + locations.size());
	int		line = 1;

	for (; p < endP; line++, p++)
	{
		if (p->getAddr() == address)
			 return line;
	}
	return 0;
}

// break_list_cb is called from the Event_list whenever a breakpoint is set
// or deleted
void
Disassembly_pane::break_list_cb(void *, Reason_code_break rc, void *, Stop_event *event)
{
	ProcObj	*proc = window_set->current_obj();
	if (!proc  || !event->has_obj(proc))
		return;

	Breakpoint	*breaks = event->get_breakpts();

	// an event may have more than one breakpoint - handle all the ones that
	// fall within the current function
	for (int i = event->get_nbreakpts(); i; i--, breaks++)
	{
		int line;
		if ((line = has_inst(breaks->addr)) == 0)
			continue;

		if (rc == BK_add)
		{
			pane->set_stop(line);
		}
		else if (rc == BK_delete)
		{
			// if there is another enabled event on this address,
			// 	do not delete the sign
			// Note that at this point, the event may or
			//  may not exist on the event list for the 
			//  process(disabled, or deleted), but
			//  the event always exists on the event list
			int	*pEventNum;
			Vector	*v = vec_pool.get();

			// get the events on this instruction
			proc->get_event_list(breaks->addr, v);
			pEventNum = (int *)v->ptr();

			for (; *pEventNum; pEventNum++)
			{
				if (*pEventNum != event->get_id()
					&& event_list.findEvent(*pEventNum)->get_state() == ES_valid)
					break; //one enabled event exists

			}
			if (!*pEventNum)
				pane->clear_stop(line);
			vec_pool.put(v);
		}
	}
	parent->set_sensitivity();
}

void
Disassembly_pane::popup()
{
	update_cb(0, RC_set_current, 0, window_set->current_obj());
	window_set->change_current.add(this, (Notify_func)(&Disassembly_pane::update_cb), 0);
	window_set->change_state.add(this, (Notify_func)(&Disassembly_pane::update_state_cb), 0);
	event_list.change_list.add(this, (Notify_func)(&Disassembly_pane::break_list_cb), 0);
}

void
Disassembly_pane::popdown()
{
	window_set->change_current.remove(this, (Notify_func)(&Disassembly_pane::update_cb), 0);
	window_set->change_state.remove(this, (Notify_func)(&Disassembly_pane::update_state_cb), 0);
	event_list.change_list.remove(this, (Notify_func)(&Disassembly_pane::break_list_cb), 0);
}

void
Disassembly_pane::toggle_break_cb(Component *, void *cdata)
{
	selected_line = (int)cdata;
	if (pane->has_breakpoint(selected_line))
		delete_break_cb(0,0);
	else
		set_break_cb(0,0);
}

void
Disassembly_pane::set_break_cb(Component *, void *)
{
	if (! selected_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	DispLine	*p = ((DispLine *)locations.ptr()) + selected_line - 1;
	ProcObj	*proc = window_set->current_obj();
	int	lev = window_set->get_event_level();

	dispatcher.send_msg(this, proc->get_id(), "stop -p %s 0x%x\n",
		lev == PROGRAM_LEVEL ?
			proc->get_program()->get_name() : 
			(lev == PROCESS_LEVEL ? 
				proc->get_process()->get_name() :
				proc->get_name()), 
		p->getAddr());
}

void
Disassembly_pane::delete_break_cb(Component *, void *)
{
	if (! selected_line)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}


	Vector		*v = vec_pool.get();
	ProcObj	*pp = window_set->current_obj();
	int		*pEventNum = 0;
	DispLine	*p = ((DispLine *)locations.ptr()) + selected_line - 1;

	// test if there are mutiple events on the line
	pp->get_event_list(p->getAddr(), v);
	pEventNum = (int *)v->ptr();

	if (*pEventNum != 0 && *(pEventNum + 1) != 0)
		display_msg((Callback_ptr)(&Disassembly_pane::ok_to_delete), this,
			LAB_delete, LAB_cancel, GE_multiple_events);
	else if (((Stop_event *)event_list.findEvent(*pEventNum))->get_nbreakpts() > 1)
		display_msg((Callback_ptr)(&Disassembly_pane::ok_to_delete), this,
			LAB_delete, LAB_cancel, GE_multiple_breaks);
	else
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
			"delete %d\n", *pEventNum);
	vec_pool.put(v);
}

// ok_to_delete is the callback from the notice displayed by delete_break_cb,
// asking if it is ok to delete an event with multiple breakpoints, or multiple
// events on one line
void
Disassembly_pane::ok_to_delete(Component *, int delete_flag)
{
	if (!delete_flag)
		return;

	DispLine	*p = ((DispLine *)locations.ptr()) + selected_line - 1;
	ProcObj		*proc = window_set->current_obj();
	int		*pEventNum = 0;
	Vector		*v = vec_pool.get();

	proc->get_event_list(p->getAddr(), v);
	pEventNum = (int *)v->ptr();

	for (; pEventNum && *pEventNum; pEventNum++)
	{
		dispatcher.send_msg(this, proc->get_id(), "delete %d\n", *pEventNum);
	}
	vec_pool.put(v);
}

void
Disassembly_pane::copy_cb(Component *, void *)
{
	pane->copy_selection();
}

void
Disassembly_pane::copy_selection()
{
	pane->copy_selection();
}

void
Disassembly_pane::show_function_cb(Component *, void *)
{
	if (show_function == 0)
		show_function = new Function_dialog(this);

	show_function->display();
#ifdef DEBUG_THREADS
	show_function->set_plist(parent, THREAD_LEVEL);
#else
	show_function->set_plist(parent, PROCESS_LEVEL);
#endif
}

void
Disassembly_pane::show_loc_cb(Component *, void *)
{
	if (!show_loc)
		show_loc = new Show_loc_dialog(this);
	
	if (selected_line)
		show_loc->set_string(get_selected_addr());

	show_loc->display();
}

void
Disassembly_pane::select_cb(Text_display *, int line)
{
	if (!line && !selected_line)
		return;

	selected_line = line;
	parent->set_selection(selected_line ? this : 0);
}

Selection_type
Disassembly_pane::selection_type()
{
	return SEL_instruction;
}

char *
Disassembly_pane::get_selection()
{
	return pane->get_selection();
}

// disassemble the location 
void
Disassembly_pane::disasm_addr(Iaddr loc, char *s)
{
	current_line = has_inst(loc);
	
	if (current_line != 0)
	{
		pane->position(current_line);
		pane->set_line(current_line);
		return;
	}

	disasm_func(s);
	caption->set_label(" ");
}

void
Disassembly_pane::disasm_set_func(const char *loc)
{
	delete current_func;
	current_func = makestr(loc);
	caption->set_label(loc);
	disasm_func(loc);
}

void
Disassembly_pane::disasm_func(const char *loc)
{
	dis_buffer.clear();
	locations.clear();

	parent->inc_busy();
	if (strstr(loc, "@PLT") != 0)
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
		 	"dis -f %s\n", window_set->current_obj()->get_location());
	else	
		dispatcher.send_msg(this, window_set->current_obj()->get_id(),
		 	"dis -f %s\n", loc);
}

void
Disassembly_pane::set_display_line()
{
	int returnLine = has_inst(current_loc);

	current_line = selected_line = 0;

	if (returnLine != 0)
	{
		current_line = returnLine;
		pane->position(current_line);
		pane->set_line(current_line);
	}
}

int
Disassembly_pane::has_inst(const char *s)
{
	const char	*loc = s;

	// location is a hex number or file@line
	if (strchr(s, '@') != 0)
	{
		Message	*msg;
		char	*tmp;

		dispatcher.query(this, window_set->current_obj()->get_id(),
			"print %%loc\n");
		while ((msg = dispatcher.get_response()) != 0)
		{
			if (msg->get_msg_id() == MSG_print_val)
			{
				msg->unbundle(tmp);
				loc = tmp;
			}
			else
			{
				display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
				return 0;
			}
		}
	}

	char	*endP = 0;
	Iaddr	retLong = strtoul(loc, &endP, 0);

	if (retLong != ULONG_MAX && endP != 0 && (*endP == '\0' || *endP == '\n')) 
	{
		return has_inst(retLong);
	}
	return 0;
}

const char *
Disassembly_pane::get_selected_addr()
{
	static char	buf[ sizeof(Iaddr) * 2 + 3 ];

	buf[0] = '\0';
	if (!selected_line) 
		return buf;

	DispLine	*p = ((DispLine *)locations.ptr()) + selected_line - 1;

	sprintf(buf, "%#x", p->getAddr());
	return buf;
}

void
Disassembly_pane::cmd_complete()
{
	Iaddr	*breaklist = 0;
	Vector	*v = vec_pool.get();
	Vector	*v2 = vec_pool.get();
	int	line;

	pane->set_buf((char *)dis_buffer);
	set_display_line();

	window_set->current_obj()->get_break_list(start_addr, end_addr, v);
	breaklist = (Iaddr *)v->ptr();
	v2->clear();

	// translate instruction address into display line number 
	for (Iaddr *pb = breaklist; *pb; pb++)
	{
		line = has_inst(*pb);
		v2->add(&line, sizeof(int));
	}

	line = 0;
	v2->add(&line, sizeof(int));
	pane->set_breaklist((int *)v2->ptr());
	vec_pool.put(v2);
	vec_pool.put(v);
	parent->dec_busy();
	parent->set_sensitivity();
}

void
Disassembly_pane::de_message(Message *m)
{
	Word	line;
	Word	addr;
	char	*instruction;
	char	*tmp;

	switch(m->get_msg_id())
	{
		case MSG_dis_header:
			start_addr = (unsigned)-1;
			end_addr = 0;
			break;

		case MSG_dis_line:
			dis_buffer.add(m->format());
			m->unbundle(line, tmp, addr, instruction);
			if (start_addr == (unsigned)-1)
				start_addr = addr;
			end_addr = addr;
			{
				DispLine tmpDisp((Iaddr)addr, (int)line);
				locations.add(&tmpDisp, sizeof(DispLine));
			}
			break;

		case MSG_disassembly:
			dis_buffer.add(m->format());
			m->unbundle(addr, instruction);
			if (start_addr == (unsigned)-1)
				start_addr = addr;
			end_addr = addr;
			{
				DispLine tmpDisp((Iaddr)addr);
				locations.add(&tmpDisp, sizeof(DispLine));
			}
			break;

		default:
			if (Mtable.msg_class(m->get_msg_id()) == MSGCL_error)
				display_msg(m);
			else if (!gui_only_message(m))
				parent->get_window_shell()->display_msg(m);
			break;
	}
}	
