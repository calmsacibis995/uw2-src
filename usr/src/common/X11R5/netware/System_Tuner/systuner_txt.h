/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)systuner:systuner_txt.h	1.6"
// systuner_txt.h

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
///////////////////////////////////////////////////////////////////

#ifndef SYSTUNER_TXT_H
#define SYSTUNER_TXT_H

#define APP_NAME	"systuner"
#define APPNAME		"systuner"

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define TXT_title		"systuner:1" FS "System Tuner"
#define TXT_setuid_error	"systuner:2" FS "setuid error - no permission."
#define TXT_fopen_error1	"systuner:3" FS "fopen error - /etc/conf/cf.d/mtune_p."
#define TXT_fopen_error2	"systuner:4" FS "fopen error - /etc/conf/cf.d/mtune_d."
#define TXT_ok_button		"systuner:5" FS "OK"
#define TXT_reset_button	"systuner:6" FS "Reset"
#define TXT_cancel_button	"systuner:7" FS "Cancel"
#define TXT_help_button		"systuner:8" FS "Help..."
#define TXT_reset_factory_button	"systuner:9" FS "Reset to Factory"
#define TXT_toggle_label	"systuner:10" FS "auto"
#define TXT_autotuning		"systuner:11" FS "autotuning"
#define TXT_no_description	"systuner:12" FS "No text descriptions are available for this selected menu item or parameter."
#define TXT_question1		"systuner:13" FS "You have made changes.  Discard?"
#define TXT_question2		"systuner:14" FS "Do you want to rebuild the kernel now?"
#define TXT_question3		"systuner:15" FS "Do you want to reboot the system now?"
#define TXT_message1		"systuner:16" FS "The kernel will be rebuilt during\nnext system reboot."
#define TXT_message2		"systuner:17" FS "The new kernel will be installed\nduring next system reboot."
#define TXT_message3		"systuner:18" FS "Kernel rebuilding will take a few minutes.\nPlease wait."
#define TXT_error1		"systuner:19" FS "Error(s) have occurred during kernel rebuilding.\nPlease check /tmp/kernel_status file for error conditions."
#define TXT_yes_button		"systuner:20" FS "Yes"
#define TXT_no_button		"systuner:21" FS "No"
#define TXT_icon_label		"systuner:22" FS "System_Tuner"
#define MNEM_ok			"systuner:23" FS "O"
#define MNEM_reset		"systuner:24" FS "R"
#define MNEM_cancel		"systuner:25" FS "C"
#define MNEM_help		"systuner:26" FS "H"
#define MNEM_reset_factory	"systuner:27" FS "F"
#define MNEM_yes		"systuner:28" FS "Y"
#define MNEM_no			"systuner:29" FS "N"

#endif
