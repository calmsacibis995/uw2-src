/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libclass:msgs.h	1.2"
///////////////////////////////////////////////////////////////
// msgs.h: 
///////////////////////////////////////////////////////////////
#ifndef msgs_h
#define msgs_h

#ifndef FS
#define FS	"\001"
#define FS_CHR	'\001'
#endif

#define TXT_ok			"libclasses:1" FS "Ok"
#define TXT_cancel		"libclasses:2" FS "Cancel"
#define TXT_apply		"libclasses:3" FS "Apply"
#define TXT_help		"libclasses:4" FS "Help"
#define MNEM_ok			"libclasses:5" FS "O"
#define MNEM_cancel		"libclasses:6" FS "C"
#define MNEM_apply		"libclasses:7" FS "A"
#define MNEM_help		"libclasses:8" FS "H"

#define TXT_login		"libclasses:9" FS "Login id :"
#define TXT_pwd			"libclasses:10" FS "Password :"
#define TXT_loginto		"libclasses:11" FS "Login to NetWare Server : "

#define TXT_yes			"libclasses:12" FS "Yes"
#define TXT_no			"libclasses:13" FS "No"
#define MNEM_yes		"libclasses:14" FS "Y"
#define MNEM_no			"libclasses:15" FS "N"

#endif
