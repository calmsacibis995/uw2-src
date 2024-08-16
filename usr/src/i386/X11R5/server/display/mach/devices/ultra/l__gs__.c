/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)mach:mach/devices/ultra/l__gs__.c	1.2"


#include <sidep.h>

extern void lfb_bitblt__gs_change__(void);
extern void lfb_points__gs_change__(void);
extern void lfb_arc__gs_change__(void);

void
lfb__gs_change__(void)
{
	lfb_bitblt__gs_change__();
	lfb_points__gs_change__();
	lfb_arc__gs_change__();
}
