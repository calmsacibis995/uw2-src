/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/del_Hdrs.c	1.2.3.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)del_Hdrs.c	1.4 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	del_Hdrs - delete a header

    SYNOPSIS
	void del_Hdrs(Hdrs*)

    DESCRIPTION
	Deallocate a header.
*/

void del_Hdrs(old)
Hdrs *old;
{
    s_free(old->value);
    if (old->name && old->hdrtype == H_NAMEVALUE)
	free((char*)old->name);
    free((char*)old);
}
