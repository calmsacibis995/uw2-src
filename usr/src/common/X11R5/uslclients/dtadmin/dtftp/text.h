/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#pragma ident	"@(#)dtftp:text.h	1.1.3.1"
#endif

#define BUT_FILE		"dtftp:1" FS "File"
#define BUT_EDIT		"dtftp:2" FS "Edit"
#define BUT_VIEW		"dtftp:3" FS "View"
#define BUT_FOLDERS		"dtftp:4" FS "Folders"
#define BUT_HELP		"dtftp:5" FS "Help"
#define BUT_CONTINUE		"dtftp:6" FS "Continue"

#define BUT_NEW_FOLDER		"dtftp:10" FS "New Folder..."
#define BUT_OPEN		"dtftp:11" FS "Open"
#define BUT_PRINT		"dtftp:12" FS "Print"
#define BUT_EXIT		"dtftp:13" FS "Exit"

#define BUT_COPY_DDD		"dtftp:20" FS "Copy..."
#define BUT_EDIT_RENAME		"dtftp:21" FS "Rename..."
#define BUT_DELETE		"dtftp:22" FS "Delete"
#define BUT_SEL_ALL		"dtftp:23" FS "Select All"
#define BUT_DESEL_ALL		"dtftp:24" FS "Deselect All"
#define BUT_FPROPERTIES		"dtftp:25" FS "File Properties..."
#define BUT_COPY		"dtftp:26" FS "Copy"

#define BUT_SORT		"dtftp:30" FS "Sort"
#define BUT_FORMAT		"dtftp:31" FS "Format"
#define BUT_UPDATE		"dtftp:32" FS "Update"

#define BUT_PARENT		"dtftp:40" FS "Parent Folder"
#define BUT_OK			"dtftp:42" FS "OK"

#define BUT_RESET		"dtftp:50" FS "Reset"
#define BUT_CANCEL		"dtftp:51" FS "Cancel"
#define BUT_RETRY		"dtftp:52" FS "Retry"
#define BUT_STOP		"dtftp:53" FS "Stop"
#define BUT_RENAME		"dtftp:54" FS "Rename"
#define BUT_OVERWRITE		"dtftp:55" FS "Overwrite"
#define BUT_DONTWRITE		"dtftp:56" FS "Don't Overwrite"

#define BUT_CREATE_OPEN		"dtftp:60" FS "Create & Open"
#define BUT_CREATE		"dtftp:61" FS "Create"
#define BUT_RESET_C		"dtftp:62" FS "Reset Connection"
#define BUT_READ		"dtftp:63" FS "Read"
#define BUT_WRITE		"dtftp:64" FS "Write"
#define BUT_EXECUTE		"dtftp:65" FS "Execute"
#define BUT_VIEW_MSG		"dtftp:66" FS "Show Messages"
#define BUT_HIDE_MSG		"dtftp:67" FS "Hide Messages"

#define BUT_BY_TYPE		"dtftp:70" FS "By Type"
#define BUT_BY_NAME		"dtftp:71" FS "By Name"
#define BUT_BY_SIZE		"dtftp:72" FS "By Size"
#define BUT_BY_TIME		"dtftp:73" FS "By Time"
#define BUT_ICONS		"dtftp:74" FS "Icons"
#define BUT_SHORT		"dtftp:75" FS "Short"
#define BUT_LONG		"dtftp:76" FS "Long"
#define BUT_DONT_EXIT		"dtftp:77" FS "Don't Exit"
#define BUT_ALWAYS		"dtftp:78" FS "Always Display This Warning " \
					       "When Opening Files"
#define BUT_CPROPERTIES		"dtftp:79" FS "Connection Properties..."
#define BUT_PROPERTIES		"dtftp:80" FS "Properties..."
#define BUT_YES			"dtftp:81" FS "Yes"
#define BUT_NO			"dtftp:82" FS "No"
#define BUT_ASCII		"dtftp:83" FS "ASCII"
#define BUT_BINARY		"dtftp:84" FS "Binary"
#define BUT_PROGRAM		"dtftp:85" FS "Program Determines"
#define BUT_DISPLAY_PROGRESS	"dtftp:86" FS "Display Progress Window When " \
					      "Transferring Files"
#define BUT_WARN_READ_ONLY	"dtftp:87" FS "Display 'Read Only' Warning " \
					      "When Opening Files"
