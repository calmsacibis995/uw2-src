/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)vtools:gui/vgamsg.h	1.24"

#ifndef	_VGAMSG_H_
#define	_VGAMSG_H_

/*
 * This file contains all the message strings for the Desktop UNIX
 * Graphical client gsetvideo.
 * 
 */
#define FS			"\000"
#define MESS_FILE		"gsetvideo:"

#define TITLE			"1" FS "Display Setup"
#define ICON_TITLE		"2" FS "Display_Setup"
#define TEST_TITLE		"3" FS "Display Setup: Test"
#define SAVE_TITLE    		"4" FS "Display Setup: Save"
#define	CANCEL_TITLE		"5" FS "Display Setup: Cancel"
#define	VIEW_LABEL		"6" FS "View:"
#define	VIEW_MNE		"7" FS "V"
#define	HELP_LABEL		"8" FS "Help"
#define	HELP_MNE		"9" FS "H"
#define CANCEL_MNE		"10" FS "C"
#define FULLLIST_LABEL		"11" FS "All Choices"
#define FULLIST_MNE		"12" FS "A"
#define CHIPLIST_LABEL		"13" FS "Detected Choices"
#define CHIPLIST_MNE		"14" FS "D"
#define VENDOR			"15" FS "Vendor"
#define CHIPSET			"16" FS "Chipset"
#define DESCRIPTION		"17" FS "Description"
#define HW_DETECTED_CAPTION	"18" FS "Hardware Detected:" 
#define	MSG_CHIPSET		"19" FS "chipset"
#define MSG_NODETECT		"20" FS "Cannot detect type of video card\
 installed."
#define MEMORY_LABEL		"21" FS "Video Memory (Megabytes):"
#define	MEMORY_CAPTION		"22" FS "Video Memory Detected:"
#define MSG_UNDETECT		"23" FS "Unable to detect memory size"
#define MSG_MEGABYTE		"24" FS "Megabytes"
#define	MODEL			"25" FS "Model"
#define RESOLUTION		"26" FS "Resolution"
#define MONITOR			"27" FS "Monitor"
#define	COLORS			"28" FS "Colors"
#define MONITOR_LABEL		"29" FS "Monitor Size:"
#define INCH			"30" FS "in"
#define INCH_12			"31" FS "12"
#define INCH_13			"32" FS "13"
#define INCH_14			"33" FS "14"
#define INCH_15			"34" FS "15"
#define INCH_16			"35" FS "16"
#define INCH_17			"36" FS "17"
#define INCH_19			"37" FS "19"
#define INCH_21			"38" FS "21"
#define CENTIMETER		"39" FS "cm"
#define CM_30			"40" FS "30"
#define CM_33			"41" FS "33"
#define CM_35			"42" FS "36"
#define CM_38			"43" FS "38"
#define CM_41			"44" FS "41"
#define CM_43			"45" FS "43"
#define CM_45			"46" FS "46"
#define CM_53			"47" FS "53"
#define OTHER			"48" FS "Other"
#define WIDTH_LABEL		"49" FS "Width"
#define HEIGHT_LABEL		"50" FS "Height"
#define	TEST_LABEL		"51" FS "Test"
#define TEST_MNE		"52" FS "T"
#define	SAVE_LABEL		"53" FS "Save"
#define SAVE_MNE		"54" FS "S"
#define	RESTORE_VGA_LABEL	"55" FS "Standard VGA"
#define	RESTORE_VGA_MNE		"56" FS "a"
#define RESET_LABEL		"57" FS "Reset"
#define RESET_MNE		"58" FS "R"
#define OK_LABEL		"59" FS "OK"
#define	CANCEL_LABEL		"60" FS "Cancel"
#define YES_LABEL		"61" FS "Yes"
#define	NO_LABEL		"62" FS "No"
#define	CONTINUE_LABEL		"63" FS "Continue"
#define HALF_MEG		"64" FS "1/2"
#define ONE_MEG			"65" FS "1"
#define	TWO_MEG			"66" FS "2"
#define	THREE_MEG		"67" FS "3"
#define FOUR_MEG		"68" FS "4"
#define CONFIG_ERR		"69" FS "Error generating configuration file"
#define CONFIG_OPEN_ERR 	"70" FS "Cannot open Xwinconfig file:"
#define CONFIG_READ_ERR 	"71" FS "Cannot read Xwinconfig file:"
#define POSTINSTALL_ERR		"72" FS "Error Vendor's POSTINSTALL command\
 failed"
#define OBS_MSG1		"73" FS "Obsolete"
#define OBS_MSG2		"74" FS "Obsolete"
#define OBS_MSG3		"75" FS "Obsolete"
#define SAVE_MSG		"76" FS "If the test pattern just displayed\
 appeared normal then\nyou may save the new settings. Otherwise you\
 should\nclick 'Cancel' and select a different set of options."
#define EXIT_MSG		"77" FS "You have not saved the changes to\
 your Video Display Setup.\n\nDo you want to save your changes before\
 exiting?"
#define LOGOUT_MSG		"78" FS "Your changes will not take effect\
 until you logout."
#define EXIT_MODE_MSG		"79" FS "You do not have the mode\
 information file for the selected vendor.\n\nPlease install the required\
 files before running Video Display Setup program."
#define OBS_MSG4		"80" FS "Obsolete"
#define OBS_MSG5		"81" FS "Obsolete"
#define OBS_MSG6		"82" FS "Obsolete"
#define OBS_MSG7		"83" FS "Obsolete"
#define TESTNOTDONE_MSG		"84" FS "It is recommended that you test\
 your settings\nbefore saving them. Click `Test' to test these\nsettings\
 now or `Save' to save without testing."
