/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/popper/pop_xtnd.c	1.1"
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_xtnd.c	2.1  2.1 3/18/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include "popper.h"

/* 
 *  xtnd:   Handle extensions to the POP protocol suite
 */

extern  xtnd_table  *   pop_get_subcommand();

int pop_xtnd (p)
POP     *   p;
{
    xtnd_table  *   x;

    /*  Convert the XTND subcommand to lower case */
    pop_lower(p->pop_subcommand);

    /*  Search for the subcommand in the XTND command table */
    if ((x = pop_get_subcommand(p)) == NULL) return(POP_FAILURE);

    /*  Call the function associated with this subcommand */
    if (x->function) return((*x->function)(p));

    /*  Otherwise assume NOOP */
    return (pop_msg(p,POP_SUCCESS,NULL));
}
