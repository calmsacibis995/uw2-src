/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Lineinfo_h
#define Lineinfo_h
#ident	"@(#)debugger:inc/common/Lineinfo.h	1.1"

#include "Itype.h"

struct LineEntry {
	Iaddr		addr;
	long		linenum;
};

struct Lineinfo {
	int		entrycount;
	LineEntry *	addrpart;
	LineEntry *	linepart;
};

#define BIG_LINE	1000000000

#endif

// end of Lineinfo.h

