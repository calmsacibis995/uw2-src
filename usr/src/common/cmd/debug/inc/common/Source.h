/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Source_h
#define Source_h
#ident	"@(#)debugger:inc/common/Source.h	1.3"

#include	"Iaddr.h"

struct	Lineinfo;

class Source {
	Lineinfo *	lineinfo;
	Iaddr		ss_base;
	friend class	Symbol;
public:
			Source();
			Source(const Source& );
	int		isnull() { return (ss_base == 0 && lineinfo == 0); }
	void		null() { ss_base = 0; lineinfo = 0; }
	void		pc_to_stmt( Iaddr pc, long& line, int slide = -1,
				Iaddr *stmt_start = 0);
	void		stmt_to_pc( long line, Iaddr& pc, int slide  = 0 );
	Source &	operator=( const Source & );
};

#endif

// end of Source.h

