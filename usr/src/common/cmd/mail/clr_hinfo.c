/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/clr_hinfo.c	1.7.2.4"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)clr_hinfo.c	2.10 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	clr_hdrinfo - clean out mail header information

    SYNOPSIS
	void clr_hdrinfo(Hdrinfo *phdrinfo)

    DESCRIPTION
	Clr_hinfo() cleans out hdrlines[] and other associated data
	in preparation for the next message.
*/

void clr_hdrinfo(phdrinfo)
Hdrinfo *phdrinfo;
{
    pophdrlist(phdrinfo, phdrinfo->hdrs[H_FROM]);
    phdrinfo->hdrs[H_FROM] = 0;
    pophdrlist(phdrinfo, phdrinfo->hdrs[H_RFROM]);
    phdrinfo->hdrs[H_RFROM] = 0;
    pophdrlist(phdrinfo, phdrinfo->hdrs[H_RETURN_PATH]);
    phdrinfo->hdrs[H_RETURN_PATH] = 0;
    pophdrlist(phdrinfo, phdrinfo->hdrhead);
    phdrinfo->hdrhead = phdrinfo->hdrtail = 0;
    init_Hdrinfo(phdrinfo);
}
