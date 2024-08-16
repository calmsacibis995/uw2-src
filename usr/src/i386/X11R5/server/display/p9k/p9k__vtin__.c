/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k__vtin__.c	1.1"

#include <sidep.h>

extern void p9000_colormap__vt_switch_in__(void);
extern void p9000_cursor__vt_switch_in__(void);

void
p9000__vt_switch_in__(void)
{
	p9000_colormap__vt_switch_in__();
	p9000_cursor__vt_switch_in__();
}
