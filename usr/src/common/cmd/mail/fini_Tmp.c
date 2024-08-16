/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/fini_Tmp.c	1.3.4.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)fini_Tmp.c	1.6 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	fini_Tmpfile - delete the resources used by a temp file
	del_Tmpfile - delete temp file information

    SYNOPSIS
	void fini_Tmpfile(Tmpfile *ptmpfile)
	void del_Tmpfile(Tmpfile *ptmpfile)

    DESCRIPTION
	fini_Tmpfile() delete the resources used by a temp file.
	del_Tmpfile() calls fini_Tmpfile() and then frees the space.
*/

void fini_Tmpfile(ptmpfile)
Tmpfile *ptmpfile;
{
    Tmpfile *p;

    /* Remove all traces of the tmpfile */
    if (ptmpfile->tmpf)
	{
	fclose(ptmpfile->tmpf);
	ptmpfile->tmpf = 0;
	}

    if (ptmpfile->lettmp && ptmpfile->pid == getpid())
	{
	unlink(ptmpfile->lettmp);
	free(ptmpfile->lettmp);
	ptmpfile->lettmp = 0;
	}

    /* Remove the link for this tmpfile. */
    if (toptmpfile == ptmpfile)
	toptmpfile = ptmpfile->next;

    else
	for (p = toptmpfile; p; p = p->next)
	    if (p->next == ptmpfile)
		{
		p->next = ptmpfile->next;
		break;
		}
}

void del_Tmpfile(ptmpfile)
Tmpfile *ptmpfile;
{
    fini_Tmpfile(ptmpfile);
    free((char*)ptmpfile);
}
