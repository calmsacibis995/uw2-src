#ident	"@(#)debugger:gui.d/common/Help.C	1.22"

#include "Help.h"

static char gui_file[] = "debug.hlp";

Help_info Help_files[HELP_final] =
{
	{0,0,0},			// HELP_none
	{gui_file,"1"},			// HELP_window

	// Panes
	{gui_file,"2"},		// HELP_ps_pane
	{gui_file,"3"},		// HELP_stack_pane
	{gui_file,"4"},		// HELP_syms_pane
	{gui_file,"7"},		// HELP_source_pane
	{gui_file,"5"},		// HELP_regs_pane
	{gui_file,"6"},		// HELP_dis_pane
	{gui_file,"8"},		// HELP_event_pane
	{gui_file,"9"},		// HELP_command_pane
	{gui_file,"91"},	// HELP_transcript_pane
	{gui_file,"92"},	// HELP_command_line
	{gui_file,"10"},	// HELP_status_pane

	// File menu
	{gui_file,"110"},		// HELP_file_menu
	{gui_file,"111"},		// HELP_create_dialog
	{gui_file,"112"},		// HELP_grab_core_dialog
	{gui_file,"113"},		// HELP_grab_process_dialog
	{gui_file,"114"},		// HELP_release_cmd
	{gui_file,"115"},		// HELP_open_dialog
	{gui_file,"116"},		// HELP_new_source_cmd
	{gui_file,"1161"},		// HELP_move_dialog,
	{gui_file,"117"},		// HELP_script_dialog
	{gui_file,"118"},		// HELP_cd_dialog
	{gui_file,"119"},		// HELP_new_window_set_cmd,
	{gui_file,"1110"},		// HELP_windows_menu,
	{gui_file,"11101"},		// HELP_popup_button,
	{gui_file,"1111"},		// HELP_dismiss_cmd,
	{gui_file,"1112"},		// HELP_exit_cmd,

	// Edit menu
	{gui_file,"120"},		// HELP_edit_menu
	{gui_file,"121"},		// HELP_copy_cmd
	{gui_file,"122"},		// HELP_set_current_cmd
	{gui_file,"123"},		// HELP_export_cmd
	{gui_file,"124"},		// HELP_disable_event_cmd
	{gui_file,"125"},		// HELP_enable_event_cmd
	{gui_file,"126"},		// HELP_delete_event_cmd
	{gui_file,"127"},		// HELP_input_dialog
	{gui_file,"128"},		// HELP_interrupt_cmd
	{gui_file,"129"},		// HELP_sym_pin_cmd,
	{gui_file,"1210"},		// HELP_sym_unpin_cmd,

	// View menu
	{gui_file,"130"},		// HELP_view_menu,
	{gui_file,"131"},		// HELP_expand_dialog,
	{gui_file,"132"},		// HELP_show_value_dialog,
	{gui_file,"133"},		// HELP_set_value_dialog,
	{gui_file,"134"},		// HELP_show_type_dialog,
	{gui_file,"135"},		// HELP_show_line_dialog,
	{gui_file,"136"},		// HELP_show_source_function_dialog,
	{gui_file,"137"},		// HELP_show_dis_function_dialog,
	{gui_file,"138"},		// HELP_show_location_dialog,
	{gui_file,"139"},		// HELP_search_dialog,
	{gui_file,"1310"},		// HELP_dump_dialog,
	{gui_file,"1311"},		// HELP_map_dialog,

	// Control menu
	{gui_file,"14"},		// HELP_control_menu,
	{gui_file,"141"},		// HELP_run_cmd,
	{gui_file,"142"},		// HELP_return_cmd,
	{gui_file,"143"},		// HELP_run_dialog,
	{gui_file,"144"},		// HELP_step_stmt_cmd,
	{gui_file,"145"},		// HELP_step_instr_cmd,
	{gui_file,"146"},		// HELP_next_stmt_cmd,
	{gui_file,"147"},		// HELP_next_instr_cmd,
	{gui_file,"148"},		// HELP_step_dialog,
	{gui_file,"149"},		// HELP_animate_source_cmd,
	{gui_file,"1410"},		// HELP_animate_dis_cmd,
	{gui_file,"1411"},		// HELP_jump_dialog,
	{gui_file,"1412"},		// HELP_halt_cmd,

	// Event menu
	{gui_file,"15"},		// HELP_event_menu,
	{gui_file,"151"},		// HELP_set_breakpoint_cmd,
	{gui_file,"152"},		// HELP_delete_breakpoint_cmd,
	{gui_file,"153"},		// HELP_set_watchpoint_cmd,
	{gui_file,"154"},		// HELP_stop_on_function_dialog,
	{gui_file,"155"},		// HELP_stop_dialog,
	{gui_file,"156"},		// HELP_signal_dialog,
	{gui_file,"157"},		// HELP_syscall_dialog,
	{gui_file,"158"},		// HELP_on_stop_dialog,
	{gui_file,"159"},		// HELP_cancel_dialog,
	{gui_file,"1510"},		// HELP_destroy_cmd,
	{gui_file,"1511"},		// HELP_kill_dialog,
	{gui_file,"1512"},		// HELP_ignore_signals_dialog,
	{gui_file,"1513"},		// HELP_change_dialog,

	// Properties menu
	{gui_file,"16"},		// HELP_properties_menu
	{gui_file,"162"},		// HELP_panes_dialog,
	{gui_file,"161"},		// HELP_symbols_dialog,
	{gui_file,"163"},		// HELP_path_dialog,
	{gui_file,"164"},		// HELP_language_dialog,
	{gui_file,"165"},		// HELP_granularity_dialog,
	{gui_file,"166"},		// HELP_action_dialog
	{gui_file,"167"},		// HELP_animation_dialog
};
