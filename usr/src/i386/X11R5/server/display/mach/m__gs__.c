/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/m__gs__.c	1.1"


#include <sidep.h>

extern void mach_bitblt__gs_change__(void);
extern void mach_fill__gs_change__(void);
extern void mach_line__gs_change__(void);
extern void mach_polypoint__gs_change__(void);
extern void mach_fillspans__gs_change__(void);
extern void mach_state__gs_change__(void);

void
mach__gs_change__(void)
{
	mach_bitblt__gs_change__();
	mach_fill__gs_change__();
	mach_line__gs_change__();
	mach_polypoint__gs_change__();
	mach_fillspans__gs_change__();
	mach_state__gs_change__();
}