#define BUT_RECONNECT		"dtftp:88" FS "Reconnect"
#define BUT_NEW_C		"dtftp:89" FS "New Connection"

#define TXT_FTP_ICON_NAME	"dtftp:300" FS "Graphical ftp"
#define TXT_JUST_FTP		"dtftp:301" FS "dtftp"
#define TXT_PASSWD_TITLE	"dtftp:302" FS "%s: %s - Password"
#define TXT_ENTER_PASSWORD	"dtftp:303" FS "Enter Password for %s:"
#define TXT_LOGIN_FAILED	"dtftp:304" FS "Login on %s failed.  Would you like to try again?\n\nReason given by remote:\n\n%s"
#define TXT_CONNECT_FAILED	"dtftp:305" FS "Login on %s failed.\n\nReason given by remote:\n\n%s"
#define TXT_FTP_TITLE		"dtftp:306" FS "%s: %s - %s"
#define TXT_ERROR		"dtftp:307" FS "FTP - Error"
#define TXT_ENTER_USERNAME	"dtftp:308" FS "Enter User Name for %s:"
#define TXT_SYNTAX_ERROR	"dtftp:310" FS "Syntax error, command unrecognized"
#define TXT_SYNTAX_PARAM_ERROR	"dtftp:311" FS "Syntax error in parameters or arguments"
#define TXT_NO_CMD		"dtftp:312" FS "Command not implemented"
#define TXT_BAD_SEQUENCE	"dtftp:313" FS "Bad sequence of commands"
#define TXT_NO_SPACE		"dtftp:314" FS "Exceeded storage allocation"
#define TXT_SLIDER_MIN		"dtftp:320" FS "0%"
#define TXT_SLIDER_MAX		"dtftp:321" FS "100%"
#define TXT_FILE_TRANS		"dtftp:322" FS "FTP - File Transfer in Progress"
#define TXT_TO			"dtftp:327" FS "To:"
#define TXT_OPEN_FOLDER		"dtftp:328" FS "Open"
#define TXT_RENAME_TITLE	"dtftp:329" FS "%s: %s - Rename"
#define TXT_RENAME_CAPTION	"dtftp:330" FS "New name:"
#define TXT_COPY_TITLE		"dtftp:331" FS "Copy From %s to %s"
#define TXT_COPY_FILES		"dtftp:332" FS "Copy:"
#define TXT_COMMA		"dtftp:333" FS ", "
#define TXT_ELIPSIS		"dtftp:334" FS "..."
#define TXT_FILE_EXISTS		"dtftp:335" FS "The file `%s' already " \
					       "exists on %s."\
					    "\n\nDo you want to overwrite it?"
#define TXT_FILE_EXISTS_TITLE	"dtftp:336" FS "FTP - Overwrite Warning"
#define TXT_SELECT_FILE		"dtftp:337" FS "Please select a file"
#define TXT_CANT_COPY		"dtftp:338" FS "Cannot copy file:"
#define TXT_DEST_DIR		"dtftp:339" FS "Desination is a directory"
#define TXT_NEW_DIR		"dtftp:340" FS "FTP - New Folder"
#define TXT_NOT_EMPTY		"dtftp:341" FS "Directory `%s' not empty"
#define TXT_FILE_NAME		"dtftp:342" FS "File Name:"
#define TXT_OWNER		"dtftp:343" FS "Owner:"
#define TXT_GROUP		"dtftp:344" FS "Group:"
#define TXT_MOD_TIME		"dtftp:345" FS "Modification Time:"
#define TXT_OWNER_ACCESS	"dtftp:346" FS "Owner Access:"
#define TXT_GROUP_ACCESS	"dtftp:347" FS "Group Access:"
#define TXT_OTHER_ACCESS	"dtftp:348" FS "Other Access:"
#define TXT_FILE_PROPERTY	"dtftp:349" FS "FTP - File Properties"
#define TXT_COPY_IN_PROGRESS	"dtftp:350" FS "You are copying files:\n\n"\
					       "Do you want to exit?"
#define TXT_IN_PROGRESS_TITLE	"dtftp:351" FS "FTP - Copy In Progress"
#define TXT_ABBEND		"dtftp:352" FS \
	"An unrecoverable error was encountered while processing output " \
	"from the remote host.  You can view a history of the events " \
	"leading upto the error in the file %s."
