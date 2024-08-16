/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64__vtin__.c	1.2"

#include <sidep.h>

extern void m64_state__vt_switch_in__(void);
extern void m64_cmap__vt_switch_in__(void);
extern void m64_cursor__vt_switch_in__(void);

void
m64__vt_switch_in__(void)
{
	m64_state__vt_switch_in__();
	m64_cmap__vt_switch_in__();
	m64_cursor__vt_switch_in__();
}
