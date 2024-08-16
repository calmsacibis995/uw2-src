#ident	"@(#)debugger:gui.d/common/config.C	1.16"

// GUI headers
#include "Dis.h"
#include "Events.h"
#include "Source.h"
#include "Command.h"
#include "Menu.h"
#include "Button_bar.h"
#include "Panes.h"
#include "Ps_pane.h"
#include "Syms_pane.h"
#include "Stack_pane.h"
#include "config.h"
#include "Windows.h"

// Debug headers
#include "Machine.h"
#include "str.h"

#include <stdlib.h>
#include <ctype.h>

#ifdef MULTIBYTE
#include <wctype.h>
#endif

#define MAX_MERGE	((int)PT_last + 2)
				// 9 panes + default menu + 
				// primary window menu
#define MAX_MENUS	(sizeof(default_menus)/sizeof(Menu_bar_table))

static Menu_table def_file_pane[] =
{
	{ LAB_windows, LAB_windows_mne, Menu_button, PT_last, SEN_always, 0, 
		HELP_windows_menu },
	{ LAB_dismiss, LAB_dismiss_mne, Set_cb,  PT_last, SEN_always,	
		(Callback_ptr)(&Window_set::dismiss),
		HELP_dismiss_cmd },
	{ LAB_exit, LAB_exit_mne, Set_cb,  PT_last, SEN_all_but_script,	
		(Callback_ptr)(&Window_set::ok_to_quit), HELP_exit_cmd },
};

static const Menu_table def_control_pane[] =
{
	{ LAB_run, LAB_run_mne, Set_cb, PT_last, (SEN_process|SEN_proc_runnable), 
		(Callback_ptr)(&Window_set::run_button_cb),
		HELP_run_cmd},
	{ LAB_return, LAB_return_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::run_r_button_cb),
		HELP_return_cmd },
	{ LAB_run_until_dlg, LAB_run_until_dlg_mne, Set_cb, PT_last,
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::run_dialog_cb),
		HELP_run_dialog },
	{ LAB_step_statement, LAB_step_statement_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::step_button_cb),
		HELP_step_stmt_cmd },
	{ LAB_step_instruction, LAB_step_instruction_mne, Set_cb, PT_last,
		(SEN_process|SEN_proc_runnable), 
		(Callback_ptr)(&Window_set::step_i_button_cb),
		HELP_step_instr_cmd },
	{ LAB_next_statement, LAB_next_statement_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::step_o_button_cb),
		HELP_next_stmt_cmd },
	{ LAB_next_instruction, LAB_next_instruction_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::step_oi_button_cb),
		HELP_next_instr_cmd },
	{ LAB_step_dlg, LAB_step_dlg_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::step_dialog_cb),
		HELP_step_dialog },
	{ LAB_jump_dlg, LAB_jump_mne, Set_cb, PT_last,
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::jump_dialog_cb),
		HELP_jump_dialog },
	{ LAB_halt, LAB_halt_mne, Set_cb, PT_last,
		(SEN_process|SEN_proc_running|SEN_animated),
		(Callback_ptr)(&Window_set::halt_button_cb),
		HELP_halt_cmd },
};

static const Menu_table def_event_pane[] =
{
	{ LAB_stop_on_func_dlg, LAB_stop_on_func_dlg_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_stopped|SEN_single_sel),
		(Callback_ptr)(&Window_set::stop_on_function_cb),
		HELP_stop_on_function_dialog },
	{ LAB_stop_dlg, LAB_stop_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_stopped),
		(Callback_ptr)(&Window_set::stop_dialog_cb),
		HELP_stop_dialog },
	{ LAB_signal_dlg, LAB_signal_dlg_mne,  Set_cb, PT_last, 
		(SEN_process|SEN_proc_stopped),
		(Callback_ptr)(&Window_set::signal_dialog_cb),
		HELP_signal_dialog },
	{ LAB_syscall_dlg, LAB_syscall_dlg_mne,  Set_cb, PT_last,
		(SEN_process|SEN_proc_stopped),
		(Callback_ptr)(&Window_set::syscall_dialog_cb),
		HELP_syscall_dialog },
	{ LAB_on_stop_dlg, LAB_on_stop_mne,  Set_cb, PT_last, 
		(SEN_process|SEN_proc_stopped),
		(Callback_ptr)(&Window_set::onstop_dialog_cb),
		HELP_on_stop_dialog },
	{ LAB_cancel_dlg, LAB_cancel_mne,  Set_cb, PT_last, 
		(SEN_process|SEN_proc_stopped|SEN_single_sel),
		(Callback_ptr)(&Window_set::cancel_dialog_cb),
		HELP_cancel_dialog },
	{ LAB_destroy, LAB_destroy_mne, Set_cb, PT_last, (SEN_process|SEN_animated),
		(Callback_ptr)(&Window_set::destroy_process_cb),
		HELP_destroy_cmd, },
	{ LAB_kill_dlg, LAB_kill_mne, Set_cb, PT_last, SEN_process|SEN_proc_live,
		(Callback_ptr)(&Window_set::kill_dialog_cb),
		HELP_kill_dialog },
	{ LAB_ignore_signals_dlg, LAB_ignore_signals_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_stopped|SEN_single_sel),
		(Callback_ptr)(&Window_set::setup_signals_dialog_cb),
		HELP_ignore_signals_dialog },
};

static const Menu_table def_help_pane[] =
{
	{ LAB_table_of_cont_hlp, LAB_table_of_cont_mne, Set_cb, PT_last, SEN_always,
		(Callback_ptr)(&Window_set::help_toc_cb) },
	{ LAB_help_desk_hlp, LAB_help_desk_mne, Set_cb, PT_last, SEN_always,
		(Callback_ptr)(&Window_set::help_helpdesk_cb) },
};

static const Menu_bar_table	default_menus[] =
{
	{ LAB_fileb, def_file_pane, LAB_fileb_mne, 
		sizeof(def_file_pane)/sizeof(Menu_table),
		HELP_file_menu, },
	{ LAB_edit, 0, LAB_edit_mne, 0, HELP_edit_menu },
	{ LAB_view, 0, LAB_view_mne, 0, HELP_view_menu },
	{ LAB_control, def_control_pane, LAB_control_mne,
		sizeof(def_control_pane)/sizeof(Menu_table),
		HELP_control_menu},
	{ LAB_event, def_event_pane, LAB_event_mne,
		sizeof(def_event_pane)/sizeof(Menu_table),
		HELP_event_menu },
	{ LAB_props, 0, LAB_props_mne, 0, HELP_properties_menu },
	{ LAB_help, def_help_pane, LAB_help_mne,
		sizeof(def_help_pane)/sizeof(Menu_table) },
};

static const Menu_table first_file_pane[] =
{
	{ LAB_create_dlg, LAB_create_dlg_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::create_dialog_cb),
		HELP_create_dialog },
	{ LAB_grab_core_dlg, LAB_grab_core_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::grab_core_dialog_cb),
		HELP_grab_core_dialog },
	{ LAB_grab_proc_dlg, LAB_grab_proc_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::grab_process_dialog_cb), 
		HELP_grab_process_dialog },
	{ LAB_cd_dlg, LAB_cd_mne, Set_cb, PT_last,
		SEN_all_but_script,
		(Callback_ptr)(&Window_set::cd_dialog_cb),
		HELP_cd_dialog },
};

