/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)mach64:mach64/m64__gs__.c	1.2"


#include <sidep.h>

extern void m64_fill__gs_change__(void);
extern void m64_font__gs_change__(void);
extern void m64_line__gs_change__(void);
extern void m64_arc__gs_change__(void);
extern void m64_fillspans__gs_change__(void);

void
m64__gs_change__(void)
{
	m64_fill__gs_change__();
	m64_font__gs_change__();
	m64_line__gs_change__();
	m64_arc__gs_change__();
	m64_fillspans__gs_change__();
}
