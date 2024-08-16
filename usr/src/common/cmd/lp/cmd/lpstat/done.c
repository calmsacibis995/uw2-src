/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpstat/done.c	1.4.6.3"
#ident	"$Header: $"

#include "lpstat.h"

/**
 ** done() - CLEAN UP AND EXIT
 **/

void
#if	defined(__STDC__)
done (
	int			rc
)
#else
done (rc)
	int			rc;
#endif
{
	(void)mclose ();

	if (!rc && exit_rc)
		exit (exit_rc);
	else
		exit (rc);
}
