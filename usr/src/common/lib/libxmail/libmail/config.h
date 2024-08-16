/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992 UNIX System Laboratories, Inc.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 AT&T	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF     	*/
/*	UNIX System Laboratories, Inc.                     	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/config.h	1.1"
#ident	"@(#)libmail:libmail/config.h	1.1"
/*
 * These are configurable parameters for system aliases
 */
extern const char libdir[];	/* /etc/mail */
extern const char sysalias[];	/* alias file list: /etc/mail/namefiles */
extern const char useralias[];	/* personal aliases: /lib/names */
extern const char maildir[];	/* directory for mail files */
extern const char mailsave[];	/* dir for save files */
extern const char mailfwrd[];	/* dir for forward files */
extern const char spoolsmtpq[];	/* dir for smtp files */