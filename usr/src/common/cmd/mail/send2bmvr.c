/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2bmvr.c	1.2.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)send2bmvr.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2bmvrecip - move a recipient from one surrogate list to beginning of another

    SYNOPSIS
	void send2bmvrecip(Msg *pmsg, int osurr_num, int nsurr_num)

    DESCRIPTION
	send2bmvrecip moves the top recipient from one surrogate list
	to the top of another surrogate list.

	NOTE: It assumes that there is a recipient on the list to be moved.
*/

void send2bmvrecip(pmsg, osurr_num, nsurr_num)
Msg *pmsg;
int osurr_num;
int nsurr_num;
{
    static const char pn[] = "send2bmvrecip";
    Reciplist *o = &pmsg->preciplist[osurr_num];
    Reciplist *n = &pmsg->preciplist[nsurr_num];
    Recip *or = o->recip_list.next;
    Recip *sv = or->next;

    /* move recipient to beginning of new list */
    Dout(pn, 50, "name='%s', from=%d, to=%d\n", s_to_c(or->name), osurr_num, nsurr_num);
    or->next = n->recip_list.next;
    n->recip_list.next = or;
    if (n->last_recip == &n->recip_list)
	n->last_recip = or;

    /* patch up the old list */
    o->recip_list.next = sv;
    if (o->last_recip == or)
	o->last_recip = &o->recip_list;
}
