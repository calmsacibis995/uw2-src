/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef BUTTON_BAR_H
#define BUTTON_BAR_H

#ident	"@(#)debugger:gui.d/common/Button_bar.h	1.7"

#include "Button_barP.h"
#include "Panes.h"
#include "gui_label.h"
#include "Label.h"

enum CButtons {
	B_set_current,
	B_animate_src,
	B_animate_dis,
	B_disable,
	B_enable,
	B_delete,
	B_input,
	B_interrupt,
	B_run,
	B_return,
	B_step_stmt,
	B_next_stmt,
	B_step_inst,
	B_next_inst,
	B_halt,
	B_destroy,
	B_set_watchpt,
	B_sym_pin,
	B_sym_unpin,
	B_export,
	B_expand,
	B_popup,
	B_last
};

// descriptor for the buttons in the button_bar
struct Button_bar_table
{
	CButtons	type;
	LabelId		labelid;
	unsigned int	sensitivity;	// determines button sensitivity
	Callback_ptr	callback;	// function called this button is selected
	Help_id		help_msg;	// context_sensitive help, maybe 0
	unsigned char	flags;		// callback flags
	Pane_type	pane_type;	// for Pane_cb
	int		cdata;		// client data for callback
	char		*window;	// window name for popup buttons
	char		*label;		// label for user defined names
	const char	*name() { return label ?
					(const char *)label :
					labeltab.get_label(labelid); }
};

class Button_bar : public Component
{
	BUTTON_BAR_TOOLKIT_SPECIFICS

private:
	Base_window		*window;
	Window_set		*window_set;
	int			nbuttons;
	const Button_bar_table	**table;

public:
				Button_bar(Component *parent,
					Base_window *w,
					Window_set *window_set,
					const Button_bar_table **tab,
					int num_buttons,
					Help_id help_msg = HELP_none);
				~Button_bar();

				// access functions
	Base_window		*get_window() { return window; }
	Window_set		*get_window_set() { return window_set; }
	int			get_nbuttons() { return nbuttons; }
	const Button_bar_table	**get_table() { return table; }

				// set sensitivity of an individual button
	void			set_sensitivity(int button, Boolean sense);
};

#endif /* BUTTON_BAR_H*/
