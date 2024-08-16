/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/setmail.c	1.2.3.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)setmail.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	setmailfile - set global mailfile to proper value

    SYNOPSIS
	int setmailfile()

    DESCRIPTION
	If we are not using an alternate mailfile, then get
	the $MAIL value and build the filename for the mailfile.
	If $MAIL is set, but is NOT the 'standard' place, then
	use it but set flgf to circumvent :saved processing.

    RETURNS
	1 if using non-standard mailbox
	0 otherwise
*/

int setmailfile()
{
    static const char pn[] = "setmailfile";
    int ret = 0;
    char *p;

    Dout(pn, 0, "Entered\n");
    if (flgf)
	mailfile = flgf;

    else
        {
	if ((p = malloc((unsigned)(strlen(maildir) + strlen(my_name) + 1))) == NULL)
	    {
	    errmsg(E_MEM,"");
	    return 0;
	    }

	cat(p, maildir, my_name);

	mailfile = getenv("MAIL");
	if (!mailfile || !*mailfile)
	    {
	    /* $MAIL not set, use standard path to mailfile */
	    mailfile = p;
	    }

	else
	    {
	    if (strcmp(mailfile, p) != 0)
	        {
		flgf = mailfile;
		ret = 1;
		Dout(pn, 0, "$MAIL ('%s') != standard path\n", mailfile);
		Dout("", 0, "\tSetting flgf to 1.\n");
		}
	    free (p);
	    }
	}

    return ret;
}