#define TXT_ERROR_LOG		"dtftp:353" FS "/var/tmp/dtftp.log"
#define TXT_ERROR_HEADER	"dtftp:354" FS "--- Start dtftp error log ---\n"
#define TXT_ERROR_TRAILER	"dtftp:355" FS "--- End dtftp error log ---\n"
#define TXT_ERROR_OUTPUT	"dtftp:356" FS "Output: "
#define TXT_ERROR_READ		"dtftp:357" FS "Read: "
#define TXT_MESSAGES		"dtftp:359" FS "Messages:"
#define TXT_INVALID_DROP_SITE	"dtftp:360" FS "The icon you dropped on " \
					       "does not accept drops."
#define TXT_NO_LINKS		"dtftp:361" FS "Linking files across ftp " \
					       "connections is not allowed"
#define TXT_READ_ONLY_TITLE	"dtftp:362" FS "FTP - Open File As 'Read Only'"
#define TXT_READ_ONLY		"dtftp:363" FS \
"This file is being opened for 'Read Only' access.  Although the\n" \
"application being run will allow you to make changes, any\n" \
"changes you make will not be saved on the remote system.\n" \
"You must copy the file to your local system if you want to\n" \
"make copies to it."
#define TXT_SELECTED_ITEMS	"dtftp:364" FS "Selected Items: %d"
#define TXT_TOTAL_ITEMS		"dtftp:365" FS "Total Items: %d - " \
					       "Sorted By: %s"
#define TXT_SORT_BY_TYPE	"dtftp:366" FS "Type"
#define TXT_SORT_BY_NAME	"dtftp:367" FS "Name"
#define TXT_SORT_BY_SIZE	"dtftp:368" FS "Size"
#define TXT_SORT_BY_TIME	"dtftp:369" FS "Time"
#define TXT_CONNECTION_PROPERTY	"dtftp:370" FS "FTP - Connection Properties"
#define TXT_SYSTEM_NAME		"dtftp:371" FS "You logged into:"
#define TXT_LOGGED_IN_AS	"dtftp:372" FS "As:"
#define TXT_ACTUAL_SYSTEM	"dtftp:373" FS "Actual Connection is to:"
#define TXT_CONNECT_SITE	"dtftp:374" FS "Connection Site"
#define TXT_TIMEOUTS		"dtftp:375" FS "Time-outs"
#define TXT_FILE_TRANSFERS	"dtftp:376" FS "File Transfers"
#define TXT_TIMEOUT_INTERVAL	"dtftp:377" FS "Time-out Interval (Minutes):"
#define TXT_DISCONNECT_AFTER	"dtftp:378" FS "Disconnect After Time-Out?"
#define TXT_TRANSFER_FOLDER	"dtftp:379" FS "Transfer Folder:"
#define TXT_TRANSFER_MODE	"dtftp:380" FS "Transfer Mode:"
#define TXT_TIMEOUT_TITLE	"dtftp:381" FS "FTP - Connection Time Out"
#define TXT_CONNECT_TIMEOUT	"dtftp:382" FS "Your connection to '%s' has "\
					       "been disconnected after %d " \
					       "minutes of dis-use.\n\n" \
					       "Click 'Reconnect' if you want to re-establish the connection."
#define TXT_TITLE_DIR_DNE	"dtftp:383" FS "FTP - Folder Does Not Exist"
#define TXT_DIR_DOESNT_EXIST	"dtftp:384" FS "'%s' does not exist.  " \
					       "Do you want to create it?"
#define TXT_CANT_CREATE_DIR	"dtftp:385" FS "Cannot create folder '%s': %s"
#define TXT_COPY_ERROR		"dtftp:386" FS "FTP - Copy Error"
#define TXT_NOT_PLAIN_FILE	"dtftp:387" FS "not a plain file."
#define TXT_CANT_LIST		"dtftp:390" FS "Cannot list directory:"
#define TXT_ENTER_DIRNAME	"dtftp:391" FS "Directory Name:"
#define TXT_COPYING_LABEL	"dtftp:392" FS "Copying:"
#define TXT_COPYING_FROM_LABEL	"dtftp:393" FS "From:"
#define TXT_COPYING_TO_LABEL	"dtftp:394" FS "to:"
#define TXT_COPYING_ON_LABEL	"dtftp:395" FS "on:"
#define TXT_COPYING		"dtftp:396" FS "'%s'"
#define TXT_COPYING_FROM	"dtftp:397" FS "%s"
#define TXT_COPYING_TO		"dtftp:398" FS "'%s' folder"
#define TXT_COPYING_ON		"dtftp:399" FS "%s"
#define TXT_NO_DROPS		"dtftp:400" FS "The object you dropped on " \
					       "doesn't accept drops"
