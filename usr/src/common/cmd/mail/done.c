/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/done.c	1.7.2.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)done.c	2.11 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	done - clean up lock files and exit

    SYNOPSIS
	int done(int needtmp)

    DESCRIPTION
	Do whatever cleanup processing is necessary
	to exit, such as cleaning up lock files
	and tmp files.

    NAME
	cleantempfiles - clean out all temp files in our list

    SYNOPSIS
	void cleantempfiles(int needtmp)

    DESCRIPTION
	Unless needtmp is nonzero, unlink all temp files kept
	in the list of temp files.
*/

void done(needtmp)
int	needtmp;
{
	static const char pn[] = "done";
	unlock();

	if (!maxerr) {
		maxerr = error;
		Dout(pn, 0, "maxerr set to %d\n", maxerr);
		if (flgx > 0)
			unlink(dbgfname);
	}

	if (maxerr)
		mkdead(topmsg);

	cleantempfiles(needtmp);

	exit(maxerr);
	/* NOTREACHED */
}

/* clean out all temp files in our list */
void cleantempfiles(needtmp)
int needtmp;
{
    Tmpfile *p;
    pid_t curpid = getpid();
    if (needtmp)
	return;

    for (p = toptmpfile; p; p = p->next)
        if (p->lettmp && (p->pid == curpid))
	    {
	    fprintf(stderr, "%d: cleantempfiles() unlinking %s\n", getpid(), p->lettmp);
            unlink(p->lettmp);
	    }
}
