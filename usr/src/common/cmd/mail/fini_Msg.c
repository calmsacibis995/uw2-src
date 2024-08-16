/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/fini_Msg.c	1.3.4.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)fini_Msg.c	1.7 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	fini_Msg - delete the resources used by a message
	del_Hdrinfo - delete header information

    SYNOPSIS
	void fini_Msg (Msg *pmsg)
	void del_Hdrinfo(Hdrinfo *phdrinfo)

    DESCRIPTION
	fini_Msg() frees the space used by a message.
	del_Hdrinfo() clears and frees the space used by header information.
*/

void fini_Msg(pmsg)
Msg *pmsg;
{
    register int i;
    if (pmsg->errmsg)
	del_Msg(pmsg->errmsg);
    topmsg = pmsg->parent;
    for (i = surr_len + RECIPS_MAX; i-- > 0; )
	fini_Reciplist(&pmsg->preciplist[i]);
    free((char*)pmsg->preciplist);
    del_Hdrinfo(pmsg->phdrinfo);
    s_free(pmsg->Rpath);
    s_free(pmsg->orig);
    del_Tmpfile(pmsg->tmpfile);
    fini_Tmpfile(&pmsg->SURRinput);
    if (pmsg->SURRerrfile)
	fclose(pmsg->SURRerrfile);
    if (pmsg->SURRoutfile)
	fclose(pmsg->SURRoutfile);
    s_free(pmsg->SURRcmd);
}

void del_Hdrinfo(phdrinfo)
Hdrinfo *phdrinfo;
{
    clr_hdrinfo(phdrinfo);
    free((char*)phdrinfo);
}
