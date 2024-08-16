/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwaccess:main.h	1.6"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/NetWare_Access/main.h,v 1.8 1994/09/22 16:14:53 renuka Exp $"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#endif
*/

#ifndef MAIN_H
#define MAIN_H

typedef struct {
    XtArgVal	lbl;
    XtArgVal	mnem;
    XtArgVal	sensitive;
    XtArgVal	selectProc;
    XtArgVal	dflt;
    XtArgVal	userData;
    XtArgVal	subMenu;
} MenuItem;

typedef struct {
    char	*title;
    char 	*section;
} HelpText;

typedef struct {
    XtArgVal	lbl;
    XtArgVal	selected;
    XtArgVal	mnem;
    XtArgVal	dflt;
} ButtonItem;

Boolean 	PopdownOK;
#define         successful    	1
#define         not_attached    2
#define         sorry    	3
#define         cantfind      	4	
#define         attached_not_authenticated      	5	


#define APPNAME 	"NetWare_Access"

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define HELP_FILE		"NetWare_Access/NetWare_Access.hlp"
#define TXT_actions		"xnetware:1" FS "Actions"
#define MNEM_actions		"xnetware:2" FS "A"
#define TXT_exit		"xnetware:3" FS "Exit"
#define MNEM_exit		"xnetware:4" FS "x"
#define TXT_appTitle		"xnetware:5" FS "NetWare Access"

#define TXT_application		"xnetware:6" FS "NetWare Access..."
#define MNEM_application	"xnetware:7" FS "N"
#define TXT_appHelp		"xnetware:8" FS "NetWare Access"
#define TXT_appHelpSect		"xnetware:9" FS "10"
#define TXT_helpDesk		"xnetware:10" FS "Help Desk..."
#define MNEM_helpDesk		"xnetware:11" FS "H"
#define TXT_TOC			"xnetware:12" FS "Table of Contents..."
#define MNEM_TOC		"xnetware:13" FS "T"
#define TXT_tocHelp		"xnetware:14" FS "Table of Contents"
#define TXT_helpW		"xnetware:15" FS "Help..."
#define MNEM_helpW		"xnetware:16" FS "H"
#define TXT_appName		"xnetware:17" FS "NetWare_Access"

#define TXT_new			"xnetware:18" FS "New User"
#define MNEM_new		"xnetware:19" FS "N"
#define TXT_change		"xnetware:20" FS "  Password "
#define MNEM_pwd		"xnetware:21" FS "P"

#define TXT_help        	"xnetware:22" FS "Help"
#define MNEM_help       	"xnetware:23" FS "H"
#define TXT_continue        	"xnetware:24" FS "Continue"

#define TXT_FileServers         "xnetware:25" FS "File Servers"
#define TXT_NodeAddress       	"xnetware:26" FS "Node Address"
#define TXT_NwAddress       	"xnetware:27" FS "Network Address"
#define TXT_userName       	"xnetware:28" FS "Authenticated"

#define TXT_mask       		"xnetware:29" FS "  Select   "
#define MNEM_mask       	"xnetware:30" FS "S"
#define TXT_authenticate       	"xnetware:31" FS "   Login   "
#define MNEM_auth      		"xnetware:32" FS "I"
#define TXT_deauthenticate     	"xnetware:33" FS "  Logout   "
#define MNEM_deauth		"xnetware:34" FS "O"
#define TXT_Novolumes     	"xnetware:35" FS "No volume information found"
#define MNEM_unmask    		"xnetware:36" FS "L"
#define TXT_unmask    		"xnetware:37" FS "  Show All "
#define TXT_userlist    	"xnetware:38" FS " User List "
#define MNEM_users	    	"xnetware:39" FS "U"

