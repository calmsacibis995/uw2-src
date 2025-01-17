/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	_WINDOWS_H
#define	_WINDOWS_H
#ident	"@(#)debugger:gui.d/common/Windows.h	1.37"

#include "Component.h"
#include "Dialogs.h"
#include "Notifier.h"
#include "Sender.h"
#include "List.h"
#include "Base_win.h"
#include "Reason.h"
#include "gui_msg.h"

// Identifies button that was pressed in the Control Menu
enum Control_type
{
	CT_run,
	CT_return,
	CT_step_statement,
	CT_step_instruction,
	CT_next_statement,
	CT_next_instruction,
	CT_halt,
	CT_release_running,
	CT_release_suspended,
	CT_destroy,
};

// Identifies action to be taken when an event notification is
// received or a running process generates output
enum Action
{
	A_raise,
	A_beep,
	A_message,
	A_none,
};

// Thread state change actions
#define	TCA_stop	1
#define TCA_beep	2

// Animation states
enum Animate_state 
{ 
	AS_step_start,	// step cmd issued
	AS_step_done,	// step finished
};


class Buffer;
class Base_window;
class Event_pane;
class Window_shell;
class Window_set;
class Message;
class ProcObj;
class Process;
class Plink;
class Stop_dialog;
class Signal_dialog;
class Symbols_pane;
class Syscall_dialog;
class Onstop_dialog;
class Cancel_dialog;
class Kill_dialog;
class Setup_signals_dialog;
class Search_dialog;
class Stop_on_function_dialog;
class Dump_dialog;
class Map_dialog;
class Expand_dialog;
class Timer;
class Transcript;
class Window_descriptor;

// These dialogs are shared - used by more than one window - so they
// are with the Window_set instead of a specific window

class Set_value_dialog : public Process_dialog
{
	Text_line	*expression;
	char		*save_expr;
public:
			Set_value_dialog(Window_set *);
			~Set_value_dialog() { delete save_expr; };

			// callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

			// initialization
	void		set_expression(const char *);

			// functions inherited from Dialog_box
	void		cmd_complete();

			// inherited from Process_dialog
	void		set_obj(Boolean);
};

class Show_value_dialog : public Process_dialog
{
	Text_line	*expression;
	Text_line	*format_line;
	Text_area	*result;
	Radio_list	*formats;
	Buffer		*buffer;
public:
			Show_value_dialog(Window_set *);
			~Show_value_dialog() {};

			// initialization routines
	void		set_expression(const char *);
	void		clear_result();
	const char	*get_format();

			// callbacks
	void		do_it(Component *, void *);
	void		set_format_type(Component *, void *);

			// functions inherited from Dialog_box
	void		de_message(Message *);
	void		cmd_complete();

			// inherited from Process_dialog
	void		update_obj(ProcObj *);
	void		set_obj(Boolean);
};

class Show_type_dialog : public Process_dialog
{
	Text_line	*expression;
	Text_area	*result;
public:
			Show_type_dialog(Window_set *);
			~Show_type_dialog() {};

			// initialization routines
	void		set_expression(const char *);
	void		clear_result();

			// callbacks
	void		apply(Component *, void *);

			// functions inherited from Dialog_box
	void		de_message(Message *);
	void		cmd_complete();
};

class Run_dialog : public Process_dialog
{
	Text_line	*location;
	char		*save_string;
public:

			Run_dialog(Window_set *);
			~Run_dialog() { delete save_string; }

			// display functions and callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

			// initialization
	void		set_location(const char *);
};

class Step_dialog : public Process_dialog
{
	Text_line	*count;
	Radio_list	*type;
	Radio_list	*times;
	Toggle_button	*over;
	char		*save_count;
	int		type_state;
	int		times_state;
	Boolean		step_over;
public:

			Step_dialog(Window_set *);
			~Step_dialog() { delete save_count; }

			// callbacks
	void		apply(Component *, void *);
	void		reset(Component *, void *);

	void		set_times(Component *, void *);
};

class Jump_dialog : public Process_dialog
{
	Text_line	*location;
	char		*save_string;
public:

			Jump_dialog(Window_set *);
			~Jump_dialog() { delete save_string; }

			// display functions and callbacks
	void		apply(Component *, void *);
	void		cancel(Component *, void *);

			// initialization
	void		set_location(const char *);
};

class Path_dialog : public Process_dialog
{
	Text_area	*path_area;
	Radio_list	*choice;
	int		state;
public:
			Path_dialog(Window_set *);
			~Path_dialog() {}

			// callbacks
	void		apply(Component *, void *);
	void		reset(Component *, void *);
	void		set_path_type(Radio_list *, int);

			// inherited from Process_dialog
	void		set_obj(Boolean);
	void		cmd_complete();
};

class Granularity_dialog : public Dialog_box
{
	Radio_list	*event_choice;
	Radio_list	*command_choice;
	int		event_state;
	int		command_state;
public:
			Granularity_dialog(Window_set *);
			~Granularity_dialog() {}

			// callbacks
	void		apply(Component *, void *);
	void		reset(Component *, void *);
};