static const Menu_table first_prop_pane[] =
{
	{ LAB_language_dlg, LAB_language_mne, Set_cb, PT_last, SEN_all_but_script,	
		(Callback_ptr)(&Window_set::set_language_dialog_cb),
		HELP_language_dialog },
	{ LAB_granularity_dlg, LAB_granularity_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::set_granularity_cb),
		HELP_granularity_dialog },
	{ LAB_output_dlg, LAB_output_mne, Set_cb, PT_last, SEN_all_but_script,	
		(Callback_ptr)(&Window_set::action_dialog_cb),
		HELP_action_dialog },
	{ LAB_path_dlg, LAB_path_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::path_dialog_cb),
		HELP_path_dialog },
};

static const Menu_table first_help_pane[] =
{
	{ LAB_version, LAB_version_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::version_cb) },
};

static const Menu_bar_table	first_window_menu[] =
{
	{ LAB_fileb, first_file_pane, LAB_fileb_mne,
		sizeof(first_file_pane)/sizeof(Menu_table), },
	{ LAB_props, first_prop_pane, LAB_props_mne,
		sizeof(first_prop_pane)/sizeof(Menu_table), },
	{ LAB_help, first_help_pane,  LAB_help_mne,
		sizeof(first_help_pane)/sizeof(Menu_table) },
};

// Process Pane
static const Menu_table release_pane[] =
{
	{ LAB_rel_running, LAB_rel_running_mne, Set_cb, PT_last, SEN_process|SEN_proc_live,
		(Callback_ptr)(&Window_set::release_running_cb),
		HELP_release_cmd },
	{ LAB_rel_susp, LAB_rel_susp_mne, Set_cb, PT_last, SEN_process|SEN_proc_only,
		(Callback_ptr)(&Window_set::release_suspended_cb),
		HELP_release_cmd },
};

static const Menu_table ps_file_pane[] =
{
	{ LAB_create_dlg, LAB_create_dlg_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::create_dialog_cb),
		HELP_create_dialog },
	{ LAB_grab_core_dlg, LAB_grab_core_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::grab_core_dialog_cb),
		HELP_grab_core_dialog },
	{ LAB_grab_proc_dlg, LAB_grab_proc_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::grab_process_dialog_cb), 
		HELP_grab_process_dialog },
	{ LAB_release, LAB_release_mne, Menu_button, PT_last, SEN_process,
		0, HELP_release_cmd, 
		sizeof(release_pane)/sizeof(Menu_table), 0,
		release_pane },
	{ LAB_new_window_set, LAB_new_window_set_mne, Set_cb, PT_last, SEN_all_but_script,	
		(Callback_ptr)(&Window_set::new_window_set_cb),
		HELP_new_window_set_cmd },
	{ LAB_move_dlg, LAB_move_dlg_mne, Set_cb, PT_last, SEN_process,
		(Callback_ptr)(&Window_set::move_to_ws_cb),
		HELP_move_dialog },
};

static const Menu_table ps_edit_pane[] =
{
	{ LAB_set_current, LAB_set_current_mne, Set_cb, PT_last, 
		(SEN_frame_sel|SEN_process_sel|SEN_single_sel),	
		(Callback_ptr)(&Window_set::set_current_cb),
		HELP_set_current_cmd },
};

static const Menu_table ps_view_pane[] =
{
	{ LAB_map_dlg, LAB_map_mne, Set_cb, PT_last, (SEN_process|SEN_single_sel),
		(Callback_ptr)(&Window_set::map_dialog_cb),
		HELP_map_dialog },
};

static const Menu_table ps_prop_pane[] =
{
	{ LAB_panes_dlg, LAB_panes_mne, Window_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Base_window::setup_panes_cb),
		HELP_panes_dialog },
	{ LAB_granularity_dlg, LAB_granularity_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::set_granularity_cb),
		HELP_granularity_dialog },
};

// for help on panes, help msg is in cdata field
static const Menu_table ps_help_pane[] =
{
	{ LAB_process_pane_hlp, LAB_process_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_none, HELP_ps_pane },
};

static const Menu_bar_table ps_menu_table[] =
{
	{ LAB_fileb, ps_file_pane, LAB_fileb_mne,
		sizeof(ps_file_pane)/sizeof(Menu_table), },
	{ LAB_edit, ps_edit_pane, LAB_edit_mne,
		sizeof(ps_edit_pane)/sizeof(Menu_table), },
	{ LAB_view, ps_view_pane, LAB_view_mne,
		sizeof(ps_view_pane)/sizeof(Menu_table), },
	{ LAB_props, ps_prop_pane, LAB_props_mne,
		sizeof(ps_prop_pane)/sizeof(Menu_table), },
	{ LAB_help, ps_help_pane, LAB_help_mne,
		sizeof(ps_help_pane)/sizeof(Menu_table) },
};

static const Menu_table stack_edit_pane[] =
{
	{ LAB_set_current, LAB_set_current_mne, Set_cb, PT_last, 
		(SEN_frame_sel|SEN_process_sel|SEN_single_sel),	
		(Callback_ptr)(&Window_set::set_current_cb),
		HELP_set_current_cmd },
};

static const Menu_table stack_prop_pane[] =
{
	{ LAB_panes_dlg, LAB_panes_mne, Window_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Base_window::setup_panes_cb),
		HELP_panes_dialog },
};

static const Menu_table stack_help_pane[] =
// for help on panes, help msg is in cdata field
{
	{ LAB_stack_pane_hlp, LAB_stack_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_none, HELP_stack_pane },
};

static const Menu_bar_table stack_menu_table[] =
{
	{ LAB_edit, stack_edit_pane, LAB_edit_mne,
		sizeof(stack_edit_pane)/sizeof(Menu_table) },
	{ LAB_props, stack_prop_pane, LAB_props_mne,
		sizeof(stack_prop_pane)/sizeof(Menu_table), },
	{ LAB_help, stack_help_pane, LAB_help_mne,
		sizeof(stack_help_pane)/sizeof(Menu_table) },
};

static const Menu_table syms_edit_pane[] =
{
	{ LAB_export, LAB_export_mne, Pane_cb, PT_symbols,
		SEN_user_symbol|SEN_sel_required,
		(Callback_ptr)(&Symbols_pane::export_syms_cb),
		HELP_export_cmd },
	{ LAB_pin, LAB_pin_mne, Pane_cb, PT_symbols,
		(SEN_symbol_sel|SEN_sel_has_unpin_sym),
		(Callback_ptr)(&Symbols_pane::pin_sym_cb),
		HELP_sym_pin_cmd },
	{ LAB_unpin, LAB_unpin_mne, Pane_cb, PT_symbols,
		(SEN_symbol_sel|SEN_sel_has_pin_sym),
		(Callback_ptr)(&Symbols_pane::unpin_sym_cb),
		HELP_sym_unpin_cmd },
};

