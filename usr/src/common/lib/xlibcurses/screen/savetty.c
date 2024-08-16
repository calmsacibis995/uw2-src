/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/*
 *	Copyright (c) 1982, 1986, 1988
 *	The Regents of the University of California
 *	All Rights Reserved.
 *	Portions of this document are derived from
 *	software developed by the University of
 *	California, Berkeley, and its contributors.
 */

#ident	"@(#)curses:common/lib/xlibcurses/screen/savetty.c	1.6.2.3"
#ident  "$Header: savetty.c 1.2 91/06/26 $"
/*
 * Routines to deal with setting and resetting modes in the tty driver.
 * See also setupterm.c in the termlib part.
 */
#include "curses_inc.h"

savetty()
{
    SP->save_tty_buf = PROGTTY;
#ifdef DEBUG
# ifdef SYSV
    if (outf)
	fprintf(outf,"savetty(), file %x, SP %x, flags %x,%x,%x,%x\n",
	    cur_term->Filedes, SP, PROGTTY.c_iflag, PROGTTY.c_oflag,
	    PROGTTY.c_cflag, PROGTTY.c_lflag);
# else
    if (outf)
	fprintf(outf, "savetty(), file %x, SP %x, flags %x\n",
	    cur_term->Filedes, SP, PROGTTY.sg_flags);
# endif
#endif
    return (OK);
}
