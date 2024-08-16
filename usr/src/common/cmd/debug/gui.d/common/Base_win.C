#ident	"@(#)debugger:gui.d/common/Base_win.C	1.13"

// GUI headers
#include "Base_win.h"
#include "Boxes.h"
#include "Caption.h"
#include "Command.h"
#include "Dialogs.h"
#include "Dialog_sh.h"
#include "Dis.h"
#include "Events.h"
#include "Menu.h"
#include "Button_bar.h"
#include "Panes.h"
#include "Proclist.h"
#include "Ps_pane.h"
#include "Radio.h"
#include "Regs_pane.h"
#include "Sch_dlg.h"
#include "Source.h"
#include "Stack_pane.h"
#include "Syms_pane.h"
#include "Status.h"
#include "Table.h"
#include "UI.h"
#include "Window_sh.h"
#include "config.h"
#include "gui_label.h"

// Debug headers
#include "Machine.h"
#include "Message.h"
#include "Msgtab.h"

// Panes_dialog is created from Properties dialog's "Panes" button
// radio list states for Panes_dialog
#define	TRUNCATED	0
#define	WRAPPED		1

struct	Radio_descriptor {
	Radio_list	*radio;
	Table		*table;
	unsigned int	state;
};

class Panes_dialog : public Dialog_box
{
	Radio_descriptor	*radios;
	int			nradios;
public:
			Panes_dialog(Base_window *);
			// individual radio buttons deleted as
			// components of the window
			~Panes_dialog() { delete radios; }

	void		apply(Component *, void *);
	void		reset(Component *, void *);
	void		add_radio(Radio_descriptor *, Packed_box *,
				Table *, LabelId clabel, char *rlabel);
};

Panes_dialog::Panes_dialog(Base_window *win) : DIALOG_BOX(win->get_window_set())
{
	Radio_descriptor	*rptr;

	static const Button	buttons[] =
	{
		{ B_apply,   LAB_none, LAB_none,
			(Callback_ptr)(&Panes_dialog::apply) },
		{ B_reset,   LAB_none, LAB_none, 
			(Callback_ptr)(&Panes_dialog::reset) },
		{ B_cancel,  LAB_none,	LAB_none, 
			(Callback_ptr)(&Panes_dialog::reset) },
		{ B_help,    LAB_none,	LAB_none, 0 },
	};

	Packed_box	*box;
	Stack_pane	*stack_pane;
	Symbols_pane	*syms_pane;
	Ps_pane		*ps_pane;
	Event_pane	*event_pane;

	nradios = 0;

	if ((stack_pane = (Stack_pane *)win->get_pane(PT_stack)))
		nradios++;
	if ((syms_pane = (Symbols_pane *)win->get_pane(PT_symbols)))
		nradios++;
	if ((ps_pane = (Ps_pane *)win->get_pane(PT_process)))
		nradios++;
	if ((event_pane = (Event_pane *)win->get_pane(PT_event)))
		// add one for regular events, one for onstop
		nradios += 2;
	
	radios = new Radio_descriptor[nradios];

	dialog = new Dialog_shell(win->get_window_shell(), LAB_panes, 
		0, this,
		buttons, sizeof(buttons)/sizeof(Button),
		HELP_panes_dialog);
	box = new Packed_box(dialog, "properties", OR_vertical);

	rptr = radios;
	if (ps_pane)
	{
		add_radio(rptr, box, ps_pane->get_table(),
			LAB_process_pane, "ps pane");
		rptr++;
	}
	if (stack_pane)
	{
		add_radio(rptr, box, stack_pane->get_table(),
			LAB_stack_pane, "stack pane");
		rptr++;
	}
	if (syms_pane)
	{
		add_radio(rptr, box, syms_pane->get_table(),
			LAB_syms_pane, "syms pane");
		rptr++;
	}
	if (event_pane)
	{
		add_radio(rptr, box, event_pane->getEventPane(),
			LAB_mevent_pane, "main event pane");
		rptr++;
		add_radio(rptr, box, event_pane->getOnstopPane(),
			LAB_onstop_pane, "onstop pane");
		rptr++;
	}
	dialog->add_component(box);
}