// The Action dialog allows the user to specify how the GUI will behave
// when it gets either an event notification or output from a process
// (the two actions are separately controllable).  The choices are:
// beeping, raising the command window, bringing up an alert box,
// or doing nothing.  In any case, the event messages and output are
// always displayed in the transcript pane.
// For threads, there are two additional buttons to specify how the
// GUI will behave when a thread state transition occurs. The choices are
// to beep or not, and to stop or not.

class Action_dialog : public Dialog_box
{
	Radio_list	*event_action;
	Radio_list	*output_action;
#ifdef DEBUG_THREADS
	Toggle_button	*thread_action;
#endif
public:
			Action_dialog(Window_set *);
			~Action_dialog() {};

			// Button callbacks
	void		apply(Component *, void *);
	void		reset(Component *, void *);
};

// The Animation dialog lets the user control the length of
// the interval between steps.  The default is no delay.
// The user may slow the stepping down as far as one second
// per step.

class Animation_dialog : public Dialog_box
{
	Slider		*slider;

public:
			Animation_dialog(Window_set *);
			~Animation_dialog() {}

			// Button callbacls
	void		apply(Component *, void *);
	void		reset(Component *, void *);
};

class Window_set : public Command_sender
{
	Base_window		**base_windows;
	Window_shell		*first_window;

	Pane			*animated_pane;

	
				// shared dialogs - only allocated as needed
	Create_dialog		*create_box;
	Grab_process_dialog	*grab_process;
	Grab_core_dialog	*grab_core;
	Set_language_dialog	*set_language;
	Set_value_dialog	*set_value;
	Show_value_dialog	*show_value;
	Show_type_dialog	*show_type;
	Run_dialog		*run_box;
	Step_dialog		*step_box;
	Jump_dialog		*jump_box;
	Stop_dialog		*stop_box;
	Signal_dialog		*signal_box;
	Syscall_dialog		*syscall_box;
	Onstop_dialog		*onstop_box;
	Cancel_dialog		*cancel_box;
	Kill_dialog		*kill_box;
	Setup_signals_dialog	*setup_signals;
	Path_dialog		*path_box;
	Granularity_dialog	*granularity_box;
	Cd_dialog		*cd_box;
	Action_dialog		*action_box;
	Animation_dialog	*animation_box;
	Dump_dialog		*dump_box;
	Map_dialog		*map_box;
	Expand_dialog		*expand_box;
	Stop_on_function_dialog	*stop_on_function_dialog;
	Move_dialog		*move_dlg;

				// process list - subset of global proclist
	Plink			*head;
	Plink			*tail;
	ProcObj			*cur_obj;

	Action			event_action;
	Action			output_action;
#ifdef DEBUG_THREADS
	unsigned char		thread_action;
#endif
	unsigned char		event_level;	// granularity
	unsigned char		command_level;
	unsigned char		id;
	unsigned char		step_interval;
	Boolean			animated;
	short			open_windows;

	Animate_state		animate_state;
	Timer			*timer;
	Transcript		*transcript;

	void			add_to_transcript(Message *, int, int);

	void			apply_to_obj(Control_type, Base_window *);
	Plink			*find_add_location(int &, ProcObj *);
	Plink			*add_to_list(Plink *, ProcObj *);
	void			get_name_components(const char *, long &, int &);
	void			update_ps_panes(Reason_code, int slot, 
					ProcObj *);

	void			make_windows();
	void			add_ws(int id);
	void			delete_ws(int id);
	void			change_current_notify(Reason_code, ProcObj *);

public:
	Notifier	change_current;
	Notifier	change_any;
	Notifier	change_state;	// global actions like script or animate
	List		source_windows;
	List		command_panes;
	List		ps_panes;
	List		set_line_exclusive_wins;

			Window_set();
			~Window_set();

			// access functions
	ProcObj		*current_obj()		{ return cur_obj; }
	Plink		*get_obj_list()		{ return head; }
	int		get_id()		{ return id; }
	void		inc_open_windows()	{ open_windows++; }
	void		dec_open_windows()	{ open_windows--; }
	void		set_event_action(Action a) { event_action = a; }
	Action		get_event_action()	{ return event_action; }
	void		set_output_action(Action a) { output_action = a; }
	Action		get_output_action()	{ return output_action; }
	Window_shell		*get_window_shell()	{ return first_window; }
#ifdef DEBUG_THREADS
	void		set_thread_action(unsigned char a) { thread_action = a; }
	unsigned char	get_thread_action()	{ return thread_action; }
#endif
	unsigned char	get_event_level()	{ return event_level; }
	void		set_event_level(unsigned char l)	{ event_level = l; }
	unsigned char	get_command_level()	{ return command_level; }
	void		set_command_level(unsigned char l) { command_level = l; }
	Stop_dialog	*get_stop_box()		{ return stop_box; }
	Signal_dialog	*get_signal_box()	{ return signal_box; }
	Syscall_dialog	*get_syscall_box()	{ return syscall_box; }
	Onstop_dialog	*get_onstop_box()	{ return onstop_box; }
	void		set_stop_box(Stop_dialog *d) { stop_box = d; }
	void		set_signal_box(Signal_dialog *d) { signal_box = d; }
	void		set_syscall_box(Syscall_dialog *d) { syscall_box = d; }
	void		set_onstop_box(Onstop_dialog *d) { onstop_box = d; }
	Boolean		is_animated()	{ return animated; }
	int		get_interval()	{ return step_interval; }
	void		set_interval(int interval)	{ step_interval = interval; }
	Base_window	**get_base_windows()	{ return base_windows; }

