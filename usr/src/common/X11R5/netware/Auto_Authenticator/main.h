/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)autoauthent:main.h	1.10"
#ident  "$Header: /SRCS/esmp/usr/src/nw/X11R5/netware/Auto_Authenticator/main.h,v 1.12 1994/09/06 19:18:06 plc Exp $"

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIVEL Inc.                     			*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/
/*
#ifndef NOIDENT
#ident  "xauto:main.h   1.0"
#endif
*/

#ifndef MAIN_H
#define MAIN_H

#ifdef DEBUG
extern char debug_buf[];
#endif

/****************************************************
        argument positions for xauto
***********************************************************/
#define SERVER_NAME_ARG_POS 1
#define USER_NAME_ARG_POS   2

/****************************************************
        items and fields
***********************************************************/
typedef struct  {
        Widget          form, foot, login, pwd;
        char            server[50];
		char			*passwd;
}  Fields;

enum { FATAL_ERROR = 1, WARNING, OK };


#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define APPNAME          "Auto-Authenticator"
#define HELP_FILE        "Auto_Authenticator/xauto.hlp"
#define HELP_SECT        "10"

#define TXT_appTitle     "xauto2:2" FS "Auto-Authenticator"
#define TXT_appHelp      "xauto2:3" FS "Auto-Authenticator"
#define TXT_appHelpSect  "xauto2:4" FS "10"

#define TXT_ok            	"xauto2:5" FS "OK"
#define TXT_cancel         	"xauto2:6" FS "Cancel"
#define TXT_help        	"xauto2:7" FS "Help"

#define TXT_serverName	    "xauto2:8" FS "Server Name :"
#define TXT_login          	"xauto2:9" FS "Login :"
#define TXT_pwd          	"xauto2:10" FS "Password :"

#define TXT_errorPanel	    "xauto2:11" FS "Error"
#define TXT_warningPanel	"xauto2:12" FS "Warning"
#define TXT_display      	"xauto2:13" FS "Sorry. Could not open display."

#define TXT_invalidconn     "xauto2:14" FS "Invalid connection."
#define TXT_disableacc      "xauto2:15" FS "Disabled account."
#define TXT_nograce         "xauto2:16" FS "Password has expired. No grace."
#define TXT_pwdexpired      "xauto2:17" FS "Password has expired."
#define TXT_unexpected      "xauto2:18" FS "Unexpected Error."
#define TXT_badpwd          "xauto2:19" FS "Invalid password."
#define TXT_baduserID		"xauto2:20" FS "Invalid user ID."
#define TXT_maxLoginsErr	"xauto2:21" FS "Maximum server logins exceeded."
#define TXT_attachFailed	"xauto2:22" FS "Could not attach to server."

#define TXT_nwLoginMsg      "xauto2:23" FS "Auto_Authentication can not be accomplished.\nUse \"nwlogin\" to log into the NetWare server.\n"

#define MNEM_ok            	"xauto2:24" FS "O"
#define MNEM_cancel        	"xauto2:25" FS "C"
#define MNEM_help        	"xauto2:26" FS "H"
#endif /* MAIN_H */