void
Panes_dialog::add_radio(Radio_descriptor *rptr, Packed_box *box,
		Table *t, LabelId clabel, char *rlabel)
{
	Caption		*caption;

	static const LabelId choices[] =
	{
		LAB_truncate,
		LAB_wrap,
	};

	caption = new Caption(box, clabel, CAP_LEFT);
	rptr->radio = new Radio_list(caption, rlabel,
		OR_horizontal, choices,
		sizeof(choices)/sizeof(LabelId), 0);
	rptr->table = t;
	rptr->state = TRUNCATED;
	caption->add_component(rptr->radio);
	box->add_component(caption);
}

void
Panes_dialog::apply(Component *, void *)
{
	Radio_descriptor	*rptr = radios;

	for(int i = 0; i < nradios; i++, rptr++)
	{
		rptr->state = rptr->radio->which_button();
		rptr->table->wrap_columns(rptr->state);
	}
}

void
Panes_dialog::reset(Component *, void *)
{
	Radio_descriptor	*rptr = radios;

	for(int i = 0; i < nradios; i++, rptr++)
	{
		rptr->radio->set_button(rptr->state);
	}
}

Base_window::Base_window(Window_set *ws, const Window_descriptor *wdesc)
	: COMMAND_SENDER(ws)
{
	Expansion_box	*box1;
	Divided_box	*box2 = 0;
	Pane_descriptor	**pdesc = wdesc->panes;
	int		i;
	const char	*wname;
	int		set_line_excl = 0;

	flags = 0;
	sense_count = 0;
	busy_count = 0;
	set_line_count = 0;
	selection_pane = 0;

	for(i = 0; i < PT_last; i++)
		panes[i] = 0;

	panes_dialog = 0;
	search_dialog = 0;
	win_desc = wdesc;

	wname = wdesc->name ? wdesc->name : labeltab.get_label(wdesc->label);
	window = new Window_shell(wname, 0, this, HELP_window);
	box1 = new Expansion_box(window, "expansion box", OR_vertical);
	menu_bar = new Menu_bar(box1, this, window_set, wdesc->menu_table,
		wdesc->nmenus);
	box1->add_component(menu_bar);
	window->add_component(box1);

	if (wdesc->flags & W_HAS_STATUS)
	{
		panes[PT_status] = (Pane *)new Status_pane(ws, this, 
			box1);
	}

	if (wdesc->nbuttons && !(wdesc->flags & W_BUTTONS_BOTTOM))
	{
		button_bar = new Button_bar(box1, this, ws, 
			(const Button_bar_table **)wdesc->button_table,
			wdesc->nbuttons);
		box1->add_component(button_bar);
	}
	else
		button_bar = 0; 

	if (wdesc->npanes > 2 || ((wdesc->npanes == 2) && 
		!(wdesc->flags & W_HAS_STATUS)) || 
		(wdesc->flags & W_HAS_EVENT))
	{
		// use divided box if more than 2 panes
		// or exactly two and neither is status
		// or if we have an event pane
		box2 = new Divided_box(box1, "divided_box");
		box1->add_component(box2, TRUE);
	}

	Box			*box = box2 ? (Box *)box2 : (Box *)box1;

	for (i = 0; i < wdesc->npanes; i++, pdesc++)
	{
		switch ((*pdesc)->type)
		{
		case PT_process:
			panes[PT_process] = (Pane *)new Ps_pane(ws,
				this, box, *pdesc);
			ws->ps_panes.add(panes[PT_process]);
			break;

		case PT_stack:
			panes[PT_stack] = (Pane *)new Stack_pane(ws, 
				this, box, *pdesc);
			break;

		case PT_symbols:
			panes[PT_symbols] = 
				(Pane *)new Symbols_pane(ws, this, 
					box, *pdesc);
			break;

		case PT_registers:
			panes[PT_registers] = 
				(Pane *)new Register_pane(ws, this, 
					box, *pdesc);
			break;

		case PT_status:
			break;

		case PT_event:
			panes[PT_event] = (Pane *)new Event_pane(ws,
				this, box, *pdesc);
			break;

		case PT_command:
			panes[PT_command] = 
				(Pane *)new Command_pane(ws, this, 
				box, *pdesc);
			break;

		case PT_disassembler:
			panes[PT_disassembler] = 
				(Pane *)new Disassembly_pane(ws,
				this, box, *pdesc);
			set_line_excl++;
			break;

		case PT_source:
			if (wdesc != &second_source_wdesc)
			{

				// primary source pane
				panes[PT_source] = (Pane *)new 
					Source_pane(ws, this, box, 
					*pdesc, TRUE);
				set_line_excl++;
			}
			else
			{
				// secondary source pane
				panes[PT_source] = (Pane *)new 
					Source_pane(ws, this, box,
					*pdesc, FALSE);
				ws->inc_open_windows();
				ws->source_windows.add(this);
			}
			break;

		default:
			display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
			break;
		}
	}

	if (set_line_excl >= 2)
		flags |= BW_SET_LINE_EXCLUSIVE;

	if (wdesc->nbuttons && (wdesc->flags & W_BUTTONS_BOTTOM))
	{
		button_bar = new Button_bar(box1, this, ws, 
			(const Button_bar_table **)wdesc->button_table, 
			wdesc->nbuttons);
		box1->add_component(button_bar);
	}

	// make sure the windows menu in our Files menu knows
	// about all of the window sets
	Window_set	*wptr = (Window_set *)windows.first();
	Menu 		*mp = menu_bar->find_item(LAB_fileb);
	int		my_id = window_set->get_id();
	if (!mp || (mp = mp->find_item(LAB_windows)) == 0)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}

	const char	*lab = labeltab.get_label(LAB_window_set);
	int		tlen = strlen(lab) + MAX_INT_DIGITS + 2;

	for ( ; wptr; wptr = (Window_set *)windows.next())
	{
		char	*title;
		int	ws_id = wptr->get_id();

		if (ws_id == my_id)
			// don't need to add ourselves
			continue;


		title = new char[tlen];
		sprintf(title, "%s %d", lab, ws_id);

		if (mp->find_item(title) == 0)
		{
			Menu_table item;

			item.name = title;
			item.label = LAB_none;
			item.mnemonic = LAB_none;
			if (ws_id < 10)
			{
				int mne;
				mne = '0' + ws_id;
				item.mnemonic_name = (mnemonic_t)mne;
			}
			else
				item.mnemonic_name = 0;
			item.flags = Set_data_cb;
			item.sensitivity = SEN_always;
			item.callback = (Callback_ptr)(&Window_set::popup_ws);
			item.help_msg = HELP_windows_menu;
			item.cdata = ws_id;
			mp->add_item(&item);
		}
	}
}