#define MEM_WARNING_TYPE1	"85" FS "Displaying %d colors at the\
 resolution you selected requires\n%.2f megabytes of video memory. Please\
 verify you have enough\nmemory and select the setting below that matches\
 your hardware.\n\nVideo Memory (Megabytes): <> 1/2  <> 1  <> 2  <> 3  <> 4"
#define MEM_WARNING_TYPE2	"86" FS "Displaying %d colors at the\
 resolution you selected requires\n%.2f megabytes of video memory. This is\
 more memory than you\nhave indicated is available.\n\nSelect an entry with\
 a lower resoultion or few colors."
#define TEST_MSG		"87" FS "When you click the 'Continue' below,\
 a Test Pattern will be displayed.\nIf the pattern and the text appear normal\
 then the new video board\nsettings may be used. If not, then you must choose\
 a different set\nof options in the Video Display Setup\
 window.\n\n                       * WARNING\
 *\n\nIf you have selected settings that are incompatible with your\nhardware.\
 (e.g. you selected the wrong vendor) you might not be\nable to read the\
 display after ending the test or might even damage\nyour hardware.\n\nIf\
 you cannot read your display\
 after ending the test, press the\n<Ctrl>, <Alt> and x keys at the same time\
 to exit the desktop. If\nyou still cannot read your display after this then\
 you must reboot\n(restart) your computer to reset the display. Be sure to\
 warn any\nother users of your system before rebooting. You can reboot\
 your\ncomputer by pressing the <Ctrl> <Alt> and <Del> keys at the\
 same\ntime.\n\nClick 'Continue' now to begin the test."
#define TEST_CHIPSET_MSG1	"88" FS "You selected the %s chipset but\
 auto-detection\nindicates you have a %s chipset.\n\nDo confirm that\
 selected chipset is compatible.\n\nTesting the display using the wrong\
 chipset can leave\nyour display unreadable after the test which\
 might\nrequire you to reboot (restart) your computer to correct.\nUsing the\
 wrong chipset setting could even damage\nyour hardware.\n\nDue to the\
 lack of industry standards, auto-detection\nmight not always be\
 accurate.\n\nDo you wish to continue with the test?"
#define TEST_CHIPSET_MSG2	"89" FS "You selected the %s chipset but\
 auto-detection\nwasn't able to detect the chipset.\n\nDo confirm that\
 selected chipset is compatible.\n\nTesting the display using the wrong\
 chipset can leave\nyour display unreadable after the test which\
 might\nrequire you to reboot (restart) your computer to correct.\nUsing the\
 wrong chipset setting could even damage\nyour hardware.\n\nDue to the\
 lack of industry standards, auto-detection\nmight not always be\
 accurate.\n\nDo you wish to continue with the test?"

#define MINI_HELP_VIEW		"90" FS "Detected Choices restricts\
 vendor/chipset choices to those appropriate for the hardware detected.\
 All Choices includes all supported vendor/chipset combinations."
#define MINI_HELP_COMBO1	"91" FS "Vendor is the brand or make of your\
 graphics card. Chipset is the model of its graphics processor (sometimes\
 called the graphics controller or accelerator). Standard VGA works with\
 all makes and models but at low resolution only."
#define MINI_HELP_MEM		"92" FS "Video Memory is the amount of\
 memory on your graphics card."
#define MINI_HELP_COMBO2	"93" FS "Model is the model of your graphics\
 card. Resolution indicates how much detail you can display. Monitor type may\
 include horizontal and vertical frequencies in KHz and Hz respectively. Check\
 your monitor manual to see what resolutions and frequencies it supports.\
 Colors is the maximum number of different colors you can display."
#define MINI_HELP_SIZE		"94" FS "Monitor size is the diagonal length\
 of your screen.  Select `Other' if your size is not in the list or if your\
 screen is higher than it is wide."
#define MINI_HELP_INCH		"95" FS "`in' indicates your monitor size is\
 specified in inches."
#define MINI_HELP_CM		"96" FS "`cm' indicates your monitor size is\
 specified in centimeters."
#define MINI_HELP_WD		"97" FS "Enter the width of your monitor."
#define MINI_HELP_HI		"98" FS "Enter the height of your monitor."
#define MINI_HELP_TEST		"99" FS "Test checks if your settings work\
 with your hardware."
#define MINI_HELP_SAVE		"100" FS "Save accepts your new settings."
#define MINI_HELP_SVGA		"101" FS "Standard VGA restores your settings\
 to values guaranteed to work with all hardware. Standard VGA is low\
 resolution and only displays 16 colors."
#define MINI_HELP_RESET		"102" FS "Reset restores your settings to the\
 last saved values."
#define MINI_HELP_CANCEL	"103" FS "Cancel closes this window."
#define MINI_HELP_HELP		"104" FS "Help provides additional information\
 about using Display Setup."

#define OWN_SEL_ERR		"105" FS "Cannot own selection"
#define WIDTH_MNE		"106" FS "W"
#define HEIGHT_MNE		"107" FS "e"
#define IN_MNE			"108" FS "i"
#define CM_MNE			"109" FS "m"
#define COMBO3_MNE		"110" FS "z"
#define COMBO1_MNE		"111" FS "n"
#define COMBO2_MNE		"112" FS "o"
#define MEM_MNE			"113" FS "d"
#define OK_MNE			"114" FS "O"
#define YES_MNE			"115" FS "Y"
#define NO_MNE			"116" FS "N"
#define CONT_MNE		"117" FS "o"
#define INCH_20			"118" FS "20"
#define CM_50			"119" FS "50"

#endif /* _VGAMSG_H_ */
