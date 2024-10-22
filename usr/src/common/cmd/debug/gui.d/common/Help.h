/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef	HELP_H
#define	HELP_H
#ident	"@(#)debugger:gui.d/common/Help.h	1.20"

enum Help_id
{
	HELP_none,
	HELP_window,

	// Panes
	HELP_ps_pane,
	HELP_stack_pane,
	HELP_syms_pane,
	HELP_source_pane,
	HELP_regs_pane,
	HELP_dis_pane,
	HELP_event_pane,
	HELP_command_pane,
	HELP_transcript_pane,
	HELP_command_line,
	HELP_status_pane,

	// File Menus
	HELP_file_menu,
	HELP_create_dialog,
	HELP_grab_core_dialog,
	HELP_grab_process_dialog,
	HELP_release_cmd,
	HELP_open_dialog,
	HELP_new_source_cmd,
	HELP_move_dialog,	
	HELP_script_dialog,
	HELP_cd_dialog,
	HELP_new_window_set_cmd,
	HELP_windows_menu,
	HELP_popup_button,
	HELP_dismiss_cmd,
	HELP_exit_cmd,

	// Edit menu
	HELP_edit_menu,
	HELP_copy_cmd,
	HELP_set_current_cmd,
	HELP_export_cmd,
	HELP_disable_event_cmd,
	HELP_enable_event_cmd,
	HELP_delete_event_cmd,
	HELP_input_dialog,
	HELP_interrupt_cmd,
	HELP_sym_pin_cmd,
	HELP_sym_unpin_cmd,

	// View menu
	HELP_view_menu,
	HELP_expand_dialog,
	HELP_show_value_dialog,
	HELP_set_value_dialog,
	HELP_show_type_dialog,
	HELP_show_line_dialog,
	HELP_show_source_function_dialog,
	HELP_show_dis_function_dialog,
	HELP_show_location_dialog,
	HELP_search_dialog,
	HELP_dump_dialog,
	HELP_map_dialog,

	// Control menu
	HELP_control_menu,
	HELP_run_cmd,
	HELP_return_cmd,
	HELP_run_dialog,
	HELP_step_stmt_cmd,
	HELP_step_instr_cmd,
	HELP_next_stmt_cmd,
	HELP_next_instr_cmd,
	HELP_step_dialog,
	HELP_animate_source_cmd,
	HELP_animate_dis_cmd,
	HELP_jump_dialog,
	HELP_halt_cmd,

	// Event menu
	HELP_event_menu,
	HELP_set_breakpoint_cmd,
	HELP_delete_breakpoint_cmd,
	HELP_set_watchpoint_cmd,
	HELP_stop_on_function_dialog,
	HELP_stop_dialog,
	HELP_signal_dialog,
	HELP_syscall_dialog,
	HELP_on_stop_dialog,
	HELP_cancel_dialog,
	HELP_destroy_cmd,
	HELP_kill_dialog,
	HELP_ignore_signals_dialog,
	HELP_change_dialog,

	// Properties menu
	HELP_properties_menu,
	HELP_panes_dialog,
	HELP_symbols_dialog,
	HELP_path_dialog,
	HELP_language_dialog,
	HELP_granularity_dialog,
	HELP_action_dialog,
	HELP_animation_dialog,

	HELP_final,
};

enum Help_mode
{
	HM_section,	// general help for specific section
	HM_toc,		// table of contents for specific section
};

struct Help_info {
	const char *filename;	// file name of help file
	const char *section;	// section within the file
	void 	   *data;	// toolkit specific data
};

extern Help_info	Help_files[];
extern char		*Help_path;

#endif
