/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/popper/pop_get_subcommand.c	1.1"
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_get_subcommand.c	2.1  2.1 3/18/91";
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
 *  get_subcommand: Extract a POP XTND subcommand from a client input line
 */

static xtnd_table subcommands[] = {
        "xmit",     0,  0,  pop_xmit,
        NULL
};

xtnd_table *pop_get_subcommand(p)
POP     *   p;
{
    xtnd_table      *   s;

    /*  Search for the POP command in the command/state table */
    for (s = subcommands; s->subcommand; s++) {

        if (strcmp(s->subcommand,p->pop_subcommand) == 0) {

            /*  Were too few parameters passed to the subcommand? */
            if ((p->parm_count-1) < s->min_parms)
                return((xtnd_table *)pop_msg(p,POP_FAILURE,
                    "Too few arguments for the %s %s command.",
                        p->pop_command,p->pop_subcommand));

            /*  Were too many parameters passed to the subcommand? */
            if ((p->parm_count-1) > s->max_parms)
                return((xtnd_table *)pop_msg(p,POP_FAILURE,
                    "Too many arguments for the %s %s command.",
                        p->pop_command,p->pop_subcommand));

            /*  Return a pointer to the entry for this subcommand 
                in the XTND command table */
            return (s);
        }
    }
    /*  The client subcommand was not located in the XTND command table */
    return((xtnd_table *)pop_msg(p,POP_FAILURE,
        "Unknown command: \"%s %s\".",p->pop_command,p->pop_subcommand));
}
