/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2mvr.c	1.2.2.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)send2mvr.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2mvrecip,send2mvrecip2 - move a recipient from one surrogate list to another

    SYNOPSIS
	void send2mvrecip(Msg *pmsg, int osurr_num, int nsurr_num)
	void send2mvrecip2(Msg *pfrommsg, Msg *ptomsg, int osurr_num, int nsurr_num)

    DESCRIPTION
	send2mvrecip and send2mvrecip2 move the top recipient from one surrogate list
	to the end of another surrogate list. send2mvrecip2 can move the recipient
	between Msg's.

	NOTE: It assumes that there is a recipient on the list to be moved.
*/

void send2mvrecip(pmsg, osurr_num, nsurr_num)
Msg *pmsg;
int osurr_num;
int nsurr_num;
{
    send2mvrecip2(pmsg, pmsg, osurr_num, nsurr_num);
}

void send2mvrecip2(pfrommsg, ptomsg, osurr_num, nsurr_num)
Msg *pfrommsg;
Msg *ptomsg;
int osurr_num;
int nsurr_num;
{
    static const char pn[] = "send2mvrecip";
    Reciplist *o = &pfrommsg->preciplist[osurr_num];
    Reciplist *n = &ptomsg->preciplist[nsurr_num];
    Recip *or = o->recip_list.next;
    Recip *sv = or->next;

    /* move recipient to end of new list */
    Dout(pn, 50, "name='%s', from=%d, to=%d\n", s_to_c(or->name), osurr_num, nsurr_num);
    or->next = (Recip *)NULL;
    n->last_recip->next = or;
    n->last_recip = or;

    /* patch up the old list */
    o->recip_list.next = sv;
    if (o->last_recip == or)
	o->last_recip = &o->recip_list;
}
