/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systemmon:main.h	1.7"
///////////////////////////////////////////////////////////////
// main.h: 
///////////////////////////////////////////////////////////////
#ifndef main_h
#define main_h
#include <X11/StringDefs.h>
#include <X11/Xlib.h>

#define EQUAL_TO		"="
#define	SAR_TOTAL		13
#define GLOBAL			99
#define SLASH			"/"	
#define PLAYBACK_INDICATOR	"SYSTEM MONITOR PLAYBACK FILE"
#define SAVE_FILE		".sys_mon"

#define PIXEL			1
#define PERCENT			100
#define UNIT			10
#define YOFFSET			10
#define EACH_WIDTH		10

static const XmStringCharSet charset=(XmStringCharSet)XmSTRING_DEFAULT_CHARSET;
static const double PERCENTAGE = 100.00;

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define HELP_FILE			"System_Monitor/System_Monitor.hlp"

#define TXT_appHelp			"System_Monitor:1" FS "System Monitor"
#define TXT_appHelpSect		"System_Monitor:2" FS "10"
#define TXT_optionHelpSect	"System_Monitor:3" FS "30"
#define TXT_logHelpSect		"System_Monitor:4" FS "40"
#define TXT_selectHelpSect	"System_Monitor:5" FS "60"
#define TXT_alarmHelpSect	"System_Monitor:6" FS "80"
#define TXT_helpDesk		"System_Monitor:7" FS "Help Desk..."
#define MNEM_helpDesk		"System_Monitor:8" FS "H"
#define TXT_TOC				"System_Monitor:9" FS "Table of Contents..."
#define MNEM_TOC			"System_Monitor:10" FS "T"
#define TXT_tocHelp			"System_Monitor:11" FS "Table of Contents"
#define TXT_Actions			"System_Monitor:12" FS "Actions"
#define MNEM_Actions		"System_Monitor:13" FS "A"
#define TXT_exit			"System_Monitor:14" FS "Exit"
#define MNEM_exit			"System_Monitor:15" FS "x"
#define TXT_application		"System_Monitor:16" FS "System Monitor..."
#define MNEM_application	"System_Monitor:17" FS "S"
#define TXT_options			"System_Monitor:18" FS "Options..."
#define MNEM_options		"System_Monitor:19" FS "O"
#define TXT_usr 			"System_Monitor:20" FS "CPU Usage - User time"
#define TXT_sys 			"System_Monitor:21" FS "CPU Usage - System time"
#define TXT_wio 			"System_Monitor:22" FS "CPU Usage - Wait I/O time"
#define TXT_idle 			"System_Monitor:23" FS "CPU Usage - Idle Time"
#define TXT_apptitle 		"System_Monitor:24" FS "System Monitor"
#define TXT_apply 			"System_Monitor:25" FS "OK"
#define MNEM_apply 			"System_Monitor:26" FS "O"
#define TXT_cancel 			"System_Monitor:27" FS "Cancel"
#define MNEM_cancel 		"System_Monitor:28" FS "C"
#define TXT_help 			"System_Monitor:29" FS "Help"
#define MNEM_help 			"System_Monitor:30" FS "H"
#define TXT_color 			"System_Monitor:31" FS "Color"
#define MNEM_color			"System_Monitor:32" FS "C"
#define TXT_list 			"System_Monitor:33" FS "List of System Monitor options:"
#define TXT_colorlist 		"System_Monitor:34" FS "List of Colors:"
#define TXT_wait 			"System_Monitor:35" FS "Please Wait: Loading Colors"
#define TXT_100				"System_Monitor:36" FS "100"
#define TXT_scale			"System_Monitor:37" FS "Scale"
#define TXT_10				"System_Monitor:38" FS "10"
#define TXT_1000			"System_Monitor:39" FS "1000"
#define TXT_10000			"System_Monitor:40" FS "10000"
#define TXT_100000			"System_Monitor:41" FS "100000"
#define TXT_percerror 		"System_Monitor:42" FS "This is a percentage value.  Scale for this is not changeable."
#define TXT_scaleerr		"System_Monitor:43" FS "Scale Error"
#define TXT_interval		"System_Monitor:44" FS "Interval : "
#define TXT_vertical		"System_Monitor:45" FS "Vertical Grid"
#define TXT_horizontal		"System_Monitor:46" FS "Horizontal Grid"
#define TXT_rangetitle		"System_Monitor:47" FS "Range Error"
#define TXT_rangerr			"System_Monitor:48" FS "Value not in range. Valid range =  1 - 86400 seconds."
#define TXT_pgin			"System_Monitor:49" FS "Page-in requests/per sec"
#define TXT_pgout			"System_Monitor:50" FS "Page-out requests/per sec"
#define TXT_freemem			"System_Monitor:51" FS "Free memory pages"
#define TXT_freeswap		"System_Monitor:52" FS "Free blocks for swapping"
#define TXT_swapin			"System_Monitor:53" FS "System swap in/per sec"
#define TXT_swapout			"System_Monitor:54" FS "System swap out/per sec"
#define TXT_bswapin			"System_Monitor:55" FS "Block swap in/per sec"
#define TXT_bswapout		"System_Monitor:56" FS "Block swap out/per sec"
#define TXT_pswch			"System_Monitor:57" FS "Process switching/per sec"
#define TXT_helptitle 		"System_Monitor:58" FS "System_Monitor"
#define TXT_OptionsCtrl 	"System_Monitor:59" FS "Options"
#define TXT_PlaybackCtrl 	"System_Monitor:60" FS "Playback"
#define TXT_AlarmCtrl 		"System_Monitor:61" FS "Alarm"
#define TXT_settings		"System_Monitor:62" FS "Save Settings"
#define MNEM_settings		"System_Monitor:63" FS "S"
#define TXT_logtofile		"System_Monitor:64" FS "Log data to file..."
#define MNEM_logtofile		"System_Monitor:65" FS "L"
#define TXT_playback		"System_Monitor:66" FS "Playback log data..."
#define MNEM_playback		"System_Monitor:67" FS "P"
#define TXT_stoplog			"System_Monitor:68" FS "Stop Logging Data"
#define MNEM_stoplog		"System_Monitor:69" FS "D"
#define TXT_alarm			"System_Monitor:70" FS "Alarms..."
#define MNEM_alarm			"System_Monitor:71" FS "A"
#define TXT_View			"System_Monitor:72" FS "View"
#define MNEM_View			"System_Monitor:73" FS "V"
#define TXT_nofile			"System_Monitor:74" FS "Could not open file"
#define TXT_logq			"System_Monitor:75" FS "Do you want to append to existing log file? "
#define TXT_logtitle		"System_Monitor:76" FS "System Monitor: Log File Question"
#define TXT_logfailed		"System_Monitor:77" FS "Failed to write the log file. Check disk space/permissions."
#define TXT_saroptions		"System_Monitor:78" FS "System Monitor: Options"
#define TXT_errdialog		"System_Monitor:79" FS "System Monitor: Error Dialog"
#define TXT_promptlabel		"System_Monitor:80" FS "Log File Name"
#define TXT_prompt			"System_Monitor:81" FS "System Monitor: Log File"
#define TXT_enterfile		"System_Monitor:82" FS "Please enter a log file name."
#define TXT_savefailed		"System_Monitor:83" FS "Failed to save the settings."
#define TXT_alarmsetup		"System_Monitor:84" FS "System Monitor: Alarm Setup"
#define TXT_alarmlist		"System_Monitor:85" FS "List of selected options"
#define TXT_deletealarm		"System_Monitor:86" FS "Delete Alarm"
#define TXT_abovelabel		"System_Monitor:87" FS "Alarm Above:"
#define TXT_belowlabel		"System_Monitor:88" FS "Alarm Below:"
#define TXT_beep			"System_Monitor:89" FS "Beep"
#define TXT_flash			"System_Monitor:90" FS "Flash Header"
#define TXT_setalarm		"System_Monitor:91" FS "Set Alarm"
#define TXT_selectone		"System_Monitor:92" FS "Please select one option"
#define TXT_lastupdate		"System_Monitor:93" FS "Last update at : "
#define TXT_date			"System_Monitor:94" FS "Date: "
#define TXT_play			"System_Monitor:95" FS "Forward"
#define TXT_back			"System_Monitor:96" FS "Back"
#define TXT_stop			"System_Monitor:97" FS "Stop"
#define TXT_playtitle		"System_Monitor:98" FS " Playback saved data from "
#define TXT_select			"System_Monitor:99" FS "System Monitor: Select Playback File"
#define TXT_selectfile		"System_Monitor:100" FS "Cannot access file : "
#define TXT_notplayfile		"System_Monitor:101" FS "This file is not a playback file"
#define TXT_time			"System_Monitor:102" FS "Time : "
#define TXT_speed			"System_Monitor:103" FS "Speed : "
#define TXT_empty			"System_Monitor:104" FS "Empty log file"
#define TXT_numerical		"System_Monitor:105" FS "Invalid entry. Please enter a correct value."
#define TXT_proc			"System_Monitor:106" FS "Processor # : "
#define TXT_global			"System_Monitor:107" FS "Average"
#define TXT_single			"System_Monitor:108" FS "This is a single processor machine"
#define TXT_nometrics		"System_Monitor:109" FS "The kernel metrics library cannot be opened."
#define TXT_stoplogq		"System_Monitor:110" FS "Data logging will stop. Change processors? "
#define TXT_nospace			"System_Monitor:111" FS "Playback file too large. No memory available to execute."
#define TXT_maxalarm			"System_Monitor:112" FS "Maximum scale value has been exceeded."
#define TXT_noalarmvalues			"System_Monitor:113" FS "Beep/Flash with no alarm values."
#define TXT_nosamevalues			"System_Monitor:114" FS "Above and below values cannot be the same."
#define TXT_nozero				"System_Monitor:115" FS "Below value cannot be zero."
#define TXT_seconds				"System_Monitor:116" FS "seconds"

/*
#define TXT_process		"System_Monitor:96" FS "Process..."
#define MNEM_process		"System_Monitor:97" FS "P"
*/
#define TXT_file			"System_Monitor:117" FS "File"
#define TXT_path			"System_Monitor:118" FS "Path"
#define TXT_select_title		"System_Monitor:119" FS "Select Playback File"
#define TXT_playing			"System_Monitor:120" FS "Currently viewing a playback file."

#endif
