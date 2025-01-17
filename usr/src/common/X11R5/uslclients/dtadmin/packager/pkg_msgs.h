/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma	ident	"@(#)dtadmin:packager/pkg_msgs.h	1.46"
#endif
/*
 * This file contains all the message strings for the Desktop UNIX
 * dtadmin client PackageMgr.
 * 
 */
#define	string_appName		"dtpkg:1" FS "Application Setup"
#define	string_badLog		"dtpkg:2" FS \
				 "Unable to determine application contents"
#define	string_noSelect		"dtpkg:3" FS "No set or application is selected"
#define	string_syspkgs		"dtpkg:4" FS \
			"%d %s Applications/Sets currently installed on %s"
#define	string_spoolpkgs	"dtpkg:5" FS \
			"%d Applications/Sets available for installation"
#define	string_mailMsg		"dtpkg:6" FS \
"The following software has been installed on the system;\n\
use Application_Setup to find icons for your Desktop.\n\n"
#define	string_noIcon		"dtpkg:7" FS "No icons selected to install"
#define	string_spoolPrompt	"dtpkg:8" FS "View Folder"
#define	string_promptMsg	"dtpkg:9" FS \
	"Choose a folder to examine for applications"

#define	tag_addOp		"dtpkg:10" FS "Software installation"
#define	tag_instOp		"dtpkg:11" FS "Creation of desktop icon"
#define	tag_delOp		"dtpkg:12" FS "Deletion"
#define	tag_good		"dtpkg:13" FS "succeeded"
#define	tag_bad			"dtpkg:14" FS "failed"
#define	folder_apps		"dtpkg:15" FS "Applications"

#define	format_opFmt		"dtpkg:20" FS "%s of %s %s"
#define	format_insFmt		"dtpkg:21" FS "Insert %s in %s and click %s"
#define	format_pkgcnt		"dtpkg:22" FS "%d %s %s"
#define	format_install		"dtpkg:23" FS "%s installed in %s Folder"
#define	format_iconcnt		"dtpkg:24" FS "%d icons installed in %s Folder"
#define	format_notPkg		"dtpkg:25" FS \
			"This %s is not in package format.\n\n"
#define	format_wait		"dtpkg:26" FS \
			"Cataloging applications on %s;\nplease wait."
#define	format_cantRead		"dtpkg:27" FS \
			"Unable to catalog applications on %s."
#define	format_numPkgs		"dtpkg:28" FS "%d %s packages in set %s."
#define	format_numSets		"dtpkg:29" FS " in %d sets"

#define	label_action		"dtpkg:30" FS "Actions"
#define	label_file		"dtpkg:31" FS "File"
#define	label_view		"dtpkg:32" FS "View"
#define	label_edit		"dtpkg:33" FS "Application"
#define	label_help		"dtpkg:34" FS "Help"
#define	label_add		"dtpkg:35" FS "Install"
#define	label_delete		"dtpkg:36" FS "Remove..."
#define	label_install		"dtpkg:37" FS "Install to Desktop"
#define	label_info		"dtpkg:38" FS "Properties..."
#define	label_icons		"dtpkg:39" FS "Show Contents..."
#define	label_cancel		"dtpkg:40" FS "Cancel"
#define	label_go		"dtpkg:41" FS "Continue"
#define	label_select		"dtpkg:42" FS "Select"
#define	label_exit		"dtpkg:43" FS "Exit"
#define	label_apps		"dtpkg:44" FS "Add-On"
#define	label_system		"dtpkg:45" FS "System"
#define	label_all		"dtpkg:46" FS "All"
#define	label_spooled		"dtpkg:47" FS "Other..."
#define	label_medium		"dtpkg:48" FS "medium"

#define	mnemonic_action		"dtpkg:50" FS "A"
#define	mnemonic_file		"dtpkg:51" FS "F"
#define	mnemonic_edit		"dtpkg:52" FS "p"
#define	mnemonic_view		"dtpkg:53" FS "V"
#define	mnemonic_help		"dtpkg:54" FS "H"
#define	mnemonic_add		"dtpkg:55" FS "I"
#define	mnemonic_delete		"dtpkg:56" FS "R"
#define	mnemonic_install	"dtpkg:57" FS "I"
#define	mnemonic_info		"dtpkg:58" FS "P"
#define	mnemonic_icons		"dtpkg:59" FS "S"
#define	mnemonic_cancel		"dtpkg:60" FS "C"
#define	mnemonic_go		"dtpkg:61" FS "o"
#define	mnemonic_select		"dtpkg:62" FS "S"
#define	mnemonic_exit		"dtpkg:63" FS "X"
#define	mnemonic_apps		"dtpkg:64" FS "O"
#define	mnemonic_system		"dtpkg:65" FS "S"
#define	mnemonic_all		"dtpkg:66" FS "A"
#define	mnemonic_spooled	"dtpkg:67" FS "O"