Base_window::~Base_window()
{
	if (win_desc == &second_source_wdesc)
	{
		// secondary source window
		// open_windows already decremented in
		// Window_set::dismiss
		window_set->source_windows.remove(this);
	}
	delete window;
	delete panes_dialog;
	delete search_dialog;
	if (panes[PT_process])
	{
		window_set->ps_panes.remove(panes[PT_process]);
	}
	for(int i = 0; i < PT_last; i++)
		delete panes[i];
}

void
Base_window::popup(Boolean grab_focus)
{
	if (flags & BW_IS_OPEN)
	{
		window->raise(grab_focus);
		return;
	}

	flags |= BW_IS_OPEN;
	if (flags & BW_SET_LINE_EXCLUSIVE)
		window_set->set_line_exclusive_wins.add(this);

	window->popup();
	set_sensitivity();

	for(int i = 0; i < PT_last; i++)
	{
		if (panes[i])
			panes[i]->popup();
	}

	window_set->change_current.add(this, (Notify_func)(&Base_window::update_state_cb), 0);
	if (panes[PT_process])
		window_set->change_state.add(this,
			(Notify_func)(&Base_window::update_state_cb), 0);
}

void
Base_window::popdown()
{
	flags &= ~BW_IS_OPEN;
	window->popdown();
	for(int i = 0; i < PT_last; i++)
	{
		if (panes[i])
			panes[i]->popdown();
	}

	if (flags & BW_SET_LINE_EXCLUSIVE)
		window_set->set_line_exclusive_wins.remove(this);

	window_set->change_current.remove(this, (Notify_func)(&Base_window::update_state_cb), 0);
	if (panes[PT_process])
		window_set->change_state.remove(this,
			(Notify_func)(&Base_window::update_state_cb), 0);
	if (win_desc == &second_source_wdesc)
	{
		// secondary source window
		delete this;
	}
}

