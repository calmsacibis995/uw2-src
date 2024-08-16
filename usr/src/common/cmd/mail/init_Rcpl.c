/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Rcpl.c	1.2.3.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)init_Rcpl.c	1.4 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Reciplist - initialize a recipient list
	new_Reciplist - allocate and initialize a recipient list

    SYNOPSIS
	void init_Reciplist (Reciplist *list)
	Reciplist *new_Reciplist ()

    DESCRIPTION
	Initialize a recipient list to have no recipients.
*/

void init_Reciplist (plist)
Reciplist	*plist;
{
	static const char pn[] = "init_Reciplist";
	Dout(pn, 0, "entered\n");
	plist->recip_list.next = 0;
	plist->recip_list.name = 0;
	plist->recip_list.cmdl = 0;
	plist->recip_list.cmdr = 0;
	plist->recip_list.parent = 0;
	plist->recip_list.SURRcmd = 0;
	plist->recip_list.SURRoutput = 0;
	plist->last_recip = &plist->recip_list;
}

Reciplist *new_Reciplist()
{
    register int i;
    Reciplist *ret = (Reciplist*) malloc(sizeof(Reciplist) * (surr_len + RECIPS_MAX));
    if (!ret)
        {
	pfmt(stderr, MM_ERROR, ":10:Out of memory: %s\n", Strerror(errno));
	error = E_MEM;
	Dout("new_Reciplist", 0, "Can't allocate memory\n");
	done(0);
	}

    for (i = surr_len + RECIPS_MAX; i-- > 0; )
	init_Reciplist(&ret[i]);
    return ret;
}