			// functions called by proclist to keep window set's process
			// list up to date
	void		add_obj(ProcObj *, Boolean make_current);
#ifdef DEBUG_THREADS
	void		add_process(Process *, Boolean make_current);
	void		delete_process(Process *);
#else
	void		add_process(Process *p, Boolean make_current) { add_obj((ProcObj *)p, make_current); }
	void		delete_process(Process *p)	{ delete_obj((ProcObj *)p); }
#endif
	void		rename_process(Process *);
	void		delete_obj(ProcObj *);
	void		update_obj(ProcObj *);
	Plink		*find_obj(ProcObj *, int &index);
	void		set_frame(ProcObj *);
	int		get_frame(int frame, const char *&function,
				const char *&location);
	void		clear_animation();
	void		common_animate(Base_window *, Gui_msg_id);
	void		command_add_text(char *);

			// functions called by the Dispatcher to keep the
			// window set up to date
	void		inform(Message *, ProcObj *);
	void		update_create_dialog(const char *args);
	void		update_language_dialog();
	void		update_cd_dialog(const char *s);
	void		set_in_script(Boolean);

			// change the window set's process list
	void		set_current(ProcObj *);

			// display window, initialize if necessary
	void		popup_context();
	void		popup_command();
	void		clear_msgs();
	void		popup_window(int index, Boolean focus = TRUE);
	void		popup_ws(Component *, void *);
	void		init_sense_counts();
	void		set_sensitivity();

			// callbacks to bring up dialogs
	void		create_dialog_cb(Component *, Base_window *);
	void		grab_process_dialog_cb(Component *, Base_window *);
	void		grab_core_dialog_cb(Component *, Base_window *);
	void		set_language_dialog_cb(Component *, Base_window *);
	void		set_value_dialog_cb(Component *, Base_window *);
	void		show_value_dialog_cb(Component *, Base_window *);
	void		show_type_dialog_cb(Component *, Base_window *);
	void		dump_dialog_cb(Component *, Base_window *);
	void		map_dialog_cb(Component *, Base_window *);
	void		run_dialog_cb(Component *, Base_window *);
	void		step_dialog_cb(Component *, Base_window *);
	void		jump_dialog_cb(Component *, Base_window *);
	void		stop_dialog_cb(Component *, Base_window *);
	void		signal_dialog_cb(Component *, Base_window *);
	void		syscall_dialog_cb(Component *, Base_window *);
	void		onstop_dialog_cb(Component *, Base_window *);
	void		cancel_dialog_cb(Component *, Base_window *);
	void		kill_dialog_cb(Component *, Base_window *);
	void		setup_signals_dialog_cb(Component *, Base_window *);
	void		path_dialog_cb(Component *, Base_window *);
	void		set_granularity_cb(Component *, Base_window *);
	void		cd_dialog_cb(Component *, void *);
	void		action_dialog_cb(Component *, Base_window *);
	void		animation_dialog_cb(Component *, Base_window *);
	void		expand_dialog_cb(Component *, Base_window *);
	void		stop_on_function_cb(Component *, Base_window *);
	void		move_to_ws_cb(Component *, Base_window *);
	void		popup_window_cb(Component *, int);

			// callbacks for simple command buttons
	void		run_button_cb(Component *, Base_window *);
	void		run_r_button_cb(Component *, Base_window *);
	void		step_button_cb(Component *, Base_window *);
	void		step_i_button_cb(Component *, Base_window *);
	void		step_o_button_cb(Component *, Base_window *);
	void		step_oi_button_cb(Component *, Base_window *);
	void		halt_button_cb(Component *, Base_window *);
	void		destroy_process_cb(Component *, Base_window *);
	void		release_running_cb(Component *, Base_window *);
	void		release_suspended_cb(Component *, Base_window *);
	void		version_cb(Component *, Base_window *);
	void		help_toc_cb(Component *, Base_window *);
	void		help_helpdesk_cb(Component *, Base_window *);
	void		animate_src_cb(Component *, Base_window *);
	void		animate_dis_cb(Component *, Base_window *);
	void		timer_cb(Component *, void *);
	void		new_window_set_cb(Component *, Base_window *);
	void		set_current_cb(Component *, Base_window *);

	void		animate_cleanup();

			// functions inherited from Command_sender
	void		de_message(Message *);

			// close and destroy the window set or a single window
	void		dismiss(Component *, Base_window *);
	void		ok_to_quit();
	void		quit_cb(Component *, int);
	void		send_interrupt_cb(Component *, void *);
};

extern	List		windows;	// the list of all window sets
extern  char 		*truncate_selection(char *);

#endif // _WINDOWS_H
