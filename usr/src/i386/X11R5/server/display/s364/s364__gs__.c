/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#pragma ident	"@(#)s364:s364/s364__gs__.c	1.2"
#include <sidep.h>

extern void s364_fill__gs_change__(void);
extern void s364_line__gs_change__(void);
extern void s364_arc__gs_change__(void);
extern void s364_fillspans__gs_change__(void);

void
s364__gs_change__(void)
{
	s364_fill__gs_change__();
	s364_line__gs_change__();
	s364_arc__gs_change__();
	s364_fillspans__gs_change__();
}
