/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)instlsrvr:main.h	1.2"
#ident	"@(#)main.h	9.1 "
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Install_Server/main.h,v 1.1 1994/02/01 22:57:20 renu Exp $"

/*
#ifndef NOIDENT
#ident	"@(#)install_server:main.h	1.0"
#endif
*/

#ifndef MAIN_H
#define MAIN_H

typedef struct {
    XtArgVal	lbl;
    XtArgVal	mnem;
    XtArgVal    dflt;
    XtArgVal	selectProc;
    XtArgVal	sensitive;
    XtArgVal    userData;
} MenuItem;

typedef struct {
    char	*title;
    char	*file;
    char	*section;
} HelpText;

typedef struct {
    XtArgVal	lbl;
    XtArgVal	mnem;
    XtArgVal	set;
    XtArgVal	sensitive;
} ButtonItem;

#define CONFIG_FILE        	"/etc/installd.conf"
#define ApplicationServer    	"as"
#define PersonalEdition       	"pe"
#define as_found	       	1
#define pe_found	       	2
#define both_found	       	3
#define CONFIGURE	       	1
#define LOAD	       		2
#define REMOVE	       		3
#define ONLY_PE	       		4
#define POUND_SIGN		'#'
#define OK			1	
#define On			True	
#define Off			False	
#define SAPD_FILE		"/var/spool/sap/out/0x3e2"
#define PID_SAPD_FILE		"/var/spool/sap/sapdpid"
#define RM_COMMAND 		"/usr/bin/rm  "
#define RM_FILE 		"/usr/bin/rm  -rf "
#define CREATE_CMD		"echo 36986 > "
#define DD_COMMAND		"/usr/bin/dd " 
#define INPUT_FILE		"if=" 
#define OUTPUT_FILE		" of=" 
#define BLOCK_SIZE		" bs=64k " 
#define TAPE_FILE		"/dev/rmt/ctape1" 
#define PACKAGE			"/unixware" 
#define PKG_COMMAND		"/usr/bin/pkginfo -c set -d " 
#define WHITESPACE		" " 
#define SPACES			"     " 
#define ONE_SPACE		" " 
#define NEWLINE			"\n"	
#define ONE			"1" 
#define DISK_SPACE_REQD		120000 
#define BLOCK_SIZ		512 
#define SLASH			"/"	

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define HELP_FILE		"Install_Server/Install_Server.hlp"

#define TXT_configured	     	"install_server:1" FS "Changed the configuration of the Install Server"
#define TXT_removed		"install_server:2" FS "Removed the Packages"
#define TXT_cancel	  	"install_server:3" FS "Cancel"
#define MNEM_cancel		"install_server:4" FS "C"
#define TXT_help		"install_server:5" FS "Help"
#define MNEM_help 	    	"install_server:6" FS "H"
#define TXT_configure		"install_server:7" FS "Configure"
#define MNEM_configure		"install_server:8" FS "O"
#define TXT_load		"install_server:9" FS "Load"
#define MNEM_load		"install_server:10" FS "L"
#define TXT_remove		"install_server:11" FS "Remove"
#define MNEM_remove		"install_server:12" FS "R"

#define TXT_Title		"install_server:13" FS "Install Server Setup"
#define TXT_appName		"install_server:14" FS "Install_Server"
#define MNEM_application	"install_server:15" FS "N"
#define TXT_setupHelp		"install_server:16" FS "Install Server Setup"
#define TXT_setupHelpSect	"install_server:17" FS "10"

#define TXT_loaded		"install_server:18" FS "The package set has been loaded onto the disk"
#define TXT_loadingtape		"install_server:19" FS "Loading tape. Please don't remove the tape ..."
#define TXT_AppServer		"install_server:20" FS "Application Server"
#define TXT_PersonalEdition	"install_server:21" FS "Personal Edition"

#define TXT_UnixWare		"install_server:22" FS "Configure UnixWare Packages:"
#define TXT_notape		"install_server:23" FS "No tape device."
#define TXT_continue		"install_server:24" FS "Continue"
#define TXT_errpipe		"install_server:25" FS "Pipe Open Error."
#define TXT_sorry		"install_server:26" FS "Pipe error. Could not execute the command."
#define TXT_openfailed		"install_server:27" FS "Warning: Failed to open Sapd File. Cannot disable."
#define TXT_pidfailed		"install_server:28" FS "Warning: Cannot get the process id of the SAPD.  Unable to send signal to the daemon."
#define TXT_signalfailed	"install_server:29" FS "Warning: Unable to send signal to the SAPD daemon "
#define MNEM_Appserver 		"install_server:30" FS "S"
#define MNEM_Personaledition	"install_server:31" FS "P"
#define TXT_LoadFromTape 	"install_server:32" FS "Tape"
#define TXT_LoadFromCDROM 	"install_server:33" FS "CD-ROM"
#define TXT_ActionMenu          "install_server:34" FS "Install Server Setup: Load" 
#define TXT_ActionType          "install_server:35" FS "From: "
#define TXT_Pathname           	"install_server:36" FS "To: "
#define MNEM_LoadfromTape      	"install_server:37" FS "T"
#define MNEM_LoadfromCDROM     	"install_server:38" FS "D"
#define TXT_failedexec     	"install_server:39" FS "Sorry.  Load did not execute"
#define TXT_failedconfig     	"install_server:40" FS "Sorry. Failed to configure the install server"
#define TXT_fileopen     	"install_server:41" FS "Sorry. Failed to open file"
#define TXT_nospace     	"install_server:42" FS "Sorry. Not enough space. Need about 60 megabytes to load the tape"
#define TXT_statfailed     	"install_server:43" FS "Sorry. Could not access file"
#define TXT_notdir     		"install_server:44" FS "Sorry. Directory does not exist"
#define TXT_invalidfs     	"install_server:45" FS "Invalid file system for the CD ROM"
#define TXT_configfile     	"install_server:46" FS "Configuration file error"
#define TXT_mustbedir     	"install_server:47" FS "Invalid path. Must be a directory"
#define TXT_enterfile     	"install_server:49" FS "Enter a path name."
#define TXT_loadCD     		"install_server:50" FS "Configured the packages from CD ROM"
#define TXT_cantremove     	"install_server:51" FS "Cannot remove the package selected"
#define TXT_removeq     	"install_server:52" FS "Do you really want to remove the packages ?"
#define TXT_Ok     		"install_server:53" FS "Yes"
#define MNEM_Ok     		"install_server:54" FS "Y"
#define TXT_No     		"install_server:55" FS "No"
#define MNEM_No     		"install_server:56" FS "N"
#define MNEM_continue		"install_server:57" FS "C"
#define TXT_invalidcdpath     	"install_server:58" FS "Package file does not exist in the CD ROM file system"
#define TXT_path     		"install_server:59" FS "/net_install"
#define TXT_On     		"install_server:60" FS "On: "
#define TXT_makefailed 		"install_server:61" FS "Could not make the default directory."
#define TXT_mustclick 		"install_server:62" FS "Nothing has been selected to configure"
#define TXT_failedtape 		"install_server:63" FS "Could not open the tape."
#define TXT_sapdremove 		"install_server:64" FS "Warning: Failed to remove the sapd file."
#define TXT_sapdcreate 		"install_server:65" FS "Warning: Failed to create the sapd file."

#endif