static const Menu_table syms_view_pane[] =
{
	{ LAB_expand_dlg, LAB_expand_dlg_mne, Set_cb, PT_last,
		(SEN_program_symbol|SEN_single_sel|SEN_symbol_sel|SEN_source_sel),
		(Callback_ptr)(&Window_set::expand_dialog_cb),
		HELP_expand_dialog },
	{ LAB_show_value_dlg, LAB_show_value_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::show_value_dialog_cb),
		HELP_show_value_dialog },
	{ LAB_set_value_dlg, LAB_set_value_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::set_value_dialog_cb),
		HELP_set_value_dialog },
	{ LAB_show_type_dlg, LAB_show_type_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::show_type_dialog_cb),
		HELP_show_type_dialog },
	{ LAB_dump_dlg, LAB_dump_dlg_mne, Set_cb, PT_last,
		(SEN_process|SEN_single_sel|SEN_proc_stopped_core),
		(Callback_ptr)(&Window_set::dump_dialog_cb),
		HELP_dump_dialog },
};

static const Menu_table syms_event_pane[] =
{
    	{ LAB_set_watch, LAB_set_watch_mne, Pane_cb, PT_symbols, 
		(SEN_process|SEN_proc_stopped|SEN_program_symbol|SEN_symbol_sel),
		(Callback_ptr)(&Symbols_pane::set_watchpoint_cb),
		HELP_set_watchpoint_cmd },
};

static const Menu_table syms_prop_pane[] =
{
	{ LAB_panes_dlg, LAB_panes_mne, Window_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Base_window::setup_panes_cb),
		HELP_panes_dialog },
	{ LAB_symbols_dlg, LAB_symbols_mne, Pane_cb, PT_symbols, SEN_all_but_script,	
		(Callback_ptr)(&Symbols_pane::setup_syms_cb),
		HELP_symbols_dialog},
};

static const Menu_table syms_help_pane[] =
// for help on panes, help msg is in cdata field
{
	{ LAB_syms_pane_hlp, LAB_syms_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_none, HELP_syms_pane },
};

static const Menu_bar_table syms_menu_table[] =
{
	{ LAB_edit, syms_edit_pane, LAB_edit_mne,
		sizeof(syms_edit_pane)/sizeof(Menu_table), },
	{ LAB_view, syms_view_pane, LAB_view_mne,
		sizeof(syms_view_pane)/sizeof(Menu_table), },
	{ LAB_event, syms_event_pane, LAB_event_mne,
		sizeof(syms_event_pane)/sizeof(Menu_table), },
	{ LAB_props, syms_prop_pane, LAB_props_mne,
		sizeof(syms_prop_pane)/sizeof(Menu_table), },
	{ LAB_help, syms_help_pane, LAB_help_mne,
		sizeof(syms_help_pane)/sizeof(Menu_table) },
};

//Command
static const Menu_table cmd_file_pane[] =
{
	{ LAB_cd_dlg, LAB_cd_mne, Set_cb, PT_last,
		SEN_all_but_script,
		(Callback_ptr)(&Window_set::cd_dialog_cb),
		HELP_cd_dialog },
	{ LAB_script_dlg, LAB_script_dlg_mne, Pane_cb, PT_command, SEN_all_but_script,	
		(Callback_ptr)(&Command_pane::script_dialog_cb),
		HELP_script_dialog},
};

static const Menu_table cmd_edit_pane[] =
{
	{ LAB_copy, LAB_copy_mne, Window_cb, PT_last, SEN_text_sel,
		(Callback_ptr)(&Base_window::copy_cb),
		HELP_copy_cmd },
	{ LAB_input_dlg, LAB_input_mne, Pane_cb, PT_command, 
		(SEN_process|SEN_proc_live|SEN_proc_io_redirected|SEN_animated),	
		(Callback_ptr)(&Command_pane::input_dialog_cb),
		HELP_input_dialog },
	{ LAB_interrupt, LAB_interrupt_mne, Pane_cb, PT_command, SEN_always,
		(Callback_ptr)(&Command_pane::interrupt_cb),
		HELP_interrupt_cmd },
};

static const Menu_table cmd_prop_pane[] =
{
	{ LAB_output_dlg, LAB_output_mne, Set_cb, PT_last, SEN_all_but_script,	
		(Callback_ptr)(&Window_set::action_dialog_cb),
		HELP_action_dialog },
	{ LAB_path_dlg, LAB_path_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::path_dialog_cb),
		HELP_path_dialog },
	{ LAB_language_dlg, LAB_language_mne, Set_cb, PT_last, SEN_all_but_script,	
		(Callback_ptr)(&Window_set::set_language_dialog_cb),
		HELP_language_dialog },
};

static const Menu_table cmd_help_pane[] =
// for help on panes, help msg is in cdata field
{
	{ LAB_command_pane_hlp, LAB_command_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb),
		HELP_none,  HELP_command_pane },
};

static const Menu_bar_table cmd_menu_table[] =
{
	{ LAB_fileb, cmd_file_pane, LAB_fileb_mne,
		sizeof(cmd_file_pane)/sizeof(Menu_table), 
		HELP_file_menu },
	{ LAB_edit, cmd_edit_pane, LAB_edit_mne,
		sizeof(cmd_edit_pane)/sizeof(Menu_table),
		HELP_edit_menu },
	{ LAB_props, cmd_prop_pane, LAB_props_mne,
		sizeof(cmd_prop_pane)/sizeof(Menu_table),
		HELP_properties_menu },
	{ LAB_help, cmd_help_pane, LAB_help_mne,
		sizeof(cmd_help_pane)/sizeof(Menu_table) },
};

//Source
static const Menu_table src_file_pane[] =
{
	{ LAB_open_dlg, LAB_open_mne, Pane_cb, PT_source, SEN_process, 
		(Callback_ptr)(&Source_pane::open_dialog_cb),
		HELP_open_dialog },
	{ LAB_new_source, LAB_new_source_mne, Pane_cb, PT_source, SEN_process,
		(Callback_ptr)(&Source_pane::new_source_cb),
		HELP_new_source_cmd },
};

static const Menu_table src_view_pane[] =
{
	{ LAB_expand_dlg, LAB_expand_dlg_mne, Set_cb, PT_last,
		(SEN_program_symbol|SEN_single_sel|SEN_symbol_sel|SEN_source_sel),
		(Callback_ptr)(&Window_set::expand_dialog_cb),
		HELP_expand_dialog },
	{ LAB_show_line_dlg, LAB_show_line_dlg_mne, Pane_cb,	PT_source, SEN_file_required,
		(Callback_ptr)(&Source_pane::show_line_cb),
		HELP_show_line_dialog },
	{ LAB_show_func_source_dlg, LAB_show_func_source_mne, Pane_cb, PT_source, 
		SEN_process,
		(Callback_ptr)(&Source_pane::show_function_cb),
		HELP_show_source_function_dialog },
	{ LAB_search_dlg, LAB_search_mne, Window_cb, PT_last,
		(SEN_file_required|SEN_disp_dis_required),
		(Callback_ptr)(&Base_window::search_dialog_cb),
		HELP_search_dialog },
	{ LAB_show_value_dlg, LAB_show_value_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::show_value_dialog_cb),
		HELP_show_value_dialog },
	{ LAB_set_value_dlg, LAB_set_value_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::set_value_dialog_cb),
		HELP_set_value_dialog },
	{ LAB_show_type_dlg, LAB_show_type_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::show_type_dialog_cb),
		HELP_show_type_dialog },
};