#define TXT_CANT_DELETE		"dtftp:401" FS "Cannot delete file:"
#define TXT_INCOMPLETE_TRANSFER	"dtftp:402" FS "The file: %s may not be complete"
#define TXT_WARNING		"dtftp:410" FS "FTP - Warning"
#define TXT_NEW_MESSAGE		"dtftp:411" FS "New message"
#define TXT_NO_REMOTE_REMOTE	"dtftp:412" FS "Drag and drop between " \
					       "remotes is not allowed"

#define MNEM_FILE		"dtftp:500" FS "F"
#define MNEM_EDIT		"dtftp:501" FS "E"
#define MNEM_VIEW		"dtftp:502" FS "V"
#define MNEM_FOLDERS		"dtftp:503" FS "D"
#define MNEM_HELP		"dtftp:504" FS "H"
#define MNEM_CONTINUE		"dtftp:505" FS "O"

#define MNEM_NEW_FOLDER		"dtftp:510" FS "N"
#define MNEM_OPEN		"dtftp:511" FS "O"
#define MNEM_PRINT		"dtftp:512" FS "P"
#define MNEM_EXIT		"dtftp:513" FS "X"

#define MNEM_COPY		"dtftp:520" FS "C"
#define MNEM_RENAME		"dtftp:521" FS "R"
#define MNEM_DELETE		"dtftp:522" FS "D"
#define MNEM_SEL_ALL		"dtftp:523" FS "S"
#define MNEM_DESEL_ALL		"dtftp:524" FS "A"
#define MNEM_FPROPERTIES	"dtftp:525" FS "F"

#define MNEM_SORT		"dtftp:530" FS "S"
#define MNEM_FORMAT		"dtftp:531" FS "F"
#define MNEM_UPDATE		"dtftp:532" FS "U"
#define MNEM_HIDE_PATH		"dtftp:533" FS "H"

#define MNEM_PARENT		"dtftp:540" FS "A"

#define MNEM_CANCEL		"dtftp:550" FS "C"
#define MNEM_APPLY		"dtftp:551" FS "A"
#define MNEM_RESET		"dtftp:552" FS "R"
#define MNEM_RETRY		"dtftp:553" FS "R"
#define MNEM_OK			"dtftp:554" FS "O"
#define MNEM_STOP		"dtftp:555" FS "S"
#define MNEM_COPY_CANCEL	"dtftp:556" FS "X"
#define MNEM_OVERWRITE		"dtftp:557" FS "O"
#define MNEM_DONTWRITE		"dtftp:558" FS "D"

#define MNEM_CREATE_OPEN	"dtftp:560" FS "O"
#define MNEM_CREATE		"dtftp:561" FS "R"
#define MNEM_RESET_C		"dtftp:562" FS "S"
#define MNEM_VIEW_MSG		"dtftp:563" FS "S"
#define MNEM_HIDE_MSG		"dtftp:564" FS "H"

#define MNEM_BY_TYPE		"dtftp:570" FS "P"
#define MNEM_BY_NAME		"dtftp:571" FS "N"
#define MNEM_BY_SIZE		"dtftp:572" FS "S"
#define MNEM_BY_TIME		"dtftp:573" FS "T"
#define MNEM_ICONS		"dtftp:574" FS "I"
#define MNEM_SHORT		"dtftp:575" FS "S"
#define MNEM_LONG		"dtftp:576" FS "L"
#define MNEM_DONT_EXIT		"dtftp:577" FS "D"

#define MNEM_ICON_OPEN		"dtftp:580" FS "O"
#define MNEM_ICON_PROP		"dtftp:581" FS "T"
#define MNEM_ICON_DELETE	"dtftp:582" FS "D"
#define MNEM_ICON_PRINT		"dtftp:583" FS "P"
#define MNEM_BAR_DELETE		"dtftp:584" FS "L"
#define MNEM_CPROPERTIES	"dtftp:585" FS "P"
#define MNEM_RECONNECT		"dtftp:586" FS "R"
#define MNEM_YES		"dtftp:587" FS "Y"
#define MNEM_NO			"dtftp:588" FS "N"
#define MNEM_NEW_C		"dtftp:588" FS "C"
