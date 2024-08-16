/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/new_Msg.c	1.1.1.1"
#ident "@(#)new_Msg.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	new_Msg - allocate and initialize a Msg

    SYNOPSIS
	Msg *new_Msg()

    DESCRIPTION
	allocate and initialize a Msg.
*/

static char MAnomem[] = ":407:malloc failed in %s(): %s\n";

Msg *new_Msg()
{
    static const char pn[] = "new_Msg";
    Msg *pmsg;

    Dout(pn, 0, "entered\n");

    if ((pmsg = (Msg*)malloc(sizeof(Msg))) == (Msg*)NULL)
	{
	errmsg(E_MEM, MAnomem, pn, Strerror(errno));
	done(1);
	}

    init_Msg(pmsg);

    return pmsg;
}
