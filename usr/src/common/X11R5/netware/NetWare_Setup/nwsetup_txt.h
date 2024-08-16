/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)nwsetup:nwsetup_txt.h	1.10"
// nwsetup_txt.h

//////////////////////////////////////////////////////////////////
// Copyright (c) 1993 Novell
// All Rights Reserved
//
// THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF NOVELL
//
// The copyright notice above does not evidence any
// actual or intended publication of such source code.
//////////////////////////////////////////////////////////////////

#ifndef NWSETUP_TXT_H
#define NWSETUP_TXT_H

#define APP_NAME	"nwsetup"
#define APPNAME		"nwsetup"

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define TXT_title			"nwsetup2:1" FS "NetWare Setup"
#define TXT_hide_item			"nwsetup2:2" FS "Hide Main Help Text"
#define TXT_show_item			"nwsetup2:3" FS "Show Main Help Text"
#define TXT_action_menu			"nwsetup2:4" FS "Action"
#define TXT_scs_item			"nwsetup2:5" FS "Save Current Settings"
#define TXT_rps_item			"nwsetup2:6" FS "Restore Previous Settings"
#define TXT_rds_item			"nwsetup2:7" FS "Restore Default Settings"
#define TXT_exit_item			"nwsetup2:8" FS "Exit   "
#define TXT_help_menu			"nwsetup2:9" FS "Help"
#define TXT_nwsetup_help_item		"nwsetup2:10" FS "NetWare Setup..."
#define TXT_toc_help_item		"nwsetup2:11" FS "Table of Contents..."
#define TXT_hd_help_item		"nwsetup2:12" FS "Help Desk..."
#define TXT_string1			"nwsetup2:13" FS "NetWare UNIX Client: "
#define TXT_string2			"nwsetup2:14" FS "Server Name: "
#define TXT_string3			"nwsetup2:15" FS "IPX Internal LAN Address: "
#define TXT_string4			"nwsetup2:16" FS "IPX Maximum Hops: "
#define TXT_string5			"nwsetup2:17" FS "Logical LAN Configuration: "
#define TXT_string6			"nwsetup2:18" FS "Sequenced Packet eXchange: "
#define TXT_string7			"nwsetup2:19" FS "Service Advertising Protocol: "
#define TXT_string8			"nwsetup2:20" FS "Network Management: "
#define TXT_string9			"nwsetup2:21" FS "Diagnostics Daemon: "
#define TXT_frametype_string1		"nwsetup2:22" FS "Ethernet II"
#define TXT_frametype_string2		"nwsetup2:23" FS "Ethernet 802.2"
#define TXT_frametype_string3		"nwsetup2:24" FS "Ethernet 802.3"
#define TXT_frametype_string4		"nwsetup2:25" FS "Ethernet SNAP"
#define TXT_frametype_string5		"nwsetup2:26" FS "Token Ring 802.5"
#define TXT_frametype_string6		"nwsetup2:27" FS "Token Ring SNAP"
#define TXT_on_string			"nwsetup2:28" FS "On"
#define TXT_off_string			"nwsetup2:29" FS "Off"
#define TXT_string5a			"nwsetup2:30" FS "LAN options..."
#define TXT_dismiss			"nwsetup2:31" FS "Close"
#define TXT_string8a			"nwsetup2:32" FS "Network options..."
#define TXT_dialog5_string1		"nwsetup2:33" FS "Logical LAN: "
#define TXT_dialog5_string2		"nwsetup2:34" FS "IPX LAN Device: "
#define TXT_dialog5_string3		"nwsetup2:35" FS "IPX LAN Frame Type: "
#define TXT_dialog5_string4		"nwsetup2:36" FS "IPX External LAN Address: "
#define TXT_dialog5_string5		"nwsetup2:37" FS "LAN Speed (kilobytes/second): "
#define TXT_ok				"nwsetup2:38" FS "OK"
#define TXT_invalid_frame_type		"nwsetup2:39" FS "Frame type must be unique on an IPX LAN\ndevice.  Please re-select a different\nframe type or LAN device."
#define TXT_dialog6_string1		"nwsetup2:40" FS "SPX Network Remote Login (NVT): "
#define TXT_dialog6_string2		"nwsetup2:41" FS "Maximum SPX Connections: "
#define TXT_dialog6_string3		"nwsetup2:42" FS "Maximum SPX Sockets: "
#define TXT_cancel			"nwsetup2:43" FS "Cancel"
#define TXT_help			"nwsetup2:44" FS "Help..."
#define TXT_lan_string1			"nwsetup2:45" FS "Logical LAN 1"
#define TXT_lan_string2			"nwsetup2:46" FS "Logical LAN 2"
#define TXT_lan_string3			"nwsetup2:47" FS "Logical LAN 3"
#define TXT_lan_string4			"nwsetup2:48" FS "Logical LAN 4"
#define TXT_lan_string5			"nwsetup2:49" FS "Logical LAN 5"
#define TXT_lan_string6			"nwsetup2:50" FS "Logical LAN 6"
#define TXT_lan_string7			"nwsetup2:51" FS "Logical LAN 7"
#define TXT_lan_string8			"nwsetup2:52" FS "Logical LAN 8"
#define TXT_exit_mnemonic		"nwsetup2:53" FS "x"
#define TXT_file_menu			"nwsetup2:54" FS "File"
#define TXT_invalid_external_addr	"nwsetup2:55" FS "External LAN address must be unique.\nPlease re-enter a unique address."
#define TXT_invalid_spx_sockets		"nwsetup2:56" FS "Invalid value for Maximum SPX Sockets\nfield.  Please re-enter a new value."
#define TXT_invalid_spx_connections	"nwsetup2:57" FS "Invalid value for Maximum SPX Connections\nfield.  Please re-enter a new value."
#define TXT_dialog7_string1		"nwsetup2:58" FS "SAP Servers: "
#define TXT_invalid_sap_servers		"nwsetup2:59" FS "Invalid value for SAP Servers field.\nPlease re-enter a new value."
#define TXT_dialog8_string1		"nwsetup2:60" FS "NUC Network Management: "
#define TXT_dialog8_string2		"nwsetup2:61" FS "Host Resource MIB Network Management: "
#define TXT_dialog8_string3		"nwsetup2:62" FS "NPS Network Management: "
#define TXT_dialog8_string4		"nwsetup2:63" FS "Network Management Trap Time: "
#define TXT_invalid_trap_time		"nwsetup2:64" FS "Invalid value for Network Management Trap\nTime field.  Please re-enter a new value."
#define TXT_reboot_decision		"nwsetup2:65" FS "The system must be rebooted for\nchanges to take effect.  Do you\nwant to reboot the system now?"
#define TXT_reboot_info			"nwsetup2:66" FS "Changes will take effect after\nnext system reboot."
#define TXT_icon_label			"nwsetup2:67" FS "NetWare_Setup"
#define TXT_string2a			"nwsetup2:68" FS "Enable IPX Auto Discovery: "
#define TXT_string10			"nwsetup2:69" FS "Remote NUC Auto Authentication: "
#define TXT_invalid_frame_option	"nwsetup2:70" FS "ETHERNET 802.3 are exclusive of ETHERNET\n802.2 and ETHERNET SNAP.  Only ETHERNET II\nis inclusive with either group.  Please re-select\na different frame type or LAN device."
#define TXT_get_param_error		"nwsetup2:71" FS "Error in NWCMGetParam call - "
#define TXT_set_param_error		"nwsetup2:72" FS "Error in NWCMSetParam call - "
#define TXT_get_param_default_error	"nwsetup2:73" FS "Error in NWCMGetParamDefault call - "
#define TXT_view_menu			"nwsetup2:74" FS "View"
#define TXT_file_mnemonic		"nwsetup2:75" FS "F"
#define TXT_help_mnemonic		"nwsetup2:76" FS "H"
#define TXT_action_mnemonic		"nwsetup2:77" FS "A"
#define TXT_view_mnemonic		"nwsetup2:78" FS "V"
#define TXT_scs_mnemonic		"nwsetup2:79" FS "C"
#define TXT_rps_mnemonic		"nwsetup2:80" FS "P"
#define TXT_rds_mnemonic		"nwsetup2:81" FS "D"
#define TXT_show_mnemonic		"nwsetup2:82" FS "S"
#define TXT_hide_mnemonic		"nwsetup2:83" FS "H"
#define TXT_nwsetup_help_mnemonic	"nwsetup2:84" FS "N"
#define TXT_toc_help_mnemonic		"nwsetup2:85" FS "T"
#define TXT_hd_help_mnemonic		"nwsetup2:86" FS "H"
#define TXT_string11			"nwsetup2:87" FS "Enable NetWare Single Login: "
#define TXT_yes				"nwsetup2:88" FS "Yes"
#define TXT_no				"nwsetup2:89" FS "No"
#define MNEM_ok				"nwsetup2:90" FS "O"
#define MNEM_cancel			"nwsetup2:91" FS "C"
#define MNEM_help			"nwsetup2:92" FS "H"
#define MNEM_yes			"nwsetup2:93" FS "Y"
#define MNEM_no				"nwsetup2:94" FS "N"
#define TXT_string7a			"nwsetup2:95" FS "Peer to Peer Communication: "
#define TXT_not_saved			"nwsetup2:96" FS "Your changes have not been saved.\nSave them before exiting? "
#define TXT_none			"nwsetup2:97" FS "None"

#endif