#define	string_invokeCustom	"dtpkg:70" FS "Invoking custom utility ..."
#define	string_invokePkgOp	"dtpkg:71" FS "Invoking package utility ..."
#define	string_addTitle		"dtpkg:72" FS "Add Application: "
#define	string_remTitle		"dtpkg:73" FS "Delete Application: "
#define	string_cusTitle		"dtpkg:74" FS "Custom Installation"
#define	string_iconName		"dtpkg:75" FS "Appl-n_Setup"
#define	string_iconTitle	"dtpkg:76" FS "Application Setup: Icons"
#define	string_infoTitle	"dtpkg:77" FS "Application Setup: Properties"
#define	string_badCustom	"dtpkg:78" FS "Unable to invoke custom utility."
#define	string_badPkgOp		"dtpkg:79" FS "Unable to invoke package utility."
#define	string_badInstPkg	"dtpkg:80" FS "Unable to invoke installpkg."
#define	string_badRemPkg	"dtpkg:81" FS "Unable to invoke removepkg."
#define	string_invokeInstPkg	"dtpkg:82" FS "Invoking  installpkg ..."
#define	string_invokeRemPkg	"dtpkg:83" FS "Invoking  removepkg ..."
#define	string_svr3Title	"dtpkg:84" FS \
			"Application Setup: SVR3.2 Package Installation"
#define	string_rmPkgTitle	"dtpkg:85" FS \
			"Application Setup: SVR3.2 Package Removal"
#define	string_mediaTitle	"dtpkg:86" FS "Application Setup: Package Media"
#define	string_msgTitle		"dtpkg:87" FS "Application Setup: Cataloging"
#define	string_pkgTitle		"dtpkg:88" FS "Application Setup: %s"

#define	info_name		"dtpkg:90" FS "Application Name:"
#define	info_desc		"dtpkg:91" FS "Description:"
#define	info_cat		"dtpkg:92" FS "Category:"
#define	info_vendor		"dtpkg:93" FS "Vendor:"
#define	info_version		"dtpkg:94" FS "Version:"
#define	info_arch		"dtpkg:95" FS "Architecture:"
#define	info_date		"dtpkg:96" FS "Date Installed"
#define	info_size		"dtpkg:97" FS "Size (blocks):"
#define	info_icons		"dtpkg:98" FS "Installable Icons:"
#define	info_execs		"dtpkg:99" FS \
				"No application icons. Executable Programs:"

#define	label_intro		"dtpkg:100" FS "Appl'n Setup..."
#define	label_toc		"dtpkg:101" FS "Table of Contents..."
#define	label_hlpdsk		"dtpkg:102" FS "Help Desk..."
#define	mnemonic_intro		"dtpkg:103" FS "A"
#define	mnemonic_toc		"dtpkg:104" FS "T"
#define	mnemonic_hlpdsk		"dtpkg:105" FS "K"

#define	help_intro		"dtpkg:106" FS "10"
#define	help_props		"dtpkg:107" FS "260"
#define	help_icons		"dtpkg:108" FS "220"
#define	help_uninstalled	"dtpkg:109" FS "120"
#define	help_folder		"dtpkg:110" FS "290"
#define	help_pkgwin		"dtpkg:111" FS "160"

#define	string_instTitle	"dtpkg:120" FS \
					"Application Setup: Installed - %s"
#define	string_uninstTitle	"dtpkg:121" FS \
					"Application Setup: Uninstalled - %s"
#define	label_inst		"dtpkg:122" FS "Installed Appl'ns"
#define	label_uninst		"dtpkg:123" FS "Uninstalled Appl'ns"
#define	mnemonic_inst		"dtpkg:124" FS "I"
#define	mnemonic_uninst		"dtpkg:125" FS "U"
#define	string_badClass		"dtpkg:126" FS \
					"Invalid icon class definition in %s."
#define	string_badHelp		"dtpkg:127" FS \
					"Invalid help file definition in %s."
#define	string_regClass		"dtpkg:128" FS \
		"Icon class definitions updated in Desktop Manager."
#define	string_regFailed	"dtpkg:129" FS \
		"Desktop Manager icon class definition update failed."