static const Menu_table src_edit_pane[] =
{
	{ LAB_copy, LAB_copy_mne, Window_cb, PT_last, SEN_text_sel,
		(Callback_ptr)(&Base_window::copy_cb),
		HELP_copy_cmd },
};

static const Menu_table src_prop_pane[] =
{
	{ LAB_path_dlg, LAB_path_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::path_dialog_cb),
		HELP_path_dialog },
	{ LAB_language_dlg, LAB_language_mne, Set_cb, PT_last, SEN_all_but_script,	
		(Callback_ptr)(&Window_set::set_language_dialog_cb),
		HELP_language_dialog },
	{ LAB_granularity_dlg, LAB_granularity_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::set_granularity_cb),
		HELP_granularity_dialog },
	{ LAB_animation_dlg, LAB_animation_mne, Set_cb, PT_last, SEN_all_but_script, 
		(Callback_ptr)(&Window_set::animation_dialog_cb),
		HELP_animation_dialog },
};

static const Menu_table src_control_pane[] =
{
	{ LAB_animate_source, LAB_animate_source_mne, Set_cb, PT_last, 
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::animate_src_cb),
		HELP_animate_source_cmd },
};

static const Menu_table src_event_pane[] =
{
	{ LAB_set_break, LAB_set_break_mne, Window_cb, PT_last, 
		(SEN_process|SEN_proc_stopped|SEN_source_sel|SEN_dis_sel),
		(Callback_ptr)(&Base_window::set_break_cb),
		HELP_set_breakpoint_cmd },
	{ LAB_delete_break, LAB_delete_break_mne, Window_cb, PT_last,
		(SEN_process|SEN_proc_stopped|SEN_source_sel|SEN_breakpt_required|SEN_dis_sel),
		(Callback_ptr)(&Base_window::delete_break_cb),
		HELP_delete_breakpoint_cmd },
};

static const Menu_table src_help_pane[] =
// for help on panes, help msg is in cdata field
{
	{ LAB_source_pane_hlp, LAB_source_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_none, HELP_source_pane },
};

static const Menu_bar_table src_menu_table[] =
{
	{ LAB_fileb,src_file_pane, LAB_fileb_mne,
		sizeof(src_file_pane)/sizeof(Menu_table),
		HELP_file_menu },
	{ LAB_edit,src_edit_pane, LAB_edit_mne,
		sizeof(src_edit_pane)/sizeof(Menu_table),
		HELP_edit_menu },
	{ LAB_view,src_view_pane, LAB_view_mne,
		sizeof(src_view_pane)/sizeof(Menu_table),
		HELP_view_menu },
	{ LAB_control,src_control_pane, LAB_control_mne,
		sizeof(src_control_pane)/sizeof(Menu_table),
		HELP_control_menu },
	{ LAB_event,src_event_pane, LAB_event_mne,
		sizeof(src_event_pane)/sizeof(Menu_table),
		HELP_event_menu },
	{ LAB_props,src_prop_pane, LAB_props_mne,
		sizeof(src_prop_pane)/sizeof(Menu_table),
		HELP_properties_menu },
	{ LAB_help,src_help_pane, LAB_help_mne,
		sizeof(src_help_pane)/sizeof(Menu_table) },
};
					
static const Menu_table second_src_prop_pane[] =
{
	{ LAB_path_dlg, LAB_path_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::path_dialog_cb),
		HELP_path_dialog },
	{ LAB_language_dlg, LAB_language_mne, Set_cb, PT_last, SEN_all_but_script,	
		(Callback_ptr)(&Window_set::set_language_dialog_cb),
		HELP_language_dialog },
	{ LAB_granularity_dlg, LAB_granularity_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::set_granularity_cb),
		HELP_granularity_dialog },
};

static Menu_bar_table second_src_menu_table[] =
{
	{ LAB_fileb,src_file_pane, LAB_fileb_mne,
		sizeof(src_file_pane)/sizeof(Menu_table),
		HELP_file_menu },
	{ LAB_edit,src_edit_pane, LAB_edit_mne,
		sizeof(src_edit_pane)/sizeof(Menu_table),
		HELP_edit_menu },
	{ LAB_view,src_view_pane, LAB_view_mne,
		sizeof(src_view_pane)/sizeof(Menu_table),
		HELP_view_menu },
	{ LAB_event,src_event_pane, LAB_event_mne,
		sizeof(src_event_pane)/sizeof(Menu_table),
		HELP_event_menu },
	{ LAB_props, second_src_prop_pane, LAB_props_mne,
		sizeof(second_src_prop_pane)/sizeof(Menu_table),
		HELP_properties_menu },
	{ LAB_help,src_help_pane, LAB_help_mne,
		sizeof(src_help_pane)/sizeof(Menu_table) },
};
static const Menu_table dis_view_pane[] =
{
	{ LAB_show_loc_dlg, LAB_show_loc_dlg_mne, Pane_cb, PT_disassembler, SEN_process,
		(Callback_ptr)(&Disassembly_pane::show_loc_cb),
		HELP_show_location_dialog },
	{ LAB_show_func_dis_dlg, LAB_show_func_dis_mne, Pane_cb, PT_disassembler,
		SEN_process,
		(Callback_ptr)(&Disassembly_pane::show_function_cb),
		HELP_show_dis_function_dialog },
	{ LAB_search_dlg, LAB_search_mne, Window_cb, PT_last,
		(SEN_file_required|SEN_disp_dis_required),
		(Callback_ptr)(&Base_window::search_dialog_cb),
		HELP_search_dialog },
	{ LAB_show_value_dlg, LAB_show_value_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::show_value_dialog_cb),
		HELP_show_value_dialog },
	{ LAB_set_value_dlg, LAB_set_value_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::set_value_dialog_cb),
		HELP_set_value_dialog },
	{ LAB_dump_dlg, LAB_dump_dlg_mne, Set_cb, PT_last,
		(SEN_process|SEN_single_sel|SEN_proc_stopped_core),
		(Callback_ptr)(&Window_set::dump_dialog_cb),
		HELP_dump_dialog },
	{ LAB_map_dlg, LAB_map_mne, Set_cb, PT_last, (SEN_process|SEN_single_sel),
		(Callback_ptr)(&Window_set::map_dialog_cb),
		HELP_map_dialog },
};

static const Menu_table dis_edit_pane[] =
{
	{ LAB_copy, LAB_copy_mne, Window_cb, PT_last, SEN_text_sel,
		(Callback_ptr)(&Base_window::copy_cb),
		HELP_copy_cmd },
};

static const Menu_table dis_control_pane[] =
{
	{ LAB_animate_dis, LAB_animate_dis_mne, Set_cb, PT_last,
		(SEN_process|SEN_proc_runnable),
		(Callback_ptr)(&Window_set::animate_dis_cb),
		HELP_animate_dis_cmd },
};

