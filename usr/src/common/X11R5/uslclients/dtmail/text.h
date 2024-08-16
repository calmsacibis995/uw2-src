/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef NOIDENT
#ident	"@(#)dtmail:text.h	1.50"
#endif

#define BUT_READ_PROP		"dtmail2:2" FS "Read Options..."
#define BUT_SEND_PROP		"dtmail2:3" FS "Compose Options..."
#define BUT_EXIT		"dtmail2:4" FS "Exit"
#define BUT_MANAGE_MAIL_DDD	"dtmail2:5" FS "Manager..."
#define BUT_READ_MAIL_DDD	"dtmail2:6" FS "Read..."
#define BUT_SEND_MAIL_DDD	"dtmail2:7" FS "Compose..."
#define BUT_ALIAS_MNG_DDD	"dtmail2:8" FS "Alias Manager..."
#define BUT_MAIL_DDD		"dtmail2:9" FS "Mail..."
#define BUT_TOC_DDD		"dtmail2:10" FS "Table of Contents..."
#define BUT_HELP_DESK_DDD	"dtmail2:11" FS "Help Desk..."
#define BUT_MANAGER_DDD		"dtmail2:12" FS "Mail Manager..."
#define BUT_READER_DDD		"dtmail2:13" FS "Read Mail..."
#define BUT_SENDER_DDD		"dtmail2:14" FS "Compose Message..."
#define BUT_ALIAS_DDD		"dtmail2:15" FS "Alias Manager..."
#define BUT_FILE		"dtmail2:16" FS "File"
#define BUT_EDIT		"dtmail2:17" FS "Edit"
#define BUT_VIEW		"dtmail2:18" FS "View"
#define BUT_MAIL		"dtmail2:19" FS "Mail"
#define BUT_HELP		"dtmail2:20" FS "Help"
#define BUT_MESS		"dtmail2:21" FS "Message"
#define BUT_OPEN		"dtmail2:22" FS "Open"
#define BUT_OPEN_DDD		"dtmail2:23" FS "Open..."
#define BUT_SAVE_MESSAGE	"dtmail2:24" FS "Save Message"
#define BUT_SAVE_MESSAGE_AS_DDD	"dtmail2:25" FS "Save Message To ..."
#define BUT_PRINT		"dtmail2:26" FS "Print Message(s)"
#define BUT_SAVE_MESSAGES	"dtmail2:27" FS "Save Message(s)"
#define BUT_SAVE_MESSAGES_AS_DDD	"dtmail2:28" FS "Save Message(s) To ..."
#define BUT_UNDO		"dtmail2:29" FS "Undo"
#define BUT_CUT			"dtmail2:30" FS "Cut"
#define BUT_COPY		"dtmail2:31" FS "Copy"
#define BUT_PASTE		"dtmail2:32" FS "Paste"
#define BUT_DELETE		"dtmail2:33" FS "Delete"
#define BUT_SELECT_ALL		"dtmail2:34" FS "Select All"
#define BUT_UNSELECT_ALL	"dtmail2:35" FS "Unselect All"
#define BUT_UNDELETE_DDD	"dtmail2:36" FS "Undelete..."
#define BUT_MAIL_MANAGER_DDD	"dtmail2:37" FS "Mail"
#define BUT_READ_DDD		"dtmail2:38" FS "Read..."
#define BUT_MAILER_DDD		"dtmail2:39" FS "Compose..."
#define BUT_REPLY_SENDER_DDD	"dtmail2:40" FS "Reply to Sender..."
#define BUT_REPLY_SENDER_ATT_DDD "dtmail2:41" FS "Reply to Sender (Att.)..."
#define BUT_REPLY_ALL_DDD	"dtmail2:42" FS "Reply to All..."
#define BUT_REPLY_ALL_ATT_DDD	"dtmail2:43" FS "Reply to All (Att.)..."
#define BUT_FORWARD_DDD		"dtmail2:44" FS "Forward..."
#define BUT_NEXT		"dtmail2:45" FS "Next"
#define BUT_PREV		"dtmail2:46" FS "Prev"
#define BUT_UNDELETE		"dtmail2:47" FS "Undelete"
#define OLD_BUT_SAVE		"dtmail2:48" FS "Save Unsent Message"
#define BUT_INCLUDE		"dtmail2:49" FS "Insert Selected"
#define BUT_SAVE_AS_DDD		"dtmail2:50" FS "Save Unsent To..."
#define BUT_NEW			"dtmail2:51" FS "Compose New"
#define BUT_ADD			"dtmail2:52" FS "Add"
#define BUT_OVERWRITE		"dtmail2:53" FS "Overwrite"
#define BUT_ALIASES_DDD		"dtmail2:54" FS "Aliases..."
#define BUT_BRIEF		"dtmail2:55" FS "Brief"
#define BUT_FULL		"dtmail2:56" FS "Full"
#define BUT_OK			"dtmail2:57" FS " OK "
#define BUT_CANCEL		"dtmail2:58" FS "Cancel"
#define BUT_SEND		"dtmail2:59" FS "Send"
#define BUT_APPLY		"dtmail2:60" FS "Apply"
#define BUT_RESET		"dtmail2:61" FS "Reset"
#define BUT_SHOW		"dtmail2:62" FS "Show Errors"
#define BUT_ON			"dtmail2:63" FS "On"
#define BUT_OFF			"dtmail2:64" FS "Off"
#define TXT_TO			"dtmail2:65" FS "To:"
#define TXT_SUBJECT		"dtmail2:66" FS "Subject:"
#define TXT_CC			"dtmail2:67" FS "Cc:"
#define TXT_BCC			"dtmail2:68" FS "Bcc:"
#define TXT_NAMES		"dtmail2:69" FS "Names:"
#define TXT_MAIL_MNG_DELETE	"dtmail2:70" FS "Mail Manager: Undelete"
#define TXT_DOTML		"dtmail2:71" FS ".ml"
#define TXT_DOT_MAILRC		"dtmail2:72" FS ".mailrc"
#define TXT_SYSTEM_MAILRC	"dtmail2:73" FS "/usr/lib/mailx/mailx.rc"
#define TXT_MNG_ALREADY_ACTIVE	"dtmail2:74" FS "Already have a Mail Manager displaying this file"
#define TXT_FILE_ALREADY_OPEN	"dtmail2:75" FS "Mail file (%s) is already open."
#define TXT_TOO_LARGE		"dtmail2:76" FS "File too large to read"
#define TXT_CANT_OPEN		"dtmail2:77" FS "Cannot open file: "
#define TXT_INVALID_SELECTION	"dtmail2:78" FS "Invalid selection"
#define TXT_INVALID_TARGET	"dtmail2:79" FS "Messages can only be dropped on mail files (*.ml)"
#define TXT_INVALID_FILENAME	"dtmail2:80" FS "File name is invalid."
#define TXT_MAILRC_ENV		"dtmail2:81" FS "MAILRC"
#define TXT_FORWARD		"dtmail2:82" FS "Your mail is being forwarded"
#define CANT_READ_MF_OPEN	"dtmail2:83" FS "Cannot read mail file: it's already being read"
#define TXT_EMPTY_FILE		"dtmail2:84" FS "Empty file"
#define TXT_CANT_BE_OPENED	"dtmail2:85" FS "Mail file %s cannot be opened - %s"
#define TXT_FILE_DOESNT_EXIST	"dtmail2:86" FS "Mail file %s cannot be opened - file does not exist"
#define TXT_NO_MORE_MESSAGES	"dtmail2:87" FS "No more messages in mail file %s"
#define TXT_CANT_SAVE_FILE	"dtmail2:88" FS "Could not save in %s - %s"
#define TXT_MESSAGE_SAVED	"dtmail2:89" FS "Message(s) saved to %s"
#define TXT_NEW_MAIL		"dtmail2:90" FS "You have new mail in %s"
#define TXT_E_MAIL_COLON	"dtmail2:91" FS "Mail"
#define TXT_MANAGE_RIGHT_FOOTER	"dtmail2:92" FS "%d Messages, %d New, %d Unread"
#define TXT_FIRST_HALF_HEADER	"dtmail2:93" FS "   No Status From               "
#define TXT_SECOND_HALF_HEADER	"dtmail2:94" FS "Date     Time    Size       Subject                  "
#define TXT_ELECTRONIC_MAIL	"dtmail2:95" FS "Mail %s"
#define TXT_E_MAIL_ICON_NAME	"dtmail2:96" FS "Mail"
#define TXT_PRINTED_MSGS	"dtmail2:97" FS "Selected messages have been sent to the default printer"
#define TXT_PRINTED_MSG		"dtmail2:98" FS "Message has been sent to the default printer"
#define TXT_NO_MAIL		"dtmail2:99" FS "No mail"
#define TXT_PRINT_FAILED	"dtmail2:100" FS "Print failed"
#define TXT_NOT_SENT		"dtmail2:101" FS "Message has not been sent.  Exit Sender anyway?"
#define TXT_NEW_TEXT		"dtmail2:102" FS "Message has not been sent.  Clear window anyway?"
#define TXT_RU_SURE		"dtmail2:103" FS "Exit all Mail windows.  Are you sure?"
#define TXT_OPEN_OUTGOING	"dtmail2:104" FS "Compose: Open Unsent Message"
#define TXT_SAVE_OUTGOING	"dtmail2:105" FS "Compose: Save Unsent Message"
#define TXT_SAVE_MANAGE_MAIL	"dtmail2:106" FS "Mail Manager: Save Mail Messages"
#define TXT_SAVE_READ_MAIL	"dtmail2:107" FS "Read Mail: Save Mail Message"
#define TXT_FILE_OVERWRITE	"dtmail2:108" FS "File already exists.  Overwrite it?"
#define TXT_OPEN_MANAGER_FILE	"dtmail2:109" FS "Mail Manager: Open Mail File"
#define TXT_STAR_DOT_ML		"dtmail2:110" FS "*.ml"
#define TXT_OPEN_READ_FILE	"dtmail2:111" FS "Read Mail: Open Mail File"
#define TXT_NO_CHANGE		"dtmail2:112" FS "This mail message has already been sent.  Send it again?"
#define TXT_TRANS_ERROR		"dtmail2:113" FS "Mail may not have been sent due to invalid address"
#define TXT_AT_FIRST		"dtmail2:114" FS "Already at first message."
#define TXT_AT_LAST		"dtmail2:115" FS "Already at last message."
#define TXT_EMAIL_READER	"dtmail2:116" FS "Mail: Read"
#define TXT_MANAGER_ACTIVE	"dtmail2:117" FS "Already have a Mail Manager displaying this file"
#define TXT_BINARY_TEXT		"dtmail2:118" FS "Cannot display message.  It contains binary data."
#define TXT_NO_ITEM		"dtmail2:119" FS "No message selected"
#define TXT_MESSAGE_SENT	"dtmail2:120" FS "Message has been sent"
#define TXT_INVALID_ADDRESS	"dtmail2:121" FS "Mail contains invalid addresses:"
#define TXT_COMPOSE		"dtmail2:122" FS "Mail Sender: Create Message"
#define TXT_HEADER_COLON	"dtmail2:123" FS "Header:"
#define TXT_BRIEF_HEADER	"dtmail2:124" FS "Brief Header Shows:"
#define TXT_PROP_READER		"dtmail2:125" FS "Read Mail Options"
#define TXT_REPLY_TO		"dtmail2:126" FS "Mail Sender: Reply to message from %s"
#define TXT_FORWARD_FROM	"dtmail2:127" FS "Mail Sender: Forwarding message from %s"
#define TXT_SAVED_IN		"dtmail2:128" FS "Saved message in %s"
#define TXT_SEND_PRINTED	"dtmail2:129" FS "Message sent to the default printer"
#define TXT_NOT_A_SAVE_FILE	"dtmail2:130" FS "Cannot open %s: file does not contain a \"Saved\" message."
#define TXT_JUST_MAIL		"dtmail2:131" FS "Mail"
#define TXT_NOT_A_MAIL_FILE	"dtmail2:132" FS "Cannot open %s: not a mail file"
#define TXT_RECORD_OUTGOING	"dtmail2:133" FS "Save copy of outgoing mail:"
#define TXT_SIGNATURE		"dtmail2:134" FS "Signature:"
#define TXT_COMPOSE_PROPERTIES	"dtmail2:135" FS "Compose Options"
#define TXT_OUT_GOING		"dtmail2:136" FS "Mail: Compose Message"
#define TXT_SENDER_ICON_NAME	"dtmail2:137" FS "Compose"
#define TXT_MANAGER_ICON_NAME	"dtmail2:138" FS "Mail"
#define TXT_READER_ICON_NAME	"dtmail2:139" FS "Read"
#define TXT_ALIAS_HEAD_1	"dtmail2:140" FS "  Name                             "
#define TXT_ALIAS_HEAD_2	"dtmail2:141" FS "  Mail Addresses                                             "
#define TXT_NAME_COLON		"dtmail2:142" FS "Name:"
#define TXT_ADDRESS_COLON	"dtmail2:143" FS "Address:"
#define TXT_ALIAS_MANAGER	"dtmail2:144" FS "Mail Alias Manager"
#define TXT_ALIAS_MANAGER_ICON	"dtmail2:145" FS "Alias Mgr"
#define TXT_OVERWRITE_ALIAS	"dtmail2:146" FS "Alias name already in use.	 Overwrite it?"
#define TXT_MUST_BE_A_NAME	"dtmail2:147" FS "You have not entered the name of a new or existing Alias."
#define TXT_MUST_BE_AN_ADDRESS	"dtmail2:148" FS "You have not entered an Address."
#define TXT_ILLEGAL_CHARACTER	"dtmail2:149" FS "\"Name\" may only contain letters and numbers."
#define TXT_NO_CHANGE_NAME	"dtmail2:150" FS "You did not make any changes."
#define TXT_DOT_ALIAS		"dtmail2:151" FS "/tmp/.ALIAS%-d"
#define TXT_CANT_OPEN_4_PRINT	"dtmail2:152" FS "Could not open file for printing"
#define TXT_ALIAS_2_PRINTER	"dtmail2:153" FS "Alias list sent to default printer."
#define TXT_FILE_WAS_SPOOLED	"dtmail2:154" FS "File sent to printer."
#define TXT_PRINT_CANCELED	"dtmail2:155" FS "Print request cancelled."
#define TXT_COULDNT_B_PRINTED	"dtmail2:156" FS "File could not be printed."
#define TXT_PRINTER_NA		"dtmail2:157" FS "Printer not available."
#define TXT_PRINT_TITLE		"dtmail2:158" FS "                           Alias List"
#define TXT_TITLE_NAME		"dtmail2:159" FS "Name"
#define TXT_TITLE_ADDR		"dtmail2:160" FS "Mail Address or Translation"
#define TXT_MAILRC_UNREADABLE	"dtmail2:161" FS "Unreadable .mailrc file"
#define TXT_MAILRC_WRITTEN	"dtmail2:162" FS "Aliases saved."
#define TXT_E_MAIL_UNDELETE	"dtmail2:163" FS "Mail Alias Manager: Undelete"
#define TXT_E_MAIL_ALIASES	"dtmail2:164" FS "Mail Aliases"
#define TXT_R_U_SURE		"dtmail2:165" FS "You have unsaved Alias changes.  Are you sure you want to exit?"
#define TXT_ONLY_ONE_FILE	"dtmail2:166" FS "You can only drop one file on this window"
#define TXT_LUCY_BOLD		"dtmail2:167" FS "-*-lucidatypewriter-bold-r-*-*-*-*-*-*-*-*-iso8859-1"
#define TXT_LUCY_MEDIUM		"dtmail2:168" FS "-*-lucidatypewriter-medium-r-*-*-*-*-*-*-*-*-iso8859-1"
#define TXT_LIMITS_EXCEEDED	"dtmail2:169" FS "Warning: Only the first %d messages can be displayed"
#define TXT_DELETE_LM_EXCEEDED	"dtmail2:170" FS "Warning: Only the first %d deleted messages can be displayed"
#define HELP_MAIL_TITLE			"dtmail2:171" FS "The Mail Application"
#define HELP_MAIL_SECT			"10"
#define HELP_MANAGER_TITLE		"dtmail2:172" FS "Manager"
#define HELP_MANAGER_SECT		"20"
#define HELP_READER_TITLE		"dtmail2:173" FS "Reader Window"
#define HELP_READER_SECT		"30"
#define HELP_SENDER_TITLE		"dtmail2:174" FS "Sender Window"
#define HELP_SENDER_SECT		"90"
#define HELP_ALIAS_MANAGER_TITLE	"dtmail2:175" FS "Alias Manager Window"
#define HELP_ALIAS_MANAGER_SECT		"130"
#define HELP_TOC_TITLE			"dtmail2:176" FS "Table of Contents"
#define HELP_TOC_SECT			"TOC"
#define HELP_DESK_TITLE			"dtmail2:177" FS "Help Desk"
#define HELP_DESK_SECT			"HelpDesk"
#define HELP_MANAGER_OPEN_TITLE		"dtmail2:178" FS "Mail Manager: Open Mail File Window"
#define HELP_MANAGER_OPEN_SECT		"10"
#define HELP_MANAGER_SAVEAS_TITLE	"dtmail2:179" FS "Mail Manager: Save Mail Messages Window"
#define HELP_MANAGER_SAVEAS_SECT	"40"
#define HELP_MANAGER_UNDELETE_TITLE	"dtmail2:180" FS "Mail Manager: Undelete Mail Messages Window"
#define HELP_MANAGER_UNDELETE_SECT	"140"

