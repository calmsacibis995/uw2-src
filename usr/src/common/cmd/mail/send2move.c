/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/send2move.c	1.1.2.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)send2move.c	1.2 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	send2move, send2move2 - move list of recipients from one surrogate entry to another

    SYNOPSIS
	void send2move(Msg *pmsg, int osurr_num, int nsurr_num)
	void send2move2(Msg *pfrommsg, Msg *ptomsg, int osurr_num, int nsurr_num)

    DESCRIPTION
	send2move moves an entire list of recipients from one surrogate list
	to another surrogate list. send2move2 can move them between Msg's.
*/

void send2move(pmsg, osurr_num, nsurr_num)
Msg *pmsg;
int osurr_num;
int nsurr_num;
{
    send2move2(pmsg, pmsg, osurr_num, nsurr_num);
}

void send2move2(pfrommsg, ptomsg, osurr_num, nsurr_num)
Msg *pfrommsg;
Msg *ptomsg;
int osurr_num;
int nsurr_num;
{
    Recip *l;

    for (l = recips_head(pfrommsg, osurr_num); l->next != (Recip*) NULL; )
	{
	/* move recip to ptomsg->preciplist[nsurr_num] */
	send2mvrecip2(pfrommsg, ptomsg, osurr_num, nsurr_num);
	}
}
