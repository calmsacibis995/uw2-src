/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpadmin/done.c	1.5.4.3"
#ident	"$Header: $"

extern	void	exit();

#include "lp.h"
#include "msgs.h"

#include "lpadmin.h"

/**
 ** done() - CLEAN UP AND EXIT
 **/

void			done (rc)
	int			rc;
{
	(void)mclose ();
	exit (rc);
}
