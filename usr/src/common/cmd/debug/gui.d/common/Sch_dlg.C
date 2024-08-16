#ident	"@(#)debugger:gui.d/common/Sch_dlg.C	1.8"

#include "Boxes.h"
#include "Sch_dlg.h"
#include "Dialog_sh.h"
#include "Text_disp.h"
#include "Text_line.h"
#include "Caption.h"
#include "Dis.h"
#include "Panes.h"
#include "Radio.h"
#include "Source.h"
#include "Windows.h"
#include "UI.h"
#include "Window_sh.h"
#include "gui_label.h"

// debug headers
#include "Message.h"
#include "Msgtab.h"
#include "str.h"

// choices for search
#define SOURCE	0
#define DIS	1

Search_dialog::Search_dialog(Base_window *win) : DIALOG_BOX(win->get_window_set())
{
	static const Button	buttons[] =
	{
		{ B_non_exec, LAB_forward, LAB_forward_mne,
			(Callback_ptr)(&Search_dialog::apply) },
		{ B_non_exec, LAB_backward, LAB_backward_mne,
			(Callback_ptr)(&Search_dialog::apply) },
		{ B_cancel, LAB_none, LAB_none,
			(Callback_ptr)(&Search_dialog::cancel) },
		{ B_help, LAB_none, LAB_none,0 },
	};

	Caption			*caption;
	Packed_box		*box;

	save_string = 0;

	dialog = new Dialog_shell(window_set->get_window_shell(),
			LAB_search, 0, this, 
			buttons, sizeof(buttons)/sizeof(Button),
			 HELP_search_dialog);

	box = new Packed_box(dialog, "search", OR_vertical);

	radio = 0;
	source = (Source_pane *)win->get_pane(PT_source);
	dis = (Disassembly_pane *)win->get_pane(PT_disassembler);

	if (!source && !dis)
	{
		display_msg(E_ERROR, GE_internal, __FILE__,
			__LINE__);
		return;
	}
	if (source && dis)
	{
		// must ask user for which one is intended
		static const LabelId choices[] =
		{
			LAB_source,
			LAB_disassembly,
		};
		caption = new Caption(box, LAB_search_line, CAP_LEFT);
		radio = new Radio_list(caption, 
			"search pane", OR_horizontal, choices,
			sizeof(choices)/sizeof(LabelId), 0);
		choice = SOURCE;
		caption->add_component(radio);
		box->add_component(caption);
	}
	else if (dis)
		choice = DIS;
	else
		choice = SOURCE;
	caption = new Caption(box, LAB_text_line, CAP_LEFT);
	string = new Text_line(caption, "text", "", 15, TRUE);
	caption->add_component(string);
	box->add_component(caption);
	dialog->add_component(box);
	dialog->set_popup_focus(string);
}

void
Search_dialog::apply(Component *, int mnemonic)
{
	char	*s = string->get_text();

	if (!s || !*s)
	{
		dialog->error(E_ERROR, GE_no_reg_expr);
		return;
	}

	delete save_string;
	save_string = makestr(s);

	Search_return	sr = SR_notfound;

	if (radio)
		choice = radio->which_button();
	// warn the user if the base window is not on the screen 
	if (choice == SOURCE)
	{
		if (source->get_parent()->is_open())
			sr = source->get_pane()->search(s, mnemonic == 'F');
		else
		{
			dialog->error(E_ERROR, GE_no_source_win);
			return;
		}

	}
	else if (choice == DIS)
	{
		if (dis->get_parent()->is_open())
			sr = dis->get_pane()->search(s, mnemonic == 'F');
		else
		{
			dialog->error(E_ERROR, GE_no_disasm_win);
			return;
		}
	}
	else
	{
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		return;	
	}

	switch(sr)
	{
	case SR_found:
		break;

	case SR_notfound:
		dialog->error(E_ERROR, GE_expr_not_found);
		break;

	case SR_bad_expression:
		dialog->error(E_ERROR, GE_bad_expr);
		break;

	default:
		display_msg(E_ERROR, GE_internal, __FILE__, __LINE__);
		break;
	}
}

void
Search_dialog::cancel(Component *, void *)
{
	if (radio)
		radio->set_button(choice);
	string->set_text(save_string);
}

void
Search_dialog::set_string(const char *s)
{
	string->set_text(s);
}
