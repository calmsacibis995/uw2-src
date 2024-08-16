/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libld:common/ldohseek.c	1.5"
#include	<stdio.h>
#include	"filehdr.h"
#include	"synsyms.h"
#include	"ldfcn.h"

int
ldohseek(ldptr)

LDFILE		*ldptr;

{

    extern int		vldldptr( );

    if (vldldptr(ldptr) == SUCCESS) {
	if (HEADER(ldptr).f_opthdr != 0) {
	    if (FSEEK(ldptr, (long) FILHSZ, BEGINNING) == OKFSEEK) {
		return(SUCCESS);
	    }
	}
    }

    return(FAILURE);
}

