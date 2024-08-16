/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/smtp/src/s5sysname.c	1.4.2.3"
#ident "@(#)s5sysname.c	1.5 'attmail mail(1) command'"
/* get the system's name -- System V */

#include <libmail.h>

extern char *
sysname_read()
{
	return mailsystem(1);
}

extern char *
domainname_read()
{
	static string *fullname = 0;
	if (!fullname)
		fullname = s_xappend((string*)0, sysname_read(), maildomain(), (char*)0);
	return s_to_c(fullname);
}