static const Menu_table dis_event_pane[] =
{
	{ LAB_set_break, LAB_set_break_mne, Window_cb, PT_last, 
		(SEN_process|SEN_proc_stopped|SEN_dis_sel|SEN_source_sel),
		(Callback_ptr)(&Base_window::set_break_cb),
		HELP_set_breakpoint_cmd },
	{ LAB_delete_break, LAB_delete_break_mne, Window_cb, PT_last,
		(SEN_process|SEN_proc_stopped|SEN_source_sel|SEN_breakpt_required|SEN_dis_sel),
		(Callback_ptr)(&Base_window::delete_break_cb),
		HELP_delete_breakpoint_cmd },
};

static const Menu_table dis_prop_pane[] =
{
	{ LAB_animation_dlg, LAB_animation_mne, Set_cb, PT_last, SEN_all_but_script, 
		(Callback_ptr)(&Window_set::animation_dialog_cb),
		HELP_animation_dialog },
};

static const Menu_table dis_help_pane[] =
// for help on panes, help msg is in cdata field
{
	{ LAB_dis_pane_hlp, LAB_dis_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_none, HELP_dis_pane },
};

static const Menu_bar_table dis_menu_table[] =
{
	{ LAB_edit, dis_edit_pane, LAB_edit_mne,
		sizeof(dis_edit_pane)/sizeof(Menu_table),
		HELP_edit_menu },
	{ LAB_view, dis_view_pane, LAB_view_mne,
		sizeof(dis_view_pane)/sizeof(Menu_table),
		HELP_view_menu },
	{ LAB_control, dis_control_pane, LAB_control_mne,
		sizeof(dis_control_pane)/sizeof(Menu_table),
		HELP_control_menu },
	{ LAB_event, dis_event_pane, LAB_event_mne,
		sizeof(dis_event_pane)/sizeof(Menu_table),
		HELP_event_menu },
	{ LAB_props, dis_prop_pane, LAB_props_mne,
		sizeof(dis_prop_pane)/sizeof(Menu_table),
		HELP_properties_menu },
	{ LAB_help, dis_help_pane, LAB_help_mne,
		sizeof(dis_help_pane)/sizeof(Menu_table) },
};

// Register pane

static const Menu_table regs_help_pane[] =
// for help on panes, help msg is in cdata field
{
	{ LAB_regs_pane_hlp, LAB_regs_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb), 
		HELP_none, HELP_regs_pane },
};

static const Menu_bar_table regs_menu_table[] =
{
	{ LAB_edit, dis_edit_pane, LAB_edit_mne,
		sizeof(dis_edit_pane)/sizeof(Menu_table),
		HELP_edit_menu },
	{ LAB_help, regs_help_pane, LAB_help_mne,
		sizeof(regs_help_pane)/sizeof(Menu_table) },
};
			
// Events
static const Menu_table event_edit_pane[] =
{
	{ LAB_disable, LAB_disable_mne, Pane_cb, PT_event,
		SEN_event_able_sel|SEN_event_sel,
		(Callback_ptr)(&Event_pane::disableEventCb),
		HELP_disable_event_cmd },
	{ LAB_enable, LAB_enable_mne, Pane_cb, PT_event, 
		SEN_event_dis_sel|SEN_event_sel,
		(Callback_ptr)(&Event_pane::enableEventCb),
		HELP_enable_event_cmd },
	{ LAB_delete, LAB_delete_mne, Pane_cb, PT_event, SEN_event_sel,
		(Callback_ptr)(&Event_pane::deleteEventCb),
		HELP_delete_event_cmd },
};

static const Menu_table event_prop_pane[] =
{
	{ LAB_panes_dlg, LAB_panes_mne, Window_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Base_window::setup_panes_cb),
		HELP_panes_dialog },
	{ LAB_granularity_dlg, LAB_granularity_mne, Set_cb, PT_last, SEN_all_but_script,
		(Callback_ptr)(&Window_set::set_granularity_cb),
		HELP_granularity_dialog  },
};

static const Menu_table event_event_pane[] =
{
	{ LAB_change_dlg, LAB_change_dlg_mne, Pane_cb, PT_event,
		(SEN_event_sel|SEN_single_sel),
		(Callback_ptr)(&Event_pane::changeEventCb),
		HELP_change_dialog },
};

static const Menu_table event_help_pane[] =
// for help on panes, help msg is in cdata field
{
	{ LAB_event_pane_hlp, LAB_event_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb) , 
		HELP_none, HELP_event_pane },
};

static const Menu_bar_table event_menu_table[] =
{
	{ LAB_edit, event_edit_pane, LAB_edit_mne,
		sizeof(event_edit_pane)/sizeof(Menu_table),
		HELP_edit_menu },
	{ LAB_event, event_event_pane, LAB_event_mne,
		sizeof(event_event_pane)/sizeof(Menu_table),
		HELP_event_menu },
	{ LAB_props, event_prop_pane, LAB_props_mne,
		sizeof(event_prop_pane)/sizeof(Menu_table),
		HELP_properties_menu },
	{ LAB_help, event_help_pane, LAB_help_mne,
		sizeof(event_help_pane)/sizeof(Menu_table) },
};

static const Menu_table status_help_pane[] =
// for help on panes, help msg is in cdata field
{
	{ LAB_status_pane_hlp, LAB_status_pane_mne, Window_cb, PT_last, SEN_always,
		(Callback_ptr)(&Base_window::help_sect_cb) , 
		HELP_none, HELP_status_pane },
};

static const Menu_bar_table status_menu_table[] =
{
	{ LAB_help, status_help_pane,  LAB_help_mne,
		sizeof(status_help_pane)/sizeof(Menu_table) },
};

// indices of default window descriptions
#define W_SOURCE	0
#define W_PROCESS	1
#define W_SYMBOLS	2
#define W_DIS		3
#define W_EVENT		4
#define W_COMMAND	5

// button descriptors
static const Button_bar_table set_current_button = {
	B_set_current, LAB_set_current,
	(SEN_frame_sel|SEN_process_sel|SEN_single_sel),	
	(Callback_ptr)(&Window_set::set_current_cb),
	HELP_set_current_cmd , Set_cb 
};

static const Button_bar_table run_button = {
	B_run, LAB_run, (SEN_process|SEN_proc_runnable), 
	(Callback_ptr)(&Window_set::run_button_cb),
	HELP_run_cmd, Set_cb
};

static const Button_bar_table return_button = {
	B_return, LAB_return, (SEN_process|SEN_proc_runnable),
	(Callback_ptr)(&Window_set::run_r_button_cb), 
	HELP_return_cmd, Set_cb
};

static const Button_bar_table step_stmt_button = {
	B_step_stmt, LAB_step_stmt, (SEN_process|SEN_proc_runnable),
	(Callback_ptr)(&Window_set::step_button_cb),
	HELP_step_stmt_cmd, Set_cb
};

static const Button_bar_table next_stmt_button = {
	B_next_stmt, LAB_next_stmt, (SEN_process|SEN_proc_runnable),
	(Callback_ptr)(&Window_set::step_o_button_cb), 
	HELP_next_stmt_cmd, Set_cb
};

static const Button_bar_table halt_button = {
	B_halt, LAB_halt, (SEN_process|SEN_proc_running|SEN_animated),
	(Callback_ptr)(&Window_set::halt_button_cb),
	HELP_halt_cmd, Set_cb
};

