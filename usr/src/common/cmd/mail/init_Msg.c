/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Msg.c	1.4.5.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)init_Msg.c	1.10 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Msg - initialize a Msg structure
	new_Hdrinfo - allocate a Hdrinfo structure

    SYNOPSIS
	void init_Msg(Msg *pmsg)
	Hdrinfo *new_Hdrinfo()

    DESCRIPTION
	init_Msg() initializes a Msg structure as if it had just been created.
	new_Hdrinfo() allocates and initializes a Hdrinfo structure.
*/

void init_Msg(pmsg)
Msg *pmsg;
{
    static const char pn[] = "init_Msg";

    Dout(pn, 0, "Entered\n");
    pmsg->type = Msg_msg;
    pmsg->preciplist = new_Reciplist();
    pmsg->Rpath = s_new();
    pmsg->binflag = C_Text;
    pmsg->phdrinfo = new_Hdrinfo();
    pmsg->orig = s_new();
    pmsg->msgsize = 0;
    pmsg->ret_on_error = (flgT || flglb) ? 0 : 1;
    pmsg->SURRerrfile = 0;
    pmsg->SURRoutfile = 0;
    pmsg->SURRcmd = 0;
    pmsg->parent = 0;
    pmsg->surg_rc = SURG_RC_DEF;
    pmsg->delivopts = 0;
    pmsg->errmsg = 0;
    pmsg->localmessage = 0;
    topmsg = pmsg;
    pmsg->tmpfile = new_Tmpfile();
    init_Tmpfile(&pmsg->SURRinput);
}

Hdrinfo *new_Hdrinfo()
{
    static const char pn[] = "new_Hdrinfo";
    Hdrinfo *phdrinfo = New(Hdrinfo);
    if (!phdrinfo)
        {
	pfmt(stderr, MM_ERROR, ":10:Out of memory: %s\n", Strerror(errno));
	error = E_MEM;
	Dout(pn, 0, "Can't allocate memory\n");
	done(0);
	}

    init_Hdrinfo(phdrinfo);
    return phdrinfo;
}
