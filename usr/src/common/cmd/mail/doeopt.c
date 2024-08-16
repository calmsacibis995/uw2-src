/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/doeopt.c	1.1.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)doeopt.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	doeopt - report on mail via an exit code

    SYNOPSIS
	void doeopt()

    DESCRIPTION
	Check to see if anything is in the mail file
	and exit accordingly.
*/

void doeopt()
{
    struct stat statb;
    (void) setmailfile();
    exit(!((stat(mailfile, &statb) == 0) && (statb.st_size != 0)));
    /* NOTREACHED */
}
