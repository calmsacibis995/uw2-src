/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)libld:common/ldaclose.c	1.5"
#include    <stdio.h>
#include    "filehdr.h"
#include	"synsyms.h"
#include    "ldfcn.h"

int
ldaclose(ldptr)

LDFILE    *ldptr; 

{

    extern int		vldldptr( );
    extern int		freeldptr( );

    if (vldldptr(ldptr) == FAILURE) {
	return(FAILURE);
    }

    (void) fclose(IOPTR(ldptr));
    (void) freeldptr(ldptr);

    return(SUCCESS);
}

