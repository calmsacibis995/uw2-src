/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/init_Let.c	1.2.3.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)init_Let.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	init_Letinfo - initialize a Letinfo structure

    SYNOPSIS
	void init_Letinfo(Letinfo *pmsg)

    DESCRIPTION
	init_Letinfo initializes a Letinfo structure as if it had just been created
*/

void init_Letinfo(pletinfo)
register Letinfo *pletinfo;
{
    static const char pn[] = "init_Letinfo";
    Dout(pn, 0, "Entered\n");
    pletinfo->phdrinfo = new_Hdrinfo();
    init_Hdrinfo(pletinfo->phdrinfo);
    pletinfo->let[0].adr = 0;
    pletinfo->let[1].adr = 0;
    pletinfo->changed = 0;
    pletinfo->nlet = 0;
    pletinfo->onlet = 0;
    init_Tmpfile(&pletinfo->tmpfile);
}
