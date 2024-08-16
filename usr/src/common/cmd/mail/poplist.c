/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/poplist.c	1.6.4.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)poplist.c	2.13 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	pophdrlist - pop all headers from a header list

    SYNOPSIS
	void pophdrlist(Hdrinfo *phdrinfo, Hdrs *hdr2rm)

    DESCRIPTION
	Poplist removes an entry and all subsequent entries,
	along with the continuation entries, from a message's
	header linked list and frees the header.
*/

void pophdrlist(phdrinfo, hdr2rm)
Hdrinfo *phdrinfo;
Hdrs *hdr2rm;
{
    while (hdr2rm != (Hdrs*)NULL)
        {
	Hdrs *nexthdr2rm = hdr2rm->next;

	if (hdr2rm->hdrtype == H_AFWDFROM)
	    phdrinfo->affcnt--;

        del_Hdrs(hdr2rm);
	hdr2rm = nexthdr2rm;
	}
}
