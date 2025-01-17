/*	Copyright (c) 1990, 1991, 1992, 1993, 1994 Novell, Inc. All Rights Reserved.	*/
/*	Copyright (c) 1988, 1990 Novell, Inc. All Rights Reserved.	*/
/*	  All Rights Reserved  	*/

/*	THIS IS UNPUBLISHED PROPRIETARY SOURCE CODE OF Novell Inc.	*/
/*	The copyright notice above does not evidence any   	*/
/*	actual or intended publication of such source code.	*/

#ident	"@(#)debugger:libsymbol/common/Reflist.h	1.1"
#ifndef Reflist_h
#define Reflist_h

#include	"Rbtree.h"

struct Attribute;

class Refnode : public Rbnode {
public:
	long			diskloc;
	Attribute *		nodeloc;
	friend class		Reflist;
				Refnode(long l, Attribute * s = 0)
				 { diskloc = l; nodeloc = s; }
				~Refnode() {}

	// virtual functions
	Rbnode *		makenode();
	int			cmp( Rbnode &t );
};

class Reflist : public Rbtree {
public:
				Reflist() {}
				~Reflist() {}
	int			lookup(long, Attribute * &);
	void			add(long, Attribute *);
};

#endif /* Reflist_h */
