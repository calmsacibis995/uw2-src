/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1993 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)r5config:common/X11R5/config/ccimake.c	1.2"

/*
 * $XConsortium: ccimake.c,v 1.12 89/10/16 12:09:23 jim Exp $
 * 
 * Warning:  This file must be kept as simple as posible so that it can 
 * compile without any special flags on all systems.  Do not touch it unless
 * you *really* know what you're doing.  Make changes in imakemdep.h, not here.
 */

#define CCIMAKE			/* only get imake_ccflags definitions */
#include "imakemdep.h"		/* things to set when porting imake */

#ifndef imake_ccflags
#define imake_ccflags "-O"
#endif

main()
{
	write(1, imake_ccflags, sizeof(imake_ccflags) - 1);
	exit(0);
}

