/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/stamp.c	1.9.2.3"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)stamp.c	2.9 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	stamp - restore time stamp on a mail file

    SYNOPSIS
	void stamp(struct utimbuf *utimep)

    DESCRIPTION
	If the mailfile still exists (it may have been deleted),
	time-stamp it; so that our re-writing of mail back to the
	mailfile does not make shell think that NEW mail has arrived
	(by having the file times change).
*/

void stamp(utimep)
struct utimbuf *utimep;
{
	if ((access(mailfile, F_OK) == CSUCCESS) && (utimep->modtime != -1))
		if (utime(mailfile, utimep) != CSUCCESS)
			errmsg(E_FILE,":413:Cannot time-stamp mailfile\n");
}