#define TXT_NWWHOAMI1           "xnetware:40" FS "Management portal open failed"
#define TXT_NWWHOAMI2           "xnetware:41" FS "ScanServiceByUser returned error"
#define TXT_Notattached         "xnetware:42" FS "You are not authenticated to "
#define TXT_Sorry 		"xnetware:43" FS "Sorry."
#define TXT_Cantfind            "xnetware:44" FS "Cannot find file server " 
#define TXT_noneselected        "xnetware:45" FS "Cannot De-authenticate. No server selected."
#define TXT_didnotauth          "xnetware:46" FS "De-authentication Errors : "
#define TXT_cantauth            "xnetware:47" FS "Cannot Authenticate more than one server at a time"

#define TXT_save            	"xnetware:48" FS "Apply"
#define TXT_cancel            	"xnetware:49" FS "Cancel"
#define MNEM_save            	"xnetware:50" FS "A"
#define MNEM_cancel            	"xnetware:51" FS "C"

#define TXT_authtitle          	"xnetware:52" FS "Authentication Panel"
#define TXT_login          	"xnetware:53" FS "Login :"
#define TXT_pwd          	"xnetware:54" FS "Password :"
#define TXT_cantlogin          	"xnetware:55" FS "Sorry You are already authenticated"
#define TXT_unknownserver      	"xnetware:56" FS "Cannot Find File Server "
#define TXT_sorrylogin         	"xnetware:57" FS "Sorry. Authentication failed."
#define TXT_loginsucc         	"xnetware:58" FS "Authentication was successful."
#define TXT_cantattach         	"xnetware:59" FS "Cannot Attach to server:"
#define TXT_enteruser      	"xnetware:60" FS "Please enter the login name."
#define TXT_enterpasswd      	"xnetware:61" FS "Please enter the password."
#define TXT_noneauthselected    "xnetware:62" FS "Cannot Authenticate. No server selected."
#define TXT_header    		"xnetware:63" FS "File Server                         Network Address   NodeAddress    Authenticated"

#define TXT_volume      	"xnetware:64" FS "Volume List"
#define MNEM_vol          	"xnetware:65" FS "M"
#define TXT_notlogged    	"xnetware:66" FS "You are not logged into any servers"

#define TXT_ulist    		"xnetware:67" FS "User Information for "

#define TXT_failedconn		"xnetware:68" FS "Failed to get connection information from the server"
#define TXT_logoutfail		"xnetware:69" FS "Failed to de-authenticate from "
#define TXT_nwattach		"xnetware:70" FS "Failed to attach to the server"
#define TXT_connection 		"xnetware:71" FS "Connection ID"
#define TXT_uname 		"xnetware:72" FS "User"
#define TXT_logintime 		"xnetware:73" FS "Login Time"
#define TXT_slistfail 		"xnetware:74" FS "Failed to get server list"
#define TXT_cantshowuser	"xnetware:75" FS "Cannot show user list for more than one server"
#define TXT_nouserselected	"xnetware:76" FS "Cannot show user list. No server selected"
#define TXT_notbeenauth		"xnetware:77" FS "Cannot show user list. Not authenticated to the server"
#define TXT_userHelp		"xnetware:78" FS "User List Help"
#define TXT_userHelpSect	"xnetware:79" FS "10"
#define TXT_userfail		"xnetware:80" FS "Failed to get the user list."
#define TXT_cantshowselect	"xnetware:81" FS "Cannot Show select. None selected"
#define TXT_cantgetserverinfo	"xnetware:82" FS "Could not reach the server for information"
#define TXT_cantshowvolume	"xnetware:83" FS "Cannot show volume list for more than one server"
#define TXT_novolselected	"xnetware:84" FS "Cannot show volume list. No server selected"
#define TXT_volnotbeenauth	"xnetware:85" FS "Cannot show volume list. Not authenticated to the server"
#define TXT_volumename		"xnetware:86" FS "Volume"
#define TXT_totblocks		"xnetware:87" FS "Total Bytes"
#define TXT_availblocks		"xnetware:88" FS "Avail Bytes"
#define TXT_totentries		"xnetware:89" FS "Total Directories"
#define TXT_availentries	"xnetware:90" FS "Avail Directories"
#define TXT_VolHelp		"xnetware:91" FS "Volume List Help"
#define TXT_VolHelpSect		"xnetware:92" FS "10"