static const Button_bar_table destroy_button = {
	B_destroy, LAB_destroy,  (SEN_process|SEN_animated),
	(Callback_ptr)(&Window_set::destroy_process_cb),
	HELP_destroy_cmd, Set_cb
};

static const Button_bar_table symbols_button = {
	B_popup, LAB_symbols_pop, SEN_all_but_script,
	(Callback_ptr)(&Window_set::popup_window_cb),
	HELP_popup_button, Set_data_cb, PT_last, W_SYMBOLS
};

static const Button_bar_table cmd_button = {
	B_popup, LAB_cmd_pop, SEN_all_but_script,
	(Callback_ptr)(&Window_set::popup_window_cb),
	HELP_popup_button, Set_data_cb, PT_last, W_COMMAND
};

static const Button_bar_table dis_button = {
	B_popup, LAB_dis_pop, SEN_all_but_script,
	(Callback_ptr)(&Window_set::popup_window_cb),
	HELP_popup_button, Set_data_cb, PT_last, W_DIS
};

static const Button_bar_table event_button = {
	B_popup, LAB_event_pop, SEN_all_but_script,
	(Callback_ptr)(&Window_set::popup_window_cb),
	HELP_popup_button, Set_data_cb, PT_last, W_EVENT
};

static const Button_bar_table source_button = {
	B_popup, LAB_source_pop, SEN_all_but_script,
	(Callback_ptr)(&Window_set::popup_window_cb),
	HELP_popup_button, Set_data_cb, PT_last, W_SOURCE
};

static const Button_bar_table process_button = {
	B_popup, LAB_process_pop, SEN_all_but_script,
	(Callback_ptr)(&Window_set::popup_window_cb),
	HELP_popup_button, Set_data_cb, PT_last, W_PROCESS
};

static const Button_bar_table step_inst_button = {
	B_step_inst, LAB_step_inst, (SEN_process|SEN_proc_runnable), 
	(Callback_ptr)(&Window_set::step_i_button_cb),
	HELP_step_instr_cmd, Set_cb
};

static const Button_bar_table next_inst_button = {
	B_next_inst, LAB_next_inst, (SEN_process|SEN_proc_runnable),
	(Callback_ptr)(&Window_set::step_oi_button_cb),
	HELP_next_instr_cmd, Set_cb 
};

static const Button_bar_table disable_button = {
	B_disable, LAB_disable, SEN_event_able_sel|SEN_event_sel,
	(Callback_ptr)(&Event_pane::disableEventCb),
	HELP_disable_event_cmd, Pane_cb, PT_event
};

static const Button_bar_table enable_button = {
	B_enable, LAB_enable, SEN_event_dis_sel|SEN_event_sel,
	(Callback_ptr)(&Event_pane::enableEventCb),
	HELP_enable_event_cmd, Pane_cb, PT_event 
};

static const Button_bar_table delete_button = {
	B_delete, LAB_delete, SEN_event_sel,
	(Callback_ptr)(&Event_pane::deleteEventCb),
	HELP_delete_event_cmd, Pane_cb, PT_event
};

static const Button_bar_table input_button = {
	B_input, LAB_input_dlg,
	(SEN_process|SEN_proc_live|SEN_proc_io_redirected|SEN_animated),	
	(Callback_ptr)(&Command_pane::input_dialog_cb),
	HELP_input_dialog, Pane_cb, PT_command
};

static const Button_bar_table interrupt_button = {
	B_interrupt, LAB_interrupt, SEN_always,
	(Callback_ptr)(&Command_pane::interrupt_cb),
	HELP_interrupt_cmd, Pane_cb, PT_command
};

static const Button_bar_table export_button = {
	 B_export, LAB_export,
	(SEN_user_symbol|SEN_symbol_sel),
	(Callback_ptr)(&Symbols_pane::export_syms_cb),
	HELP_export_cmd, Pane_cb, PT_symbols
};

static const Button_bar_table expand_button = {
	 B_expand, LAB_expand_dlg,
	(SEN_program_symbol|SEN_single_sel|SEN_symbol_sel),
	(Callback_ptr)(&Window_set::expand_dialog_cb),
	HELP_expand_dialog, Set_cb, PT_last,
};

static const Button_bar_table pin_button = {
	B_sym_pin, LAB_pin,
	(SEN_symbol_sel|SEN_sel_has_unpin_sym),
	(Callback_ptr)(&Symbols_pane::pin_sym_cb),
	HELP_sym_pin_cmd, Pane_cb, PT_symbols
};

static const Button_bar_table unpin_button = {
	B_sym_unpin, LAB_unpin,
	(SEN_symbol_sel|SEN_sel_has_pin_sym),
	(Callback_ptr)(&Symbols_pane::unpin_sym_cb),
	HELP_sym_unpin_cmd, Pane_cb, PT_symbols
};

static const Button_bar_table set_watch_button = {
	B_set_watchpt, LAB_set_watchpt,
	(SEN_process|SEN_proc_stopped|SEN_program_symbol|SEN_symbol_sel),
	(Callback_ptr)(&Symbols_pane::set_watchpoint_cb),
	HELP_set_watchpoint_cmd, Pane_cb, PT_symbols 
};

static const Button_bar_table animate_src_button = {
	B_animate_src, LAB_animate, (SEN_process|SEN_proc_runnable),
	(Callback_ptr)(&Window_set::animate_src_cb),
	HELP_animate_source_cmd, Set_cb
};

static const Button_bar_table animate_dis_button = {
	B_animate_dis, LAB_animate, (SEN_process|SEN_proc_runnable),
	(Callback_ptr)(&Window_set::animate_dis_cb),
	HELP_animate_dis_cmd, Set_cb
};

const Button_bar_table *
get_button_desc(CButtons button)
{
	switch(button)
	{
	default:
		break;
	case B_set_current:
		return &set_current_button;
	case B_animate_src:
		return &animate_src_button;
	case B_animate_dis:
		return &animate_dis_button;
	case B_disable:
		return &disable_button;
	case B_enable:
		return &enable_button;
	case B_delete:
		return &delete_button;
	case B_input:
		return &input_button;
	case B_interrupt:
		return &interrupt_button;
	case B_run:
		return &run_button;
	case B_return:
		return &return_button;
	case B_step_stmt:
		return &step_stmt_button;
	case B_next_stmt:
		return &next_stmt_button;
	case B_step_inst:
		return &step_inst_button;
	case B_next_inst:
		return &next_inst_button;
	case B_halt:
		return &halt_button;
	case B_destroy:
		return &destroy_button;
	case B_export:
		return &export_button;
	case B_expand:
		return &expand_button;
	case B_sym_pin:
		return &pin_button;
	case B_sym_unpin:
		return &unpin_button;
	case B_set_watchpt:
		return &set_watch_button;
	case B_popup:
		return &source_button;	// any popup button will do
	}
	return 0;
}

static const Button_bar_table	*source_button_desc[] = {
	&set_current_button,
	&run_button,
	&return_button,
	&step_stmt_button,
	&next_stmt_button,
	&halt_button,
	&destroy_button,
	&symbols_button,
};

static const Button_bar_table	*syms_button_desc[] = {
	&expand_button,
	&pin_button,
	&unpin_button,
	&set_watch_button,
	&cmd_button,
	&dis_button,
	&event_button,
	&process_button,
	&source_button,
};

