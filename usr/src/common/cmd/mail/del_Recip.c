/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/del_Recip.c	1.2.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)del_Recip.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	del_Recip - delete a recipient

    SYNOPSIS
	void del_Reciplist (Recip *old)

    DESCRIPTION
	Free the space used by a recipient.
*/

void del_Recip(old)
Recip	*old;
{
    static const char pn[] = "del_Recip";
    Dout(pn, 0, "entered\n");
    s_free(old->name);
    s_free(old->cmdl);
    s_free(old->cmdr);
    s_free(old->SURRcmd);
    s_free(old->SURRoutput);
    free((char*)old);
}
