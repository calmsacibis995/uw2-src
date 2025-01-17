/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/validmsg.c	1.3.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)validmsg.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	validmsg - Check message number for validity

    SYNOPSIS
	int validmsg(Letinfo *pletinfo, int);

    DESCRIPTION
	Check a message number. If valid, return 1.
	Otherwise print a message and return 0.
*/

int validmsg(pletinfo, i)
Letinfo	*pletinfo;
int	i;
{
	if ((i < 0) || (i > pletinfo->nlet)) {
		pfmt(stdout, MM_ERROR, ":60:No such message\n");
		return(0);
	}
	return(1);
}
