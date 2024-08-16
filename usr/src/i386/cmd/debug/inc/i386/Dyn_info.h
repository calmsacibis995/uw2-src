/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef _DYN_INFO_H
#define _DYN_INFO_H

#ident	"@(#)debugger:inc/i386/Dyn_info.h	1.1"

#include "Iaddr.h"

// machine specific, per-object dynamic information;
// maintained by the Seglist and Symnode classes.

struct Dyn_info {
	Iaddr	pltaddr;	// address of procedure linkage table
	long	pltsize;	// size of procedure linkage table
	Iaddr	gotaddr;	// address of global offset table
};

#endif
