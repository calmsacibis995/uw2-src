/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libld:common/ldtbread.c	1.7"
#include	<stdio.h>
#include	"filehdr.h"
#include	"syms.h"
#include	"synsyms.h"
#include	"ldfcn.h"

int
ldtbread(ldptr, symnum, symentry)

LDFILE	*ldptr;
long	symnum;
SYMENT	*symentry;

{

    extern int		vldldptr( );

    if (vldldptr(ldptr) == SUCCESS) {
	if ((symnum >= 0) && (symnum < (HEADER(ldptr)).f_nsyms)) {
	    if (FSEEK(ldptr,
		HEADER(ldptr).f_symptr + symnum * SYMESZ, BEGINNING)
		== OKFSEEK) {
		if (FREAD((char *)symentry,SYMESZ,1,ldptr) == 1) {
		    return(SUCCESS);
		}
	    }
	}
    }

    return(FAILURE);
}

