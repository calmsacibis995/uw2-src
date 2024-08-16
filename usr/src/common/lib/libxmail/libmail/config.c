/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/config.c	1.1"
#ident	"@(#)libmail:libmail/config.c	1.1"
#include "libmail.h"
/*
 * These are configurable parameters for system aliases
 */
const char libdir[] = "/etc/mail";
const char sysalias[] = "/etc/mail/namefiles";
const char useralias[] = "/lib/names";
const char maildir[] = MAILDIR;		/* directory for mail files */
const char mailsave[] = SAVEDIR;	/* dir for save files */
const char mailfwrd[] = FWRDDIR;	/* dir for forward files */
const char spoolsmtpq[] = SPOOLSMTPQ;	/* dir for smtp files */
