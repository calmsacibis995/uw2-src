/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libmail:libmail/mgetcharset.c	1.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)mgetcharset.c	1.1 'attmail mail(1) command'"
#include "libmail.h"
/*
    NAME
	mail_get_charset - get the character set associated with the current locale

    SYNOPSIS
	const char *mail_get_charset()

    DESCRIPTION
	Return the character set associated with the current locale.
*/

const char *mail_get_charset()
{
	char *p = getenv("MM_CHARSET");
	if (p && *p)
		return p;

	p = getenv("LANG");
	if (!p)
		p = getenv("LOCALE");
	if (p) {
		static char buf[MAXPATHLEN];
		FILE *fp;
		(void) sprintf(buf, "/etc/mail/charset/%s", p);
		if (fp = fopen(buf, "r")) {
			if (fgets(buf, sizeof(buf), fp)) {
				for (p = buf; *p && *p != '\n'; p++)
					;
				*p = '\0';

				(void) fclose(fp);
				return buf;
			}
			(void) fclose(fp);
		}
	}

	return "us-ascii";
}