#define TXT_chgpwd		"xnetware:93" FS "Change Password "
#define TXT_cantshowpwd		"xnetware:94" FS "Cannot change password for more than one server"
#define TXT_nopwdselected	"xnetware:95" FS "Cannot change password. No server selected"
#define TXT_pwdnotbeenauth	"xnetware:96" FS "Cannot change password. Not authenticated to the server"
#define TXT_enteroldpwd      	"xnetware:97" FS "Please enter the old password"
#define TXT_enternewpwd      	"xnetware:98" FS "Please enter the new password"
#define TXT_oldpwd      	"xnetware:99" FS "Old Password :"
#define TXT_newpwd      	"xnetware:100" FS "New Password :"
#define TXT_chgpwdsucc      	"xnetware:101" FS "Changed the Password"
#define TXT_duppwd      	"xnetware:102" FS "Password not unique"
#define TXT_noright      	"xnetware:103" FS "No Property write privilege to change password"
#define TXT_invalidbindery     	"xnetware:104" FS "Invalid bindery security"
#define TXT_nosuchobject     	"xnetware:105" FS "Cannot change password.  No such object"
#define TXT_loginHelp		"xnetware:106" FS "Login"
#define TXT_loginHelpSect	"xnetware:107" FS "10"
#define TXT_pwdHelp		"xnetware:108" FS "Change Password"
#define TXT_pwdHelpSect		"xnetware:109" FS "10"

#define TXT_nousersfound	"xnetware:110" FS "Sorry no other users found on the UnixWare System"
#define TXT_newusers		"xnetware:111" FS "Choose User"
#define TXT_loginname		"xnetware:112" FS "    Users"
#define TXT_NewLoginHelp	"xnetware:113" FS "New User Help"
#define TXT_NewLoginHelpSect	"xnetware:114" FS "10"
#define TXT_execfailed		"xnetware:115" FS "Failed to fork new process"
#define TXT_selectuser		"xnetware:116" FS "Please select user"

#define TXT_view		"xnetware:117" FS "View"
#define TXT_format		"xnetware:118" FS "Format"
#define TXT_long		"xnetware:119" FS "Long"
#define TXT_short		"xnetware:120" FS "Short"
#define MNEM_view		"xnetware:121" FS "V"
#define MNEM_format		"xnetware:122" FS "F"
#define MNEM_long		"xnetware:123" FS "L"
#define MNEM_short		"xnetware:124" FS "S"
#define MNEM_saveselect		"xnetware:125" FS "S"
#define TXT_saveselect		"xnetware:126" FS "Save Select"

#define TXT_nomatch    		"xnetware:127" FS "Sorry. Passwords do not match."
#define TXT_noservers    	"xnetware:128" FS "No File Servers selected for saving"
#define TXT_filenotfound    	"xnetware:129" FS "Database file not found"
#define TXT_savedselect    	"xnetware:130" FS "Saved the selected servers"
#define TXT_enterpwd    	"xnetware:131" FS "Password :"
#define TXT_EnterpwdHelp	"xnetware:132" FS "Enter the password for user"
#define TXT_EnterpwdHelpSect	"xnetware:133" FS "10"
#define TXT_Noserver		"xnetware:134" FS "No server found matching the wildcard "
#define TXT_Detachfailed	"xnetware:135" FS "Detach failed"
#define TXT_Attachfailed	"xnetware:136" FS "Attach to NetWare server failed"
#define TXT_invalidparam	"xnetware:137" FS "Invalid parameters"
#define MNEM_continue		"xnetware:138" FS "C"
#define TXT_ErrorWindow		"xnetware:139" FS "Deauthentication Errors"
#define TXT_attachednotauth	"xnetware:140" FS "You were attached, not authenticated to "
#define TXT_invalidpwd		"xnetware:141" FS "Sorry. Invalid password."
#define TXT_toomany		"xnetware:142" FS "Too many failures. Try again later."
#define TXT_retypepwd      	"xnetware:143" FS "Retype New Password :"
#define TXT_retypenewpwd      	"xnetware:144" FS "Sorry. Retype New Password."
#define TXT_nosuchprop 		"xnetware:145" FS "Password property does not exist."
#define TXT_loginlock 		"xnetware:146" FS "Login Lockout."
#define TXT_pwdshort 		"xnetware:147" FS "Password too short."
#define TXT_nosuchset 		"xnetware:148" FS "Password property set does not exist."
#define TXT_badpwd 		"xnetware:149" FS "Sorry. Bad password."
#define TXT_failedpwdchg 	"xnetware:150" FS "Sorry. Failed to change the password."
#define TXT_update 		"xnetware:151" FS "Update Servers"
#define MNEM_update 		"xnetware:152" FS "U"
#define TXT_userempty 		"xnetware:153" FS "Empty User List."