void
Base_window::update_state_cb(void *, Reason_code rc, void *, ProcObj *proc)
{
	if (proc && proc->is_animated() && rc == RC_animate)
		return;

	if (rc == RC_delete || rc == RC_rename)
		return;

	set_sensitivity();
}

// There are 2 types of sensitivities: those that are
// of interest only to a particular pane, and those
// that are of global interest.  There are also two
// types of buttons (where sensitivity is concerned):
// those that require a selection and those that do not.
// For buttons that require a selection, we:
// 1. Check that there is a selection; if not, insensitive.
// 2. Check the selection pane for correct criteria.
// 3. Check global criteria.
// For those buttons not requiring a selection, we:
// 1. Check each pane that contributes sensitivity
//    criteria to this button.  If there is at least one,
//    than at least one of these sets of criteria must be
//    satisfied.
// 2. Check global criteria.
int
Base_window::check_sensitivity(int sense)
{
	// if window set contains a pane that is animated, everything
	// but Halt and Destroy should be insenstivive
	if (window_set->is_animated())
		return (sense & SEN_animated) ? 1 : 0;

	if (sense & SEN_sel_required)
	{
		if (!selection_pane ||
			!selection_pane->check_sensitivity(sense))
			return 0;
	}
	else
	{
		// no selection required
		// check each pane that might have contributed
		// requirements
		// Currently only source and dis panes have
		// requirements that do not require a selection

		if ((sense & SEN_dis_only) &&
			panes[PT_disassembler])
		{
			if (!panes[PT_disassembler]->check_sensitivity(sense))
				return 0;
		}
		if ((sense & SEN_source_only) &&
			panes[PT_source])
		{
			if (!panes[PT_source]->check_sensitivity(sense))
				return 0;
		}
	}
	if (sense & SEN_process)
	{
		if (selection_type() == SEL_process)
		{
			// applies to selected process(es)
			if (sense & SEN_process_sel)
				// already checked above
				return 1;
			else return(selection_pane->check_sensitivity(sense));
		}
		// applies to current process or thread 
		// since none was selected
		ProcObj *cur_obj = window_set->current_obj();

		if (!cur_obj)
			return 0;
#ifdef DEBUG_THREADS
		// current object is a thread , and it's the only 
		// one in the process
		if ((sense&SEN_proc_only) && 
			cur_obj->is_thread() &&
			window_set->get_command_level() == THREAD_LEVEL)
		{
			// we want a threaded process with only
			// 1 thread
			if (cur_obj->get_process()->get_head()->next())
				return 0;
		}
#endif
		if (!cur_obj->check_sensitivity(sense))
			return 0;
	}
	return 1;
}

// Note that set_sensitivity is not recursive - currently only three
// levels are supported.
// We use sense_count to avoid multiple sensitivity checks for
// a single operation.  The counter has basically three values:
// 0, 1, 2 (really > 1).
// If the counter is greater than 0, it
// is simply incremented, and we return.  Dispatcher::process_msg
// sets the count to 1 for all windows in the affected window
// set.  Before returning, it checks whether anyone has called
// set_sensitivity for each window (count > 1), sets the count
// to 0, and calls set_sensitivity for the windows where the
// count was greater than 1.  This ensures that sensitivity
// is set only after all operations on the panes of that window
// have been performed and is only called once.  It allows
// synchronous calls (select_cb) to happen naturally, since
// count will be 0 for these.

