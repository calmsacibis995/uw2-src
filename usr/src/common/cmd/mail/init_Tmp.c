/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Tmp.c	1.3.3.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)init_Tmp.c	1.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Tmpfile - initialize a Tmpfile structure
	new_Tmpfile - create a new, initialized Tmpfile structure

    SYNOPSIS
	void init_Tmpfile(Tmpfile *ptmpfile)
	Tmpfile *new_Tmpfile()

    DESCRIPTION
	init_Tmpfile() initializes a Tmpfile structure as if it had just been created.
	new_Tmpfile() creates a Tmpfile structure, and calls init_Tmpfile() on it.

*/

void init_Tmpfile(ptmpfile)
Tmpfile *ptmpfile;
{
    static const char pn[] = "init_Tmpfile";
    Dout(pn, 0, "Entered\n");
    ptmpfile->lettmp = 0;
    ptmpfile->tmpf = 0;
    ptmpfile->next = toptmpfile;
    ptmpfile->pid = 0;
    toptmpfile = ptmpfile;
}

Tmpfile *new_Tmpfile()
{
    static const char pn[] = "new_Tmpfile";
    Tmpfile *ret = New(Tmpfile);
    if (!ret)
        {
	pfmt(stderr, MM_ERROR, ":10:Out of memory: %s\n", Strerror(errno));
	error = E_MEM;
	Dout(pn, 0, "Can't allocate memory\n");
	done(0);
	}

    init_Tmpfile(ret);
    return ret;
}

