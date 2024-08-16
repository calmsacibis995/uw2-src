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

#ident	"@(#)curses:common/lib/xlibcurses/screen/resetty.c	1.5.2.3"
#ident  "$Header: resetty.c 1.2 91/06/26 $"

#include	"curses_inc.h"

resetty()
{
    if ((_BR(SP->save_tty_buf)) != 0)
    {
	PROGTTY = SP->save_tty_buf;
#ifdef	DEBUG
	if (outf)
#ifdef	SYSV
	    fprintf(outf, "resetty(), file %x, SP %x, flags %x, %x, %x, %x\n",
		cur_term->Filedes, SP, PROGTTY.c_iflag, PROGTTY.c_oflag,
		PROGTTY.c_cflag, PROGTTY.c_lflag);
#else	/* SYSV */
	    fprintf(outf, "resetty(), file %x, SP %x, flags %x\n",
		cur_term->Filedes, SP, PROGTTY.sg_flags);
#endif	/* SYSV */
#endif	/* DEBUG */
	reset_prog_mode();
    }
    return (OK);
}
