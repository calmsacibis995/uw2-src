/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Protoline.h	1.1"
#ifndef Protoline_h
#define Protoline_h

#include	"Vector.h"
#include	"Iaddr.h"

struct Lineinfo;

class Protoline {
	Vector		vector;
	int		count;
public:
			Protoline() {	count = 0;	}
			~Protoline()	{}
	Protoline &	add_line( Iaddr, long line_num );
	Lineinfo *	put_line( Iaddr );
};

#endif /* Protoline_h */
