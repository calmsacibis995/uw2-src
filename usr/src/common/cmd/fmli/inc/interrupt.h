/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1984, 1985, 1986, 1987, 1988, 1989, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/


/* defines and data structures for interrupt  feature */
/*
 * Copyright  (c) 1988 AT&T
 *	All Rights Reserved
 */
#ident	"@(#)fmli:inc/interrupt.h	1.1.4.3"


#define DEF_ONINTR "`message Operation interrupted!`NOP"

#ifndef TYPE_BOOL
/* curses.h also  does a typedef bool */
#ifndef CURSES_H
#define TYPE_BOOL
typedef char bool;
#endif
#endif

extern struct {
    bool  interrupt;
    char *oninterrupt;
    bool  skip_eval;
} Cur_intr;
