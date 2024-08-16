/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/popper/pop_user.c	1.1"
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_user.c	2.1  2.1 3/18/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include "popper.h"
#if defined NOVELL || defined SVR4
#include <string.h>
#else
#include <strings.h>
#endif

/* 
 *  user:   Prompt for the user name at the start of a POP session
 */

int pop_user (p)
POP     *   p;
{
    /*  Save the user name */
    (void)strcpy(p->user, p->pop_parm[1]);

    /*  Tell the user that the password is required */
    return (pop_msg(p,POP_SUCCESS,"Password required for %s.",p->user));
}