#define TXT_message 		"xnetware:154" FS "Message"
#define TXT_for 		    "xnetware:155" FS " for "
#define TXT_on 		        "xnetware:156" FS " on "

#define TXT_defaultconnfailed	"xnetware:157" FS "Default Connection failed."
#define TXT_connstatusfailed	"xnetware:158" FS "Failed to get connection status."
#define TXT_propreadfail	"xnetware:159" FS "Failed to read property value."
#define TXT_invalidconn		"xnetware:160" FS "Invalid connection."
#define TXT_connlogged		"xnetware:161" FS "Connection logged in."
#define TXT_disableacc		"xnetware:162" FS "Disabled account."
#define TXT_nograce		"xnetware:163" FS "Password has expired. No grace."
#define TXT_pwdexpired		"xnetware:164" FS "Password has expired."
#define TXT_intruderlock	"xnetware:165" FS "Intruder Lock."
#define TXT_unexpected		"xnetware:166" FS "Unexpected Error."

#define TXT_singleloginenabled	"xnetware:167" FS "Disable Single Login"
#define MNEM_singleloginenabled	"xnetware:168" FS "D"
#define TXT_singlelogindisabled	"xnetware:169" FS "Enable Single Login"
#define MNEM_singlelogindisabled "xnetware:170" FS "E"

#define TXT_setprimaryserver	"xnetware:171" FS "Set Primary Server"
#define MNEM_setprimaryserver   "xnetware:172" FS "P"
#define TXT_primservername	"xnetware:173" FS "Primary Server:"
#define TXT_SetPrimHelp		"xnetware:174" FS "Set Primary Server Help"
#define TXT_psreadfail		"xnetware:175" FS "Failed to read Primary Server file "
#define TXT_initfail		"xnetware:176" FS "Requester initialization failed."
#define TXT_getprimconnfail	"xnetware:177" FS "Failed to get Primary Server connection ID."
#define TXT_getserverfail	"xnetware:178" FS "Failed to get server name by connection ID."
#define TXT_setprimconnfail	"xnetware:179" FS "Failed to set Primary Server connection ID."
#define TXT_psopenfail		"xnetware:180" FS "Failed to open Primary Server file "
#define TXT_pswritefail		"xnetware:181" FS "Failed to write to Primary Server file "
#define TXT_enterprimserver	"xnetware:182" FS "Please enter the primary server name"

#define TXT_noserver		"xnetware:183" FS "Error finding NetWare Servers."

#define TXT_slopenfail		"xnetware:184" FS "Error opening single login file."
#define TXT_slremovefail	"xnetware:185" FS "Error removing single login file."
#define TXT_psserverlength	"xnetware:186" FS "Server name cannot exceed 48 characters."
#define MNEM_login		"xnetware:187" FS "L"

#endif /* MAIN_H */
