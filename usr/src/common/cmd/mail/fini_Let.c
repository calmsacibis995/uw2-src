/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/fini_Let.c	1.1.4.2"
/*	Copyright (c) 1990, 1991, 1992, 1993 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident "@(#)fini_Let.c	1.3 'attmail mail(1) command'"
#include "mail.h"
/*
    NAME
	fini_Letinfo - delete the resources used by a message

    SYNOPSIS
	void fini_Letinfo (Letinfo *pletinfo)

    DESCRIPTION
	Free the space used by a letter.
*/

void fini_Letinfo(pletinfo)
Letinfo *pletinfo;
{
    register int i, j;
    clr_hdrinfo(pletinfo->phdrinfo);
    free((char*)pletinfo->phdrinfo);
    fini_Tmpfile(&pletinfo->tmpfile);
    for (i = 0, j = pletinfo->nlet; i < j; i++)
	{
	s_free(pletinfo->let[i].content_type);
	if (pletinfo->let[i].encoding_type)
	    free(pletinfo->let[i].encoding_type);
	}
}
