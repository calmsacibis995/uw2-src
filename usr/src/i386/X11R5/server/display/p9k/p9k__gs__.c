/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)p9k:p9k/p9k__gs__.c	1.2"

#include <sidep.h>

extern void p9000_arc__gs_change__(void);
extern void p9000_bitblt__gs_change__(void);
extern void p9000_line__gs_change__(void);
extern void p9000_polypoint__gs_change__(void);
extern void p9000_screen_state__gs_change__(void);
extern void p9000_solid__gs_change__(void);
extern void p9000_tile__gs_change__(void);
extern void p9000_stipple__gs_change__(void);

void
p9000__gs_change__(void)
{
	p9000_arc__gs_change__();
	p9000_bitblt__gs_change__();
	p9000_line__gs_change__();
	p9000_polypoint__gs_change__();
	p9000_screen_state__gs_change__();
	p9000_solid__gs_change__();
	p9000_tile__gs_change__();
	p9000_stipple__gs_change__();
}