static const Button_bar_table	*process_button_desc[] = {
	&set_current_button,
	&cmd_button,
	&dis_button,
	&event_button,
	&source_button,
	&symbols_button,
};

static const Button_bar_table	*dis_button_desc[] = {
	&run_button,
	&return_button,
	&step_inst_button,
	&next_inst_button,
	&halt_button,
	&destroy_button,
	&cmd_button,
	&process_button,
	&source_button,
};

static const Button_bar_table	*cmd_button_desc[] = {
	&input_button,
	&interrupt_button,
	&dis_button,
	&event_button,
	&process_button,
	&source_button,
	&symbols_button,
};

static const Button_bar_table	*event_button_desc[] = {
	&disable_button,
	&enable_button,
	&delete_button,
	&cmd_button,
	&dis_button,
	&process_button,
	&source_button,
	&symbols_button,
};

static const Pane_descriptor status_pane_desc = {
	"Status Pane", PT_status, 1, 0, status_menu_table
};

static const Pane_descriptor stack_pane_desc = {
	"Stack Pane", PT_stack, 4, 0, stack_menu_table
};

static const Pane_descriptor source_pane_desc = {
	"Source Pane", PT_source, 10, 60, src_menu_table
};

static const Pane_descriptor syms_pane_desc = {
	"Symbols Pane", PT_symbols, 5, 0, syms_menu_table
};

static const Pane_descriptor dis_pane_desc = {
	"Disassembly Pane", PT_disassembler, 10, 70, dis_menu_table
};

static const Pane_descriptor regs_pane_desc = {
	"Register Pane", PT_registers, REG_DISP_LINE, 70, 
		regs_menu_table
};

static const Pane_descriptor event_pane_desc = {
	"Event Pane", PT_event, 8, 0, event_menu_table 
};

static const Pane_descriptor cmd_pane_desc = {
	"Transcript Pane", PT_command, 10, 60, cmd_menu_table
};

static const Pane_descriptor process_pane_desc = {
	"Process Pane", PT_process, 8, 0, ps_menu_table
};

static const Pane_descriptor second_src_pane_desc = {
	"Source Pane", PT_source, 10, 60, second_src_menu_table
};

static const Pane_descriptor	*source_win_panes[] = {
	&status_pane_desc,
	&stack_pane_desc,
	&source_pane_desc,
};

static const Pane_descriptor	*syms_win_panes[] = {
	&status_pane_desc,
	&syms_pane_desc,
};

static const Pane_descriptor	*process_win_panes[] = {
	&process_pane_desc,
};

static const Pane_descriptor	*event_win_panes[] = {
	&status_pane_desc,
	&event_pane_desc,
};

static const Pane_descriptor	*dis_win_panes[] = {
	&status_pane_desc,
	&regs_pane_desc,
	&dis_pane_desc,
};

static const Pane_descriptor	*cmd_win_panes[] = {
	&status_pane_desc,
	&cmd_pane_desc,
};

static const Pane_descriptor	*second_src_win_panes[] = {
	&status_pane_desc,
	&second_src_pane_desc,
};

static const Pane_descriptor	*extra_regs_win_panes[] = {
	&status_pane_desc,
	&regs_pane_desc,
};

static const Pane_descriptor	*extra_dis_win_panes[] = {
	&status_pane_desc,
	&dis_pane_desc,
};

static const Pane_descriptor	*extra_stack_win_panes[] = {
	&status_pane_desc,
	&stack_pane_desc,
};

static const Pane_descriptor	*extra_src_win_panes[] = {
	&status_pane_desc,
	&source_pane_desc,
};

static Window_descriptor default_wdesc[] =
{
	{ LAB_source_pop, (Pane_descriptor **)source_win_panes, 0, 0, 
		sizeof(source_win_panes)/sizeof(Pane_descriptor *),
		(Button_bar_table **)source_button_desc,
		sizeof(source_button_desc)/sizeof(Button_bar_table *),
		W_AUTO_POPUP|W_HAS_STATUS },
	{ LAB_process_pop, (Pane_descriptor **)process_win_panes, 0, 0,
		sizeof(process_win_panes)/sizeof(Pane_descriptor *),
		(Button_bar_table **)process_button_desc,
		sizeof(process_button_desc)/sizeof(Button_bar_table *),
		0 }, 
	{ LAB_symbols_pop, (Pane_descriptor **)syms_win_panes, 0, 0,
		sizeof(syms_win_panes)/sizeof(Pane_descriptor *),
		(Button_bar_table **)syms_button_desc, 
		sizeof(syms_button_desc)/sizeof(Button_bar_table *),
		W_HAS_STATUS },
	{ LAB_dis_win, (Pane_descriptor **)dis_win_panes, 0, 0,
		sizeof(dis_win_panes)/sizeof(Pane_descriptor *),
		(Button_bar_table **)dis_button_desc, 
		sizeof(dis_button_desc)/sizeof(Button_bar_table *),
		W_HAS_STATUS },
	{ LAB_event_pop, (Pane_descriptor **)event_win_panes, 0, 0,
		sizeof(event_win_panes)/sizeof(Pane_descriptor *),
		(Button_bar_table **)event_button_desc,
		sizeof(event_button_desc)/sizeof(Button_bar_table *),
		W_HAS_STATUS|W_HAS_EVENT },
	{ LAB_command_win, (Pane_descriptor **)cmd_win_panes, 0, 0,
		sizeof(cmd_win_panes)/sizeof(Pane_descriptor *),
		(Button_bar_table **)cmd_button_desc,
		sizeof(cmd_button_desc)/sizeof(Button_bar_table *),
		W_HAS_STATUS|W_HAS_COMMAND },
};

// Fixed window descriptor for secondary source windows
Window_descriptor	second_source_wdesc = {
	LAB_second_source, (Pane_descriptor **)second_src_win_panes, 0, 0,
		sizeof(second_src_win_panes)/sizeof(Pane_descriptor *),
		(Button_bar_table **)source_button_desc, 
		sizeof(source_button_desc)/sizeof(Button_bar_table),
		W_HAS_STATUS,
};

// If a user provides a configuration, but does not specify
// all panes, we provide default windows for the unspecified
// panes.
static const Window_descriptor	extra_windows[] = {
	
	{ LAB_process_pop, (Pane_descriptor **)process_win_panes, 0, 0, 
		sizeof(process_win_panes)/sizeof(Pane_descriptor *),
		0, 0, 0 },
	{ LAB_stack_win, (Pane_descriptor **)extra_stack_win_panes, 0, 0,
		sizeof(extra_stack_win_panes)/sizeof(Pane_descriptor *),
		0, 0, W_HAS_STATUS },
	{ LAB_symbols_pop, (Pane_descriptor **)syms_win_panes, 0, 0,
		sizeof(syms_win_panes)/sizeof(Pane_descriptor *),
		0, 0, W_HAS_STATUS },
	{ LAB_source_pop, (Pane_descriptor **)extra_src_win_panes, 0, 0,
		sizeof(extra_src_win_panes)/sizeof(Pane_descriptor *),
		0, 0, W_HAS_STATUS },
	{ LAB_dis_win, (Pane_descriptor **)extra_dis_win_panes, 0, 0,
		sizeof(extra_dis_win_panes)/sizeof(Pane_descriptor *),
		0, 0, W_HAS_STATUS },
	{ LAB_regs_win, (Pane_descriptor **)extra_regs_win_panes, 0, 0,
		sizeof(extra_regs_win_panes)/sizeof(Pane_descriptor *),
		0, 0, W_HAS_STATUS },
	{ LAB_event_pop, (Pane_descriptor **)event_win_panes, 0, 0,
		sizeof(event_win_panes)/sizeof(Pane_descriptor *),
		0, 0, W_HAS_STATUS|W_HAS_EVENT },
	{ LAB_command_win, (Pane_descriptor **)cmd_win_panes, 0, 0,
		sizeof(cmd_win_panes)/sizeof(Pane_descriptor *),
		0, 0, W_HAS_STATUS|W_HAS_COMMAND },
};

