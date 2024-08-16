/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/setsig.c	1.5.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)setsig.c	2.5 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	setsig - set a signal

    SYNOPSIS
	void (*setsig(int i, void(*f)()))();

    DESCRIPTION
	Check a signal and if it isn't being ignored,
	set it to be caught by function f.

		i	-> signal number
		f	-> signal routine

    RETURNS
		rc	-> former signal
 */

void (*setsig(i, f))()
int      i;
void      (*f)();
{
	register void (*rc)();

	if ((rc = signal(i, SIG_IGN)) != (void (*)()) SIG_IGN)
		signal(i, f);
	return(rc);
}