#define	string_cantRun		"dtpkg:130" FS \
		"You are not allowed to run %f."
#define string_cantExec		"dtpkg:140" FS "You don't have permission to install this package"

/* New Strings */

#define mnemonic_put		"dtpkg:200" FS "P"
#define mnemonic_ok		"dtpkg:201" FS "O"
#define mnemonic_show		"dtpkg:202" FS "S"
#define mnemonic_n_info		"dtpkg:203" FS "N"
#define mnemonic_set_format	"dtpkg:204" FS "S"
#define mnemonic_one		"dtpkg:205" FS "1"
#define mnemonic_two		"dtpkg:206" FS "2"
#define mnemonic_on_window	"dtpkg:207" FS "W"
#define mnemonic_on_keys	"dtpkg:208" FS "K"
#define mnemonic_index		"dtpkg:209" FS "I"
#define mnemonic_on_version	"dtpkg:210" FS "V"
#define mnemonic_find		"dtpkg:211" FS "F"
#define mnemonic_show_apps	"dtpkg:212" FS "S"
#define mnemonic_contents	"dtpkg:213" FS "C"
#define mnemonic_sco		"dtpkg:214" FS "S"
#define mnemonic_other		"dtpkg:215" FS "T"
#define mnemonic_reset		"dtpkg:216" FS "R"
#define	mnemonic_y_system	"dtpkg:217" FS "Y"
#define	mnemonic_l_all		"dtpkg:218" FS "L"
#define mnemonic_iinfo		"dtpkg:219" FS "I"

#define label_put		"dtpkg:300" FS "Put in Folder..."
#define label_ok		"dtpkg:301" FS "Ok"
#define label_show		"dtpkg:302" FS "Show Programs..."
#define label_iinfo		"dtpkg:303" FS "Info..."
#define label_set_format	"dtpkg:304" FS "Set Format"
#define label_on_help		"dtpkg:305" FS "On Help..."
#define label_on_window		"dtpkg:306" FS "On Window..."
#define label_on_keys		"dtpkg:307" FS "On Keys..."
#define label_index		"dtpkg:308" FS "Index..."
#define label_on_version	"dtpkg:309" FS "On Version..."
#define label_find		"dtpkg:310" FS "Find Folder..."
#define label_show_apps		"dtpkg:311" FS "Show Apps"
#define label_sco		"dtpkg:312" FS "SCO"
#define label_other		"dtpkg:313" FS "Other"
#define label_reset		"dtpkg:314" FS "Reset"

#define string_huh		"dtpkg:400" FS "???"
#define string_folder		"dtpkg:401" FS "Folder:"
#define string_install_from	"dtpkg:402" FS "Install From:"
#define string_apps_to_show	"dtpkg:403" FS "Applications to Show:"
#define string_app_type		"dtpkg:404" FS "Application Type:"
#define string_scompat		"dtpkg:405" FS "SCOMPAT Enivronment Variable"
#define string_un_app_type	"dtpkg:406" FS \
	"Application Installer: Uninstalled Application Type"

#define format_no_icons		"dtpkg:500" FS \
"There are no program icons in the %s\npackage that can be put in a folder."
#define format_apps_installed	"dtpkg:501" FS \
	"%s Applications Currently Installed on %s"
#define format_apps_in		"dtpkg:502" FS \
	"Applications in %s"
#define format_show_apps_on	"dtpkg:503" FS \
	"Show Applications on %s"
#define format_hide_apps_on	"dtpkg:504" FS \
	"Hide Applications on %s"
#define string_mailMsg2		"dtpkg:505" FS \
"The following software has been removed from the system;\n\
use Application Installer to remove icons from your Desktop.\n\n"
#define string_sco_default	"dtpkg:506" FS "3.2"
#define mnemonic_copy		"dtpkg:507" FS "F"
#define label_copy		"dtpkg:508" FS "Copy to Folder..."
#define label_set_option	"dtpkg:509" FS "Options"
#define string_option_type	"dtpkg:510" FS \
"Application Installer: Options"
#define format_icons_none	"dtpkg:511" FS \
"There are no program icons in the %s\npackage that can be copied to a folder."
#define string_init_msg		"dtpkg:512" FS "Select where to install from\
 or insert package and click ''Show Apps''"
#define string_copy_choices	"dtpkg:513" FS "Copy selected Icons to\
 Applications Folder for:\n"