#define HELP_READER_OPEN_TITLE		"dtmail2:181" FS "Mail Reader: Open Read Mail Messages Window"
#define HELP_READER_OPEN_SECT		"30"
#define HELP_READER_SAVE_TITLE		"dtmail2:182" FS "Mail Reader: Save Read Mail Messages Window"
#define HELP_READER_SAVE_SECT		"50"
#define HELP_SENDER_OPEN_TITLE		"dtmail2:183" FS "Mail Sender: Open Outgoing Mail Messages Window"
#define HELP_SENDER_OPEN_SECT		"90"
#define HELP_SENDER_SAVE_TITLE		"dtmail2:184" FS "Mail Sender: Save Outgoing Mail Messages Window"
#define HELP_MANAGEPROP_SECT		"25"
#define HELP_READPROP_TITLE		"dtmail2:185" FS "Mail Read Properties Window"
#define HELP_READPROP_SECT		"35"
#define HELP_SENDPROP_TITLE		"dtmail2:186" FS "Mail Send Properties Window"
#define HELP_SENDPROP_SECT		"95"
#define HELP_ALIASES_TITLE		"dtmail2:187" FS "Mail Aliases Window"
#define HELP_ALIASES_SECT		"130"
#define HELP_ALIAS_MANAGER_UNDELETE_TITLE "dtmail2:188" FS "Mail Alias Manager Undelete Window"
#define HELP_ALIAS_MANAGER_UNDELETE_SECT  "130"
#define HELP_ALIAS_MANAGER_OVERWRITE_TITLE "dtmail2:189" FS "Mail Alias Manager Overwrite Window"
#define HELP_ALIAS_MANAGER_OVERWRITE_SECT  "10"
#define HELP_ALIAS_MANAGER_SURE_TITLE "dtmail2:190" FS "Mail Alias Manager Check Exit Window"
#define HELP_ALIAS_MANAGER_SURE_SECT  "10"
#define MNEM_ALIASES_DDD		"dtmail2:191" FS "A"
#define MNEM_ALIAS_DDD			"dtmail2:192" FS "A"
#define MNEM_ALIAS_MNG_DDD		"dtmail2:193" FS "A"
#define MNEM_APPLY			"dtmail2:194" FS "A"
#define MNEM_BRIEF			"dtmail2:195" FS "B"
#define MNEM_CANCEL			"dtmail2:196" FS "C"
#define MNEM_COPY			"dtmail2:197" FS "C"
#define MNEM_CUT			"dtmail2:198" FS "t"
#define MNEM_DELETE			"dtmail2:199" FS "D"
#define MNEM_EDIT			"dtmail2:200" FS "E"
#define MNEM_EXIT_E			"dtmail2:201" FS "X"
#define MNEM_EXIT_X			"dtmail2:202" FS "X"
#define MNEM_FILE			"dtmail2:203" FS "F"
#define MNEM_FORWARD_DDD		"dtmail2:204" FS "F"
#define MNEM_FULL			"dtmail2:205" FS "F"
#define MNEM_HELP			"dtmail2:206" FS "H"
#define OLD_MNEM_HELP_DESK_DDD		"dtmail2:207" FS "D"
#define MNEM_INCLUDE			"dtmail2:208" FS "I"
#define MNEM_MAIL			"dtmail2:209" FS "M"
#define MNEM_MAILER_DDD			"dtmail2:210" FS "S"
#define MNEM_MAIL_DDD			"dtmail2:211" FS "M"
#define MNEM_MAIL_MANAGER_DDD		"dtmail2:212" FS "M"
#define MNEM_MANAGER_DDD		"dtmail2:213" FS "M"
#define MNEM_MANAGE_MAIL_DDD		"dtmail2:214" FS "M"
#define MNEM_MESS			"dtmail2:215" FS "M"
#define MNEM_NEW			"dtmail2:216" FS "N"
#define MNEM_NEXT			"dtmail2:217" FS "N"
#define MNEM_OFF			"dtmail2:218" FS "f"
#define MNEM_OK				"dtmail2:219" FS "O"
#define MNEM_ON				"dtmail2:220" FS "O"
#define MNEM_OPEN			"dtmail2:221" FS "O"
#define MNEM_OPEN_DDD			"dtmail2:222" FS "O"
#define MNEM_OVERWRITE			"dtmail2:223" FS "O"
#define MNEM_PASTE			"dtmail2:224" FS "P"
#define MNEM_PREV			"dtmail2:225" FS "P"
#define MNEM_PRINT			"dtmail2:226" FS "P"
#define MNEM_READER_DDD			"dtmail2:227" FS "R"
#define MNEM_READ_DDD			"dtmail2:228" FS "R"
#define MNEM_READ_MAIL_DDD		"dtmail2:229" FS "R"
#define MNEM_READ_PROP			"dtmail2:230" FS "R"
#define MNEM_REPLY_ALL_ATT_DDD		"dtmail2:231" FS "l"
#define MNEM_REPLY_ALL_DDD		"dtmail2:232" FS "A"
#define MNEM_REPLY_SENDER_ATT_DDD	"dtmail2:233" FS "t"
#define MNEM_REPLY_SENDER_DDD		"dtmail2:234" FS "p"
#define MNEM_RESET			"dtmail2:235" FS "R"
#define MNEM_SAVE			"dtmail2:236" FS "S"
#define MNEM_SAVE_AS_DDD		"dtmail2:237" FS "A"
#define MNEM_SAVE_MESSAGE		"dtmail2:238" FS "S"
#define MNEM_SAVE_MESSAGES		"dtmail2:239" FS "S"
#define MNEM_SAVE_MESSAGES_AS_DDD	"dtmail2:240" FS "A"
#define MNEM_SAVE_MESSAGE_AS_DDD	"dtmail2:241" FS "A"
#define MNEM_SELECT_ALL			"dtmail2:242" FS "S"
#define MNEM_SEND			"dtmail2:243" FS "S"
#define MNEM_SENDER_DDD			"dtmail2:244" FS "S"
#define MNEM_SEND_MAIL_DDD		"dtmail2:245" FS "S"
#define MNEM_SEND_PROP_S		"dtmail2:246" FS "S"
#define MNEM_SEND_PROP_t		"dtmail2:247" FS "t"
#define MNEM_TOC_DDD			"dtmail2:248" FS "T"
#define MNEM_UNDELETE			"dtmail2:249" FS "U"
#define MNEM_UNDELETE_DDD_L		"dtmail2:250" FS "L"
#define MNEM_UNDELETE_DDD_N		"dtmail2:251" FS "N"
#define MNEM_UNDO			"dtmail2:252" FS "U"
#define MNEM_UNSELECT_ALL		"dtmail2:253" FS "A"
#define MNEM_VIEW			"dtmail2:254" FS "V"
#define MNEM_SHOW			"dtmail2:255" FS "S"
#define HELP_SENDER_SAVE_TITLE2		"dtmail2:256" FS "Mail Sender: Save Outgoing Mail Messages Window"
#define HELP_SENDER_SAVE_SECT		"90"
#define HELP_SENDER_NEW_SECT		"90"
#define HELP_MAIN_EXIT_SECT		"10"
#define HELP_SENDER_EXIT_SECT		"90"
#define HELP_MAIN_EXIT_TITLE		"dtmail2:257" FS "Exit All Mail Windows"
#define HELP_SENDER_NEW_TITLE		"dtmail2:258" FS "Mail Sender: New Sender Window"
#define TXT_NOT_A_REG_FILE	"dtmail2:259" FS "Cannot open %s: this is not a regular file"
#define TXT_DIRECTORY		"dtmail2:260" FS "Cannot open %s: it is a directory"
#define HELP_SENDER_EXIT_TITLE		"dtmail2:261" FS "Mail Sender: Exit Window"
#define BUT_SAVE_TO_DDD			"dtmail2:262" FS "Save To ..."
#define BUT_NEXT_MSG			"dtmail2:263" FS "Next Msg"
#define BUT_PREV_MSG			"dtmail2:264" FS "Prev Msg"
#define MNEM_SAVE_TO_DDD		"dtmail2:265" FS "v"
#define MNEM_BAR_FORWARD_DDD		"dtmail2:266" FS "w"
#define MNEM_NEXT_MSG			"dtmail2:267" FS "N"
#define MNEM_PREV_MSG			"dtmail2:268" FS "r"
#define TXT_MNGSENDERNOTSENT		"dtmail2:269" FS "You have unsent mail.  Exit anyway?"
#define TXT_FORWARDED_MAIL		"dtmail2:270" FS "Forwarded Mail"
#define TXT_SUBJECT_FORWARDED		"dtmail2:271" FS "%s (fwd)"
#define MNEM_ALIAS_DDD2                 "dtmail2:272" FS "l"
#define TXT_DBLC_MESSAGE_IN		"dtmail2:273" FS "Doubleclick Opens Message in:"
#define BUT_OPEN_READER			"dtmail2:274" FS "Open Reader"
#define BUT_NEW_READER			"dtmail2:275" FS "New Reader"
#define MNEM_OPEN_READER		"dtmail2:276" FS "O"
#define MNEM_NEW_READER			"dtmail2:277" FS "N"
#define TXT_OPEN_NEW_MANAGER_FILE	"dtmail2:278" FS "Mail Manager: Open-New Mail File"
#define BUT_OPEN_NEW_DDD		"dtmail2:279" FS "Open-New..."
#define MNEM_OPEN_NEW_DDD		"dtmail2:280" FS "N"
#define BUT_MANAGE_PROP			"dtmail2:281" FS "Mail Options..."
#define MNEM_MANAGE_PROP		"dtmail2:282" FS "M"
#define BUT_READ_NEW_DDD		"dtmail2:283" FS "Read-New"
#define MNEM_READ_NEW_DDD		"dtmail2:284" FS "N"
#define MINI_HELP_MANAGER_NEW		"dtmail2:285" FS "Compose New Message"
#define MINI_HELP_MANAGER_REPLY		"dtmail2:286" FS "Reply to Message Sender (with Attachment)"
#define MINI_HELP_MANAGER_REPLYALL	"dtmail2:287" FS "Reply to All (with Attachment)"
#define MINI_HELP_MANAGER_FORWARD	"dtmail2:288" FS "Forward Message(s) to Other Users"
#define MINI_HELP_MANAGER_PRINT		"dtmail2:289" FS "Print Message(s)"
#define MINI_HELP_MANAGER_DELETE	"dtmail2:290" FS "Delete Message"
#define MINI_HELP_MANAGER_SAVE		"dtmail2:291" FS "Save Message(s) to SavedMail"
#define MINI_HELP_MANAGER_SAVEAS	"dtmail2:292" FS "Save Message(s) to Specified Mail File"
#define MINI_HELP_MANAGER_OPEN		"dtmail2:293" FS "Open New Mailfile"
#define MINI_HELP_READ_PREVIOUS		"dtmail2:294" FS "Previous Message"
#define MINI_HELP_READ_NEXT		"dtmail2:295" FS "Next Message"
#define MINI_HELP_READ_REPLY		"dtmail2:296" FS "Reply to Message Sender (with Attachment)"
#define MINI_HELP_READ_REPLYALL		"dtmail2:297" FS "Reply to All (with Attachment)"
#define MINI_HELP_READ_FORWARD		"dtmail2:298" FS "Forward Message(s) to Other Users"
#define MINI_HELP_READ_SAVE		"dtmail2:299" FS "Save Message(s) to Standard Mailfile"
#define MINI_HELP_READ_PRINT		"dtmail2:300" FS "Print Message"
#define MINI_HELP_READ_DELETE		"dtmail2:301" FS "Delete Message"
#define MINI_HELP_SEND_SEND		"dtmail2:302" FS "Send Message"
#define MINI_HELP_SEND_NEW		"dtmail2:303" FS "Compose New Message"
#define MINI_HELP_SEND_SAVE		"dtmail2:304" FS "Save Unsent Message"
#define MINI_HELP_SEND_PRINT		"dtmail2:305" FS "Print Message"
#define TXT_HEADER_NO                   "dtmail2:306" FS "No"
#define TXT_HEADER_STATUS               "dtmail2:307" FS "Status"
#define TXT_HEADER_FROM                 "dtmail2:308" FS "From"
#define TXT_HEADER_DATE                 "dtmail2:309" FS "Date"
#define TXT_HEADER_TIME                 "dtmail2:310" FS "Time"
#define TXT_HEADER_SIZE                 "dtmail2:311" FS "Size"
#define TXT_HEADER_SUBJECT              "dtmail2:312" FS "Subject"

/* new for q7 */
#define HELP_MANAGEPROP_TITLE		"dtmail2:313" FS "Mail Manager Properties Window"
#define MNEM_HELP_DESK_DDD		"dtmail2:314" FS "H"
#define BUT_SAVE			"dtmail2:315" FS "Save"
#define TXT_DEFAULT_MAIL_SAVE_FILE	"dtmail2:316" FS "Mailbox/UnsentMailMessage"

/* new for q8 */
#define TXT_MAIL_PROPERTIES		"dtmail2:317" FS "Mail Options"
#define BUT_ALIAS_PRINT_DDD		"dtmail2:318" FS "Print"
