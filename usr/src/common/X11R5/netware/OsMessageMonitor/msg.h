/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)osmsgmon:msg.h	1.10"
#ident  "@(#)msg.h    6.1 "
#ident "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/OsMessageMonitor/msg.h,v 1.15 1994/09/29 15:22:26 plc Exp $"

/*  Copyright (c) 1993 Univel                           */
/*    All Rights Reserved                               */

/*  THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF      */
/*  Univel.                                             */
/*  The copyright notice above does not evidence any    */
/*  actual or intended publication of such source code. */

#define APP_NAME           "Msg_Monitor"
#define APP_CLASS_NAME     "Msg_Monitor"
#define HELP_FILE_NAME     "osMessageMonitor/osMessageMonitor.hlp"
#define HELP_SECT          "10"
#define HELP_SECT_SAVE     "30"
#define ICON_NAME          "messmon48.icon"

#ifndef FS
#define FS  "\001"
#define FS_CHR  '\001'
#endif

/*
 * Start of application international messages
 */
#define TXT_title          "osmMonitor:1" FS "Msg_Monitor"
#define TXT_appHelpSect    "osmMonitor:2" FS "10"

/*
 * General messages
 */
#define TXT_defFileName    "osmMonitor:3" FS "/tmp/osmLog"
#define TXT_conLog         "osmMonitor:4" FS "Console log for "
#define TXT_abort          "osmMonitor:5" FS "ABORTING\n"
#define TXT_osmOpen        "osmMonitor:6" FS "Couldn't open /dev/osm "
#define TXT_openErr        "osmMonitor:7" FS "Couldn't open %s"
#define TXT_pipeOpen       "osmMonitor:8" FS "Couldn't open pipe\n"
#define TXT_pipeErr        "osmMonitor:9" FS "Couldn't associate stream with the pipe fd\n"
#define TXT_forkErr        "osmMonitor:10" FS "Could not fork the /dev/osm reader process.\n"

/*
 * Option and usage messages
 */
#define TXT_syntax         "osmMonitor:11" FS "Invalid command syntax: \n"

#define TXT_use            "osmMonitor:12" FS "use: Msg_Monitor [-/+nf] [-/+nd] [-/+nc] [-/+v] [-/+e] [-/+t] [-/+daemon] \n"
#define TXT_notifyDeiconify "osmMonitor:13" FS "-/+nd	turn on/off notification of user when new data arrives by deiconifying the application window\n"
#define TXT_notifyFlash    "osmMonitor:14" FS "-/+nf	turn on/off notification of user when new data arrives by flashing application's icon window\n"
#define TXT_notifyColor    "osmMonitor:15" FS "-/+nc	turn on/off notification of user when new data arrives by changing application's icon window's color\n"
#define TXT_verbose        "osmMonitor:16" FS "-/+v	turn on/off display of hostname info\n"
#define TXT_exitOnFl       "osmMonitor:17" FS "-/+e	turn on/off exit on /dev/osm open failure\n"
#define TXT_tstamp         "osmMonitor:18" FS "-/+t	turn on/off timestamping\n"
#define TXT_daemon         "osmMonitor:19" FS "-/+daemon	turn on/off application backgrounding\n"

/*
 * Menu strings
 */
#define TXT_file              "osmMonitor:20" FS "File"
#define TXT_FILE_MNEMONIC     "osmMonitor:21" FS "F"

#define TXT_save              "osmMonitor:22" FS "Save"
#define TXT_SAVE_MNEMONIC     "osmMonitor:23" FS "S"

#define TXT_saveAs            "osmMonitor:24" FS "Save As..."
#define TXT_SAVEAS_MNEMONIC   "osmMonitor:25" FS "A"

#define TXT_append            "osmMonitor:26" FS "Append To"
#define TXT_APPEND_MNEMONIC   "osmMonitor:27" FS "T"

#define TXT_exit              "osmMonitor:28" FS "Exit"
#define TXT_EXIT_MNEMONIC     "osmMonitor:29" FS "x"

#define TXT_view              "osmMonitor:30" FS "View"
#define TXT_VIEW_MNEMONIC     "osmMonitor:31" FS "V"

#define TXT_clear             "osmMonitor:32" FS "Clear"
#define TXT_CLEAR_MNEMONIC    "osmMonitor:33" FS "C"

#define TXT_options           "osmMonitor:34" FS "Options"
#define TXT_OPTIONS_MNEMONIC  "osmMonitor:35" FS "O"

#define TXT_notify_option     "osmMonitor:36" FS "Notification"
#define TXT_NOTIFY_MNEMONIC   "osmMonitor:37" FS "N"

#define TXT_tstamp_option     "osmMonitor:38" FS "TimeStamp"
#define TXT_TSTAMP_MNEMONIC   "osmMonitor:39" FS "T"

#define TXT_deiconify_option     "osmMonitor:40" FS "Deiconify"
#define TXT_DEICONIFY_MNEMONIC   "osmMonitor:41" FS "D"

#define TXT_flash_option      "osmMonitor:42" FS "Flash"
#define TXT_FLASH_MNEMONIC    "osmMonitor:43" FS "F"

#define TXT_colorChange_option      "osmMonitor:44" FS "Color Change"
#define TXT_COLORCHANGE_MNEMONIC    "osmMonitor:45" FS "C"

#define TXT_off_option        "osmMonitor:46" FS "Off"
#define TXT_OFF_MNEMONIC      "osmMonitor:47" FS "O"

#define TXT_help              "osmMonitor:48" FS "Help"
#define TXT_HELP_MNEMONIC     "osmMonitor:49" FS "H"
 
#define TXT_hosmMonitor       "osmMonitor:50" FS "Msg_Monitor..."
#define TXT_XHELP_MNEMONIC    "osmMonitor:51" FS "M"

#define TXT_table             "osmMonitor:52" FS "Table of Contents..."
#define TXT_TABLE_MNEMONIC    "osmMonitor:53" FS "T"

#define TXT_helpDesk          "osmMonitor:54" FS "Help Desk..."
#define TXT_HDESK_MNEMONIC    "osmMonitor:55" FS "H"

/*
 * Log save messages
 */
#define TXT_fileDName      "osmMonitor:56" FS "File Name"
#define TXT_saveAsDName    "osmMonitor:57" FS "Save"
#define TXT_warningDName   "osmMonitor:58" FS "Warning"
#define TXT_errorDName     "osmMonitor:59" FS "Error"

#define TXT_fileNamePrompt "osmMonitor:60" FS "File Name:"

#define TXT_fileWriteError "osmMonitor:61" FS "File write error: Could not write"
#define TXT_fileOpenErr    "osmMonitor:62" FS "File open error: Could not save"
#define TXT_fileExists     "osmMonitor:63" FS "File exists.  OK to overwrite?"
/*
 * Font error messages
 */

#define TXT_fontLoadErr    "osmMonitor:64" FS "Could not load default font.\n"
#define TXT_fontListErr    "osmMonitor:65" FS "Could not load Motif fontlist.\n"

#define TXT_ok             "osmMonitor:66" FS "OK"
#define TXT_cancel         "osmMonitor:67" FS "Cancel"
#define TXT_fileLabel      "osmMonitor:68" FS "File:"
#define TXT_pathLabel      "osmMonitor:69" FS "Path:"
#define TXT_OK_MNEMONIC     "osmMonitor:70" FS "O"
#define TXT_CANCEL_MNEMONIC "osmMonitor:71" FS "C"
#define TXT_Title           "osmMonitor:72" FS "Message Monitor"
