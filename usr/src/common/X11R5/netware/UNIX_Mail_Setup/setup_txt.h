/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)umailsetup:setup_txt.h	1.4"
//	Copyright (c) 1993 Univel


#ifndef SETUP_TXT_H
#define SETUP_TXT_H


#ifndef FS
#define FS	"\001"
#endif	//  FS
#ifndef FS_CHR
#define FS_CHR	'\001'
#endif	//  FS_CHR


//  Help file related items.
#define HELP_FILE		"/usr/X/lib/locale/C/help/setup/setup.hlp"

#define TXT_appHelpTitle	"gensetup:1"  FS "No-Name Setup Application"
#define TXT_appHelpSection	"gensetup:2"  FS "10"

#define TXT_appNoName		"gensetup:3"  FS "No-Name Setup Application"
#define TXT_iconNoName		"gensetup:4"  FS "No-Name"
#define TXT_defaultIconFile	"gensetup:5"  FS "exec48.icon"

#define TXT_category		"gensetup:6"  FS "Category: "
#define MNEM_category		"gensetup:7"  FS "C"
#define TXT_basic		"gensetup:8"  FS "Basic"
#define MNEM_basic		"gensetup:9"  FS "B"
#define TXT_extended		"gensetup:10" FS "Extended"
#define MNEM_extended		"gensetup:11" FS "E"

#define TXT_leftTitle		"gensetup:12" FS "Variables"
#define TXT_rightTitle		"gensetup:13" FS "Description"


//  Toggle labels
#define TXT_toggleOff		"gensetup:14" FS "No"
#define TXT_toggleOn		"gensetup:15" FS "Yes"


//  Action Area Button Labels
#define TXT_OkButton		"gensetup:16" FS "OK"
#define TXT_ResetButton		"gensetup:17" FS "Reset"
#define TXT_CancelButton	"gensetup:18" FS "Cancel"
#define TXT_HelpButton		"gensetup:19" FS "Help..."


#define TXT_fontLoadErr		"gensetup:20" FS "Error loading Desktop fonts."
#define TXT_fontListErr		"gensetup:21" FS "Error creating fonts."

//  Error messages which appear in the error dialog popup.
#define TXT_setupWebNoAccess	"gensetup:22" FS \
	"The setup library initialization has failed.\n"\
	"Please see your system administrator."
#define TXT_noModes		"gensetup:23" FS \
	"This application has neither a basic\n"\
	"nor an extended mode.  Consequently,\n"\
	"there is nothing to display."
#define TXT_noPerms		"gensetup:24" FS \
	"You do not have permission to change\n"\
	"parameters in this setup application."

//  Error messages which appear in the error dialog popup for the password field.
#define TXT_invalPasswd		"gensetup:25" FS \
	"Your second password does not match the first.\n"\
	"Please re-enter your password."

//  Action Area Button Mnemonics (unfortunately, mnemonic support was added
//  later, so these don't appear above with the action area button labels).
#define MNEM_OkButton		"gensetup:26" FS "O"
#define MNEM_ResetButton	"gensetup:27" FS "R"
#define MNEM_CancelButton	"gensetup:28" FS "C"
#define MNEM_HelpButton		"gensetup:29" FS "H"

#define TXT_invalIntChars	"gensetup:30" FS \
	"This integer field contains invalid characters.\n"\
	"Please re-enter this field."
#define TXT_invalIntRange	"gensetup:31" FS \
	"This integer is either too large or too small.\n"\
	"Please re-enter this field."

#endif	//  SETUP_TXT_H
