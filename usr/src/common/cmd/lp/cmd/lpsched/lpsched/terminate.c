/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


#ident	"@(#)lp:cmd/lpsched/lpsched/terminate.c	1.2.8.2"
#ident  "$Header: terminate.c 1.2 91/06/27 $"

#include "lpsched.h"

/*
 * Procedure:     terminate
 *
 * Restrictions:
 *               kill(2): None
 * Notes - STOP A CHILD PROCESS
 */

void
#ifdef	__STDC__
terminate (
	register EXEC *		ep
)
#else
terminate (ep)
	register EXEC		*ep;
#endif
{
	DEFINE_FNNAME (terminate)

	if (ep->pid > 0) {

#ifdef	DEBUG
		if (debug & DB_EXEC)
			execlog (
				"KILL: pid %d%s%s\n",
				ep->pid,
				((ep->flags & EXF_KILLED)?
					  ", second time"
					: ""
				),
				(kill(ep->pid, 0) == -1?
					  ", but child is GONE!"
					: ""
				)
			);
#endif

		if (ep->flags & EXF_KILLED)
			return;
		ep->flags |= EXF_KILLED;

		/*
		 * Theoretically, the following "if-then" is not needed,
		 * but there's some bug in the code that occasionally
		 * prevents us from hearing from a finished child.
		 * (Kill -9 on the child would do that, of course, but
		 * the problem has occurred in other cases.)
		 */
		if (kill(-ep->pid, SIGTERM) == -1 && errno == ESRCH) {
			ep->pid = -99;
			ep->status = SIGTERM;
			ep->errno = 0;
			DoneChildren++;
		}
	}
	return;
}