void
Base_window::set_sensitivity()
{
	if (!(flags & BW_IS_OPEN))
		return;

	switch(sense_count)
	{
	case 0:
		break;
	case 1:
		sense_count++;
		// FALLTHROUGH
	default:
		// never really need to know if greater than
		// 2; avoid problem of wrap around by simply
		// stopping at 2
		return;
	}

	Menu	**menup = menu_bar->get_menus();
	int	total = menu_bar->get_nbuttons();
	int	i, j;

	for (i = 0; i < total; i++, menup++)
	{
		const Menu_table	*table = (*menup)->get_table();

		for (j = 0; j < (*menup)->get_nbuttons(); j++, table++)
			(*menup)->set_sensitive(j, is_sensitive(table->sensitivity));

		Menu *sub_menu = (*menup)->first_child();
		for ( ; sub_menu; sub_menu = (*menup)->next_child())
		{
			table = sub_menu->get_table();
			for (j = 0; j < sub_menu->get_nbuttons(); j++, table++)
				sub_menu->set_sensitive(j,
					is_sensitive(table->sensitivity));
		}
	}
	if (button_bar)
	{
		const Button_bar_table	**btable = button_bar->get_table();
		total = button_bar->get_nbuttons();
		for(i = 0; i < total; i++, btable++)
		{
			button_bar->set_sensitivity(i, 
				is_sensitive((*btable)->sensitivity));
		}
	}
}

int
Base_window::is_sensitive(int sense)
{
	if (sense == SEN_always)
		return 1;
	if (sense == SEN_all_but_script)
	{
		if (in_script)
			return 0;
		return 1;
	}

	return check_sensitivity(sense);
}

void
Base_window::de_message(Message *m)
{
	Msg_id	mtype = m->get_msg_id();

	if (Mtable.msg_class(mtype) == MSGCL_error)
		display_msg(m);
	else if (!gui_only_message(m))
		window->display_msg(m);
}

void
Base_window::set_selection(Pane *pane)
{
	if (selection_pane && selection_pane != pane)
		selection_pane->deselect();
	selection_pane = pane;
	set_sensitivity();
}

void
Base_window::inc_busy()
{
	if (!busy_count)
	{
		window->set_busy(TRUE);
	}
	++busy_count;
}

void
Base_window::dec_busy()
{
	if (!busy_count)
		return;
	--busy_count;
	if (!busy_count)
	{
		window->set_busy(FALSE);
	}
}

char *
Base_window::get_selection()
{
	if (selection_pane)
		return selection_pane->get_selection();
	return 0;
}

int
Base_window::get_selections(Vector *v)
{
	if (selection_pane)
		return selection_pane->get_selections(v);
	return 0;
}

Selection_type
Base_window::selection_type()
{
	if (selection_pane)
		return selection_pane->selection_type();
	return SEL_none;
}

void 
Base_window::copy_cb(Component *, void *)
{
	if (!selection_pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	selection_pane->copy_selection();
}

void 
Base_window::set_break_cb(Component *comp, void *cdata)
{
	if (!selection_pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	switch(selection_pane->get_type())
	{
	case PT_source:
		((Source_pane *)selection_pane)->set_break_cb(comp, cdata);
		break;
	case PT_disassembler:
		((Disassembly_pane *)selection_pane)->set_break_cb(comp, cdata);
		break;
	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

void 
Base_window::delete_break_cb(Component *comp, void *cdata)
{
	if (!selection_pane)
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;
	}
	switch(selection_pane->get_type())
	{
	case PT_source:
		((Source_pane *)selection_pane)->delete_break_cb(comp, cdata);
		break;
	case PT_disassembler:
		((Disassembly_pane *)selection_pane)->delete_break_cb(comp, cdata);
		break;
	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

void 
Base_window::setup_panes_cb(Component *, void *)
{
	if (!panes_dialog)
		panes_dialog = new Panes_dialog(this);
	panes_dialog->display();
}

void
Base_window::search_dialog_cb(Component *, void *)
{
	if (!search_dialog)
		search_dialog = new Search_dialog(this);

	switch (selection_type())
	{
	case SEL_src_line:
	case SEL_regs:
	case SEL_instruction:
		search_dialog->set_string(truncate_selection(get_selection()));
		break;

	default:
		break;
	}

	search_dialog->display();
}

// called when help button is pressed for help on a specific pane
void
Base_window::help_sect_cb(Component *, void *h)
{
	Help_id help = (Help_id)h;

	if(!help || help > HELP_final)
		return;
	display_help(window->get_widget(), HM_section, help);
}
