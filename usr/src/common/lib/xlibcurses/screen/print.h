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

#ident	"@(#)curses:common/lib/xlibcurses/screen/print.h	1.2.2.3"
#ident  "$Header: print.h 1.2 91/06/26 $"

/* externs from iexpand.c, cexpand.c */
extern void tpr();
extern int cpr();
extern char *cexpand(), *iexpand(), *cconvert(), *rmpadding();

/* externs from print.c */
enum printtypes
    {
    pr_none,
    pr_terminfo,		/* print terminfo listing */
    pr_cap,			/* print termcap listing */
    pr_longnames		/* print C variable name listing */
    };

extern void pr_onecolumn();
extern void pr_caprestrict();
extern void pr_width();
extern void pr_init();
extern void pr_heading();
extern void pr_bheading();
extern void pr_boolean();
extern void pr_bfooting();
extern void pr_nheading();
extern void pr_number();
extern void pr_nfooting();
extern void pr_sheading();
extern void pr_string();
extern void pr_sfooting();