const Pane_descriptor *
get_pane_desc(Pane_type pane)
{
	switch(pane)
	{
	default:
		break;
	case PT_command:
		return &cmd_pane_desc;
	case PT_disassembler:
		return &dis_pane_desc;
	case PT_event:
		return &event_pane_desc;
	case PT_process:
		return &process_pane_desc;
	case PT_registers:
		return &regs_pane_desc;
	case PT_source:
		return &source_pane_desc;
	case PT_stack:
		return &stack_pane_desc;
	case PT_symbols:
		return &syms_pane_desc;
	case PT_status:
		return &status_pane_desc;
	}
	return 0;
}

const Window_descriptor *
get_win_desc(Pane_type pane)
{
	switch(pane)
	{
	case PT_status:
	default:
		break;
	case PT_command:
		return &extra_windows[7];
	case PT_disassembler:
		return &extra_windows[4];
	case PT_event:
		return &extra_windows[6];
	case PT_process:
		return &extra_windows[0];
	case PT_registers:
		return &extra_windows[5];
	case PT_source:
		return &extra_windows[3];
	case PT_stack:
		return &extra_windows[1];
	case PT_symbols:
		return &extra_windows[2];
	}
	return 0;
}

Window_descriptor	*window_descriptor;
int			windows_per_set;
int			cmd_panes_per_set = 1;
int			max_rows = 10;

// merge buttons for a single menu for a single window,
// taking contributions from each pane making up that window
// and the default for that menu - avoid duplicate buttons
static int
merge(Menu_table *final, const Menu_bar_table **menus, int entries)
{
	const Menu_bar_table	*mptr;
	int			nbuttons = 0;
	Menu_table		*first_final = final;

	for (int i = 0; i < entries; i++)
	{
		mptr = menus[i];
		for (int j = 0; j < mptr->nbuttons; j++)
		{
			int found;
			found = 0;
			// avoid duplicate entries
			for(int k = 0; k < nbuttons; k++)
			{
				if (first_final[k].label ==
					mptr->table[j].label)
				{
					found = 1;
					break;
				}
			}
			if (!found)
			{
				nbuttons++;
				*final++ = mptr->table[j];
			}
		}
	}
	return nbuttons;
}

// For each menu button (File, Edit, View, etc., merge
// contributions to the menu from each pane in the window.
static void
merge_menus(Window_descriptor *wd)
{
	int			menus_to_merge = 0;
	Menu_bar_table		*final_menu = new Menu_bar_table[MAX_MENUS];
	const Menu_bar_table	*menu_bar[MAX_MERGE];
	int	menu_count = 0;
	int	i = 0;
	int	j;

	if (wd->flags & W_AUTO_POPUP)
	{
		menu_bar[menus_to_merge++] = first_window_menu;
	}
	for (i = 0; i < wd->npanes; i++)
	{
		if (wd->panes[i]->menu_table)
			menu_bar[menus_to_merge++] = wd->panes[i]->menu_table;
	}
	menu_bar[menus_to_merge++] = default_menus;

	wd->menu_table = final_menu;
	for (i = 0; i < MAX_MENUS; i++)
	{
		const Menu_bar_table	*table[MAX_MERGE];
		int	max_buttons = 0;
		int	nentries = 0;

		for (j = 0; j < menus_to_merge; j++)
		{
			if (menu_bar[j]->label == default_menus[i].label)
			{
				if (menu_bar[j]->nbuttons)
				{
					max_buttons += menu_bar[j]->nbuttons;
					table[nentries++] = menu_bar[j];
				}
				menu_bar[j]++;
			}
		}
		if (!max_buttons)
			continue;

		Menu_table	*tab;

		tab = new Menu_table[max_buttons];
		final_menu->label = default_menus[i].label;
		final_menu->mnemonic = default_menus[i].mnemonic;
		final_menu->help_msg = default_menus[i].help_msg;
		final_menu->nbuttons = merge(tab, table, nentries);
		final_menu->table = tab;
		final_menu++;
		menu_count++;
	}
	wd->nmenus = menu_count;
}

Window_descriptor *
make_descriptors()
{
	Window_descriptor	*wd;
	Window_descriptor	*wptr;
	Menu_table		*windows_pane;
	Menu_table		*wmenu;
	int			i;
	mnemonic_t		mnemonics[PT_last];

	if ((wd = read_user_file()) == 0)
	{
		wd = default_wdesc;
		windows_per_set = sizeof(default_wdesc)/sizeof(Window_descriptor);
	}

	// build "Windows" menu option in File menu
	wmenu = windows_pane = new Menu_table[windows_per_set];
	for (i = 0, wptr = wd; i < windows_per_set; i++, wmenu++, wptr++)
	{
		if (!wptr->name)
		{
			wptr->name = labeltab.get_label(wptr->label);
		}
		// assign mnemonic, checking for duplicates
		const char	*current = wptr->name;
#ifdef MULTIBYTE
		wchar_t		c, cupper;
		int		len;
		while((len = mbtowc(&c, current, MB_CUR_MAX)) > 0)
		{
			current += len;
			cupper = towupper(c);
#else
		char	c, cupper;
		while((c = *current++) != 0)
		{
			cupper = toupper(c);
#endif
			int dup = 0;
			for(int j = 0; j < i; j++)
			{
				if (mnemonics[j] == cupper)
				{
					dup = 1;
					break;
				}
			}
			if (!dup)
				break;
		}
		// mnemonic could be 0 if no conflict-free choice found
		mnemonics[i] = cupper;
		wmenu->mnemonic = LAB_none;
		wmenu->mnemonic_name = c;
		wmenu->label = LAB_none;
		wmenu->name = wptr->name;
		wmenu->flags = Set_data_cb;
		wmenu->callback = (Callback_ptr)(&Window_set::popup_window_cb);
		wmenu->help_msg = HELP_windows_menu;
		wmenu->cdata = i;
		wmenu->sub_table = 0;
		wmenu->sensitivity = SEN_all_but_script;
	}
	def_file_pane[0].cdata = windows_per_set;
	def_file_pane[0].sub_table = windows_pane;

	for (i = 0, wptr = wd; i < windows_per_set; i++, wptr++)
		merge_menus(wptr);

	merge_menus(&second_source_wdesc);

	return	wd;
}
