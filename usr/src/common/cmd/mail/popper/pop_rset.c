/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mail:common/cmd/mail/popper/pop_rset.c	1.1"
/*
 * Copyright (c) 1989 Regents of the University of California.
 * All rights reserved.  The Berkeley software License Agreement
 * specifies the terms and conditions for redistribution.
 */

#ifndef lint
static char copyright[] = "Copyright (c) 1990 Regents of the University of California.\nAll rights reserved.\n";
static char SccsId[] = "@(#)@(#)pop_rset.c	2.1  2.1 3/18/91";
#endif not lint

#include <stdio.h>
#include <sys/types.h>
#include "popper.h"

/* 
 *  rset:   Unflag all messages flagged for deletion in a POP maildrop
 */

int pop_rset (p)
POP     *   p;
{
    MsgInfoList     *   mp;         /*  Pointer to the message info list */
    register int        i;

    /*  Unmark all the messages */
    for (i = p->msg_count, mp = p->mlp; i > 0; i--, mp++)
        mp->del_flag = FALSE; 
    
    /*  Reset the messages-deleted and bytes-deleted counters */
    p->msgs_deleted = 0;
    p->bytes_deleted = 0;
    
    /*  Reset the last-message-access flag */
    p->last_msg = 0;

    return (pop_msg(p,POP_SUCCESS,"Maildrop has %u messages (%u octets)",
        p->msg_count,p->drop_size));
}
