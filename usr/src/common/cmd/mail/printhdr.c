/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/printhdr.c	1.5.3.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)printhdr.c	2.7 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	printhdr - Print a header from the structure

    SYNOPSIS
	int	printhdr (int type, int hdrtype, Hdrs *hptr, FILE *fp, int Pflg);

    DESCRIPTION
	printhdr() prints the headers of the given type from the
	hdrlines structure. It returns 0 on success, -1 on failure.
	On failure, sav_errno will hold the value of errno.
*/

int printhdr (type, hdrtype, hptr, fp, Pflg)
int	hdrtype;
Hdrs	*hptr;
FILE	*fp;
int	Pflg;
{
	Hdrs	 	*contptr;

	if (sel_disp(type, hdrtype, header[hdrtype].tag, Pflg) < 0) {
		return (0);
	}

	(void) fputs(header[hdrtype].tag, fp);
	(void) putc(' ', fp);
	(void) fwrite(s_to_c(hptr->value), 1, s_curlen(hptr->value), fp);
	(void) putc('\n', fp);
	
	if (ferror(fp) || (fflush(fp) == EOF)) {
		sav_errno = errno;
		return(-1);
	}

	return (0);
}
