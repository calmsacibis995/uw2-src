/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_MENU_H
#define	_MENU_H
#ident	"@(#)debugger:gui.d/common/Menu.h	1.20"

#include "Component.h"
#include "List.h"
#include "MenuP.h"
#include "Menu_barP.h"
#include "Panes.h"
#include "gui_label.h"
#ifdef MULTIBYTE
#include <stdlib.h>  // for wchar_t
typedef	wchar_t		mnemonic_t;
#else
typedef unsigned char	mnemonic_t;
#endif

class Window_set;
class Base_window;

// The flags field in Menu_table describes what type of framework object
// the button's callback references, or if the button brings up another menu
// Callbacks are invoked as
// Pane_cb:	pane->function((Menu *)this, table->cdata)
// Window_cb:	base_win->function((Menu *)this, table->cdata)
// Set_cb:	window_set->function((Menu *)this, window)
// Set_data_cb:	window_set->function((Menu *)this, table->cdata)

#define	Pane_cb		1	// individual pane
#define Window_cb	2	// base window
#define	Set_cb		4	// window set, with window where button was pushed
#define	Menu_button	8	// cascade, brings up another menu
#define Set_data_cb	16	// window set, with call data

#define	SEN_always		0xffffffff	// all bits set == always available
// the following used by the event pane
// all require a selection
#define	SEN_event_sel		0x00000001	// 1 or more events selected
#define	SEN_process_sel		0x00000002	// 1 or more processes selected
#define	SEN_symbol_sel		0x00000004	// 1 or more symbols selected
#define	SEN_source_sel		0x00000008	// 1 or more source lines selected
#define	SEN_dis_sel		0x00000010	// 1 or more instructions selected
#define	SEN_frame_sel		0x00000020	//  stack frame selected
#define	SEN_text_sel		0x00000040	// text selected

#define SEN_sel_required 	0x0000007f	// some selection required

// The following further define the type of selection required
#define	SEN_single_sel		0x00000080	// single selection only

#define SEN_user_symbol		0x00000100	// selection is user symbol
#define SEN_program_symbol	0x00000200	// selection is program symbol
#define SEN_sel_has_pin_sym	0x00000400	// selections contain pin symbol
#define SEN_sel_has_unpin_sym	0x00000800	// selections contain no pin symbols
#define	SEN_event_dis_sel	0x00001000	// selections are disabled events
#define	SEN_event_able_sel	0x00002000	// selections are enabled events
#define SEN_breakpt_required    0x00004000	// The selected line must have a breakpoint set

// the following apply even if no selection required
#define SEN_file_required       0x00008000	// The source window must have a current file
#define SEN_source_only      	SEN_file_required
#define	SEN_disp_dis_required	0x00010000 	// display of instruction 
#define SEN_dis_only      	SEN_disp_dis_required

// the following apply to the selected or current process
#define SEN_process		0x00020000	// program, process, or thread
#define SEN_proc_running	0x00040000	// process must be running
#define SEN_proc_stopped	0x00080000	// process must be stopped
#define SEN_proc_runnable	0x00100000	// process must be stopped and runnable
#define	SEN_proc_live		0x00200000	// live process (non-core) required
						// live includes both running and
						// stopped
#define SEN_proc_single		0x00400000	// single process only
#define SEN_proc_only		0x00800000	// must not be threads
#define SEN_proc_stopped_core	0x01000000	// process must be stopped or a core file
#define SEN_proc_io_redirected	0x02000000	// process IO is through a pseudo-terminal

// The following are global states
#define SEN_animated		0x04000000	// sensitive during either source or dis animation
#define SEN_all_but_script	0x08000000	// always sensitive except while
						// executing a script

// descriptor for the buttons in the individual menus
struct Menu_table
{
	LabelId		label;		// button label
	LabelId		mnemonic; 	// for mouseless operations - should be unique
				   	// within the menu
	unsigned char	flags;		// callback type
	Pane_type 	pane_type;	// if pane-specific, which pane?
	unsigned int	sensitivity;	// determines button's availability
	Callback_ptr	callback;	// function called when this button is selected
	Help_id		help_msg;	// context sensitive help, may be 0
	int		cdata;		// client data used in callbacks, or
					// number of buttons for cascading menus
	const char	*name;
					// if label field == LAB_none,
					// name contains  
					// user-defined name
	const Menu_table *sub_table;	// secondary menu, only when flags == Menu_button
	mnemonic_t	mnemonic_name;  // if mnemonic field == LAB_none,
					// mneomonic_name contains
					// the mnemonic
};

// descriptor for each entry in the menu bar
struct Menu_bar_table
{
	LabelId			label;		// button label
	const Menu_table	*table;		// menu's descriptor
	LabelId			mnemonic;	// must be unique within menu bar
	int			nbuttons;	// number of buttons in the menu
	Help_id			help_msg;	// context sensitive help, may be 0
};

class Menu : public Component
{
	MENU_TOOLKIT_SPECIFICS

private:
	Base_window		*window;
	Window_set		*window_set;
	int			nbuttons;
	List			children;
	LabelId			mlabel;
	const Menu_table	*table;

public:
				Menu(Component *parent, LabelId l,
					Boolean title, Widget pwidget,
					Base_window *window, Window_set *window_set,
					const Menu_table *, int num_buttons,
					Help_id help_msg = HELP_none);
				~Menu();

				// access functions
	Base_window		*get_window()		{ return window; }
	Window_set		*get_window_set()	{ return window_set; }
	int			get_nbuttons()		{ return nbuttons; }
	const Menu_table	*get_table()		{ return table; }
	Menu			*find_item(LabelId);
	Menu			*find_item(const char *);
	void			add_item(const Menu_table *);
	void			delete_item(const char *);

	Menu			*first_child()	{ return (Menu *)children.first(); }
	Menu			*next_child()	{ return (Menu *)children.next(); }

				// set the sensitivity of an individual button
	void			set_sensitive(int button, Boolean);
	LabelId			get_label() { return mlabel; }
};

class Menu_bar : public Component
{
	MENUBAR_TOOLKIT_SPECIFICS

private:
	const Menu_bar_table *bar_table;
	int		nbuttons;
	Menu		**children;

public:
			Menu_bar(Component *parent, Base_window *window,
				Window_set *window_set, const Menu_bar_table *,
				int nbuttons, Help_id help_msg = HELP_none);
			~Menu_bar();

			// access functions
	Menu		**get_menus()	{ return children; }
	int		get_nbuttons()	{ return nbuttons; }
	Menu		*find_item(LabelId);
};

#endif	// _MENU_H