#define label_apply		"dtpkg:514" FS "Apply"
#define label_self		"dtpkg:515" FS "Self"
#define label_current		"dtpkg:516" FS "Current Desktop Users"
#define label_future		"dtpkg:517" FS "Current and Future Desktop \
Users"
#define label_specific		"dtpkg:518" FS "Specific Users"
#define mnemonic_self		"dtpkg:519" FS "S"
#define mnemonic_current	"dtpkg:520" FS "D"
#define mnemonic_future		"dtpkg:521" FS "F"
#define mnemonic_specific	"dtpkg:522" FS "U"
#define mnemonic_apply		"dtpkg:523" FS "A"
#define string_copy_folder	"dtpkg:524" FS "Application Installer: Copy to Folder"
#define string_newappName	"dtpkg:525" FS "Application Installer"
#define string_newiconName	"dtpkg:526" FS "App_Installer"
#define string_newiconTitle	"dtpkg:527" FS "Application Installer: Icons"
#define string_newinfoTitle	"dtpkg:528" FS "Application Installer: Properties"
#define string_newsvr3Title	"dtpkg:529" FS "Application Installer: SVR3.2 Package Installation"
#define string_newmediaTitle	"dtpkg:530" FS "Application Installer: Package Media"
#define string_newmsgTitle	"dtpkg:531" FS "Application Installer: Cataloging"
#define string_newpkgTitle	"dtpkg:532" FS "Application Installer: %s"
#define mnemonic_option		"dtpkg:533" FS "O"

#define help_newintro		"dtpkg:534" FS "10"
#define help_newprops		"dtpkg:535" FS "170"
#define help_newicons		"dtpkg:536" FS "70"
#define help_newfolder		"dtpkg:537" FS "90"
#define help_newpkgwin		"dtpkg:538" FS "140"
#define help_newcatalog		"dtpkg:539" FS "40"
#define help_newpkg		"dtpkg:540" FS "80"
#define help_newcompat		"dtpkg:541" FS "120"
#define	string_modal_msg	"dtpkg:542" FS "Cannot display two Copy to Folder windows at once"
#define label_update_view	"dtpkg:543" FS "Update View"
#define string_update_view	"dtpkg:544" FS "Select where to install from\
 or insert package and click ''Update View''"
#define mnemonic_update_view	"dtpkg:545" FS "U"
#define label_set_option2	"dtpkg:546" FS "Options..."
#define alias_disk              "dtpkg:547" FS "Disk_%c"
#define alias_ctape1            "dtpkg:548" FS "Cartridge_Tape"
#define tag_disk                "dtpkg:549" FS "A"
#define desc_disk3              "dtpkg:550" FS "3.5 inch"
#define desc_disk5              "dtpkg:551" FS "5.25 inch"
#define desc_disk0              "dtpkg:552" FS "Floppy Disk Drive %c"
#define desc_ctape1             "dtpkg:553" FS "Cartridge Tape Drive"
#define alias_cdrom             "dtpkg:554" FS "CD-ROM"
#define desc_cdrom              "dtpkg:555" FS "Compact Disc-ROM Drive"
#define alias_ctape2            "dtpkg:556" FS "Cartridge_Tape 2"
#define desc_ctape2             "dtpkg:557" FS "Cartridge Tape Drive 2"
#define label_intro2		"dtpkg:558" FS "Application Installer..."
#define format_instgood		"dtpkg:559" FS "Creation of desktop icon %s succeeded"
#define format_instbad		"dtpkg:560" FS "Creation of desktop icon %s failed"
#define format_goodFmt		"dtpkg:561" FS "Software installation of %s succeeded"
#define format_badFmt		"dtpkg:562" FS "Software installation of %s failed"
#define format_delgood		"dtpkg:563" FS "Deletion of %s succeeded"
#define format_delbad		"dtpkg:564" FS "Deletion of %s failed"
#define	label_network		"dtpkg:565" FS "Network"
#define mnemonic_network	"dtpkg:566" FS "N"
#define label_nwname		"dtpkg:567" FS "Server:"
#define label_server		"dtpkg:568" FS "Find Server"
#define mnemonic_server		"dtpkg:569" FS "S"
#define string_select		"dtpkg:570" FS "Select icons to be installed"
#define string_server		"dtpkg:571" FS "Available Servers"
#define string_dtuser		"dtpkg:572" FS "Desktop Users:"
#define label_ok2		"dtpkg:573" FS "OK"
#define	label_spooled2		"dtpkg:574" FS "Other"
#define label_spooldir	        "dtpkg:575" FS "Spool Directory"
