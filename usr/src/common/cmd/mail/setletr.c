/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/setletr.c	1.2.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)setletr.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	setletr - set status on letter

    SYNOPSIS
	void setletr(Letinfo *pletinfo, int letter, int status);

    DESCRIPTION
	Set letter to passed status, and adjust changed as necessary.
*/

void setletr(pletinfo, letter, status)
Letinfo	*pletinfo;
int	letter;
int	status;
{
	if (status == ' ') {
		if (pletinfo->let[letter].change != ' ') 
			if (pletinfo->changed)
				pletinfo->changed--;
	}
	else if (pletinfo->let[letter].change == ' ')
		pletinfo->changed++;
	pletinfo->let[letter].change = (char)status;
}
