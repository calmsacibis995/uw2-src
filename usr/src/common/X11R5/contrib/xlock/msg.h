/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5xlock:msg.h	1.1"

#ifndef MSG_H
#define MSG_H

#ifndef FS
#define FS  "\001"
#define FS_CHR  '\001'
#endif


#define TXT_username       "xlock:1" FS "Name: " 
#define TXT_password       "xlock:2" FS "Password: " 
#define TXT_info           "xlock:3" FS "Enter password to unlock; select icon to lock."
#define TXT_validate       "xlock:4" FS "Validating login..."
#define TXT_invalid        "xlock:5" FS "Invalid login."

#endif
