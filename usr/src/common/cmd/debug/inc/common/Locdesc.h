/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Locdesc_h
#define Locdesc_h
#ident	"@(#)debugger:inc/common/Locdesc.h	1.3"

#include	"Vector.h"
#include	"Wordstack.h"
#include	"Place.h"

typedef void *	Addrexp;	// just a pointer to LocOp

class ProcObj;
class Frame;

class Locdesc {
	Vector		vector;
	Wordstack	stack;
	void		calculate_expr( Place &, ProcObj *, Frame * );
public:
			Locdesc()		{}
			Locdesc(Locdesc &l)	{ *this = l; }
			~Locdesc()		{}

	Locdesc &	clear();
	Locdesc &	add();
	Locdesc &	deref4();
	Locdesc &	reg( int );
	Locdesc &	basereg( int );
	Locdesc &	offset( long );
	Locdesc &	addr( Iaddr );

	Locdesc &	adjust( Iaddr );

	Addrexp		addrexp();
	int		size()	{ return vector.size();	}
	Locdesc &	operator=( Locdesc& );
	Locdesc &	operator=( Addrexp );

	Place		place( ProcObj *, Frame * );
	Place		place( ProcObj *, Frame *, Iaddr base );
};

#endif /* Locdesc_h */
