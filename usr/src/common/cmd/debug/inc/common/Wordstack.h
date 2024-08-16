/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ifndef Wordstack_h
#define Wordstack_h
#ident	"@(#)debugger:inc/common/Wordstack.h	1.2"

#include	"Vector.h"

class Wordstack {
	Vector		vector;
	int		count;
public:
			Wordstack();
			~Wordstack()	{}
	void		push( unsigned long );
	unsigned long	pop();
	int		not_empty()	{	return count>0;	}
	void		clear()		{	vector.clear(); count = 0; }
};

#endif	/* Wordstack_h */
